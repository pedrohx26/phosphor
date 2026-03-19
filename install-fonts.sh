#!/usr/bin/env bash
# ╔══════════════════════════════════════════════════════════════════════════════╗
# ║  Phosphor — Font installer for all CRT presets                             ║
# ║  Installs 17 font groups used by the 43 historical presets                 ║
# ╚══════════════════════════════════════════════════════════════════════════════╝
#
# Usage:  ./install-fonts.sh             Install all fonts
#         ./install-fonts.sh --status    Show which fonts are already installed
#
# License: GPL-2.0-or-later
set -euo pipefail

# ── Colors ─────────────────────────────────────────────────────────────────────
R='\033[0;31m' G='\033[0;32m' Y='\033[1;33m' C='\033[0;36m'
BOLD='\033[1m' DIM='\033[2m' NC='\033[0m'
TICK="${G}✓${NC}" CROSS="${R}✗${NC}"

# ── Paths ──────────────────────────────────────────────────────────────────────
FONT_DIR="$HOME/.local/share/fonts/retro-terminal"

# ── Helpers ────────────────────────────────────────────────────────────────────
msg()  { echo -e "$*"; }
info() { echo -e "${C}[info]${NC} $*"; }
ok()   { echo -e "${G}[ ok ]${NC} $*"; }
warn() { echo -e "${Y}[warn]${NC} $*"; }
err()  { echo -e "${R}[err ]${NC} $*" >&2; }
die()  { err "$*"; exit 1; }

has()  { command -v "$1" &>/dev/null; }
font_present() { fc-list 2>/dev/null | grep -qi "${1%%,*}"; }

download() {
    local url="$1" dest="$2" desc="${3:-file}"
    info "Downloading: $desc"
    if has curl; then
        curl -fsSL --progress-bar "$url" -o "$dest" && return 0
    elif has wget; then
        wget -q --show-progress "$url" -O "$dest" && return 0
    fi
    warn "Download failed: $url"
    return 1
}

# ══════════════════════════════════════════════════════════════════════════════
# STATUS — show which fonts are present
# ══════════════════════════════════════════════════════════════════════════════
cmd_status() {
    echo ""
    echo -e "${BOLD}${C}Font status for Phosphor presets${NC}"
    echo ""
    printf "  %-30s %-40s %s\n" "FONT" "PRESETS" "STATUS"
    printf "  %-30s %-40s %s\n" "──────────────────────────────" "────────────────────────────────────────" "──────────"
    local ok_n=0 miss_n=0

    declare -A FMAP
    FMAP["PxPlus IBM VGA"]="IBM VGA, EGA, CGA, MDA, 3270, Tandy, Wyse"
    FMAP["VT323"]="DEC VT100, GT40, ZX Spectrum, MSX"
    FMAP["Topaz Unicode"]="Amiga 500, Amiga WorkBench 2"
    FMAP["C64 Pro Mono"]="Commodore 64, VIC-20"
    FMAP["Share Tech Mono"]="Radar"
    FMAP["Terminus"]="SVGA, Trinitron"
    FMAP["Silkscreen"]="Apple Macintosh 128K"
    FMAP["Bedstead"]="Teletext, BBC Micro"
    FMAP["Glass TTY VT220"]="IBM 2260"
    FMAP["Print Char 21"]="Apple II"
    FMAP["Atari Classic"]="Atari 400/800"
    FMAP["Lucida Console"]="NeXT Station, Sun-3"
    FMAP["Pet Me"]="Commodore PET 2001"
    FMAP["Another Man's Treasure"]="TRS-80 Model I"
    FMAP["Hot CoCo"]="TRS-80 Color Computer"
    FMAP["LisaTerminal"]="Apple Lisa"
    FMAP["Shaston"]="Apple IIgs"
    FMAP["Mizuno"]="Sharp MZ-700"
    FMAP["Antiquarius"]="Mattel Aquarius"
    FMAP["Project Jason"]="Atari ST SM124"
    FMAP["PxPlus Kaypro"]="Kaypro II"
    FMAP["PxPlus CompaqPort"]="Compaq Portable"
    FMAP["PxPlus DEC Rainbow"]="DEC Rainbow 100"
    FMAP["PxPlus TeleVideo"]="TeleVideo TVI-925"
    FMAP["PxPlus Amstrad"]="Amstrad PC1512"
    FMAP["PxPlus NEC APC3"]="NEC APC III"
    FMAP["PxPlus HP 150"]="HP 150 Touchscreen"

    for font in "${!FMAP[@]}"; do
        local presets="${FMAP[$font]}"
        if font_present "${font%% *}"; then
            printf "  ${G}✓${NC} %-28s %-40s ${G}installed${NC}\n" "$font" "$presets"
            ((ok_n++))
        else
            printf "  ${R}✗${NC} %-28s %-40s ${R}missing${NC}\n" "$font" "$presets"
            ((miss_n++))
        fi
    done | sort
    echo ""
    msg "  ${G}✓ Installed: $ok_n${NC}   ${R}✗ Missing: $miss_n${NC}"
    echo ""
}

# ══════════════════════════════════════════════════════════════════════════════
# INSTALL — download and install all font groups
# ══════════════════════════════════════════════════════════════════════════════
cmd_install() {
    echo ""
    echo -e "${BOLD}${C}Installing fonts for all Phosphor presets${NC}"
    echo ""

    # Check requirements
    has curl || has wget || die "Need curl or wget — install via: sudo pacman -S curl"
    has unzip || die "Need unzip — install via: sudo pacman -S unzip"
    has fc-cache || die "Need fc-cache — install via: sudo pacman -S fontconfig"

    mkdir -p "$FONT_DIR"
    local TMP
    TMP=$(mktemp -d /tmp/phosphor-fonts.XXXXXX)
    trap 'rm -rf "$TMP"' EXIT

    # ── 1. Terminus ──────────────────────────────────────────────────────────
    if ! font_present "Terminus"; then
        info "1/17 — Terminus (SVGA, Trinitron)"
        if has pacman; then
            pacman -Qi terminus-font &>/dev/null || {
                info "Installing via pacman (may need password)..."
                pkexec pacman -S --needed --noconfirm terminus-font 2>/dev/null || true
            }
        fi
        has yay && yay -S --needed --noconfirm ttf-terminus-nerd 2>/dev/null || true
        font_present "Terminus" && ok "Terminus installed" || warn "Terminus not found — install manually: pacman -S terminus-font"
    else msg "  $TICK Terminus (already installed)"; fi

    # ── 2. int10h Oldschool PC Font Pack ─────────────────────────────────────
    if ! font_present "PxPlus"; then
        info "2/17 — int10h Oldschool PC Font Pack (IBM, Tandy, Wyse, Kaypro, Compaq, DEC, etc.)"
        if has yay; then
            yay -S --needed --noconfirm ttf-oldschool-pc-font-pack 2>/dev/null && ok "int10h via AUR" || true
        fi
        if ! font_present "PxPlus"; then
            download "https://int10h.org/oldschool-pc-fonts/files/oldschool_pc_font_pack_v2.2.zip" \
                     "$TMP/int10h.zip" "int10h Pack v2.2" && \
            unzip -q "$TMP/int10h.zip" -d "$TMP/int10h/" 2>/dev/null && \
            find "$TMP/int10h/" -name "*.ttf" -exec cp {} "$FONT_DIR/" \; && \
            ok "int10h Pack installed" || \
            warn "int10h download failed — manual: https://int10h.org/oldschool-pc-fonts/"
        fi
    else msg "  $TICK int10h Oldschool PC Font Pack (already installed)"; fi

    # ── 3. Amiga Topaz Unicode ───────────────────────────────────────────────
    if ! font_present "Topaz"; then
        info "3/17 — Amiga Topaz Unicode (Amiga 500, WorkBench)"
        if has git; then
            git clone --depth=1 -q https://github.com/rewtnull/amigafonts.git "$TMP/amiga" 2>/dev/null || \
            download "https://github.com/rewtnull/amigafonts/archive/refs/heads/master.zip" \
                     "$TMP/amiga.zip" "Amiga fonts" && \
            unzip -q "$TMP/amiga.zip" -d "$TMP/" 2>/dev/null || true
            local adir
            adir=$(find "$TMP" -maxdepth 2 -type d -name "amigafonts*" | head -1)
            [[ -n "$adir" ]] && find "$adir" \( -name "*.ttf" -o -name "*.otf" \) -exec cp {} "$FONT_DIR/" \;
        fi
        font_present "Topaz" && ok "Topaz installed" || warn "Topaz failed — https://github.com/rewtnull/amigafonts"
    else msg "  $TICK Topaz Unicode (already installed)"; fi

    # ── 4. C64 Pro Mono ──────────────────────────────────────────────────────
    if ! font_present "C64"; then
        info "4/17 — C64 Pro Mono (Commodore 64, VIC-20)"
        has yay && yay -S --needed --noconfirm ttf-c64-truetype 2>/dev/null || true
        if ! font_present "C64"; then
            download "https://style64.org/file/C64_TrueType_v1.2.1-(STYLE).zip" \
                     "$TMP/c64.zip" "C64 Pro Mono" && \
            unzip -q "$TMP/c64.zip" -d "$TMP/c64/" && \
            find "$TMP/c64/" -name "*.ttf" -exec cp {} "$FONT_DIR/" \; || \
            download "https://github.com/statico/dotfiles/raw/master/.fonts/C64_Pro_Mono.ttf" \
                     "$FONT_DIR/C64_Pro_Mono.ttf" "C64 Pro Mono (single file)" || \
            warn "C64 Pro Mono failed — https://style64.org/c64-truetype"
        fi
    else msg "  $TICK C64 Pro Mono (already installed)"; fi

    # ── 5. VT323 ─────────────────────────────────────────────────────────────
    if ! font_present "VT323"; then
        info "5/17 — VT323 (DEC VT100, GT40, ZX Spectrum, MSX)"
        download "https://github.com/googlefonts/vt323/raw/main/fonts/VT323-Regular.ttf" \
                 "$FONT_DIR/VT323-Regular.ttf" "VT323" && ok "VT323 installed" || \
        warn "VT323 failed — https://fonts.google.com/specimen/VT323"
    else msg "  $TICK VT323 (already installed)"; fi

    # ── 6. Share Tech Mono ───────────────────────────────────────────────────
    if ! font_present "ShareTechMono" && ! font_present "Share Tech"; then
        info "6/17 — Share Tech Mono (Radar)"
        download "https://github.com/googlefonts/sharetechmono/raw/main/fonts/ShareTechMono-Regular.ttf" \
                 "$FONT_DIR/ShareTechMono-Regular.ttf" "Share Tech Mono" && ok "Share Tech Mono installed" || \
        warn "Share Tech Mono failed — https://fonts.google.com/specimen/Share+Tech+Mono"
    else msg "  $TICK Share Tech Mono (already installed)"; fi

    # ── 7. Silkscreen ────────────────────────────────────────────────────────
    if ! font_present "Silkscreen"; then
        info "7/17 — Silkscreen (Apple Macintosh 128K)"
        download "https://github.com/googlefonts/Silkscreen/raw/main/fonts/ttf/Silkscreen-Regular.ttf" \
                 "$FONT_DIR/Silkscreen-Regular.ttf" "Silkscreen" && \
        download "https://github.com/googlefonts/Silkscreen/raw/main/fonts/ttf/Silkscreen-Bold.ttf" \
                 "$FONT_DIR/Silkscreen-Bold.ttf" "Silkscreen Bold" && ok "Silkscreen installed" || \
        warn "Silkscreen failed — https://kottke.org/plus/type/silkscreen/"
    else msg "  $TICK Silkscreen (already installed)"; fi

    # ── 8. Bedstead ──────────────────────────────────────────────────────────
    if ! font_present "Bedstead"; then
        info "8/17 — Bedstead (Teletext, BBC Micro)"
        download "https://bjh21.me.uk/bedstead/bedstead.otf" \
                 "$FONT_DIR/bedstead.otf" "Bedstead" && ok "Bedstead installed" || \
        warn "Bedstead failed — https://bjh21.me.uk/bedstead/"
    else msg "  $TICK Bedstead (already installed)"; fi

    # ── 9. Print Char 21 ────────────────────────────────────────────────────
    if ! font_present "Print Char"; then
        info "9/17 — Print Char 21 (Apple II)"
        download "https://www.kreativekorp.com/swdownload/fonts/Print_Char_21.zip" \
                 "$TMP/pc21.zip" "Print Char 21" && \
        unzip -q "$TMP/pc21.zip" -d "$TMP/pc21/" && \
        find "$TMP/pc21/" \( -name "*.ttf" -o -name "*.otf" \) -exec cp {} "$FONT_DIR/" \; && \
        ok "Print Char 21 installed" || \
        warn "Print Char 21 failed — https://www.kreativekorp.com/software/fonts/apple2/"
    else msg "  $TICK Print Char 21 (already installed)"; fi

    # ── 10. Atari Classic ────────────────────────────────────────────────────
    if ! font_present "Atari"; then
        info "10/17 — Atari Classic (Atari 400/800)"
        has yay && yay -S --needed --noconfirm ttf-atari-classic 2>/dev/null || true
        if ! font_present "Atari"; then
            download "https://github.com/AtariAge/AtariFont/raw/main/AtariClassic-Regular.ttf" \
                     "$FONT_DIR/AtariClassic-Regular.ttf" "Atari Classic" && ok "Atari Classic installed" || \
            warn "Atari Classic failed — https://www.dafont.com/atari-classic.font"
        fi
    else msg "  $TICK Atari Classic (already installed)"; fi

    # ── 11. Glass TTY VT220 ──────────────────────────────────────────────────
    if ! font_present "Glass"; then
        info "11/17 — Glass TTY VT220 (IBM 2260)"
        download "https://github.com/svofski/glasstty/raw/master/GlassTTYVT220.ttf" \
                 "$FONT_DIR/GlassTTYVT220.ttf" "Glass TTY VT220" && ok "Glass TTY installed" || \
        warn "Glass TTY failed — https://github.com/svofski/glasstty"
    else msg "  $TICK Glass TTY VT220 (already installed)"; fi

    # ── 12. Unscii ───────────────────────────────────────────────────────────
    if ! font_present "unscii" && ! font_present "Unscii"; then
        info "12/17 — Unscii (BBC Micro / Mode 7 style)"
        download "https://github.com/viznut/unscii/raw/master/unscii-16-full.ttf" \
                 "$FONT_DIR/unscii-16-full.ttf" "Unscii-16" && ok "Unscii installed" || \
        warn "Unscii failed — http://viznut.fi/unscii/"
    else msg "  $TICK Unscii (already installed)"; fi

    # ── 13–17. Kreativekorp Retro Fonts ──────────────────────────────────────
    if ! font_present "Pet Me" && ! font_present "PetMe"; then
        info "13/17 — Pet Me (Commodore PET 2001)"
        download "https://www.kreativekorp.com/swdownload/fonts/retro/petme.zip" \
                 "$TMP/petme.zip" "Pet Me" && \
        unzip -q "$TMP/petme.zip" -d "$TMP/petme/" 2>/dev/null && \
        find "$TMP/petme/" \( -name "*.ttf" -o -name "*.otf" \) -exec cp {} "$FONT_DIR/" \; && \
        ok "Pet Me installed" || \
        warn "Pet Me failed — https://www.kreativekorp.com/software/fonts/retro/"
    else msg "  $TICK Pet Me (already installed)"; fi

    if ! font_present "Another Man" && ! font_present "AnotherMan"; then
        info "14/17 — Another Man's Treasure (TRS-80 Model I)"
        download "https://www.kreativekorp.com/swdownload/fonts/retro/amtreasure.zip" \
                 "$TMP/amtreasure.zip" "Another Man's Treasure" && \
        unzip -q "$TMP/amtreasure.zip" -d "$TMP/amtreasure/" 2>/dev/null && \
        find "$TMP/amtreasure/" \( -name "*.ttf" -o -name "*.otf" \) -exec cp {} "$FONT_DIR/" \; && \
        ok "Another Man's Treasure installed" || \
        warn "Another Man's Treasure failed — https://www.kreativekorp.com/software/fonts/retro/"
    else msg "  $TICK Another Man's Treasure (already installed)"; fi

    # Remaining Kreativekorp fonts: Hot CoCo, LisaTerminal, Shaston, Mizuno, Antiquarius, Project Jason
    for kreative_pkg in \
        "Hot CoCo:hotcoco:TRS-80 Color Computer" \
        "LisaTerminal:lisa1:Apple Lisa" \
        "Shaston:shaston:Apple IIgs" \
        "Mizuno:mizuno:Sharp MZ-700" \
        "Antiquarius:aq2:Mattel Aquarius" \
        "Project Jason:projason:Atari ST SM124"
    do
        IFS=':' read -r font_name pkg_name desc <<< "$kreative_pkg"
        if ! font_present "$font_name"; then
            info "15/17 — $font_name ($desc)"
            download "https://www.kreativekorp.com/swdownload/fonts/retro/${pkg_name}.zip" \
                     "$TMP/${pkg_name}.zip" "$font_name" && \
            unzip -q "$TMP/${pkg_name}.zip" -d "$TMP/${pkg_name}/" 2>/dev/null && \
            find "$TMP/${pkg_name}/" \( -name "*.ttf" -o -name "*.otf" \) -exec cp {} "$FONT_DIR/" \; && \
            ok "$font_name ($desc) installed" || \
            warn "$font_name failed — https://www.kreativekorp.com/software/fonts/retro/"
        else msg "  $TICK $font_name (already installed)"; fi
    done

    # ── int10h extra check ───────────────────────────────────────────────────
    if ! font_present "Kaypro" && ! font_present "PxPlus Kaypro"; then
        warn "Kaypro/Compaq/DEC/TeleVideo/Amstrad/NEC/HP fonts not found"
        warn "These are part of the int10h Oldschool PC Font Pack (step 2)."
        warn "Manual download: https://int10h.org/oldschool-pc-fonts/download/"
        warn "Copy desired TTF files to: $FONT_DIR"
    else msg "  $TICK int10h extra fonts (Kaypro/Compaq/DEC/TeleVideo/Amstrad/NEC/HP)"; fi

    # ── Finish ───────────────────────────────────────────────────────────────
    echo ""
    info "Updating font cache..."
    fc-cache -fv "$FONT_DIR" 2>/dev/null | tail -2 || fc-cache -f
    local n
    n=$(find "$FONT_DIR" \( -name "*.ttf" -o -name "*.otf" \) 2>/dev/null | wc -l)
    ok "$n font files installed in $FONT_DIR"
    echo ""
    cmd_status
}

# ══════════════════════════════════════════════════════════════════════════════
# Main
# ══════════════════════════════════════════════════════════════════════════════
case "${1:-}" in
    --status|-s) cmd_status ;;
    --help|-h)
        echo "Usage: ./install-fonts.sh [--status]"
        echo ""
        echo "  (no args)    Install all fonts for Phosphor presets"
        echo "  --status     Show which fonts are already installed"
        ;;
    *) cmd_install ;;
esac
