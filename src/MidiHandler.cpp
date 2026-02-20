// ============================================================
// MidiHandler.cpp -- USB MIDI message routing implementation
//
// Routes each incoming MIDI message to the appropriate target:
//
//   NoteOn / NoteOff   → live synth always
//                      → looper (if recording, via Looper class)
//   ProgramChange      → live synth preset + looper preset tracking
//   ControlChange      → both synths (volume, echo parameters)
//
// Uses the CC constants from config.h rather than magic numbers.
// ============================================================

#include "MidiHandler.h"
#include "MyDsp.h"
#include "Looper.h"
#include "config.h"
#include <Arduino.h>

// File-static pointers set by begin().  This avoids globals
// while keeping the API simple (no need to pass objects every call).
static MyDsp*  sLive   = nullptr;
static MyDsp*  sLooper = nullptr;
static Looper* sLoop   = nullptr;

/// Convert a 7-bit MIDI CC value (0..127) to a float in [0, 1].
static inline float ccTo01(uint8_t v) {
  return (float)v / 127.0f;
}

// --- Public API ------------------------------------------------

void MidiHandler::begin(MyDsp& liveSynth, MyDsp& looperSynth, Looper& looper) {
  sLive   = &liveSynth;
  sLooper = &looperSynth;
  sLoop   = &looper;

  usbMIDI.begin();
  Serial.println("[MIDI] USB MIDI started");
}

void MidiHandler::process() {
  uint8_t type = usbMIDI.getType();

  // ---- NoteOn -------------------------------------------------
  if (type == usbMIDI.NoteOn) {
    uint8_t note = usbMIDI.getData1();
    uint8_t vel  = usbMIDI.getData2();

    Serial.print("[MIDI] NoteON: note=");
    Serial.print(note);
    Serial.print(" vel=");
    Serial.println(vel);

    if (vel > 0) {
      sLive->noteOn(note, vel);
      sLoop->recordNoteOn(note, vel);   // no-op if not recording
    } else {
      // NoteOn with velocity 0 is equivalent to NoteOff (MIDI spec)
      sLive->noteOff(note);
      sLoop->recordNoteOff(note);
    }
  }

  // ---- NoteOff ------------------------------------------------
  else if (type == usbMIDI.NoteOff) {
    uint8_t note = usbMIDI.getData1();

    Serial.print("[MIDI] NoteOFF: note=");
    Serial.println(note);

    sLive->noteOff(note);
    sLoop->recordNoteOff(note);
  }

  // ---- Program Change -----------------------------------------
  else if (type == usbMIDI.ProgramChange) {
    uint8_t pgm = usbMIDI.getData1();
    int preset = pgm % 4;   // wrap to 0..3

    sLive->setPreset(preset);
    sLoop->setLivePreset(preset);

    Serial.print("[MIDI] Program Change -> preset: ");
    Serial.println(preset);
  }

  // ---- Control Change -----------------------------------------
  else if (type == usbMIDI.ControlChange) {
    uint8_t cc  = usbMIDI.getData1();
    uint8_t val = usbMIDI.getData2();

    // Apply CC to BOTH synths (live + looper) so they share
    // the same effect settings and volume.

    if (cc == CC_MASTER_VOL) {
      float gain = ccTo01(val);
      sLive->setMasterGain(gain);
      sLooper->setMasterGain(gain);
    }
    else if (cc == CC_ECHO_ON) {
      bool on = val >= 64;
      sLive->setEchoOn(on);
      sLooper->setEchoOn(on);
    }
    else if (cc == CC_ECHO_MIX) {
      float mix = ccTo01(val);
      sLive->setEchoMix(mix);
      sLooper->setEchoMix(mix);
    }
    else if (cc == CC_ECHO_FB) {
      float fb = 0.85f * ccTo01(val);   // scale to max 0.85
      sLive->setEchoFb(fb);
      sLooper->setEchoFb(fb);
    }
    else if (cc == CC_ECHO_MS) {
      float ms = 30.0f + (770.0f * ccTo01(val));   // 30..800 ms
      sLive->setEchoMs(ms);
      sLooper->setEchoMs(ms);
    }
  }
}
