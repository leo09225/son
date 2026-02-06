// Un fichier qui teste les boutons en C++
// Copie Colle sur le fichier Arduino qui sera généerer Par Faust
//teste si lorsque qu'on appui sur le bouton next, le preset du piano change

//On collera ce code a notre Firmware Arduino principale PolyPiano


// --- Pins boutons (à adapter si besoin) ---
#define BTN_NEXT 2
#define BTN_PREV 3

int preset = 0;                 // 0..3
unsigned long lastMs = 0;
const unsigned long debounceMs = 180;

void sendPreset() {
  usbMIDI.sendProgramChange(preset, 1); // canal 1
}

void setup() {
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);

  usbMIDI.begin();

  sendPreset(); // preset initial (optionnel)
}

void loop() {
  // Important : lire le MIDI entrant pour garder l'USB "healthy"
  while (usbMIDI.read()) {}

  unsigned long now = millis();
  if (now - lastMs < debounceMs) return;

  if (digitalRead(BTN_NEXT) == LOW) {
    preset = (preset + 1) % 4;
    sendPreset();
    lastMs = now;
  }

  if (digitalRead(BTN_PREV) == LOW) {
    preset = (preset + 3) % 4; // -1 mod 4
    sendPreset();
    lastMs = now;
  }
}

