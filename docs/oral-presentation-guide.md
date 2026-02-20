# Synteensyzer — Oral Presentation Guide

Guide for presenting the Synteensyzer project during a 1st-year engineering school oral exam (~10-15 min).

---

## 1. Opening Hook (30 sec)

Start with a **live demo** of 30 seconds: play a few notes, switch presets, record a loop. This grabs attention and sets the context immediately. If a live demo is not possible, a short video works just as well.

## 2. The "What" — Project Overview (2 min)

> "This is a real-time polyphonic synthesiser running on a Teensy 4.0 microcontroller. The PC sends MIDI notes over USB, the Teensy computes the audio and outputs it to headphones through an audio codec."

Show the architecture diagram — a single slide is enough:

```
  PC (MIDI keyboard)
        | USB MIDI
        v
   +------------+
   |  Teensy    |     I2S      +--------------+
   |   4.0      |------------->| Audio Shield |---> headphones
   | (600 MHz)  |              |  SGTL5000    |
   +------------+              +--------------+
```

List the **features** in one sentence each:
- 8 simultaneous polyphonic voices
- 4 timbres (sine, organ, electric, pad)
- ADSR envelope per voice
- Configurable echo effect (delay)
- Looper with record / playback in a loop
- Physical button control (short / long press)

## 3. The "How" — Software Architecture (3-4 min)

This is the **technical section** where the jury expects rigour. Present the modular architecture by showing how each file has **one single responsibility**:

| Module           | Role                                   | Key Concept                              |
|------------------|----------------------------------------|------------------------------------------|
| `MyDsp`          | Generates audio sample by sample       | Real-time DSP, AudioStream               |
| `Looper`         | Records and replays MIDI events        | State machine                            |
| `MidiHandler`    | Routes MIDI messages                   | Communication protocol                   |
| `Button`         | Reads a physical button with debounce  | Debounce, embedded systems               |
| `main.cpp`       | Connects all modules together          | Architecture, separation of concerns     |
| `config.h`       | Shared constants                       | Single source of truth                   |

Key point to highlight: **`main.cpp` is 85 lines** while the total project is ~800 lines. This shows that complexity is well distributed across focused modules.

## 4. Technical Deep Dive — Pick 2-3 Strengths (3-4 min)

Don't try to explain everything. Pick 2 or 3 topics that are well understood and go deeper. Suggestions:

### a) Real-Time Audio Synthesis (MyDsp)

- `update()` is called by a hardware interrupt every 2.9 ms (128 samples at 44.1 kHz)
- For each sample: loop over 8 voices -> oscillator -> envelope -> mix -> echo -> output
- Real-time constraint: if the computation exceeds 2.9 ms, the audio cracks/glitches
- Presets use different techniques: sine wavetable lookup, additive synthesis, noise burst

### b) The Looper (State Machine)

- 4 states: EMPTY -> RECORDING -> PLAYING -> STOPPED

```
         short press        short press       short press
EMPTY ──────────────> RECORDING ──────────> PLAYING ──────────> STOPPED
  ^                                            |                   |
  |              long press (3s)               |   short press     |
  +--------------------------------------------+-------------------+
                         CLEAR
```

- Records events as `{timestamp, type, note}` and replays them at the right time
- The preset is "frozen" at the start of recording: the loop keeps its timbre even if you switch sounds while playing live
- Good example of the **State design pattern** taught in software courses

### c) ISR / Main Loop Concurrency

- `update()` runs in an interrupt context, `noteOn()`/`noteOff()` are called from `loop()`
- Without protection -> data race -> crash or audio glitch
- Solution: `__disable_irq()` / `__enable_irq()` around critical sections
- Direct link to embedded systems / concurrency courses

## 5. Challenges Encountered and Solutions (2 min)

The jury appreciates **honesty** and **problem-solving methodology**. Possible examples:

- "Initially everything was in a single 400-line file -> we restructured into separate modules"
- "We experienced audio glitches -> we discovered the ISR concurrency problem"
- "The looper was losing the correct sound -> we added preset freezing"
- "Frequency calculation was slowing down the DSP -> we cached it in `noteOn()` instead of recomputing every sample"

## 6. Conclusion + Perspectives (1 min)

- Recap: working synthesiser, real-time, clean architecture
- Possible extensions:
  - True stereo output (L/R panning)
  - More presets / user-defined timbres
  - ADSR parameters controllable via MIDI CC
  - Loop quantisation (snap to tempo grid)
  - PC graphical interface

---

## Practical Tips

### Preparation

- Prepare **one diagram per concept** rather than text-heavy slides
- Know the **key numbers**: 44,100 Hz, 128 samples/block, 8 voices, 600 MHz CPU, 512 KB RAM
- Re-read each file and be able to explain **why** each design choice was made (not just "how")

### Facing the Jury

- If a question comes up and you don't know the answer: *"I haven't explored that point, but I think that..."* is always better than bluffing
- Show that you understand the **trade-offs** (e.g., "we chose direct references instead of callbacks because it's more readable for a project of this size")
- Use **technical vocabulary**: ISR, polling, debounce, state machine, ring buffer, wavetable, ADSR — it demonstrates mastery

### What Will Make the Difference

In a 1st-year programme, many projects stay on the surface. What impresses a jury is seeing that a student understands **the layers**: the MIDI protocol, the I2S bus, sample-by-sample DSP, the audio interrupt, the real-time constraint. Even if not everything is perfect, showing that you see the system **end to end** is what makes the difference.
