#include "MidiRouter.h"

MidiRouter::MidiRouter(MyDsp& d) : dsp(d) {}

void MidiRouter::update() {
  while (usbMIDI.read()) {
    uint8_t type = usbMIDI.getType();

    if (type == usbMIDI.NoteOn) {
      uint8_t note = usbMIDI.getData1();
      uint8_t vel  = usbMIDI.getData2();
      if (vel > 0) dsp.noteOn(note, vel);
      else dsp.noteOff(note);
    }
    else if (type == usbMIDI.NoteOff) {
      uint8_t note = usbMIDI.getData1();
      dsp.noteOff(note);
    }
    else if (type == usbMIDI.ProgramChange) {
      uint8_t pgm = usbMIDI.getData1(); // 0..127
      dsp.setPreset(pgm % 4);
    }
    else if (type == usbMIDI.ControlChange) {
      uint8_t cc  = usbMIDI.getData1();
      uint8_t val = usbMIDI.getData2();

      if (cc == CC_MASTER_VOL) {
        dsp.setMasterGain(ccTo01(val));
      } else if (cc == CC_ECHO_ON) {
        dsp.setEchoOn(val >= 64);
      } else if (cc == CC_ECHO_MIX) {
        dsp.setEchoMix(ccTo01(val));
      } else if (cc == CC_ECHO_FB) {
        dsp.setEchoFb(0.85f * ccTo01(val));
      } else if (cc == CC_ECHO_MS) {
        // map 0..127 -> 30..800 ms
        float ms = 30.0f + (770.0f * ccTo01(val));
        dsp.setEchoMs(ms);
      }
    }
  }
}
