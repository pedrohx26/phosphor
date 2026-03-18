cmake_minimum_required(VERSION 3.20)
project(retro-term VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# ── Extra CMake Modules ───────────────────────────────────────────────────────
find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH})

# ── Set Qt6 version for CMake macros ──────────────────────────────────────────
set(QT_DEFAULT_MAJOR_VERSION 6)

# ──  KDE install directory ONLY (bypass KDE settings to avoid Qt6Qml) ──────────
include(KDEInstallDirs6)

# ── Dependencies ──────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Core Gui OpenGL Widgets)
find_package(KWin REQUIRED)

# Print what we found (useful for debugging build issues)
message(STATUS "KWin include dirs: ${KWin_INCLUDE_DIRS}")
message(STATUS "KWin libraries:    ${KWin_LIBRARIES}")

# ── Plugin ────────────────────────────────────────────────────────────────────
add_library(kwin_effect_retro_term MODULE
    retro_term_effect.cpp
)

# On Arch, KWin headers are in /usr/include/kwin/
# KWin::kwin target should export these, but we add them explicitly as fallback
target_include_directories(kwin_effect_retro_term PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    /usr/include/kwin
)

target_link_libraries(kwin_effect_retro_term PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::OpenGL
    Qt6::Widgets
    KWin::kwin
)

# The plugin file must not have a "lib" prefix — KWin loads it by exact name
set_target_properties(kwin_effect_retro_term PROPERTIES
    PREFIX ""
)

# ── Installation ──────────────────────────────────────────────────────────────

# 1. Plugin .so  →  Qt plugin directory for KWin effects
#    Arch: /usr/lib/qt6/plugins/kwin/effects/plugins/
install(TARGETS kwin_effect_retro_term
    DESTINATION ${KDE_INSTALL_PLUGINDIR}/kwin/effects/plugins
)

# 2. Fragment shader  →  KWin data directory
#    Arch: /usr/share/kwin/effects/retro-term/
install(FILES src/retro.frag
    DESTINATION ${KDE_INSTALL_DATADIR}/kwin/effects/retro-term
)

# 3. Plugin metadata  →  same data directory
install(FILES metadata.json
    DESTINATION ${KDE_INSTALL_DATADIR}/kwin/effects/retro-term
)
