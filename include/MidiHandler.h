#pragma once
// ============================================================
// MidiHandler.h -- USB MIDI message routing
//
// Reads incoming USB MIDI messages and dispatches them:
//   - NoteOn / NoteOff  → live synth  (+ looper if recording)
//   - ProgramChange     → preset selection
//   - ControlChange     → volume, echo parameters
//
// Implemented as a namespace with free functions rather than a
// class, because there is no meaningful per-instance state --
// the handler simply routes messages between existing objects.
// ============================================================

class MyDsp;
class Looper;

namespace MidiHandler {

/// Register the synth and looper instances.  Call once in setup().
void begin(MyDsp& liveSynth, MyDsp& looperSynth, Looper& looper);

/// Process one pending MIDI message.
/// Call inside: while (usbMIDI.read()) { MidiHandler::process(); }
void process();

}  // namespace MidiHandler
