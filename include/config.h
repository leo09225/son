#pragma once
// ============================================================
// config.h -- Project-wide constants and helper functions
//
// Centralises polyphony settings, MIDI CC assignments,
// hardware pin numbers, and timing parameters so every
// module draws from one source of truth.
// ============================================================

#include <Arduino.h>

// --- Polyphony -------------------------------------------------
constexpr int kVoices = 8;

// --- MIDI Control Change numbers (match external controller) ---
constexpr int CC_MASTER_VOL = 7;
constexpr int CC_ECHO_ON    = 80;
constexpr int CC_ECHO_MIX   = 91;
constexpr int CC_ECHO_FB    = 93;
constexpr int CC_ECHO_MS    = 94;

// --- Hardware pins ---------------------------------------------
constexpr int kLoopButtonPin = 0;

// --- Button timing ---------------------------------------------
constexpr uint32_t kDebounceMs  = 30;
constexpr uint32_t kLongPressMs = 3000;

// --- Looper sizing ---------------------------------------------
constexpr int kMaxLoopEvents = 2048;

// --- Helpers ---------------------------------------------------

/// Clamp a float value between [lo, hi].
inline float clampf(float x, float lo, float hi) {
  return (x < lo) ? lo : (x > hi) ? hi : x;
}
