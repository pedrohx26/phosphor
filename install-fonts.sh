#!/usr/bin/env bash
# ╔══════════════════════════════════════════════════════════════════════════════╗
# ║  Phosphor — Font installer for all CRT presets                             ║
# ║  Installs the 30 vendored font groups used by the 43 historical presets    ║
# ╚══════════════════════════════════════════════════════════════════════════════╝
#
# Usage:  ./install-fonts.sh             Install all fonts
#         ./install-fonts.sh --status    Show which fonts are already installed
#
# All fonts are vendored under fonts/ next to this script (see fonts/README.md
# for sources and licenses) — installing is a local copy, no network access
# needed except for Terminus, which stays a distro package (pacman/pkexec).
#
# License: GPL-2.0-or-later
set -euo pipefail

# ── Colors ─────────────────────────────────────────────────────────────────────
R='\033[0;31m' G='\033[0;32m' Y='\033[1;33m' C='\033[0;36m'
BOLD='\033[1m' DIM='\033[2m' NC='\033[0m'
TICK="${G}✓${NC}" CROSS="${R}✗${NC}"

# ── Paths ──────────────────────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FONTS_SRC="$SCRIPT_DIR/fonts"
FONT_DIR="$HOME/.local/share/fonts/retro-terminal"

# ── Helpers ────────────────────────────────────────────────────────────────────
msg()  { echo -e "$*"; }
info() { echo -e "${C}[info]${NC} $*"; }
ok()   { echo -e "${G}[ ok ]${NC} $*"; }
warn() { echo -e "${Y}[warn]${NC} $*"; }
err()  { echo -e "${R}[err ]${NC} $*" >&2; }
die()  { err "$*"; exit 1; }

has()  { command -v "$1" &>/dev/null; }
# "fc-list | grep -q" is a classic pipefail trap: -q makes grep exit the
# instant it finds a match, and with hundreds of fonts installed grep very
# often wins that race and exits before fc-list has finished writing —
# fc-list then dies of SIGPIPE, "set -o pipefail" turns that into exit 141,
# and font_present() reports "missing" for a font that is, in fact,
# installed. Grepping a cached copy of fc-list's output instead of a live
# pipe removes the process fc-list could be SIGPIPE'd from.
_FC_LIST_CACHE=""
font_present() {
    [[ -z "$_FC_LIST_CACHE" ]] && _FC_LIST_CACHE="$(fc-list 2>/dev/null)"
    grep -qi "$1" <<< "$_FC_LIST_CACHE"
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

    # Keys are the exact embedded font-family name (verified with
    # `fc-scan --format "%{family}\n"` against the vendored files) — matching
    # on the full name instead of a loose first-word prefix means two
    # differently-named presets that happen to share a family prefix (every
    # "Px437 ..." font, for instance) can't shadow each other's status.
    declare -A FMAP
    FMAP["VT323"]="Default, DEC VT100/GT40, ZX Spectrum, MSX"
    FMAP["Glass TTY VT220"]="IBM 2260"
    FMAP["Px437 IBM 3270pc"]="IBM 3270"
    FMAP["Px437 Wyse700b"]="Wyse WY-50, TeleVideo TVI-925"
    FMAP["Share Tech Mono"]="Militair Radar"
    FMAP["C64 Pro Mono"]="Commodore 64, VIC-20"
    FMAP["Bedstead"]="BBC Micro, Teletext/Ceefax"
    FMAP["Atari Classic"]="Atari 400/800"
    FMAP["PxPlus IBM MDA"]="IBM PC MDA"
    FMAP["PxPlus IBM CGA"]="IBM PC CGA"
    FMAP["PxPlus IBM EGA 8x14"]="IBM PC EGA"
    FMAP["Px437 Tandy1K-I 200L"]="Tandy 1000"
    FMAP["PxPlus IBM VGA 9x16"]="IBM PS/2 VGA"
    FMAP["Topaz a500a1000a2000"]="Amiga 500"
    FMAP["Topaz a600a1200a400"]="Amiga WorkBench 2"
    FMAP["Print Char 21"]="Apple II, Apple IIgs"
    FMAP["Terminus"]="SVGA, Trinitron, Minimaal"
    FMAP["Pet Me"]="Commodore PET 2001, VIC-20"
    FMAP["Another Mans Treasure MIA Raw"]="TRS-80 Model I"
    FMAP["Hot CoCo"]="TRS-80 Color Computer"
    FMAP["Px437 Kaypro2K G"]="Kaypro II"
    FMAP["Px437 Compaq Port3"]="Compaq Portable"
    FMAP["PxPlus Rainbow100 re.40"]="DEC Rainbow 100"
    FMAP["LisaTerminal Paper Raw"]="Apple Lisa"
    FMAP["PxPlus Amstrad PC"]="Amstrad PC1512"
    FMAP["Project Jason Small"]="Atari ST SM124"
    FMAP["Px437 NEC APC3 8x16"]="NEC APC III"
    FMAP["PxPlus HP 150 re."]="HP 150 Touchscreen"
    FMAP["Mizuno"]="Sharp MZ-700"
    FMAP["Antiquarius"]="Mattel Aquarius"

    # Two bugs in one: "done | sort" makes the loop run in a subshell, so ok_n/
    # miss_n updates never reach the outer scope and the summary below always
    # printed "0 0" — and under set -e, "((ok_n++))" from a starting value of 0
    # returns exit status 1 (arithmetic result 0 = "false"), which used to kill
    # the whole script after the very first increment. Building the sorted text
    # first and counting in the same (non-subshell) loop that emits it fixes both.
    local lines=""
    for font in "${!FMAP[@]}"; do
        local presets="${FMAP[$font]}"
        if font_present "$font"; then
            lines+=$(printf "  ${G}✓${NC} %-28s %-40s ${G}installed${NC}\n" "$font" "$presets")$'\n'
            ok_n=$((ok_n + 1))
        else
            lines+=$(printf "  ${R}✗${NC} %-28s %-40s ${R}missing${NC}\n" "$font" "$presets")$'\n'
            miss_n=$((miss_n + 1))
        fi
    done
    printf '%s' "$lines" | sort
    echo ""
    msg "  ${G}✓ Installed: $ok_n${NC}   ${R}✗ Missing: $miss_n${NC}"
    msg "  ${DIM}(DejaVu Sans Mono and Terminus cover the remaining 4 presets and aren't vendored — see README.md)${NC}"
    echo ""
}

# ══════════════════════════════════════════════════════════════════════════════
# INSTALL — copy vendored fonts, install Terminus from the distro
# ══════════════════════════════════════════════════════════════════════════════
cmd_install() {
    echo ""
    echo -e "${BOLD}${C}Installing fonts for all Phosphor presets${NC}"
    echo ""

    has fc-cache || die "Need fc-cache — install via: sudo pacman -S fontconfig"
    [[ -d "$FONTS_SRC" ]] || die "fonts/ directory not found next to this script ($FONTS_SRC)"

    mkdir -p "$FONT_DIR"

    # ── 1. Terminus — distro package, not a vendored file ───────────────────
    if ! font_present "Terminus"; then
        info "Terminus (SVGA, Trinitron, Minimaal)"
        if has pacman; then
            pacman -Qi terminus-font &>/dev/null || {
                info "Installing via pacman (may need password)..."
                # pkexec needs a polkit agent, which only exists in a graphical
                # session; a headless/SSH shell has none, so try passwordless
                # sudo first — several setups (like the one this was tested on)
                # allow "pacman -S --noconfirm *" without a password — and only
                # fall back to pkexec where that isn't the case.
                sudo -n pacman -S --needed --noconfirm terminus-font 2>/dev/null || \
                pkexec pacman -S --needed --noconfirm terminus-font 2>/dev/null || true
            }
        fi
        has yay && yay -S --needed --noconfirm ttf-terminus-nerd 2>/dev/null || true
        font_present "Terminus" && ok "Terminus installed" || warn "Terminus not found — install manually: pacman -S terminus-font"
    else msg "  $TICK Terminus (already installed)"; fi

    # ── 2. Everything else — copy from the vendored fonts/ tree ─────────────
    info "Copying vendored fonts from $FONTS_SRC"
    local n_copied=0
    while IFS= read -r -d '' f; do
        cp -f "$f" "$FONT_DIR/"
        n_copied=$((n_copied + 1))
    done < <(find "$FONTS_SRC" \( -iname "*.ttf" -o -iname "*.otf" \) -print0)
    ok "$n_copied vendored font files copied to $FONT_DIR"

    # DejaVu Sans Mono (NeXT Station, Sun-3 Workstation) ships with virtually
    # every desktop Linux install already; nothing to do here if it's missing —
    # it's not ours to vendor or fetch.
    font_present "DejaVu Sans Mono" || \
        warn "DejaVu Sans Mono not found — install via your distro's font packages (e.g. pacman -S ttf-dejavu)"

    # ── Finish ───────────────────────────────────────────────────────────────
    echo ""
    info "Updating font cache..."
    fc-cache -fv "$FONT_DIR" 2>/dev/null | tail -2 || fc-cache -f
    local n
    n=$(find "$FONT_DIR" \( -name "*.ttf" -o -name "*.otf" \) 2>/dev/null | wc -l)
    ok "$n font files installed in $FONT_DIR"
    echo ""
    # font_present()'s cache is fine for a single query, or for --status run on
    # its own, but a whole install run adds dozens of fonts to the filesystem
    # between the first font_present() call (Terminus, near the very top) and
    # this final summary — reusing that first snapshot here made freshly
    # installed fonts read back as "missing" in the very report meant to
    # confirm they installed. Dropping the cache forces one honest re-read of
    # fc-list right before the summary that actually needs it to be current.
    _FC_LIST_CACHE=""
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
