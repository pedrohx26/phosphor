#!/usr/bin/env bash
# ╔══════════════════════════════════════════════════════════════════════════════╗
# ║  phosphor — KWin Effect Build Script                                     ║
# ║  Compiles and installs automatically, no IDE needed                         ║
# ╚══════════════════════════════════════════════════════════════════════════════╝
# Usage: ./build.sh [options]
#   --check-deps   Check requirements without building
#   --rebuild      Remove build/ and rebuild from scratch
#   --debug        Debug build (more logging in journalctl)
#   --uninstall    Remove the installed plugin
#   --prefix=PATH  Installation prefix (default: /usr)
#   --help         Show this help screen
set -euo pipefail

# ── TTY guard — restore terminal state on exit/interrupt ──────────────────────
_TTY_SAVED=$(stty -g 2>/dev/null || true)
_tty_restore() {
    [[ -n "$_TTY_SAVED" ]] && stty "$_TTY_SAVED" 2>/dev/null || stty sane 2>/dev/null || true
    printf '\033[?25h'   # ensure cursor visible
}
trap '_tty_restore' EXIT INT TERM

# ── Colors ────────────────────────────────────────────────────────────────────
R='\033[0;31m' G='\033[0;32m' Y='\033[1;33m' C='\033[0;36m'
BOLD='\033[1m' DIM='\033[2m' NC='\033[0m'
ok()    { echo -e "${G}[ ok ]${NC}  $*"; }
info()  { echo -e "${C}[info]${NC}  $*"; }
warn()  { echo -e "${Y}[warn]${NC}  $*"; }
err()   { echo -e "${R}[err ]${NC}  $*" >&2; }
die()   { err "$*"; exit 1; }
sep()   { echo -e "${DIM}──────────────────────────────────────────────────────${NC}"; }
banner(){ echo -e "\n${BOLD}${C}▶ $*${NC}"; }

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
BUILD_TYPE="Release"
PREFIX="/usr"
DO_REBUILD=0
DO_CHECK=0
DO_UNINSTALL=0

for arg in "$@"; do
    case "$arg" in
        --check-deps)  DO_CHECK=1 ;;
        --rebuild)     DO_REBUILD=1 ;;
        --debug)       BUILD_TYPE="Debug" ;;
        --uninstall)   DO_UNINSTALL=1 ;;
        --prefix=*)    PREFIX="${arg#*=}" ;;
        --help|-h)
            sed -n '4,14p' "$0" | sed 's/^# \{0,2\}//'
            exit 0 ;;
    esac
done

# ══════════════════════════════════════════════════════════════════════════════
# UNINSTALL
# ══════════════════════════════════════════════════════════════════════════════
if [[ $DO_UNINSTALL -eq 1 ]]; then
    banner "Uninstalling"
    # Look for the .so in common locations
    for d in \
        /usr/lib/qt6/plugins/kwin/effects/plugins \
        /usr/lib/kwin/effects/plugins \
        "$PREFIX/lib/qt6/plugins/kwin/effects/plugins"
    do
        f="$d/kwin_effect_retro_term.so"
        if [[ -f "$f" ]]; then
            sudo rm -f "$f" && ok "Removed: $f" || warn "Could not remove: $f"
        fi
    done
    sudo rm -rf /usr/share/kwin/effects/phosphor \
                "$PREFIX/share/kwin/effects/phosphor" 2>/dev/null || true
    ok "Data directory removed"
    # Reloading KWin
    qdbus6 org.kde.KWin /KWin reconfigure 2>/dev/null || \
    qdbus  org.kde.KWin /KWin reconfigure 2>/dev/null || \
    warn "Log out and back in to reload KWin"
    exit 0
fi

# ══════════════════════════════════════════════════════════════════════════════
# DEPENDENCY CHECK
# ══════════════════════════════════════════════════════════════════════════════
check_deps() {
    banner "Checking requirements"
    local n_fail=0

    chk_cmd() {
        if command -v "$1" &>/dev/null; then
            echo -e "  ${G}✓${NC} $1  ${DIM}($(command -v "$1"))${NC}"
        else
            echo -e "  ${R}✗${NC} $1  ${DIM}→ $2${NC}"
            (( n_fail++ ))
        fi
    }

    chk_file() {
        local result
        result=$(find $3 -name "$1" 2>/dev/null) || true
        if [[ -n "$result" ]]; then
            echo -e "  ${G}✓${NC} $1  ${DIM}(${3})${NC}"
        else
            echo -e "  ${R}✗${NC} $1  ${DIM}→ $2${NC}"
            (( n_fail++ ))
        fi
    }

    chk_pkg() {
        # pkg-config check
        if pkg-config --exists "$1" 2>/dev/null; then
            echo -e "  ${G}✓${NC} $1 $(pkg-config --modversion "$1" 2>/dev/null)"
        else
            echo -e "  ${R}✗${NC} $1  ${DIM}→ $2${NC}"
            (( n_fail++ ))
        fi
    }

    echo ""
    echo -e "  ${BOLD}Build tools:${NC}"
    chk_cmd cmake          "sudo pacman -S cmake"
    chk_cmd make           "sudo pacman -S make"
    chk_cmd g++            "sudo pacman -S gcc"
    chk_cmd pkg-config     "sudo pacman -S pkgconf"

    echo ""
    echo -e "  ${BOLD}Qt6:${NC}"
    chk_pkg "Qt6Core"      "sudo pacman -S qt6-base"
    chk_pkg "Qt6Gui"       "sudo pacman -S qt6-base"
    chk_pkg "Qt6Widgets"   "sudo pacman -S qt6-base"
    chk_pkg "Qt6DBus"      "sudo pacman -S qt6-base"
    chk_pkg "Qt6OpenGL"    "sudo pacman -S qt6-base"

    echo ""
    echo -e "  ${BOLD}KDE Frameworks 6:${NC}"
    chk_file "ECMConfig.cmake"       "sudo pacman -S extra-cmake-modules" \
             "/usr/share/ECM /usr/lib/cmake/ECM"
    chk_file "KF6ConfigConfig.cmake" "sudo pacman -S kconfig" \
             "/usr/lib/cmake/KF6Config"
    chk_file "KF6CoreAddonsConfig.cmake" "sudo pacman -S kcoreaddons" \
             "/usr/lib/cmake/KF6CoreAddons"
    chk_file "KF6KCMUtilsConfig.cmake" "sudo pacman -S kcmutils" \
             "/usr/lib/cmake/KF6KCMUtils"
    chk_file "KF6I18nConfig.cmake" "sudo pacman -S ki18n" \
             "/usr/lib/cmake/KF6I18n"
    chk_file "KF6ConfigWidgetsConfig.cmake" "sudo pacman -S kconfigwidgets" \
             "/usr/lib/cmake/KF6ConfigWidgets"

    echo ""
    echo -e "  ${BOLD}KWin headers (kwin package on Arch):${NC}"
    if [[ -f /usr/include/kwin/effect/effect.h ]]; then
        local kwin_ver
        kwin_ver=$(grep -r 'KWIN_VERSION_STRING' /usr/include/kwin/ 2>/dev/null \
                   | head -1 | grep -oP '"[0-9.]+"' | tr -d '"' || echo "unknown")
        echo -e "  ${G}✓${NC} /usr/include/kwin/effect/effect.h  ${DIM}(version: $kwin_ver)${NC}"
    else
        echo -e "  ${R}✗${NC} /usr/include/kwin/effect/effect.h  ${DIM}→ sudo pacman -S kwin${NC}"
        (( n_fail++ ))
    fi

    if [[ -f /usr/include/kwin/opengl/glshader.h ]]; then
        echo -e "  ${G}✓${NC} /usr/include/kwin/opengl/glshader.h"
    else
        echo -e "  ${R}✗${NC} /usr/include/kwin/opengl/glshader.h  ${DIM}→ sudo pacman -S kwin${NC}"
        (( n_fail++ ))
    fi

    echo ""
    if [[ $n_fail -gt 0 ]]; then
        err "$n_fail requirement(s) missing"
        sep
        echo -e "${BOLD}Install everything with one command:${NC}"
        echo ""
        echo -e "  sudo pacman -S cmake make gcc extra-cmake-modules \\"
        echo -e "                 kwin qt6-base kconfig kcoreaddons kcmutils ki18n kconfigwidgets"
        return 1
    else
        ok "All requirements present ($((n_fail == 0 ? 1 : 0)) errors)"
        return 0
    fi
}

# ── Always run a dependency check ─────────────────────────────────────────────
check_deps || die "Fix missing requirements and try again."
[[ $DO_CHECK -eq 1 ]] && exit 0

# ══════════════════════════════════════════════════════════════════════════════
# BUILD
# ══════════════════════════════════════════════════════════════════════════════

# Remove build directory on --rebuild
if [[ $DO_REBUILD -eq 1 && -d "$BUILD_DIR" ]]; then
    banner "Rebuilding — removing build directory"
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# ── CMake configuration ───────────────────────────────────────────────────────
banner "Configuring CMake ($BUILD_TYPE)"
cmake "$SCRIPT_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DKDE_INSTALL_USE_QT_SYS_PATHS=ON \
    2>&1

sep

# ── Compiling ────────────────────────────────────────────────────────────────
JOBS=$(nproc 2>/dev/null || echo 4)
banner "Compiling (${JOBS} cores)"
cmake --build . --parallel "$JOBS" -- VERBOSE=1 2>&1

sep

# ── Installing ────────────────────────────────────────────────────────────────
banner "Installing to $PREFIX"
sudo cmake --install . 2>&1

sep

# ── Verifying ─────────────────────────────────────────────────────────────────
banner "Verifying installation"
PLUGIN_FOUND=0
for d in \
    "$PREFIX/lib/qt6/plugins/kwin/effects/plugins" \
    /usr/lib/qt6/plugins/kwin/effects/plugins \
    /usr/lib/kwin/effects/plugins
do
    if [[ -f "$d/kwin_effect_retro_term.so" ]]; then
        ok "Plugin: $d/kwin_effect_retro_term.so"
        PLUGIN_FOUND=1
        break
    fi
done
[[ $PLUGIN_FOUND -eq 0 ]] && warn "Plugin .so not found — check cmake --install output"

KCM_FOUND=0
for d in \
    "$PREFIX/lib/qt6/plugins/plasma/kcms/systemsettings_qwidgets" \
    /usr/lib/qt6/plugins/plasma/kcms/systemsettings_qwidgets
do
    if [[ -f "$d/kcm_retro_term.so" ]]; then
        ok "KCM: $d/kcm_retro_term.so"
        KCM_FOUND=1
        break
    fi
done
[[ $KCM_FOUND -eq 0 ]] && warn "KCM .so not found — check cmake --install output"

SHADER_FOUND=0
for d in \
    "$PREFIX/share/kwin/effects/retro-term" \
    /usr/share/kwin/effects/retro-term
do
    if [[ -f "$d/retro.frag" ]]; then
        ok "Shader: $d/retro.frag"
        SHADER_FOUND=1
        break
    fi
done
[[ $SHADER_FOUND -eq 0 ]] && warn "retro.frag not found"

sep

# ── Reloading KWin ─────────────────────────────────────────────────────────────
banner "Reloading KWin"
if qdbus6 org.kde.KWin /KWin reconfigure 2>/dev/null; then
    ok "Reloading KWin via qdbus6"
elif qdbus org.kde.KWin /KWin reconfigure 2>/dev/null; then
    ok "Reloading KWin via qdbus"
else
    warn "KWin not reachable via D-Bus — log out and back in to activate the effect"
fi

# ── Autostart entry for automatic rebuild after KWin update ──────────────────
AUTOSTART="$HOME/.config/autostart/phosphor-rebuild.desktop"
mkdir -p "$(dirname "$AUTOSTART")"
cat > "$AUTOSTART" <<DESKTOP
[Desktop Entry]
Type=Application
Name=Retro Term — KWin Effect Rebuild Check
Comment=Recompiles phosphor if the KWin version changed
Exec=bash -c 'kwin_ver=\$(pacman -Q kwin 2>/dev/null | awk "{print \$2}"); stamp="$BUILD_DIR/.kwin_ver"; [ "\$(cat \$stamp 2>/dev/null)" != "\$kwin_ver" ] && (cd ${SCRIPT_DIR} && ./build.sh --rebuild && echo \$kwin_ver > \$stamp) || true'
X-KDE-autostart-condition=ksmserver
Hidden=false
DESKTOP
ok "Autostart-entry: $AUTOSTART"

# ── Summary ───────────────────────────────────────────────────────────────────
echo ""
echo -e "${BOLD}${G}╔══════════════════════════════════════════════════════════╗"
echo -e "║  Installation successful!                                   ║"
echo -e "╚══════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "  ${BOLD}Enable effect:${NC}"
echo -e "  System Settings -> Workspace Behavior -> Screen Effects -> '${BOLD}Phosphor CRT${NC}'"
echo ""
echo -e "  ${BOLD}Or via command line:${NC}"
echo -e "  kwriteconfig6 --file kwinrc --group Effect-retro-terminal --key warmupEnabled true"
echo -e "  qdbus6 org.kde.KWin /KWin reconfigure"
echo ""
echo -e "  ${BOLD}Rebuild after a KWin update:${NC}"
echo -e "  ./build.sh --rebuild"
echo -e "  ${DIM}(or log out/in — autostart entry handles this automatically)${NC}"
echo ""
echo -e "  ${BOLD}View debug logging:${NC}"
echo -e "  ${DIM}journalctl -f | grep retro-term${NC}"
echo ""
