# Phosphor — KDE Plasma 6 C++ Effect Plugin

A CRT phosphor simulation as a compiled KWin 6 plugin.
Appears in System Settings -> Workspace Behavior -> Screen Effects -> Phosphor CRT after installation.

## Quick Start

```bash
# Install dependencies (Arch / Garuda)
sudo pacman -S cmake make gcc extra-cmake-modules kwin qt6-base kconfig kcoreaddons kcmutils ki18n kconfigwidgets

# Build and install
cd src
chmod +x build.sh
./build.sh
```

Then enable: System Settings -> Workspace Behavior -> Screen Effects -> Phosphor CRT.
Click Settings to access presets, scope mode, and sliders.

## Project Structure

```text
phosphor/
├── README.md                  This documentation
├── src/
│   ├── build.sh               Fully automated build and install script
│   ├── CMakeLists.txt         CMake configuration
│   ├── metadata.json          KWin plugin metadata
│   ├── retro_term_effect.h    C++ effect header
│   ├── retro_term_effect.cpp  C++ effect implementation
│   ├── retro_term_kcm.h       KCM UI header (presets + scope)
│   ├── retro_term_kcm.cpp     KCM UI implementation
│   ├── retro-term-kcm.desktop.in  KCM service descriptor
│   └── retro.frag             GLSL 1.40 fragment shader with optional pixel scaling
└── .gitignore
```

## Features

- CRT phosphor simulation (P1/P3/P4/P39)
- Bloom, scanlines, barrel curvature
- Sync artifacts, flicker, jitter, burn-in
- Warmup and degauss animations
- Built-in pixel scaling (`pixelScale`, `targetRes`, `sampleMode`)
- Per-window targeting via `targetClasses`
- Preset and scope UI in System Settings (Off / Terminals / All / Custom)

Pixel scaling behavior:

- `pixelScale = 0.0` -> scaling off (modern/default look)
- `pixelScale = 1.0` -> pixel-exact historical resolution
- values between `0.0` and `1.0` -> blended transition

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

## Requirements

| Package | Arch name |
|---|---|
| CMake >= 3.20 | `cmake` |
| GCC / C++20 | `gcc` |
| Extra CMake Modules | `extra-cmake-modules` |
| KWin 6 (with headers) | `kwin` |
| Qt6 Base + OpenGL | `qt6-base` |
| KF6 Config | `kconfig` |
| KF6 CoreAddons | `kcoreaddons` |
| KF6 KCMUtils | `kcmutils` |
| KF6 I18n | `ki18n` |
| KF6 ConfigWidgets | `kconfigwidgets` |

Install with:

```bash
sudo pacman -S cmake make gcc extra-cmake-modules kwin qt6-base kconfig kcoreaddons kcmutils ki18n kconfigwidgets
```

## What the Build Script Does

1. Dependency check — verifies all requirements and reports missing packages.
2. CMake configuration — detects KWin and Qt/KF6 versions automatically.
3. Compilation — uses all CPU cores (`nproc`).
4. `sudo cmake --install` — installs `.so`, shader, and metadata.
5. KWin reload — via `qdbus6 org.kde.KWin /KWin reconfigure`.
6. Autostart entry — `~/.config/autostart/phosphor-rebuild.desktop` rebuilds after KWin package updates.

## Installation Paths

After `./build.sh`, files are installed to:

```text
/usr/lib/qt6/plugins/kwin/effects/plugins/kwin_effect_retro_term.so
/usr/lib/qt6/plugins/plasma/kcms/systemsettings_qwidgets/kcm_retro_term.so
/usr/share/kwin/effects/retro-term/retro.frag
/usr/share/kwin/effects/retro-term/metadata.json
/usr/share/kservices6/retro-term-kcm.desktop
```

## Configuration

All parameters are read from `~/.config/kwinrc` section `[Effect-retro-terminal]`.

Primary workflow (recommended):

1. Open System Settings -> Workspace Behavior -> Screen Effects.
2. Enable Phosphor CRT.
3. Click Settings.
4. Pick a preset and scope mode (Off / Terminals / All / Custom).
5. Apply & Reload KWin.

Advanced CLI workflow (optional):

You can edit values with `kwriteconfig6` and then reload KWin.

Example:

```bash
kwriteconfig6 --file kwinrc --group Effect-retro-terminal --key phosphorType 0
kwriteconfig6 --file kwinrc --group Effect-retro-terminal --key bloom 0.70
kwriteconfig6 --file kwinrc --group Effect-retro-terminal --key screenCurvature 0.30
kwriteconfig6 --file kwinrc --group Effect-retro-terminal --key warmupEnabled true
kwriteconfig6 --file kwinrc --group Effect-retro-terminal --key targetClasses "konsole,alacritty"
qdbus6 org.kde.KWin /KWin reconfigure
```

Phosphor type values:

- `0` = P1 green
- `1` = P3 amber
- `2` = P4 white
- `3` = P39 radar

## After a KWin Update

After each `pacman -Syu` that updates KWin, the plugin should be rebuilt.
The autostart entry does this automatically at the next login.

Manual rebuild:

```bash
cd src
./build.sh --rebuild
```

Follow debug logging:

```bash
journalctl -f | grep retro-term
```

## How It Works

The plugin implements the `KWin::Effect` C++20 interface and hooks into `paintWindow()`.
For each window that matches `targetClasses` it:

1. Activates the GLSL shader via `ShaderManager::instance()->pushShader()`.
2. Sets all uniforms each frame.
3. Calls `effects->paintWindow()` so KWin renders the window through the shader.
4. Tracks per-window animation timers for warmup and degauss.

The GLSL shader applies, in order:
barrel distortion, sync distortion, jitter, chromatic aberration, character smearing,
bloom, ghosting, phosphor persistence, phosphor tint, color temperature, saturation,
contrast/brightness, scanlines, static noise, flickering, vignette, glass reflection,
burn-in, glowing line, warmup animation, degauss animation.

## License

GPL-2.0-or-later
