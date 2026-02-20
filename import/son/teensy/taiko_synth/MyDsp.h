#pragma once

#include <Audio.h>
#include <Arduino.h>
#include "config.h"

// =======================================================
// MyDsp (FULL C++) : poly synth + ADSR + echo global
// API identique pour MidiRouter : noteOn/noteOff + setters
// =======================================================

class MyDsp : public AudioStream {
public:
  MyDsp();
  ~MyDsp();

  void update(void) override;

  // --- MIDI-driven controls ---
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
  // ---------- Synth helpers ----------
  static float midiToFreq(int note);

  // Simple shared sine table
  static constexpr int   kSineSize = 2048;
  static float           sSineTable[kSineSize];
  static bool            sSineInit;
  static void            initSineTable();

  // ✅ FIX: méthode membre (peut accéder aux private)
  float sineFromPhase(float phase01) const;

  // One pole LP for pad preset (per voice)
  struct OnePoleLP {
    float z = 0.0f;
    float a = 0.2f; // 0..1 (plus petit = plus filtré)
    inline float tick(float x) { z += a * (x - z); return z; }
  };

  enum EnvStage : uint8_t { OFF, ATTACK, DECAY, SUSTAIN, RELEASE };

  struct Voice {
    bool active = false;
    uint8_t note = 0;
    uint32_t age = 0;

    // Osc phase in [0,1)
    float phase = 0.0f;

    // Envelope
    EnvStage stage = OFF;
    float env = 0.0f;     // current envelope [0..1]
    float vel = 0.0f;     // velocity gain [0..1]

    // Extra for transient (preset 2)
    float transient = 0.0f;

    // Pad filter (preset 3)
    OnePoleLP lp;
  };

  Voice voices[kVoices];
  uint32_t ageCounter = 1;

  int findFreeVoice() const;
  int stealVoice() const;

  // ---------- Global params ----------
  int   preset = 0;
  float masterGain = 0.35f;

  bool  echoOn = false;
  float echoMix = 0.25f;
  float echoFb  = 0.45f;
  float echoMs  = 280.0f;

  // ADSR constants (tu pourras les rendre contrôlables plus tard)
  float atkS = 0.01f;
  float decS = 0.10f;
  float susL = 0.70f;
  float relS = 0.20f;

  // ---------- Echo (global) ----------
  static constexpr int kMaxEchoSamples = 36000; // ~0.816s @44.1kHz
  float* echoBuf = nullptr;
  int    echoLen = 12000;  // delay in samples (set from echoMs)
  int    echoIdx = 0;

  void   updateEchoLen();
  float  processEcho(float x);

  // ---------- utils ----------
  static inline float softClip(float x) { return tanhf(x); }
  static inline float fastRand01(); // [0..1]
};
