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
#include <QFontDatabase>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSet>
#include <QSlider>
#include <QSpinBox>
#include <QStandardPaths>
#include <QString>
#include <QTabWidget>
#include <QTimer>
#include <QWidget>
#include <QMap>

// ── Target mode ───────────────────────────────────────────────────────────────
// Controls which windows receive the CRT effect.
enum class TargetMode {
    Off        = 0,   // Effect niet actief (lege klasse-lijst)
    Terminals  = 1,   // Gedetecteerde terminal-emulators, per stuk aan/uit te vinken
    AllWindows = 2,   // Verwijderd uit de UI; alleen nog gebruikt om oude configs te migreren
    Custom     = 3    // Vrije invoer van WM_CLASS namen
};

// Terminal-kandidaten voor TargetMode::Terminals. "exec" is zowel de naam die
// QStandardPaths::findExecutable() opzoekt als de token die in targetClasses
// terechtkomt — het effect vergelijkt daar met een substring-match tegen
// w->windowClass() tegenaan, dus deze korte namen zijn precies wat moet matchen.
struct TerminalCandidate { const char *exec; const char *display; };
static const TerminalCandidate TERMINAL_CANDIDATES[] = {
    {"konsole",         "Konsole"},
    {"yakuake",         "Yakuake"},
    {"kitty",           "Kitty"},
    {"alacritty",       "Alacritty"},
    {"wezterm",         "WezTerm"},
    {"xterm",           "xterm"},
    {"gnome-terminal",  "GNOME Terminal"},
    {"tilix",           "Tilix"},
    {"cool-retro-term", "CoolRetroTerm"},
};

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
    // Historical text-mode character grid (columns × rows). Not derived from
    // targetResX/Y — several machines shared a pixel resolution across
    // different text grids (e.g. IBM PC 320x200 was 40x25 on a composite/TV
    // preset but 80x25 on a direct-monitor one), so this needs its own,
    // separately sourced value. 0 = no documented fixed grid for this machine
    // (a GUI/bitmap system like the Mac 128K or Amiga, or sources conflict) —
    // stays unset rather than being back-computed from pixel size, which
    // reads as authoritative when it would actually just be a guess.
    int    targetCols          = 0;
    int    targetRows          = 0;
    // Konsole colorscheme id for this preset (see SCHEME_DEFS in the .cpp) —
    // colors are a terminal setting, exactly like the font: the shader can
    // tint a monochrome phosphor, but "light blue on C64 blue" has to come
    // from the terminal's own palette. Empty = leave the profile's scheme
    // alone (preset makes no color claim).
    QString scheme;
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
    QWidget  *buildGeneralTab();
    QWidget  *buildSetupTab();
    QGroupBox *buildPresetSection();
    QGroupBox *buildFontSection();
    QGroupBox *buildScreenSection();
    QWidget  *buildEffectsTab();
    QGroupBox *makeGroup(const QString &title, QFormLayout *&layout);
    // Wraps a tab page in a QScrollArea so it degrades gracefully on a short/
    // constrained System Settings window instead of clipping silently — every
    // tab gets this, not just the parameter-heavy Effects tab.
    static QWidget *scrollWrap(QWidget *page);
    ParamRow  *addParam(QFormLayout *fl, const QString &label,
                        double min, double max, double step,
                        const QString &key, const QString &tip);
    void      applyPreset(const PresetValues &p);
    void      markChanged();
    void      setTargetMode(TargetMode mode);
    TargetMode currentTargetMode() const;

    // ── Terminal-checklist (TargetMode::Terminals) ────────────────────────────
    // priorChecked: exec-tokens die al aangevinkt moeten starten (uit een eerder
    // opgeslagen configuratie, of de huidige widget-staat bij een rescan).
    void        rebuildTerminalList(const QMap<QString, bool> &priorChecked);
    QMap<QString, bool> currentTerminalState() const;
    QStringList checkedTerminalClasses() const;
    void        updateTerminalSummary();
    void        updatePresetInfo(const PresetValues &p);

    // ── Live preview ──────────────────────────────────────────────────────────
    // Every control change schedules a debounced save + reconfigureEffect, so a
    // slider shows its result while being dragged instead of only after Apply.
    void        schedulePreview();
    void        pushLivePreview();
    void        reconfigureKWinEffect();

    // ── Konsole font bridge ───────────────────────────────────────────────────
    // A KWin effect post-processes pixels a terminal has already rasterized, so
    // it can never choose the font those glyphs were drawn with. The only way to
    // honour a preset's font is to write it into the terminal's own profile.
    // cols/rows (0 = leave the profile's grid alone) sets Konsole's own
    // TerminalColumns/TerminalRows so the *rendered* window actually spans the
    // historical text grid, instead of an unrelated modern terminal size the
    // shader's pixel-scaling then has to quantize through arbitrarily.
    void        refreshKonsoleProfiles();
    // fontPixelSize > 0 switches the profile's font to an exact pixel size
    // (QFont pixelSize) instead of a point size — the difference between "a
    // cell of whatever physical size 14pt happens to be" and "a cell of
    // exactly native×k physical pixels", which the integer-zoom render path
    // requires. Also zeroes the profile's TerminalMargin then, so the content
    // area is exactly cols×cellW×k by rows×cellH×k with nothing added.
    bool        applyFontToKonsole(const QString &family, int pointSize,
                                    int cols, int rows, int fontPixelSize,
                                    const QString &scheme, QString *error);
    // Writes ~/.local/share/konsole/Phosphor <id>.colorscheme from SCHEME_DEFS
    // (Konsole color schemes are plain user-writable INI files — no install
    // step involved). Returns the scheme name to reference from a profile, or
    // empty when the id is unknown.
    QString     ensureColorScheme(const QString &id);
    // Largest integer zoom k that fits the preset's virtual screen on the
    // current display, or 0 when the preset has no clean sourced grid+cell.
    static int  zoomFor(const PresetValues &p);
    bool        restoreKonsoleFont(QString *error);
    void        updateFontTabInfo();

    // ── Modus-selector (bovenaan, meest prominent) ────────────────────────────
    QRadioButton *m_modeOff        = nullptr;
    QRadioButton *m_modeTerminals  = nullptr;
    QRadioButton *m_modeCustom     = nullptr;
    QButtonGroup *m_modeGroup      = nullptr;
    QWidget      *m_customRow      = nullptr;   // verborgen tenzij Custom
    QLineEdit    *m_targetClasses  = nullptr;   // vrije invoer
    QLabel       *m_customHint     = nullptr;

    QWidget      *m_terminalListWrap = nullptr;   // verborgen tenzij Terminals
    QListWidget  *m_terminalList     = nullptr;
    QPushButton  *m_rescanTerminals  = nullptr;
    QLabel       *m_terminalSummary  = nullptr;

    // ── Preset selector ───────────────────────────────────────────────────────
    QComboBox   *m_presetCombo  = nullptr;
    QPushButton *m_applyPreset  = nullptr;
    QPushButton *m_applyKWin    = nullptr;
    QLabel      *m_presetInfo   = nullptr;   // toont aanbevolen font + resolutie

    // ── Tabs + live preview ───────────────────────────────────────────────────
    QTabWidget  *m_tabs           = nullptr;
    QCheckBox   *m_livePreview    = nullptr;
    QTimer      *m_previewTimer   = nullptr;

    // ── Fonts-tab ─────────────────────────────────────────────────────────────
    QComboBox   *m_konsoleProfile = nullptr;
    QPushButton *m_applyFontBtn   = nullptr;
    QPushButton *m_restoreFontBtn = nullptr;
    QCheckBox   *m_autoApplyFont  = nullptr;
    QLabel      *m_fontStatus     = nullptr;
    QLabel      *m_fontRecommend  = nullptr;
    // Font/grid van de laatst geladen preset; leeg/0 zolang er geen preset
    // geladen is.
    QString      m_presetFont;
    int          m_presetFontSize = 14;
    int          m_presetCols     = 0;
    int          m_presetRows     = 0;
    int          m_presetFontPx   = 0;   // pixel-exact font size (cell × k), 0 = use points
    QString      m_presetScheme;         // colorscheme-id van de laatst geladen preset

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
    QSpinBox      *m_integerZoomSpin = nullptr; // 0 = uit, k>0 = bron-uitgelijnde zoom

    static constexpr const char *CFG_GROUP = "Effect-retro-terminal";
};
