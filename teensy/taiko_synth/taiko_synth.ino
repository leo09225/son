#include <Audio.h>
#include "MyDsp.h"
#include "MidiRouter.h"

MyDsp myDsp;
AudioOutputI2S out;
AudioConnection patchL(myDsp, 0, out, 0);
AudioConnection patchR(myDsp, 1, out, 1);
AudioControlSGTL5000 audioShield;

MidiRouter midi(myDsp);

void setup() {
  AudioMemory(30);
  audioShield.enable();
  audioShield.volume(0.5);

  usbMIDI.begin();
}

void loop() {
  midi.update();
}
