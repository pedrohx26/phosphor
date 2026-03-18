# retro-term — KWin 6 CRT Shader Effect

CRT-fosforsimulatie als KWin 6 compositor-plugin voor KDE Plasma 6.  
46 historische presets, pixel scaling, volledig configureerbaar via GUI.

---

## Snelstart

```bash
# Vereisten (Arch / Garuda)
sudo pacman -S cmake gcc extra-cmake-modules kwin qt6-base \
               kconfig kcoreaddons kcmutils ki18n kconfigwidgets

# Bouwen en installeren
chmod +x build.sh
./build.sh

# Effect activeren
# Systeeminstellingen → Werkruimte-effecten → "Retro Terminal (CRT)"
# Klik "Instellingen" voor presets, sliders en pixel scaling
```

Of zonder compilatie (Bash-only):

```bash
chmod +x retro-term
retro-term install && retro-term install-fonts && retro-term on
retro-term preset C64
```

---

## Projectstructuur

```
retro-term/
├── retro-term              CLI-script: installatie, fonts, presets, beheer
├── build.sh                Geautomatiseerd C++ compileer- en installatiescript
├── CMakeLists.txt          CMake build-definitie
├── metadata.json           KWin plugin-descriptor
├── retro-term-kcm.desktop.in   KCM service-bestand
├── README.md               Deze documentatie
└── src/
    ├── retro.frag              GLSL 1.40 fragment shader (363 regels)
    ├── retro_term_effect.h     KWin 6 C++ effect header
    ├── retro_term_effect.cpp   KWin 6 C++ effect implementatie
    ├── retro_term_kcm.h        KCM configuratiepaneel header
    └── retro_term_kcm.cpp      KCM configuratiepaneel implementatie
```

---

## Vereisten

| Pakket | Arch-naam | Waarvoor |
|---|---|---|
| CMake ≥ 3.20 | `cmake` | Bouwtool |
| GCC / C++20 | `gcc` | Compiler |
| Extra CMake Modules | `extra-cmake-modules` | KDE CMake helpers |
| KWin 6 (met headers) | `kwin` | Effect API |
| Qt6 Base + OpenGL | `qt6-base` | Qt framework |
| KF6 Config | `kconfig` | Configuratieopslag |
| KF6 CoreAddons | `kcoreaddons` | Plugin factory |
| KF6 KCMUtils | `kcmutils` | KCM framework |
| KF6 I18n | `ki18n` | Vertalingen |
| KF6 ConfigWidgets | `kconfigwidgets` | UI-widgets |

```bash
sudo pacman -S cmake gcc extra-cmake-modules kwin qt6-base \
               kconfig kcoreaddons kcmutils ki18n kconfigwidgets
```

---

## Build-opties

```bash
./build.sh                   # Release-build + installatie
./build.sh --check-deps      # Alleen vereisten controleren
./build.sh --rebuild         # Verwijder build/ en herbouw
./build.sh --debug           # Debug-build (meer logging)
./build.sh --uninstall       # Verwijder geïnstalleerde plugin
./build.sh --prefix=/pad     # Alternatief prefix (standaard: /usr)
```

---

## Installatiepaden

Na `./build.sh`:

```
/usr/lib/qt6/plugins/kwin/effects/plugins/kwin_effect_retro_term.so
/usr/lib/qt6/plugins/plasma/kcms/systemsettings_qwidgets/kcm_retro_term.so
/usr/share/kwin/effects/retro-term/retro.frag
/usr/share/kwin/effects/retro-term/metadata.json
/usr/share/kservices6/retro-term-kcm.desktop
```

---

## Shader-effecten (retro.frag)

De GLSL 1.40 shader verwerkt elk frame in volgorde:

1. **Barrel distortion** — bolronde CRT-buis simulatie
2. **Sync distortion** — 4 modi: stabiel / sinusdrift / rolling / ghosting
3. **Jitter** — willekeurige per-pixel horizontale verschuiving
4. **Pixel scaling** — downsampling naar originele resolutie (zie hieronder)
5. **Chromatische aberratie** — RGB-kanalen horizontaal verschoven
6. **Karakter-smearing** — horizontale tekenvervaging
7. **Bloom** — 13-tap Gaussiaanse gloedhalo
8. **Ghosting** — frame-echo bij syncMode=3
9. **Fosfor-persistentie** — nagloed
10. **Fosfor-tint** — P1 groen / P3 amber / P4 wit / P39 radar
11. **Kleurtemperatuur** — 3000–9300 Kelvin
12. **Saturatie** — kleurversterking
13. **Contrast / helderheid**
14. **Scanlines** — 4 modi: geen / horizontaal / pixelraster / subpixel RGB
15. **Statische ruis** — granulair beeldruis
16. **Flickering** — 50/60Hz helderheidsflikkering
17. **Vignette** — randverduistering
18. **Glasreflectie** — schermoppervlak spiegelreflectie
19. **Burn-in** — fosfor burn-in simulatie
20. **Glowing line** — horizontale lijngloed
21. **Warmup-animatie** — koude CRT-start
22. **Degauss-animatie** — magnetische demagnetisatie

---

## Pixel Scaling

Het pixel scaling-systeem simuleert de originele schermresolutie van een historisch
systeem door het vensterframe te downsampling naar die resolutie en terug op te schalen.

```
pixelScale = 0.0   →  modern scherm, geen schaling (standaard)
pixelScale = 1.0   →  pixel-exact origineel (blokpixels op ware grootte)
tussenwaarden      →  vloeiende menging van beide
```

**Drie sampling-modi:**

| Modus | Naam | Gebruik |
|---|---|---|
| 0 | Nearest-neighbour | Harde blokpixels, precies zoals originele hardware |
| 1 | Bilineair | Zachte interpolatie, goed voor tussenwaarden 0.3–0.7 |
| 2 | Sharp bilineair | Simuleert Gaussiaanse elektronenbundel CRT — aanbevolen |

Sharp bilineair (modus 2) geeft scherpe pixelgrenzen maar zonder aliasing,
wat het meest authentieke CRT-pixel-look geeft.

**Alle CRT-effecten schalen mee:** scanlines, bloom, smearing, nagloed en
glowing line gebruiken de effectieve resolutie zodat ze altijd uitlijnen
met de gescalede pixels.

**Font-grootte:** het CLI-script berekent automatisch de optimale font-grootte
via `calc_font_size resX resY` zodat de terminal precies de originele resolutie vult.

```bash
# Voorbeeld: pixel-exact Commodore 64
retro-term preset C64
retro-term set pixelScale 1.0
retro-term set sampleMode 2

# Voorbeeld: subtiele retro sfeer
retro-term preset DEC_VT100
retro-term set pixelScale 0.3
retro-term set sampleMode 1
```

---

## KCM — Configuratiepaneel

De KCM (KDE Configuration Module) verschijnt als "Instellingen"-knop naast
het effect in Systeeminstellingen → Werkruimte-effecten.

**Secties:**
- **Doelvensters** — Uit / Alleen terminals / Alle vensters / Aangepast
- **Historisch preset** — 45 presets met tijdperkomschrijving
- **Fosfory en kleur** — type, vergeeling, kleurtemperatuur, nagloed
- **Schermgeometrie** — curvature, vignette, glasreflectie
- **Scanlines** — modus, intensiteit, scherpte
- **Bloom en gloed** — bloom, lijngloed, helderheid, contrast
- **Ruis en synchronisatie** — ruis, jitter, sync-modus, flickering, ghosting
- **Kleur en optiek** — kleurretentie, saturatie, aberratie, smearing, burn-in
- **Animaties** — warmup en degauss aan/uit en duur
- **Pixel scaling** — schaalfactor, sampling-modus, originele resolutie

De knop "✓ Toepassen & KWin herladen" slaat op en activeert direct.

---

## 46 Presets

Alle presets zijn voorzien van:
- Fosfortype, kleurtemperatuur en curvature op basis van gedocumenteerde hardware-specificaties
- Originele schermresolutie voor pixel scaling (waar historisch gedocumenteerd)
- Authentiek font (gratis te downloaden via `retro-term install-fonts`)

Categorieën: 1960s mainframe, 1970s terminals, home computers 1977–1983,
IBM PC 1981–1990, professionele werkstations, Amiga/Mac/NeXT, SVGA-tijdperk,
en film/sci-fi.

Zie `retro-term presets` voor de volledige lijst.

---

## Fonts installeren

```bash
retro-term install-fonts          # Alles (17 font-groepen)
retro-term install-fonts --status # Toon welke aanwezig zijn
```

Bronnen (alle gratis): int10h Oldschool PC Font Pack (CC BY-SA 4.0),
Kreative Korporation retro fonts (gratis), Google Fonts (SIL OFL),
en diverse andere vrije font-projecten.

---

## Na een KWin-update

Na elke `pacman -Syu` die KWin bijwerkt:

```bash
./build.sh --rebuild
```

Het build-script installeert automatisch een autostart-entry die dit bij
de volgende login uitvoert als de KWin-versie gewijzigd is.

Debug-logging:
```bash
journalctl -f | grep retro-term
```

---

## Hoe de presetwaarden tot stand komen

De shader-parameters zijn bepaald op basis van drie lagen:

1. **Hard technisch feit** — fosfortype uit fabrikantsdatasheets, kleurtemperatuur
   per categorie (professioneel ~8500K, consumentenTV ~6500K), curvature afgeleid
   van schermdiameter (9"≈0.40, 12"≈0.22, Trinitron≈0.04)

2. **Visueel redeneren** — scanline-intensiteit, bloom en vignette afgesteld op
   basis van gepubliceerde foto's en video's van werkende originele hardware

3. **Artistieke keuze** — noise, jitter en flickering representeren een
   "goed onderhouden" exemplaar van de machine, niet gemeten waarden

Exacte metingen aan werkende hardware (colorimeter, oscilloscoop, macrofotografie)
zouden nauwkeurigere waarden geven, maar zijn niet op schaal beschikbaar.

---

## Licentie

GPL-2.0-or-later.  
Fonts vallen elk onder hun eigen licentie — zie `retro-term readme` voor details.
