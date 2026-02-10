#!/usr/bin/env python3
import os
import sys
import glob
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent

# --- chemins projet (adapte si besoin) ---
SKETCH_DIR = ROOT / "teensy" / "taiko_synth"
SKETCH_INO = SKETCH_DIR / "taiko_synth.ino"

TOOL_DIR = ROOT / "tools" / "keyboard_to_midi_cpp"
TOOL_BUILD = TOOL_DIR / "build"
TOOL_BIN = TOOL_BUILD / "keyboard_to_midi"

# Teensy 4.0
FQBN = "teensy:avr:teensy40"

# USB Type côté Teensy (le bon choix pour avoir un port /dev/cu.usbmodem*)
USBTYPE = "USB_MIDI_SERIAL"


def run(cmd, cwd=None):
    print(f"\n$ {' '.join(cmd)}")
    subprocess.run(cmd, cwd=cwd, check=True)


def ensure_exists(p: Path, label: str):
    if not p.exists():
        print(f"❌ {label} introuvable: {p}")
        sys.exit(1)


def detect_port():
    """
    Détecte le port série Teensy sur macOS.
    Teensy apparaît généralement en /dev/cu.usbmodem*
    """
    modem = sorted(glob.glob("/dev/cu.usbmodem*"))
    serial = sorted(glob.glob("/dev/cu.usbserial*"))

    candidates = modem + serial
    if not candidates:
        return None

    # Heuristique simple : prendre le dernier (souvent le plus récent)
    return candidates[-1]


def main():
    ensure_exists(SKETCH_INO, "Sketch Arduino (.ino)")
    ensure_exists(TOOL_DIR, "Dossier tool clavier")

    # 0) détecter port
    port = detect_port()
    if not port:
        print("❌ Aucun port USB détecté.")
        print("👉 Vérifie :")
        print("   - Teensy branchée avec un câble DATA (pas charge-only)")
        print("   - Arduino IDE : Tools -> USB Type = MIDI + Serial")
        print("   - puis rebranche la Teensy")
        print("\n(Ports attendus: /dev/cu.usbmodem* ou /dev/cu.usbserial*)")
        sys.exit(1)

    print(f"✅ Port détecté: {port}")

    # 1) Upload Teensy via arduino-cli
    print("\n=== 1) Upload Teensy (arduino-cli) ===")
    print("⚠️ Si l’upload attend le mode program, appuie sur le bouton de la Teensy.")

    run([
        "arduino-cli", "compile",
        "--fqbn", FQBN,
        "--build-property", f"build.usbtype={USBTYPE}",
        "--upload",
        "-p", port,
        str(SKETCH_DIR),
    ])

    # 2) Build tool clavier (CMake)
    print("\n=== 2) Build tool clavier (CMake) ===")
    TOOL_BUILD.mkdir(exist_ok=True)

    # Configure une seule fois
    if not (TOOL_BUILD / "CMakeCache.txt").exists():
        run(["cmake", "-S", str(TOOL_DIR), "-B", str(TOOL_BUILD)])

    # Build
    run(["cmake", "--build", str(TOOL_BUILD), "-j"])

    # 3) Lancer le clavier
    print("\n=== 3) Lancer le clavier virtuel ===")
    print("🎹 Une fenêtre SDL s’ouvre. ESC pour quitter.")
    run([str(TOOL_BIN)])

    print("\n✅ Terminé.")


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as e:
        print("\n❌ Une commande a échoué.")
        sys.exit(e.returncode)
    except KeyboardInterrupt:
        print("\n👋 Interrompu (Ctrl+C).")
        sys.exit(0)
