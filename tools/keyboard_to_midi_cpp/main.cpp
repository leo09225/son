#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <algorithm>

#include <SDL2/SDL.h>
#include <RtMidi.h>

// ---------- Mapping AZERTY façon GarageBand ----------
// Ligne du bas = touches blanches (C D E F G A B)
// Ligne du haut = touches noires   (C# D#    F# G# A#)
static const std::unordered_map<SDL_Keycode, int> KEY_TO_MIDI = {
    // White keys
    { SDLK_q, 60 }, // C4
    { SDLK_s, 62 }, // D4
    { SDLK_d, 64 }, // E4
    { SDLK_f, 65 }, // F4
    { SDLK_g, 67 }, // G4
    { SDLK_h, 69 }, // A4
    { SDLK_j, 71 }, // B4

    // Black keys
    { SDLK_z, 61 }, // C#4
    { SDLK_e, 63 }, // D#4
    { SDLK_t, 66 }, // F#4
    { SDLK_y, 68 }, // G#4
    { SDLK_u, 70 }, // A#4
};

static bool containsCaseInsensitive(const std::string& haystack, const std::string& needle) {
    auto toLower = [](unsigned char c){ return (char)std::tolower(c); };
    std::string h(haystack.size(), '\0');
    std::string n(needle.size(), '\0');
    std::transform(haystack.begin(), haystack.end(), h.begin(), toLower);
    std::transform(needle.begin(), needle.end(), n.begin(), toLower);
    return h.find(n) != std::string::npos;
}

static void sendNoteOn(RtMidiOut& midi, int note, int velocity) {
    std::vector<unsigned char> msg = {
        (unsigned char)0x90, // NoteOn channel 1
        (unsigned char)(note & 0x7F),
        (unsigned char)(velocity & 0x7F)
    };
    midi.sendMessage(&msg);
}

static void sendNoteOff(RtMidiOut& midi, int note) {
    std::vector<unsigned char> msg = {
        (unsigned char)0x80, // NoteOff channel 1
        (unsigned char)(note & 0x7F),
        (unsigned char)0x00
    };
    midi.sendMessage(&msg);
}

static void sendProgramChange(RtMidiOut& midi, int program) {
    std::vector<unsigned char> msg = {
        (unsigned char)0xC0,                 // Program Change ch1
        (unsigned char)(program & 0x7F)
    };
    midi.sendMessage(&msg);
}

static void sendCC(RtMidiOut& midi, int cc, int value) {
    std::vector<unsigned char> msg = {
        (unsigned char)0xB0,                 // Control Change ch1
        (unsigned char)(cc & 0x7F),
        (unsigned char)(value & 0x7F)
    };
    midi.sendMessage(&msg);
}

int main() {
    const int VELOCITY = 100;
    int currentPreset = 0;
    int master = 80;      // CC7 (0..127)
    bool echoOn = false;  // CC80 (0/127)
    int echoMix = 32;     // CC91 (0..127)

    // ---------- MIDI OUT ----------
    RtMidiOut midi;
    unsigned int ports = midi.getPortCount();

    if (ports == 0) {
        std::cerr << "❌ Aucun port MIDI de sortie trouvé. Branche la Teensy et réessaie.\n";
        return 1;
    }

    int teensyPort = -1;
    std::cout << "Ports MIDI disponibles:\n";
    for (unsigned int i = 0; i < ports; i++) {
        std::string name = midi.getPortName(i);
        std::cout << "  [" << i << "] " << name << "\n";
        if (teensyPort < 0 && containsCaseInsensitive(name, "teensy")) {
            teensyPort = (int)i;
        }
    }

    if (teensyPort < 0) {
        std::cerr << "\n❌ Aucun port contenant 'Teensy' n'a été trouvé.\n"
                  << "👉 Astuce: vérifie que Tools -> USB Type = MIDI + Serial dans Arduino,\n"
                  << "   puis rebranche la Teensy.\n";
        return 1;
    }

    midi.openPort((unsigned int)teensyPort);
    std::cout << "\n✅ Connecté au port Teensy: " << midi.getPortName((unsigned int)teensyPort) << "\n";
    std::cout << "🎹 Joue avec: Q S D F G H J (blanches) et Z E T Y U (noires)\n";
    std::cout << "⛔️ ESC pour quitter\n\n";

    // ---------- SDL (clavier press/release propre) ----------
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "❌ SDL_Init error: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Keyboard->MIDI (Teensy)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        480, 120,
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        std::cerr << "❌ SDL_CreateWindow error: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    // Pour éviter les répétitions auto (quand tu gardes une touche enfoncée)
    SDL_EventState(SDL_TEXTINPUT, SDL_DISABLE);

    std::unordered_set<SDL_Keycode> pressedKeys;

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT:
                    running = false;
                    break;

                case SDL_KEYDOWN: {
                    if (e.key.keysym.sym == SDLK_ESCAPE) {
                        running = false;
                        break;
                    }

                    // Ignore key repeat (quand la touche reste appuyée)
                    if (e.key.repeat != 0) break;
                    
                    // --- Presets: 1..4 -> ProgramChange 0..3 ---
                    if (e.key.keysym.sym == SDLK_1) { currentPreset = 0; sendProgramChange(midi, currentPreset); break; }
                    if (e.key.keysym.sym == SDLK_2) { currentPreset = 1; sendProgramChange(midi, currentPreset); break; }
                    if (e.key.keysym.sym == SDLK_3) { currentPreset = 2; sendProgramChange(midi, currentPreset); break; }
                    if (e.key.keysym.sym == SDLK_4) { currentPreset = 3; sendProgramChange(midi, currentPreset); break; }
                    
                    // --- Master volume CC7 : - / = ---
                    if (e.key.keysym.sym == SDLK_MINUS) {
                        master = std::max(0, master - 5);
                        sendCC(midi, 7, master);
                        break;
                    }
                    if (e.key.keysym.sym == SDLK_EQUALS) {
                        master = std::min(127, master + 5);
                        sendCC(midi, 7, master);
                        break;
                    }

                    // --- Echo toggle : R (CC80) ---
                    if (e.key.keysym.sym == SDLK_r) {
                        echoOn = !echoOn;
                        sendCC(midi, 80, echoOn ? 127 : 0);
                        break;
                    }

                    // --- Echo mix : [ / ] (CC91) ---
                    if (e.key.keysym.sym == SDLK_b) {
                        echoMix = std::max(0, echoMix - 5);
                        sendCC(midi, 91, echoMix);
                        break;
                    }
                    if (e.key.keysym.sym == SDLK_n) {
                        echoMix = std::min(127, echoMix + 5);
                        sendCC(midi, 91, echoMix);
                        break;
                    }

                    auto it = KEY_TO_MIDI.find(e.key.keysym.sym);
                    if (it != KEY_TO_MIDI.end()) {
                        SDL_Keycode k = e.key.keysym.sym;
                        if (pressedKeys.find(k) == pressedKeys.end()) {
                            pressedKeys.insert(k);
                            int note = it->second;
                            sendNoteOn(midi, note, VELOCITY);
                        }
                    }
                    break;
                }

                case SDL_KEYUP: {
                    auto it = KEY_TO_MIDI.find(e.key.keysym.sym);
                    if (it != KEY_TO_MIDI.end()) {
                        SDL_Keycode k = e.key.keysym.sym;
                        if (pressedKeys.find(k) != pressedKeys.end()) {
                            pressedKeys.erase(k);
                            int note = it->second;
                            sendNoteOff(midi, note);
                        }
                    }
                    break;
                }

                default:
                    break;
            }
        }

        SDL_Delay(1);
    }

    // Safety: éteindre toutes les notes si on quitte
    for (const auto& kv : KEY_TO_MIDI) {
        sendNoteOff(midi, kv.second);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    std::cout << "👋 Quit.\n";
    return 0;
}
