# Vendored preset fonts

Every non-system font referenced by a Phosphor preset is vendored here, in its
original unmodified form, grouped by license/source. `install-fonts.sh`
installs these files directly — no network access required. Two presets use
fonts that ship with any normal desktop and are **not** vendored:
`DejaVu Sans Mono` (nearly always preinstalled) and `Terminus` (installed via
the distro package manager, see the main README).

| Directory | License | Fonts |
|---|---|---|
| [`int10h/`](int10h/) | CC BY-SA 4.0 | int10h.org Oldschool PC Font Pack (IBM/Compaq/Kaypro/NEC/Tandy/Wyse/Amstrad/DEC/HP variants) |
| [`kreativekorp/`](kreativekorp/) | Kreative Software Free Use License v1.2f | Another Mans Treasure, Antiquarius, Hot CoCo, LisaTerminal, Mizuno, Pet Me, Print Char 21, Project Jason, Shaston |
| [`c64-pro-mono/`](c64-pro-mono/) | style64.org custom license (software-package clause) | C64 Pro Mono |
| [`atari-classic/`](atari-classic/) | Freeware | Atari Classic |
| [`amiga-topaz/`](amiga-topaz/) | GPL Font Exception | Topaz (Amiga) |
| [`google-fonts/`](google-fonts/) | SIL OFL 1.1 | VT323, Share Tech Mono, Silkscreen |
| [`misc/`](misc/) | Public domain / see upstream | Bedstead, Glass TTY VT220 |

Each directory carries its own `LICENSE.txt` (or `OFL-*.txt`) exactly as
required by that font's license — none of the files were renamed or modified
from what their respective authors distribute.

## Preset → font → file

| Preset | Font (embedded family name) | File |
|---|---|---|
| Default (amber) | VT323 | `google-fonts/VT323-Regular.ttf` |
| IBM 2260 (1964) | Glass TTY VT220 | `misc/Glass_TTY_VT220.ttf` |
| DEC GT40 (1972) | VT323 | `google-fonts/VT323-Regular.ttf` |
| DEC VT100 (1978) | VT323 | `google-fonts/VT323-Regular.ttf` |
| IBM 3270 (1971) | Px437 IBM 3270pc | `int10h/Px437_IBM_3270pc.ttf` |
| Wyse WY-50 (1979) | Px437 Wyse700b | `int10h/Px437_Wyse700b.ttf` |
| Militair Radar (1965) | Share Tech Mono | `google-fonts/ShareTechMono-Regular.ttf` |
| Apple II (1977) | Print Char 21 | `kreativekorp/PrintChar21.ttf` |
| Commodore 64 (1982) | C64 Pro Mono | `c64-pro-mono/C64_Pro_Mono-STYLE.ttf` |
| ZX Spectrum (1982) | VT323 | `google-fonts/VT323-Regular.ttf` |
| BBC Micro (1981) | Bedstead | `misc/bedstead.otf` |
| Atari 400/800 (1979) | Atari Classic | `atari-classic/AtariClassic-gry3.ttf` |
| IBM PC MDA (1981) | PxPlus IBM MDA | `int10h/PxPlus_IBM_MDA.ttf` |
| IBM PC CGA (1981) | PxPlus IBM CGA | `int10h/PxPlus_IBM_CGA.ttf` |
| IBM PC EGA (1984) | PxPlus IBM EGA 8x14 | `int10h/PxPlus_IBM_EGA_8x14.ttf` |
| Tandy 1000 (1984) | PxPlus Tandy1K-II 200L | `int10h/PxPlus_Tandy1K-II_200L.ttf` |
| IBM PS/2 VGA (1987) | PxPlus IBM VGA 9x16 | `int10h/PxPlus_IBM_VGA_9x16.ttf` |
| Amiga 500 (1987) | Topaz a500a1000a2000 | `amiga-topaz/Topaz_a500_v1.0.ttf` |
| Amiga WorkBench 2 (1990) | Topaz a500a1000a2000 | `amiga-topaz/Topaz_a500_v1.0.ttf` |
| Apple Macintosh 128K (1984) | Silkscreen | `google-fonts/Silkscreen-Regular.ttf` |
| NeXT Station (1990) | DejaVu Sans Mono | *(system font, not vendored)* |
| SVGA Multisync (1992) | Terminus | *(distro package, not vendored)* |
| Sony Trinitron (1997) | Terminus | *(distro package, not vendored)* |
| Teletext / Ceefax (1974) | Bedstead | `misc/bedstead.otf` |
| Minimaal (laag GPU) | Terminus | *(distro package, not vendored)* |
| Commodore PET 2001 (1977) | Pet Me 2Y | `kreativekorp/PetMe2Y.ttf` |
| TRS-80 Model I (1977) | Another Mans Treasure MIA Raw | `kreativekorp/AnotherMansTreasureMIARaw.ttf` |
| TRS-80 Color Computer (1980) | Hot CoCo | `kreativekorp/HotCoCo.ttf` |
| Kaypro II (1982) | Px437 Kaypro2K G | `int10h/Px437_Kaypro2K_G.ttf` |
| Compaq Portable (1982) | Px437 Compaq Port3 | `int10h/Px437_Compaq_Port3.ttf` |
| DEC Rainbow 100 (1982) | PxPlus Rainbow100 re.40 | `int10h/PxPlus_Rainbow100_re_40.ttf` |
| TeleVideo TVI-925 (1982) | Px437 Wyse700b | `int10h/Px437_Wyse700b.ttf` |
| Apple Lisa (1983) | LisaTerminal Paper Raw | `kreativekorp/LisaTerminalPaperRaw.ttf` |
| Amstrad PC1512 (1986) | PxPlus Amstrad PC-2y | `int10h/PxPlus_Amstrad_PC-2y.ttf` |
| Atari ST SM124 (1985) | Project Jason Small | `kreativekorp/ProjectJasonSmall.ttf` |
| NEC APC III (1983) | Px437 NEC APC3 8x16 | `int10h/Px437_NEC_APC3_8x16.ttf` |
| HP 150 Touchscreen (1983) | PxPlus HP 150 re. | `int10h/PxPlus_HP_150_re.ttf` |
| Apple IIgs (1986) | Shaston 320 | `kreativekorp/Shaston320.ttf` |
| Sharp MZ-700 (1982) | Mizuno | `kreativekorp/Mizuno.ttf` |
| Mattel Aquarius (1983) | Antiquarius | `kreativekorp/Antiquarius.ttf` |
| Commodore VIC-20 (1981) | C64 Pro Mono | `c64-pro-mono/C64_Pro_Mono-STYLE.ttf` |
| MSX (1983) | VT323 | `google-fonts/VT323-Regular.ttf` |
| Sun-3 Workstation (1985) | DejaVu Sans Mono | *(system font, not vendored)* |

## Notes on the int10h and TeleVideo/Kaypro/Compaq/NEC/Tandy/HP/DEC entries

The int10h Oldschool PC Font Pack ships each font family in several CP437/ANSI
variants (`Ac437`, `AcPlus`, `Mx437`, `MxPlus`, `Px437`, `PxPlus`, ...). Not
every family has a `PxPlus` (CP437 + box-drawing + ANSI) release — several of
the presets above were originally written against font-family names that
don't exist in the pack (`PxPlus CompaqPort`, `PxPlus Kaypro 2000`,
`PxPlus TeleVideo TVI-925`, `PxPlus NEC APC3 8x16`, `PxPlus Tandy 1000`,
`PxPlus Wyse WY700b 2x`, `PxPlus IBM 3270 Semi-Graphics`,
`PxPlus DEC Rainbow100-8x10`). Those preset strings have been corrected to
point at the closest actually-shipped variant (usually the plain `Px437`
release of the same machine). `PxPlus TeleVideo TVI-925` doesn't exist under
any name in the pack at all — the TeleVideo TVI-925 preset now reuses
`Px437 Wyse700b` (same era, same green P1 CRT terminal class).
