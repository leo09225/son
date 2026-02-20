// ============================================================
// Looper.cpp -- MIDI event looper implementation
//
// Records timestamped NoteOn/NoteOff events during recording,
// then replays them in an endless loop during playback.
// The synth preset is "frozen" at record start so the loop
// keeps its original timbre even if the live preset changes.
// ============================================================

#include "Looper.h"
#include "MyDsp.h"

// --- Constructor -----------------------------------------------

Looper::Looper(MyDsp& liveSynth, MyDsp& looperSynth)
  : live_(liveSynth)
  , looper_(looperSynth)
{
}

// --- Internal helpers ------------------------------------------

/// Send NoteOff for every note the looper currently has sounding.
/// This prevents "stuck notes" on state transitions.
void Looper::killActiveNotes() {
  Serial.println("[LOOPER] Killing active looper notes");
  for (int i = 0; i < 128; i++) {
    if (notesOn_[i]) {
      looper_.noteOff(i);
      notesOn_[i] = false;
    }
  }
}

/// Reset everything back to the initial empty state.
void Looper::clear() {
  Serial.println("[LOOPER] CLEAR");
  killActiveNotes();
  eventCount_   = 0;
  loopLengthMs_ = 0;
  state_        = LOOP_EMPTY;
  playIndex_    = 0;
}

/// Begin recording: freeze the current live preset for the looper,
/// reset the event buffer, and start the timestamp clock.
void Looper::startRecording() {
  Serial.println("[LOOPER] START RECORDING");
  killActiveNotes();

  // Freeze the live preset into the looper synth
  frozenPreset_ = livePreset_;
  looper_.setPreset(frozenPreset_);

  Serial.print("[LOOPER] Frozen preset for loop: ");
  Serial.println(frozenPreset_);

  eventCount_   = 0;
  loopLengthMs_ = 0;
  recStartMs_   = millis();
  state_        = LOOP_RECORDING;
}

/// Stop recording and immediately start playback.
/// If no events were recorded, go back to EMPTY instead.
void Looper::stopRecordingAndPlay() {
  if (eventCount_ <= 0) {
    Serial.println("[LOOPER] No events recorded -> back to EMPTY");
    state_ = LOOP_EMPTY;
    return;
  }

  loopLengthMs_ = millis() - recStartMs_;

  Serial.print("[LOOPER] STOP RECORDING. Duration: ");
  Serial.print(loopLengthMs_);
  Serial.print(" ms, Events: ");
  Serial.print(eventCount_);
  Serial.print(", Preset: ");
  Serial.println(frozenPreset_);

  killActiveNotes();
  playStartMs_ = millis();
  playIndex_   = 0;
  state_       = LOOP_PLAYING;
}

/// Stop playback (loop stays in memory and can be restarted).
void Looper::stopPlayback() {
  Serial.println("[LOOPER] STOP PLAYING");
  killActiveNotes();
  state_     = LOOP_STOPPED;
  playIndex_ = 0;
}

/// Append one event to the buffer (with overflow protection).
void Looper::addEvent(uint8_t type, uint8_t note, uint8_t vel) {
  if (eventCount_ >= kMaxLoopEvents) {
    Serial.println("[LOOPER] !!! BUFFER FULL !!!");
    return;
  }

  uint32_t t = millis() - recStartMs_;
  events_[eventCount_++] = { t, type, note, vel };

  Serial.print("[LOOPER] Event #");
  Serial.print(eventCount_);
  Serial.print(" @ ");
  Serial.print(t);
  Serial.print(" ms: ");
  Serial.print(type == EVT_NOTE_ON ? "NoteON" : "NoteOFF");
  Serial.print(" note=");
  Serial.println(note);
}

// --- Public: state transitions ---------------------------------

void Looper::onShortPress() {
  Serial.print("[BTN] Action: SHORT PRESS. State=");
  Serial.println(state_);

  switch (state_) {
    case LOOP_EMPTY:
    case LOOP_STOPPED:
      Serial.println("[BTN] -> START RECORDING");
      startRecording();
      break;

    case LOOP_RECORDING:
      Serial.println("[BTN] -> STOP REC, START PLAY");
      stopRecordingAndPlay();
      break;

    case LOOP_PLAYING:
      Serial.println("[BTN] -> STOP PLAY");
      stopPlayback();
      break;
  }
}

void Looper::onLongPress() {
  Serial.println("[BTN] Action: LONG PRESS -> CLEAR");
  clear();
}

// --- Public: MIDI event recording ------------------------------

void Looper::recordNoteOn(uint8_t note, uint8_t vel) {
  if (state_ != LOOP_RECORDING) return;
  looper_.noteOn(note, vel);
  addEvent(EVT_NOTE_ON, note, vel);
}

void Looper::recordNoteOff(uint8_t note) {
  if (state_ != LOOP_RECORDING) return;
  looper_.noteOff(note);
  addEvent(EVT_NOTE_OFF, note, 0);
}

void Looper::setLivePreset(int preset) {
  livePreset_ = preset;

  // If currently recording, update the looper synth too
  if (state_ == LOOP_RECORDING) {
    frozenPreset_ = preset;
    looper_.setPreset(preset);
    Serial.print("[LOOPER] Preset changed during recording: ");
    Serial.println(preset);
  }
}

// --- Public: playback tick -------------------------------------

void Looper::tick() {
  if (state_ != LOOP_PLAYING || eventCount_ <= 0) return;

  uint32_t now     = millis();
  uint32_t elapsed = now - playStartMs_;

  // Wrap around: when we reach the end of the loop, rewind
  if (elapsed >= loopLengthMs_) {
    Serial.print("[LOOPER] Loop finished (");
    Serial.print(elapsed);
    Serial.print("/");
    Serial.print(loopLengthMs_);
    Serial.println(" ms) -> REWIND");

    killActiveNotes();
    playStartMs_ = millis();
    playIndex_   = 0;
    elapsed      = 0;
  }

  // Replay all events whose timestamp has been reached
  while (playIndex_ < eventCount_ && events_[playIndex_].timeMs <= elapsed) {
    const LoopEvent& ev = events_[playIndex_];

    Serial.print("[LOOPER] Playing event #");
    Serial.print(playIndex_);
    Serial.print(" @ ");
    Serial.print(elapsed);
    Serial.print(" ms: ");
    Serial.print(ev.type == EVT_NOTE_ON ? "NoteON" : "NoteOFF");
    Serial.print(" note=");
    Serial.println(ev.note);

    if (ev.type == EVT_NOTE_ON) {
      looper_.noteOn(ev.note, ev.velocity);
      notesOn_[ev.note] = true;
    } else if (ev.type == EVT_NOTE_OFF) {
      looper_.noteOff(ev.note);
      notesOn_[ev.note] = false;
    }

    playIndex_++;
  }
}
