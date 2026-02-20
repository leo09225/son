// ============================================================
// Button.cpp -- Debounced push-button implementation
//
// Reads the digital pin every loop() iteration and applies a
// simple time-based debounce filter.  Once the button is
// released, the held duration determines whether the short
// or long press callback is fired.
//
// Wiring assumption: button connects pin to GND; the pin is
// configured as INPUT_PULLUP, so pressed = LOW, released = HIGH.
// ============================================================

#include "Button.h"

DebouncedButton::DebouncedButton(int pin, Callback onShort, Callback onLong)
  : pin_(pin)
  , onShort_(onShort)
  , onLong_(onLong)
{
}

void DebouncedButton::begin() {
  pinMode(pin_, INPUT_PULLUP);

  Serial.print("[BTN] Pin ");
  Serial.print(pin_);
  Serial.print(" configured, initial state: ");
  Serial.println(digitalRead(pin_) ? "HIGH" : "LOW");
}

void DebouncedButton::update() {
  bool raw = digitalRead(pin_);
  uint32_t now = millis();

  // ---- Debounce: restart the timer on every raw transition ----
  if (raw != lastRawRead_) {
    lastRawRead_  = raw;
    lastChangeMs_ = now;
    Serial.print("[BTN] Pin change: ");
    Serial.println(raw ? "HIGH" : "LOW");
  }

  // Ignore changes that haven't been stable long enough
  if ((now - lastChangeMs_) < kDebounceMs) return;

  // ---- Stable transition detected -----------------------------
  if (raw != stableState_) {
    stableState_ = raw;

    Serial.print("[BTN] Stable: ");
    Serial.println(stableState_ ? "HIGH (released)" : "LOW (pressed)");

    if (stableState_ == false) {
      // Button just pressed -- record the timestamp
      Serial.println("[BTN] >>> BUTTON PRESSED <<<");
      pressStartMs_ = now;

    } else {
      // Button just released -- measure how long it was held
      uint32_t held = now - pressStartMs_;

      Serial.print("[BTN] >>> BUTTON RELEASED after ");
      Serial.print(held);
      Serial.println(" ms");

      if (held >= kLongPressMs) {
        Serial.println("[BTN] -> LONG PRESS");
        if (onLong_) onLong_();
      } else {
        Serial.println("[BTN] -> SHORT PRESS");
        if (onShort_) onShort_();
      }
    }
  }
}
