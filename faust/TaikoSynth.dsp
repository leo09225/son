import("stdfaust.lib");

// =======================================================
// TaikoSynth.dsp (adapté de PolyPiano.dsp)
// Compatible avec MyDsp (MapUI) : paramètres attendus
//   - "freq", "gain", "gate"
// + extras optionnels : preset, ADSR, echo, masterGain
// =======================================================

declare name "TaikoSynth (PolyPiano adapted)";
declare author "Projet SON";
declare version "0.2";

// -------------------------------------------------------
// PARAMÈTRES PRINCIPAUX (CEUX QUE MyDsp SET)
// -------------------------------------------------------
// IMPORTANT: ces noms DOIVENT correspondre à MyDsp::setParamValue(...)
freq = nentry("freq", 200, 50, 2000, 0.01) : si.smoo;
gain = nentry("gain", 0.50, 0, 1, 0.001) : si.smoo;      // (vélocité/volume depuis PC)
gate = button("gate") : si.smoo;                         // note ON/OFF

// -------------------------------------------------------
// PARAMÈTRES OPTIONNELS (tu peux les laisser par défaut)
// -------------------------------------------------------
preset = nentry("preset", 0, 0, 3, 1);                   // 0..3

// Gain général (volume master)
masterGain = hslider("masterGain", 0.35, 0, 1, 0.01);

// ADSR (par note) – contrôle de l’enveloppe
atk = hslider("env/attack",  0.01, 0.001, 1.0, 0.001);
dec = hslider("env/decay",   0.10, 0.001, 2.0, 0.001);
sus = hslider("env/sustain", 0.70, 0.0,   1.0, 0.001);
rel = hslider("env/release", 0.20, 0.001, 3.0, 0.001);

// Enveloppe (utilise gate) + amplitude globale (gain)
env = en.adsr(atk, dec, sus, rel, gate) * gain;

// -------------------------------------------------------
// ECHO (global) – optionnel
// -------------------------------------------------------
echoOn  = checkbox("fx/echoOn");
echoMix = hslider("fx/echoMix", 0.25, 0, 1, 0.01);
echoFb  = hslider("fx/echoFb",  0.45, 0, 0.85, 0.01);
echoMs  = hslider("fx/echoMs",  280,  30, 800, 1);

SR = ma.SR;
delaySamps = int(echoMs * SR / 1000.0);

// Echo feedback stable : y[n] = x[n] + fb * y[n-D]
echoLine(x)  = x : (+ ~ ( @(delaySamps) : *(echoFb) ));
applyEcho(x) = (1.0 - echoMix)*x + echoMix*echoLine(x);
fx(x)        = (echoOn == 1) ? applyEcho(x) : x;

// Sécurité : soft clipping
safe(x) = tanh(x);

// -------------------------------------------------------
// PRESETS (0..3) – synthèses de timbre
// -------------------------------------------------------

// Preset 0 : sinusoïdal pur
p0 = os.osc(freq);

// Preset 1 : additif (fondamentale + harmoniques)
p1 = os.osc(freq)
   + 0.50 * os.osc(2.0 * freq)
   + 0.30 * os.osc(3.0 * freq)
   + 0.20 * os.osc(1.5 * freq);

// Preset 2 : “piano électrique” (transient bruit + partiels)
transient = (no.noise * 0.15) * en.adsr(0.001, 0.03, 0.0, 0.03, gate);
p2 = os.osc(freq)
   + 0.35 * os.osc(2.0 * freq)
   + 0.15 * os.osc(4.0 * freq)
   + transient;

// Preset 3 : pad doux (désaccord léger + lowpass)
det = 0.004;
pad = 0.6 * os.osc(freq * (1.0 - det))
    + 0.6 * os.osc(freq * (1.0 + det));
p3 = fi.lowpass(1, 1800, pad);

// Sélection du preset (sans switch/case, compatible partout)
is0 = (preset == 0);
is1 = (preset == 1);
is2 = (preset == 2);
is3 = (preset == 3);

osc = is0*p0 + is1*p1 + is2*p2 + is3*p3;

// Voix finale
voice = osc * env;

// Sortie finale stéréo
process = voice : *(masterGain) : fx : safe <: _,_;
