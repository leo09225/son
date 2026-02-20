#pragma once

// Polyphonie
constexpr int kVoices = 8;

// MIDI CC (aligné avec ton collègue)
constexpr int CC_MASTER_VOL = 7;
constexpr int CC_ECHO_ON    = 80;
constexpr int CC_ECHO_MIX   = 91;
constexpr int CC_ECHO_FB    = 93;
constexpr int CC_ECHO_MS    = 94;

// Helpers
inline float clampf(float x, float lo, float hi) {
  return (x < lo) ? lo : (x > hi) ? hi : x;
}
