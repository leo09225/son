#pragma once
// ============================================================
// Looper.h -- MIDI event looper with record / play / stop
//
// Records NoteOn/NoteOff events with timestamps into a fixed
// buffer, then plays them back in a continuous loop.
//
// The synthesiser preset active at recording start is "frozen"
// so the loop always sounds the same regardless of later preset
// changes on the live synth.
//
// State machine (transitions via onShortPress / onLongPress):
//
//         short press        short press       short press
// EMPTY ──────────────► RECORDING ──────────► PLAYING ──────────► STOPPED
//   ▲                                            │                   │
//   │              long press (3s)                │   short press     │
//   └────────────────────────────────────────────┴───────────────────┘
//                         CLEAR
// ============================================================

#include <Arduino.h>
#include "config.h"

class MyDsp;   // forward declaration (avoids circular include)

// --- Event types stored in the loop buffer ---------------------

enum LoopEventType : uint8_t {
  EVT_NOTE_ON  = 1,
  EVT_NOTE_OFF = 2
};

/// A single recorded MIDI event with its timestamp.
struct LoopEvent {
  uint32_t timeMs;      // offset in ms from recording start
  uint8_t  type;        // EVT_NOTE_ON or EVT_NOTE_OFF
  uint8_t  note;        // MIDI note number
  uint8_t  velocity;    // velocity (only meaningful for NOTE_ON)
};

// --- Looper states ---------------------------------------------

enum LoopState : uint8_t {
  LOOP_EMPTY = 0,
  LOOP_RECORDING,
  LOOP_PLAYING,
  LOOP_STOPPED
};

// --- Looper class ----------------------------------------------

class Looper {
public:
  /// Constructor: takes references to both synth instances.
  /// @param liveSynth   the synth used for live playing
  /// @param looperSynth the synth dedicated to loop playback
  Looper(MyDsp& liveSynth, MyDsp& looperSynth);

  /// Advance playback by one tick.  Must be called every
  /// iteration of loop() so events are replayed on time.
  void tick();

  // --- State transitions (called by Button callbacks) ----------

  /// Cycle through: EMPTY→REC, REC→PLAY, PLAY→STOP, STOP→REC
  void onShortPress();

  /// Always clear the loop and return to EMPTY.
  void onLongPress();

  // --- MIDI event recording (called by MidiHandler) ------------

  /// Forward a NoteOn to the looper synth (if recording) and
  /// store it in the event buffer.
  void recordNoteOn(uint8_t note, uint8_t vel);

  /// Forward a NoteOff to the looper synth (if recording) and
  /// store it in the event buffer.
  void recordNoteOff(uint8_t note);

  /// Track the current live preset so it can be "frozen" when
  /// recording starts.  If already recording, the looper synth
  /// preset is updated immediately.
  void setLivePreset(int preset);

  /// Query the current state (useful for LED feedback).
  LoopState state() const { return state_; }

private:
  void startRecording();
  void stopRecordingAndPlay();
  void stopPlayback();
  void clear();
  void killActiveNotes();
  void addEvent(uint8_t type, uint8_t note, uint8_t vel);

  MyDsp& live_;
  MyDsp& looper_;

  LoopState state_ = LOOP_EMPTY;

  // Fixed-size event buffer (no dynamic allocation)
  LoopEvent events_[kMaxLoopEvents];
  int       eventCount_ = 0;

  // Timing
  uint32_t recStartMs_   = 0;
  uint32_t loopLengthMs_ = 0;
  uint32_t playStartMs_  = 0;
  int      playIndex_    = 0;

  // Track which notes the looper synth currently has sounding,
  // so we can kill them cleanly on state transitions.
  bool notesOn_[128] = {};

  // Preset freeze: the live preset is tracked continuously;
  // it gets "frozen" into the looper synth at record start.
  int livePreset_   = 0;
  int frozenPreset_ = 0;
};
