$ErrorActionPreference = "Stop"

# Se placer à la racine du repo
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Resolve-Path (Join-Path $ScriptDir "..")
Set-Location $RepoRoot

New-Item -ItemType Directory -Force -Path "faust/build" | Out-Null

faust -i `
  -a faust/architectures/faustMinimal.h `
  faust/TaikoSynth.dsp `
  -o faust/build/TaikoSynth.h

Write-Host "✅ Généré: faust/build/TaikoSynth.h"
Write-Host "👉 Pense à copier ce .h dans teensy/taiko_synth/ si nécessaire."
