#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// retro-term — KCM (KDE Configuration Module)

#include <KCModule>
#include <KConfigGroup>
#include <KSharedConfig>

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSlider>
#include <QString>
#include <QWidget>
#include <QMap>

// ── Target mode ───────────────────────────────────────────────────────────────
// Controls which windows receive the CRT effect.
enum class TargetMode {
    Off        = 0,   // Effect niet actief (lege klasse-lijst)
    Terminals  = 1,   // Alleen bekende terminal-emulators
    AllWindows = 2,   // Alle vensters op het scherm
    Custom     = 3    // Vrije invoer van WM_CLASS namen
};

// Bekende terminals (gebruikt voor TargetMode::Terminals)
static const char *KNOWN_TERMINALS =
    "konsole,cool-retro-term,yakuake,kitty,alacritty,wezterm,xterm,gnome-terminal,tilix";

// ── Preset data ───────────────────────────────────────────────────────────────
struct PresetValues {
    QString name;
    QString era;
    int    phosphorType        = 1;
    double phosphorAgeing      = 0.05;
    double colorTemperature    = 7000;
    double phosphorPersistence = 0.10;
    double screenCurvature     = 0.25;
    double vignetteIntensity   = 0.35;
    double ambientReflection   = 0.04;
    int    rasterizationMode   = 1;
    double scanlinesIntensity  = 0.35;
    double scanlinesSharpness  = 0.50;
    double bloom               = 0.55;
    double glowingLine         = 0.20;
    double brightness          = 0.50;
    double contrast            = 0.80;
    double staticNoise         = 0.08;
    double jitter              = 0.10;
    int    syncMode            = 0;
    double horizontalSync      = 0.05;
    double flickering          = 0.08;
    double ghostingIntensity   = 0.00;
    double chromaColor         = 0.20;
    double saturationColor     = 0.20;
    double rbgShift            = 0.10;
    double characterSmearing   = 0.08;
    double burnIn              = 0.20;
    bool   warmupEnabled       = true;
    double warmupDuration      = 8.0;
    bool   degaussOnStart      = true;
    double degaussDuration     = 2.5;
    QString font;
    int    fontSize            = 14;
    // Pixel scaling: original screen resolution of the system
    double targetResX          = 0.0;   // 0 = disabled
    double targetResY          = 0.0;
};

// ── Slider + spinbox combo ────────────────────────────────────────────────────
class ParamRow : public QWidget
{
    Q_OBJECT
public:
    ParamRow(const QString &label, double min, double max,
             double step, const QString &tooltip, QWidget *parent = nullptr);
    double value() const;
    void   setValue(double v);
Q_SIGNALS:
    void valueChanged(double);
private:
    QSlider        *m_slider;
    QDoubleSpinBox *m_spin;
    double          m_min, m_max;
};

// ── KCM ──────────────────────────────────────────────────────────────────────
class RetroTermKCM : public KCModule
{
    Q_OBJECT
public:
    explicit RetroTermKCM(QObject *parent, const KPluginMetaData &data);
    void load()     override;
    void save()     override;
    void defaults() override;

private:
    void      buildPresets();
    void      buildUI();
    QGroupBox *makeGroup(const QString &title, QFormLayout *&layout);
    ParamRow  *addParam(QFormLayout *fl, const QString &label,
                        double min, double max, double step,
                        const QString &key, const QString &tip);
    void      applyPreset(const PresetValues &p);
    void      markChanged();
    void      setTargetMode(TargetMode mode);
    TargetMode currentTargetMode() const;

    // ── Modus-selector (bovenaan, meest prominent) ────────────────────────────
    QRadioButton *m_modeOff        = nullptr;
    QRadioButton *m_modeTerminals  = nullptr;
    QRadioButton *m_modeAll        = nullptr;
    QRadioButton *m_modeCustom     = nullptr;
    QButtonGroup *m_modeGroup      = nullptr;
    QWidget      *m_customRow      = nullptr;   // verborgen tenzij Custom
    QLineEdit    *m_targetClasses  = nullptr;   // vrije invoer
    QLabel       *m_customHint     = nullptr;

    // ── Preset selector ───────────────────────────────────────────────────────
    QComboBox   *m_presetCombo  = nullptr;
    QPushButton *m_applyPreset  = nullptr;
    QPushButton *m_applyKWin    = nullptr;

    // ── Parameter widgets ─────────────────────────────────────────────────────
    QMap<QString, ParamRow *>       m_params;
    QMap<QString, QComboBox *>      m_combos;
    QMap<QString, QCheckBox *>      m_checks;
    QMap<QString, QDoubleSpinBox *> m_spins;

    QList<PresetValues> m_presets;

    // Pixel scaling widgets
    ParamRow      *m_pixelScaleRow  = nullptr;
    QComboBox     *m_sampleModeCombo = nullptr;
    QDoubleSpinBox *m_targetResX    = nullptr;
    QDoubleSpinBox *m_targetResY    = nullptr;
    QWidget       *m_targetResRow   = nullptr;  // zichtbaar als preset resolutie heeft

    static constexpr const char *CFG_GROUP = "Effect-retro-terminal";
};
