import("stdfaust.lib");
// Faust(ce doosier) génère le code Teensy
// =======================================================
// PolyPiano.dsp
// Synthétiseur polyphonique (8 voix) piloté en MIDI
// Cible : Teensy + Audio Shield
//
//Un langage de description DSP qui génère automatiquement du C++ audio temps réel

// Le DSP (audio) est entièrement décrit en FAUST.
// L’interaction (clavier PC, boutons, UI) se fait via MIDI.
// =======================================================

declare name "PolyPiano";
declare author "Projet SON";
declare version "0.1";

// =======================================================
// -------- PARAMÈTRES MIDI PAR VOIX ----------------------
// Ces paramètres sont dupliqués automatiquement
// pour chaque voix par le moteur polyphonique.
// =======================================================

// Fréquence de la note (déduite de la touche MIDI)
freq = hslider("freq[midi:key]", 440, 20, 2000, 0.01);

// Gate : 1 quand la note est appuyée, 0 quand relâchée
gate = button("gate[midi:gate]");

// Vélocité MIDI (intensité de l’appui)
vel  = hslider("vel[midi:velocity]", 0.8, 0, 1, 0.01);

// Sélection du preset (0 à 3)
// piloté par Program Change MIDI
preset = nentry("preset[midi:pgm]", 0, 0, 3, 1);


// =======================================================
// -------- PARAMÈTRES GLOBAUX -----------------------------
// Appliqués après le mixage des voix
// =======================================================

// Gain général (volume master)
// CC7 = standard MIDI pour le volume
master = hslider("masterGain[midi:ctrl 7]", 0.35, 0, 1, 0.01);

// =======================================================
// -------- ENVELOPPE ADSR (PAR VOIX) ---------------------
// Contrôle l’évolution de l’amplitude dans le temps
// =======================================================

atk = hslider("env/attack",  0.01, 0.001, 1.0, 0.001); // montée
dec = hslider("env/decay",   0.10, 0.001, 2.0, 0.001); // descente
sus = hslider("env/sustain", 0.70, 0.0,   1.0, 0.001); // niveau maintenu
rel = hslider("env/release", 0.20, 0.001, 3.0, 0.001); // relâchement

// Enveloppe ADSR appliquée à chaque voix
env = en.adsr(atk, dec, sus, rel, gate) * vel;

// =======================================================
// -------- PARAMÈTRES ECHO (GLOBAUX) ---------------------
// =======================================================
//"hslider("nom", valeur_initiale, valeur_min, valeur_max, pas)"

master  = hslider("masterGain[midi:ctrl 7]", 0.35, 0, 1, 0.01);   // Volume standard
echoOn  = checkbox("fx/echoOn[midi:ctrl 80]");                    // Toggle echo
echoMix = hslider("fx/echoMix[midi:ctrl 91]", 0.25, 0, 1, 0.01);  // Reverb/FX mix standard-ish
echoFb  = hslider("fx/echoFb[midi:ctrl 93]", 0.45, 0, 0.85, 0.01);// Chorus/FX depth standard-ish
echoMs  = hslider("fx/echoMs[midi:ctrl 94]", 280, 30, 800, 1);    // FX param (delay)


// Fréquence d’échantillonnage
SR = ma.SR;

// Conversion millisecondes → échantillons
delaySamps = int(echoMs * SR / 1000.0);

// =======================================================
// -------- DÉFINITION DES PRESETS ------------------------
// Chaque preset correspond à une "sonorité de piano"
// =======================================================

// Preset 0 : piano sinusoïdal pur
p0 = os.osc(freq);

// Preset 1 : piano additif (fondamentale + harmoniques)
p1 = os.osc(freq)
   + 0.50 * os.osc(2.0 * freq)
   + 0.30 * os.osc(3.0 * freq)
   + 0.20 * os.osc(1.5 * freq);

// Preset 2 : piano électrique
// Attaque brillante + caractère percussif
transient = (no.noise * 0.15)
          * en.adsr(0.001, 0.03, 0.0, 0.03, gate);

p2 = os.osc(freq)
   + 0.35 * os.osc(2.0 * freq)
   + 0.15 * os.osc(4.0 * freq)
   + transient;

// Preset 3 : piano doux / pad
// Léger désaccord + filtrage passe-bas
det = 0.004;
pad = 0.6 * os.osc(freq * (1.0 - det))
    + 0.6 * os.osc(freq * (1.0 + det));

p3 = fi.lowpass(1, 1800, pad);

// Sélection du preset actif
is0 = (preset == 0);
is1 = (preset == 1);
is2 = (preset == 2);
is3 = (preset == 3);

// Oscillateur de la voix courante
voiceOsc = is0*p0 + is1*p1 + is2*p2 + is3*p3;

// Signal final d’une voix (oscillateur × enveloppe)
voice = voiceOsc * env;

// =======================================================
// -------- ECHO AVEC FEEDBACK STABLE --------------------
// y[n] = x[n] + feedback * y[n - D]
// =======================================================

echoLine(x) = x : (+ ~ ( @(delaySamps) : *(echoFb) ));

applyEcho(x) = (1.0 - echoMix)*x + echoMix*echoLine(x);

// Activation conditionnelle de l’echo
fx(x) = (echoOn == 1) ? applyEcho(x) : x;

// Sécurité : soft clipping pour éviter la saturation
safe(x) = tanh(x);

// =======================================================
// -------- SORTIE FINALE --------------------------------
// =======================================================

// Chaque voix est mixée automatiquement par le moteur polyphonique
// puis traitée par le gain, l’echo et le soft-clip
process = voice : *(master) : fx : safe <: _,_;


// =======================================================
// ---------------------- TEST RAPIDE ---------------------
// =======================================================
//
// 1) Compiler avec polyphonie 8 voix (Arduino)
//    → option de compilation : -nvoices 8
//
// 2) Brancher un clavier MIDI virtuel (VMPK par ex.)
//
// 3) Tester les presets :
//    preset = 0 → piano sinusoïdal
//    preset = 1 → piano additif
//    preset = 2 → piano électrique
//    preset = 3 → piano doux / pad
//
// 4) Tester l’echo :
//    - activer fx/echoOn
//    - augmenter fx/echoMix progressivement
//    - garder fx/echoFb < 0.85 pour éviter l’instabilité
//
// 5) Tester la polyphonie :
//    - jouer des accords (2 à 8 notes)
//    - vérifier qu’aucune saturation n’apparaît
//
// =======================================================
