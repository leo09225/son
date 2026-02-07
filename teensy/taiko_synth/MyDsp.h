#pragma once

#include <Audio.h>
#include "config.h"

// Le header généré par Faust (copié dans ce dossier)
#include "TaikoSynth.h"
#include "faust/gui/MapUI.h"

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

private:
  struct Voice {
    dsp*   dspObj = nullptr;
    MapUI* ui     = nullptr;
    float* outL   = nullptr;
    float* outR   = nullptr;

    bool   active = false;
    uint8_t note  = 0;
    uint32_t age  = 0;   // pour voice stealing
  };

  Voice voices[kVoices];
  uint32_t ageCounter = 1;

  // Paramètres globaux (appliqués à toutes les voix)
  int   preset = 0;
  float masterGain = 0.35f;
  bool  echoOn = false;
  float echoMix = 0.25f;
  float echoFb  = 0.45f;
  float echoMs  = 280.0f;

  // Helpers
  int findFreeVoice() const;
  int stealVoice() const;
  void applyGlobals(Voice& v);
  void setVoiceParam(Voice& v, const char* path, float val);

  static float midiToFreq(int note);
};
