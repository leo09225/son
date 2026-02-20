#pragma once
// ============================================================
// MyDsp.h -- Polyphonic synthesiser engine
//
// Inherits from AudioStream (Teensy Audio Library) to produce
// real-time audio.  Features:
//   - 8-voice polyphony with oldest-voice stealing
//   - ADSR envelope per voice
//   - 4 timbres (presets): sine, additive, electric, pad
//   - Global mono echo effect (ring-buffer delay)
//
// IMPORTANT: update() runs inside the audio ISR at ~345 Hz.
// All public setters are called from loop() (main thread) and
// are protected with __disable_irq() / __enable_irq() to
// avoid data races with the ISR.
// ============================================================

#include <Audio.h>
#include <Arduino.h>
#include "config.h"

class MyDsp : public AudioStream {
public:
  MyDsp();
  ~MyDsp();

  /// Called automatically by the Teensy Audio Library inside the
  /// audio ISR (~345 times/sec).  Fills one block of 128 stereo
  /// samples by mixing all active voices through the echo effect.
  void update(void) override;

  // --- MIDI-driven controls (called from loop context) ---------
  void noteOn(uint8_t note, uint8_t vel);
  void noteOff(uint8_t note);

  void setPreset(int p);            // 0..3
  void setMasterGain(float g);      // 0..1

  void setEchoOn(bool on);
  void setEchoMix(float mix);       // 0..1
  void setEchoFb(float fb);         // 0..0.85
  void setEchoMs(float ms);         // 30..800
  void allNotesOff();

private:
  // ---------- Synth helpers ------------------------------------

  /// Convert a MIDI note number to a frequency in Hz.
  static float midiToFreq(int note);

  // Shared sine wavetable (2048 samples, one period).
  // Initialised lazily on first MyDsp construction.
  static constexpr int kSineSize = 2048;
  static float         sSineTable[kSineSize];
  static bool          sSineInit;
  static void          initSineTable();

  /// Look up the sine table using a normalised phase in [0, 1).
  float sineFromPhase(float phase01) const;

  // ---------- Per-voice structures -----------------------------

  /// Simple one-pole low-pass filter used by the "pad" preset.
  struct OnePoleLP {
    float z = 0.0f;
    float a = 0.2f;   // coefficient in 0..1 (smaller = more filtered)
    inline float tick(float x) { z += a * (x - z); return z; }
  };

  /// ADSR envelope stages.
  enum EnvStage : uint8_t { OFF, ATTACK, DECAY, SUSTAIN, RELEASE };

  /// State for a single polyphonic voice.
  struct Voice {
    bool     active   = false;
    uint8_t  note     = 0;
    uint32_t age      = 0;       // used by voice-stealing (oldest = smallest)

    float phase    = 0.0f;       // oscillator phase in [0, 1)
    float phaseInc = 0.0f;       // phase increment per sample (cached from midiToFreq)

    EnvStage stage = OFF;
    float env      = 0.0f;       // current envelope level [0..1]
    float vel      = 0.0f;       // velocity-based gain   [0..1]

    float transient = 0.0f;      // noise burst for "electric" preset
    OnePoleLP lp;                // filter for "pad" preset
  };

  Voice    voices[kVoices];
  uint32_t ageCounter = 1;

  /// Find the first inactive voice, or -1 if all are busy.
  int findFreeVoice() const;
  /// Steal the oldest active voice (smallest age value).
  int stealVoice() const;

  // ---------- Global parameters --------------------------------
  int   preset     = 0;
  float masterGain = 0.35f;

  // Echo parameters
  bool  echoOn  = false;
  float echoMix = 0.25f;        // wet/dry balance
  float echoFb  = 0.45f;        // feedback amount
  float echoMs  = 280.0f;       // delay time in milliseconds

  // ADSR timing (could be made controllable via CC later)
  float atkS = 0.01f;           // attack  time in seconds
  float decS = 0.10f;           // decay   time in seconds
  float susL = 0.70f;           // sustain level [0..1]
  float relS = 0.20f;           // release time in seconds

  // ---------- Echo ring buffer ---------------------------------
  static constexpr int kMaxEchoSamples = 36000;  // ~0.816 s @ 44.1 kHz
  float* echoBuf = nullptr;
  int    echoLen = 12000;        // current delay length in samples
  int    echoIdx = 0;            // write/read head position

  void  updateEchoLen();
  float processEcho(float x);

  // ---------- Utilities ----------------------------------------
  static inline float softClip(float x) { return tanhf(x); }
  static inline float fastRand01();      // returns [0..1)
};
