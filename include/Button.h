#pragma once
// ============================================================
// Button.h -- Debounced push-button with short / long press
//
// Handles hardware debouncing for a normally-open button wired
// with INPUT_PULLUP (active LOW).  On release, the held
// duration is measured to distinguish a short press from a
// long press.  Two user-supplied callbacks are invoked
// accordingly.
//
// Usage:
//   DebouncedButton btn(pin, onShort, onLong);
//   btn.begin();          // in setup()
//   btn.update();         // in loop(), every iteration
// ============================================================

#include <Arduino.h>
#include "config.h"

class DebouncedButton {
public:
  /// Function pointer type for callbacks (no arguments, no return).
  using Callback = void (*)();

  /// @param pin     GPIO pin number (will be configured as INPUT_PULLUP)
  /// @param onShort called on release after a short press (< kLongPressMs)
  /// @param onLong  called on release after a long  press (>= kLongPressMs)
  DebouncedButton(int pin, Callback onShort, Callback onLong);

  /// Configure the pin.  Call once in setup().
  void begin();

  /// Read the pin and fire callbacks if needed.
  /// Call every iteration of loop().
  void update();

private:
  int      pin_;
  Callback onShort_;
  Callback onLong_;

  bool     stableState_   = true;   // HIGH = released (INPUT_PULLUP)
  bool     lastRawRead_   = true;
  uint32_t lastChangeMs_  = 0;
  uint32_t pressStartMs_  = 0;
};
