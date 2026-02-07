#pragma once
#include <Arduino.h>
#include "MyDsp.h"
#include "config.h"

class MidiRouter {
public:
  MidiRouter(MyDsp& dsp);

  void update();

private:
  MyDsp& dsp;

  float ccTo01(uint8_t v) const { return v / 127.0f; }
};
