# Phosphor — KDE Plasma 6 CRT Effect Plugin

A CRT phosphor simulation as a compiled KWin 6 plugin.
Turns your terminal (or any window) into a faithful recreation of classic CRT monitors — from 1960s IBM mainframes to 1990s Trinitrons.

Appears in **System Settings → Workspace Behavior → Screen Effects → Phosphor CRT** after installation.

> **Terminal-first design** — Phosphor is built for terminal users.
> The default mode applies the effect only to terminal emulators (Konsole, Yakuake, kitty, Alacritty, etc.)
> so your IDE, browser, and other apps stay untouched.  Set a historical preset, pick a matching
> retro font, and work in a terminal that looks and feels like a DEC VT100 or Commodore 64.

---

## Quick Start

```bash
# Install dependencies (Arch / Garuda)
sudo pacman -S cmake make gcc extra-cmake-modules kwin qt6-base \
    kconfig kcoreaddons kcmutils ki18n kconfigwidgets

# Build and install
cd src
chmod +x build.sh
./build.sh
```

Then enable: **System Settings → Workspace Behavior → Screen Effects → Phosphor CRT**.
Click **Settings** to access presets, scope mode, and sliders.

Optional — install retro fonts and the CLI helper:

```bash
./install-fonts.sh          # download all retro fonts used by presets
./phosphor-cli.sh presets   # list all 43 presets
./phosphor-cli.sh preset IBM_VGA   # apply a preset from the terminal
```

---

## Terminal How-To

### 1. Enable the effect

Open System Settings → Screen Effects → enable **Phosphor CRT** → click **Settings**.

### 2. Pick a preset

Choose a historical preset from the dropdown (e.g. *DEC VT100*, *Commodore 64*, *IBM PS/2 VGA*).
Click **Load preset** — all shader parameters and the pixel scaling resolution are filled in automatically.

### 3. Set the scope

The scope selector at the top determines which windows get the effect:

| Mode | What receives the effect |
|------|--------------------------|
| **Off** | Nothing — effect stays loaded but inactive |
| **Terminals only** | Konsole, Yakuake, kitty, Alacritty, wezterm, xterm, gnome-terminal, tilix |
| **All windows** | Every window on screen becomes retro |
| **Custom** | You specify WM\_CLASS names (use `xprop WM_CLASS` to find them) |

### 4. Install a matching font

Each preset is designed for a specific retro font. See the [Font Installation](#font-installation) section below for all 20+ font families and where to get them.

Set the font in your terminal emulator:
- **Konsole**: Settings → Edit Profile → Appearance → Font
- **kitty**: `font_family` in `~/.config/kitty/kitty.conf`
- **Alacritty**: `font.normal.family` in `~/.config/alacritty/alacritty.toml`

### 5. Apply & reload

Click **✓ Apply & reload KWin** — the effect becomes active immediately.

### Advanced CLI workflow

The included `phosphor-cli.sh` wraps `kwriteconfig6` / `kreadconfig6` and provides preset management:

```bash
./phosphor-cli.sh presets            # list all 43 presets by era
./phosphor-cli.sh preset DEC_VT100   # apply a preset (writes kwinrc + reloads KWin)
./phosphor-cli.sh status             # show current mode, resolution, targets
./phosphor-cli.sh get                # dump all active parameters
./phosphor-cli.sh set bloom 0.80     # tweak a single parameter
./phosphor-cli.sh on                 # enable (terminals only)
./phosphor-cli.sh off                # disable
./phosphor-cli.sh params             # full parameter reference
```

Or use `kwriteconfig6` directly:

```bash
kwriteconfig6 --file kwinrc --group Effect-retro-terminal --key phosphorType 0
kwriteconfig6 --file kwinrc --group Effect-retro-terminal --key bloom 0.70
qdbus6 org.kde.KWin /KWin reconfigure
```

---

## Presets

43 built-in presets covering real hardware from 1964 to 1997, plus a low-GPU minimal mode.

### 1960s

| Preset | System | Phosphor | Resolution | Font |
|--------|--------|----------|------------|------|
| IBM 2260 (1964) | First IBM video terminal | P4 white | 640×250 | Glass TTY VT220 |

### 1970s

| Preset | System | Phosphor | Resolution | Font |
|--------|--------|----------|------------|------|
| DEC GT40 (1972) | PDP-11 vector terminal | P39 radar | 1024×768 | VT323 |
| DEC VT100 (1978) | The reference terminal | P1 green | 800×240 | VT323 |
| IBM 3270 (1971–1980s) | Mainframe block-mode terminal | P1 green | 720×350 | PxPlus IBM 3270 |
| Wyse WY-50 (1979) | UNIX work terminal | P1 green | 720×360 | PxPlus Wyse WY700b |
| Military Radar (1965) | SAGE radar console | P39 radar | 1024×1024 | Share Tech Mono |
| Teletext / Ceefax (1974) | Analog teletext PAL-TV | P4 white | 480×250 | Bedstead |

### Home Computers 1977–1983

| Preset | System | Phosphor | Resolution | Font |
|--------|--------|----------|------------|------|
| Apple II (1977) | NTSC-TV, composite video | P4 white | 280×192 | Print Char 21 |
| Commodore PET 2001 (1977) | Built-in 9″ white CRT | P4 white | 320×200 | Pet Me 2Y |
| TRS-80 Model I (1977) | Composite to TV, uppercase-only | P4 white | 384×192 | Another Man's Treasure |
| Atari 400/800 (1979) | ANTIC/GTIA, NTSC-TV | P4 white | 320×192 | Atari Classic |
| TRS-80 Color Computer (1980) | MC6847, composite color TV | P4 white | 256×192 | Hot CoCo |
| BBC Micro (1981) | British school computer | P4 white | 320×256 | Bedstead |
| Commodore VIC-20 (1981) | First color Commodore | P4 white | — | C64 Pro Mono |
| Commodore 64 (1982) | Best-selling home computer | P4 white | 320×200 | C64 Pro Mono |
| ZX Spectrum (1982) | British home computer icon | P4 white | 256×192 | VT323 |
| Kaypro II (1982) | Portable CP/M, 9″ green CRT | P1 green | 640×192 | PxPlus Kaypro 2000 |
| Sharp MZ-700 (1982) | Japanese Sharp, 12″ white CRT | P4 white | 320×200 | Mizuno |
| MSX (1983) | Japanese standard (Sony, Philips) | P4 white | — | VT323 |
| Mattel Aquarius (1983) | Mattel's failed home computer | P4 white | 320×200 | Antiquarius |

### IBM PC Era 1981–1990

| Preset | System | Phosphor | Resolution | Font |
|--------|--------|----------|------------|------|
| IBM PC MDA (1981) | IBM 5151, monochrome | P39 radar | 720×350 | PxPlus IBM MDA |
| IBM PC CGA (1981) | IBM 5153, composite color | P4 white | 320×200 | PxPlus IBM CGA |
| IBM PC EGA (1984) | IBM 5154, 16 colors | P4 white | 640×350 | PxPlus IBM EGA 8x14 |
| Tandy 1000 (1984) | Enhanced CGA, games | P4 white | 320×200 | PxPlus Tandy 1000 |
| IBM PS/2 VGA (1987) | The DOS standard | P4 white | 720×400 | PxPlus IBM VGA 9x16 |
| Compaq Portable (1982) | First IBM clone, 9″ amber | P3 amber | 640×200 | PxPlus CompaqPort |
| Amstrad PC1512 (1986) | Cheap British IBM clone | P4 white | 640×200 | PxPlus Amstrad PC-2y |

### Professional Workstations / Terminals 1982–1990

| Preset | System | Phosphor | Resolution | Font |
|--------|--------|----------|------------|------|
| DEC Rainbow 100 (1982) | CP/M+DOS hybrid, VR201 green | P1 green | 800×240 | PxPlus DEC Rainbow |
| TeleVideo TVI-925 (1982) | UNIX terminal, 12″ P1 green | P1 green | 720×360 | PxPlus TeleVideo TVI-925 |
| NEC APC III (1983) | Japanese professional PC, 640×400 | P1 green | 640×400 | PxPlus NEC APC3 |
| HP 150 Touchscreen (1983) | HP's first touchscreen PC | P4 white | 640×256 | PxPlus HP 150 |
| Apple Lisa (1983) | First Apple GUI, Sony 12″ b/w | P4 white | 720×364 | LisaTerminal Paper |
| Atari ST SM124 (1985) | Atari ST mono, 640×400 | P4 white | 640×400 | Project Jason |
| Apple IIgs (1986) | RGB monitor, 4096 colors | P4 white | 320×200 | Shaston 320 |

### Amiga / Mac / NeXT / UNIX 1984–1991

| Preset | System | Phosphor | Resolution | Font |
|--------|--------|----------|------------|------|
| Apple Macintosh 128K (1984) | 9″ Sony CRT b/w | P4 white | 512×342 | Silkscreen |
| Sun-3 Workstation (1985) | UNIX lab computer | P4 white | — | Lucida Console |
| Amiga 500 (1987) | PAL-TV or 1084S RGB | P4 white | 320×256 | Topaz Unicode |
| Amiga WorkBench 2 (1990) | 1084S RGB monitor | P4 white | 640×256 | Topaz Unicode |
| NeXT Station (1990) | 1120×832 grayscale | P4 white | 1120×832 | Lucida Console |

### Late DOS / SVGA 1990–1995

| Preset | System | Phosphor | Resolution | Font |
|--------|--------|----------|------------|------|
| SVGA Multisync (1992) | 800×600, shadow mask | P4 white | 800×600 | Terminus |
| Sony Trinitron (1989–1997) | Aperture grille, near flat | P4 white | 1024×768 | Terminus |

### Utility

| Preset | Description | Font |
|--------|-------------|------|
| Minimal (low GPU) | Subtle effect, minimum GPU load | Terminus |

*Resolution "—" means no pixel scaling is applied (modern display).*

---

## Parameter Reference

All parameters are stored in `~/.config/kwinrc` under `[Effect-retro-terminal]`.

### Phosphor & Color

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| `phosphorType` | int | 0–3 | 0=P1 green, 1=P3 amber, 2=P4 white, 3=P39 radar |
| `phosphorAgeing` | float | 0.0–1.0 | Yellowing (0=new, 1=aged) |
| `colorTemperature` | float | 3000–9300 | Kelvin (3000=warm yellow, 9300=cold blue-white) |
| `phosphorPersistence` | float | 0.0–1.0 | Phosphor afterglow duration |

### Screen Geometry

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| `screenCurvature` | float | 0.0–1.0 | Barrel distortion (0=flat, 1=strongly curved) |
| `vignetteIntensity` | float | 0.0–1.0 | Edge darkening |
| `ambientReflection` | float | 0.0–0.30 | Screen glass reflection |

### Scanlines / Rasterization

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| `rasterizationMode` | int | 0–3 | 0=none, 1=scanlines, 2=pixel grid, 3=sub-pixel RGB |
| `scanlinesIntensity` | float | 0.0–1.0 | How dark the gaps between lines are |
| `scanlinesSharpness` | float | 0.0–1.0 | Transition sharpness (0=soft, 1=sharp) |

### Bloom & Glow

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| `bloom` | float | 0.0–1.0 | Glow halo (13-tap Gaussian blur) |
| `glowingLine` | float | 0.0–1.0 | Horizontal line glow |
| `brightness` | float | 0.0–1.0 | Overall brightness |
| `contrast` | float | 0.0–1.0 | Contrast |

### Noise & Sync Artifacts

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| `staticNoise` | float | 0.0–1.0 | Grain-like image noise |
| `jitter` | float | 0.0–1.0 | Per-pixel horizontal offset |
| `syncMode` | int | 0–3 | 0=stable, 1=sine drift, 2=rolling scan, 3=ghosting |
| `horizontalSync` | float | 0.0–1.0 | Sync artifact intensity |
| `flickering` | float | 0.0–1.0 | 50/60Hz brightness flicker |
| `ghostingIntensity` | float | 0.0–0.5 | Frame echo (only with `syncMode=3`) |

### Color & Optical Aberrations

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| `chromaColor` | float | 0.0–1.0 | Color retention (0=grayscale, 1=full color) |
| `saturationColor` | float | 0.0–1.0 | Additional color saturation |
| `rbgShift` | float | 0.0–1.0 | Chromatic aberration (horizontally shifted RGB) |
| `characterSmearing` | float | 0.0–1.0 | Horizontal character blur |
| `burnIn` | float | 0.0–1.0 | Phosphor burn-in (brighter screen center) |

### Animations

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| `warmupEnabled` | bool | true/false | CRT warmup animation when opening a window |
| `warmupDuration` | float | 0.5–30 | Warmup duration in seconds |
| `degaussOnStart` | bool | true/false | Degauss animation when opening a window |
| `degaussDuration` | float | 0.5–10 | Degauss duration in seconds |

### Pixel Scaling

Simulates the original screen resolution of historical systems by downscaling and re-upscaling the window content.

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| `pixelScale` | float | 0.0–1.0 | 0.0=no scaling (modern), 1.0=exact original pixels |
| `sampleMode` | int | 0–2 | 0=nearest-neighbour, 1=bilinear, 2=sharp bilinear |
| `targetResX` | float | 40–3840 | Original horizontal resolution (pixels) |
| `targetResY` | float | 24–2160 | Original vertical resolution (pixels) |

**How pixel scaling works:**
The shader quantizes UV coordinates to the target resolution grid before sampling.
At `pixelScale=0`, the original UV passes through unchanged.
At `pixelScale=1`, UVs snap to the nearest cell of the target resolution, producing true block pixels.
Values between 0 and 1 blend both modes smoothly.

**Sharp bilinear** (`sampleMode=2`, recommended) simulates a CRT's Gaussian electron beam — crisp pixel edges without harsh stair-stepping. This produces the most authentic CRT look.

When loading a preset that includes a resolution, `pixelScale` is automatically set to 0.7 (a good balance
between retro appearance and readability) and `sampleMode` to sharp bilinear.

### Target Windows

| Parameter | Type | Description |
|-----------|------|-------------|
| `targetMode` | int | 0=off, 1=terminals, 2=all windows, 3=custom |
| `targetClasses` | string | Comma-separated WM\_CLASS names (lowercase) |

Special values for `targetClasses`:
- Empty string: effect disabled
- `*`: all windows
- `konsole,yakuake,kitty,...`: specific windows

---

## Font Installation

Each preset is designed for a specific font that matches the original hardware's character ROM.
Below are all font families used by presets, grouped by source.

### int10h Oldschool PC Font Pack (CC BY-SA 4.0)

The largest collection of authentic PC fonts. Contains all IBM, Tandy, Wyse, Kaypro, Compaq, DEC, TeleVideo, Amstrad, NEC, and HP fonts.

**Used by:** IBM 2260^, IBM 3270, Wyse WY-50, IBM MDA, IBM CGA, IBM EGA, Tandy 1000, IBM VGA, Compaq Portable, DEC Rainbow, TeleVideo TVI-925, Amstrad PC1512, NEC APC III, HP 150, Kaypro II

**Install (Arch):**
```bash
# AUR package
yay -S ttf-oldschool-pc-font-pack
# Or manual download
curl -LO https://int10h.org/oldschool-pc-fonts/download/oldschool_pc_font_pack_v2.2_linux.zip
unzip oldschool_pc_font_pack_v2.2_linux.zip -d /tmp/int10h
cp /tmp/int10h/**/*.ttf ~/.local/share/fonts/
fc-cache -fv
```

**Source:** https://int10h.org/oldschool-pc-fonts/

^IBM 2260 uses Glass TTY VT220 (see below)

### VT323 (SIL OFL — Google Fonts)

**Used by:** DEC GT40, DEC VT100, ZX Spectrum, MSX, WarGames, Matrix

```bash
# AUR
yay -S ttf-google-fonts-vt323
# Or manual
curl -L "https://fonts.google.com/download?family=VT323" -o /tmp/vt323.zip
unzip /tmp/vt323.zip -d ~/.local/share/fonts/
fc-cache -fv
```

### C64 Pro Mono

**Used by:** Commodore 64, Commodore VIC-20

**Source:** https://style64.org/c64-truetype

### Topaz Unicode

**Used by:** Amiga 500, Amiga WorkBench 2

**Source:** https://github.com/rewtnull/amigafonts

### Kreativekorp Retro Fonts (free)

Individual downloads for specific systems:

| Font | Used by | Download |
|------|---------|----------|
| Pet Me 2Y | Commodore PET 2001 | https://www.kreativekorp.com/swdownload/fonts/retro/petme.zip |
| Another Man's Treasure | TRS-80 Model I | https://www.kreativekorp.com/swdownload/fonts/retro/amtreasure.zip |
| Hot CoCo | TRS-80 Color Computer | https://www.kreativekorp.com/swdownload/fonts/retro/hotcoco.zip |
| LisaTerminal Paper | Apple Lisa | https://www.kreativekorp.com/swdownload/fonts/retro/lisa1.zip |
| Shaston 320 | Apple IIgs | https://www.kreativekorp.com/swdownload/fonts/retro/shaston.zip |
| Mizuno | Sharp MZ-700 | https://www.kreativekorp.com/swdownload/fonts/retro/mizuno.zip |
| Antiquarius | Mattel Aquarius | https://www.kreativekorp.com/swdownload/fonts/retro/aq2.zip |
| Project Jason | Atari ST SM124 | https://www.kreativekorp.com/swdownload/fonts/retro/projason.zip |

Install any Kreativekorp font:
```bash
curl -LO https://www.kreativekorp.com/swdownload/fonts/retro/<package>.zip
unzip <package>.zip -d /tmp/<package>
find /tmp/<package> \( -name "*.ttf" -o -name "*.otf" \) -exec cp {} ~/.local/share/fonts/ \;
fc-cache -fv
```

### Other Fonts

| Font | Used by | License | Source |
|------|---------|---------|--------|
| Share Tech Mono | Radar | SIL OFL | https://fonts.google.com/specimen/Share+Tech+Mono |
| Silkscreen | Apple Macintosh 128K | SIL OFL | https://kottke.org/plus/type/silkscreen/ |
| Terminus | SVGA, Trinitron | SIL OFL | https://terminus-font.sourceforge.net/ (`pacman -S terminus-font`) |
| Bedstead | Teletext, BBC Micro | Free | https://bjh21.me.uk/bedstead/ |
| Glass TTY VT220 | IBM 2260 | Free | https://github.com/svofski/glasstty |
| Print Char 21 | Apple II | Free | https://www.kreativekorp.com/software/fonts/apple2/ |
| Atari Classic | Atari 400/800 | Free | https://www.dafont.com/atari-classic.font |
| Lucida Console | NeXT Station, Sun-3 | System font | Pre-installed on most systems |

---

## Project Structure

```text
phosphor/
├── README.md                  This documentation
├── install-fonts.sh           Standalone font installer (17 font groups)
├── phosphor-cli.sh            CLI helper: presets, status, set/get params
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

- CRT phosphor simulation (P1 green / P3 amber / P4 white / P39 radar)
- 43 historical presets with authentic parameters and font recommendations
- Bloom, scanlines (classic / pixel grid / aperture grille)
- Barrel curvature, vignette, glass reflection
- Sync artifacts, flicker, jitter, character smearing
- Phosphor persistence, burn-in, chromatic aberration
- Warmup and degauss animations
- Built-in pixel scaling to original hardware resolutions
- Per-window scoping (Off / Terminals / All / Custom)
- Full KCM UI in System Settings with sliders, presets, and quick-apply

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

Install all at once:

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

## How It Works

The plugin implements the `KWin::Effect` C++20 interface and hooks into `paintWindow()`.
For each window that matches `targetClasses` it:

1. Activates the GLSL shader via `ShaderManager::instance()->pushShader()`.
2. Sets all uniforms each frame.
3. Calls `effects->paintWindow()` so KWin renders the window through the shader.
4. Tracks per-window animation timers for warmup and degauss.

The GLSL shader applies, in order:
barrel distortion, pixel scaling, sync distortion, jitter, chromatic aberration, character smearing,
bloom, ghosting, phosphor persistence, phosphor tint, color temperature, saturation,
contrast/brightness, scanlines, static noise, flickering, vignette, glass reflection,
burn-in, glowing line, warmup animation, degauss animation.

## After a KWin Update

After each `pacman -Syu` that updates KWin, the plugin should be rebuilt.
The autostart entry does this automatically at the next login.

Manual rebuild:

```bash
cd src
./build.sh --rebuild
```

Debug logging:

```bash
journalctl -f | grep retro-term
```

## License

GPL-2.0-or-later
