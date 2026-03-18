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
retro-term set phosphorType 0     # 0=P1 green  1=P3 amber  2=P4 white  3=P39 radar
retro-term set screenCurvature 0.3
retro-term set warmupEnabled true
retro-term set warmupDuration 12
retro-term set targetClasses "konsole,alacritty"
retro-term get                    # show all current settings
```

---

## After a KWin Update

After each `pacman -Syu` that updates KWin, the plugin should be rebuilt.
The autostart entry does this automatically at the next login. Manual rebuild:

```bash
./build.sh --rebuild
```

Follow debug logging:
```bash
journalctl -f | grep retro-term
```

---

## How It Works

The plugin implements the `KWin::Effect` C++20 interface and hooks into `paintWindow()`.
For each window that matches `targetClasses` it:

1. Activates the GLSL shader via `ShaderManager::instance()->pushShader()`
2. Sets all uniforms each frame
3. Calls `effects->paintWindow()` so KWin renders the window through the shader
4. Tracks per-window animation timers for warmup and degauss

The GLSL shader applies, in order:
barrel distortion, sync distortion, jitter, chromatic aberration, character smearing,
bloom, ghosting, phosphor persistence, phosphor tint, color temperature, saturation,
contrast/brightness, scanlines, static noise, flickering, vignette, glass reflection,
burn-in, glowing line, warmup animation, degauss animation.

---

## License

GPL-2.0-or-later
