#!/bin/bash
set -e

# Se placer à la racine du repo, peu importe d'où on lance le script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_ROOT"

mkdir -p faust/build

faust -i \
  -a faust/architectures/faustMinimal.h \
  faust/TaikoSynth.dsp \
  -o faust/build/TaikoSynth.h

echo "✅ Généré: faust/build/TaikoSynth.h"
echo "👉 Pense à copier ce .h dans teensy/taiko_synth/ si ton projet l’attend là."
