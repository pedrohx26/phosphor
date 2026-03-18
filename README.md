# Phosphor — KDE Plasma 6 CRT Shader Effect

A KWin 6 CRT phosphor simulation plugin for KDE Plasma.

This repository now contains a single implementation path. Pixel scaling is built in and can be configured per profile:

- `pixelScale = 0.0` -> scaling off (modern/default look)
- `pixelScale = 1.0` -> pixel-exact historical resolution
- values between `0.0` and `1.0` -> blended transition

## Quick Start

```bash
cd src
chmod +x build.sh
./build.sh
```

Then enable in System Settings:

`Workspace Behavior -> Screen Effects -> Phosphor CRT`

## Project Structure

```
phosphor/
├── src/
│   ├── build.sh
│   ├── CMakeLists.txt
│   ├── metadata.json
│   ├── retro.frag
│   ├── retro_term_effect.h
│   └── retro_term_effect.cpp
├── docs/
│   └── BUILD.md
├── scripts/
└── .gitignore
```

## Features

- CRT phosphor simulation (P1/P3/P4/P39)
- Bloom, scanlines, barrel curvature
- Sync artifacts, flicker, jitter, burn-in
- Warmup and degauss animations
- Built-in pixel scaling (`pixelScale`, `targetRes`, `sampleMode`)
- Per-window targeting via `targetClasses`

## Build Requirements

Install on Arch/Garuda:

```bash
sudo pacman -S cmake make gcc extra-cmake-modules kwin qt6-base kconfig kcoreaddons
```

Detailed instructions: `docs/BUILD.md`

## License

GPL-2.0-or-later
