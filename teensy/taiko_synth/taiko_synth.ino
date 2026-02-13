#include <Audio.h>
#include "MyDsp.h"

// =======================
// AUDIO - DEUX SYNTHÉS !
// =======================
MyDsp myDspLive;    // ← pour jouer live
MyDsp myDspLooper;  // ← pour la boucle

// Mixer pour combiner les deux
AudioMixer4 mixerL;
AudioMixer4 mixerR;

AudioOutputI2S out;

// Connexions : Live + Looper → Mixer → Output
AudioConnection patchLiveL(myDspLive, 0, mixerL, 0);
AudioConnection patchLiveR(myDspLive, 1, mixerR, 0);
AudioConnection patchLooperL(myDspLooper, 0, mixerL, 1);
AudioConnection patchLooperR(myDspLooper, 1, mixerR, 1);
AudioConnection patchOutL(mixerL, 0, out, 0);
AudioConnection patchOutR(mixerR, 0, out, 1);

AudioControlSGTL5000 audioShield;

// =======================
// LOOP BUTTON
// =======================
static const int LOOP_BTN_PIN = 0;
static const uint32_t DEBOUNCE_MS = 30;
static const uint32_t LONG_PRESS_MS = 3000;

// =======================
// LOOPER
// =======================
enum LoopEventType : uint8_t { EVT_NOTE_ON = 1, EVT_NOTE_OFF = 2 };

struct LoopEvent {
  uint32_t t_ms;
  uint8_t type;
  uint8_t d1;     // note
  uint8_t d2;     // vel
};

static const int MAX_EVENTS = 2048;
static LoopEvent events[MAX_EVENTS];
static int eventCount = 0;

enum LoopState : uint8_t { LOOP_EMPTY = 0, LOOP_RECORDING, LOOP_PLAYING, LOOP_STOPPED };
static LoopState loopState = LOOP_EMPTY;

static uint32_t recStartMs = 0;
static uint32_t loopLengthMs = 0;

static uint32_t playStartMs = 0;
static int playIndex = 0;

// tracker les notes actives du looper
static bool looperNotesOn[128] = {false};

// ← NOUVEAU : preset actif pour chaque synthé
static int currentLivePreset = 0;
static int frozenLooperPreset = 0;  // preset "gelé" au moment de l'enregistrement

// button state
static bool btnStable = true;
static bool btnLastRead = true;
static uint32_t btnLastChangeMs = 0;
static uint32_t btnPressStartMs = 0;

// =======================
// Helpers
// =======================
static inline float ccTo01(uint8_t v) { return (float)v / 127.0f; }

static void looperKillActiveNotes() {
  Serial.println("[LOOPER] Killing active looper notes");
  for (int i = 0; i < 128; i++) {
    if (looperNotesOn[i]) {
      myDspLooper.noteOff(i);
      looperNotesOn[i] = false;
    }
  }
}

static void looperClear() {
  Serial.println("[LOOPER] CLEAR");
  looperKillActiveNotes();
  eventCount = 0;
  loopLengthMs = 0;
  loopState = LOOP_EMPTY;
  playIndex = 0;
}

static void looperStartRecord() {
  Serial.println("[LOOPER] START RECORDING");
  looperKillActiveNotes();
  
  // ← NOUVEAU : gèle le preset actuel du live pour le looper
  frozenLooperPreset = currentLivePreset;
  myDspLooper.setPreset(frozenLooperPreset);
  
  Serial.print("[LOOPER] Frozen preset for loop: ");
  Serial.println(frozenLooperPreset);
  
  eventCount = 0;
  loopLengthMs = 0;
  recStartMs = millis();
  loopState = LOOP_RECORDING;
}

static void looperStopRecordAndPlay() {
  if (eventCount <= 0) {
    Serial.println("[LOOPER] No events recorded -> back to EMPTY");
    loopState = LOOP_EMPTY;
    return;
  }
  
  loopLengthMs = millis() - recStartMs;
  
  Serial.print("[LOOPER] STOP RECORDING. Duration: ");
  Serial.print(loopLengthMs);
  Serial.print(" ms, Events: ");
  Serial.print(eventCount);
  Serial.print(", Preset: ");
  Serial.println(frozenLooperPreset);
  
  looperKillActiveNotes();
  playStartMs = millis();
  playIndex = 0;
  loopState = LOOP_PLAYING;
}

static void looperStopPlay() {
  Serial.println("[LOOPER] STOP PLAYING");
  looperKillActiveNotes();
  loopState = LOOP_STOPPED;
  playIndex = 0;
}

static void looperAdd(uint8_t type, uint8_t d1, uint8_t d2) {
  if (eventCount >= MAX_EVENTS) {
    Serial.println("[LOOPER] !!! BUFFER FULL !!!");
    return;
  }
  uint32_t now = millis();
  uint32_t t = now - recStartMs;
  events[eventCount++] = { t, type, d1, d2 };
  
  Serial.print("[LOOPER] Event #");
  Serial.print(eventCount);
  Serial.print(" @ ");
  Serial.print(t);
  Serial.print(" ms: ");
  Serial.print(type == EVT_NOTE_ON ? "NoteON" : "NoteOFF");
  Serial.print(" note=");
  Serial.println(d1);
}

static void looperTickPlayback() {
  if (loopState != LOOP_PLAYING || eventCount <= 0) return;

  uint32_t now = millis();
  uint32_t elapsed = now - playStartMs;

  // reboucler
  if (elapsed >= loopLengthMs) {
    Serial.print("[LOOPER] Loop finished (");
    Serial.print(elapsed);
    Serial.print("/");
    Serial.print(loopLengthMs);
    Serial.println(" ms) -> REWIND");
    
    looperKillActiveNotes();
    playStartMs = millis();
    playIndex = 0;
    elapsed = 0;
  }

  // joue tous les events "dus"
  while (playIndex < eventCount && events[playIndex].t_ms <= elapsed) {
    const LoopEvent &ev = events[playIndex];

    Serial.print("[LOOPER] Playing event #");
    Serial.print(playIndex);
    Serial.print(" @ ");
    Serial.print(elapsed);
    Serial.print(" ms: ");
    Serial.print(ev.type == EVT_NOTE_ON ? "NoteON" : "NoteOFF");
    Serial.print(" note=");
    Serial.println(ev.d1);

    if (ev.type == EVT_NOTE_ON) {
      myDspLooper.noteOn(ev.d1, ev.d2);
      looperNotesOn[ev.d1] = true;
    } else if (ev.type == EVT_NOTE_OFF) {
      myDspLooper.noteOff(ev.d1);
      looperNotesOn[ev.d1] = false;
    }
    playIndex++;
  }
}

// =======================
// MIDI routing
// =======================
static void handleMidiMessage() {
  uint8_t type = usbMIDI.getType();

  if (type == usbMIDI.NoteOn) {
    uint8_t note = usbMIDI.getData1();
    uint8_t vel  = usbMIDI.getData2();

    Serial.print("[MIDI] NoteON: note=");
    Serial.print(note);
    Serial.print(" vel=");
    Serial.println(vel);

    if (vel > 0) {
      myDspLive.noteOn(note, vel);
      
      // Pendant l'enregistrement, joue aussi sur le looper
      if (loopState == LOOP_RECORDING) {
        myDspLooper.noteOn(note, vel);
        looperAdd(EVT_NOTE_ON, note, vel);
      }
    } else {
      myDspLive.noteOff(note);
      
      if (loopState == LOOP_RECORDING) {
        myDspLooper.noteOff(note);
        looperAdd(EVT_NOTE_OFF, note, 0);
      }
    }
  }
  else if (type == usbMIDI.NoteOff) {
    uint8_t note = usbMIDI.getData1();
    Serial.print("[MIDI] NoteOFF: note=");
    Serial.println(note);
    
    myDspLive.noteOff(note);
    
    if (loopState == LOOP_RECORDING) {
      myDspLooper.noteOff(note);
      looperAdd(EVT_NOTE_OFF, note, 0);
    }
  }
  else if (type == usbMIDI.ProgramChange) {
    uint8_t pgm = usbMIDI.getData1();
    int preset = pgm % 4;
    
    // ← CHANGE : sauvegarde le preset live
    currentLivePreset = preset;
    myDspLive.setPreset(preset);
    
    Serial.print("[MIDI] Program Change -> LIVE preset: ");
    Serial.println(preset);
    
    // ← NOUVEAU : si on est en recording, change aussi le looper ET gèle le nouveau preset
    if (loopState == LOOP_RECORDING) {
      frozenLooperPreset = preset;
      myDspLooper.setPreset(preset);
      Serial.print("[MIDI] Program Change -> LOOPER preset (recording): ");
      Serial.println(preset);
    }
    // Si on est en PLAY ou autre, le looper garde son preset gelé
  }
  else if (type == usbMIDI.ControlChange) {
    uint8_t cc  = usbMIDI.getData1();
    uint8_t val = usbMIDI.getData2();

    // Applique les CC aux DEUX synthés
    if (cc == 7) {
      float gain = ccTo01(val);
      myDspLive.setMasterGain(gain);
      myDspLooper.setMasterGain(gain);
    } else if (cc == 80) {
      bool on = val >= 64;
      myDspLive.setEchoOn(on);
      myDspLooper.setEchoOn(on);
    } else if (cc == 91) {
      float mix = ccTo01(val);
      myDspLive.setEchoMix(mix);
      myDspLooper.setEchoMix(mix);
    } else if (cc == 93) {
      float fb = 0.85f * ccTo01(val);
      myDspLive.setEchoFb(fb);
      myDspLooper.setEchoFb(fb);
    } else if (cc == 94) {
      float ms = 30.0f + (770.0f * ccTo01(val));
      myDspLive.setEchoMs(ms);
      myDspLooper.setEchoMs(ms);
    }
  }
}

// =======================
// Button (short/long press)
// =======================
static void handleLoopButton() {
  bool raw = digitalRead(LOOP_BTN_PIN);
  uint32_t now = millis();

  // debounce
  if (raw != btnLastRead) {
    btnLastRead = raw;
    btnLastChangeMs = now;
    Serial.print("[BTN] Pin change: ");
    Serial.println(raw ? "HIGH" : "LOW");
  }
  
  if ((now - btnLastChangeMs) < DEBOUNCE_MS) return;

  // stable change?
  if (raw != btnStable) {
    btnStable = raw;
    Serial.print("[BTN] Stable: ");
    Serial.println(btnStable ? "HIGH (released)" : "LOW (pressed)");

    if (btnStable == false) {
      // pressed
      Serial.println("[BTN] >>> BUTTON PRESSED <<<");
      btnPressStartMs = now;
    } else {
      // released
      uint32_t held = now - btnPressStartMs;
      Serial.print("[BTN] >>> BUTTON RELEASED after ");
      Serial.print(held);
      Serial.println(" ms");

      if (held >= LONG_PRESS_MS) {
        Serial.println("[BTN] Action: LONG PRESS -> CLEAR");
        looperClear();
      } else {
        Serial.print("[BTN] Action: SHORT PRESS. State=");
        Serial.println(loopState);
        
        if (loopState == LOOP_EMPTY || loopState == LOOP_STOPPED) {
          Serial.println("[BTN] -> START RECORDING");
          looperStartRecord();
        } else if (loopState == LOOP_RECORDING) {
          Serial.println("[BTN] -> STOP REC, START PLAY");
          looperStopRecordAndPlay();
        } else if (loopState == LOOP_PLAYING) {
          Serial.println("[BTN] -> STOP PLAY");
          looperStopPlay();
        }
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== TEENSY LOOPER DEBUG (DUAL SYNTH + PRESET FREEZE) ===");
  Serial.println("Loop preserves the preset used during recording!");
  
  AudioMemory(60);  // 2x synthés
  audioShield.enable();
  audioShield.volume(0.5);
  
  // Configure mixer gains
  mixerL.gain(0, 0.5);  // live left
  mixerL.gain(1, 0.5);  // looper left
  mixerR.gain(0, 0.5);  // live right
  mixerR.gain(1, 0.5);  // looper right
  
  Serial.println("Audio initialized (dual synth + mixer)");

  pinMode(LOOP_BTN_PIN, INPUT_PULLUP);
  Serial.print("Loop button on pin ");
  Serial.print(LOOP_BTN_PIN);
  Serial.print(", initial state: ");
  Serial.println(digitalRead(LOOP_BTN_PIN) ? "HIGH" : "LOW");

  usbMIDI.begin();
  Serial.println("USB MIDI started");
  
  // Init les deux synthés avec le preset 0
  myDspLive.setPreset(0);
  myDspLooper.setPreset(0);
  currentLivePreset = 0;
  frozenLooperPreset = 0;
  
  looperClear();
  Serial.println("Ready!\n");
}

void loop() {
  handleLoopButton();
  
  while (usbMIDI.read()) {
    handleMidiMessage();
  }
  
  looperTickPlayback();
}