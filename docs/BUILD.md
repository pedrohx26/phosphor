# Phosphor — KDE Plasma 6 C++ Effect Plugin

A CRT phosphor simulation as a compiled KWin 6 plugin.  
Appears in **System Settings → Workspace Behavior → Screen Effects → "Phosphor CRT"** after installation.

---

## Quick Start

```bash
# Install dependencies (Arch / Garuda)
sudo pacman -S cmake make gcc extra-cmake-modules kwin qt6-base kconfig kcoreaddons

# Build and install
cd src
chmod +x build.sh
./build.sh
```

Then enable: **System Settings → Workspace Behavior → Screen Effects → "Phosphor CRT"**

---

## Project Structure

```
phosphor/
├── README.md                  Project overview
├── docs/BUILD.md              This documentation
└── src/
    ├── build.sh               Fully automated build and install script
    ├── CMakeLists.txt         CMake configuration
    ├── metadata.json          KWin plugin metadata
    ├── retro_term_effect.h    C++ effect header
    ├── retro_term_effect.cpp  C++ effect implementation
    └── retro.frag             GLSL 1.40 fragment shader with optional pixel scaling
```

---

## Build Options

```bash
cd src
./build.sh                   # Standard Release build + installation
./build.sh --check-deps      # Check dependencies only
./build.sh --rebuild         # Clean rebuild (delete build/)
./build.sh --debug           # Debug build (more logging via journalctl)
./build.sh --uninstall       # Remove installed plugin
./build.sh --prefix=/path    # Alternative install prefix (default: /usr)
```

---

## Requirements

| Package | Arch name |
|---|---|
| CMake ≥ 3.20 | `cmake` |
| GCC / C++20 | `gcc` |
| Extra CMake Modules | `extra-cmake-modules` |
| KWin 6 (with headers) | `kwin` |
| Qt6 Base + OpenGL | `qt6-base` |
| KF6 Config | `kconfig` |
| KF6 CoreAddons | `kcoreaddons` |

Install with:
```bash
sudo pacman -S cmake make gcc extra-cmake-modules kwin qt6-base kconfig kcoreaddons
```

---

## What the Build Script Does

1. **Dependency check** — verifies all requirements, shows which package is missing
2. **CMake configuration** — detects KWin and Qt/KF6 versions automatically
3. **Compilation** — with all CPU cores (`nproc`)
4. **`sudo cmake --install`** — installs `.so`, shader, and metadata to correct locations
5. **KWin reload** — via `qdbus6 org.kde.KWin /KWin reconfigure`
6. **Autostart entry** — `~/.config/autostart/phosphor-rebuild.desktop` rebuilds after KWin package updates

---

## Installation Paths

After `./build.sh`, files are installed to:

```
/usr/lib/qt6/plugins/kwin/effects/plugins/kwin_effect_retro_term.so
/usr/share/kwin/effects/retro-term/retro.frag
/usr/share/kwin/effects/retro-term/metadata.json
```

---

## Configuration

All parameters are read from `~/.config/kwinrc` section `[Effect-retro-terminal]`
and can be configured via the `retro-term` management script:

```bash
retro-term on
retro-term preset IBM_VGA
```
retro-term preset DEC_VT100
retro-term preset AMIGA500
retro-term set bloom 0.70
retro-term set phosphorType 0     # 0=P1 groen  1=P3 amber  2=P4 wit  3=P39 radar
retro-term set screenCurvature 0.3
retro-term set warmupEnabled true
retro-term set warmupDuration 12
retro-term set targetClasses "konsole,alacritty"
retro-term get                    # alle huidige instellingen tonen
```

---

## Na een KWin-update

Na elke `pacman -Syu` die KWin bijwerkt, moet de plugin worden herbouwd. De autostart-entry
doet dit automatisch bij de volgende login. Handmatig:

```bash
./build.sh --rebuild
```

Debug-logging volgen:
```bash
journalctl -f | grep retro-term
```

---

## Hoe het werkt

De plugin implementeert de `KWin::Effect` C++20 interface en koppelt in op `paintWindow()`.
Voor elk venster dat overeenkomt met `targetClasses` wordt:

1. De GLSL-shader geactiveerd via `ShaderManager::instance()->pushShader()`
2. Alle 30 uniform-variabelen gezet per frame
3. `effects->paintWindow()` aangeroepen — KWin rendert het venster door de shader
4. Per-venster animatietimers bijgehouden voor warmup en degauss

De GLSL-shader implementeert in volgorde:
barrel distortion, sync-vervorming, jitter, chromatische aberratie, karakter-smearing,
bloom, ghosting, fosfor-persistentie, fosfor-tint, kleurtemperatuur, saturatie,
contrast/helderheid, scanlines, statische ruis, flickering, vignette, schermglas-reflectie,
burn-in, glowing line, warmup-animatie, degauss-animatie.

---

## Licentie

GPL-2.0-or-later
