cmake_minimum_required(VERSION 3.20)
project(retro-term VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# ── Extra CMake Modules ───────────────────────────────────────────────────────
find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH})

include(KDEInstallDirs6)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)

# ── Dependencies ──────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Core Gui OpenGL Widgets DBus)
find_package(KF6 REQUIRED COMPONENTS Config ConfigWidgets CoreAddons I18n KCMUtils)
find_package(KWin REQUIRED)

message(STATUS "Qt6 version:      ${Qt6_VERSION}")
message(STATUS "KF6 version:      ${KF6_VERSION}")
message(STATUS "KWin include dir: ${KWin_INCLUDE_DIRS}")

# ══════════════════════════════════════════════════════════════════════════════
# 1. KWin Effect plugin  (kwin_effect_retro_term.so)
# ══════════════════════════════════════════════════════════════════════════════
add_library(kwin_effect_retro_term MODULE
    src/retro_term_effect.cpp
)

target_include_directories(kwin_effect_retro_term PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    /usr/include/kwin
)

target_link_libraries(kwin_effect_retro_term PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::OpenGL
    KF6::ConfigCore
    KF6::CoreAddons
    KWin::kwin
)

set_target_properties(kwin_effect_retro_term PROPERTIES PREFIX "")

install(TARGETS kwin_effect_retro_term
    DESTINATION ${KDE_INSTALL_PLUGINDIR}/kwin/effects/plugins
)

# ══════════════════════════════════════════════════════════════════════════════
# 2. KCM configuration module  (kcm_retro_term.so)
#    This is what appears as the "Settings" button in Werkruimte-effecten.
# ══════════════════════════════════════════════════════════════════════════════
add_library(kcm_retro_term MODULE
    src/retro_term_kcm.cpp
)

target_include_directories(kcm_retro_term PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(kcm_retro_term PRIVATE
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

set_target_properties(kcm_retro_term PROPERTIES PREFIX "")

# KCM .so goes into the KDE plugin dir for kcms
install(TARGETS kcm_retro_term
    DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/kcms/systemsettings_qwidgets
)

# ══════════════════════════════════════════════════════════════════════════════
# 3. Shared data files
# ══════════════════════════════════════════════════════════════════════════════
install(FILES src/retro.frag
    DESTINATION ${KDE_INSTALL_DATADIR}/kwin/effects/retro-term
)

# metadata.json serves double duty:
#  - KWin effect registration (read by kwin_effect_retro_term)
#  - KCM registration (read by kcm_retro_term via K_PLUGIN_FACTORY_WITH_JSON)
install(FILES metadata.json
    DESTINATION ${KDE_INSTALL_DATADIR}/kwin/effects/retro-term
)

# KCM desktop service file — tells System Settings about the KCM
configure_file(retro-term-kcm.desktop.in retro-term-kcm.desktop)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/retro-term-kcm.desktop"
    DESTINATION ${KDE_INSTALL_DATADIR}/kservices6
)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
