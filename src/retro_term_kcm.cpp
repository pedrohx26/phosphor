// SPDX-License-Identifier: GPL-2.0-or-later
// retro-term — KCM Implementation

#include "retro_term_kcm.h"

#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>
#include <KConfigGroup>

#include <QButtonGroup>
#include <QDBusInterface>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QGuiApplication>
#include <QScreen>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QScrollArea>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QSplitter>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QDebug>

#include <algorithm>

K_PLUGIN_FACTORY_WITH_JSON(RetroTermKCMFactory,
                            "kcm_metadata.json",
                            registerPlugin<RetroTermKCM>();)

// ══════════════════════════════════════════════════════════════════════════════
// ParamRow
// ══════════════════════════════════════════════════════════════════════════════
ParamRow::ParamRow(const QString &, double min, double max,
                   double step, const QString &tip, QWidget *parent)
    : QWidget(parent), m_min(min), m_max(max)
{
    auto *hl = new QHBoxLayout(this);
    hl->setContentsMargins(0,0,0,0);
    m_slider = new QSlider(Qt::Horizontal, this);
    m_slider->setRange(0, 1000);
    m_slider->setToolTip(tip);
    m_spin = new QDoubleSpinBox(this);
    m_spin->setRange(min, max);
    m_spin->setSingleStep(step);
    m_spin->setDecimals(3);
    m_spin->setFixedWidth(80);
    m_spin->setToolTip(tip);
    hl->addWidget(m_slider, 1);
    hl->addWidget(m_spin);
    connect(m_slider, &QSlider::valueChanged, this, [this](int v) {
        const double val = m_min + (m_max - m_min) * v / 1000.0;
        QSignalBlocker b(m_spin); m_spin->setValue(val);
        Q_EMIT valueChanged(val);
    });
    connect(m_spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double val) {
        QSignalBlocker b(m_slider);
        m_slider->setValue(qRound((val - m_min) / (m_max - m_min) * 1000.0));
        Q_EMIT valueChanged(val);
    });
}
double ParamRow::value() const { return m_spin->value(); }
void   ParamRow::setValue(double v) {
    QSignalBlocker bs(m_slider), bsp(m_spin);
    m_spin->setValue(v);
    m_slider->setValue(qRound((v - m_min) / (m_max - m_min) * 1000.0));
}

// ══════════════════════════════════════════════════════════════════════════════
// Preset database
// ══════════════════════════════════════════════════════════════════════════════
// Sourcing rule for everything below, after an audit found sixteen presets
// carrying invented data (a Wyse terminal dated two years before Wyse existed,
// an amber phosphor on a green-screen Compaq, a built-in CRT on the one Sharp
// MZ that famously dropped it, and pixel rasters for character terminals that
// never published one):
//
//   * A year, resolution or phosphor here must be traceable to a real source.
//   * Where a machine published no pixel raster, targetRes stays 0/0 — pixel
//     scaling then simply stays off. A plausible cols×cell-size product is a
//     guess, and a guess that renders is indistinguishable from a fact.
//   * Where only "green" or "white" is documented, the comment says so rather
//     than naming a phosphor number the sources do not give. Several comments
//     used to attribute phosphor choices to vendors ("Motorola specificeerde
//     P4", "Kaypro specificeerde P1") — none of those attributions were real.
//
// The look of a preset is an artistic judgement and needs no citation. The
// hardware claims wrapped around it do.
//
// One artistic judgement did turn out to be a factual error, so it is written
// down here: every TV-connected home computer used to carry syncMode 1 (sine
// drift) plus heavy jitter, on the reasoning that "a TV picture is unstable".
// It isn't. The VIC-II, TMS9918, ANTIC and friends emitted proper sync pulses
// and the set locked to them; a picture that visibly wobbles horizontally is a
// failing sync circuit or a worn videotape, not a working home computer. What
// an RF/composite connection genuinely produced — colour bleeding, dot crawl,
// softness, noise — is modelled by characterSmearing, rbgShift and staticNoise
// instead, which is where it belongs. Sync artifacts are now reserved for
// machines that plausibly had them: the 1964 delay-line IBM 2260, broadcast
// teletext on a weak signal (rolling), and vector/radar beam instability.
//
// tools/check-presets.py enforces the cross-field half of this mechanically
// (a monochrome machine must not retain colour, a colour machine must not be
// tinted through a coloured phosphor, a P39 preset must actually decay slowly,
// a cell size must divide its resolution, and so on). It reports one standing
// warning, which is answered here rather than silenced: the IBM 2260 carries a
// long persistence (0.60) with the neutral P4 white phosphor. Its actual
// phosphor is undocumented — the earlier audit marked it unverifiable — but a
// 1964 display refreshed from a sonic delay line needed a slow decay to avoid
// flicker, so the persistence is deliberate while the white tint stays neutral
// rather than claiming the green P39 the shader would otherwise imply.
void RetroTermKCM::buildPresets()
{
    auto p = [&](PresetValues pv) { m_presets.append(pv); };

    p({"Default (amber)","—",1,0.05,7000,0.10,0.25,0.35,0.04,1,0.35,0.50,0.55,0.20,0.50,0.80,0.08,0.10,0,0.05,0.08,0.00,0.20,0.20,0.10,0.08,0.20,true,8.0,true,2.5,"VT323",16,0.0,0.0,0,0,"mono"});
    p({"IBM 2260 (1964)","1964 — Vroege IBM-mainframeterminal, 80×12",2,0.55,8500,0.60,0.45,0.65,0.12,0,0.35,0.50,0.80,0.45,0.42,0.90,0.18,0.08,1,0.20,0.22,0.00,0.00,0.00,0.15,0.30,0.50,true,15.0,false,4.0,"Glass TTY VT220",16,0.0,0.0,80,12,"mono"});
    p({"DEC GT40 (1972)","1972 — Vectorterminal PDP-11, P39",3,0.40,7800,0.80,0.20,0.55,0.08,0,0.35,0.50,0.85,0.60,0.38,0.85,0.10,0.08,0,0.08,0.15,0.00,0.05,0.00,0.06,0.15,0.35,true,12.0,false,3.5,"VT323",18,1024.0,768.0,0,0,"mono"});
    p({"DEC VT100 (1978)","1978 — Dé referentieterminal",0,0.12,8000,0.18,0.22,0.38,0.05,1,0.40,0.55,0.52,0.22,0.52,0.82,0.06,0.08,0,0.05,0.07,0.00,0.05,0.00,0.05,0.10,0.22,true,9.0,false,2.5,"VT323",18,800.0,240.0,80,24,"mono"});
    p({"IBM 3270 (1971)","1971 — IBM-mainframe blokmodus",0,0.18,8200,0.25,0.28,0.42,0.06,1,0.35,0.60,0.48,0.18,0.50,0.88,0.05,0.06,0,0.04,0.05,0.00,0.04,0.00,0.04,0.08,0.35,true,10.0,false,2.8,"Px437 IBM 3270pc",16,0.0,0.0,80,24,"mono"});
    p({"Wyse WY-50 (1983)","1983 — UNIX-werkterminal, 14\" groen",0,0.08,8400,0.14,0.18,0.32,0.04,1,0.38,0.65,0.55,0.20,0.55,0.85,0.04,0.06,0,0.03,0.05,0.00,0.04,0.00,0.04,0.08,0.20,true,8.0,false,2.5,"Px437 Wyse700b",16,800.0,312.0,80,24,"mono"});
    p({"Militair Radar (1958)","1958 — SAGE AN/FSQ-7, 19\" P14 nalichtend",3,0.50,7000,0.90,0.15,0.70,0.10,0,0.35,0.50,0.90,0.70,0.35,0.95,0.15,0.12,0,0.10,0.18,0.00,0.04,0.00,0.06,0.20,0.55,true,20.0,false,5.0,"Share Tech Mono",16,0.0,0.0,0,0,"mono"});
    p({"Apple II (1977)","1977 — NTSC-TV, composite",2,0.30,6500,0.22,0.38,0.50,0.08,1,0.55,0.35,0.65,0.28,0.48,0.78,0.14,0.02,0,0.12,0.14,0.00,0.90,0.35,0.18,0.35,0.28,true,10.0,true,3.0,"Print Char 21",16,280.0,192.0,40,24,"apple2"});
    p({"Commodore 64 (1982)","1982 — VIC-II, PAL-TV",2,0.22,6200,0.18,0.35,0.45,0.07,1,0.50,0.38,0.60,0.25,0.50,0.80,0.12,0.02,0,0.10,0.12,0.00,0.90,0.40,0.14,0.28,0.25,true,9.0,true,2.8,"C64 Pro Mono",14,320.0,200.0,40,25,"c64"});
    p({"ZX Spectrum (1982)","1982 — PAL-TV, attribuutcellen",2,0.25,6300,0.16,0.38,0.48,0.08,1,0.52,0.33,0.62,0.24,0.52,0.78,0.13,0.02,0,0.11,0.13,0.00,0.90,0.45,0.16,0.30,0.20,true,9.0,true,2.8,"VT323",14,256.0,192.0,32,24,"zx"});
    p({"BBC Micro (1981)","1981 — Britse schoolcomputer",2,0.20,6600,0.16,0.30,0.44,0.07,1,0.50,0.38,0.60,0.22,0.50,0.80,0.10,0.02,0,0.09,0.11,0.00,0.90,0.40,0.14,0.25,0.22,true,9.0,true,2.8,"Bedstead",16,320.0,256.0,40,32,"teletext"});
    p({"Atari 400/800 (1979)","1979 — ANTIC/CTIA, NTSC-TV",2,0.26,6400,0.19,0.36,0.47,0.07,1,0.52,0.35,0.62,0.26,0.50,0.79,0.12,0.02,0,0.10,0.13,0.00,0.90,0.38,0.15,0.30,0.24,true,9.0,true,2.8,"Atari Classic",16,320.0,192.0,40,24,"atari8"});
    p({"IBM PC MDA (1981)","1981 — IBM 5151, P39",3,0.15,7500,0.50,0.20,0.40,0.06,1,0.42,0.60,0.58,0.28,0.50,0.88,0.05,0.07,0,0.04,0.06,0.00,0.04,0.00,0.05,0.10,0.30,true,8.0,false,2.5,"PxPlus IBM MDA",16,720.0,350.0,80,25,"mono"});
    p({"IBM PC CGA (1981)","1981 — CGA op composite/TV",2,0.20,7000,0.15,0.25,0.38,0.06,1,0.45,0.50,0.55,0.22,0.52,0.83,0.08,0.10,0,0.06,0.08,0.00,0.90,0.45,0.10,0.15,0.22,true,8.0,true,2.5,"PxPlus IBM CGA",16,320.0,200.0,40,25,"cga"});
    p({"IBM PC EGA (1984)","1984 — IBM 5154, 16 kleuren",2,0.14,7200,0.12,0.20,0.32,0.05,1,0.38,0.58,0.50,0.18,0.54,0.84,0.06,0.08,0,0.04,0.06,0.00,0.90,0.38,0.08,0.12,0.18,true,8.0,true,2.5,"PxPlus IBM EGA 8x14",14,640.0,350.0,80,25,"cga"});
    p({"Tandy 1000 (1984)","1984 — Verbeterde CGA",2,0.22,6800,0.15,0.28,0.42,0.07,1,0.48,0.42,0.58,0.22,0.50,0.80,0.10,0.02,0,0.08,0.10,0.00,0.90,0.48,0.12,0.20,0.24,true,9.0,true,2.8,"PxPlus Tandy1K-II 200L",16,320.0,200.0,40,25,"cga"});
    p({"IBM PS/2 VGA (1987)","1987 — De DOS-standaard",2,0.10,7400,0.10,0.15,0.28,0.04,1,0.32,0.62,0.45,0.15,0.55,0.85,0.05,0.06,0,0.03,0.05,0.00,0.90,0.35,0.07,0.10,0.15,true,7.0,true,2.2,"PxPlus IBM VGA 9x16",16,720.0,400.0,80,25,"cga"});
    p({"Amiga 500 (1987)","1987 — PAL-TV of 1084S",2,0.15,6800,0.14,0.25,0.38,0.06,1,0.48,0.44,0.55,0.20,0.52,0.81,0.08,0.10,0,0.05,0.08,0.00,0.90,0.42,0.10,0.18,0.18,true,8.0,true,2.5,"Topaz a500a1000a2000",14,320.0,256.0,0,0,"wb13"});
    p({"Amiga WorkBench 2 (1990)","1990 — 1084S RGB-monitor",2,0.10,7000,0.10,0.20,0.30,0.05,1,0.40,0.52,0.48,0.16,0.56,0.83,0.06,0.08,0,0.04,0.06,0.00,0.90,0.38,0.08,0.14,0.14,true,7.0,true,2.2,"Topaz a500a1000a2000",14,640.0,256.0,0,0,"wb2"});
    p({"Apple Macintosh 128K (1984)","1984 — 9-inch b/w CRT",2,0.35,9000,0.08,0.15,0.55,0.12,2,0.20,0.70,0.35,0.10,0.60,0.92,0.03,0.04,0,0.02,0.04,0.00,0.00,0.00,0.04,0.05,0.40,true,6.0,false,2.0,"Silkscreen",12,512.0,342.0,0,0,"paper"});
    // Font: DejaVu Sans Mono — "Lucida Console" is a Microsoft font with no
    // free-redistributable source, so install-fonts.sh never had anything to
    // fetch for it; DejaVu Sans Mono ships in every distro's base fonts package
    // (already present on both test machines) and reads the same UNIX-console way.
    p({"NeXT Station (1990)","1990 — 1120×832 grijs",2,0.08,8000,0.06,0.08,0.22,0.05,0,0.35,0.50,0.32,0.08,0.62,0.88,0.02,0.03,0,0.02,0.03,0.00,0.00,0.00,0.03,0.04,0.12,true,5.0,false,1.8,"DejaVu Sans Mono",13,1120.0,832.0,0,0,"paper"});
    p({"SVGA Multisync (1992)","1992 — 800×600, shadow mask",2,0.06,7600,0.06,0.10,0.20,0.04,1,0.22,0.72,0.35,0.10,0.60,0.87,0.03,0.04,0,0.02,0.04,0.00,0.90,0.30,0.05,0.06,0.10,true,6.0,true,2.0,"Terminus",14,800.0,600.0,0,0,"cga"});
    p({"Sony Trinitron (1997)","1997 — Aperture-grille beeldbuis",2,0.05,7800,0.05,0.04,0.18,0.04,3,0.18,0.78,0.30,0.08,0.62,0.88,0.02,0.03,0,0.02,0.03,0.00,0.90,0.28,0.04,0.04,0.08,true,5.0,true,1.8,"Terminus",14,1024.0,768.0,0,0,"cga"});
    p({"Teletext / Ceefax (1974)","1974 — PAL-TV, 8 kleuren",2,0.30,6200,0.20,0.40,0.52,0.09,1,0.62,0.28,0.70,0.30,0.46,0.76,0.20,0.1,2,0.18,0.20,0.00,0.90,0.55,0.22,0.40,0.28,true,12.0,true,3.5,"Bedstead",16,480.0,240.0,40,24,"teletext"});
    p({"Minimaal (laag GPU)","— Subtiel, min. belasting",1,0.05,7000,0.10,0.10,0.15,0.04,1,0.20,0.50,0.20,0.08,0.55,0.85,0.00,0.00,0,0.00,0.00,0.00,0.20,0.20,0.00,0.00,0.10,false,8.0,false,2.5,"Terminus",14,0.0,0.0,0,0});

    // ── Nieuwe presets: echte hardware, geverifieerde fonts ──────────────────
    //
    // Bronverantwoording per waarde:
    //
    // phosphorType:     Fabrikantsdatasheet of gedocumenteerde historiografie
    // colorTemperature: Categorie-schatting (mono-prof ~8500K, TV ~6500K, RGB ~7500K)
    // screenCurvature:  Afgeleid van schermdiameter (9"≈0.40, 12"≈0.22, 14"≈0.15, flat≈0.04)
    // scanlinesIntensity: Visueel afgesteld op foto's/video's van werkende hardware
    // bloom/noise/jitter: Artistieke keuze, representeert "goed onderhouden" conditie
    // chromaColor:      0.0 = monochroomscherm, 0.6+ = kleurscherm

    // Commodore PET 2001 (1977)
    // Hardware: Motorola 6845 CRTC, ingebouwde 9" fosfor-CRT
    // Fosfor: P4 wit — de oorspronkelijke 2001 had inderdaad een wit scherm;
    //         groen kwam pas met de 2001-N (1979). Motorola leverde de CRTC,
    //         niet de monitor: die toeschrijving stond hier eerder ten onrechte
    // Curvature: 0.40 — kleine 9" bolronde buis, gelijkend aan Mac 128K maar ouder
    // Scans: 0.52 — 40×25 tekenmodus, 8×8 pixels per teken, duidelijke scanlijnen
    // Chroma: 0.00 — monochroomscherm, geen kleur
    // Font: Pet Me 2Y — pixel-perfecte recreatie van Commodore PET character ROM
    //        https://www.kreativekorp.com/swdownload/fonts/retro/petme.zip (gratis)
    p({"Commodore PET 2001 (1977)","1977 — Eerste Commodore, ingebouwde 9\" wit-fosfor CRT",
       2,0.35,8500,0.12, 0.40,0.55,0.10, 1,0.52,0.58,
       0.65,0.18,0.55,0.90, 0.06,0.08,0,0.04,0.06,0.00,
       0.00,0.00,0.05,0.12,0.38, true,11.0,false,3.0, "Pet Me 2Y",16,320.0,200.0,40,25,"mono"});

    // TRS-80 Model I (1977)
    // Hardware: discrete TTL-videoschakeling, composite naar gewone TV. Niet de
    //           MC6847 — die zat in de CoCo hieronder, niet in de Model I
    // Fosfor: P4 via composite TV — warm-wit door NTSC-encoding
    // Curvature: 0.35 — consumentenTV, matige bolrondheid
    // Smearing 0.28: composite artefacten waren berucht op de TRS-80
    // Chroma: 0.10 — bijna monochroom maar composite geeft lichte kleurtint
    // Font: Another Man's Treasure MIA — Model I character ROM
    //        https://www.kreativekorp.com/swdownload/fonts/retro/amtreasure.zip (gratis)
    p({"TRS-80 Model I (1977)","1977 — Tandy/RadioShack, composite naar TV, uppercase-only",
       2,0.28,6800,0.14, 0.35,0.48,0.08, 1,0.52,0.35,
       0.58,0.18,0.50,0.80, 0.14,0.02,0,0.10,0.12,0.00,
       0.00,0.00,0.14,0.28,0.22, true,9.0,false,2.8, "Another Mans Treasure MIA Raw",16,384.0,192.0,64,16,"mono"});

    // TRS-80 Color Computer (1980)
    // Hardware: MC6847, composite naar TV, later Tandy CM-2 monitor
    // Fosfor: P4 via composite, maar MC6847 had groen/zwart als standaard kleurpaar
    // Curvature: 0.35 — consumentenTV
    // Chroma: 0.60 — kleurmode was het onderscheidende kenmerk van de CoCo
    // Font: Hot CoCo — MC6847 character ROM voor CoCo I & II
    //        https://www.kreativekorp.com/swdownload/fonts/retro/hotcoco.zip (gratis)
    p({"TRS-80 Color Computer (1980)","1980 — CoCo, MC6847, composite kleur-TV",
       2,0.25,6500,0.16, 0.35,0.46,0.08, 1,0.50,0.32,
       0.60,0.22,0.48,0.78, 0.13,0.02,0,0.11,0.13,0.00,
       0.90,0.45,0.15,0.30,0.22, true,9.0,true,2.8, "Hot CoCo",16,256.0,192.0,32,16,"coco"});

    // Kaypro II (1982)
    // Hardware: ingebouwde 9" green-phosphor CRT, Z80, CP/M
    // Fosfor: groen — dat het specifiek P1 was, is niet gedocumenteerd; alleen
    //         "9-inch groen" staat vast
    // Curvature: 0.42 — kleine 9" buis, meer dan een 12" monitor
    // Scans: 0.44 — 80×24 tekenmodus, redelijk zichtbare scanlijnen
    // Chroma: 0.00 — monochroomscherm, geen kleur
    // Font: PxPlus Kaypro 2000 — authentieke Kaypro character ROM, int10h pack
    //        https://int10h.org/oldschool-pc-fonts/download/ (CC BY-SA 4.0)
    p({"Kaypro II (1982)","1982 — Draagbare CP/M, ingebouwde 9\" groene CRT",
       0,0.14,8100,0.16, 0.42,0.52,0.07, 1,0.44,0.60,
       0.58,0.22,0.50,0.86, 0.05,0.07,0,0.04,0.06,0.00,
       0.00,0.00,0.04,0.08,0.25, true,9.0,false,2.8, "Px437 Kaypro2K G",16,640.0,192.0,80,24,"mono"});

    // Compaq Portable (1982)
    // Hardware: ingebouwde 9" groene CRT, eerste IBM-compatibele draagbare
    // Fosfor: groen nalichtend — de Portable had géén amberbuis; dat is een
    //         hardnekkig misverstand, mogelijk door verwarring met latere
    //         Compaq-desktopmonitoren
    // Curvature: 0.40 — kleine 9" spherische buis
    // Chroma: 0.00 — monochroom scherm
    // Resolutie: CGA-compatibel; de exacte interne raster van de ingebouwde
    //            buis is niet gepubliceerd, 640×200 is de CGA-modus zelf
    // Font: Px437 Compaq Port3 — Compaq Portable BIOS-font, int10h pack
    //        https://int10h.org/oldschool-pc-fonts/download/ (CC BY-SA 4.0)
    p({"Compaq Portable (1982)","1982 — Eerste IBM-compatibele draagbare, 9\" groen",
       0,0.12,7600,0.20, 0.40,0.50,0.08, 1,0.42,0.55,
       0.60,0.25,0.52,0.88, 0.06,0.08,0,0.04,0.06,0.00,
       0.00,0.00,0.05,0.10,0.28, true,9.0,false,2.8, "Px437 Compaq Port3",16,640.0,200.0,80,25,"mono"});

    // DEC Rainbow 100 (1982)
    // Hardware: VR201 monitor, 80×24, CP/M en DOS
    // Fosfor: groen — de VR201-B is een gedocumenteerde groene variant; welk
    //         fosfornummer DEC daarvoor gebruikte is niet gepubliceerd
    // Resolutie: 800×240 is afgeleid uit 80×24 met VT100-celmaten, geen
    //            gepubliceerd cijfer — vandaar dat het klopt als benadering
    //            maar niet als bronvermelding moet gelden
    // Curvature: 0.18 — 12" monitor, minder dan 9", DEC-kwaliteitsglas
    // Scans: 0.38 — hogere kwaliteit dan IBM CGA, scherpere rasterweergave
    // Chroma: 0.00 — monochroomscherm
    // Font: PxPlus DEC Rainbow — authentieke Rainbow BIOS font, int10h pack
    //        https://int10h.org/oldschool-pc-fonts/download/ (CC BY-SA 4.0)
    p({"DEC Rainbow 100 (1982)","1982 — DEC's CP/M+DOS hybride, VR201 groene monitor",
       0,0.10,8200,0.14, 0.18,0.35,0.05, 1,0.38,0.62,
       0.52,0.18,0.55,0.86, 0.04,0.05,0,0.03,0.04,0.00,
       0.00,0.00,0.04,0.08,0.18, true,8.0,false,2.5, "PxPlus Rainbow100 re.40",16,800.0,240.0,80,24,"mono"});

    // TeleVideo 925 (1982)
    // Hardware: 12" groene CRT, 80×24, UNIX/CP/M kantoor-terminal
    // Fosfor: P31 groen — zo staat het in de TeleVideo 925 User's Guide (jan 1983),
    //         "12-inch non-glare P31 green". Dichtstbijzijnde optie hier is het
    //         gewone groene fosfor; P31 is korter nalichtend dan P39
    // Curvature: 0.20 — 12" monitor
    // Chroma: 0.00 — monochroomscherm
    // Resolutie: geen gepubliceerde pixelraster; 80×24 tekens is wat vaststaat,
    //            dus geen pixel-scaling in plaats van een verzonnen getal
    // Font: Px437 Wyse700b — de int10h-pack bevat geen TeleVideo-font; dit is
    //        het dichtstbijzijnde tijdgenoot-terminalfont uit dezelfde klasse
    //        https://int10h.org/oldschool-pc-fonts/download/ (CC BY-SA 4.0)
    p({"TeleVideo TVI-925 (1982)","1982 — Populaire UNIX-terminal, 12\" P31 groen",
       0,0.07,8300,0.12, 0.20,0.34,0.04, 1,0.36,0.66,
       0.50,0.16,0.56,0.87, 0.04,0.05,0,0.03,0.04,0.00,
       0.00,0.00,0.04,0.07,0.18, true,8.0,false,2.2, "Px437 Wyse700b",16,0.0,0.0,80,24,"mono"});

    // Apple Lisa (1983)
    // Hardware: 12" monochrome CRT, 720×364, eerste GUI-computer van Apple.
    //           Welke fabrikant de buis leverde is niet gedocumenteerd; hier
    //           stond eerder "Sony", wat nergens te staven is
    // Fosfor: wit (P4 als aanname bij een zwart-witscherm)
    // Curvature: 0.14 — vlakke buis, weinig bolrondheid voor 1983
    // Scans: mode 0 (geen scanlines) — hoge resolutie voor zijn tijd, amper zichtbaar
    // Chroma: 0.00 — monochroom zwart-wit scherm
    // Font: LisaTerminal Paper — LisaTerminal bitmap font
    //        https://www.kreativekorp.com/swdownload/fonts/retro/lisa1.zip (gratis)
    p({"Apple Lisa (1983)","1983 — Eerste Apple GUI-computer, 12\" b/w CRT",
       2,0.10,8800,0.06, 0.14,0.38,0.08, 0,0.15,0.70,
       0.38,0.08,0.62,0.91, 0.02,0.03,0,0.02,0.03,0.00,
       0.00,0.00,0.03,0.04,0.30, true,6.0,false,2.0, "LisaTerminal Paper Raw",13,720.0,364.0,0,0,"paper"});

    // Amstrad PC1512 (1986)
    // Hardware: geleverd met PC-CD (kleur) of PC-MD (mono) monitor. Let op: de
    //           CTM640 en GT65 zijn monitoren van de Amstrad CPC-homecomputers,
    //           niet van de PC1512 — die verwarring stond hier eerder
    // Fosfor: P4 via shadow mask voor de PC-CD kleurenmonitor
    // Curvature: 0.22 — 14" CGA-klasse monitor
    // Chroma: 0.70 — kleurenmonitor, 640×200 in 16 kleuren
    // Font: PxPlus Amstrad PC — int10h pack
    //        https://int10h.org/oldschool-pc-fonts/download/ (CC BY-SA 4.0)
    p({"Amstrad PC1512 (1986)","1986 — Goedkope Britse IBM-kloon, PC-CD kleurenmonitor",
       2,0.16,6800,0.12, 0.22,0.36,0.06, 1,0.42,0.48,
       0.52,0.20,0.52,0.82, 0.07,0.09,0,0.06,0.08,0.00,
       0.90,0.42,0.08,0.14,0.18, true,8.0,true,2.5, "PxPlus Amstrad PC-2y",16,640.0,200.0,80,25,"cga"});

    // Atari ST — SM124 mono (1985)
    // Hardware: SM124 monochroom monitor, 640×400, P4 wit fosfor
    // Fosfor: P4 — Atari SM124 gebruikt P4 wit
    // Curvature: 0.08 — 12" vlak glas, één van de scherpste monitors van die periode
    // Scans: mode 1 maar laag (0.18) — hoge resolutie, amper zichtbare scanlijnen
    // Chroma: 0.00 — monochroom scherm
    // Font: Project Jason — Atari ST GEM system font
    //        https://www.kreativekorp.com/swdownload/fonts/retro/projason.zip (gratis)
    p({"Atari ST SM124 (1985)","1985 — Atari ST mono, SM124 wit fosfor, 640×400",
       2,0.08,8600,0.06, 0.08,0.28,0.05, 1,0.18,0.75,
       0.35,0.08,0.60,0.90, 0.02,0.03,0,0.02,0.03,0.00,
       0.00,0.00,0.03,0.04,0.15, true,6.0,false,1.8, "Project Jason Small",14,640.0,400.0,80,25,"paper"});

    // NEC APC III (1984)
    // Hardware: Japanse professionele PC, 14" monochrome monitor, 640×400
    // Fosfor: groen — dat de serie specifiek P1 gebruikte is nergens
    //         gedocumenteerd; alleen "groen" staat vast
    // Curvature: 0.16 — professionele monitor, weinig bolrondheid
    // Scans: 0.32 — 640×400 is hoog voor de tijd, scanlijnen minder zichtbaar
    // Chroma: 0.00 — monochroomscherm, professioneel gebruik
    // Font: Px437 NEC APC3 8x16 — int10h pack
    //        https://int10h.org/oldschool-pc-fonts/download/ (CC BY-SA 4.0)
    p({"NEC APC III (1984)","1984 — Japanse professionele PC, 14\" groen, 640×400",
       0,0.08,8400,0.10, 0.16,0.30,0.04, 1,0.32,0.68,
       0.46,0.14,0.58,0.88, 0.03,0.04,0,0.02,0.04,0.00,
       0.00,0.00,0.04,0.06,0.14, true,7.0,false,2.0, "Px437 NEC APC3 8x16",16,640.0,400.0,80,25,"mono"});

    // HP 150 Touchscreen (1983)
    // Hardware: ingebouwde 9" CRT, eerste touchscreen-PC (infraroodraster)
    // Fosfor: monochroom wit; dat HP hier expliciet P4 voor specificeerde is
    //         niet terug te vinden, P4 is hier de aanname die bij "wit" past
    // Resolutie: 512×390 bitmap (80×27 tekst) — dit stond eerder op 640×256,
    //            een resolutie die de 150 niet had
    // Curvature: 0.22 — 9" buis
    // Chroma: 0.00 — monochroom wit scherm
    // Font: PxPlus HP 150 re. — int10h pack
    //        https://int10h.org/oldschool-pc-fonts/download/ (CC BY-SA 4.0)
    p({"HP 150 Touchscreen (1983)","1983 — HP's eerste touchscreen-PC, 9\" b/w CRT",
       2,0.08,8700,0.07, 0.22,0.40,0.06, 1,0.40,0.62,
       0.48,0.14,0.60,0.89, 0.03,0.04,0,0.02,0.03,0.00,
       0.00,0.00,0.04,0.06,0.24, true,7.0,false,2.0, "PxPlus HP 150 re.",16,512.0,390.0,80,27,"mono"});

    // Apple IIgs (1986)
    // Hardware: Apple RGB monitor A2M6014, shadow mask, 320×200 of 640×200
    // Fosfor: P4 via shadow mask RGB
    // Kleurtemperatuur: 7400K — Apple RGB-monitor was goed gekalibreerd voor die tijd
    // Curvature: 0.18 — 13" Apple-monitor, redelijk vlak voor consumentenkwaliteit
    // Chroma: 0.75 — kleurenmonitor, IIgs had rijke kleurpalette (4096 kleuren)
    // Font: Shaston 320 — Apple IIgs GS/OS system font
    //        https://www.kreativekorp.com/swdownload/fonts/retro/shaston.zip (gratis)
    p({"Apple IIgs (1986)","1986 — Apple IIgs, RGB-monitor, 4096 kleuren",
       2,0.08,7400,0.08, 0.18,0.32,0.05, 1,0.36,0.55,
       0.48,0.14,0.56,0.85, 0.04,0.05,0,0.03,0.05,0.00,
       0.90,0.45,0.07,0.10,0.14, true,7.0,true,2.0, "Shaston 320",14,280.0,192.0,40,24,"iigs"});

    // Sharp MZ-700 (1982)
    // Hardware: géén ingebouwde monitor — dat was juist dé verandering ten
    //           opzichte van de MZ-80K/MZ-80B, die wél een vaste buis hadden.
    //           De MZ-700 sloot aan op een gewone TV of externe monitor.
    //           Hier stond eerder "ingebouwde 12\" monitor (MZ-1D05)"
    // Fosfor: P4 — kleuren-TV via composite
    // Curvature: 0.36 — consumenten-TV in plaats van een monitor
    // Font: Mizuno — Sharp MZ character ROM
    //        https://www.kreativekorp.com/swdownload/fonts/retro/mizuno.zip (gratis)
    p({"Sharp MZ-700 (1982)","1982 — Japanse Sharp, externe TV/monitor",
       2,0.16,6600,0.12, 0.36,0.44,0.07, 1,0.46,0.44,
       0.54,0.18,0.52,0.83, 0.09,0.02,0,0.06,0.08,0.00,
       0.90,0.32,0.10,0.16,0.22, true,8.0,true,2.5, "Mizuno",14,320.0,200.0,40,25,"mz700"});

    // Mattel Aquarius (1983)
    // Hardware: composite naar TV, Zilog Z80, Mattel's mislukte home computer
    // Fosfor: P4 via composite TV
    // Curvature: 0.36 — consumentenTV
    // Smearing: 0.35, noise: 0.18 — slechte composite-kwaliteit, dit apparaat stond
    //   bekend als "de computer voor de computer-generatie" maar was technisch matig
    // Font: Antiquarius — Mattel Aquarius character ROM
    //        https://www.kreativekorp.com/swdownload/fonts/retro/aq2.zip (gratis)
    p({"Mattel Aquarius (1983)","1983 — Mattel's mislukte home computer, composite-TV",
       2,0.22,6400,0.18, 0.36,0.48,0.08, 1,0.54,0.30,
       0.62,0.22,0.48,0.78, 0.18,0.02,0,0.14,0.14,0.00,
       0.90,0.38,0.16,0.35,0.24, true,10.0,true,3.0, "Antiquarius",16,320.0,192.0,40,24,"aquarius"});

    // Commodore VIC-20 (1981)
    // Hardware: MOS 6560/6561 (VIC), composite naar TV
    // Font: C64 Pro Mono — shares Commodore character ROM lineage
    // Resolution: 176x184 — VIC-20 high-res graphics mode (22x23 text cells at
    // 8x8 px); missing from the original entry, which left pixel scaling off.
    p({"Commodore VIC-20 (1981)","1981 — First color Commodore home computer",
       2,0.28,6000,0.20, 0.40,0.52,0.09, 1,0.58,0.30,
       0.68,0.30,0.46,0.75, 0.16,0.02,0,0.15,0.16,0.00,
       0.90,0.42,0.20,0.38,0.22, true,10.0,true,3.0, "C64 Pro Mono",16,176.0,184.0,22,23,"vic20"});

    // MSX (1983)
    // Hardware: TMS9918 video, composite naar TV
    // Font: VT323 — similar to common MSX screen fonts
    // Resolution: 256x192 — TMS9918 standard screen mode, shared by every
    // TMS9918-based system (MSX, ColecoVision, SG-1000); missing from the
    // original entry, which left pixel scaling off.
    p({"MSX (1983)","1983 — Japanese home computer standard (Sony, Philips, Panasonic)",
       2,0.24,6500,0.18, 0.35,0.46,0.07, 1,0.52,0.36,
       0.62,0.24,0.50,0.79, 0.12,0.02,0,0.10,0.12,0.00,
       0.90,0.42,0.15,0.28,0.22, true,9.0,true,2.8, "VT323",14,256.0,192.0,40,24,"msx"});

    // Sun-3 Workstation (1985)
    // Hardware: bwtwo monochrome framebuffer, 19" monochrome CRT. Niet "GX":
    //           dat is een SBus-kaart uit het SPARC-tijdperk (1989+) en bestond
    //           in 1985 nog niet — die naam stond hier eerder
    // Font: DejaVu Sans Mono — see the NeXT Station comment above; same
    //       Lucida-Console-is-not-redistributable reasoning applies here.
    // Resolution: 1152x900 — de standaard Sun-framebuffergrootte van dit tijdperk
    p({"Sun-3 Workstation (1985)","1985 — UNIX workstation, bwtwo framebuffer",
       2,0.12,8200,0.08, 0.10,0.25,0.04, 1,0.28,0.68,
       0.40,0.12,0.58,0.86, 0.03,0.04,0,0.02,0.04,0.00,
       0.00,0.00,0.04,0.06,0.12, true,6.0,false,2.0, "DejaVu Sans Mono",14,1152.0,900.0,0,0,"paper"});

}

// ══════════════════════════════════════════════════════════════════════════════
// UI helpers
// ══════════════════════════════════════════════════════════════════════════════
QGroupBox *RetroTermKCM::makeGroup(const QString &title, QFormLayout *&fl)
{
    auto *gb = new QGroupBox(title);
    fl = new QFormLayout(gb);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    return gb;
}

// ── Konsole colorschemes per preset ───────────────────────────────────────────
// Colors are a terminal setting, like the font: the shader tints monochrome
// phosphors, but a color machine's palette must come from Konsole's own
// colorscheme. These are written as user files (~/.local/share/konsole/
// Phosphor <id>.colorscheme) on demand — Konsole colorschemes are plain INI,
// no installation step exists or is needed.
//
// Sourcing rule (same bar as the presets themselves): a palette here must be
// traceable to documentation — chip datasheets, the Pepto/colodore C64
// measurements, the RGBI standard, machine manuals. "mono" is the exception:
// it's not a historical claim but a normalization, forcing white-on-black so
// the shader's phosphor tint starts from a neutral source instead of from
// whatever colorful scheme the user's profile happened to have.
struct SchemeColor { int r, g, b; };
struct SchemeDef {
    const char *id;          // preset refers to this
    const char *name;        // file/display name: "Phosphor <name>"
    SchemeColor bg, fg;
    SchemeColor ansi[16];    // ANSI 0-7 normal + 8-15 intense
};
// Palettes fact-checked per machine (VICE colodore .vpl files for the
// Commodore machines, the CGA RGBI standard, Apple IIgs Technote #63, the
// TMS9918 table, MAME's MC6847/GTIA tables, machine manuals for boot colors —
// full source list in the commit that introduced each entry). ANSI-slot
// mapping onto a machine palette is a judgment call by hue; the boot fg/bg
// pairs are the sourced facts. Machines whose text mode was effectively
// two-tone (Atari GRAPHICS 0, Workbench's four colors) map most ANSI slots
// onto those same few colors on purpose: that IS what the machine showed.
static const SchemeDef SCHEME_DEFS[] = {
    // Normalization, not history: white on black so the shader's phosphor
    // tint starts from a neutral source. ANSI stays CGA for TUI usability.
    {"mono", "Mono",
     {0,0,0}, {255,255,255},
     {{0,0,0},{170,0,0},{0,170,0},{170,85,0},{0,0,170},{170,0,170},{0,170,170},{170,170,170},
      {85,85,85},{255,85,85},{85,255,85},{255,255,85},{85,85,255},{255,85,255},{85,255,255},{255,255,255}}},
    // Black-on-white GUI machines (Mac 128K, Lisa, Atari ST mono, NeXT, Sun-3
    // bwtwo — all verified as dark-on-light, several were assumed wrong before).
    {"paper", "Paper",
     {255,255,255}, {0,0,0},
     {{0,0,0},{170,0,0},{0,170,0},{170,85,0},{0,0,170},{170,0,170},{0,170,170},{170,170,170},
      {85,85,85},{255,85,85},{85,255,85},{255,255,85},{85,85,255},{255,85,255},{85,255,255},{255,255,255}}},
    // IBM CGA/EGA/VGA text mode: light grey on black + the RGBI sixteen
    // (https://en.wikipedia.org/wiki/Color_Graphics_Adapter#Color_palette).
    {"cga", "DOS CGA",
     {0,0,0}, {170,170,170},
     {{0,0,0},{170,0,0},{0,170,0},{170,85,0},{0,0,170},{170,0,170},{0,170,170},{170,170,170},
      {85,85,85},{255,85,85},{85,255,85},{255,255,85},{85,85,255},{255,85,255},{85,255,255},{255,255,255}}},
    // Commodore 64 boot: light blue on blue (colors 14/6). Colodore palette
    // as shipped in VICE (data/C64/colodore.vpl).
    {"c64", "Commodore 64",
     {39,36,196}, {104,100,255},
     {{0,0,0},{150,40,46},{65,185,54},{159,72,21},{39,36,196},{159,45,173},{91,214,206},{174,174,174},
      {71,71,71},{218,95,102},{145,255,132},{239,243,71},{104,100,255},{159,45,173},{91,214,206},{255,255,255}}},
    // VIC-20 boot: blue text on white, cyan border — register 36879 powers up
    // at 27 (VIC-20 Programmer's Reference). Colodore VIC palette (VICE).
    {"vic20", "Commodore VIC-20",
     {255,255,255}, {37,35,144},
     {{0,0,0},{109,35,39},{126,218,117},{164,100,59},{37,35,144},{142,60,151},{160,254,248},{255,255,255},
      {0,0,0},{242,167,171},{215,255,206},{255,255,134},{157,154,255},{255,180,255},{219,255,255},{255,255,255}}},
    // ZX Spectrum boot: black ink on white paper (Sinclair BASIC ch.16).
    // Conventional 0xD7/0xFF approximation of the PAL colors.
    {"zx", "ZX Spectrum",
     {215,215,215}, {0,0,0},
     {{0,0,0},{215,0,0},{0,215,0},{215,215,0},{0,0,215},{215,0,215},{0,215,215},{215,215,215},
      {0,0,0},{255,0,0},{0,255,0},{255,255,0},{0,0,255},{255,0,255},{0,255,255},{255,255,255}}},
    // Teletext/BBC MODE 7: white on black, the saturated 3-bit RGB eight
    // (SAA5050 — Level 1 teletext can't even set black as a foreground).
    {"teletext", "Teletext",
     {0,0,0}, {255,255,255},
     {{0,0,0},{255,0,0},{0,255,0},{255,255,0},{0,0,255},{255,0,255},{0,255,255},{255,255,255},
      {0,0,0},{255,0,0},{0,255,0},{255,255,0},{0,0,255},{255,0,255},{0,255,255},{255,255,255}}},
    // Atari 400/800 GRAPHICS 0: light blue text on blue (registers 709/710
    // power-on defaults $CA/$94, Mapping the Atari app.5; RGB via MAME's
    // GTIA palette). Two-tone mode — ANSI slots deliberately collapse to it.
    {"atari8", "Atari 8-bit",
     {17,81,155}, {119,183,255},
     {{17,81,155},{119,183,255},{119,183,255},{119,183,255},{119,183,255},{119,183,255},{119,183,255},{119,183,255},
      {17,81,155},{119,183,255},{119,183,255},{119,183,255},{119,183,255},{119,183,255},{119,183,255},{255,255,255}}},
    // Apple II 40-col text: white on black; lo-res sixteen per Apple IIgs
    // Technote #63 "Master Color Values" (also used for the IIgs scheme).
    {"apple2", "Apple II",
     {0,0,0}, {255,255,255},
     {{0,0,0},{221,0,51},{0,119,34},{136,85,0},{0,0,153},{221,34,221},{102,170,255},{170,170,170},
      {85,85,85},{255,102,0},{17,221,0},{255,255,0},{34,34,255},{255,153,136},{68,255,153},{255,255,255}}},
    // Apple IIgs boot: white on medium blue ($22F), Control Panel defaults.
    {"iigs", "Apple IIgs",
     {34,34,255}, {255,255,255},
     {{0,0,0},{221,0,51},{0,119,34},{136,85,0},{0,0,153},{221,34,221},{102,170,255},{170,170,170},
      {85,85,85},{255,102,0},{17,221,0},{255,255,0},{34,34,255},{255,153,136},{68,255,153},{255,255,255}}},
    // MSX boot: COLOR 15,4,7 — white on dark blue (MSX BIOS defaults).
    // TMS9918 datasheet-derived palette.
    {"msx", "MSX",
     {43,45,227}, {255,255,255},
     {{0,0,0},{189,41,37},{10,173,30},{189,162,43},{43,45,227},{175,50,154},{30,226,239},{178,178,178},
      {0,0,0},{255,95,76},{52,200,76},{215,180,84},{81,75,251},{175,50,154},{30,226,239},{255,255,255}}},
    // Sharp MZ-700: white on blue boot (monitor/IPL), 3-bit digital RGB eight.
    {"mz700", "Sharp MZ-700",
     {0,0,255}, {255,255,255},
     {{0,0,0},{255,0,0},{0,255,0},{255,255,0},{0,0,255},{255,0,255},{0,255,255},{255,255,255},
      {0,0,0},{255,0,0},{0,255,0},{255,255,0},{0,0,255},{255,0,255},{0,255,255},{255,255,255}}},
    // Mattel Aquarius: black text on light blue-green (default fg 0 / bg 6,
    // TEA1002-derived community palette).
    {"aquarius", "Mattel Aquarius",
     {51,204,204}, {17,17,17},
     {{17,17,17},{255,17,17},{17,255,17},{255,255,17},{34,34,238},{255,17,255},{51,204,204},{255,255,255},
      {51,51,51},{187,34,34},{34,221,68},{255,255,119},{68,17,153},{204,34,204},{51,187,187},{204,204,204}}},
    // Amiga Workbench 1.3: white on blue, the four-color palette
    // ($05A/$FFF/$002/$F80). Four colors were the whole world.
    {"wb13", "Amiga Workbench 1.3",
     {0,85,170}, {255,255,255},
     {{0,0,34},{255,136,0},{255,255,255},{255,136,0},{0,85,170},{255,136,0},{255,255,255},{255,255,255},
      {0,0,34},{255,136,0},{255,255,255},{255,136,0},{0,85,170},{255,136,0},{255,255,255},{255,255,255}}},
    // Amiga Workbench 2.04: black on grey ($AAA/$000/$FFF/$68B).
    {"wb2", "Amiga Workbench 2",
     {170,170,170}, {0,0,0},
     {{0,0,0},{102,136,187},{102,136,187},{102,136,187},{102,136,187},{102,136,187},{102,136,187},{255,255,255},
      {0,0,0},{102,136,187},{102,136,187},{102,136,187},{102,136,187},{102,136,187},{102,136,187},{255,255,255}}},
    // TRS-80 Color Computer: MC6847 alphanumeric mode, dark green characters
    // on a bright green field (RGB from MAME's mc6847 palette) — a color
    // machine, wrongly assumed monochrome before this audit.
    {"coco", "TRS-80 CoCo",
     {48,210,0}, {0,124,0},
     {{38,48,22},{154,50,54},{0,124,0},{193,229,0},{76,58,180},{200,78,240},{65,175,113},{191,200,173},
      {38,48,22},{212,127,0},{48,210,0},{193,229,0},{76,58,180},{200,78,240},{65,175,113},{191,200,173}}},
};

// The integer-zoom render path (see retro.frag's integerZoom uniform) is only
// honest when the character cell divides the sourced resolution cleanly — a
// fractional cell means the machine's real grid doesn't map to whole virtual
// pixels (HP 150 is the documented example) and the exact-reconstruction
// guarantee doesn't hold. In that case, and for presets with no sourced grid
// at all, this returns 0 and the preset falls back to the resample path.
int RetroTermKCM::zoomFor(const PresetValues &p, int minCols, bool authenticSize)
{
    if (p.targetCols <= 0 || p.targetRows <= 0
        || p.targetResX <= 0.0 || p.targetResY <= 0.0)
        return 0;
    const int resX = (int)p.targetResX, resY = (int)p.targetResY;
    if (resX % p.targetCols != 0 || resY % p.targetRows != 0)
        return 0;

    QSize scr(1920, 1080);
    if (const QScreen *s = QGuiApplication::primaryScreen())
        scr = s->availableGeometry().size();

    if (authenticSize) {
        // Historical dimensions: fit the whole virtual screen on the display.
        const int k = std::min(scr.width() * 9 / 10 / resX,
                               scr.height() * 9 / 10 / resY);
        return std::clamp(k, 1, 8);
    }

    // Usable dimensions (the default). Integer zoom inherently trades columns
    // for pixel size: at cell width cw, a window of W pixels holds W/(cw*k)
    // columns, so every step up in k quarters... thirds... the usable width.
    // On a 1920px screen with an 8px cell that is ~240 columns at k=1, 120 at
    // k=2, 80 at k=3, 60 at k=4. Picking k to fill the screen (what this used
    // to do) therefore lands on a terminal far too narrow for modern shells:
    // fish prompts, git status and eza listings assume 80+ columns and wrap
    // into mush below that, which is exactly the breakage this now avoids.
    //
    // So: largest k that still leaves minCols columns. Pixel density becomes
    // as chunky as it can be *without* making the terminal unusable, and the
    // window keeps whatever size the user gave it — the shader derives the
    // virtual resolution from the actual content size anyway, so the result
    // stays pixel-exact, just on a bigger virtual screen than the original
    // machine had.
    const int cellW = resX / p.targetCols;
    const int cellH = resY / p.targetRows;
    const int kByW = (scr.width()  * 9 / 10) / (cellW * std::max(minCols, 20));
    // Keep a classic 24-line terminal's worth of height as well.
    const int kByH = (scr.height() * 9 / 10) / (cellH * 24);
    return std::clamp(std::min(kByW, kByH), 1, 8);
}

QWidget *RetroTermKCM::scrollWrap(QWidget *page)
{
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(page);
    return scroll;
}

ParamRow *RetroTermKCM::addParam(QFormLayout *fl, const QString &label,
                                  double min, double max, double step,
                                  const QString &key, const QString &tip)
{
    auto *row = new ParamRow(label, min, max, step, tip);
    fl->addRow(label, row);
    m_params[key] = row;
    connect(row, &ParamRow::valueChanged, this, &RetroTermKCM::markChanged);
    return row;
}

void RetroTermKCM::markChanged()
{
    setNeedsSave(true);
    schedulePreview();
}

// ── Target mode ───────────────────────────────────────────────────────────────
void RetroTermKCM::setTargetMode(TargetMode mode)
{
    if (m_customRow)
        m_customRow->setVisible(mode == TargetMode::Custom);
    if (m_terminalListWrap)
        m_terminalListWrap->setVisible(mode == TargetMode::Terminals);

    auto btn = [&]() -> QRadioButton * {
        switch (mode) {
            case TargetMode::Off:        return m_modeOff;
            case TargetMode::Terminals:  return m_modeTerminals;
            case TargetMode::Custom:     return m_modeCustom;
            case TargetMode::AllWindows: return nullptr;  // niet meer selecteerbaar; zie load()
        }
        return nullptr;
    }();
    if (btn && !btn->isChecked()) {
        QSignalBlocker b(m_modeGroup);
        btn->setChecked(true);
    }
}

TargetMode RetroTermKCM::currentTargetMode() const
{
    return static_cast<TargetMode>(m_modeGroup->checkedId());
}

// ── Terminal-checklist ──────────────────────────────────────────────────────────
QMap<QString, bool> RetroTermKCM::currentTerminalState() const
{
    QMap<QString, bool> state;
    if (!m_terminalList) return state;
    for (int i = 0; i < m_terminalList->count(); ++i) {
        auto *it = m_terminalList->item(i);
        state.insert(it->data(Qt::UserRole).toString(), it->checkState() == Qt::Checked);
    }
    return state;
}

QStringList RetroTermKCM::checkedTerminalClasses() const
{
    QStringList out;
    if (!m_terminalList) return out;
    for (int i = 0; i < m_terminalList->count(); ++i) {
        auto *it = m_terminalList->item(i);
        if (it->checkState() == Qt::Checked)
            out << it->data(Qt::UserRole).toString();
    }
    return out;
}

void RetroTermKCM::updateTerminalSummary()
{
    if (!m_terminalSummary || !m_terminalList) return;
    int checked = 0;
    for (int i = 0; i < m_terminalList->count(); ++i)
        if (m_terminalList->item(i)->checkState() == Qt::Checked) ++checked;
    m_terminalSummary->setText(i18n("%1 of %2 selected", checked, m_terminalList->count()));
}

// Rebuilds the checklist from what's actually on this system (QStandardPaths::
// findExecutable against PATH — cheap, dependency-free, and "found on the
// system" in the most literal sense). priorChecked seeds which rows start
// checked: on load() this is the terminal set from a previously saved config,
// on Rescan it's simply the list's current on-screen state, so neither loses
// anything the other doesn't already know about.
//
// Rows only ever come from two sources: a detected candidate, or a leftover
// key in priorChecked that detection didn't find (a terminal that's since been
// uninstalled, or a WM_CLASS carried over from an older Custom setup). Because
// save() only ever writes *checked* classes into targetClasses, an unchecked-
// but-installed candidate has no persisted trace — reopening the panel simply
// treats it as "no history yet" and checks it by default again. Remembering an
// explicit uncheck across restarts would need a second config key; not worth
// it unless it turns out to matter in practice.
void RetroTermKCM::rebuildTerminalList(const QMap<QString, bool> &priorChecked)
{
    if (!m_terminalList) return;
    m_terminalList->clear();

    QSet<QString> seen;
    auto addRow = [&](const QString &exec, const QString &display, bool notDetected) {
        if (seen.contains(exec)) return;
        seen.insert(exec);
        const bool checked = priorChecked.contains(exec) ? priorChecked.value(exec) : true;

        auto *item = new QListWidgetItem(m_terminalList);
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
        item->setData(Qt::UserRole, exec);
        item->setText(notDetected
            ? i18n("%1  (%2 — not detected)", display, exec)
            : i18n("%1  (%2)", display, exec));
        if (notDetected)
            item->setForeground(m_terminalList->palette().color(QPalette::Disabled, QPalette::Text));
    };

    for (const auto &cand : TERMINAL_CANDIDATES) {
        const QString exec = QString::fromLatin1(cand.exec);
        if (!QStandardPaths::findExecutable(exec).isEmpty())
            addRow(exec, QString::fromLatin1(cand.display), false);
    }
    for (auto it = priorChecked.constBegin(); it != priorChecked.constEnd(); ++it)
        if (!seen.contains(it.key()))
            addRow(it.key(), it.key(), true);

    updateTerminalSummary();
}

// ══════════════════════════════════════════════════════════════════════════════
// Constructor
// ══════════════════════════════════════════════════════════════════════════════
RetroTermKCM::RetroTermKCM(QObject *parent, const KPluginMetaData &data)
    : KCModule(parent, data)
{
    buildPresets();
    buildUI();
    load();
}

// ══════════════════════════════════════════════════════════════════════════════
// buildUI
// ══════════════════════════════════════════════════════════════════════════════
void RetroTermKCM::buildUI()
{
    auto *outerVBox = new QVBoxLayout(widget());
    outerVBox->setSpacing(8);

    m_tabs = new QTabWidget;
    m_tabs->addTab(scrollWrap(buildGeneralTab()), i18n("General"));
    m_tabs->addTab(scrollWrap(buildSetupTab()),   i18n("Setup"));
    // Effects tab already builds its own internal QScrollArea (needed there
    // regardless, for the sidebar+stack layout) — wrapping it again would just
    // nest two scroll areas around the same content for no benefit.
    m_tabs->addTab(buildEffectsTab(), i18n("Effects"));
    outerVBox->addWidget(m_tabs, 1);

    // ── Voettekst: live preview + handmatige apply ────────────────────────────
    // Deze twee horen bij geen enkel tabblad in het bijzonder — ze gelden voor
    // alles wat je in welk tabblad dan ook verandert, dus staan ze eronder.
    {
        auto *foot = new QHBoxLayout;

        m_livePreview = new QCheckBox(i18n("Live preview"));
        m_livePreview->setChecked(true);
        m_livePreview->setToolTip(i18n(
            "Applies every change immediately, so you can see a slider's effect "
            "while dragging it instead of only after pressing Apply.\n"
            "Each change is written to kwinrc and the effect is reloaded, "
            "so this also saves your settings as you go."));
        foot->addWidget(m_livePreview);

        m_applyKWin = new QPushButton(i18n("✓  Apply & reload KWin"));
        connect(m_applyKWin, &QPushButton::clicked, this, [this] {
            save();
            QDBusInterface kwin(QStringLiteral("org.kde.KWin"),
                                QStringLiteral("/KWin"),
                                QStringLiteral("org.kde.KWin"));
            kwin.call(QStringLiteral("reconfigure"));
            reconfigureKWinEffect();
        });

        // Met live preview aan doet elke wijziging dit al vanzelf, dus de knop
        // blijft dan technisch werken maar is voor 95% van de sessies overbodig
        // — zonder enig visueel signaal daarvan zag hij er hetzelfde uit als
        // wanneer hij wél de enige manier was om iets toe te passen. Nu grijst
        // hij uit en verandert het label mee, zodat "waarom staat deze knop
        // hier" zichzelf beantwoordt.
        auto updateApplyButtonState = [this] {
            const bool live = m_livePreview->isChecked();
            m_applyKWin->setEnabled(!live);
            m_applyKWin->setText(live
                ? i18n("✓  Applied automatically")
                : i18n("✓  Apply & reload KWin"));
            m_applyKWin->setToolTip(live
                ? i18n("Live preview is on — every change is already saved and "
                       "applied as you make it. Turn live preview off to apply "
                       "changes manually with this button instead.")
                : i18n("Saves all settings and reloads KWin "
                       "so the effect becomes active immediately."));
        };
        connect(m_livePreview, &QCheckBox::toggled, this, updateApplyButtonState);
        updateApplyButtonState();

        foot->addStretch();
        foot->addWidget(m_applyKWin);
        outerVBox->addLayout(foot);
    }

    // Debounce: een slider vuurt tientallen valueChanged-signalen per seconde af
    // en elke push is een kwinrc-write plus een D-Bus-round-trip naar KWin. Zonder
    // deze timer zou slepen de compositor onder schrijfacties bedelven.
    m_previewTimer = new QTimer(this);
    m_previewTimer->setSingleShot(true);
    m_previewTimer->setInterval(120);
    connect(m_previewTimer, &QTimer::timeout, this, &RetroTermKCM::pushLivePreview);
}

// ── Tabblad: Algemeen ─────────────────────────────────────────────────────────
QWidget *RetroTermKCM::buildGeneralTab()
{
    auto *page      = new QWidget;
    auto *outerVBox = new QVBoxLayout(page);
    outerVBox->setSpacing(8);

    {
        // Dit is de belangrijkste keuze op het hele tabblad, maar een gekleurde
        // stylesheet-rand op de QGroupBox was niet de juiste manier om dat te
        // laten zien — het botst met Breeze/andere Plasma-thema's en is verder
        // nergens anders in de UI terug te vinden. Positie (bovenaan, als eerste
        // ding dat je ziet) doet het werk al; standaard QGroupBox-chrome volstaat.
        auto *mgb = new QGroupBox(i18n("Which windows should receive the effect?"));
        auto *mvbox = new QVBoxLayout(mgb);
        mvbox->setSpacing(8);

        m_modeGroup = new QButtonGroup(this);

        m_modeOff = new QRadioButton(
            i18n("Off  —  no window gets the effect"));
        m_modeOff->setToolTip(i18n(
            "The effect stays loaded but does nothing. "
            "Useful for temporarily disabling it without unloading the plugin."));
        m_modeGroup->addButton(m_modeOff, static_cast<int>(TargetMode::Off));

        m_modeTerminals = new QRadioButton(
            i18n("Terminals  —  pick which detected terminals below"));
        m_modeTerminals->setToolTip(i18n(
            "Each terminal emulator found on this system gets its own checkbox "
            "in the list below."));
        m_modeGroup->addButton(m_modeTerminals, static_cast<int>(TargetMode::Terminals));

        m_modeCustom = new QRadioButton(
            i18n("Custom  —  choose specific applications"));
        m_modeCustom->setToolTip(i18n(
            "Enter a comma-separated list of WM_CLASS names.\n"
            "Use 'xprop WM_CLASS' to find a window class name."));
        m_modeGroup->addButton(m_modeCustom, static_cast<int>(TargetMode::Custom));

        mvbox->addWidget(m_modeOff);
        mvbox->addWidget(m_modeTerminals);

        // Terminal-checklist — verborgen tenzij "Terminals" geselecteerd. Alleen
        // het vinkje zelf is aan te klikken; de lijst heeft bewust geen selectie
        // (NoSelection), dus er is niets anders om per ongeluk te "selecteren".
        m_terminalListWrap = new QWidget;
        auto *tlw = new QVBoxLayout(m_terminalListWrap);
        tlw->setContentsMargins(28, 2, 0, 4);
        tlw->setSpacing(4);

        m_terminalList = new QListWidget;
        m_terminalList->setSelectionMode(QAbstractItemView::NoSelection);
        m_terminalList->setFocusPolicy(Qt::NoFocus);
        m_terminalList->setMaximumHeight(190);
        m_terminalList->setAlternatingRowColors(true);
        tlw->addWidget(m_terminalList);

        auto *tlFooter = new QHBoxLayout;
        m_rescanTerminals = new QPushButton(i18n("Rescan installed applications"));
        m_rescanTerminals->setFlat(true);
        m_terminalSummary = new QLabel;
        m_terminalSummary->setStyleSheet(QStringLiteral("color: palette(disabled-text);"));
        tlFooter->addWidget(m_rescanTerminals);
        tlFooter->addStretch();
        tlFooter->addWidget(m_terminalSummary);
        tlw->addLayout(tlFooter);

        mvbox->addWidget(m_terminalListWrap);
        m_terminalListWrap->setVisible(false);

        mvbox->addWidget(m_modeCustom);

        // Aangepast invoerveld — verborgen tenzij Custom geselecteerd
        m_customRow = new QWidget;
        auto *crow = new QHBoxLayout(m_customRow);
        crow->setContentsMargins(28, 2, 0, 2);
        crow->addWidget(new QLabel(i18n("Window classes:")));
        m_targetClasses = new QLineEdit;
        m_targetClasses->setPlaceholderText(
            i18n("e.g. konsole,firefox,code  (lowercase, comma-separated)"));
        m_targetClasses->setToolTip(i18n(
            "WM_CLASS names. Find them via:\n"
            "  xprop WM_CLASS  (then click the window)\n"
            "  qdbus6 org.kde.KWin /KWin org.kde.KWin.queryWindowInfo"));
        crow->addWidget(m_targetClasses, 1);
        mvbox->addWidget(m_customRow);
        m_customRow->setVisible(false);

        // Verbindingen
        connect(m_modeGroup, QOverload<int>::of(&QButtonGroup::idClicked),
                this, [this](int id) {
            setTargetMode(static_cast<TargetMode>(id));
            markChanged();
        });
        connect(m_targetClasses, &QLineEdit::textChanged,
                this, &RetroTermKCM::markChanged);
        connect(m_terminalList, &QListWidget::itemChanged, this, [this](QListWidgetItem *) {
            updateTerminalSummary();
            markChanged();
        });
        connect(m_rescanTerminals, &QPushButton::clicked, this, [this] {
            rebuildTerminalList(currentTerminalState());
            markChanged();
        });

        outerVBox->addWidget(mgb);
    }

    outerVBox->addStretch();
    return page;
}

// ── Tabblad: Setup ────────────────────────────────────────────────────────────
// Preset, font, and screen resolution used to be three separate tabs. They're
// really one decision made in three parts: pick an era, get its font and
// pixel size along with it. Splitting that across tabs meant picking a
// preset on one tab, then hunting across two more tabs to see (and act on)
// what it had just set — reworked into one tab so the whole "what machine am
// I simulating" choice reads top to bottom in one place. Effects (how it
// looks — bloom, scanlines, noise, ...) stays separate: that's a distinct,
// much larger decision space, and now has its own sidebar navigation.
QWidget *RetroTermKCM::buildSetupTab()
{
    auto *page      = new QWidget;
    auto *outerVBox = new QVBoxLayout(page);
    outerVBox->setSpacing(8);

    outerVBox->addWidget(buildPresetSection());
    outerVBox->addWidget(buildFontSection());
    outerVBox->addWidget(buildScreenSection());
    outerVBox->addStretch();

    return page;
}

// ── Setup-onderdeel: preset ───────────────────────────────────────────────────
QGroupBox *RetroTermKCM::buildPresetSection()
{
    auto *pgb = new QGroupBox(i18n("Historical preset"));
    auto *pfl = new QFormLayout(pgb);

    m_presetCombo = new QComboBox;
    m_presetCombo->addItem(i18n("— Choose a preset —"));
    for (const auto &pv : m_presets)
        m_presetCombo->addItem(pv.name);

    m_applyPreset = new QPushButton(i18n("Load preset"));
    m_applyPreset->setEnabled(false);

    pfl->addRow(i18n("Preset:"), m_presetCombo);

    // Shows the preset's recommended font and target resolution the moment it's
    // picked — before "Load preset" is even clicked — since neither is something
    // this effect can set for you: the font lives in the terminal emulator's own
    // profile, entirely outside what a KWin effect can reach.
    m_presetInfo = new QLabel;
    m_presetInfo->setTextFormat(Qt::RichText);
    m_presetInfo->setWordWrap(true);
    pfl->addRow(QString(), m_presetInfo);

    auto *btnRow = new QHBoxLayout;
    btnRow->addWidget(m_applyPreset);
    btnRow->addStretch();
    pfl->addRow(QString(), btnRow);

    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        m_applyPreset->setEnabled(idx > 0);
        updatePresetInfo(idx > 0 ? m_presets.at(idx - 1) : PresetValues{});
    });
    connect(m_applyPreset, &QPushButton::clicked, this, [this] {
        const int idx = m_presetCombo->currentIndex();
        if (idx > 0) applyPreset(m_presets.at(idx - 1));
    });

    return pgb;
}

// Dit onderdeel bestaat omdat een KWin-effect het lettertype van een terminal
// principieel niet kán zetten: KWin krijgt het venster pas te zien nadat de
// terminal de glyphs al gerasterd heeft, en verwerkt alleen die kant-en-klare
// pixels na. Het font is en blijft een instelling van de terminal zelf. De enige
// eerlijke manier om een preset-font toch te laten werken, is het in het profiel
// van die terminal schrijven — wat deze sectie voor Konsole doet.
QGroupBox *RetroTermKCM::buildFontSection()
{
    QFormLayout *fl = nullptr;
    auto *gb = makeGroup(i18n("Font"), fl);

    {
        // Explanation lives as the group's first row rather than a separate
        // label above it — this section moved into the combined Setup tab,
        // where a floating label before a GroupBox reads as detached from it.
        auto *expl = new QLabel(i18n(
            "<p>A KWin effect post-processes pixels that the terminal has "
            "<i>already drawn</i>, so it cannot pick the font those characters "
            "were rendered with — the font belongs to the terminal emulator, not "
            "to KWin. Each preset therefore only <i>recommends</i> a font.</p>"
            "<p>That also means the CRT effect's pixel-scaling (Setup tab) and "
            "Konsole's own rendering are otherwise unrelated: the shader "
            "resamples whatever Konsole already drew, at whatever size Konsole "
            "happened to draw it. Where a preset has a sourced historical text "
            "grid, this page sets Konsole's own window size to match — so the "
            "font actually renders across the same grid the shader is "
            "simulating, not an arbitrary modern terminal size cut through by "
            "a generic resample.</p>"
            "<p>Konsole stores its font (and window size) in a profile file, "
            "which this page can write for you. For other terminals, set the "
            "font yourself:</p>"
            "<ul>"
            "<li><b>kitty</b> — <code>font_family</code> in <code>~/.config/kitty/kitty.conf</code></li>"
            "<li><b>Alacritty</b> — <code>font.normal.family</code> in <code>~/.config/alacritty/alacritty.toml</code></li>"
            "<li><b>WezTerm</b> — <code>font = wezterm.font(...)</code> in <code>~/.wezterm.lua</code></li>"
            "</ul>"));
        expl->setWordWrap(true);
        expl->setTextFormat(Qt::RichText);
        fl->addRow(expl);
    }

    {
        m_fontRecommend = new QLabel;
        m_fontRecommend->setTextFormat(Qt::RichText);
        m_fontRecommend->setWordWrap(true);
        fl->addRow(i18n("Preset font:"), m_fontRecommend);

        m_konsoleProfile = new QComboBox;
        m_konsoleProfile->setToolTip(i18n(
            "Which Konsole profile to write the font into. "
            "The profile marked (default) is the one Konsole opens with."));
        fl->addRow(i18n("Konsole profile:"), m_konsoleProfile);

        m_autoApplyFont = new QCheckBox(
            i18n("Set this font automatically when loading a preset"));
        m_autoApplyFont->setChecked(true);
        m_autoApplyFont->setToolTip(i18n(
            "With this on, \"Load preset\" also writes that preset's font into "
            "the selected Konsole profile, so the era-correct typeface appears "
            "along with the era-correct CRT look."));
        fl->addRow(QString(), m_autoApplyFont);

        m_applyFontBtn = new QPushButton(i18n("Write font to profile now"));
        connect(m_applyFontBtn, &QPushButton::clicked, this, [this] {
            if (m_presetFont.isEmpty()) {
                m_fontStatus->setText(i18n(
                    "<span style=\"color:#c0392b;\">Load a preset first — "
                    "it decides which font to write.</span>"));
                return;
            }
            QString err;
            if (applyFontToKonsole(m_presetFont, m_presetFontSize,
                                    m_presetCols, m_presetRows,
                                    m_presetFontPx, m_presetScheme, &err)) {
                const QString gridPart = (m_presetCols > 0 && m_presetRows > 0)
                    ? i18n(" and resized it to %1×%2 characters", m_presetCols, m_presetRows)
                    : QString();
                m_fontStatus->setText(i18n(
                    "<span style=\"color:#27ae60;\">Wrote <b>%1</b> %2pt to "
                    "%3%4.</span> Open a new Konsole tab or window to see it — "
                    "Konsole reads a profile when a session starts.",
                    m_presetFont, m_presetFontSize,
                    m_konsoleProfile->currentText(), gridPart));
            } else {
                m_fontStatus->setText(i18n(
                    "<span style=\"color:#c0392b;\">Could not write the profile: "
                    "%1</span>", err));
            }
        });
        m_restoreFontBtn = new QPushButton(i18n("Restore my original font"));
        m_restoreFontBtn->setToolTip(i18n(
            "Puts back the font this profile had before Phosphor first changed it. "
            "The original is saved once, the first time a preset font is written, "
            "so this always returns your own choice — never an earlier preset's."));
        connect(m_restoreFontBtn, &QPushButton::clicked, this, [this] {
            QString err;
            if (restoreKonsoleFont(&err)) {
                m_fontStatus->setText(i18n(
                    "<span style=\"color:#27ae60;\">Restored the original font in "
                    "%1.</span> Open a new Konsole tab or window to see it.",
                    m_konsoleProfile->currentText()));
            } else {
                m_fontStatus->setText(i18n(
                    "<span style=\"color:#c0392b;\">Nothing restored: %1</span>", err));
            }
        });

        auto *row = new QHBoxLayout;
        row->addWidget(m_applyFontBtn);
        row->addWidget(m_restoreFontBtn);
        row->addStretch();
        fl->addRow(QString(), row);

        m_fontStatus = new QLabel;
        m_fontStatus->setTextFormat(Qt::RichText);
        m_fontStatus->setWordWrap(true);
        fl->addRow(QString(), m_fontStatus);
    }

    refreshKonsoleProfiles();
    updateFontTabInfo();
    return gb;
}

// ── Tabblad: Effecten ─────────────────────────────────────────────────────────
// Effects used to be one long QVBoxLayout of eight-plus GroupBoxes inside a
// single QScrollArea — everything worked, but finding "Bloom" meant scrolling
// past Phosphor, Geometry, and Scanlines first every time, with no sense of
// where you were in the list. That's the flat-stack anti-pattern: it scales
// to three groups, not eight-plus params-heavy ones. This is the same
// sidebar-list + stacked-pages structure System Settings itself uses (also
// Dolphin's and Kate's preferences) — a category list on the left drives a
// QStackedWidget on the right, so only one group's worth of controls is ever
// on screen and jumping to "Animations" is one click instead of a scroll.
QWidget *RetroTermKCM::buildEffectsTab()
{
    auto *page      = new QWidget;
    auto *outerVBox = new QVBoxLayout(page);
    outerVBox->setContentsMargins(0, 0, 0, 0);

    auto *splitter = new QSplitter(Qt::Horizontal);

    auto *nav = new QListWidget;
    nav->setSelectionMode(QAbstractItemView::SingleSelection);
    nav->setMaximumWidth(180);
    nav->setUniformItemSizes(true);
    splitter->addWidget(nav);

    auto *stack = new QStackedWidget;
    splitter->addWidget(stack);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    // Each group keeps its own QGroupBox title (used elsewhere, e.g. tooltips
    // referencing "the Pixel scaling group") — the nav gets a shorter label
    // where the two would otherwise wrap awkwardly in a 180px-wide list.
    auto addPage = [&](const QString &navLabel, QWidget *groupBox) {
        nav->addItem(navLabel);
        // scrollWrap() here too: a single group can still run long (Pixel
        // Scaling has five rows plus quick-pick buttons) on a short window,
        // and every other tab already gets this same resize safety net.
        stack->addWidget(scrollWrap(groupBox));
    };

    { // Fosfory
        QFormLayout *fl = nullptr;
        auto *gb = makeGroup(i18n("Phosphor and color"), fl);
        auto *phc = new QComboBox;
        phc->addItem(i18n("P1 — Bright green (VT100, Wyse)"));
        phc->addItem(i18n("P3 — Amber (IBM 3101, early terminals)"));
        phc->addItem(i18n("P4 — White/cream (MDA, Mac, VGA)"));
        phc->addItem(i18n("P39 — Radar green (long persistence)"));
        fl->addRow(i18n("Phosphor type:"), phc);
        m_combos["phosphorType"] = phc;
        connect(phc, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &RetroTermKCM::markChanged);
        addParam(fl, i18n("Aging:"),             0.0,  1.0,  0.01, "phosphorAgeing",      i18n("0=new, 1=aged yellow/brown"));
        addParam(fl, i18n("Color temperature (K):"),3000,9300,50, "colorTemperature",   i18n("3000=warm yellow, 9300=cold blue-white"));
        addParam(fl, i18n("Persistence:"),       0.0,  1.0,  0.01, "phosphorPersistence", i18n("How long phosphor glow remains visible"));
        addPage(i18n("Phosphor & Color"), gb);
    }
    { // Geometrie
        QFormLayout *fl = nullptr;
        auto *gb = makeGroup(i18n("Screen geometry"), fl);
        addParam(fl, i18n("Barrel distortion:"), 0.0, 1.0,  0.01, "screenCurvature",   i18n("0=flat, 1=strongly curved"));
        addParam(fl, i18n("Vignette:"),          0.0, 1.0,  0.01, "vignetteIntensity", i18n("Edge darkening"));
        addParam(fl, i18n("Glass reflection:"),  0.0, 0.30, 0.005,"ambientReflection", i18n("Screen glass reflection"));
        addPage(i18n("Screen Geometry"), gb);
    }
    { // Scanlines
        QFormLayout *fl = nullptr;
        auto *gb = makeGroup(i18n("Scanlines / Rasterization"), fl);
        auto *rc = new QComboBox;
        rc->addItem(i18n("None")); rc->addItem(i18n("Scanlines (classic)"));
        rc->addItem(i18n("Pixel grid (shadow mask)")); rc->addItem(i18n("Sub-pixel RGB (aperture grille)"));
        fl->addRow(i18n("Mode:"), rc);
        m_combos["rasterizationMode"] = rc;
        connect(rc, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &RetroTermKCM::markChanged);
        addParam(fl, i18n("Intensity:"), 0.0, 1.0, 0.01, "scanlinesIntensity", i18n("How dark the gaps are"));
        addParam(fl, i18n("Sharpness:"), 0.0, 1.0, 0.01, "scanlinesSharpness", i18n("0=soft, 1=sharp"));
        addPage(i18n("Scanlines"), gb);
    }
    { // Bloom
        QFormLayout *fl = nullptr;
        auto *gb = makeGroup(i18n("Bloom and glow"), fl);
        addParam(fl, i18n("Bloom:"),      0.0, 1.0, 0.01, "bloom",       i18n("Glow halo (13-tap Gaussian)"));
        addParam(fl, i18n("Line glow:"),  0.0, 1.0, 0.01, "glowingLine", i18n("Horizontal line glow"));
        addParam(fl, i18n("Brightness:"), 0.0, 1.0, 0.01, "brightness",  i18n("Overall brightness"));
        addParam(fl, i18n("Contrast:"),  0.0, 1.0, 0.01, "contrast",    i18n("Contrast"));
        addPage(i18n("Bloom & Glow"), gb);
    }
    { // Ruis
        QFormLayout *fl = nullptr;
        auto *gb = makeGroup(i18n("Noise and sync artifacts"), fl);
        addParam(fl, i18n("Static noise:"), 0.0, 1.0,  0.01, "staticNoise",       i18n("Grain-like image noise"));
        addParam(fl, i18n("Jitter:"),         0.0, 1.0,  0.01, "jitter",            i18n("Per-pixel horizontal offset"));
        auto *sc2 = new QComboBox;
        sc2->addItem(i18n("Stable")); sc2->addItem(i18n("Sine drift")); sc2->addItem(i18n("Rolling scan")); sc2->addItem(i18n("Ghosting"));
        fl->addRow(i18n("Sync mode:"), sc2);
        m_combos["syncMode"] = sc2;
        connect(sc2, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &RetroTermKCM::markChanged);
        addParam(fl, i18n("Sync intensity:"),  0.0, 1.0,  0.01, "horizontalSync",    i18n("Artifact strength"));
        addParam(fl, i18n("Flicker:"),         0.0, 1.0,  0.01, "flickering",        i18n("50/60Hz brightness flicker"));
        addParam(fl, i18n("Ghost intensity:"), 0.0, 0.5,  0.005,"ghostingIntensity", i18n("Frame echo (only in Ghosting sync mode)"));
        addPage(i18n("Noise & Sync"), gb);
    }
    { // Kleur
        QFormLayout *fl = nullptr;
        auto *gb = makeGroup(i18n("Color and optical aberrations"), fl);
        addParam(fl, i18n("Color retention:"),  0.0, 1.0, 0.01, "chromaColor",       i18n("0=grayscale, 1=full color"));
        addParam(fl, i18n("Saturation:"),       0.0, 1.0, 0.01, "saturationColor",   i18n("Additional color saturation"));
        addParam(fl, i18n("Chrom. aberration:"),0.0, 1.0, 0.01, "rbgShift",          i18n("Horizontally shifted RGB channels"));
        addParam(fl, i18n("Character smearing:"),0.0,1.0, 0.01, "characterSmearing", i18n("Horizontal character smearing"));
        addParam(fl, i18n("Burn-in:"),          0.0, 1.0, 0.01, "burnIn",            i18n("Slightly brighter screen center"));
        addPage(i18n("Color & Aberrations"), gb);
    }
    { // Animaties
        QFormLayout *fl = nullptr;
        auto *gb = makeGroup(i18n("Animations"), fl);
        auto *wuc = new QCheckBox(i18n("CRT warmup animation when opening a window"));
        fl->addRow(wuc); m_checks["warmupEnabled"] = wuc;
        connect(wuc, &QCheckBox::checkStateChanged, this, &RetroTermKCM::markChanged);
        auto *wus = new QDoubleSpinBox; wus->setRange(0.5,30.0); wus->setSuffix(i18n(" sec")); wus->setSingleStep(0.5); wus->setDecimals(1);
        fl->addRow(i18n("Warmup duration:"), wus); m_spins["warmupDuration"] = wus;
        connect(wus, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &RetroTermKCM::markChanged);
        auto *dgc = new QCheckBox(i18n("Degauss animation when opening a window"));
        fl->addRow(dgc); m_checks["degaussOnStart"] = dgc;
        connect(dgc, &QCheckBox::checkStateChanged, this, &RetroTermKCM::markChanged);
        auto *dgs = new QDoubleSpinBox; dgs->setRange(0.5,10.0); dgs->setSuffix(i18n(" sec")); dgs->setSingleStep(0.5); dgs->setDecimals(1);
        fl->addRow(i18n("Degauss duration:"), dgs); m_spins["degaussDuration"] = dgs;
        connect(dgs, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &RetroTermKCM::markChanged);
        addPage(i18n("Animations"), gb);
    }

    nav->setCurrentRow(0);
    connect(nav, &QListWidget::currentRowChanged, stack, &QStackedWidget::setCurrentIndex);

    outerVBox->addWidget(splitter, 1);
    return page;
}

// Pixel scaling moved out of Effects and into the Setup tab: it isn't a look
// parameter you tune by eye like bloom or scanlines, it's "what resolution was
// this machine" — the same category of decision as which preset or font you
// picked, and it belongs next to those, not buried as one more entry in an
// eight-item effects sidebar.
QGroupBox *RetroTermKCM::buildScreenSection()
{
    QFormLayout *fl = nullptr;
    auto *gb = makeGroup(i18n("Screen resolution — simulate original pixel size"), fl);
    gb->setToolTip(i18n(
        "Downscales the window image to the original pixel resolution of the "
        "historical system, then scales it back up to full screen.\n"
        "0.0 = no scaling (modern display)\n"
        "1.0 = exact original pixels (true-size block pixels)\n"
        "Values in between blend both views."));

    // Bron-uitgelijnde integer zoom — de eerlijke route naar een virtueel
    // scherm. Het resample-pad hieronder (pixel scale) kan tekst die op hoge
    // resolutie gerasterd is nooit meer authentiek 320×200 maken: het prikt
    // samples in klaargetekende glyphs en vermorzelt dunne beeldlijnen. Bij
    // integer zoom k rendert Konsole zélf het pixelfont op cel×k fysieke
    // pixels (dit tabblad schrijft die fontgrootte plus rand 0 het profiel
    // in), waarna de shader het virtuele scherm exact terugleest als
    // content/k. "Load preset" vult k automatisch in voor presets met een
    // gedocumenteerd tekstraster.
    m_integerZoomSpin = new QSpinBox;
    m_integerZoomSpin->setRange(0, 8);
    m_integerZoomSpin->setSpecialValueText(i18n("off — use pixel scale below"));
    m_integerZoomSpin->setToolTip(i18n(
        "Source-aligned integer scaling. At k > 0 the preset font is written "
        "into the Konsole profile at exactly (cell × k) physical pixels and "
        "the shader treats every k×k block as one virtual pixel — a "
        "pixel-perfect virtual screen, unlike the approximate resample below. "
        "Loading a preset with a documented text grid fills this in "
        "automatically. Requires the preset font to be active in Konsole."));
    fl->addRow(i18n("Integer zoom (k):"), m_integerZoomSpin);
    connect(m_integerZoomSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &RetroTermKCM::markChanged);

    // Kolom-ondergrens. Zonder deze rem koos "Load preset" de grootst mogelijke
    // k, en dat is precies waar een authentiek beeld botst met een bruikbare
    // terminal: elke stap in k deelt het aantal kolommen. Bij k=4 en een cel van
    // 8px houdt een venster van 1280px nog 40 kolommen over — te smal voor
    // fish-prompts, git-status of eza, die dan halverwege woorden afbreken.
    m_minColumns = new QSpinBox;
    m_minColumns->setRange(20, 300);
    m_minColumns->setValue(80);
    m_minColumns->setSuffix(i18n(" columns"));
    m_minColumns->setToolTip(i18n(
        "The zoom factor chosen when loading a preset is the largest one that "
        "still leaves at least this many columns. 80 is the classic terminal "
        "width nearly every shell prompt and command-line tool is designed "
        "for; lower values give chunkier pixels but start to wrap modern "
        "output badly."));
    fl->addRow(i18n("Keep at least:"), m_minColumns);
    connect(m_minColumns, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &RetroTermKCM::markChanged);
    // Beide knoppen sturen welke k straks gekozen wordt, dus het preset-label
    // (op het Setup-tabblad hierboven) moet meteen meebewegen — anders staat
    // daar een zoomfactor die niet meer klopt met wat "Load preset" doet.
    auto refreshPresetInfo = [this] {
        if (!m_presetCombo) return;
        const int idx = m_presetCombo->currentIndex();
        updatePresetInfo(idx > 0 ? m_presets.at(idx - 1) : PresetValues{});
    };
    connect(m_minColumns, QOverload<int>::of(&QSpinBox::valueChanged),
            this, refreshPresetInfo);

    m_authenticSize = new QCheckBox(
        i18n("Resize the terminal to the machine's exact grid"));
    m_authenticSize->setToolTip(i18n(
        "Writes the historical character grid (40×25 for a C64, 80×25 for a "
        "DOS machine, ...) into the Konsole profile, so the window matches the "
        "original screen exactly.\n\n"
        "Authentic, but be warned: a 40-column terminal is too narrow for most "
        "modern shell prompts and listings — they will wrap mid-word. Leave "
        "this off to keep your own terminal size and get the era-correct "
        "pixels without the era-correct cramping."));
    fl->addRow(QString(), m_authenticSize);
    connect(m_authenticSize, &QCheckBox::toggled,
            this, &RetroTermKCM::markChanged);
    connect(m_authenticSize, &QCheckBox::toggled, this, refreshPresetInfo);
    // De kolom-ondergrens stuurt alleen de niet-authentieke keuze; in
    // authentieke modus bepaalt het historische raster alles.
    connect(m_authenticSize, &QCheckBox::toggled, m_minColumns, &QWidget::setDisabled);
    m_minColumns->setDisabled(m_authenticSize->isChecked());

    // Hoofdslider
    m_pixelScaleRow = new ParamRow(
        i18n("Pixel scale:"), 0.0, 1.0, 0.01,
        i18n("0.0 = no scaling  |  1.0 = exact original pixels"),
        gb);
    fl->addRow(i18n("Pixel scale:"), m_pixelScaleRow);
    connect(m_pixelScaleRow, &ParamRow::valueChanged,
            this, &RetroTermKCM::markChanged);
    // Enable-logica in één plek: bij integer zoom is het hele resample-blok
    // (pixel scale, sampling, doelresolutie) inert — de shader negeert het —
    // en bij pixelScale 0 zijn de resolutievelden dat ook. Uitgrijzen laat
    // die staat zien in plaats van hem in een tooltip te verstoppen.
    auto updateScalingEnables = [this] {
        const bool zoomed = m_integerZoomSpin && m_integerZoomSpin->value() > 0;
        if (m_pixelScaleRow)   m_pixelScaleRow->setEnabled(!zoomed);
        if (m_sampleModeCombo) m_sampleModeCombo->setEnabled(!zoomed);
        if (m_targetResRow)
            m_targetResRow->setEnabled(!zoomed
                && m_pixelScaleRow && m_pixelScaleRow->value() > 0.001);
    };
    connect(m_pixelScaleRow, &ParamRow::valueChanged, this, updateScalingEnables);
    connect(m_integerZoomSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, updateScalingEnables);

    // Sampling-modus combobox
    m_sampleModeCombo = new QComboBox;
    m_sampleModeCombo->addItem(i18n("Nearest-neighbour  —  hard block pixels (classic)"));
    m_sampleModeCombo->addItem(i18n("Bilinear  —  smooth, good for mid values"));
    m_sampleModeCombo->addItem(i18n("Sharp bilinear  —  CRT-like: crisp edges, low aliasing"));
    m_sampleModeCombo->setCurrentIndex(2);
    m_sampleModeCombo->setToolTip(i18n(
        "Nearest: true block pixels like original hardware.\n"
        "Bilinear: smooth interpolation, good at pixelScale 0.3–0.7.\n"
        "Sharp bilinear: simulates a CRT Gaussian beam — "
        "crisp pixel edges without harsh stair-stepping. Recommended."));
    fl->addRow(i18n("Sampling:"), m_sampleModeCombo);
    connect(m_sampleModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RetroTermKCM::markChanged);

    // Originele resolutie-invoer
    // Automatisch ingevuld bij preset laden, handmatig aanpasbaar
    m_targetResRow = new QWidget;
    auto *rhl = new QHBoxLayout(m_targetResRow);
    rhl->setContentsMargins(0, 0, 0, 0);
    rhl->addWidget(new QLabel(i18n("Width:")));
    m_targetResX = new QDoubleSpinBox;
    m_targetResX->setRange(40, 3840);
    m_targetResX->setDecimals(0);
    m_targetResX->setSingleStep(8);
    m_targetResX->setValue(320);
    m_targetResX->setToolTip(i18n("Original horizontal resolution of the historical system (pixels)"));
    rhl->addWidget(m_targetResX);
    rhl->addSpacing(12);
    rhl->addWidget(new QLabel(i18n("Height:")));
    m_targetResY = new QDoubleSpinBox;
    m_targetResY->setRange(24, 2160);
    m_targetResY->setDecimals(0);
    m_targetResY->setSingleStep(8);
    m_targetResY->setValue(200);
    m_targetResY->setToolTip(i18n("Original vertical resolution of the historical system (pixels)"));
    rhl->addWidget(m_targetResY);
    rhl->addStretch();

    // Snelkeuze-knopjes voor veelvoorkomende resoluties
    auto addRes = [&](const QString &lbl, int w, int h) {
        auto *btn = new QPushButton(lbl);
        btn->setFixedWidth(90);
        btn->setToolTip(QStringLiteral("%1 × %2").arg(w).arg(h));
        rhl->addWidget(btn);
        connect(btn, &QPushButton::clicked, this, [this, w, h] {
            m_targetResX->setValue(w);
            m_targetResY->setValue(h);
            markChanged();
        });
    };
    addRes(i18n("320×200"),  320, 200);
    addRes(i18n("640×200"),  640, 200);
    addRes(i18n("640×480"),  640, 480);
    addRes(i18n("720×350"),  720, 350);

    fl->addRow(i18n("Original res.:"), m_targetResRow);
    // setValue() only emits valueChanged on an actual change, so the enable
    // states need one explicit sync here — load() calling setValue(0.0) on a
    // slider that already defaults to 0.0 fires no signal at all, and the
    // rows would otherwise start enabled regardless.
    updateScalingEnables();

    connect(m_targetResX, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &RetroTermKCM::markChanged);
    connect(m_targetResY, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &RetroTermKCM::markChanged);

    // Info-label
    auto *info = new QLabel(i18n(
        "<small><i>Tip: load a preset — original resolution is filled automatically.<br>"
        "At pixelScale = 0.0, resolution has no visual effect.</i></small>"));
    info->setWordWrap(true);
    fl->addRow(QString(), info);

    return gb;
}

// ══════════════════════════════════════════════════════════════════════════════
// Live preview
// ══════════════════════════════════════════════════════════════════════════════
// reconfigure() reloads KWin's own settings but never reaches the effects, so
// without reconfigureEffect() a save would land in kwinrc and the effect would
// keep rendering the old values until it was reloaded by hand.
void RetroTermKCM::reconfigureKWinEffect()
{
    QDBusInterface fx(QStringLiteral("org.kde.KWin"),
                      QStringLiteral("/Effects"),
                      QStringLiteral("org.kde.kwin.Effects"));
    // loadEffect() is safe to call on an already-loaded effect (KWin just
    // no-ops) and is the only way an effect that was never enabled — the
    // Plugins/retro-termEnabled flag save() now sets is only read by KWin at
    // its own startup — actually starts running in the *current* KWin
    // process, rather than requiring a logout or a full "kwin --replace"
    // before any of this KCM's settings become visible at all.
    fx.call(QStringLiteral("loadEffect"), QStringLiteral("retro-term"));
    fx.call(QStringLiteral("reconfigureEffect"), QStringLiteral("retro-term"));
}

void RetroTermKCM::schedulePreview()
{
    if (m_livePreview && m_livePreview->isChecked() && m_previewTimer)
        m_previewTimer->start();   // herstart: pas 120 ms ná de laatste wijziging
}

void RetroTermKCM::pushLivePreview()
{
    if (!m_livePreview || !m_livePreview->isChecked()) return;
    save();
    reconfigureKWinEffect();
}

// ══════════════════════════════════════════════════════════════════════════════
// Konsole font bridge
// ══════════════════════════════════════════════════════════════════════════════
void RetroTermKCM::refreshKonsoleProfiles()
{
    if (!m_konsoleProfile) return;
    m_konsoleProfile->clear();

    // Konsole leest profielen uit <genericdata>/konsole/*.profile en noteert het
    // standaardprofiel in konsolerc als bestandsnaam, niet als profielnaam.
    const QString defaultProfile =
        KSharedConfig::openConfig(QStringLiteral("konsolerc"))
            ->group(QStringLiteral("Desktop Entry"))
            .readEntry("DefaultProfile", QString());

    const QStringList dirs =
        QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    for (const QString &d : dirs) {
        QDir konsoleDir(d + QStringLiteral("/konsole"));
        const QStringList files =
            konsoleDir.entryList({QStringLiteral("*.profile")}, QDir::Files, QDir::Name);
        for (const QString &f : files) {
            const QString path = konsoleDir.filePath(f);
            // De zichtbare naam staat in [General] Name=; val terug op de
            // bestandsnaam als het profiel die sleutel niet heeft.
            const QString name =
                KSharedConfig::openConfig(path, KConfig::SimpleConfig)
                    ->group(QStringLiteral("General"))
                    .readEntry("Name", QFileInfo(f).completeBaseName());
            const QString label = (f == defaultProfile)
                ? i18n("%1  (default)", name) : name;
            if (m_konsoleProfile->findData(path) < 0)
                m_konsoleProfile->addItem(label, path);
            if (f == defaultProfile)
                m_konsoleProfile->setCurrentIndex(m_konsoleProfile->count() - 1);
        }
    }

    const bool any = m_konsoleProfile->count() > 0;
    m_konsoleProfile->setEnabled(any);
    if (m_applyFontBtn)   m_applyFontBtn->setEnabled(any);
    if (m_restoreFontBtn) m_restoreFontBtn->setEnabled(any);
    if (m_autoApplyFont)  m_autoApplyFont->setEnabled(any);
    if (!any) {
        m_konsoleProfile->addItem(i18n("— no Konsole profiles found —"));
        if (m_fontStatus)
            m_fontStatus->setText(i18n(
                "No <code>*.profile</code> files under "
                "<code>~/.local/share/konsole/</code>. Konsole writes one the "
                "first time you edit a profile (Settings → Edit Current Profile), "
                "after which it will show up here."));
    }
}

QString RetroTermKCM::ensureColorScheme(const QString &id)
{
    if (id.isEmpty()) return QString();
    const SchemeDef *def = nullptr;
    for (const auto &d : SCHEME_DEFS)
        if (id == QLatin1String(d.id)) { def = &d; break; }
    if (!def) return QString();

    const QString schemeName = QStringLiteral("Phosphor ") + QLatin1String(def->name);
    const QString dir =
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + QStringLiteral("/konsole");
    QDir().mkpath(dir);
    const QString path = dir + QLatin1Char('/') + schemeName
                       + QStringLiteral(".colorscheme");

    // Altijd overschrijven: het bestand is van ons (Phosphor-naamruimte), en zo
    // pikt een bestaand schema paletcorrecties in nieuwere versies vanzelf op.
    KSharedConfig::Ptr cfg = KSharedConfig::openConfig(path, KConfig::SimpleConfig);
    auto writeColor = [&](const QString &group, SchemeColor c) {
        cfg->group(group).writeEntry(QStringLiteral("Color"),
            QStringLiteral("%1,%2,%3").arg(c.r).arg(c.g).arg(c.b));
    };
    cfg->group(QStringLiteral("General"))
        .writeEntry(QStringLiteral("Description"), schemeName);
    writeColor(QStringLiteral("Background"), def->bg);
    writeColor(QStringLiteral("BackgroundIntense"), def->bg);
    writeColor(QStringLiteral("BackgroundFaint"), def->bg);
    writeColor(QStringLiteral("Foreground"), def->fg);
    writeColor(QStringLiteral("ForegroundIntense"), def->fg);
    writeColor(QStringLiteral("ForegroundFaint"), def->fg);
    for (int i = 0; i < 8; ++i) {
        const QString n = QString::number(i);
        writeColor(QStringLiteral("Color") + n, def->ansi[i]);
        writeColor(QStringLiteral("Color") + n + QStringLiteral("Intense"), def->ansi[i + 8]);
        writeColor(QStringLiteral("Color") + n + QStringLiteral("Faint"), def->ansi[i]);
    }
    cfg->sync();
    return schemeName;
}

bool RetroTermKCM::applyFontToKonsole(const QString &family, int pointSize,
                                       int cols, int rows, int fontPixelSize,
                                       const QString &scheme, QString *error)
{
    if (!m_konsoleProfile || !m_konsoleProfile->isEnabled()) {
        if (error) *error = i18n("no Konsole profile selected");
        return false;
    }
    const QString path = m_konsoleProfile->currentData().toString();
    if (path.isEmpty()) {
        if (error) *error = i18n("no Konsole profile selected");
        return false;
    }

    KSharedConfig::Ptr cfg = KSharedConfig::openConfig(path, KConfig::SimpleConfig);
    KConfigGroup appearance = cfg->group(QStringLiteral("Appearance"));
    KConfigGroup general    = cfg->group(QStringLiteral("General"));

    // Stash whatever font/antialiasing/grid the profile had before Phosphor
    // ever touched it, so "Restore" can put the user's own choice back.
    // Written once per profile: overwriting it on every preset load would,
    // after two presets, only be able to restore the *previous preset's*
    // choices — which is not what a user who spent time on their own profile
    // is asking to get back.
    const QString previousFont =
        appearance.readEntry(QStringLiteral("Font"), QString());
    const bool previousAA =
        appearance.readEntry(QStringLiteral("AntiAliasFonts"), true);
    const int previousCols = general.readEntry(QStringLiteral("TerminalColumns"), 0);
    const int previousRows = general.readEntry(QStringLiteral("TerminalRows"), 0);
    // -1 = "sleutel was afwezig": bij restore dan verwijderen in plaats van
    // Konsole's default terugschrijven alsof de gebruiker die ooit koos.
    const int previousMargin = general.readEntry(QStringLiteral("TerminalMargin"), -1);
    const QString previousScheme =
        appearance.readEntry(QStringLiteral("ColorScheme"), QString());
    KConfigGroup ours = KSharedConfig::openConfig(QStringLiteral("kwinrc"))
                            ->group(QLatin1String(CFG_GROUP));
    const QString backupKey =
        QStringLiteral("OriginalKonsoleFont-") + QFileInfo(path).fileName();
    const QString backupAAKey =
        QStringLiteral("OriginalKonsoleAA-") + QFileInfo(path).fileName();
    const QString backupColsKey =
        QStringLiteral("OriginalKonsoleCols-") + QFileInfo(path).fileName();
    const QString backupRowsKey =
        QStringLiteral("OriginalKonsoleRows-") + QFileInfo(path).fileName();
    const QString backupMarginKey =
        QStringLiteral("OriginalKonsoleMargin-") + QFileInfo(path).fileName();
    const QString backupSchemeKey =
        QStringLiteral("OriginalKonsoleScheme-") + QFileInfo(path).fileName();
    if (!previousFont.isEmpty() && !ours.hasKey(backupKey)) {
        ours.writeEntry(backupKey, previousFont);
        ours.writeEntry(backupAAKey, previousAA);
        ours.writeEntry(backupColsKey, previousCols);
        ours.writeEntry(backupRowsKey, previousRows);
        ours.writeEntry(backupMarginKey, previousMargin);
        ours.writeEntry(backupSchemeKey, previousScheme);
        ours.sync();
    }

    // Konsole slaat het font op als een QFont-beschrijvingsstring: familie,
    // puntgrootte, pixelgrootte, en daarna zeven velden (styleHint, weight,
    // style, underline, strikeout, fixedPitch, rawMode) die Konsole zelf ook
    // altijd op deze waarden schrijft. -1 betekent "dit veld niet gebruiken".
    //
    // fontPixelSize > 0 kiest de pixel-variant: puntgrootte -1, pixelgrootte
    // exact cel×k. Een puntgrootte levert een cel van "wat 14pt op deze
    // schermdichtheid toevallig is" — vrijwel nooit een geheel veelvoud van
    // het virtuele pixelraster, en precies daardoor vermorzelde het
    // resample-pad glyphs. Pixelgrootte maakt de cel exact.
    appearance.writeEntry(QStringLiteral("Font"), fontPixelSize > 0
        ? QStringLiteral("%1,-1,%2,5,50,0,0,0,0,0").arg(family).arg(fontPixelSize)
        : QStringLiteral("%1,%2,-1,5,50,0,0,0,0,0").arg(family).arg(pointSize));

    // These preset fonts are bitmap/pixel fonts (int10h, kreativekorp, C64 Pro
    // Mono, Topaz, ...) drawn as exact blocky pixels on purpose. Antialiasing
    // is the right default for a normal typeface, but on a pixel font it
    // blurs precisely the crisp edges the font was designed to have — the
    // opposite of what a CRT preset is going for. Off whenever Phosphor sets
    // the font; restoreKonsoleFont() below puts it back exactly as found.
    appearance.writeEntry(QStringLiteral("AntiAliasFonts"), false);

    // Machinekleuren. ensureColorScheme() schrijft/ververst het
    // "Phosphor <naam>"-schemabestand en geeft de naam terug; een preset
    // zonder schema-id laat het kleurenschema van het profiel met rust.
    if (!scheme.isEmpty()) {
        const QString schemeName = ensureColorScheme(scheme);
        if (!schemeName.isEmpty())
            appearance.writeEntry(QStringLiteral("ColorScheme"), schemeName);
    }

    // The other half of "the font renders on the normal desktop, not the
    // simulated screen": the CRT effect's pixel-scaling resamples whatever
    // Konsole already rendered, quantizing it down to the historical pixel
    // resolution — but that quantization grid means nothing to Konsole's own
    // renderer unless the *window* actually spans that many character cells.
    // Setting TerminalColumns/TerminalRows to the preset's real historical
    // text-mode grid (sourced per-machine, not derived from pixel size — see
    // PresetValues::targetCols/targetRows) means Konsole's own rendered
    // window lines up with the resolution the shader is simulating, instead
    // of an arbitrary modern terminal size the effect then has to quantize
    // through blindly. 0 (no documented grid for this machine) leaves
    // whatever size the profile already had alone.
    if (cols > 0 && rows > 0) {
        general.writeEntry(QStringLiteral("TerminalColumns"), cols);
        general.writeEntry(QStringLiteral("TerminalRows"), rows);
    } else if (ours.hasKey(backupColsKey)) {
        // Niet-authentiek: raster niet opleggen én een eerder opgelegd raster
        // ongedaan maken. Zonder deze tak blijft een profiel dat ooit op 40×25
        // gezet is daar hangen, en dan is "houd je eigen terminalgrootte" een
        // loze belofte: de gebruiker zit nog steeds in 40 kolommen.
        const int origCols = ours.readEntry(backupColsKey, 0);
        const int origRows = ours.readEntry(backupRowsKey, 0);
        if (origCols > 0 && origRows > 0) {
            general.writeEntry(QStringLiteral("TerminalColumns"), origCols);
            general.writeEntry(QStringLiteral("TerminalRows"), origRows);
        }
    }
    // Bij pixel-exacte rendering moet het content-gebied exact
    // cols×celW×k bij rows×celH×k zijn — Konsole's eigen rand (standaard
    // 1px, sleutel bevestigd in libkonsoleprivate) zou daar stille
    // fase-verschuiving bovenop leggen en de exacte reconstructie breken.
    if (fontPixelSize > 0)
        general.writeEntry(QStringLiteral("TerminalMargin"), 0);
    cfg->sync();

    if (cfg->accessMode() != KConfig::ReadWrite) {
        if (error) *error = i18n("%1 is not writable", path);
        return false;
    }
    return true;
}

bool RetroTermKCM::restoreKonsoleFont(QString *error)
{
    if (!m_konsoleProfile || !m_konsoleProfile->isEnabled()) {
        if (error) *error = i18n("no Konsole profile selected");
        return false;
    }
    const QString path = m_konsoleProfile->currentData().toString();
    if (path.isEmpty()) {
        if (error) *error = i18n("no Konsole profile selected");
        return false;
    }

    KConfigGroup ours = KSharedConfig::openConfig(QStringLiteral("kwinrc"))
                            ->group(QLatin1String(CFG_GROUP));
    const QString backupKey =
        QStringLiteral("OriginalKonsoleFont-") + QFileInfo(path).fileName();
    const QString backupAAKey =
        QStringLiteral("OriginalKonsoleAA-") + QFileInfo(path).fileName();
    const QString backupColsKey =
        QStringLiteral("OriginalKonsoleCols-") + QFileInfo(path).fileName();
    const QString backupRowsKey =
        QStringLiteral("OriginalKonsoleRows-") + QFileInfo(path).fileName();
    const QString backupMarginKey =
        QStringLiteral("OriginalKonsoleMargin-") + QFileInfo(path).fileName();
    const QString backupSchemeKey =
        QStringLiteral("OriginalKonsoleScheme-") + QFileInfo(path).fileName();
    const QString original = ours.readEntry(backupKey, QString());
    if (original.isEmpty()) {
        if (error) *error = i18n("this profile has no Phosphor-saved original font");
        return false;
    }
    // Default true/0 (Konsole's own default / "leave alone") if this profile
    // predates the AntiAliasFonts/grid backups added alongside
    // applyFontToKonsole()'s AA=false and TerminalColumns/Rows writes — an
    // existing OriginalKonsoleFont- backup from before those changes won't
    // have matching keys, and "restore to what Konsole normally does" is the
    // correct fallback, not "restore to off" or "restore to 0×0".
    const bool originalAA     = ours.readEntry(backupAAKey, true);
    const int  originalCols   = ours.readEntry(backupColsKey, 0);
    const int  originalRows   = ours.readEntry(backupRowsKey, 0);
    // -2 = "geen backup" (pre-dates margin support), -1 = "sleutel was afwezig"
    const int  originalMargin = ours.readEntry(backupMarginKey, -2);

    KSharedConfig::Ptr cfg = KSharedConfig::openConfig(path, KConfig::SimpleConfig);
    KConfigGroup appearance = cfg->group(QStringLiteral("Appearance"));
    KConfigGroup general    = cfg->group(QStringLiteral("General"));
    appearance.writeEntry(QStringLiteral("Font"), original);
    appearance.writeEntry(QStringLiteral("AntiAliasFonts"), originalAA);
    if (originalCols > 0 && originalRows > 0) {
        general.writeEntry(QStringLiteral("TerminalColumns"), originalCols);
        general.writeEntry(QStringLiteral("TerminalRows"), originalRows);
    }
    if (originalMargin >= 0)
        general.writeEntry(QStringLiteral("TerminalMargin"), originalMargin);
    else if (originalMargin == -1)
        general.deleteEntry(QStringLiteral("TerminalMargin"));
    // Kleurenschema: alleen aankomen als er ooit een backup gemaakt is
    // (hasKey), zodat een pre-scheme backup niets sloopt. Lege backup =
    // sleutel was afwezig -> verwijderen, Konsole valt dan op zijn eigen
    // default terug.
    if (ours.hasKey(backupSchemeKey)) {
        const QString originalScheme = ours.readEntry(backupSchemeKey, QString());
        if (originalScheme.isEmpty())
            appearance.deleteEntry(QStringLiteral("ColorScheme"));
        else
            appearance.writeEntry(QStringLiteral("ColorScheme"), originalScheme);
    }
    cfg->sync();

    // Backup weggooien: het profiel staat weer op de eigen keuze van de gebruiker,
    // dus de volgende preset-toepassing mag daar opnieuw een verse backup van maken.
    ours.deleteEntry(backupKey);
    ours.deleteEntry(backupAAKey);
    ours.deleteEntry(backupColsKey);
    ours.deleteEntry(backupRowsKey);
    ours.deleteEntry(backupMarginKey);
    ours.deleteEntry(backupSchemeKey);
    ours.sync();
    return true;
}

void RetroTermKCM::updateFontTabInfo()
{
    if (!m_fontRecommend) return;
    if (m_presetFont.isEmpty()) {
        m_fontRecommend->setText(i18n(
            "<i>No preset loaded yet — pick one on the Presets tab.</i>"));
        return;
    }
    const bool installed =
        QFontDatabase::families().contains(m_presetFont, Qt::CaseInsensitive);
    m_fontRecommend->setText(installed
        ? i18n("<b>%1</b>, %2pt (installed)", m_presetFont, m_presetFontSize)
        : i18n("<b>%1</b>, %2pt "
               "(<span style=\"color:#c0392b;\">not installed — run "
               "./install-fonts.sh</span>)", m_presetFont, m_presetFontSize));
}

// ══════════════════════════════════════════════════════════════════════════════
// applyPreset
// ══════════════════════════════════════════════════════════════════════════════
void RetroTermKCM::applyPreset(const PresetValues &p)
{
    if (auto *c = m_combos.value("phosphorType"))     c->setCurrentIndex(p.phosphorType);
    if (auto *c = m_combos.value("rasterizationMode"))c->setCurrentIndex(p.rasterizationMode);
    if (auto *c = m_combos.value("syncMode"))          c->setCurrentIndex(p.syncMode);
    auto s = [&](const QString &k, double v){ if (auto *r = m_params.value(k)) r->setValue(v); };
    s("phosphorAgeing",p.phosphorAgeing); s("colorTemperature",p.colorTemperature);
    s("phosphorPersistence",p.phosphorPersistence); s("screenCurvature",p.screenCurvature);
    s("vignetteIntensity",p.vignetteIntensity); s("ambientReflection",p.ambientReflection);
    s("scanlinesIntensity",p.scanlinesIntensity); s("scanlinesSharpness",p.scanlinesSharpness);
    s("bloom",p.bloom); s("glowingLine",p.glowingLine); s("brightness",p.brightness); s("contrast",p.contrast);
    s("staticNoise",p.staticNoise); s("jitter",p.jitter); s("horizontalSync",p.horizontalSync);
    s("flickering",p.flickering); s("ghostingIntensity",p.ghostingIntensity);
    s("chromaColor",p.chromaColor); s("saturationColor",p.saturationColor);
    s("rbgShift",p.rbgShift); s("characterSmearing",p.characterSmearing); s("burnIn",p.burnIn);
    if (auto *c = m_checks.value("warmupEnabled"))  c->setChecked(p.warmupEnabled);
    if (auto *c = m_checks.value("degaussOnStart")) c->setChecked(p.degaussOnStart);
    if (auto *s2 = m_spins.value("warmupDuration"))  s2->setValue(p.warmupDuration);
    if (auto *s2 = m_spins.value("degaussDuration")) s2->setValue(p.degaussDuration);

    // Pixel scaling. Preference order:
    //
    // 1. Integer zoom (k > 0) — the honest virtual screen. Only possible when
    //    the preset has a sourced grid whose cell divides the resolution
    //    cleanly; the font then gets written at exactly cell×k pixels and the
    //    shader reconstructs the virtual screen losslessly. The resample
    //    slider goes to 0: with integer zoom active the shader ignores it,
    //    and a nonzero-but-ignored slider reads as a lie.
    //
    // 2. No pixel scaling at all — for presets without a clean grid. The
    //    resample path used to be turned on at 1.0 here, which was worse than
    //    doing nothing: on an Amiga preset it point-sampled a 1850px-wide
    //    rendering down to 320 virtual pixels and turned every glyph into
    //    mush, while claiming to depict a machine whose text grid we
    //    explicitly could not source. These presets still get their font,
    //    palette, scanlines, phosphor and curvature — everything except a
    //    fake resolution that destroys the text to assert something unproven.
    const bool authentic = m_authenticSize && m_authenticSize->isChecked();
    const int  minCols   = m_minColumns ? m_minColumns->value() : 80;
    const int  k = zoomFor(p, minCols, authentic);
    if (m_integerZoomSpin) m_integerZoomSpin->setValue(k);
    if (p.targetResX > 0.0 && p.targetResY > 0.0) {
        if (m_targetResX) m_targetResX->setValue(p.targetResX);
        if (m_targetResY) m_targetResY->setValue(p.targetResY);
        if (m_pixelScaleRow) m_pixelScaleRow->setValue(0.0);
        if (m_sampleModeCombo) m_sampleModeCombo->setCurrentIndex(2);
    } else {
        if (m_pixelScaleRow) m_pixelScaleRow->setValue(0.0);
    }

    // The font is not ours to apply — see buildFontsTab(). Remember what this
    // preset wants so the Fonts tab can offer it, and write it straight into the
    // Konsole profile when the user has asked for that.
    m_presetFont     = p.font;
    m_presetFontSize = p.fontSize;
    // Alleen in authentieke modus krijgt Konsole het historische raster
    // opgelegd; anders houdt het venster zijn eigen afmetingen en volgt het
    // virtuele scherm daaruit (de shader deelt de contentgrootte door k).
    m_presetCols     = authentic ? p.targetCols : 0;
    m_presetRows     = authentic ? p.targetRows : 0;
    // Pixel-exact font size for the integer-zoom path: the historical cell
    // height (resY / rows — clean by zoomFor()'s divisibility check) times k.
    //
    // KNOWN LIMITATION: this assumes the machine's historical cell height is
    // also the font's cell height, which does not hold everywhere. Measured
    // with QFontMetricsF: "PxPlus IBM EGA 8x14" renders integrally at
    // pixelSize 16 (cell 8x14), not at the 14*k this picks; "PxPlus IBM VGA
    // 9x16" and "Px437 IBM 3270pc" have no integral size at all between 8 and
    // 64. Those presets still render far sharper than the resample path, but
    // are not demonstrably pixel-exact — the guarantee holds where the font is
    // integral at cell*k (C64 Pro Mono, Antiquarius, Mizuno, Px437 Wyse700b,
    // verified). The proper fix is to choose the pixel size by measuring the
    // font rather than deriving it from the historical cell, which also unlocks
    // integer zoom for machines with no documented grid (Topaz is integral at
    // 16, 18, 20, ...). Left as a follow-up rather than half-done here.
    m_presetFontPx   = (k > 0) ? ((int)p.targetResY / p.targetRows) * k : 0;
    m_presetScheme   = p.scheme;
    updateFontTabInfo();
    if (!p.font.isEmpty() && m_autoApplyFont && m_autoApplyFont->isChecked()
        && m_autoApplyFont->isEnabled()) {
        QString err;
        if (applyFontToKonsole(p.font, p.fontSize, m_presetCols, m_presetRows,
                               m_presetFontPx, p.scheme, &err) && m_fontStatus) {
            const QString gridPart = (m_presetCols > 0 && m_presetRows > 0)
                ? i18n(" and resized it to %1×%2 characters", m_presetCols, m_presetRows)
                : QString();
            const QString sizePart = (m_presetFontPx > 0)
                ? i18n("%1px (pixel-exact, zoom %2×)", m_presetFontPx, k)
                : i18n("%1pt", p.fontSize);
            m_fontStatus->setText(i18n(
                "<span style=\"color:#27ae60;\">Wrote <b>%1</b> %2 to %3%4.</span> "
                "Open a new Konsole tab or window to see it.",
                p.font, sizePart, m_konsoleProfile->currentText(), gridPart));
        } else if (m_fontStatus && !err.isEmpty()) {
            m_fontStatus->setText(i18n(
                "<span style=\"color:#c0392b;\">Could not write the profile: %1</span>",
                err));
        }
    }

    updatePresetInfo(p);
    markChanged();
}

// A KWin effect cannot reach into a terminal emulator's own profile settings —
// there is no API for it, and there shouldn't be — so the font a preset was
// designed for can only ever be a recommendation shown here, never applied
// automatically. QFontDatabase::families() is enough to say whether it's even
// installed, so at least that part doesn't require guessing.
void RetroTermKCM::updatePresetInfo(const PresetValues &p)
{
    if (!m_presetInfo) return;
    if (p.font.isEmpty()) { m_presetInfo->clear(); return; }

    const bool installed = QFontDatabase::families().contains(p.font, Qt::CaseInsensitive);
    const QString fontPart = installed
        ? i18n("Font: <b>%1</b>, %2pt (installed)", p.font, p.fontSize)
        : i18n("Font: <b>%1</b>, %2pt "
               "(<span style=\"color:#c0392b;\">not installed — run ./install-fonts.sh</span>)",
               p.font, p.fontSize);

    const QString resPart = (p.targetResX > 0.0 && p.targetResY > 0.0)
        ? i18n("Target resolution: %1×%2", (int)p.targetResX, (int)p.targetResY)
        : i18n("Target resolution: native (no pixel scaling)");

    // Shown *before* "Load preset" so it's clear what will actually happen:
    // which zoom gets picked, and whether the terminal will be resized to the
    // historical grid or keep its own size. Presets whose machine has no
    // documented text grid (bitmap-GUI systems, conflicting sources) can't do
    // integer zoom at all and say so rather than implying otherwise.
    const bool authentic = m_authenticSize && m_authenticSize->isChecked();
    const int  minCols   = m_minColumns ? m_minColumns->value() : 80;
    const int  k = zoomFor(p, minCols, authentic);
    QString gridPart;
    if (k <= 0) {
        gridPart = i18n("Pixel grid: no documented text grid for this machine — "
                        "no pixel scaling (font, palette and CRT effects still apply)");
    } else if (authentic) {
        gridPart = i18n("Pixel grid: zoom %1×, terminal resized to <b>%2×%3</b> "
                        "characters (authentic, but narrow for modern prompts)",
                        k, p.targetCols, p.targetRows);
    } else {
        // Estimate what the user's own window will hold at this zoom, so the
        // trade-off is a number on screen instead of a surprise afterwards.
        const int cellW = (int)p.targetResX / p.targetCols;
        QSize scr(1920, 1080);
        if (const QScreen *s = QGuiApplication::primaryScreen())
            scr = s->availableGeometry().size();
        const int cols = (scr.width() * 9 / 10) / (cellW * k);
        gridPart = i18n("Pixel grid: zoom %1×, terminal keeps its own size "
                        "(~%2 columns full-screen)", k, cols);
    }

    m_presetInfo->setText(fontPart + QStringLiteral("<br>") + resPart
                           + QStringLiteral("<br>") + gridPart);
}

// ══════════════════════════════════════════════════════════════════════════════
// load / save / defaults
// ══════════════════════════════════════════════════════════════════════════════
void RetroTermKCM::load()
{
    KConfigGroup cfg = KSharedConfig::openConfig(QStringLiteral("kwinrc"))
                           ->group(QStringLiteral("Effect-retro-terminal"));

    // Modus. "All windows" was removed as a selectable option; a config saved
    // before that change can still have targetMode==AllWindows on disk, and
    // there is no radio button left for setTargetMode() to check in that case —
    // fall back to Terminals, the closest thing that still exists.
    int savedModeRaw = cfg.readEntry("targetMode", static_cast<int>(TargetMode::Terminals));
    if (savedModeRaw == static_cast<int>(TargetMode::AllWindows))
        savedModeRaw = static_cast<int>(TargetMode::Terminals);
    const TargetMode savedMode = static_cast<TargetMode>(savedModeRaw);
    setTargetMode(savedMode);

    if (m_targetClasses)
        m_targetClasses->setText(cfg.readEntry("targetClasses", QString()));

    // The checklist's history only means something while the saved mode was
    // actually Terminals — targetClasses under Custom holds arbitrary WM_CLASS
    // text that has nothing to do with which detected terminals are checked.
    QMap<QString, bool> priorTerminals;
    if (savedMode == TargetMode::Terminals) {
        const QStringList saved = cfg.readEntry("targetClasses", QString())
                                       .split(QLatin1Char(','), Qt::SkipEmptyParts);
        for (const QString &s : saved)
            priorTerminals.insert(s.trimmed().toLower(), true);
    }
    rebuildTerminalList(priorTerminals);

    // Combos
    if (auto *c = m_combos.value("phosphorType"))      c->setCurrentIndex(cfg.readEntry("phosphorType",      1));
    if (auto *c = m_combos.value("rasterizationMode")) c->setCurrentIndex(cfg.readEntry("rasterizationMode", 1));
    if (auto *c = m_combos.value("syncMode"))           c->setCurrentIndex(cfg.readEntry("syncMode",          0));

    // Float params
    auto lf = [&](const QString &k, double d){ if (auto *r = m_params.value(k)) r->setValue(cfg.readEntry(k,d)); };
    lf("phosphorAgeing",0.05); lf("colorTemperature",7000); lf("phosphorPersistence",0.10);
    lf("screenCurvature",0.25); lf("vignetteIntensity",0.35); lf("ambientReflection",0.04);
    lf("scanlinesIntensity",0.35); lf("scanlinesSharpness",0.50);
    lf("bloom",0.55); lf("glowingLine",0.20); lf("brightness",0.50); lf("contrast",0.80);
    lf("staticNoise",0.08); lf("jitter",0.10); lf("horizontalSync",0.05); lf("flickering",0.08);
    lf("ghostingIntensity",0.00); lf("chromaColor",0.20); lf("saturationColor",0.20);
    lf("rbgShift",0.10); lf("characterSmearing",0.08); lf("burnIn",0.20);

    if (auto *c = m_checks.value("warmupEnabled"))  c->setChecked(cfg.readEntry("warmupEnabled",  true));
    if (auto *c = m_checks.value("degaussOnStart")) c->setChecked(cfg.readEntry("degaussOnStart", true));
    if (auto *s = m_spins.value("warmupDuration"))  s->setValue(cfg.readEntry("warmupDuration",  8.0));
    if (auto *s = m_spins.value("degaussDuration")) s->setValue(cfg.readEntry("degaussDuration", 2.5));

    // Pixel scaling
    if (m_pixelScaleRow)    m_pixelScaleRow->setValue(cfg.readEntry("pixelScale",  0.0));
    if (m_sampleModeCombo)  m_sampleModeCombo->setCurrentIndex(cfg.readEntry("sampleMode", 2));
    if (m_targetResX)       m_targetResX->setValue(cfg.readEntry("targetResX", 320.0));
    if (m_targetResY)       m_targetResY->setValue(cfg.readEntry("targetResY", 200.0));
    if (m_integerZoomSpin)  m_integerZoomSpin->setValue(cfg.readEntry("integerZoom", 0));
    if (m_minColumns)       m_minColumns->setValue(cfg.readEntry("minColumns", 80));
    if (m_authenticSize)    m_authenticSize->setChecked(cfg.readEntry("authenticSize", false));

    // Every setValue() above ran through markChanged(), so the live-preview timer
    // is now armed to write back the exact values just read and reload the effect
    // for nothing. Opening the KCM is not a change.
    if (m_previewTimer) m_previewTimer->stop();
    setNeedsSave(false);
}

void RetroTermKCM::save()
{
    KSharedConfig::Ptr cfg = KSharedConfig::openConfig(QStringLiteral("kwinrc"));
    KConfigGroup grp = cfg->group(QStringLiteral("Effect-retro-terminal"));

    // Modus → schrijf zowel het mode-getal als de afgeleide targetClasses
    const TargetMode mode = currentTargetMode();
    grp.writeEntry("targetMode", static_cast<int>(mode));

    QString classes;
    switch (mode) {
        case TargetMode::Off:        classes = QString(); break;
        case TargetMode::Terminals:  classes = checkedTerminalClasses().join(QLatin1Char(',')); break;
        case TargetMode::Custom:     classes = m_targetClasses ? m_targetClasses->text() : QString(); break;
        case TargetMode::AllWindows: classes = QString(); break;  // onbereikbaar via de UI; alleen voor een sluitende switch
    }
    grp.writeEntry("targetClasses", classes);

    // Combos
    if (auto *c = m_combos.value("phosphorType"))      grp.writeEntry("phosphorType",      c->currentIndex());
    if (auto *c = m_combos.value("rasterizationMode")) grp.writeEntry("rasterizationMode", c->currentIndex());
    if (auto *c = m_combos.value("syncMode"))           grp.writeEntry("syncMode",          c->currentIndex());

    // Float params
    auto sf = [&](const QString &k){ if (auto *r = m_params.value(k)) grp.writeEntry(k, r->value()); };
    sf("phosphorAgeing"); sf("colorTemperature"); sf("phosphorPersistence");
    sf("screenCurvature"); sf("vignetteIntensity"); sf("ambientReflection");
    sf("scanlinesIntensity"); sf("scanlinesSharpness");
    sf("bloom"); sf("glowingLine"); sf("brightness"); sf("contrast");
    sf("staticNoise"); sf("jitter"); sf("horizontalSync"); sf("flickering");
    sf("ghostingIntensity"); sf("chromaColor"); sf("saturationColor");
    sf("rbgShift"); sf("characterSmearing"); sf("burnIn");

    if (auto *c = m_checks.value("warmupEnabled"))  grp.writeEntry("warmupEnabled",  c->isChecked());
    if (auto *c = m_checks.value("degaussOnStart")) grp.writeEntry("degaussOnStart", c->isChecked());
    if (auto *s = m_spins.value("warmupDuration"))  grp.writeEntry("warmupDuration", s->value());
    if (auto *s = m_spins.value("degaussDuration")) grp.writeEntry("degaussDuration",s->value());

    // Pixel scaling
    if (m_pixelScaleRow)    grp.writeEntry("pixelScale",  m_pixelScaleRow->value());
    if (m_sampleModeCombo)  grp.writeEntry("sampleMode",  m_sampleModeCombo->currentIndex());
    if (m_targetResX)       grp.writeEntry("targetResX",  m_targetResX->value());
    if (m_targetResY)       grp.writeEntry("targetResY",  m_targetResY->value());
    if (m_integerZoomSpin)  grp.writeEntry("integerZoom", m_integerZoomSpin->value());
    // Alleen KCM-voorkeuren: de effect-kant leest ze niet, ze sturen alleen
    // welke k "Load preset" kiest en of het raster opgelegd wordt.
    if (m_minColumns)       grp.writeEntry("minColumns",    m_minColumns->value());
    if (m_authenticSize)    grp.writeEntry("authenticSize", m_authenticSize->isChecked());

    // This KCM is reached by clicking the effect's own config icon in Desktop
    // Effects, which implies it's already enabled there — but it's just as
    // reachable directly (search "Retro Terminal" in System Settings, or the
    // "phosphor" CLI), and someone who only ever opens *this* page could
    // configure all 30 parameters perfectly and still see nothing, because
    // KWin only auto-loads effects marked enabled here at its own startup.
    // "Off" mode is intentionally NOT wired to this flag — TargetMode::Off
    // means an empty target-class list, i.e. the effect stays loaded and
    // enabled but matches no window, exactly as m_modeOff's tooltip already
    // promises ("stays loaded but does nothing").
    KConfigGroup plugins = cfg->group(QStringLiteral("Plugins"));
    plugins.writeEntry("retro-termEnabled", true);

    cfg->sync();
    setNeedsSave(false);
}

void RetroTermKCM::defaults()
{
    setTargetMode(TargetMode::Terminals);
    if (m_targetClasses)
        m_targetClasses->clear();
    rebuildTerminalList({});   // lege prior-set -> elk gedetecteerd kandidaat start aangevinkt
    applyPreset(PresetValues{});
    setNeedsSave(true);
}

#include "retro_term_kcm.moc"
