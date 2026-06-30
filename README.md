
## Clavier et polyphonie

- Configuration initiale : **8 touches de piano virtuelles**
- **8 voix polyphoniques**
- Gestion correcte des messages `NoteOn` / `NoteOff`
- Architecture extensible vers :
  - un plus grand nombre de touches (25 / 49 / 88),
  - un plus grand nombre de voix.

Le moteur audio Faust est **indépendant du nombre de touches affichées**.

---

## Interaction matérielle (Teensy)

### Boutons initiaux
- Bouton 1 : preset suivant
- Bouton 2 : preset précédent

### Extensions possibles
- Activation / désactivation de l’echo
- Sustain
- Panic (arrêt de toutes les notes)
- Changement de mode

Les boutons sont gérés en **C++**, puis traduits en messages MIDI interprétés par Faust.

---

## Interface graphique PC

- Piano virtuel interactif
- Animation visuelle lors de l’appui sur une touche
- Communication exclusivement via MIDI (aucun son généré sur le PC)
- Découplage total entre l’interface et le moteur audio

L’interface peut ainsi évoluer indépendamment du DSP.

---

## 🗺️ Feuille de route (Roadmap)

### Phase 1 — Validation technique **validé le 5/2 sur VMPK**
- [x] Communication MIDI PC → Teensy  
- [x] Sortie audio via Audio Shield
- [x] Test de la polyphonie (4 puis 8 voix) en C++
- [x] Tests avec clavier MIDI virtuel

---

### Phase 2 — Moteur audio Faust    **fait le 5/2 ** (non testé)
- [ ] Implémentation du synthétiseur polyphonique (8 voix)
- [ ] Gestion des notes MIDI
- [ ] Enveloppes ADSR
- [ ] Effet d’echo stable (sans saturation)

---

### Phase 3 — Presets et contrôle   **validé le 5/2 (non testé)
- [ ] Implémentation des 4 presets piano
- [ ] Mapping Program Change → presets
- [ ] Mapping Control Change → gain / echo / timbre
- [ ] Intégration des boutons physiques

---

### Phase 4 — Interface graphique
- [ ] Piano virtuel à 8 touches
- [ ] Animations visuelles lors de l’appui
- [ ] Sélection de presets depuis l’interface
- [ ] Tests de latence et de stabilité

---

### Phase 5 — Finalisation
- [ ] Nettoyage et structuration du code
- [ ] README final et schémas
- [ ] Scénario de démonstration (2–3 minutes)
- [ ] (Optionnel) Boîtier imprimé en 3D

---

## Message clé pour la soutenance

> Ce projet met en œuvre un synthétiseur polyphonique temps réel exécuté sur une plateforme Teensy, dont le moteur audio est développé en Faust et contrôlé via MIDI, illustrant concrètement les notions de DSP, de contrôle et d’audio embarqué.

---

## Technologies utilisées
- **Teensy**
- **Faust**
- **C++**
- **MIDI**
- **Audio Shield SGTL5000**


PolyPiano.dsp
   ↓ (faust compiler)

Code C++ Teensy (audio + MIDI)
  
   ↓ (on compile via Arduino IDE)
Firmware unique sur Teensy
