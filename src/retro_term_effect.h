#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// retro-term — KWin 6 C++ Effect
//
// Header paths from Arch Linux kwin 6.6.x package:
//   /usr/include/kwin/effect/effect.h
//   /usr/include/kwin/effect/effecthandler.h
//   /usr/include/kwin/effect/effectwindow.h
//   /usr/include/kwin/opengl/glshader.h
//   /usr/include/kwin/opengl/glshadermanager.h
//   /usr/include/kwin/core/rendertarget.h
//   /usr/include/kwin/core/renderviewport.h

#include <kwin/effect/effect.h>
#include <kwin/effect/effecthandler.h>
#include <kwin/effect/effectwindow.h>
#include <kwin/opengl/glshader.h>
#include <kwin/opengl/glshadermanager.h>
#include <kwin/opengl/glplatform.h>
#include <kwin/core/rendertarget.h>
#include <kwin/core/renderviewport.h>

#include <QElapsedTimer>
#include <QHash>
#include <QStringList>
#include <memory>

namespace KWin
{

class RetroTermEffect : public Effect
{
    Q_OBJECT

public:
    RetroTermEffect();
    ~RetroTermEffect() override;

    void paintWindow(const RenderTarget &renderTarget,
                     const RenderViewport &viewport,
                     EffectWindow *w,
                     int mask,
                     const Region &deviceRegion,
                     WindowPaintData &data) override;

    bool isActive() const override { return true; }
    static bool supported();
    void reconfigure(ReconfigureFlags flags) override;

private:
    void loadShader();
    void loadConfig();
    bool isTarget(EffectWindow *w) const;

    struct WindowState {
        double warmupElapsed  = 0.0;   // seconds since window opened
        double degaussElapsed = 0.0;
        qint64 lastPaintMs    = -1;    // wall-clock ms at last paint call
    };
    QHash<EffectWindow *, WindowState> m_windows;

    std::unique_ptr<GLShader> m_shader;
    bool m_valid = false;

    QElapsedTimer m_wallClock;

    // Config
    QStringList m_targetClasses;
    int    m_phosphorType        = 1;
    float  m_phosphorAgeing      = 0.05f;
    float  m_colorTemperature    = 7000.f;
    float  m_phosphorPersistence = 0.10f;
    float  m_screenCurvature     = 0.25f;
    float  m_vignetteIntensity   = 0.35f;
    float  m_ambientReflection   = 0.04f;
    int    m_rasterizationMode   = 1;
    float  m_scanlinesIntensity  = 0.35f;
    float  m_scanlinesSharpness  = 0.50f;
    float  m_bloom               = 0.55f;
    float  m_glowingLine         = 0.20f;
    float  m_brightness          = 0.50f;
    float  m_contrast            = 0.80f;
    float  m_staticNoise         = 0.08f;
    float  m_jitter              = 0.10f;
    int    m_syncMode            = 0;
    float  m_horizontalSync      = 0.05f;
    float  m_flickering          = 0.08f;
    float  m_ghostingIntensity   = 0.00f;
    float  m_chromaColor         = 0.20f;
    float  m_saturationColor     = 0.20f;
    float  m_rbgShift            = 0.10f;
    float  m_characterSmearing   = 0.08f;
    float  m_burnIn              = 0.20f;
    bool   m_warmupEnabled       = true;
    float  m_warmupDuration      = 8.0f;
    bool   m_degaussOnStart      = true;
    float  m_degaussDuration     = 2.5f;

    // Pixel scaling
    float  m_pixelScale          = 0.0f;   // 0=off, 1=pixel-exact
    float  m_targetResX          = 320.0f; // original width
    float  m_targetResY          = 200.0f; // original height
    int    m_sampleMode          = 2;      // 0=nearest 1=bilinear 2=sharp-bilinear
};

} // namespace KWin
