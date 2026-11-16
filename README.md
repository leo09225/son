# 🎹 Synteensyzer — Synthétiseur polyphonique temps réel sur Teensy 4.0

> Synthétiseur numérique embarqué : le PC envoie des notes MIDI en USB, un microcontrôleur Teensy 4.0 (ARM Cortex-M7, 600 MHz) calcule le son échantillon par échantillon et le restitue au casque via un codec audio.

Projet de 3ᵉ année, département Télécommunications de l'INSA Lyon (février 2026), réalisé en binôme.

![C++](https://img.shields.io/badge/C++-17-00599C?logo=cplusplus&logoColor=white)
![Teensy](https://img.shields.io/badge/Teensy-4.0-orange)
![PlatformIO](https://img.shields.io/badge/PlatformIO-build-F5822A?logo=platformio&logoColor=white)
![MIDI](https://img.shields.io/badge/protocole-USB%20MIDI-6f42c1)
![Python](https://img.shields.io/badge/Python-launcher-3776AB?logo=python&logoColor=white)

---

## Le projet en bref

| | |
|---|---|
| **Objectif** | Concevoir un instrument de musique numérique complet, du clavier jusqu'à la sortie audio, avec une contrainte temps réel stricte. |
| **Matériel** | Teensy 4.0 + Audio Shield (codec SGTL5000, liaison I2S), bouton physique, PC hôte. |
| **Logiciel** | Firmware C++ modulaire (~1 200 lignes), outil PC C++ (SDL2 + RtMidi) qui transforme le clavier de l'ordinateur en clavier MIDI, lanceur Python. |
| **Contrainte clé** | Le moteur audio est appelé par interruption toutes les **2,9 ms** (blocs de 128 échantillons à 44,1 kHz) : au-delà, le son craque. |

## Fonctionnalités

- 🎼 **Polyphonie 8 voix** avec allocation et libération des voix sur `NoteOn` / `NoteOff`
- 🎛️ **4 timbres** : sinus (table d'onde), orgue (synthèse additive), piano électrique, pad
- 📈 **Enveloppe ADSR** indépendante par voix
- 🔁 **Écho configurable** (délai, mix, feedback) piloté par messages MIDI Control Change
- ⏺️ **Looper** : enregistrement et lecture en boucle des événements MIDI, contrôlé par un bouton physique (appui court / appui long)
- 🖥️ **Clavier virtuel sur PC** : le clavier AZERTY devient un clavier de piano qui envoie du MIDI USB à la Teensy

## Architecture

```
  PC (clavier AZERTY → keyboard_to_midi)
            │  USB MIDI
            ▼
   ┌──────────────────┐        I2S        ┌──────────────────┐
   │   Teensy 4.0     │ ────────────────▶ │  Audio Shield    │ ──▶ 🎧
   │  MidiHandler     │                   │  SGTL5000        │
   │  MyDsp ×2 (live  │                   └──────────────────┘
   │   + looper)      │
   │  Looper, Button  │ ◀── bouton physique (debounce, appui long)
   └──────────────────┘
```

Chaque module a une seule responsabilité :

| Module | Rôle | Notion mise en œuvre |
|---|---|---|
| `MyDsp` | Génère l'audio échantillon par échantillon (oscillateurs, ADSR, mixage, écho) | DSP temps réel, `AudioStream` |
| `MidiHandler` | Reçoit et route les messages MIDI (notes, Program Change, Control Change) | Protocole MIDI |
| `Looper` | Enregistre et rejoue les événements MIDI dans une seconde instance du synthé | Machine à états |
| `Button` | Lit le bouton avec anti-rebond, distingue appui court et appui long | Systèmes embarqués |
| `config.h` | Constantes partagées (voix, numéros de CC, broches, temporisations) | Source unique de vérité |
| `main.cpp` | Câble les modules et le graphe audio (deux synthés mixés en stéréo) | Séparation des responsabilités |

## Ce que j'ai fait

- Rédaction du cahier des charges et de la feuille de route en 5 phases (validation MIDI, moteur audio, presets, interface, finalisation)
- Validation de la chaîne **PC → MIDI USB → Teensy → Audio Shield** et des tests de polyphonie (4 puis 8 voix)
- Moteur audio et presets : synthèse polyphonique, enveloppes, mapping Program Change → presets et Control Change → gain / écho / timbre
- Documentation du projet et support de soutenance

## Lancer le projet

**Prérequis** : Teensy 4.0 + Audio Shield, [PlatformIO](https://platformio.org/) (ou Arduino IDE + Teensyduino), et sur macOS `brew install sdl2 rtmidi cmake pkg-config` pour l'outil clavier.

```bash
# 1. Flasher le firmware (USB type : MIDI + Serial)
pio run -e teensy40 -t upload

# 2. Compiler l'outil clavier → MIDI et tout lancer
python3 launcher_synth.py
```

Une version Arduino IDE du firmware est aussi disponible dans [`arduino_ide/Synteensyzer`](arduino_ide/Synteensyzer).

## Organisation du dépôt

```
src/ include/          Firmware Teensy (PlatformIO)
arduino_ide/           Même firmware, version Arduino IDE
tools/keyboard_to_midi_cpp/   Outil PC : clavier → MIDI USB (C++, SDL2, RtMidi)
launcher_synth.py      Compile l'outil, détecte la Teensy et lance le tout
docs/                  Mapping MIDI, organisation du projet, présentation
```

## Compétences mobilisées

Programmation embarquée C++ · traitement numérique du signal temps réel · protocole MIDI · architecture logicielle modulaire · machines à états · gestion de projet en binôme (Git)

## Équipe

- **Léo Le Calvé** — [GitHub](https://github.com/leo09225) · [LinkedIn](https://www.linkedin.com/in/l%C3%A9o-le-calv%C3%A9-b93a1b387/)
- **Victor Chachereau** — [GitHub](https://github.com/victorchach)
