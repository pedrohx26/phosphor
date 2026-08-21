// SPDX-License-Identifier: GPL-2.0-or-later
// retro-term — KWin 6 C++ Effect Implementation

#include "retro_term_effect.h"

#include <effect/effecthandler.h>
#include <opengl/glplatform.h>

#include <KConfigGroup>
#include <KSharedConfig>
#include <QDebug>
#include <QFile>
#include <QStandardPaths>
namespace KWin
{

// ══════════════════════════════════════════════════════════════════════════════

RetroTermEffect::RetroTermEffect()
{
    m_wallClock.start();
    loadConfig();
    loadShader();

    // Register windows already on screen
    for (EffectWindow *w : effects->stackingOrder())
        updateRedirect(w);

    connect(effects, &EffectsHandler::windowAdded, this, &RetroTermEffect::updateRedirect);
    connect(effects, &EffectsHandler::windowClosed, this, [this](EffectWindow *w) {
        if (m_windows.remove(w))
            unredirect(w);
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

    // The two SDKs report failure differently. The kwin-x11 SDK hands back a
    // non-null but unusable GLShader — for instance when it cannot read the
    // core-profile variant of the file — and only isValid() reveals that; the
    // window then renders pure white with no error logged anywhere. The Wayland
    // SDK dropped isValid() and returns nullptr instead.
#ifdef RETRO_SHADER_HAS_ISVALID
    m_valid = m_shader && m_shader->isValid();
#else
    m_valid = m_shader != nullptr;
#endif
    if (m_valid)
        qDebug() << "[retro-term] Shader compiled OK from" << fragPath;
    else
        qWarning() << "[retro-term] Shader compilation failed";
}

// ── Config ────────────────────────────────────────────────────────────────────
void RetroTermEffect::loadConfig()
{
    // KSharedConfig hands out a cached object, so without this a live kwinrc edit
    // followed by "qdbus org.kde.KWin /KWin reconfigure" reads back the values the
    // effect was constructed with and appears to do nothing.
    KSharedConfig::Ptr config = KSharedConfig::openConfig(QStringLiteral("kwinrc"));
    config->reparseConfiguration();
    KConfigGroup cfg = config->group(QStringLiteral("Effect-retro-terminal"));

    const QString cls = cfg.readEntry("targetClasses",
        QStringLiteral("konsole,cool-retro-term,yakuake,kitty,alacritty"));
    m_targetClasses.clear();
    for (const QString &c : cls.split(u',')) {
        const QString t = c.trimmed().toLower();
        if (!t.isEmpty()) m_targetClasses.append(t);
    }

    // Pixel scaling
    m_pixelScale  = (float)cfg.readEntry("pixelScale", 0.0);
    m_targetResX  = (float)cfg.readEntry("targetResX", 320.0);
    m_targetResY  = (float)cfg.readEntry("targetResY", 200.0);
    m_sampleMode  = cfg.readEntry("sampleMode", 2);
    m_integerZoom = cfg.readEntry("integerZoom", 0);

    m_contentInsetTop    = cfg.readEntry("contentInsetTop",    0);
    m_contentInsetBottom = cfg.readEntry("contentInsetBottom", 0);
    m_contentInsetLeft   = cfg.readEntry("contentInsetLeft",   0);
    m_contentInsetRight  = cfg.readEntry("contentInsetRight",  0);

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
    // The scope may have changed — re-evaluate every window.
    for (EffectWindow *w : effects->stackingOrder())
        updateRedirect(w);
}

// ── Redirect bookkeeping ──────────────────────────────────────────────────────
void RetroTermEffect::updateRedirect(EffectWindow *w)
{
    if (!w) return;
    const bool wanted = m_valid && isTarget(w);
    const bool active = m_windows.contains(w);
    if (wanted == active)
        return;

    if (wanted) {
        m_windows.insert(w, WindowState{});
        redirect(w);
        setShader(w, m_shader.get());
        w->addRepaintFull();
    } else {
        m_windows.remove(w);
        unredirect(w);
        w->addRepaintFull();
    }
}

// ── Target detection ──────────────────────────────────────────────────────────
bool RetroTermEffect::isTarget(EffectWindow *w) const
{
    if (!w) return false;
    // "*" means all windows.
    if (m_targetClasses.size() == 1 && m_targetClasses.first() == u"*")
        return true;
    // Empty list means effect disabled.
    if (m_targetClasses.isEmpty())
        return false;
    const QString wc = w->windowClass().toLower();
    for (const QString &t : m_targetClasses)
        if (wc.contains(t)) return true;
    return false;
}

// ── Paint ─────────────────────────────────────────────────────────────────────
// Called by OffscreenEffect::drawWindow() right before the redirected texture is
// drawn with our shader. Uniform values live in the program object, so binding
// here and unbinding again leaves them in place for the actual draw.
void RetroTermEffect::apply(EffectWindow *w, int mask, WindowPaintData &data, WindowQuadList &quads)
{
    Q_UNUSED(mask);
    Q_UNUSED(data);
    Q_UNUSED(quads);

    auto it = m_windows.find(w);
    if (!m_valid || it == m_windows.end())
        return;

    // ── Per-window delta-time ─────────────────────────────────────────────────
    auto &ws = *it;
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

    // ── Confine the effect to the terminal's content area ─────────────────────
    // The offscreen texture spans expandedGeometry(): frame, decoration and drop
    // shadow. contentsRect() is the content area within the frame, so this is
    // where the terminal itself lives. Handing the shader that sub-rectangle in
    // texture coordinates keeps the decoration and the shadow untouched, and
    // keeps scanline spacing tied to the terminal rather than to the window.
    const QRectF expanded = w->expandedGeometry();
    QRectF content = QRectF(w->contentsRect()).translated(QRectF(w->frameGeometry()).topLeft());

    // Shave off the terminal's own chrome, if the user configured any. Ignored
    // when it would leave nothing to draw on.
    const QRectF trimmed = content.adjusted(m_contentInsetLeft, m_contentInsetTop,
                                            -m_contentInsetRight, -m_contentInsetBottom);
    if (trimmed.width() > 1.0 && trimmed.height() > 1.0)
        content = trimmed;

    // The texture's Y axis runs bottom-up while window geometry runs top-down, so
    // the vertical edges swap on the way in. X is not flipped. Verified against
    // the real texture with a shader that colours the low and high edges of each
    // axis; without the flip a top inset silently trims the bottom instead.
    QVector4D contentRect(0.0f, 0.0f, 1.0f, 1.0f);
    QRectF effective = expanded;
    if (expanded.width() > 0.0 && expanded.height() > 0.0
        && content.width() > 0.0 && content.height() > 0.0) {
        contentRect = QVector4D(
            (float)((content.left()      - expanded.left())   / expanded.width()),
            (float)((expanded.bottom()   - content.bottom())  / expanded.height()),
            (float)((content.right()     - expanded.left())   / expanded.width()),
            (float)((expanded.bottom()   - content.top())     / expanded.height()));
        effective = content;
    }

    // ── Bind shader and set uniforms ──────────────────────────────────────────
    ShaderManager::instance()->pushShader(m_shader.get());

    m_shader->setUniform("contentRect",         contentRect);
    m_shader->setUniform("resolution",
        QVector2D((float)effective.width(), (float)effective.height()));
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
    m_shader->setUniform("pixelScale",          m_pixelScale);
    m_shader->setUniform("targetRes",           QVector2D(m_targetResX, m_targetResY));
    m_shader->setUniform("sampleMode",          m_sampleMode);
    m_shader->setUniform("integerZoom",         m_integerZoom);

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
    return effects->isOpenGLCompositing() && OffscreenEffect::supported();
}

// ── Factory ──────────────────────────────────────────────────────────────────
// Must use KWin's own macro: KWin looks the plugin up by EffectPluginFactory_iid
// ("org.kde.kwin.EffectPluginFactory" + KWin version). A plain KPluginFactory
// with KPluginFactory_iid compiles and installs fine but is never loaded.
KWIN_EFFECT_FACTORY_SUPPORTED(RetroTermEffect, "metadata.json",
                              return RetroTermEffect::supported();)

} // namespace KWin

// Required — includes the moc-generated code for Q_OBJECT
#include "retro_term_effect.moc"
