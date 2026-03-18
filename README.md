# Phosphor — KDE Plasma 6 CRT Shader Effect

A nostalgic CRT phosphor simulation effect for KDE Plasma 6, featuring 46 historical presets, pixel scaling, and full GUI configuration.

## Quick Start

### Bash-only Installation (Recommended)
Works immediately without compilation:

```bash
chmod +x phosphor  # (will add CLI script later)
./phosphor install
./phosphor on
./phosphor preset C64
```

### C++ Compiled Version 
Full build from source (requires Qt6 development environment):

```bash
cd src
chmod +x build.sh
./build.sh
```

## Project Structure

```
phosphor/
├── src/                    KWin plugin (C++ & GLSL shader)
│   ├── build.sh           Automated build & install
│   ├── CMakeLists.txt     Build configuration
│   ├── metadata.json      Plugin metadata
│   ├── retro.frag         GLSL 1.40 fragment shader
│   ├── retro_term_effect.h/.cpp   C++ plugin code
│
├── variants/              Alternative implementations
│   └── retro-term-scale/  Pixel-perfect scaling variant
│
├── scripts/               CLI management tools
│   └── (coming soon)
│
├── docs/                  Documentation
│   ├── PROJECT.md         Main README
│   └── VARIANTS.md        Variant documentation
│
└── .gitignore            Git configuration
```

## Documentation

- **[Project Overview](docs/PROJECT.md)** — Installation, usage, requirements
- **[Build Guide](docs/PROJECT.md#build-options)** — Compilation instructions
- **[Variants](docs/VARIANTS.md)** — Alternative versions & features

## Features

- 46 historical phosphor presets (P1/P3/P4/P39)
- Bloom, scanlines, barrel curvature
- Sync artifacts, warmup & degauss animations
- Pixel scaling (1x/2x/3x)
- Full GUI configuration
- Per-window targeting

## Requirements

### For Bash-only:
- KWin 6 (included with KDE Plasma 6)

### For C++ Build:
- CMake ≥ 3.20
- GCC/C++20
- Qt6 + OpenGL
- KDE Frameworks 6
- KWin development headers

Install on Arch:
```bash
sudo pacman -S cmake gcc extra-cmake-modules kwin qt6-base kconfig kcoreaddons
```

## License

GPL-2.0-or-later

## Links

- GitHub: https://github.com/pedrohx26/phosphor
- KDE Plasma: https://kde.org/plasma
