// SPDX-License-Identifier: GPL-2.0-or-later
// retro-term — KWin 6 C++ Effect Implementation

#include "retro_term_effect.h"

#include <kwin/effect/effecthandler.h>
#include <kwin/opengl/glplatform.h>

#include <KConfigGroup>
#include <KSharedConfig>
#include <QDebug>
#include <QFile>
#include <QStandardPaths>
// ── Factory macro — tells KWin how to instantiate this plugin ────────────────
// K_PLUGIN_FACTORY_WITH_JSON must be at global scope (outside any namespaces)
// for Qt's plugin system to find it.
#include <KPluginFactory>

class RetroTermEffectFactory : public KPluginFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID KPluginFactory_iid FILE "metadata.json")

public:
    QObject *create(const char *iface, QWidget *parentWidget, QObject *parent, const QVariantList &args, const QString &keyword)
    {
        Q_UNUSED(iface);
        Q_UNUSED(parentWidget);
        Q_UNUSED(parent);
        Q_UNUSED(args);
        Q_UNUSED(keyword);
        return new KWin::RetroTermEffect();
    }
};

namespace KWin
{

// ══════════════════════════════════════════════════════════════════════════════

RetroTermEffect::RetroTermEffect()
{
    m_wallClock.start();
    loadConfig();
    loadShader();

    // Register windows already on screen
    for (EffectWindow *w : effects->stackingOrder()) {
        if (isTarget(w))
            m_windows.insert(w, WindowState{});
    }

    connect(effects, &EffectsHandler::windowAdded, this, [this](EffectWindow *w) {
        if (isTarget(w))
            m_windows.insert(w, WindowState{});
    });
    connect(effects, &EffectsHandler::windowClosed, this, [this](EffectWindow *w) {
        m_windows.remove(w);
    });

    qDebug() << "[retro-term] Loaded. Targeting:" << m_targetClasses.join(u", ");
}

RetroTermEffect::~RetroTermEffect() = default;

// ── Shader ────────────────────────────────────────────────────────────────────
void RetroTermEffect::loadShader()
{
    // KWin installs data files to $datadir/kwin/effects/<id>/
    // On Arch: /usr/share/kwin/effects/retro-term/retro.frag
    const QString fragPath = QStandardPaths::locate(
        QStandardPaths::GenericDataLocation,
        QStringLiteral("kwin/effects/retro-term/retro.frag"));

    if (fragPath.isEmpty()) {
        qWarning() << "[retro-term] retro.frag not found in kwin data paths";
        m_valid = false;
        return;
    }

    // ShaderManager::generateShaderFromFile() takes a ShaderTrait bitmask describing
    // what the built-in vertex shader provides, plus the path to our fragment shader.
    // ShaderTrait::MapTexture provides texcoord0 (normalised UV) and the sampler.
    // Second parameter is vertexFile (empty = use default), third is fragmentFile.
    m_shader = ShaderManager::instance()->generateShaderFromFile(
        ShaderTrait::MapTexture, QString(), fragPath);

    m_valid = m_shader && m_shader->isValid();
    if (m_valid)
        qDebug() << "[retro-term] Shader compiled OK from" << fragPath;
    else
        qWarning() << "[retro-term] Shader compilation failed";
}

// ── Config ────────────────────────────────────────────────────────────────────
void RetroTermEffect::loadConfig()
{
    KConfigGroup cfg = KSharedConfig::openConfig(QStringLiteral("kwinrc"))
                           ->group(QStringLiteral("Effect-retro-terminal"));

    const QString cls = cfg.readEntry("targetClasses",
        QStringLiteral("konsole,cool-retro-term,yakuake,kitty,alacritty"));
    m_targetClasses.clear();
    for (const QString &c : cls.split(u',')) {
        const QString t = c.trimmed().toLower();
        if (!t.isEmpty()) m_targetClasses.append(t);
    }

    auto f = [&](const char *k, float d){ return (float)cfg.readEntry(k, (double)d); };
    auto i = [&](const char *k, int   d){ return cfg.readEntry(k, d); };
    auto b = [&](const char *k, bool  d){ return cfg.readEntry(k, d); };

    m_phosphorType        = i("phosphorType",        1);
    m_phosphorAgeing      = f("phosphorAgeing",      0.05f);
    m_colorTemperature    = f("colorTemperature",    7000.f);
    m_phosphorPersistence = f("phosphorPersistence", 0.10f);
    m_screenCurvature     = f("screenCurvature",     0.25f);
    m_vignetteIntensity   = f("vignetteIntensity",   0.35f);
    m_ambientReflection   = f("ambientReflection",   0.04f);
    m_rasterizationMode   = i("rasterizationMode",   1);
    m_scanlinesIntensity  = f("scanlinesIntensity",  0.35f);
    m_scanlinesSharpness  = f("scanlinesSharpness",  0.50f);
    m_bloom               = f("bloom",               0.55f);
    m_glowingLine         = f("glowingLine",         0.20f);
    m_brightness          = f("brightness",          0.50f);
    m_contrast            = f("contrast",            0.80f);
    m_staticNoise         = f("staticNoise",         0.08f);
    m_jitter              = f("jitter",              0.10f);
    m_syncMode            = i("syncMode",            0);
    m_horizontalSync      = f("horizontalSync",      0.05f);
    m_flickering          = f("flickering",          0.08f);
    m_ghostingIntensity   = f("ghostingIntensity",   0.00f);
    m_chromaColor         = f("chromaColor",         0.20f);
    m_saturationColor     = f("saturationColor",     0.20f);
    m_rbgShift            = f("rbgShift",            0.10f);
    m_characterSmearing   = f("characterSmearing",   0.08f);
    m_burnIn              = f("burnIn",              0.20f);
    m_warmupEnabled       = b("warmupEnabled",       true);
    m_warmupDuration      = f("warmupDuration",      8.0f);
    m_degaussOnStart      = b("degaussOnStart",      true);
    m_degaussDuration     = f("degaussDuration",     2.5f);
}

void RetroTermEffect::reconfigure(ReconfigureFlags /*flags*/)
{
    loadConfig();
}

// ── Target detection ──────────────────────────────────────────────────────────
bool RetroTermEffect::isTarget(EffectWindow *w) const
{
    if (!w) return false;
    const QString wc = w->windowClass().toLower();
    for (const QString &t : m_targetClasses)
        if (wc.contains(t)) return true;
    return false;
}

// ── Paint ─────────────────────────────────────────────────────────────────────
void RetroTermEffect::paintWindow(const RenderTarget &renderTarget,
                                   const RenderViewport &viewport,
                                   EffectWindow *w,
                                   int mask,
                                   const Region &deviceRegion,
                                   WindowPaintData &data)
{
    if (!m_valid || !isTarget(w)) {
        effects->paintWindow(renderTarget, viewport, w, mask, deviceRegion, data);
        return;
    }

    // ── Per-window delta-time ─────────────────────────────────────────────────
    auto &ws = m_windows[w];
    const qint64 nowMs = m_wallClock.elapsed();
    if (ws.lastPaintMs < 0) ws.lastPaintMs = nowMs;
    const double deltaS = qMin((nowMs - ws.lastPaintMs) / 1000.0, 0.1); // cap at 100ms
    ws.lastPaintMs = nowMs;

    // Advance animation timers
    if (m_warmupEnabled && ws.warmupElapsed < m_warmupDuration)
        ws.warmupElapsed = qMin(ws.warmupElapsed + deltaS, (double)m_warmupDuration);
    if (m_degaussOnStart && ws.degaussElapsed < m_degaussDuration)
        ws.degaussElapsed = qMin(ws.degaussElapsed + deltaS, (double)m_degaussDuration);

    const float warmupP  = m_warmupEnabled
        ? (float)(ws.warmupElapsed / m_warmupDuration)
        : 1.0f;
    const float degaussP = (m_degaussOnStart && ws.degaussElapsed < m_degaussDuration)
        ? (float)(ws.degaussElapsed / m_degaussDuration)
        : 0.0f;

    const float timeSec = (float)(nowMs / 1000.0);
    const QRectF geo = w->frameGeometry();

    // ── Bind shader and set uniforms ──────────────────────────────────────────
    ShaderManager::instance()->pushShader(m_shader.get());

    m_shader->setUniform("resolution",
        QVector2D((float)geo.width(), (float)geo.height()));
    m_shader->setUniform("time",                timeSec);
    m_shader->setUniform("phosphorType",        m_phosphorType);
    m_shader->setUniform("phosphorAgeing",      m_phosphorAgeing);
    m_shader->setUniform("colorTemperature",    m_colorTemperature);
    m_shader->setUniform("phosphorPersistence", m_phosphorPersistence);
    m_shader->setUniform("screenCurvature",     m_screenCurvature);
    m_shader->setUniform("vignetteIntensity",   m_vignetteIntensity);
    m_shader->setUniform("ambientReflection",   m_ambientReflection);
    m_shader->setUniform("rasterizationMode",   m_rasterizationMode);
    m_shader->setUniform("scanlinesIntensity",  m_scanlinesIntensity);
    m_shader->setUniform("scanlinesSharpness",  m_scanlinesSharpness);
    m_shader->setUniform("bloom",               m_bloom);
    m_shader->setUniform("glowingLine",         m_glowingLine);
    m_shader->setUniform("brightness",          m_brightness);
    m_shader->setUniform("contrast",            m_contrast);
    m_shader->setUniform("staticNoise",         m_staticNoise);
    m_shader->setUniform("jitter",              m_jitter);
    m_shader->setUniform("syncMode",            m_syncMode);
    m_shader->setUniform("horizontalSync",      m_horizontalSync);
    m_shader->setUniform("flickering",          m_flickering);
    m_shader->setUniform("ghostingIntensity",   m_ghostingIntensity);
    m_shader->setUniform("chromaColor",         m_chromaColor);
    m_shader->setUniform("saturationColor",     m_saturationColor);
    m_shader->setUniform("rbgShift",            m_rbgShift);
    m_shader->setUniform("characterSmearing",   m_characterSmearing);
    m_shader->setUniform("burnIn",              m_burnIn);
    m_shader->setUniform("warmupProgress",      warmupP);
    m_shader->setUniform("degaussProgress",     degaussP);

    // Paint the window — KWin's compositing pipeline renders it through our shader
    effects->paintWindow(renderTarget, viewport, w, mask, deviceRegion, data);

    ShaderManager::instance()->popShader();

    // Request continuous repaint for animated / noisy effects
    const bool animating = (m_warmupEnabled  && ws.warmupElapsed  < m_warmupDuration)
                        || (m_degaussOnStart && ws.degaussElapsed < m_degaussDuration);
    if (animating || m_staticNoise > 0.001f || m_flickering > 0.001f
        || m_jitter > 0.001f || m_syncMode != 0)
    {
        w->addRepaintFull();
    }
}

// ── supported() ──────────────────────────────────────────────────────────────
bool RetroTermEffect::supported()
{
    return effects->isOpenGLCompositing();
}

} // namespace KWin

// Required — includes the moc-generated code for Q_OBJECT
#include "retro_term_effect.moc"
