#include "MyDsp.h"
#include <Arduino.h>
#include <math.h>

#define AUDIO_OUTPUTS 2
#define MULT_16 32767

static inline float softClip(float x) {
  // petit soft clip sécurité (en plus de celui dans Faust)
  return tanhf(x);
}

MyDsp::MyDsp()
: AudioStream(0, NULL)
{
  for (int i = 0; i < kVoices; i++) {
    voices[i].dspObj = new mydsp();
    voices[i].dspObj->init(AUDIO_SAMPLE_RATE_EXACT);

    voices[i].ui = new MapUI();
    voices[i].dspObj->buildUserInterface(voices[i].ui);

    voices[i].outL = new float[AUDIO_BLOCK_SAMPLES];
    voices[i].outR = new float[AUDIO_BLOCK_SAMPLES];

    voices[i].active = false;
    voices[i].note = 0;
    voices[i].age = 0;

    applyGlobals(voices[i]);
    // état initial
    setVoiceParam(voices[i], "gate", 0.0f);
    setVoiceParam(voices[i], "gain", 0.0f);
    setVoiceParam(voices[i], "freq", 200.0f);
  }
}

MyDsp::~MyDsp() {
  for (int i = 0; i < kVoices; i++) {
    delete[] voices[i].outL;
    delete[] voices[i].outR;
    delete voices[i].dspObj;
    delete voices[i].ui;
  }
}

float MyDsp::midiToFreq(int note) {
  return 440.0f * powf(2.0f, (note - 69) / 12.0f);
}

void MyDsp::setVoiceParam(Voice& v, const char* path, float val) {
  v.ui->setParamValue(path, val);
}

void MyDsp::applyGlobals(Voice& v) {
  setVoiceParam(v, "preset", (float)preset);
  setVoiceParam(v, "masterGain", masterGain);

  setVoiceParam(v, "fx/echoOn", echoOn ? 1.0f : 0.0f);
  setVoiceParam(v, "fx/echoMix", echoMix);
  setVoiceParam(v, "fx/echoFb",  echoFb);
  setVoiceParam(v, "fx/echoMs",  echoMs);
}

int MyDsp::findFreeVoice() const {
  for (int i = 0; i < kVoices; i++) {
    if (!voices[i].active) return i;
  }
  return -1;
}

int MyDsp::stealVoice() const {
  // vole la plus vieille (age minimal non-zero)
  int best = 0;
  uint32_t bestAge = voices[0].age;
  for (int i = 1; i < kVoices; i++) {
    if (voices[i].age < bestAge) {
      bestAge = voices[i].age;
      best = i;
    }
  }
  return best;
}

void MyDsp::noteOn(uint8_t note, uint8_t vel) {
  int idx = findFreeVoice();
  if (idx < 0) idx = stealVoice();

  Voice& v = voices[idx];
  v.active = true;
  v.note = note;
  v.age = ageCounter++;

  applyGlobals(v);

  float f = midiToFreq(note);
  float g = clampf(vel / 127.0f, 0.0f, 1.0f);

  setVoiceParam(v, "freq", f);
  setVoiceParam(v, "gain", g);
  setVoiceParam(v, "gate", 1.0f);
}

void MyDsp::noteOff(uint8_t note) {
  // relâche toutes les voix portant cette note (au cas où)
  for (int i = 0; i < kVoices; i++) {
    if (voices[i].active && voices[i].note == note) {
      setVoiceParam(voices[i], "gate", 0.0f);
      // On ne met pas active=false immédiatement :
      // on laisse la release d’enveloppe se faire côté Faust.
      // (On “libère” la voix après un petit temps via un hack simple : ici on la libère direct si rel très court)
      // Version 1 : on libère direct (simple). Améliorable ensuite.
      voices[i].active = false;
    }
  }
}

void MyDsp::setPreset(int p) {
  preset = (p < 0) ? 0 : (p > 3) ? 3 : p;
  for (int i = 0; i < kVoices; i++) applyGlobals(voices[i]);
}

void MyDsp::setMasterGain(float g) {
  masterGain = clampf(g, 0.0f, 1.0f);
  for (int i = 0; i < kVoices; i++) applyGlobals(voices[i]);
}

void MyDsp::setEchoOn(bool on) {
  echoOn = on;
  for (int i = 0; i < kVoices; i++) applyGlobals(voices[i]);
}
void MyDsp::setEchoMix(float mix) {
  echoMix = clampf(mix, 0.0f, 1.0f);
  for (int i = 0; i < kVoices; i++) applyGlobals(voices[i]);
}
void MyDsp::setEchoFb(float fb) {
  echoFb = clampf(fb, 0.0f, 0.85f);
  for (int i = 0; i < kVoices; i++) applyGlobals(voices[i]);
}
void MyDsp::setEchoMs(float ms) {
  echoMs = clampf(ms, 30.0f, 800.0f);
  for (int i = 0; i < kVoices; i++) applyGlobals(voices[i]);
}

void MyDsp::update(void) {
  // Buffers de mix (float)
  static float mixL[AUDIO_BLOCK_SAMPLES];
  static float mixR[AUDIO_BLOCK_SAMPLES];

  for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
    mixL[i] = 0.0f;
    mixR[i] = 0.0f;
  }

  // Compute + mix toutes les voix
  for (int v = 0; v < kVoices; v++) {
    // compute attend outputs[2], on fabrique un tableau de pointeurs
    float* outputs[2] = { voices[v].outL, voices[v].outR };
    voices[v].dspObj->compute(AUDIO_BLOCK_SAMPLES, nullptr, outputs);

    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
      mixL[i] += voices[v].outL[i];
      mixR[i] += voices[v].outR[i];
    }
  }

  // Normalisation simple pour éviter saturation quand accords
  const float norm = 1.0f / sqrtf((float)kVoices);

  audio_block_t* outBlock[AUDIO_OUTPUTS];
  for (int ch = 0; ch < AUDIO_OUTPUTS; ch++) {
    outBlock[ch] = allocate();
    if (!outBlock[ch]) continue;

    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
      float s = (ch == 0 ? mixL[i] : mixR[i]) * norm;
      s = softClip(s);
      s = fmaxf(-1.0f, fminf(1.0f, s));
      outBlock[ch]->data[i] = (int16_t)(s * MULT_16);
    }

    transmit(outBlock[ch], ch);
    release(outBlock[ch]);
  }
}
