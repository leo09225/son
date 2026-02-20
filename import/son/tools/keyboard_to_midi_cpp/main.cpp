#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <algorithm>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <RtMidi.h>

// ---------- Mapping AZERTY façon GarageBand ----------
// White: Q S D F G H J  (C D E F G A B)
// Black: Z E   T Y U    (C# D#   F# G# A#)
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
        (unsigned char)0x90,
        (unsigned char)(note & 0x7F),
        (unsigned char)(velocity & 0x7F)
    };
    midi.sendMessage(&msg);
}

static void sendNoteOff(RtMidiOut& midi, int note) {
    std::vector<unsigned char> msg = {
        (unsigned char)0x80,
        (unsigned char)(note & 0x7F),
        (unsigned char)0x00
    };
    midi.sendMessage(&msg);
}

static void sendProgramChange(RtMidiOut& midi, int program) {
    std::vector<unsigned char> msg = {
        (unsigned char)0xC0,
        (unsigned char)(program & 0x7F)
    };
    midi.sendMessage(&msg);
}

static void sendCC(RtMidiOut& midi, int cc, int value) {
    std::vector<unsigned char> msg = {
        (unsigned char)0xB0,
        (unsigned char)(cc & 0x7F),
        (unsigned char)(value & 0x7F)
    };
    midi.sendMessage(&msg);
}

// ---------- GUI helpers (SDL2_ttf) ----------
static SDL_Texture* makeText(SDL_Renderer* r, TTF_Font* font, const std::string& text, SDL_Color color, int& w, int& h) {
    SDL_Surface* s = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!s) return nullptr;
    SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
    w = s->w; h = s->h;
    SDL_FreeSurface(s);
    return t;
}

struct KeyRect {
    SDL_Keycode keycode;   // clavier PC (SDLK_q etc.)
    int midi;              // note MIDI
    bool isBlack;          // noire/blanche
    SDL_Rect rect;         // position écran
    std::string label;     // "Q", "S", ...
};

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
                  << "👉 Astuce: Arduino -> Tools -> USB Type = MIDI + Serial, puis rebranche.\n";
        return 1;
    }

    midi.openPort((unsigned int)teensyPort);
    std::cout << "\n✅ Connecté au port Teensy: " << midi.getPortName((unsigned int)teensyPort) << "\n";
    std::cout << "⛔️ ESC pour quitter\n\n";

    // ---------- SDL ----------
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "❌ SDL_Init error: " << SDL_GetError() << "\n";
        return 1;
    }
    if (TTF_Init() != 0) {
        std::cerr << "❌ TTF_Init error: " << TTF_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    const int W = 720;
    const int H = 260;

    SDL_Window* window = SDL_CreateWindow(
        "TaikoSynth - Keyboard Piano (Teensy MIDI)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        W, H,
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        std::cerr << "❌ SDL_CreateWindow error: " << SDL_GetError() << "\n";
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "❌ SDL_CreateRenderer error: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // IMPORTANT: Mets une police dispo sur ton Mac.
    // Option 1: tu mets un .ttf dans ton dossier tools/ et tu pointes dessus.
    // Option 2: tu utilises une police système comme ci-dessous.
    const char* fontPath = "/System/Library/Fonts/Supplemental/Arial Unicode.ttf";
    TTF_Font* font = TTF_OpenFont(fontPath, 18);
    if (!font) {
        // fallback
        fontPath = "/System/Library/Fonts/Supplemental/Arial.ttf";
        font = TTF_OpenFont(fontPath, 18);
    }
    if (!font) {
        std::cerr << "❌ Impossible d'ouvrir une police TTF. Mets un .ttf dans le dossier et change fontPath.\n";
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Pour éviter répétitions auto
    SDL_EventState(SDL_TEXTINPUT, SDL_DISABLE);

    std::unordered_set<SDL_Keycode> pressedKeys;

    // ---------- Layout piano ----------
    // Zone clavier
    const int pianoX = 20;
    const int pianoY = 60;
    const int pianoW = W - 40;
    const int pianoH = 170;

    const int whiteCount = 7;
    const int whiteW = pianoW / whiteCount;
    const int whiteH = pianoH;

    const int blackW = (int)(whiteW * 0.60f);
    const int blackH = (int)(whiteH * 0.62f);

    // touches blanches dans l'ordre C D E F G A B
    std::vector<KeyRect> keys;
    keys.push_back({SDLK_q, 60, false, {pianoX + 0*whiteW, pianoY, whiteW, whiteH}, "Q"}); // C
    keys.push_back({SDLK_s, 62, false, {pianoX + 1*whiteW, pianoY, whiteW, whiteH}, "S"}); // D
    keys.push_back({SDLK_d, 64, false, {pianoX + 2*whiteW, pianoY, whiteW, whiteH}, "D"}); // E
    keys.push_back({SDLK_f, 65, false, {pianoX + 3*whiteW, pianoY, whiteW, whiteH}, "F"}); // F
    keys.push_back({SDLK_g, 67, false, {pianoX + 4*whiteW, pianoY, whiteW, whiteH}, "G"}); // G
    keys.push_back({SDLK_h, 69, false, {pianoX + 5*whiteW, pianoY, whiteW, whiteH}, "H"}); // A
    keys.push_back({SDLK_j, 71, false, {pianoX + 6*whiteW, pianoY, whiteW, whiteH}, "J"}); // B

    // touches noires: C# D# F# G# A#
    auto blackX = [&](int whiteIndexLeft) {
        // position entre deux blanches
        int left = pianoX + whiteIndexLeft * whiteW;
        return left + whiteW - blackW/2;
    };

    keys.push_back({SDLK_z, 61, true, {blackX(0), pianoY, blackW, blackH}, "Z"}); // C#
    keys.push_back({SDLK_e, 63, true, {blackX(1), pianoY, blackW, blackH}, "E"}); // D#
    keys.push_back({SDLK_t, 66, true, {blackX(3), pianoY, blackW, blackH}, "T"}); // F#
    keys.push_back({SDLK_y, 68, true, {blackX(4), pianoY, blackW, blackH}, "Y"}); // G#
    keys.push_back({SDLK_u, 70, true, {blackX(5), pianoY, blackW, blackH}, "U"}); // A#

    // ---------- Main loop ----------
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
                    if (e.key.repeat != 0) break;

                    // --- Presets 1..4 ---
                    if (e.key.keysym.sym == SDLK_1) { currentPreset = 0; sendProgramChange(midi, currentPreset); break; }
                    if (e.key.keysym.sym == SDLK_2) { currentPreset = 1; sendProgramChange(midi, currentPreset); break; }
                    if (e.key.keysym.sym == SDLK_3) { currentPreset = 2; sendProgramChange(midi, currentPreset); break; }
                    if (e.key.keysym.sym == SDLK_4) { currentPreset = 3; sendProgramChange(midi, currentPreset); break; }

                    // --- Volume: - / = ---
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

                    // --- Echo toggle: R (CC80) ---
                    if (e.key.keysym.sym == SDLK_r) {
                        echoOn = !echoOn;
                        sendCC(midi, 80, echoOn ? 127 : 0);
                        break;
                    }

                    // --- Echo mix: B / N (CC91) ---
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

                    // --- Notes ---
                    auto it = KEY_TO_MIDI.find(e.key.keysym.sym);
                    if (it != KEY_TO_MIDI.end()) {
                        SDL_Keycode k = e.key.keysym.sym;
                        if (pressedKeys.find(k) == pressedKeys.end()) {
                            pressedKeys.insert(k);
                            sendNoteOn(midi, it->second, VELOCITY);
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
                            sendNoteOff(midi, it->second);
                        }
                    }
                    break;
                }

                default:
                    break;
            }
        }

        // ---------- RENDER ----------
        // background
        SDL_SetRenderDrawColor(renderer, 18, 18, 22, 255);
        SDL_RenderClear(renderer);

        // HUD text
        SDL_Color hudColor{220, 220, 220, 255};
        std::string hud =
            "Preset(1-4): " + std::to_string(currentPreset+1) +
            "   Vol(-/=): " + std::to_string(master) +
            "   Echo(R): " + std::string(echoOn ? "ON" : "OFF") +
            "   Mix(B/N): " + std::to_string(echoMix);

        {
            int tw=0, th=0;
            SDL_Texture* t = makeText(renderer, font, hud, hudColor, tw, th);
            if (t) {
                SDL_Rect dst{20, 18, tw, th};
                SDL_RenderCopy(renderer, t, nullptr, &dst);
                SDL_DestroyTexture(t);
            }
        }

        // 1) draw white keys first
        for (const auto& k : keys) {
            if (k.isBlack) continue;

            bool down = (pressedKeys.find(k.keycode) != pressedKeys.end());

            // fill
            if (down) SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
            else      SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
            SDL_RenderFillRect(renderer, &k.rect);

            // border
            SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255);
            SDL_RenderDrawRect(renderer, &k.rect);

            // label
            int tw=0, th=0;
            SDL_Color c{20, 20, 20, 255};
            SDL_Texture* t = makeText(renderer, font, k.label, c, tw, th);
            if (t) {
                SDL_Rect dst{
                    k.rect.x + (k.rect.w - tw)/2,
                    k.rect.y + k.rect.h - th - 12,
                    tw, th
                };
                SDL_RenderCopy(renderer, t, nullptr, &dst);
                SDL_DestroyTexture(t);
            }
        }

        // 2) then black keys on top
        for (const auto& k : keys) {
            if (!k.isBlack) continue;

            bool down = (pressedKeys.find(k.keycode) != pressedKeys.end());

            // fill
            if (down) SDL_SetRenderDrawColor(renderer, 90, 90, 90, 255);
            else      SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
            SDL_RenderFillRect(renderer, &k.rect);

            // border
            SDL_SetRenderDrawColor(renderer, 10, 10, 10, 255);
            SDL_RenderDrawRect(renderer, &k.rect);

            // label
            int tw=0, th=0;
            SDL_Color c{235, 235, 235, 255};
            SDL_Texture* t = makeText(renderer, font, k.label, c, tw, th);
            if (t) {
                SDL_Rect dst{
                    k.rect.x + (k.rect.w - tw)/2,
                    k.rect.y + k.rect.h - th - 10,
                    tw, th
                };
                SDL_RenderCopy(renderer, t, nullptr, &dst);
                SDL_DestroyTexture(t);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(1);
    }

    // Safety: note off all
    for (const auto& kv : KEY_TO_MIDI) {
        sendNoteOff(midi, kv.second);
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    std::cout << "👋 Quit.\n";
    return 0;
}
