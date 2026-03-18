#!/usr/bin/env bash
# ╔══════════════════════════════════════════════════════════════════════════════╗
# ║  phosphor — KWin Effect + KCM Build Script                               ║
# ║  Compileert en installeert automatisch, zonder IDE                         ║
# ╚══════════════════════════════════════════════════════════════════════════════╝
# Gebruik:
#   ./build.sh                  Bouw en installeer (Release)
#   ./build.sh --check-deps     Controleer vereisten
#   ./build.sh --rebuild        Verwijder build/ en bouw opnieuw
#   ./build.sh --debug          Debug-build
#   ./build.sh --uninstall      Verwijder geïnstalleerde bestanden
#   ./build.sh --prefix=/pad    Alternatief installatieprefix (standaard: /usr)
set -euo pipefail

R='\033[0;31m' G='\033[0;32m' Y='\033[1;33m' C='\033[0;36m'
BOLD='\033[1m' DIM='\033[2m' NC='\033[0m'
ok()   { echo -e "${G}[ ok ]${NC}  $*"; }
info() { echo -e "${C}[info]${NC}  $*"; }
warn() { echo -e "${Y}[warn]${NC}  $*"; }
err()  { echo -e "${R}[fout]${NC}  $*" >&2; }
die()  { err "$*"; exit 1; }
sep()  { echo -e "${DIM}──────────────────────────────────────────────────────${NC}"; }
hdr()  { echo -e "\n${BOLD}${C}▶ $*${NC}"; }

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
BUILD_TYPE="Release"
PREFIX="/usr"
DO_REBUILD=0 DO_CHECK=0 DO_UNINSTALL=0

for arg in "$@"; do
    case "$arg" in
        --check-deps)  DO_CHECK=1 ;;
        --rebuild)     DO_REBUILD=1 ;;
        --debug)       BUILD_TYPE="Debug" ;;
        --uninstall)   DO_UNINSTALL=1 ;;
        --prefix=*)    PREFIX="${arg#*=}" ;;
        --help|-h)
            sed -n '3,9p' "$0" | sed 's/^# \{0,2\}//'
            exit 0 ;;
    esac
done

# ── Uninstall ─────────────────────────────────────────────────────────────────
if [[ $DO_UNINSTALL -eq 1 ]]; then
    hdr "Uninstalling"
    for d in /usr/lib/qt6/plugins/kwin/effects/plugins \
              "$PREFIX/lib/qt6/plugins/kwin/effects/plugins"; do
        [[ -f "$d/kwin_effect_retro_term.so" ]] && \
            sudo rm -f "$d/kwin_effect_retro_term.so" && ok "Effect .so verwijderd" || true
    done
    for d in /usr/lib/qt6/plugins/plasma/kcms/systemsettings_qwidgets \
              "$PREFIX/lib/qt6/plugins/plasma/kcms/systemsettings_qwidgets"; do
        [[ -f "$d/kcm_retro_term.so" ]] && \
            sudo rm -f "$d/kcm_retro_term.so" && ok "KCM .so verwijderd" || true
    done
    sudo rm -rf \
        /usr/share/kwin/effects/phosphor \
        "$PREFIX/share/kwin/effects/phosphor" \
        /usr/share/kservices6/phosphor-kcm.desktop \
        "$PREFIX/share/kservices6/phosphor-kcm.desktop" 2>/dev/null || true
    ok "Data-bestanden verwijderd"
    qdbus6 org.kde.KWin /KWin reconfigure 2>/dev/null || \
    qdbus  org.kde.KWin /KWin reconfigure 2>/dev/null || true
    exit 0
fi

# ── Dependency check ──────────────────────────────────────────────────────────
hdr "Checking requirements"
FAIL=0

chk_cmd() {
    command -v "$1" &>/dev/null \
        && echo -e "  ${G}✓${NC} $1" \
        || { echo -e "  ${R}✗${NC} $1  ${DIM}→ $2${NC}"; ((FAIL++)); }
}
chk_cmake() {
    find $2 -name "$1" 2>/dev/null | grep -q . \
        && echo -e "  ${G}✓${NC} $1" \
        || { echo -e "  ${R}✗${NC} $1  ${DIM}→ $3${NC}"; ((FAIL++)); }
}
chk_header() {
    [[ -f "/usr/include/$1" ]] \
        && echo -e "  ${G}✓${NC} /usr/include/$1" \
        || { echo -e "  ${R}✗${NC} $1  ${DIM}→ $2${NC}"; ((FAIL++)); }
}

echo -e "\n  ${BOLD}Build tools:${NC}"
chk_cmd cmake      "sudo pacman -S cmake"
chk_cmd g++        "sudo pacman -S gcc"
chk_cmd pkg-config "sudo pacman -S pkgconf"

echo -e "\n  ${BOLD}Qt6:${NC}"
for qt in Qt6Core Qt6Gui Qt6Widgets Qt6DBus Qt6OpenGL; do
    chk_cmake "${qt}Config.cmake" \
        "/usr/lib/cmake/${qt} /usr/share/cmake/${qt}" \
        "sudo pacman -S qt6-base"
done

echo -e "\n  ${BOLD}KDE Frameworks 6:${NC}"
chk_cmake "ECMConfig.cmake"          "/usr/share/ECM /usr/lib/cmake/ECM" \
    "sudo pacman -S extra-cmake-modules"
chk_cmake "KF6ConfigConfig.cmake"    "/usr/lib/cmake/KF6Config"        "sudo pacman -S kconfig"
chk_cmake "KF6CoreAddonsConfig.cmake""/usr/lib/cmake/KF6CoreAddons"    "sudo pacman -S kcoreaddons"
chk_cmake "KF6KCMUtilsConfig.cmake"  "/usr/lib/cmake/KF6KCMUtils"     "sudo pacman -S kcmutils"
chk_cmake "KF6I18nConfig.cmake"      "/usr/lib/cmake/KF6I18n"          "sudo pacman -S ki18n"
chk_cmake "KF6ConfigWidgetsConfig.cmake" "/usr/lib/cmake/KF6ConfigWidgets" "sudo pacman -S kconfigwidgets"

echo -e "\n  ${BOLD}KWin headers:${NC}"
chk_header "kwin/effect/effect.h"       "sudo pacman -S kwin"
chk_header "kwin/opengl/glshader.h"     "sudo pacman -S kwin"
chk_header "kwin/core/rendertarget.h"   "sudo pacman -S kwin"
chk_header "kwin/core/renderviewport.h" "sudo pacman -S kwin"

echo ""
if [[ $FAIL -gt 0 ]]; then
    err "$FAIL ontbrekende vereiste(n)"
    echo ""
    echo -e "${BOLD}Alles installeren:${NC}"
    echo -e "  sudo pacman -S cmake gcc extra-cmake-modules kwin \\"
    echo -e "                 qt6-base kconfig kcoreaddons kcmutils ki18n kconfigwidgets"
    exit 1
fi
ok "Alle vereisten aanwezig"
[[ $DO_CHECK -eq 1 ]] && exit 0

# ── Build ─────────────────────────────────────────────────────────────────────
[[ $DO_REBUILD -eq 1 && -d "$BUILD_DIR" ]] && rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR" && cd "$BUILD_DIR"

hdr "Configuring CMake ($BUILD_TYPE)"
cmake "$SCRIPT_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DKDE_INSTALL_USE_QT_SYS_PATHS=ON
sep

JOBS=$(nproc 2>/dev/null || echo 4)
hdr "Compiling ($JOBS kernen)"
cmake --build . --parallel "$JOBS"
sep

hdr "Installing to $PREFIX"
sudo cmake --install .
sep

# ── Verificatie ───────────────────────────────────────────────────────────────
hdr "Verificatie"
for d in /usr/lib/qt6/plugins/kwin/effects/plugins \
         "$PREFIX/lib/qt6/plugins/kwin/effects/plugins"; do
    [[ -f "$d/kwin_effect_retro_term.so" ]] && \
        ok "Effect plugin: $d/kwin_effect_retro_term.so" && break || true
done
for d in /usr/lib/qt6/plugins/plasma/kcms/systemsettings_qwidgets \
         "$PREFIX/lib/qt6/plugins/plasma/kcms/systemsettings_qwidgets"; do
    [[ -f "$d/kcm_retro_term.so" ]] && \
        ok "KCM plugin:    $d/kcm_retro_term.so" && break || true
done
for d in /usr/share/kwin/effects/phosphor \
         "$PREFIX/share/kwin/effects/phosphor"; do
    [[ -f "$d/retro.frag" ]] && ok "Shader:        $d/retro.frag" && break || true
done

# ── Reloading KWin ─────────────────────────────────────────────────────────────
hdr "Reloading KWin"
qdbus6 org.kde.KWin /KWin reconfigure 2>/dev/null && ok "Reloading KWin (qdbus6)" || \
qdbus  org.kde.KWin /KWin reconfigure 2>/dev/null && ok "Reloading KWin (qdbus)"  || \
warn "KWin not reachable via D-Bus — log uit en in"

# ── Autostart voor automatisch herbouwen na KWin-update ──────────────────────
AUTOSTART="$HOME/.config/autostart/phosphor-rebuild.desktop"
mkdir -p "$(dirname "$AUTOSTART")"
cat > "$AUTOSTART" <<DESKTOP
[Desktop Entry]
Type=Application
Name=Retro Term Rebuild Check
Exec=bash -c 'v=\$(pacman -Q kwin 2>/dev/null | awk "{print \$2}"); s="${BUILD_DIR}/.kwin_ver"; [ "\$(cat \$s 2>/dev/null)" != "\$v" ] && (cd ${SCRIPT_DIR} && ./build.sh --rebuild && echo \$v > \$s) || true'
X-KDE-autostart-condition=ksmserver
Hidden=false
DESKTOP
ok "Autostart-entry: $AUTOSTART"

# ── Samenvatting ──────────────────────────────────────────────────────────────
echo ""
echo -e "${G}${BOLD}╔══════════════════════════════════════════════════════════╗"
echo -e "║  Installation successful!                                   ║"
echo -e "╚══════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "  ${BOLD}Activeren + presets instellen:${NC}"
echo -e "  Systeeminstellingen → Werkruimte-effecten"
echo -e "  → ${BOLD}'Phosphor CRT'${NC} aanzetten"
echo -e "  → Klik ${BOLD}'Instellingen'${NC} voor preset-selector en alle sliders"
echo ""
echo -e "  ${BOLD}Via commandoregel:${NC}"
echo -e "  phosphor on && phosphor preset IBM_VGA"
echo ""
echo -e "  ${DIM}journalctl -f | grep phosphor${NC}  ← debug logging"
echo ""
