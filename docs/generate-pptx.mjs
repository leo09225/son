import PptxGenJS from "pptxgenjs";

const pptx = new PptxGenJS();

// ============================================================
// INSA Lyon inspired color palette
// ============================================================
const C = {
  red:       "E2001A",   // INSA primary red
  darkRed:   "B30015",   // darker accent
  dark:      "1A1A2E",   // near-black for text
  gray:      "4A4A5A",   // body text
  lightGray: "F0F0F5",   // slide backgrounds
  white:     "FFFFFF",
  accent:    "2D6A9F",   // blue accent for diagrams
  green:     "2E8B57",   // for success/solution items
};

// ============================================================
// Presentation metadata
// ============================================================
pptx.author = "INSA Lyon";
pptx.title = "Synteensyzer — Real-Time Polyphonic Synthesiser";
pptx.subject = "Projet 1A";
pptx.layout = "LAYOUT_16x9";

// ============================================================
// Helper: add a red top bar (INSA style)
// ============================================================
function addTopBar(slide) {
  slide.addShape(pptx.ShapeType.rect, {
    x: 0, y: 0, w: 10, h: 0.55,
    fill: { color: C.red },
  });
}

// ============================================================
// Helper: add footer with slide number
// ============================================================
function addFooter(slide, slideNum, totalSlides) {
  slide.addText(`Synteensyzer — INSA Lyon 1A`, {
    x: 0.5, y: 5.15, w: 7, h: 0.35,
    fontSize: 9, color: C.gray, fontFace: "Arial",
  });
  slide.addText(`${slideNum} / ${totalSlides}`, {
    x: 8.5, y: 5.15, w: 1, h: 0.35,
    fontSize: 9, color: C.gray, fontFace: "Arial", align: "right",
  });
}

// ============================================================
// Helper: section title slide (red background)
// ============================================================
function addSectionSlide(title, subtitle, slideNum, total) {
  const slide = pptx.addSlide();
  slide.background = { fill: C.red };

  slide.addText(title, {
    x: 0.8, y: 1.5, w: 8.4, h: 1.2,
    fontSize: 40, fontFace: "Arial", bold: true,
    color: C.white, align: "left",
  });

  if (subtitle) {
    slide.addText(subtitle, {
      x: 0.8, y: 2.8, w: 8.4, h: 0.8,
      fontSize: 20, fontFace: "Arial",
      color: "FFCCCC", align: "left",
    });
  }

  // White line accent
  slide.addShape(pptx.ShapeType.rect, {
    x: 0.8, y: 4.2, w: 2.5, h: 0.04,
    fill: { color: C.white },
  });

  slide.addText(`${slideNum} / ${total}`, {
    x: 8.5, y: 5.15, w: 1, h: 0.35,
    fontSize: 9, color: "FFAAAA", fontFace: "Arial", align: "right",
  });

  return slide;
}

const TOTAL = 11;

// ============================================================
// SLIDE 1 — Title
// ============================================================
{
  const slide = pptx.addSlide();
  slide.background = { fill: C.dark };

  // Red accent bar at top
  slide.addShape(pptx.ShapeType.rect, {
    x: 0, y: 0, w: 10, h: 0.08,
    fill: { color: C.red },
  });

  // Project name
  slide.addText("SYNTEENSYZER", {
    x: 0.8, y: 1.2, w: 8.4, h: 1.0,
    fontSize: 48, fontFace: "Arial", bold: true,
    color: C.white, align: "left", charSpacing: 4,
  });

  // Subtitle
  slide.addText("Real-Time Polyphonic Synthesiser on Teensy 4.0", {
    x: 0.8, y: 2.2, w: 8.4, h: 0.6,
    fontSize: 22, fontFace: "Arial",
    color: C.red, align: "left",
  });

  // Red line
  slide.addShape(pptx.ShapeType.rect, {
    x: 0.8, y: 3.1, w: 3.0, h: 0.05,
    fill: { color: C.red },
  });

  // Author & context
  slide.addText("Projet 1A — INSA Lyon", {
    x: 0.8, y: 3.5, w: 8.4, h: 0.5,
    fontSize: 16, fontFace: "Arial",
    color: "AAAABB", align: "left",
  });

  slide.addText("2025 – 2026", {
    x: 0.8, y: 4.0, w: 8.4, h: 0.5,
    fontSize: 14, fontFace: "Arial",
    color: "777788", align: "left",
  });
}

// ============================================================
// SLIDE 2 — Agenda
// ============================================================
{
  const slide = pptx.addSlide();
  slide.background = { fill: C.white };
  addTopBar(slide);
  addFooter(slide, 2, TOTAL);

  slide.addText("Agenda", {
    x: 0.8, y: 0.8, w: 8.4, h: 0.7,
    fontSize: 32, fontFace: "Arial", bold: true, color: C.dark,
  });

  const items = [
    { num: "01", text: "Live Demo", time: "30 sec" },
    { num: "02", text: "Project Overview", time: "2 min" },
    { num: "03", text: "System Architecture", time: "2 min" },
    { num: "04", text: "Software Architecture", time: "2 min" },
    { num: "05", text: "Technical Deep Dive", time: "4 min" },
    { num: "06", text: "Challenges & Solutions", time: "2 min" },
    { num: "07", text: "Conclusion & Perspectives", time: "1 min" },
  ];

  items.forEach((item, i) => {
    const y = 1.7 + i * 0.5;
    slide.addText(item.num, {
      x: 0.8, y, w: 0.6, h: 0.4,
      fontSize: 16, fontFace: "Arial", bold: true,
      color: C.red, align: "left",
    });
    slide.addText(item.text, {
      x: 1.5, y, w: 5.5, h: 0.4,
      fontSize: 16, fontFace: "Arial", color: C.dark, align: "left",
    });
    slide.addText(item.time, {
      x: 7.5, y, w: 2, h: 0.4,
      fontSize: 13, fontFace: "Arial", color: C.gray, align: "right",
    });
  });
}

// ============================================================
// SLIDE 3 — Live Demo
// ============================================================
addSectionSlide("Live Demo", "Play notes — Switch presets — Record a loop", 3, TOTAL);

// ============================================================
// SLIDE 4 — Project Overview
// ============================================================
{
  const slide = pptx.addSlide();
  slide.background = { fill: C.white };
  addTopBar(slide);
  addFooter(slide, 4, TOTAL);

  slide.addText("Project Overview", {
    x: 0.8, y: 0.8, w: 8.4, h: 0.7,
    fontSize: 32, fontFace: "Arial", bold: true, color: C.dark,
  });

  slide.addText(
    "A real-time polyphonic synthesiser running on a Teensy 4.0 microcontroller.\n" +
    "The PC sends MIDI notes over USB, the Teensy computes the sound\n" +
    "and outputs it to headphones through an audio codec.",
    {
      x: 0.8, y: 1.6, w: 8.4, h: 1.0,
      fontSize: 15, fontFace: "Arial", color: C.gray,
      align: "left", lineSpacingMultiple: 1.3,
    }
  );

  // Features - left column
  const features = [
    "8 simultaneous polyphonic voices",
    "4 timbres (sine, organ, electric, pad)",
    "ADSR envelope per voice",
    "Configurable echo/delay effect",
    "Looper (record / play / stop)",
    "Physical button (short / long press)",
  ];

  // Red bullet list
  features.forEach((f, i) => {
    const y = 2.9 + i * 0.38;
    slide.addShape(pptx.ShapeType.rect, {
      x: 1.0, y: y + 0.12, w: 0.15, h: 0.15,
      fill: { color: C.red },
    });
    slide.addText(f, {
      x: 1.35, y, w: 5, h: 0.38,
      fontSize: 14, fontFace: "Arial", color: C.dark, align: "left",
    });
  });

  // Tech stack box on the right
  slide.addShape(pptx.ShapeType.roundRect, {
    x: 6.8, y: 2.9, w: 2.7, h: 2.3,
    fill: { color: C.lightGray },
    rectRadius: 0.1,
  });
  slide.addText("Tech Stack", {
    x: 6.8, y: 3.0, w: 2.7, h: 0.4,
    fontSize: 14, fontFace: "Arial", bold: true, color: C.red, align: "center",
  });
  const techs = ["Teensy 4.0 (ARM 600 MHz)", "Audio Shield SGTL5000", "C++ / Arduino", "USB MIDI Protocol", "PlatformIO"];
  techs.forEach((t, i) => {
    slide.addText(t, {
      x: 7.0, y: 3.45 + i * 0.32, w: 2.5, h: 0.3,
      fontSize: 11, fontFace: "Arial", color: C.gray, align: "left",
    });
  });
}

// ============================================================
// SLIDE 5 — System Architecture
// ============================================================
{
  const slide = pptx.addSlide();
  slide.background = { fill: C.white };
  addTopBar(slide);
  addFooter(slide, 5, TOTAL);

  slide.addText("System Architecture", {
    x: 0.8, y: 0.8, w: 8.4, h: 0.7,
    fontSize: 32, fontFace: "Arial", bold: true, color: C.dark,
  });

  // PC Box
  slide.addShape(pptx.ShapeType.roundRect, {
    x: 0.8, y: 2.2, w: 2.2, h: 1.5,
    fill: { color: C.lightGray }, rectRadius: 0.1,
    line: { color: C.gray, width: 1 },
  });
  slide.addText("PC", {
    x: 0.8, y: 2.3, w: 2.2, h: 0.5,
    fontSize: 18, fontFace: "Arial", bold: true, color: C.dark, align: "center",
  });
  slide.addText("MIDI Keyboard\n(virtual or physical)", {
    x: 0.8, y: 2.8, w: 2.2, h: 0.7,
    fontSize: 11, fontFace: "Arial", color: C.gray, align: "center",
  });

  // Arrow 1: USB MIDI
  slide.addShape(pptx.ShapeType.rect, {
    x: 3.1, y: 2.9, w: 0.9, h: 0.05,
    fill: { color: C.red },
  });
  slide.addText("USB MIDI", {
    x: 3.0, y: 2.45, w: 1.1, h: 0.4,
    fontSize: 10, fontFace: "Arial", bold: true, color: C.red, align: "center",
  });

  // Teensy Box
  slide.addShape(pptx.ShapeType.roundRect, {
    x: 4.1, y: 1.9, w: 2.4, h: 2.1,
    fill: { color: C.red }, rectRadius: 0.1,
  });
  slide.addText("Teensy 4.0", {
    x: 4.1, y: 2.0, w: 2.4, h: 0.5,
    fontSize: 18, fontFace: "Arial", bold: true, color: C.white, align: "center",
  });
  slide.addText("MIDI Reception\nDSP Engine (C++)\n8-voice polyphony\nADSR + Echo", {
    x: 4.1, y: 2.5, w: 2.4, h: 1.3,
    fontSize: 11, fontFace: "Arial", color: "FFDDDD", align: "center",
    lineSpacingMultiple: 1.25,
  });

  // Arrow 2: I2S
  slide.addShape(pptx.ShapeType.rect, {
    x: 6.6, y: 2.9, w: 0.9, h: 0.05,
    fill: { color: C.accent },
  });
  slide.addText("I2S", {
    x: 6.5, y: 2.45, w: 1.1, h: 0.4,
    fontSize: 10, fontFace: "Arial", bold: true, color: C.accent, align: "center",
  });

  // Audio Shield Box
  slide.addShape(pptx.ShapeType.roundRect, {
    x: 7.6, y: 2.2, w: 2.0, h: 1.5,
    fill: { color: C.lightGray }, rectRadius: 0.1,
    line: { color: C.accent, width: 1.5 },
  });
  slide.addText("Audio Shield", {
    x: 7.6, y: 2.3, w: 2.0, h: 0.5,
    fontSize: 16, fontFace: "Arial", bold: true, color: C.dark, align: "center",
  });
  slide.addText("SGTL5000 DAC\nHeadphone output", {
    x: 7.6, y: 2.8, w: 2.0, h: 0.7,
    fontSize: 11, fontFace: "Arial", color: C.gray, align: "center",
  });

  // Key numbers at bottom
  slide.addShape(pptx.ShapeType.rect, {
    x: 0.8, y: 4.3, w: 8.8, h: 0.04,
    fill: { color: C.lightGray },
  });

  const stats = [
    { label: "Sample Rate", value: "44,100 Hz" },
    { label: "Block Size", value: "128 samples" },
    { label: "CPU", value: "600 MHz ARM" },
    { label: "RAM", value: "512 + 512 KB" },
  ];
  stats.forEach((s, i) => {
    const x = 0.8 + i * 2.3;
    slide.addText(s.value, {
      x, y: 4.45, w: 2.0, h: 0.35,
      fontSize: 16, fontFace: "Arial", bold: true, color: C.red, align: "left",
    });
    slide.addText(s.label, {
      x, y: 4.8, w: 2.0, h: 0.3,
      fontSize: 10, fontFace: "Arial", color: C.gray, align: "left",
    });
  });
}

// ============================================================
// SLIDE 6 — Software Architecture
// ============================================================
{
  const slide = pptx.addSlide();
  slide.background = { fill: C.white };
  addTopBar(slide);
  addFooter(slide, 6, TOTAL);

  slide.addText("Software Architecture", {
    x: 0.8, y: 0.8, w: 8.4, h: 0.7,
    fontSize: 32, fontFace: "Arial", bold: true, color: C.dark,
  });

  slide.addText("One file = One responsibility", {
    x: 0.8, y: 1.5, w: 8.4, h: 0.4,
    fontSize: 16, fontFace: "Arial", italic: true, color: C.red,
  });

  // Module cards
  const modules = [
    { name: "MyDsp", desc: "Audio engine\n8 voices + ADSR + Echo", concept: "Real-time DSP" },
    { name: "Looper", desc: "Record & replay\nMIDI events", concept: "State machine" },
    { name: "MidiHandler", desc: "Route MIDI\nmessages", concept: "Protocol routing" },
    { name: "Button", desc: "Debounced input\nshort/long press", concept: "Embedded I/O" },
    { name: "main.cpp", desc: "Wire modules\ntogether", concept: "Orchestration" },
    { name: "config.h", desc: "Shared\nconstants", concept: "Single source" },
  ];

  modules.forEach((m, i) => {
    const col = i % 3;
    const row = Math.floor(i / 3);
    const x = 0.8 + col * 3.05;
    const y = 2.1 + row * 1.6;

    // Card background
    slide.addShape(pptx.ShapeType.roundRect, {
      x, y, w: 2.8, h: 1.35,
      fill: { color: C.lightGray }, rectRadius: 0.08,
    });

    // Red left accent
    slide.addShape(pptx.ShapeType.rect, {
      x, y, w: 0.06, h: 1.35,
      fill: { color: C.red },
    });

    // Module name
    slide.addText(m.name, {
      x: x + 0.2, y, w: 2.5, h: 0.4,
      fontSize: 14, fontFace: "Arial", bold: true, color: C.dark, align: "left",
    });

    // Description
    slide.addText(m.desc, {
      x: x + 0.2, y: y + 0.38, w: 2.5, h: 0.5,
      fontSize: 11, fontFace: "Arial", color: C.gray, align: "left",
    });

    // Concept tag
    slide.addShape(pptx.ShapeType.roundRect, {
      x: x + 0.2, y: y + 0.95, w: 1.6, h: 0.28,
      fill: { color: C.red }, rectRadius: 0.14,
    });
    slide.addText(m.concept, {
      x: x + 0.2, y: y + 0.95, w: 1.6, h: 0.28,
      fontSize: 9, fontFace: "Arial", bold: true, color: C.white, align: "center",
    });
  });
}

// ============================================================
// SLIDE 7 — Deep Dive: Real-Time Audio
// ============================================================
{
  const slide = pptx.addSlide();
  slide.background = { fill: C.white };
  addTopBar(slide);
  addFooter(slide, 7, TOTAL);

  slide.addText("Real-Time Audio Synthesis", {
    x: 0.8, y: 0.8, w: 8.4, h: 0.7,
    fontSize: 32, fontFace: "Arial", bold: true, color: C.dark,
  });

  // Constraint callout
  slide.addShape(pptx.ShapeType.roundRect, {
    x: 0.8, y: 1.6, w: 8.8, h: 0.6,
    fill: { color: "FFF0F0" }, rectRadius: 0.08,
    line: { color: C.red, width: 1 },
  });
  slide.addText("Hard real-time constraint: update() must complete within 2.9 ms or the audio glitches", {
    x: 1.0, y: 1.6, w: 8.4, h: 0.6,
    fontSize: 14, fontFace: "Arial", bold: true, color: C.red, align: "left",
  });

  // Pipeline diagram
  slide.addText("Audio Pipeline (per block of 128 samples)", {
    x: 0.8, y: 2.45, w: 8.4, h: 0.4,
    fontSize: 14, fontFace: "Arial", bold: true, color: C.dark,
  });

  const steps = ["8 Voices", "Oscillator", "ADSR\nEnvelope", "Mix", "Echo\n(delay)", "Soft Clip", "I2S Out"];
  steps.forEach((s, i) => {
    const x = 0.6 + i * 1.3;
    slide.addShape(pptx.ShapeType.roundRect, {
      x, y: 2.95, w: 1.1, h: 0.7,
      fill: { color: i === 0 ? C.red : (i === steps.length - 1 ? C.accent : C.dark) },
      rectRadius: 0.06,
    });
    slide.addText(s, {
      x, y: 2.95, w: 1.1, h: 0.7,
      fontSize: 10, fontFace: "Arial", bold: true, color: C.white, align: "center",
      valign: "middle",
    });
    // Arrow between boxes
    if (i < steps.length - 1) {
      slide.addText("\u25B6", {
        x: x + 1.1, y: 3.05, w: 0.2, h: 0.5,
        fontSize: 10, color: C.gray, align: "center",
      });
    }
  });

  // 4 presets
  slide.addText("4 Timbres (Presets)", {
    x: 0.8, y: 3.95, w: 8.4, h: 0.4,
    fontSize: 14, fontFace: "Arial", bold: true, color: C.dark,
  });

  const presets = [
    { name: "Sine", desc: "Pure wavetable lookup" },
    { name: "Organ", desc: "Additive: f + 2f + 3f + 1.5f" },
    { name: "Electric", desc: "Harmonics + noise transient" },
    { name: "Pad", desc: "Detuned sines + low-pass filter" },
  ];
  presets.forEach((p, i) => {
    const x = 0.8 + i * 2.3;
    slide.addShape(pptx.ShapeType.roundRect, {
      x, y: 4.4, w: 2.1, h: 0.65,
      fill: { color: C.lightGray }, rectRadius: 0.06,
    });
    slide.addText(p.name, {
      x, y: 4.4, w: 2.1, h: 0.3,
      fontSize: 12, fontFace: "Arial", bold: true, color: C.red, align: "center",
    });
    slide.addText(p.desc, {
      x, y: 4.7, w: 2.1, h: 0.3,
      fontSize: 9, fontFace: "Arial", color: C.gray, align: "center",
    });
  });
}

// ============================================================
// SLIDE 8 — Deep Dive: Looper State Machine
// ============================================================
{
  const slide = pptx.addSlide();
  slide.background = { fill: C.white };
  addTopBar(slide);
  addFooter(slide, 8, TOTAL);

  slide.addText("The Looper — State Machine", {
    x: 0.8, y: 0.8, w: 8.4, h: 0.7,
    fontSize: 32, fontFace: "Arial", bold: true, color: C.dark,
  });

  // State boxes
  const states = [
    { name: "EMPTY", x: 0.8, color: C.gray },
    { name: "RECORDING", x: 3.1, color: C.red },
    { name: "PLAYING", x: 5.4, color: C.green },
    { name: "STOPPED", x: 7.7, color: C.dark },
  ];

  states.forEach((s) => {
    slide.addShape(pptx.ShapeType.roundRect, {
      x: s.x, y: 2.0, w: 2.0, h: 0.7,
      fill: { color: s.color }, rectRadius: 0.1,
    });
    slide.addText(s.name, {
      x: s.x, y: 2.0, w: 2.0, h: 0.7,
      fontSize: 13, fontFace: "Arial", bold: true, color: C.white, align: "center",
    });
  });

  // Arrows between states
  const arrows = [
    { x: 2.85, label: "short\npress" },
    { x: 5.15, label: "short\npress" },
    { x: 7.45, label: "short\npress" },
  ];
  arrows.forEach((a) => {
    slide.addShape(pptx.ShapeType.rect, {
      x: a.x, y: 2.3, w: 0.22, h: 0.04,
      fill: { color: C.red },
    });
    slide.addText(a.label, {
      x: a.x - 0.15, y: 1.55, w: 0.6, h: 0.45,
      fontSize: 8, fontFace: "Arial", color: C.red, align: "center",
    });
  });

  // Long press arrow (bottom)
  slide.addShape(pptx.ShapeType.rect, {
    x: 0.8, y: 3.2, w: 8.9, h: 0.04,
    fill: { color: C.darkRed },
  });
  slide.addText("long press (3s) \u2192 CLEAR \u2192 back to EMPTY", {
    x: 2.5, y: 3.3, w: 5, h: 0.35,
    fontSize: 11, fontFace: "Arial", bold: true, color: C.darkRed, align: "center",
  });

  // Key features
  slide.addText("Key Design Decisions", {
    x: 0.8, y: 3.9, w: 8.4, h: 0.4,
    fontSize: 16, fontFace: "Arial", bold: true, color: C.dark,
  });

  const features = [
    "Events stored as {timestamp, type, note, velocity}",
    "Replayed in real-time by comparing elapsed time to event timestamps",
    'Preset is "frozen" at record start — loop keeps its timbre',
    "Fixed buffer of 2048 events (no dynamic allocation on embedded)",
  ];
  features.forEach((f, i) => {
    const y = 4.35 + i * 0.3;
    slide.addShape(pptx.ShapeType.rect, {
      x: 1.0, y: y + 0.08, w: 0.12, h: 0.12,
      fill: { color: C.red },
    });
    slide.addText(f, {
      x: 1.3, y, w: 8, h: 0.3,
      fontSize: 12, fontFace: "Arial", color: C.gray, align: "left",
    });
  });
}

// ============================================================
// SLIDE 9 — Deep Dive: ISR Concurrency
// ============================================================
{
  const slide = pptx.addSlide();
  slide.background = { fill: C.white };
  addTopBar(slide);
  addFooter(slide, 9, TOTAL);

  slide.addText("ISR / Main Loop Concurrency", {
    x: 0.8, y: 0.8, w: 8.4, h: 0.7,
    fontSize: 32, fontFace: "Arial", bold: true, color: C.dark,
  });

  // Problem side
  slide.addText("The Problem", {
    x: 0.8, y: 1.6, w: 4, h: 0.4,
    fontSize: 18, fontFace: "Arial", bold: true, color: C.red,
  });

  // Two thread boxes
  slide.addShape(pptx.ShapeType.roundRect, {
    x: 0.8, y: 2.1, w: 3.8, h: 0.7,
    fill: { color: "FFF0F0" }, rectRadius: 0.08,
  });
  slide.addText("Audio ISR (every 2.9 ms)\nupdate() reads voices[], preset, echo params", {
    x: 0.9, y: 2.1, w: 3.6, h: 0.7,
    fontSize: 11, fontFace: "Arial", color: C.dark, align: "left",
  });

  slide.addShape(pptx.ShapeType.roundRect, {
    x: 0.8, y: 2.95, w: 3.8, h: 0.7,
    fill: { color: "F0F0FF" }, rectRadius: 0.08,
  });
  slide.addText("Main loop() (continuous)\nnoteOn/Off/setters write voices[], preset, echo", {
    x: 0.9, y: 2.95, w: 3.6, h: 0.7,
    fontSize: 11, fontFace: "Arial", color: C.dark, align: "left",
  });

  // Conflict indicator
  slide.addText("\u26A0  Same data, no protection = DATA RACE", {
    x: 0.8, y: 3.8, w: 4.0, h: 0.35,
    fontSize: 12, fontFace: "Arial", bold: true, color: C.red, align: "left",
  });

  // Solution side
  slide.addText("The Solution", {
    x: 5.5, y: 1.6, w: 4, h: 0.4,
    fontSize: 18, fontFace: "Arial", bold: true, color: C.green,
  });

  slide.addShape(pptx.ShapeType.roundRect, {
    x: 5.5, y: 2.1, w: 4.0, h: 2.2,
    fill: { color: C.lightGray }, rectRadius: 0.08,
  });

  slide.addText(
    "__disable_irq();\n" +
    "\n" +
    "// ... modify shared state ...\n" +
    "// (voices, preset, gain, echo)\n" +
    "\n" +
    "__enable_irq();",
    {
      x: 5.7, y: 2.2, w: 3.6, h: 1.8,
      fontSize: 13, fontFace: "Courier New", color: C.dark, align: "left",
      lineSpacingMultiple: 1.2,
    }
  );

  slide.addText("Standard Teensy Audio Library pattern.\nCritical section is tiny (< 1 \u00b5s) — no impact on audio latency.", {
    x: 5.5, y: 4.4, w: 4.2, h: 0.6,
    fontSize: 11, fontFace: "Arial", color: C.gray, align: "left",
  });
}

// ============================================================
// SLIDE 10 — Challenges & Solutions
// ============================================================
{
  const slide = pptx.addSlide();
  slide.background = { fill: C.white };
  addTopBar(slide);
  addFooter(slide, 10, TOTAL);

  slide.addText("Challenges & Solutions", {
    x: 0.8, y: 0.8, w: 8.4, h: 0.7,
    fontSize: 32, fontFace: "Arial", bold: true, color: C.dark,
  });

  const challenges = [
    {
      problem: "Monolithic code (400 lines in one file)",
      solution: "Restructured into 5 focused modules",
    },
    {
      problem: "Audio glitches during note changes",
      solution: "Added ISR protection (__disable_irq / __enable_irq)",
    },
    {
      problem: "Looper lost timbre on preset switch",
      solution: 'Preset "freeze" at recording start',
    },
    {
      problem: "DSP too slow (powf() called per sample)",
      solution: "Cached phaseInc in noteOn() — compute once",
    },
  ];

  challenges.forEach((c, i) => {
    const y = 1.7 + i * 0.9;

    // Problem
    slide.addShape(pptx.ShapeType.roundRect, {
      x: 0.8, y, w: 4.2, h: 0.7,
      fill: { color: "FFF5F5" }, rectRadius: 0.06,
    });
    slide.addShape(pptx.ShapeType.rect, {
      x: 0.8, y, w: 0.06, h: 0.7,
      fill: { color: C.red },
    });
    slide.addText(c.problem, {
      x: 1.05, y, w: 3.8, h: 0.7,
      fontSize: 13, fontFace: "Arial", color: C.dark, align: "left", valign: "middle",
    });

    // Arrow
    slide.addText("\u2192", {
      x: 5.05, y, w: 0.4, h: 0.7,
      fontSize: 18, fontFace: "Arial", color: C.red, align: "center", valign: "middle",
    });

    // Solution
    slide.addShape(pptx.ShapeType.roundRect, {
      x: 5.5, y, w: 4.1, h: 0.7,
      fill: { color: "F0FFF0" }, rectRadius: 0.06,
    });
    slide.addShape(pptx.ShapeType.rect, {
      x: 5.5, y, w: 0.06, h: 0.7,
      fill: { color: C.green },
    });
    slide.addText(c.solution, {
      x: 5.75, y, w: 3.7, h: 0.7,
      fontSize: 13, fontFace: "Arial", color: C.dark, align: "left", valign: "middle",
    });
  });
}

// ============================================================
// SLIDE 11 — Conclusion & Perspectives
// ============================================================
{
  const slide = pptx.addSlide();
  slide.background = { fill: C.dark };

  slide.addShape(pptx.ShapeType.rect, {
    x: 0, y: 0, w: 10, h: 0.08,
    fill: { color: C.red },
  });

  slide.addText("Conclusion", {
    x: 0.8, y: 0.6, w: 8.4, h: 0.7,
    fontSize: 36, fontFace: "Arial", bold: true, color: C.white,
  });

  // What we built
  const results = [
    "Fully working polyphonic synthesiser",
    "Real-time audio on embedded hardware",
    "Clean modular architecture (5 modules, 800 lines)",
    "MIDI-controlled with looper functionality",
  ];
  results.forEach((r, i) => {
    const y = 1.5 + i * 0.4;
    slide.addText("\u2713", {
      x: 0.8, y, w: 0.4, h: 0.35,
      fontSize: 16, fontFace: "Arial", bold: true, color: C.red, align: "center",
    });
    slide.addText(r, {
      x: 1.3, y, w: 8, h: 0.35,
      fontSize: 15, fontFace: "Arial", color: C.white, align: "left",
    });
  });

  // Perspectives
  slide.addShape(pptx.ShapeType.rect, {
    x: 0.8, y: 3.3, w: 3.0, h: 0.04,
    fill: { color: C.red },
  });

  slide.addText("Perspectives", {
    x: 0.8, y: 3.5, w: 8.4, h: 0.5,
    fontSize: 20, fontFace: "Arial", bold: true, color: C.red,
  });

  const perspectives = [
    "True stereo output (L/R panning)",
    "ADSR controllable via MIDI CC",
    "Loop quantisation (snap to tempo)",
    "PC graphical interface",
  ];
  perspectives.forEach((p, i) => {
    const y = 4.1 + i * 0.35;
    slide.addText("\u2192  " + p, {
      x: 1.0, y, w: 8, h: 0.3,
      fontSize: 13, fontFace: "Arial", color: "AAAACC", align: "left",
    });
  });

  addFooter(slide, 11, TOTAL);
}

// ============================================================
// Export
// ============================================================
const outPath = "docs/synteensyzer-presentation.pptx";
pptx.writeFile({ fileName: outPath })
  .then(() => console.log(`Presentation saved to ${outPath}`))
  .catch((err) => console.error(err));
