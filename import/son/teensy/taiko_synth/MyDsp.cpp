#include "MyDsp.h"
#include <math.h>

#define AUDIO_OUTPUTS 2
#define MULT_16 32767

float MyDsp::sSineTable[MyDsp::kSineSize];
bool  MyDsp::sSineInit = false;

static inline float clampf_local(float x, float lo, float hi) {
  return (x < lo) ? lo : (x > hi) ? hi : x;
}

void MyDsp::initSineTable() {
  if (sSineInit) return;
  for (int i = 0; i < kSineSize; i++) {
    sSineTable[i] = sinf(2.0f * (float)M_PI * (float)i / (float)kSineSize);
  }
  sSineInit = true;
}

float MyDsp::fastRand01() {
  // petit PRNG ultra simple (LCG)
  static uint32_t state = 0x12345678u;
  state = 1664525u * state + 1013904223u;
  // 24 bits -> [0..1)
  return (state >> 8) * (1.0f / 16777216.0f);
}

// ✅ FIX: méthode membre (accès aux private OK)
float MyDsp::sineFromPhase(float phase01) const {
  // phase01 in [0,1)
  int idx = (int)(phase01 * (float)kSineSize) & (kSineSize - 1);
  return sSineTable[idx];
}

MyDsp::MyDsp()
: AudioStream(0, NULL)
{
  initSineTable();

  // echo buffer mono
  echoBuf = new float[kMaxEchoSamples];
  for (int i = 0; i < kMaxEchoSamples; i++) echoBuf[i] = 0.0f;
  updateEchoLen();

  for (int i = 0; i < kVoices; i++) {
    voices[i] = Voice{};
  }
}

MyDsp::~MyDsp() {
  delete[] echoBuf;
}

float MyDsp::midiToFreq(int note) {
  return 440.0f * powf(2.0f, (note - 69) / 12.0f);
}

int MyDsp::findFreeVoice() const {
  for (int i = 0; i < kVoices; i++) {
    if (!voices[i].active) return i;
  }
  return -1;
}

int MyDsp::stealVoice() const {
  // vole la plus vieille (plus petit "age" = plus vieille dans ton code)
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
  v.note   = note;
  v.age    = ageCounter++;

  v.phase  = 0.0f;

  v.vel = clampf_local(vel / 127.0f, 0.0f, 1.0f);

  // envelope restart
  v.stage = ATTACK;
  v.env   = 0.0f;

  // transient for preset 2
  v.transient = 1.0f;

  // reset pad filter
  v.lp.z = 0.0f;
  v.lp.a = 0.12f;
}

void MyDsp::noteOff(uint8_t note) {
  for (int i = 0; i < kVoices; i++) {
    if (voices[i].active && voices[i].note == note) {
      voices[i].stage = RELEASE;
    }
  }
}

void MyDsp::setPreset(int p) {
  preset = (p < 0) ? 0 : (p > 3) ? 3 : p;
}

void MyDsp::setMasterGain(float g) {
  masterGain = clampf_local(g, 0.0f, 1.0f);
}

void MyDsp::setEchoOn(bool on) { echoOn = on; }
void MyDsp::setEchoMix(float mix) { echoMix = clampf_local(mix, 0.0f, 1.0f); }
void MyDsp::setEchoFb(float fb) { echoFb = clampf_local(fb, 0.0f, 0.85f); }
void MyDsp::setEchoMs(float ms) {
  echoMs = clampf_local(ms, 30.0f, 800.0f);
  updateEchoLen();
}

void MyDsp::updateEchoLen() {
  // convert ms -> samples, clamp into buffer
  float sr = AUDIO_SAMPLE_RATE_EXACT;
  int d = (int)(echoMs * sr / 1000.0f);
  d = (d < 1) ? 1 : d;
  if (d > kMaxEchoSamples - 1) d = kMaxEchoSamples - 1;
  echoLen = d;
  if (echoIdx >= echoLen) echoIdx = 0;
}

float MyDsp::processEcho(float x) {
  // mono echo : y = x + fb*y[n-D] with ring buffer storing "y"
  float y = x;
  if (echoOn) {
    float delayed = echoBuf[echoIdx];
    y = x + delayed * echoFb;
    echoBuf[echoIdx] = y;
    echoIdx++;
    if (echoIdx >= echoLen) echoIdx = 0;

    // wet/dry
    y = (1.0f - echoMix) * x + echoMix * y;
  }
  return y;
}

void MyDsp::allNotesOff() {
  for (int i = 0; i < kVoices; i++) {
    if (voices[i].active) {
      voices[i].stage = OFF;
      voices[i].env = 0.0f;
      voices[i].active = false;
    }
  }
}


void MyDsp::update(void) {
  // mix mono (stéréo identique)
  audio_block_t* outBlock[AUDIO_OUTPUTS];
  for (int ch = 0; ch < AUDIO_OUTPUTS; ch++) {
    outBlock[ch] = allocate();
    if (!outBlock[ch]) return;
  }

  const float sr = AUDIO_SAMPLE_RATE_EXACT;
  const float invVoices = 1.0f / sqrtf((float)kVoices);

  // precompute ADSR increments per sample
  const float atkInc = (atkS <= 0.0001f) ? 1.0f : (1.0f / (atkS * sr));
  const float decInc = (decS <= 0.0001f) ? 1.0f : ((1.0f - susL) / (decS * sr));

  for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
    float mix = 0.0f;

    for (int v = 0; v < kVoices; v++) {
      Voice& voice = voices[v];
      if (!voice.active) continue;

      // --- Envelope step ---
      switch (voice.stage) {
        case ATTACK:
          voice.env += atkInc;
          if (voice.env >= 1.0f) { voice.env = 1.0f; voice.stage = DECAY; }
          break;

        case DECAY:
          voice.env -= decInc;
          if (voice.env <= susL) { voice.env = susL; voice.stage = SUSTAIN; }
          break;

        case SUSTAIN:
          // hold
          break;

        case RELEASE:
          voice.env -= (relS <= 0.0001f) ? 1.0f : (1.0f / (relS * sr));
          if (voice.env <= 0.0f) {
            voice.env = 0.0f;
            voice.stage = OFF;
            voice.active = false;
          }
          break;

        case OFF:
        default:
          voice.active = false;
          break;
      }
      if (!voice.active) continue;

      float f = midiToFreq(voice.note);
      float phaseInc = f / sr;
      voice.phase += phaseInc;
      voice.phase -= floorf(voice.phase);

      float s = 0.0f;

      // --- Presets ---
      if (preset == 0) {
        // sine
        s = sineFromPhase(voice.phase);

      } else if (preset == 1) {
        // additive: f + 2f + 3f + 1.5f
        float p = voice.phase;
        float s1 = sineFromPhase(p);
        float s2 = sineFromPhase(fmodf(p * 2.0f, 1.0f));
        float s3 = sineFromPhase(fmodf(p * 3.0f, 1.0f));
        float s4 = sineFromPhase(fmodf(p * 1.5f, 1.0f));
        s = s1 + 0.50f*s2 + 0.30f*s3 + 0.20f*s4;

      } else if (preset == 2) {
        // "electric": sine + harmonics + short noise transient
        float p = voice.phase;
        float base = sineFromPhase(p)
                   + 0.35f * sineFromPhase(fmodf(p * 2.0f, 1.0f))
                   + 0.15f * sineFromPhase(fmodf(p * 4.0f, 1.0f));

        // transient decays fast
        voice.transient *= 0.9992f;
        float noise = (fastRand01() * 2.0f - 1.0f) * 0.15f * voice.transient;

        s = base + noise;

      } else {
        // preset 3: pad (2 detuned sines + lowpass)
        float det = 0.004f;
        float p = voice.phase;
        float sA = sineFromPhase(fmodf(p * (1.0f - det), 1.0f));
        float sB = sineFromPhase(fmodf(p * (1.0f + det), 1.0f));
        float raw = 0.6f * sA + 0.6f * sB;
        s = voice.lp.tick(raw);
      }

      // Apply per-voice envelope + velocity
      s *= voice.env * voice.vel;

      mix += s;
    }

    // normalize for chords, then master gain
    float x = mix * invVoices * masterGain;

    // global echo + safety
    x = processEcho(x);
    x = softClip(x);
    x = fmaxf(-1.0f, fminf(1.0f, x));

    int16_t out = (int16_t)(x * MULT_16);
    outBlock[0]->data[i] = out;
    outBlock[1]->data[i] = out;
  }

  transmit(outBlock[0], 0);
  transmit(outBlock[1], 1);
  release(outBlock[0]);
  release(outBlock[1]);
}
