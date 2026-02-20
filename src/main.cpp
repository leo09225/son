// ============================================================
// main.cpp -- Entry point for the Synteensyzer
//
// This file is intentionally short.  It only does three things:
//   1. Declares the Teensy Audio graph (synths, mixer, output)
//   2. Initialises hardware in setup()
//   3. Runs the main loop (button → MIDI → looper playback)
//
// All logic lives in dedicated modules:
//   MyDsp         → polyphonic synth engine
//   Looper        → record / play / stop state machine
//   DebouncedButton → hardware button with debounce
//   MidiHandler   → USB MIDI message routing
// ============================================================

#include <Arduino.h>
#include <Audio.h>
#include "config.h"
#include "MyDsp.h"
#include "Looper.h"
#include "MidiHandler.h"
#include "Button.h"

// === Audio graph ===============================================
// Two synth instances (live + looper) are mixed to stereo
// output through the SGTL5000 codec on the Audio Shield.
//
//   liveSynth ──L/R──┐
//                     ├── mixerL/R ──► AudioOutputI2S ──► headphones
//   looperSynth ─L/R─┘

MyDsp liveSynth;
MyDsp looperSynth;

AudioMixer4          mixerL;
AudioMixer4          mixerR;
AudioOutputI2S       audioOut;
AudioControlSGTL5000 codec;

// Patch cables: each synth feeds both L and R mixer inputs
AudioConnection patchLiveL (liveSynth,   0, mixerL,   0);
AudioConnection patchLiveR (liveSynth,   1, mixerR,   0);
AudioConnection patchLoopL (looperSynth, 0, mixerL,   1);
AudioConnection patchLoopR (looperSynth, 1, mixerR,   1);
AudioConnection patchOutL  (mixerL,      0, audioOut,  0);
AudioConnection patchOutR  (mixerR,      0, audioOut,  1);

// === Modules ===================================================

Looper looper(liveSynth, looperSynth);

// Button callbacks (simple wrappers forwarding to the looper)
static void onShortPress() { looper.onShortPress(); }
static void onLongPress()  { looper.onLongPress();  }

DebouncedButton loopButton(kLoopButtonPin, onShortPress, onLongPress);

// === setup =====================================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== SYNTEENSYZER (DUAL SYNTH + LOOPER) ===");

  // Allocate audio memory blocks (enough for two synth engines)
  AudioMemory(60);
  codec.enable();
  codec.volume(0.5);

  // Mix live and looper at equal gain
  mixerL.gain(0, 0.5);   // live  left
  mixerL.gain(1, 0.5);   // loop  left
  mixerR.gain(0, 0.5);   // live  right
  mixerR.gain(1, 0.5);   // loop  right
  Serial.println("Audio initialised (dual synth + mixer)");

  loopButton.begin();
  MidiHandler::begin(liveSynth, looperSynth, looper);

  // Both synths start on preset 0
  liveSynth.setPreset(0);
  looperSynth.setPreset(0);

  Serial.println("Ready!\n");
}

// === loop ======================================================

void loop() {
  loopButton.update();                   // 1. Read the physical button

  while (usbMIDI.read()) {              // 2. Process all pending MIDI messages
    MidiHandler::process();
  }

  looper.tick();                         // 3. Advance looper playback
}
