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
find_package(Qt6 REQUIRED COMPONENTS Core Gui OpenGL Widgets DBus)
find_package(KF6 REQUIRED COMPONENTS Config ConfigWidgets CoreAddons I18n KCMUtils)
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
    ${CMAKE_CURRENT_SOURCE_DIR}
    /usr/include/kwin
)

target_link_libraries(kwin_effect_retro_term PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::OpenGL
    Qt6::Widgets
    KF6::ConfigCore
    KF6::CoreAddons
    KWin::kwin
)

# The plugin file must not have a "lib" prefix — KWin loads it by exact name
set_target_properties(kwin_effect_retro_term PROPERTIES
    PREFIX ""
)

# ── Effect configuration module (Settings button) ────────────────────────────
add_library(kwin_retro_term_config MODULE
    retro_term_kcm.cpp
)

target_include_directories(kwin_retro_term_config PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(kwin_retro_term_config PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::DBus
    KF6::ConfigCore
    KF6::ConfigWidgets
    KF6::CoreAddons
    KF6::I18n
    KF6::KCMUtils
)

set_target_properties(kwin_retro_term_config PROPERTIES
    PREFIX ""
)

# ── Installation ──────────────────────────────────────────────────────────────

# Plasma 6 on Arch/Garuda loads effect and KCM plugins from the Qt6 plugin tree.
set(QT6_PLUGIN_ROOT "${CMAKE_INSTALL_FULL_LIBDIR}/qt6/plugins")

# 1. Plugin .so  →  Qt plugin directory for KWin effects
#    Arch: /usr/lib/qt6/plugins/kwin/effects/plugins/
install(TARGETS kwin_effect_retro_term
    DESTINATION ${QT6_PLUGIN_ROOT}/kwin/effects/plugins
)

# Effect config module for Desktop Effects "Settings" button
install(TARGETS kwin_retro_term_config
    DESTINATION ${QT6_PLUGIN_ROOT}/kwin/effects/configs
)

# 2. Fragment shader  →  KWin data directory
#    Arch: /usr/share/kwin/effects/retro-term/
install(FILES retro.frag
    DESTINATION ${KDE_INSTALL_DATADIR}/kwin/effects/retro-term
)

# Note: metadata.json is NOT installed separately — it is embedded in the binary
# plugin via Q_PLUGIN_METADATA. Installing it to the data dir would cause KWin
# to treat it as a scripted effect package, conflicting with the binary plugin.

# KCM desktop service file
configure_file(retro-term-kcm.desktop.in retro-term-kcm.desktop)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/retro-term-kcm.desktop"
    DESTINATION ${KDE_INSTALL_DATADIR}/kservices6
)
