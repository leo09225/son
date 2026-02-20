// ============================================================
// MyDsp.cpp -- Polyphonic synthesiser engine implementation
//
// Generates audio sample-by-sample inside update(), which is
// called from the Teensy Audio ISR.  Each voice runs an
// oscillator through an ADSR envelope, and all voices are
// summed then passed through a global echo effect.
// ============================================================

#include "MyDsp.h"
#include <math.h>

static constexpr int   AUDIO_OUTPUTS = 2;
static constexpr int16_t MULT_16     = 32767;

// ---------- Static member initialisation -----------------------
float MyDsp::sSineTable[MyDsp::kSineSize];
bool  MyDsp::sSineInit = false;

// ---------- Sine wavetable -------------------------------------

void MyDsp::initSineTable() {
  if (sSineInit) return;
  for (int i = 0; i < kSineSize; i++) {
    sSineTable[i] = sinf(2.0f * (float)M_PI * (float)i / (float)kSineSize);
  }
  sSineInit = true;
}

/// Look up the sine table with a normalised phase [0, 1).
/// Uses a bitmask for safe wrapping (kSineSize must be power of 2).
float MyDsp::sineFromPhase(float phase01) const {
  int idx = (int)(phase01 * (float)kSineSize) & (kSineSize - 1);
  return sSineTable[idx];
}

// ---------- Simple PRNG for noise ------------------------------

float MyDsp::fastRand01() {
  // Linear congruential generator -- fast, no state beyond one uint32_t.
  static uint32_t state = 0x12345678u;
  state = 1664525u * state + 1013904223u;
  return (state >> 8) * (1.0f / 16777216.0f);   // 24-bit -> [0, 1)
}

// ---------- Constructor / Destructor ---------------------------

MyDsp::MyDsp()
  : AudioStream(0, NULL)
{
  initSineTable();

  // Allocate the mono echo ring buffer and zero it out.
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

// ---------- Helpers --------------------------------------------

/// Convert MIDI note number to frequency using equal temperament.
/// A4 (note 69) = 440 Hz.
float MyDsp::midiToFreq(int note) {
  return 440.0f * powf(2.0f, (note - 69) / 12.0f);
}

/// Return the index of the first inactive voice, or -1 if none.
int MyDsp::findFreeVoice() const {
  for (int i = 0; i < kVoices; i++) {
    if (!voices[i].active) return i;
  }
  return -1;
}

/// Voice stealing: return the index of the oldest active voice
/// (the one with the smallest age counter).
int MyDsp::stealVoice() const {
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

// ---------- MIDI-driven controls (loop context) ----------------
// All public methods that modify shared state disable interrupts
// briefly to prevent data races with update() which runs in the
// audio ISR.

void MyDsp::noteOn(uint8_t note, uint8_t vel) {
  __disable_irq();

  int idx = findFreeVoice();
  if (idx < 0) idx = stealVoice();

  Voice& v  = voices[idx];
  v.active   = true;
  v.note     = note;
  v.age      = ageCounter++;
  v.phase    = 0.0f;
  v.phaseInc = midiToFreq(note) / AUDIO_SAMPLE_RATE_EXACT;  // cache once
  v.vel      = clampf(vel / 127.0f, 0.0f, 1.0f);

  // Restart ADSR envelope from the beginning
  v.stage = ATTACK;
  v.env   = 0.0f;

  // Reset per-preset state
  v.transient = 1.0f;     // noise burst for "electric" preset
  v.lp.z = 0.0f;          // low-pass filter for "pad" preset
  v.lp.a = 0.12f;

  __enable_irq();
}

void MyDsp::noteOff(uint8_t note) {
  __disable_irq();
  for (int i = 0; i < kVoices; i++) {
    if (voices[i].active && voices[i].note == note) {
      voices[i].stage = RELEASE;
    }
  }
  __enable_irq();
}

void MyDsp::setPreset(int p) {
  __disable_irq();
  preset = (p < 0) ? 0 : (p > 3) ? 3 : p;
  __enable_irq();
}

void MyDsp::setMasterGain(float g) {
  __disable_irq();
  masterGain = clampf(g, 0.0f, 1.0f);
  __enable_irq();
}

void MyDsp::setEchoOn(bool on) {
  __disable_irq();
  echoOn = on;
  __enable_irq();
}

void MyDsp::setEchoMix(float mix) {
  __disable_irq();
  echoMix = clampf(mix, 0.0f, 1.0f);
  __enable_irq();
}

void MyDsp::setEchoFb(float fb) {
  __disable_irq();
  echoFb = clampf(fb, 0.0f, 0.85f);
  __enable_irq();
}

void MyDsp::setEchoMs(float ms) {
  __disable_irq();
  echoMs = clampf(ms, 30.0f, 800.0f);
  updateEchoLen();
  __enable_irq();
}

void MyDsp::allNotesOff() {
  __disable_irq();
  for (int i = 0; i < kVoices; i++) {
    if (voices[i].active) {
      voices[i].stage  = OFF;
      voices[i].env    = 0.0f;
      voices[i].active = false;
    }
  }
  __enable_irq();
}

// ---------- Echo -----------------------------------------------

/// Convert echoMs to a sample count and clamp to buffer size.
void MyDsp::updateEchoLen() {
  float sr = AUDIO_SAMPLE_RATE_EXACT;
  int d = (int)(echoMs * sr / 1000.0f);
  d = (d < 1) ? 1 : d;
  if (d > kMaxEchoSamples - 1) d = kMaxEchoSamples - 1;
  echoLen = d;
  if (echoIdx >= echoLen) echoIdx = 0;
}

/// Process one sample through the mono echo.
/// Uses a ring buffer: y[n] = x[n] + fb * y[n - D],
/// then mixes wet/dry.
float MyDsp::processEcho(float x) {
  float y = x;
  if (echoOn) {
    float delayed = echoBuf[echoIdx];
    y = x + delayed * echoFb;
    echoBuf[echoIdx] = y;
    echoIdx++;
    if (echoIdx >= echoLen) echoIdx = 0;

    // Wet/dry mix
    y = (1.0f - echoMix) * x + echoMix * y;
  }
  return y;
}

// ---------- Audio block generation (ISR context) ---------------

void MyDsp::update(void) {
  // Allocate two output blocks (left + right).
  // If the second allocation fails, release the first to avoid leaking.
  audio_block_t* outBlock[AUDIO_OUTPUTS];
  outBlock[0] = allocate();
  if (!outBlock[0]) return;
  outBlock[1] = allocate();
  if (!outBlock[1]) {
    release(outBlock[0]);
    return;
  }

  const float sr = AUDIO_SAMPLE_RATE_EXACT;

  // Normalisation factor so chords don't clip.
  // Using 1/sqrt(N) keeps perceived loudness roughly constant.
  static constexpr float invVoices = 1.0f / 2.828427f;  // 1/sqrt(8)

  // Pre-compute ADSR envelope increments (per sample).
  const float atkInc = (atkS <= 0.0001f) ? 1.0f : (1.0f / (atkS * sr));
  const float decInc = (decS <= 0.0001f) ? 1.0f : ((1.0f - susL) / (decS * sr));
  const float relInc = (relS <= 0.0001f) ? 1.0f : (1.0f / (relS * sr));

  // --- Fill the output buffer sample by sample -----------------
  for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
    float mix = 0.0f;

    // --- Sum all active voices ---------------------------------
    for (int v = 0; v < kVoices; v++) {
      Voice& voice = voices[v];
      if (!voice.active) continue;

      // --- ADSR envelope step ---
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
          break;   // hold at sustain level

        case RELEASE:
          voice.env -= relInc;
          if (voice.env <= 0.0f) {
            voice.env    = 0.0f;
            voice.stage  = OFF;
            voice.active = false;
          }
          break;

        case OFF:
        default:
          voice.active = false;
          break;
      }
      if (!voice.active) continue;

      // --- Oscillator phase advance ---
      // phaseInc was cached in noteOn() to avoid calling powf() every sample.
      voice.phase += voice.phaseInc;
      if (voice.phase >= 1.0f) voice.phase -= 1.0f;

      // --- Waveform generation (preset-dependent) ---
      float s = 0.0f;

      if (preset == 0) {
        // Preset 0: Pure sine wave
        s = sineFromPhase(voice.phase);

      } else if (preset == 1) {
        // Preset 1: Additive (organ/bell) — fundamental + 3 harmonics
        float p = voice.phase;
        float s1 = sineFromPhase(p);
        float s2 = sineFromPhase(fmodf(p * 2.0f, 1.0f));
        float s3 = sineFromPhase(fmodf(p * 3.0f, 1.0f));
        float s4 = sineFromPhase(fmodf(p * 1.5f, 1.0f));
        s = s1 + 0.50f * s2 + 0.30f * s3 + 0.20f * s4;

      } else if (preset == 2) {
        // Preset 2: Electric — harmonics + decaying noise transient
        float p = voice.phase;
        float base = sineFromPhase(p)
                   + 0.35f * sineFromPhase(fmodf(p * 2.0f, 1.0f))
                   + 0.15f * sineFromPhase(fmodf(p * 4.0f, 1.0f));

        voice.transient *= 0.9992f;   // fast exponential decay
        float noise = (fastRand01() * 2.0f - 1.0f) * 0.15f * voice.transient;
        s = base + noise;

      } else {
        // Preset 3: Pad — two detuned sines through a low-pass filter
        float det = 0.004f;
        float p = voice.phase;
        float sA = sineFromPhase(fmodf(p * (1.0f - det), 1.0f));
        float sB = sineFromPhase(fmodf(p * (1.0f + det), 1.0f));
        float raw = 0.6f * sA + 0.6f * sB;
        s = voice.lp.tick(raw);
      }

      // Apply per-voice envelope and velocity scaling
      s *= voice.env * voice.vel;
      mix += s;
    }

    // --- Master processing -------------------------------------
    // Normalise for polyphony, apply master gain
    float x = mix * invVoices * masterGain;

    // Global echo, soft clipping, and hard safety limiter
    x = processEcho(x);
    x = softClip(x);
    x = fmaxf(-1.0f, fminf(1.0f, x));

    // Convert to 16-bit and write to both channels (mono output)
    int16_t out = (int16_t)(x * MULT_16);
    outBlock[0]->data[i] = out;
    outBlock[1]->data[i] = out;
  }

  // Send the completed blocks downstream and release them
  transmit(outBlock[0], 0);
  transmit(outBlock[1], 1);
  release(outBlock[0]);
  release(outBlock[1]);
}
