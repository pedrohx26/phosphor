#!/usr/bin/env bash
# ╔══════════════════════════════════════════════════════════════════════════════╗
# ║  Phosphor CLI — command-line helper for the Phosphor KWin CRT effect       ║
# ║  Presets, parameters, status — without recompiling                         ║
# ╚══════════════════════════════════════════════════════════════════════════════╝
#
# Usage:  ./phosphor-cli.sh <command> [args]
#
# Commands:
#   presets              List all available presets
#   preset <KEY>         Apply a preset (writes to kwinrc + reloads KWin)
#   set <param> <value>  Set a single parameter
#   get [param]          Read parameter(s) from kwinrc
#   status               Show current effect status and active settings
#   on                   Enable the effect (set targetMode=1)
#   off                  Disable the effect (set targetMode=0)
#   reload               Reload KWin to apply changes
#   params               Show parameter reference
#
# License: GPL-2.0-or-later
set -euo pipefail

# ── Colors ─────────────────────────────────────────────────────────────────────
R='\033[0;31m' G='\033[0;32m' Y='\033[1;33m' C='\033[0;36m'
BOLD='\033[1m' DIM='\033[2m' NC='\033[0m'

# ── Config ─────────────────────────────────────────────────────────────────────
CFG_GROUP="Effect-retro-terminal"
KNOWN_TERMINALS="konsole,cool-retro-term,yakuake,kitty,alacritty,wezterm,xterm,gnome-terminal,tilix"

# ── Helpers ────────────────────────────────────────────────────────────────────
msg()  { echo -e "$*"; }
info() { echo -e "${C}[info]${NC} $*"; }
ok()   { echo -e "${G}[ ok ]${NC} $*"; }
warn() { echo -e "${Y}[warn]${NC} $*"; }
err()  { echo -e "${R}[err ]${NC} $*" >&2; }
die()  { err "$*"; exit 1; }

kread()  { kreadconfig6  --file kwinrc --group "$CFG_GROUP" --key "$1" 2>/dev/null || echo ""; }
kwrite() { kwriteconfig6 --file kwinrc --group "$CFG_GROUP" --key "$1" "$2"; }

kwin_reload() {
    qdbus6 org.kde.KWin /KWin reconfigure 2>/dev/null ||
    qdbus  org.kde.KWin /KWin reconfigure 2>/dev/null ||
    warn "KWin D-Bus unreachable — restart KWin manually (kwin_wayland --replace &)"
}

# ══════════════════════════════════════════════════════════════════════════════
# PRESET DATABASE
# ══════════════════════════════════════════════════════════════════════════════
# Format: KEY|Display Name|params string|resX|resY|font|fontSize
declare -A PRESET_DATA

p() {
    PRESET_DATA["$1"]="$2|$3|$4|$5|$6|$7"
}

# ── 1960s ──────────────────────────────────────────────────────────────────────
p IBM2260    "IBM 2260 (1964)" \
  "phosphorType=2 phosphorAgeing=0.55 colorTemperature=8500 phosphorPersistence=0.60 screenCurvature=0.45 vignetteIntensity=0.65 ambientReflection=0.12 rasterizationMode=0 bloom=0.80 glowingLine=0.45 brightness=0.42 contrast=0.90 staticNoise=0.18 jitter=0.25 syncMode=1 horizontalSync=0.20 flickering=0.22 ghostingIntensity=0.00 chromaColor=0.0 saturationColor=0.0 rbgShift=0.15 characterSmearing=0.30 burnIn=0.50 warmupDuration=15 degaussDuration=4.0 targetResX=640 targetResY=250 pixelScale=0.7 sampleMode=2" \
  640 250 "Glass TTY VT220" 16

# ── 1970s ──────────────────────────────────────────────────────────────────────
p DEC_GT40   "DEC GT40 (1972)" \
  "phosphorType=3 phosphorAgeing=0.40 colorTemperature=7800 phosphorPersistence=0.80 screenCurvature=0.20 vignetteIntensity=0.55 ambientReflection=0.08 rasterizationMode=0 bloom=0.85 glowingLine=0.60 brightness=0.38 contrast=0.85 staticNoise=0.10 jitter=0.15 syncMode=0 horizontalSync=0.08 flickering=0.15 chromaColor=0.05 saturationColor=0.10 rbgShift=0.06 characterSmearing=0.15 burnIn=0.35 warmupDuration=12 degaussDuration=3.5 targetResX=1024 targetResY=768 pixelScale=0.7 sampleMode=2" \
  1024 768 "VT323" 18

p DEC_VT100  "DEC VT100 (1978)" \
  "phosphorType=0 phosphorAgeing=0.12 colorTemperature=8000 phosphorPersistence=0.18 screenCurvature=0.22 vignetteIntensity=0.38 ambientReflection=0.05 rasterizationMode=1 scanlinesIntensity=0.40 scanlinesSharpness=0.55 bloom=0.52 glowingLine=0.22 brightness=0.52 contrast=0.82 staticNoise=0.06 jitter=0.08 syncMode=0 horizontalSync=0.05 flickering=0.07 chromaColor=0.05 saturationColor=0.08 rbgShift=0.05 characterSmearing=0.10 burnIn=0.22 warmupDuration=9 degaussDuration=2.5 targetResX=800 targetResY=240 pixelScale=0.7 sampleMode=2" \
  800 240 "VT323" 18

p IBM3270    "IBM 3270 (1971)" \
  "phosphorType=0 phosphorAgeing=0.18 colorTemperature=8200 phosphorPersistence=0.25 screenCurvature=0.28 vignetteIntensity=0.42 ambientReflection=0.06 rasterizationMode=1 scanlinesIntensity=0.35 scanlinesSharpness=0.60 bloom=0.48 glowingLine=0.18 brightness=0.50 contrast=0.88 staticNoise=0.05 jitter=0.06 syncMode=0 horizontalSync=0.04 flickering=0.05 chromaColor=0.04 saturationColor=0.06 rbgShift=0.04 characterSmearing=0.08 burnIn=0.35 warmupDuration=10 degaussDuration=2.8 targetResX=720 targetResY=350 pixelScale=0.7 sampleMode=2" \
  720 350 "PxPlus IBM 3270 Semi-Graphics" 16

p WYSE50     "Wyse WY-50 (1979)" \
  "phosphorType=0 phosphorAgeing=0.08 colorTemperature=8400 phosphorPersistence=0.14 screenCurvature=0.18 vignetteIntensity=0.32 ambientReflection=0.04 rasterizationMode=1 scanlinesIntensity=0.38 scanlinesSharpness=0.65 bloom=0.55 glowingLine=0.20 brightness=0.55 contrast=0.85 staticNoise=0.04 jitter=0.06 syncMode=0 horizontalSync=0.03 flickering=0.05 chromaColor=0.04 saturationColor=0.08 rbgShift=0.04 characterSmearing=0.08 burnIn=0.20 warmupDuration=8 degaussDuration=2.5 targetResX=720 targetResY=360 pixelScale=0.7 sampleMode=2" \
  720 360 "PxPlus Wyse WY700b 2x" 16

p RADAR      "Military Radar (1965)" \
  "phosphorType=3 phosphorAgeing=0.50 colorTemperature=7000 phosphorPersistence=0.90 screenCurvature=0.15 vignetteIntensity=0.70 ambientReflection=0.10 rasterizationMode=0 bloom=0.90 glowingLine=0.70 brightness=0.35 contrast=0.95 staticNoise=0.15 jitter=0.20 syncMode=0 horizontalSync=0.10 flickering=0.18 chromaColor=0.04 saturationColor=0.08 rbgShift=0.06 characterSmearing=0.20 burnIn=0.55 warmupDuration=20 degaussDuration=5.0 targetResX=1024 targetResY=1024 pixelScale=0.7 sampleMode=2" \
  1024 1024 "Share Tech Mono" 16

p TELETEXT   "Teletext / Ceefax (1974)" \
  "phosphorType=2 phosphorAgeing=0.30 colorTemperature=6200 phosphorPersistence=0.20 screenCurvature=0.40 vignetteIntensity=0.52 ambientReflection=0.09 rasterizationMode=1 scanlinesIntensity=0.62 scanlinesSharpness=0.28 bloom=0.70 glowingLine=0.30 brightness=0.46 contrast=0.76 staticNoise=0.20 jitter=0.24 syncMode=2 horizontalSync=0.18 flickering=0.20 ghostingIntensity=0.12 chromaColor=0.80 saturationColor=0.55 rbgShift=0.22 characterSmearing=0.40 burnIn=0.28 warmupDuration=12 degaussDuration=3.5 targetResX=480 targetResY=250 pixelScale=0.7 sampleMode=2" \
  480 250 "Bedstead" 16

# ── Home computers 1977–1983 ───────────────────────────────────────────────────
p APPLE_II   "Apple II (1977)" \
  "phosphorType=2 phosphorAgeing=0.30 colorTemperature=6500 phosphorPersistence=0.22 screenCurvature=0.38 vignetteIntensity=0.50 ambientReflection=0.08 rasterizationMode=1 scanlinesIntensity=0.55 scanlinesSharpness=0.35 bloom=0.65 glowingLine=0.28 brightness=0.48 contrast=0.78 staticNoise=0.14 jitter=0.18 syncMode=1 horizontalSync=0.12 flickering=0.14 chromaColor=0.45 saturationColor=0.35 rbgShift=0.18 characterSmearing=0.35 burnIn=0.28 warmupDuration=10 degaussDuration=3.0 targetResX=280 targetResY=192 pixelScale=0.7 sampleMode=2" \
  280 192 "Print Char 21" 16

p COMMODORE_PET "Commodore PET 2001 (1977)" \
  "phosphorType=2 phosphorAgeing=0.35 colorTemperature=8500 phosphorPersistence=0.12 screenCurvature=0.40 vignetteIntensity=0.55 ambientReflection=0.10 rasterizationMode=1 scanlinesIntensity=0.52 scanlinesSharpness=0.58 bloom=0.65 glowingLine=0.18 brightness=0.55 contrast=0.90 staticNoise=0.06 jitter=0.08 syncMode=0 horizontalSync=0.04 flickering=0.06 chromaColor=0.00 saturationColor=0.00 rbgShift=0.05 characterSmearing=0.12 burnIn=0.38 warmupDuration=11 degaussDuration=3.0 targetResX=320 targetResY=200 pixelScale=0.7 sampleMode=2" \
  320 200 "Pet Me 2Y" 16

p TRS80_MODEL1 "TRS-80 Model I (1977)" \
  "phosphorType=2 phosphorAgeing=0.28 colorTemperature=6800 phosphorPersistence=0.14 screenCurvature=0.35 vignetteIntensity=0.48 ambientReflection=0.08 rasterizationMode=1 scanlinesIntensity=0.52 scanlinesSharpness=0.35 bloom=0.58 glowingLine=0.18 brightness=0.50 contrast=0.80 staticNoise=0.14 jitter=0.15 syncMode=1 horizontalSync=0.10 flickering=0.12 chromaColor=0.10 saturationColor=0.12 rbgShift=0.14 characterSmearing=0.28 burnIn=0.22 warmupDuration=9 degaussDuration=2.8 targetResX=384 targetResY=192 pixelScale=0.7 sampleMode=2" \
  384 192 "Another Man's Treasure MIA" 16

p ATARI800   "Atari 400/800 (1979)" \
  "phosphorType=2 phosphorAgeing=0.26 colorTemperature=6400 phosphorPersistence=0.19 screenCurvature=0.36 vignetteIntensity=0.47 ambientReflection=0.07 rasterizationMode=1 scanlinesIntensity=0.52 scanlinesSharpness=0.35 bloom=0.62 glowingLine=0.26 brightness=0.50 contrast=0.79 staticNoise=0.12 jitter=0.15 syncMode=1 horizontalSync=0.10 flickering=0.13 chromaColor=0.50 saturationColor=0.38 rbgShift=0.15 characterSmearing=0.30 burnIn=0.24 warmupDuration=9 degaussDuration=2.8 targetResX=320 targetResY=192 pixelScale=0.7 sampleMode=2" \
  320 192 "Atari Classic" 16

p TRS80_COCO "TRS-80 Color Computer (1980)" \
  "phosphorType=2 phosphorAgeing=0.25 colorTemperature=6500 phosphorPersistence=0.16 screenCurvature=0.35 vignetteIntensity=0.46 ambientReflection=0.08 rasterizationMode=1 scanlinesIntensity=0.50 scanlinesSharpness=0.32 bloom=0.60 glowingLine=0.22 brightness=0.48 contrast=0.78 staticNoise=0.13 jitter=0.16 syncMode=1 horizontalSync=0.11 flickering=0.13 chromaColor=0.60 saturationColor=0.45 rbgShift=0.15 characterSmearing=0.30 burnIn=0.22 warmupDuration=9 degaussDuration=2.8 targetResX=256 targetResY=192 pixelScale=0.7 sampleMode=2" \
  256 192 "Hot CoCo" 16

p BBC_MICRO  "BBC Micro (1981)" \
  "phosphorType=2 phosphorAgeing=0.20 colorTemperature=6600 phosphorPersistence=0.16 screenCurvature=0.30 vignetteIntensity=0.44 ambientReflection=0.07 rasterizationMode=1 scanlinesIntensity=0.50 scanlinesSharpness=0.38 bloom=0.60 glowingLine=0.22 brightness=0.50 contrast=0.80 staticNoise=0.10 jitter=0.12 syncMode=1 horizontalSync=0.09 flickering=0.11 chromaColor=0.55 saturationColor=0.40 rbgShift=0.14 characterSmearing=0.25 burnIn=0.22 warmupDuration=9 degaussDuration=2.8 targetResX=320 targetResY=256 pixelScale=0.7 sampleMode=2" \
  320 256 "Bedstead" 16

p VIC20      "Commodore VIC-20 (1981)" \
  "phosphorType=2 phosphorAgeing=0.28 colorTemperature=6000 phosphorPersistence=0.20 screenCurvature=0.40 vignetteIntensity=0.52 ambientReflection=0.09 rasterizationMode=1 scanlinesIntensity=0.58 scanlinesSharpness=0.30 bloom=0.68 glowingLine=0.30 brightness=0.46 contrast=0.75 staticNoise=0.16 jitter=0.20 syncMode=1 horizontalSync=0.15 flickering=0.16 chromaColor=0.50 saturationColor=0.42 rbgShift=0.20 characterSmearing=0.38 burnIn=0.22 warmupDuration=10 degaussDuration=3.0" \
  0 0 "C64 Pro Mono" 16

p C64        "Commodore 64 (1982)" \
  "phosphorType=2 phosphorAgeing=0.22 colorTemperature=6200 phosphorPersistence=0.18 screenCurvature=0.35 vignetteIntensity=0.45 ambientReflection=0.07 rasterizationMode=1 scanlinesIntensity=0.50 scanlinesSharpness=0.38 bloom=0.60 glowingLine=0.25 brightness=0.50 contrast=0.80 staticNoise=0.12 jitter=0.14 syncMode=1 horizontalSync=0.10 flickering=0.12 chromaColor=0.55 saturationColor=0.40 rbgShift=0.14 characterSmearing=0.28 burnIn=0.25 warmupDuration=9 degaussDuration=2.8 targetResX=320 targetResY=200 pixelScale=0.7 sampleMode=2" \
  320 200 "C64 Pro Mono" 14

p ZX_SPECTRUM "ZX Spectrum (1982)" \
  "phosphorType=2 phosphorAgeing=0.25 colorTemperature=6300 phosphorPersistence=0.16 screenCurvature=0.38 vignetteIntensity=0.48 ambientReflection=0.08 rasterizationMode=1 scanlinesIntensity=0.52 scanlinesSharpness=0.33 bloom=0.62 glowingLine=0.24 brightness=0.52 contrast=0.78 staticNoise=0.13 jitter=0.16 syncMode=1 horizontalSync=0.11 flickering=0.13 chromaColor=0.60 saturationColor=0.45 rbgShift=0.16 characterSmearing=0.30 burnIn=0.20 warmupDuration=9 degaussDuration=2.8 targetResX=256 targetResY=192 pixelScale=0.7 sampleMode=2" \
  256 192 "VT323" 14

p KAYPRO_II  "Kaypro II (1982)" \
  "phosphorType=0 phosphorAgeing=0.14 colorTemperature=8100 phosphorPersistence=0.16 screenCurvature=0.42 vignetteIntensity=0.52 ambientReflection=0.07 rasterizationMode=1 scanlinesIntensity=0.44 scanlinesSharpness=0.60 bloom=0.58 glowingLine=0.22 brightness=0.50 contrast=0.86 staticNoise=0.05 jitter=0.07 syncMode=0 horizontalSync=0.04 flickering=0.06 chromaColor=0.00 saturationColor=0.00 rbgShift=0.04 characterSmearing=0.08 burnIn=0.25 warmupDuration=9 degaussDuration=2.8 targetResX=640 targetResY=192 pixelScale=0.7 sampleMode=2" \
  640 192 "PxPlus Kaypro 2000" 16

p SHARP_MZ700 "Sharp MZ-700 (1982)" \
  "phosphorType=2 phosphorAgeing=0.12 colorTemperature=8400 phosphorPersistence=0.10 screenCurvature=0.26 vignetteIntensity=0.40 ambientReflection=0.06 rasterizationMode=1 scanlinesIntensity=0.40 scanlinesSharpness=0.58 bloom=0.50 glowingLine=0.16 brightness=0.55 contrast=0.87 staticNoise=0.05 jitter=0.06 syncMode=0 horizontalSync=0.03 flickering=0.05 chromaColor=0.00 saturationColor=0.00 rbgShift=0.04 characterSmearing=0.08 burnIn=0.22 warmupDuration=8 degaussDuration=2.5 targetResX=320 targetResY=200 pixelScale=0.7 sampleMode=2" \
  320 200 "Mizuno" 14

p MSX        "MSX (1983)" \
  "phosphorType=2 phosphorAgeing=0.24 colorTemperature=6500 phosphorPersistence=0.18 screenCurvature=0.35 vignetteIntensity=0.46 ambientReflection=0.07 rasterizationMode=1 scanlinesIntensity=0.52 scanlinesSharpness=0.36 bloom=0.62 glowingLine=0.24 brightness=0.50 contrast=0.79 staticNoise=0.12 jitter=0.14 syncMode=1 horizontalSync=0.10 flickering=0.12 chromaColor=0.58 saturationColor=0.42 rbgShift=0.15 characterSmearing=0.28 burnIn=0.22 warmupDuration=9 degaussDuration=2.8" \
  0 0 "VT323" 14

p MATTEL_AQUARIUS "Mattel Aquarius (1983)" \
  "phosphorType=2 phosphorAgeing=0.22 colorTemperature=6400 phosphorPersistence=0.18 screenCurvature=0.36 vignetteIntensity=0.48 ambientReflection=0.08 rasterizationMode=1 scanlinesIntensity=0.54 scanlinesSharpness=0.30 bloom=0.62 glowingLine=0.22 brightness=0.48 contrast=0.78 staticNoise=0.18 jitter=0.18 syncMode=1 horizontalSync=0.14 flickering=0.14 chromaColor=0.55 saturationColor=0.38 rbgShift=0.16 characterSmearing=0.35 burnIn=0.24 warmupDuration=10 degaussDuration=3.0 targetResX=320 targetResY=200 pixelScale=0.7 sampleMode=2" \
  320 200 "Antiquarius" 16

# ── IBM PC era 1981–1990 ──────────────────────────────────────────────────────
p IBM_MDA    "IBM PC MDA (1981)" \
  "phosphorType=3 phosphorAgeing=0.15 colorTemperature=7500 phosphorPersistence=0.30 screenCurvature=0.20 vignetteIntensity=0.40 ambientReflection=0.06 rasterizationMode=1 scanlinesIntensity=0.42 scanlinesSharpness=0.60 bloom=0.58 glowingLine=0.28 brightness=0.50 contrast=0.88 staticNoise=0.05 jitter=0.07 syncMode=0 horizontalSync=0.04 flickering=0.06 chromaColor=0.04 saturationColor=0.06 rbgShift=0.05 characterSmearing=0.10 burnIn=0.30 warmupDuration=8 degaussDuration=2.5 targetResX=720 targetResY=350 pixelScale=0.7 sampleMode=2" \
  720 350 "PxPlus IBM MDA" 16

p IBM_CGA    "IBM PC CGA (1981)" \
  "phosphorType=2 phosphorAgeing=0.20 colorTemperature=7000 phosphorPersistence=0.15 screenCurvature=0.25 vignetteIntensity=0.38 ambientReflection=0.06 rasterizationMode=1 scanlinesIntensity=0.45 scanlinesSharpness=0.50 bloom=0.55 glowingLine=0.22 brightness=0.52 contrast=0.83 staticNoise=0.08 jitter=0.10 syncMode=0 horizontalSync=0.06 flickering=0.08 chromaColor=0.65 saturationColor=0.45 rbgShift=0.10 characterSmearing=0.15 burnIn=0.22 warmupDuration=8 degaussDuration=2.5 targetResX=320 targetResY=200 pixelScale=0.7 sampleMode=2" \
  320 200 "PxPlus IBM CGA" 16

p IBM_EGA    "IBM PC EGA (1984)" \
  "phosphorType=2 phosphorAgeing=0.14 colorTemperature=7200 phosphorPersistence=0.12 screenCurvature=0.20 vignetteIntensity=0.32 ambientReflection=0.05 rasterizationMode=1 scanlinesIntensity=0.38 scanlinesSharpness=0.58 bloom=0.50 glowingLine=0.18 brightness=0.54 contrast=0.84 staticNoise=0.06 jitter=0.08 syncMode=0 horizontalSync=0.04 flickering=0.06 chromaColor=0.60 saturationColor=0.38 rbgShift=0.08 characterSmearing=0.12 burnIn=0.18 warmupDuration=8 degaussDuration=2.5 targetResX=640 targetResY=350 pixelScale=0.7 sampleMode=2" \
  640 350 "PxPlus IBM EGA 8x14" 14

p TANDY1000  "Tandy 1000 (1984)" \
  "phosphorType=2 phosphorAgeing=0.22 colorTemperature=6800 phosphorPersistence=0.15 screenCurvature=0.28 vignetteIntensity=0.42 ambientReflection=0.07 rasterizationMode=1 scanlinesIntensity=0.48 scanlinesSharpness=0.42 bloom=0.58 glowingLine=0.22 brightness=0.50 contrast=0.80 staticNoise=0.10 jitter=0.12 syncMode=1 horizontalSync=0.08 flickering=0.10 chromaColor=0.70 saturationColor=0.48 rbgShift=0.12 characterSmearing=0.20 burnIn=0.24 warmupDuration=9 degaussDuration=2.8 targetResX=320 targetResY=200 pixelScale=0.7 sampleMode=2" \
  320 200 "PxPlus Tandy 1000" 16

p IBM_VGA    "IBM PS/2 VGA (1987)" \
  "phosphorType=2 phosphorAgeing=0.10 colorTemperature=7400 phosphorPersistence=0.10 screenCurvature=0.15 vignetteIntensity=0.28 ambientReflection=0.04 rasterizationMode=1 scanlinesIntensity=0.32 scanlinesSharpness=0.62 bloom=0.45 glowingLine=0.15 brightness=0.55 contrast=0.85 staticNoise=0.05 jitter=0.06 syncMode=0 horizontalSync=0.03 flickering=0.05 chromaColor=0.65 saturationColor=0.35 rbgShift=0.07 characterSmearing=0.10 burnIn=0.15 warmupDuration=7 degaussDuration=2.2 targetResX=720 targetResY=400 pixelScale=0.7 sampleMode=2" \
  720 400 "PxPlus IBM VGA 9x16" 16

p COMPAQ_PORTABLE "Compaq Portable (1982)" \
  "phosphorType=1 phosphorAgeing=0.18 colorTemperature=5800 phosphorPersistence=0.20 screenCurvature=0.40 vignetteIntensity=0.50 ambientReflection=0.08 rasterizationMode=1 scanlinesIntensity=0.42 scanlinesSharpness=0.55 bloom=0.60 glowingLine=0.25 brightness=0.52 contrast=0.88 staticNoise=0.06 jitter=0.08 syncMode=0 horizontalSync=0.04 flickering=0.06 chromaColor=0.00 saturationColor=0.00 rbgShift=0.05 characterSmearing=0.10 burnIn=0.28 warmupDuration=9 degaussDuration=2.8 targetResX=640 targetResY=200 pixelScale=0.7 sampleMode=2" \
  640 200 "PxPlus CompaqPort" 16

p AMSTRAD_PC1512 "Amstrad PC1512 (1986)" \
  "phosphorType=2 phosphorAgeing=0.16 colorTemperature=6800 phosphorPersistence=0.12 screenCurvature=0.22 vignetteIntensity=0.36 ambientReflection=0.06 rasterizationMode=1 scanlinesIntensity=0.42 scanlinesSharpness=0.48 bloom=0.52 glowingLine=0.20 brightness=0.52 contrast=0.82 staticNoise=0.07 jitter=0.09 syncMode=0 horizontalSync=0.06 flickering=0.08 chromaColor=0.70 saturationColor=0.42 rbgShift=0.08 characterSmearing=0.14 burnIn=0.18 warmupDuration=8 degaussDuration=2.5 targetResX=640 targetResY=200 pixelScale=0.7 sampleMode=2" \
  640 200 "PxPlus Amstrad PC-2y" 16

# ── Professional workstations / terminals 1982–1990 ──────────────────────────
p DEC_RAINBOW "DEC Rainbow 100 (1982)" \
  "phosphorType=0 phosphorAgeing=0.10 colorTemperature=8200 phosphorPersistence=0.14 screenCurvature=0.18 vignetteIntensity=0.35 ambientReflection=0.05 rasterizationMode=1 scanlinesIntensity=0.38 scanlinesSharpness=0.62 bloom=0.52 glowingLine=0.18 brightness=0.55 contrast=0.86 staticNoise=0.04 jitter=0.05 syncMode=0 horizontalSync=0.03 flickering=0.04 chromaColor=0.00 saturationColor=0.00 rbgShift=0.04 characterSmearing=0.08 burnIn=0.18 warmupDuration=8 degaussDuration=2.5 targetResX=800 targetResY=240 pixelScale=0.7 sampleMode=2" \
  800 240 "PxPlus DEC Rainbow100-8x10" 16

p TELEVIDEO_925 "TeleVideo TVI-925 (1982)" \
  "phosphorType=0 phosphorAgeing=0.07 colorTemperature=8300 phosphorPersistence=0.12 screenCurvature=0.20 vignetteIntensity=0.34 ambientReflection=0.04 rasterizationMode=1 scanlinesIntensity=0.36 scanlinesSharpness=0.66 bloom=0.50 glowingLine=0.16 brightness=0.56 contrast=0.87 staticNoise=0.04 jitter=0.05 syncMode=0 horizontalSync=0.03 flickering=0.04 chromaColor=0.00 saturationColor=0.00 rbgShift=0.04 characterSmearing=0.07 burnIn=0.18 warmupDuration=8 degaussDuration=2.2 targetResX=720 targetResY=360 pixelScale=0.7 sampleMode=2" \
  720 360 "PxPlus TeleVideo TVI-925" 16

p NEC_APC3   "NEC APC III (1983)" \
  "phosphorType=0 phosphorAgeing=0.08 colorTemperature=8400 phosphorPersistence=0.10 screenCurvature=0.16 vignetteIntensity=0.30 ambientReflection=0.04 rasterizationMode=1 scanlinesIntensity=0.32 scanlinesSharpness=0.68 bloom=0.46 glowingLine=0.14 brightness=0.58 contrast=0.88 staticNoise=0.03 jitter=0.04 syncMode=0 horizontalSync=0.02 flickering=0.04 chromaColor=0.00 saturationColor=0.00 rbgShift=0.04 characterSmearing=0.06 burnIn=0.14 warmupDuration=7 degaussDuration=2.0 targetResX=640 targetResY=400 pixelScale=0.7 sampleMode=2" \
  640 400 "PxPlus NEC APC3 8x16" 16

p HP_150     "HP 150 Touchscreen (1983)" \
  "phosphorType=2 phosphorAgeing=0.08 colorTemperature=8700 phosphorPersistence=0.07 screenCurvature=0.22 vignetteIntensity=0.40 ambientReflection=0.06 rasterizationMode=1 scanlinesIntensity=0.40 scanlinesSharpness=0.62 bloom=0.48 glowingLine=0.14 brightness=0.60 contrast=0.89 staticNoise=0.03 jitter=0.04 syncMode=0 horizontalSync=0.02 flickering=0.03 chromaColor=0.00 saturationColor=0.00 rbgShift=0.04 characterSmearing=0.06 burnIn=0.24 warmupDuration=7 degaussDuration=2.0 targetResX=640 targetResY=256 pixelScale=0.7 sampleMode=2" \
  640 256 "PxPlus HP 150" 16

p APPLE_LISA "Apple Lisa (1983)" \
  "phosphorType=2 phosphorAgeing=0.10 colorTemperature=8800 phosphorPersistence=0.06 screenCurvature=0.14 vignetteIntensity=0.38 ambientReflection=0.08 rasterizationMode=0 scanlinesIntensity=0.15 scanlinesSharpness=0.70 bloom=0.38 glowingLine=0.08 brightness=0.62 contrast=0.91 staticNoise=0.02 jitter=0.03 syncMode=0 horizontalSync=0.02 flickering=0.03 chromaColor=0.00 saturationColor=0.00 rbgShift=0.03 characterSmearing=0.04 burnIn=0.30 warmupDuration=6 degaussDuration=2.0 targetResX=720 targetResY=364 pixelScale=0.7 sampleMode=2" \
  720 364 "LisaTerminal Paper" 13

p ATARI_ST_MONO "Atari ST SM124 (1985)" \
  "phosphorType=2 phosphorAgeing=0.08 colorTemperature=8600 phosphorPersistence=0.06 screenCurvature=0.08 vignetteIntensity=0.28 ambientReflection=0.05 rasterizationMode=1 scanlinesIntensity=0.18 scanlinesSharpness=0.75 bloom=0.35 glowingLine=0.08 brightness=0.60 contrast=0.90 staticNoise=0.02 jitter=0.03 syncMode=0 horizontalSync=0.02 flickering=0.03 chromaColor=0.00 saturationColor=0.00 rbgShift=0.03 characterSmearing=0.04 burnIn=0.15 warmupDuration=6 degaussDuration=1.8 targetResX=640 targetResY=400 pixelScale=0.7 sampleMode=2" \
  640 400 "Project Jason" 14

p APPLE_IIGS "Apple IIgs (1986)" \
  "phosphorType=2 phosphorAgeing=0.08 colorTemperature=7400 phosphorPersistence=0.08 screenCurvature=0.18 vignetteIntensity=0.32 ambientReflection=0.05 rasterizationMode=1 scanlinesIntensity=0.36 scanlinesSharpness=0.55 bloom=0.48 glowingLine=0.14 brightness=0.56 contrast=0.85 staticNoise=0.04 jitter=0.05 syncMode=0 horizontalSync=0.03 flickering=0.05 chromaColor=0.75 saturationColor=0.45 rbgShift=0.07 characterSmearing=0.10 burnIn=0.14 warmupDuration=7 degaussDuration=2.0 targetResX=320 targetResY=200 pixelScale=0.7 sampleMode=2" \
  320 200 "Shaston 320" 14

# ── Amiga / Mac / NeXT / UNIX ────────────────────────────────────────────────
p AMIGA500   "Amiga 500 (1987)" \
  "phosphorType=2 phosphorAgeing=0.15 colorTemperature=6800 phosphorPersistence=0.14 screenCurvature=0.25 vignetteIntensity=0.38 ambientReflection=0.06 rasterizationMode=1 scanlinesIntensity=0.48 scanlinesSharpness=0.44 bloom=0.55 glowingLine=0.20 brightness=0.52 contrast=0.81 staticNoise=0.08 jitter=0.10 syncMode=0 horizontalSync=0.05 flickering=0.08 chromaColor=0.65 saturationColor=0.42 rbgShift=0.10 characterSmearing=0.18 burnIn=0.18 warmupDuration=8 degaussDuration=2.5 targetResX=320 targetResY=256 pixelScale=0.7 sampleMode=2" \
  320 256 "Topaz Unicode" 14

p AMIGA_WB   "Amiga WorkBench 2 (1990)" \
  "phosphorType=2 phosphorAgeing=0.10 colorTemperature=7000 phosphorPersistence=0.10 screenCurvature=0.20 vignetteIntensity=0.30 ambientReflection=0.05 rasterizationMode=1 scanlinesIntensity=0.40 scanlinesSharpness=0.52 bloom=0.48 glowingLine=0.16 brightness=0.56 contrast=0.83 staticNoise=0.06 jitter=0.08 syncMode=0 horizontalSync=0.04 flickering=0.06 chromaColor=0.68 saturationColor=0.38 rbgShift=0.08 characterSmearing=0.14 burnIn=0.14 warmupDuration=7 degaussDuration=2.2 targetResX=640 targetResY=256 pixelScale=0.7 sampleMode=2" \
  640 256 "Topaz Unicode" 14

p MAC128     "Apple Macintosh 128K (1984)" \
  "phosphorType=2 phosphorAgeing=0.35 colorTemperature=9000 phosphorPersistence=0.08 screenCurvature=0.15 vignetteIntensity=0.55 ambientReflection=0.12 rasterizationMode=2 scanlinesIntensity=0.20 scanlinesSharpness=0.70 bloom=0.35 glowingLine=0.10 brightness=0.60 contrast=0.92 staticNoise=0.03 jitter=0.04 syncMode=0 horizontalSync=0.02 flickering=0.04 chromaColor=0.0 saturationColor=0.0 rbgShift=0.04 characterSmearing=0.05 burnIn=0.40 warmupDuration=6 degaussDuration=2.0 targetResX=512 targetResY=342 pixelScale=0.7 sampleMode=2" \
  512 342 "Silkscreen" 12

p NEXT_STATION "NeXT Station (1990)" \
  "phosphorType=2 phosphorAgeing=0.08 colorTemperature=8000 phosphorPersistence=0.06 screenCurvature=0.08 vignetteIntensity=0.22 ambientReflection=0.05 rasterizationMode=0 bloom=0.32 glowingLine=0.08 brightness=0.62 contrast=0.88 staticNoise=0.02 jitter=0.03 syncMode=0 horizontalSync=0.02 flickering=0.03 chromaColor=0.0 saturationColor=0.0 rbgShift=0.03 characterSmearing=0.04 burnIn=0.12 warmupDuration=5 degaussDuration=1.8 targetResX=1120 targetResY=832 pixelScale=0.7 sampleMode=2" \
  1120 832 "Lucida Console" 13

p SUN3       "Sun-3 Workstation (1985)" \
  "phosphorType=2 phosphorAgeing=0.12 colorTemperature=8200 phosphorPersistence=0.08 screenCurvature=0.10 vignetteIntensity=0.25 ambientReflection=0.04 rasterizationMode=1 scanlinesIntensity=0.28 scanlinesSharpness=0.68 bloom=0.40 glowingLine=0.12 brightness=0.58 contrast=0.86 staticNoise=0.03 jitter=0.04 syncMode=0 horizontalSync=0.02 flickering=0.04 chromaColor=0.0 saturationColor=0.0 rbgShift=0.04 characterSmearing=0.06 burnIn=0.12 warmupDuration=6 degaussDuration=2.0" \
  0 0 "Lucida Console" 14

# ── Late DOS / SVGA ──────────────────────────────────────────────────────────
p SVGA       "SVGA Multisync (1992)" \
  "phosphorType=2 phosphorAgeing=0.06 colorTemperature=7600 phosphorPersistence=0.06 screenCurvature=0.10 vignetteIntensity=0.20 ambientReflection=0.04 rasterizationMode=1 scanlinesIntensity=0.22 scanlinesSharpness=0.72 bloom=0.35 glowingLine=0.10 brightness=0.60 contrast=0.87 staticNoise=0.03 jitter=0.04 syncMode=0 horizontalSync=0.02 flickering=0.04 chromaColor=0.70 saturationColor=0.30 rbgShift=0.05 characterSmearing=0.06 burnIn=0.10 warmupDuration=6 degaussDuration=2.0 targetResX=800 targetResY=600 pixelScale=0.7 sampleMode=2" \
  800 600 "Terminus" 14

p TRINITRON  "Sony Trinitron (1989–1997)" \
  "phosphorType=2 phosphorAgeing=0.05 colorTemperature=7800 phosphorPersistence=0.05 screenCurvature=0.04 vignetteIntensity=0.18 ambientReflection=0.04 rasterizationMode=3 scanlinesIntensity=0.18 scanlinesSharpness=0.78 bloom=0.30 glowingLine=0.08 brightness=0.62 contrast=0.88 staticNoise=0.02 jitter=0.03 syncMode=0 horizontalSync=0.02 flickering=0.03 chromaColor=0.80 saturationColor=0.28 rbgShift=0.04 characterSmearing=0.04 burnIn=0.08 warmupDuration=5 degaussDuration=1.8 targetResX=1024 targetResY=768 pixelScale=0.7 sampleMode=2" \
  1024 768 "Terminus" 14

# Era groupings for the presets command
ERAS=(
    "1960s|IBM2260"
    "1970s|DEC_GT40 DEC_VT100 IBM3270 WYSE50 RADAR TELETEXT"
    "Home computers 1977–1983|APPLE_II COMMODORE_PET TRS80_MODEL1 ATARI800 TRS80_COCO BBC_MICRO VIC20 C64 ZX_SPECTRUM KAYPRO_II SHARP_MZ700 MSX MATTEL_AQUARIUS"
    "IBM PC era 1981–1990|IBM_MDA IBM_CGA IBM_EGA TANDY1000 IBM_VGA COMPAQ_PORTABLE AMSTRAD_PC1512"
    "Professional workstations 1982–1990|DEC_RAINBOW TELEVIDEO_925 NEC_APC3 HP_150 APPLE_LISA ATARI_ST_MONO APPLE_IIGS"
    "Amiga / Mac / NeXT / UNIX 1984–1991|AMIGA500 AMIGA_WB MAC128 NEXT_STATION SUN3"
    "Late DOS / SVGA 1990–1995|SVGA TRINITRON"
)

# ══════════════════════════════════════════════════════════════════════════════
# COMMANDS
# ══════════════════════════════════════════════════════════════════════════════

cmd_presets() {
    echo ""
    echo -e "${BOLD}${C}Available Phosphor presets${NC}"
    for era_entry in "${ERAS[@]}"; do
        IFS='|' read -r era_name era_keys <<< "$era_entry"
        echo -e "\n  ${BOLD}${C}── $era_name${NC}"
        for key in $era_keys; do
            if [[ -n "${PRESET_DATA[$key]+_}" ]]; then
                IFS='|' read -r name _ resX resY font fsize <<< "${PRESET_DATA[$key]}"
                local res_str=""
                if [[ "$resX" != "0" && "$resY" != "0" ]]; then
                    res_str="${resX}×${resY}"
                fi
                printf "  ${BOLD}%-20s${NC}  %-35s  %10s  ${DIM}%s${NC}\n" "$key" "$name" "$res_str" "$font"
            fi
        done
    done
    echo ""
    msg "  Usage: ${BOLD}./phosphor-cli.sh preset <KEY>${NC}"
    msg "  Example: ${BOLD}./phosphor-cli.sh preset IBM_VGA${NC}"
    echo ""
}

cmd_preset() {
    local key="${1:-}"
    [[ -z "$key" ]] && { err "No preset specified."; cmd_presets; return 1; }
    [[ -z "${PRESET_DATA[$key]+_}" ]] && { err "Unknown preset: $key"; cmd_presets; return 1; }

    IFS='|' read -r name params resX resY font fsize <<< "${PRESET_DATA[$key]}"

    echo ""
    echo -e "${BOLD}${C}Preset: $name${NC}"
    echo ""

    # Write all KWin parameters
    for pair in $params; do
        kwrite "${pair%%=*}" "${pair#*=}"
    done

    # Show resolution info
    if [[ "$resX" != "0" && "$resY" != "0" ]]; then
        info "Original resolution: ${BOLD}${resX}×${resY}${NC}"
        info "Pixel scale set to 0.7 (adjust: ./phosphor-cli.sh set pixelScale 1.0)"
    fi

    info "Font: ${BOLD}$font${NC} (${fsize}pt)"
    info "Set this font in your terminal emulator for the authentic look."

    kwin_reload
    ok "Preset '$name' active"
    echo ""
}

cmd_set() {
    local key="${1:-}" val="${2:-}"
    [[ -z "$key" || -z "$val" ]] && die "Usage: ./phosphor-cli.sh set <param> <value>"
    kwrite "$key" "$val"
    kwin_reload
    ok "$key = $val"
}

cmd_get() {
    local key="${1:-}"
    if [[ -z "$key" ]]; then
        # Show all known parameters
        echo ""
        echo -e "${BOLD}${C}Current Phosphor settings${NC}"
        echo ""
        local params=(
            phosphorType phosphorAgeing colorTemperature phosphorPersistence
            screenCurvature vignetteIntensity ambientReflection
            rasterizationMode scanlinesIntensity scanlinesSharpness
            bloom glowingLine brightness contrast
            staticNoise jitter syncMode horizontalSync flickering ghostingIntensity
            chromaColor saturationColor rbgShift characterSmearing burnIn
            warmupEnabled warmupDuration degaussOnStart degaussDuration
            pixelScale sampleMode targetResX targetResY
            targetMode targetClasses
        )
        for p in "${params[@]}"; do
            local v
            v=$(kread "$p")
            [[ -n "$v" ]] && printf "  %-24s = %s\n" "$p" "$v"
        done
        echo ""
    else
        local v
        v=$(kread "$key")
        if [[ -n "$v" ]]; then
            echo "$v"
        else
            err "Parameter '$key' not set or unknown"
            return 1
        fi
    fi
}

cmd_status() {
    echo ""
    echo -e "${BOLD}${C}Phosphor effect status${NC}"
    echo ""
    local mode
    mode=$(kread "targetMode")
    case "$mode" in
        0) msg "  Mode:    ${R}Off${NC}" ;;
        1) msg "  Mode:    ${G}Terminals only${NC}" ;;
        2) msg "  Mode:    ${G}All windows${NC}" ;;
        3) msg "  Mode:    ${G}Custom${NC}" ;;
        *) msg "  Mode:    ${DIM}not configured${NC}" ;;
    esac
    local classes
    classes=$(kread "targetClasses")
    [[ -n "$classes" ]] && msg "  Targets: $classes"
    local ps
    ps=$(kread "pixelScale")
    [[ -n "$ps" ]] && msg "  Pixel scale: $ps"
    local rx ry
    rx=$(kread "targetResX"); ry=$(kread "targetResY")
    [[ -n "$rx" && -n "$ry" ]] && msg "  Resolution: ${rx}×${ry}"
    echo ""
}

cmd_on() {
    kwrite "targetMode" "1"
    kwrite "targetClasses" "$KNOWN_TERMINALS"
    kwin_reload
    ok "Effect enabled (terminals only)"
}

cmd_off() {
    kwrite "targetMode" "0"
    kwrite "targetClasses" ""
    kwin_reload
    ok "Effect disabled"
}

cmd_reload() {
    kwin_reload
    ok "KWin reloaded"
}

cmd_params() {
    cat <<'EOF'

  PARAMETER REFERENCE — Phosphor CRT Effect

  Phosphor & Color
    phosphorType          0=P1 green  1=P3 amber  2=P4 white  3=P39 radar
    phosphorAgeing        0.0–1.0   yellowing (0=new, 1=aged)
    colorTemperature      3000–9300 Kelvin
    phosphorPersistence   0.0–1.0   phosphor afterglow

  Screen Geometry
    screenCurvature       0.0–1.0   barrel distortion
    vignetteIntensity     0.0–1.0   edge darkening
    ambientReflection     0.0–0.30  glass reflection

  Scanlines / Rasterization
    rasterizationMode     0=none  1=scanlines  2=pixel grid  3=sub-pixel RGB
    scanlinesIntensity    0.0–1.0   visibility of gaps
    scanlinesSharpness    0.0–1.0   transition sharpness

  Bloom & Glow
    bloom                 0.0–1.0   glow halo (13-tap Gaussian)
    glowingLine           0.0–1.0   horizontal line glow
    brightness            0.0–1.0   overall brightness
    contrast              0.0–1.0   contrast

  Noise & Sync
    staticNoise           0.0–1.0   static noise
    jitter                0.0–1.0   per-pixel horizontal shift
    syncMode              0=stable  1=sine drift  2=rolling  3=ghosting
    horizontalSync        0.0–1.0   sync artifact intensity
    flickering            0.0–1.0   50/60Hz brightness flicker
    ghostingIntensity     0.0–0.5   frame echo (syncMode=3 only)

  Color & Optics
    chromaColor           0.0–1.0   color retention (0=grayscale, 1=full color)
    saturationColor       0.0–1.0   extra saturation
    rbgShift              0.0–1.0   chromatic aberration
    characterSmearing     0.0–1.0   horizontal character blur
    burnIn                0.0–1.0   phosphor burn-in

  Animations
    warmupEnabled         true/false
    warmupDuration        0.5–30    seconds
    degaussOnStart        true/false
    degaussDuration       0.5–10    seconds

  Pixel Scaling
    pixelScale            0.0–1.0   (0=off, 1=pixel-exact original)
    sampleMode            0=nearest  1=bilinear  2=sharp bilinear (recommended)
    targetResX            original horizontal resolution
    targetResY            original vertical resolution

  Target Windows
    targetMode            0=off  1=terminals  2=all  3=custom
    targetClasses         comma-separated WM_CLASS names

EOF
}

cmd_help() {
    cat <<EOF

${BOLD}phosphor-cli${NC} — CLI helper for the Phosphor KWin CRT effect

${BOLD}USAGE${NC}
  ./phosphor-cli.sh <command> [args]

${BOLD}COMMANDS${NC}
  ${C}presets${NC}              List all available presets
  ${C}preset <KEY>${NC}         Apply a preset (writes kwinrc + reloads KWin)
  ${C}set <param> <value>${NC}  Set a single parameter
  ${C}get [param]${NC}          Read parameter(s) — omit param for all
  ${C}status${NC}               Show current effect status
  ${C}on${NC}                   Enable (terminals only)
  ${C}off${NC}                  Disable
  ${C}reload${NC}               Reload KWin
  ${C}params${NC}               Show parameter reference
  ${C}help${NC}                 This help

${BOLD}EXAMPLES${NC}
  ./phosphor-cli.sh preset IBM_VGA
  ./phosphor-cli.sh set pixelScale 1.0
  ./phosphor-cli.sh set phosphorType 0
  ./phosphor-cli.sh get bloom
  ./phosphor-cli.sh on

EOF
}

# ══════════════════════════════════════════════════════════════════════════════
# Main dispatcher
# ══════════════════════════════════════════════════════════════════════════════
case "${1:-help}" in
    presets)         cmd_presets ;;
    preset)          cmd_preset "${2:-}" ;;
    set)             cmd_set "${2:-}" "${3:-}" ;;
    get)             cmd_get "${2:-}" ;;
    status)          cmd_status ;;
    on)              cmd_on ;;
    off)             cmd_off ;;
    reload)          cmd_reload ;;
    params)          cmd_params ;;
    help|--help|-h)  cmd_help ;;
    *)               err "Unknown command: $1"; cmd_help; exit 1 ;;
esac
