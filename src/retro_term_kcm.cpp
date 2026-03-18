// SPDX-License-Identifier: GPL-2.0-or-later
// retro-term — KCM Implementation

#include "retro_term_kcm.h"

#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>
#include <KConfigGroup>

#include <QButtonGroup>
#include <QDBusInterface>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QScrollArea>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDebug>

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
void RetroTermKCM::buildPresets()
{
    auto p = [&](PresetValues pv) { m_presets.append(pv); };

    p({"Standaard (amber)","—",1,0.05,7000,0.10,0.25,0.35,0.04,1,0.35,0.50,0.55,0.20,0.50,0.80,0.08,0.10,0,0.05,0.08,0.00,0.20,0.20,0.10,0.08,0.20,true,8.0,true,2.5,"VT323",16,0.0,0.0});
    p({"IBM 2260 (1964)","1964 — Eerste IBM videoterminaal",2,0.55,8500,0.60,0.45,0.65,0.12,0,0.35,0.50,0.80,0.45,0.42,0.90,0.18,0.25,1,0.20,0.22,0.00,0.00,0.00,0.15,0.30,0.50,true,15.0,true,4.0,"Glass TTY VT220",16,640.0,250.0});
    p({"DEC GT40 (1972)","1972 — Vectorterminal PDP-11, P39",3,0.40,7800,0.80,0.20,0.55,0.08,0,0.35,0.50,0.85,0.60,0.38,0.85,0.10,0.15,0,0.08,0.15,0.00,0.05,0.10,0.06,0.15,0.35,true,12.0,true,3.5,"VT323",18,1024.0,768.0});
    p({"DEC VT100 (1978)","1978 — Dé referentieterminal",0,0.12,8000,0.18,0.22,0.38,0.05,1,0.40,0.55,0.52,0.22,0.52,0.82,0.06,0.08,0,0.05,0.07,0.00,0.05,0.08,0.05,0.10,0.22,true,9.0,true,2.5,"VT323",18,800.0,240.0});
    p({"IBM 3270 (1971)","1971 — IBM-mainframe blokmodus",0,0.18,8200,0.25,0.28,0.42,0.06,1,0.35,0.60,0.48,0.18,0.50,0.88,0.05,0.06,0,0.04,0.05,0.00,0.04,0.06,0.04,0.08,0.35,true,10.0,true,2.8,"PxPlus IBM 3270 Semi-Graphics",16});
    p({"Wyse WY-50 (1979)","1979 — UNIX-werkterminal, P1",0,0.08,8400,0.14,0.18,0.32,0.04,1,0.38,0.65,0.55,0.20,0.55,0.85,0.04,0.06,0,0.03,0.05,0.00,0.04,0.08,0.04,0.08,0.20,true,8.0,true,2.5,"PxPlus Wyse WY700b 2x",16,720.0,360.0});
    p({"Militair Radar (1965)","1965 — SAGE-radar, P39",3,0.50,7000,0.90,0.15,0.70,0.10,0,0.35,0.50,0.90,0.70,0.35,0.95,0.15,0.20,0,0.10,0.18,0.00,0.04,0.08,0.06,0.20,0.55,true,20.0,true,5.0,"Share Tech Mono",16,1024.0,1024.0});
    p({"Apple II (1977)","1977 — NTSC-TV, composite",2,0.30,6500,0.22,0.38,0.50,0.08,1,0.55,0.35,0.65,0.28,0.48,0.78,0.14,0.18,1,0.12,0.14,0.00,0.45,0.35,0.18,0.35,0.28,true,10.0,true,3.0,"Print Char 21",16,280.0,192.0});
    p({"Commodore 64 (1982)","1982 — VIC-II, PAL-TV",2,0.22,6200,0.18,0.35,0.45,0.07,1,0.50,0.38,0.60,0.25,0.50,0.80,0.12,0.14,1,0.10,0.12,0.00,0.55,0.40,0.14,0.28,0.25,true,9.0,true,2.8,"C64 Pro Mono",14,320.0,200.0});
    p({"ZX Spectrum (1982)","1982 — PAL-TV, attribuutcellen",2,0.25,6300,0.16,0.38,0.48,0.08,1,0.52,0.33,0.62,0.24,0.52,0.78,0.13,0.16,1,0.11,0.13,0.00,0.60,0.45,0.16,0.30,0.20,true,9.0,true,2.8,"VT323",14,256.0,192.0});
    p({"BBC Micro (1981)","1981 — Britse schoolcomputer",2,0.20,6600,0.16,0.30,0.44,0.07,1,0.50,0.38,0.60,0.22,0.50,0.80,0.10,0.12,1,0.09,0.11,0.00,0.55,0.40,0.14,0.25,0.22,true,9.0,true,2.8,"Bedstead",16,320.0,256.0});
    p({"Atari 400/800 (1979)","1979 — ANTIC/GTIA, NTSC-TV",2,0.26,6400,0.19,0.36,0.47,0.07,1,0.52,0.35,0.62,0.26,0.50,0.79,0.12,0.15,1,0.10,0.13,0.00,0.50,0.38,0.15,0.30,0.24,true,9.0,true,2.8,"Atari Classic",16,320.0,192.0});
    p({"IBM PC MDA (1981)","1981 — IBM 5151, P39",3,0.15,7500,0.30,0.20,0.40,0.06,1,0.42,0.60,0.58,0.28,0.50,0.88,0.05,0.07,0,0.04,0.06,0.00,0.04,0.06,0.05,0.10,0.30,true,8.0,true,2.5,"PxPlus IBM MDA",16,720.0,350.0});
    p({"IBM PC CGA (1981)","1981 — IBM 5153, composite",2,0.20,7000,0.15,0.25,0.38,0.06,1,0.45,0.50,0.55,0.22,0.52,0.83,0.08,0.10,0,0.06,0.08,0.00,0.65,0.45,0.10,0.15,0.22,true,8.0,true,2.5,"PxPlus IBM CGA",16,320.0,200.0});
    p({"IBM PC EGA (1984)","1984 — IBM 5154, 16 kleuren",2,0.14,7200,0.12,0.20,0.32,0.05,1,0.38,0.58,0.50,0.18,0.54,0.84,0.06,0.08,0,0.04,0.06,0.00,0.60,0.38,0.08,0.12,0.18,true,8.0,true,2.5,"PxPlus IBM EGA 8x14",14,640.0,350.0});
    p({"Tandy 1000 (1984)","1984 — Verbeterde CGA",2,0.22,6800,0.15,0.28,0.42,0.07,1,0.48,0.42,0.58,0.22,0.50,0.80,0.10,0.12,1,0.08,0.10,0.00,0.70,0.48,0.12,0.20,0.24,true,9.0,true,2.8,"PxPlus Tandy 1000",16,320.0,200.0});
    p({"IBM PS/2 VGA (1987)","1987 — De DOS-standaard",2,0.10,7400,0.10,0.15,0.28,0.04,1,0.32,0.62,0.45,0.15,0.55,0.85,0.05,0.06,0,0.03,0.05,0.00,0.65,0.35,0.07,0.10,0.15,true,7.0,true,2.2,"PxPlus IBM VGA 9x16",16,720.0,400.0});
    p({"Amiga 500 (1987)","1987 — PAL-TV of 1084S",2,0.15,6800,0.14,0.25,0.38,0.06,1,0.48,0.44,0.55,0.20,0.52,0.81,0.08,0.10,0,0.05,0.08,0.00,0.65,0.42,0.10,0.18,0.18,true,8.0,true,2.5,"Topaz Unicode",14,320.0,256.0});
    p({"Amiga WorkBench 2 (1990)","1990 — 1084S RGB-monitor",2,0.10,7000,0.10,0.20,0.30,0.05,1,0.40,0.52,0.48,0.16,0.56,0.83,0.06,0.08,0,0.04,0.06,0.00,0.68,0.38,0.08,0.14,0.14,true,7.0,true,2.2,"Topaz Unicode",14,640.0,256.0});
    p({"Apple Macintosh 128K (1984)","1984 — 9-inch Sony CRT b/w",2,0.35,9000,0.08,0.15,0.55,0.12,2,0.20,0.70,0.35,0.10,0.60,0.92,0.03,0.04,0,0.02,0.04,0.00,0.00,0.00,0.04,0.05,0.40,true,6.0,true,2.0,"Silkscreen",12,512.0,342.0});
    p({"NeXT Station (1990)","1990 — 1120×832 grijs",2,0.08,8000,0.06,0.08,0.22,0.05,0,0.35,0.50,0.32,0.08,0.62,0.88,0.02,0.03,0,0.02,0.03,0.00,0.00,0.00,0.03,0.04,0.12,true,5.0,true,1.8,"Lucida Console",13,1120.0,832.0});
    p({"SVGA Multisync (1992)","1992 — 800×600, shadow mask",2,0.06,7600,0.06,0.10,0.20,0.04,1,0.22,0.72,0.35,0.10,0.60,0.87,0.03,0.04,0,0.02,0.04,0.00,0.70,0.30,0.05,0.06,0.10,true,6.0,true,2.0,"Terminus",14,800.0,600.0});
    p({"Sony Trinitron (1997)","1989–1997 — Aperture-grille",2,0.05,7800,0.05,0.04,0.18,0.04,3,0.18,0.78,0.30,0.08,0.62,0.88,0.02,0.03,0,0.02,0.03,0.00,0.80,0.28,0.04,0.04,0.08,true,5.0,true,1.8,"Terminus",14,1024.0,768.0});
    p({"Teletext / Ceefax (1974)","1974 — PAL-TV, 8 kleuren",2,0.30,6200,0.20,0.40,0.52,0.09,1,0.62,0.28,0.70,0.30,0.46,0.76,0.20,0.24,2,0.18,0.20,0.12,0.80,0.55,0.22,0.40,0.28,true,12.0,true,3.5,"Bedstead",16,480.0,250.0});
    p({"HAL 9000 — 2001 (1968)","1968 (film) — Koud, precies",2,0.05,9300,0.05,0.06,0.25,0.06,0,0.35,0.50,0.38,0.12,0.62,0.90,0.02,0.02,0,0.01,0.02,0.00,0.00,0.00,0.03,0.03,0.10,true,4.0,true,1.5,"Share Tech Mono",14,0.0,0.0});
    p({"MU/TH/UR — Alien (1979)","1979 (film) — Verouderd groen",0,0.40,7500,0.45,0.12,0.65,0.10,0,0.35,0.50,0.82,0.50,0.38,0.92,0.12,0.15,1,0.12,0.14,0.00,0.04,0.06,0.08,0.18,0.45,true,15.0,true,4.0,"Share Tech Mono",16,0.0,0.0});
    p({"WOPR — WarGames (1983)","1983 (film) — NORAD, militair",3,0.35,7200,0.70,0.22,0.68,0.08,1,0.45,0.45,0.78,0.45,0.36,0.94,0.14,0.18,1,0.16,0.18,0.00,0.04,0.06,0.10,0.22,0.50,true,14.0,true,4.5,"VT323",18,1024.0,1024.0});
    p({"Blade Runner (1982)","1982 (film) — Dystopisch amber",1,0.35,5200,0.35,0.38,0.60,0.14,1,0.50,0.40,0.75,0.40,0.42,0.90,0.16,0.20,3,0.18,0.20,0.15,0.10,0.12,0.15,0.30,0.45,true,14.0,true,4.0,"Courier Prime",14,320.0,200.0});
    p({"Matrix Terminal (1999)","1999 (film) — Uitgeblust groen",0,0.50,7500,0.55,0.18,0.70,0.06,0,0.35,0.50,0.88,0.60,0.35,0.96,0.18,0.22,1,0.15,0.20,0.00,0.04,0.08,0.08,0.20,0.60,true,16.0,true,5.0,"VT323",20,640.0,350.0});
    p({"Minimaal (laag GPU)","— Subtiel, min. belasting",1,0.05,7000,0.10,0.10,0.15,0.04,1,0.20,0.50,0.20,0.08,0.55,0.85,0.00,0.00,0,0.00,0.00,0.00,0.20,0.20,0.00,0.00,0.10,false,8.0,false,2.5,"Terminus",14,0.0,0.0});

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
    // Fosfor: P4 wit — Motorola specificeerde P4 voor de PET-monitor
    // Curvature: 0.40 — kleine 9" bolronde buis, gelijkend aan Mac 128K maar ouder
    // Scans: 0.52 — 40×25 tekenmodus, 8×8 pixels per teken, duidelijke scanlijnen
    // Chroma: 0.00 — monochroomscherm, geen kleur
    // Font: Pet Me 2Y — pixel-perfecte recreatie van Commodore PET character ROM
    //        https://www.kreativekorp.com/swdownload/fonts/retro/petme.zip (gratis)
    p({"Commodore PET 2001 (1977)","1977 — Eerste Commodore, ingebouwde 9\" wit-fosfor CRT",
       2,0.35,8500,0.12, 0.40,0.55,0.10, 1,0.52,0.58,
       0.65,0.18,0.55,0.90, 0.06,0.08,0,0.04,0.06,0.00,
       0.00,0.00,0.05,0.12,0.38, true,11.0,true,3.0, "Pet Me 2Y",16});

    // TRS-80 Model I (1977)
    // Hardware: Motorola MC6847 video, composite naar gewone TV
    // Fosfor: P4 via composite TV — warm-wit door NTSC-encoding
    // Curvature: 0.35 — consumentenTV, matige bolrondheid
    // Smearing 0.28: composite artefacten waren berucht op de TRS-80
    // Chroma: 0.10 — bijna monochroom maar composite geeft lichte kleurtint
    // Font: Another Man's Treasure MIA — Model I character ROM
    //        https://www.kreativekorp.com/swdownload/fonts/retro/amtreasure.zip (gratis)
    p({"TRS-80 Model I (1977)","1977 — Tandy/RadioShack, composite naar TV, uppercase-only",
       2,0.28,6800,0.14, 0.35,0.48,0.08, 1,0.52,0.35,
       0.58,0.18,0.50,0.80, 0.14,0.15,1,0.10,0.12,0.00,
       0.10,0.12,0.14,0.28,0.22, true,9.0,true,2.8, "Another Man's Treasure MIA",16});

    // TRS-80 Color Computer (1980)
    // Hardware: MC6847, composite naar TV, later Tandy CM-2 monitor
    // Fosfor: P4 via composite, maar MC6847 had groen/zwart als standaard kleurpaar
    // Curvature: 0.35 — consumentenTV
    // Chroma: 0.60 — kleurmode was het onderscheidende kenmerk van de CoCo
    // Font: Hot CoCo — MC6847 character ROM voor CoCo I & II
    //        https://www.kreativekorp.com/swdownload/fonts/retro/hotcoco.zip (gratis)
    p({"TRS-80 Color Computer (1980)","1980 — CoCo, MC6847, composite kleur-TV",
       2,0.25,6500,0.16, 0.35,0.46,0.08, 1,0.50,0.32,
       0.60,0.22,0.48,0.78, 0.13,0.16,1,0.11,0.13,0.00,
       0.60,0.45,0.15,0.30,0.22, true,9.0,true,2.8, "Hot CoCo",16});

    // Kaypro II (1982)
    // Hardware: ingebouwde 9" green-phosphor CRT, Z80, CP/M
    // Fosfor: P1 helder groen — Kaypro specificeerde P1 voor hun ingebouwde monitor
    // Curvature: 0.42 — kleine 9" buis, meer dan een 12" monitor
    // Scans: 0.44 — 80×24 tekenmodus, redelijk zichtbare scanlijnen
    // Chroma: 0.00 — monochroomscherm, geen kleur
    // Font: PxPlus Kaypro 2000 — authentieke Kaypro character ROM, int10h pack
    //        https://int10h.org/oldschool-pc-fonts/download/ (CC BY-SA 4.0)
    p({"Kaypro II (1982)","1982 — Draagbare CP/M, ingebouwde 9\" groene CRT",
       0,0.14,8100,0.16, 0.42,0.52,0.07, 1,0.44,0.60,
       0.58,0.22,0.50,0.86, 0.05,0.07,0,0.04,0.06,0.00,
       0.00,0.00,0.04,0.08,0.25, true,9.0,true,2.8, "PxPlus Kaypro 2000",16});

    // Compaq Portable (1982)
    // Hardware: ingebouwde 9" amber CRT, eerste IBM-compatibele draagbare
    // Fosfor: P3 amber — Compaq koos amber voor de ingebouwde portable monitor
    // Curvature: 0.40 — kleine 9" spherische buis
    // Ageing: 0.18 — ambermonitoren vergeelden snel bij veel gebruik
    // Chroma: 0.00 — monochroom amber scherm
    // Font: PxPlus Compaq — authentieke Compaq BIOS font, int10h pack
    //        https://int10h.org/oldschool-pc-fonts/download/ (CC BY-SA 4.0)
    p({"Compaq Portable (1982)","1982 — Eerste IBM-compatibele draagbare, 9\" amber CRT",
       1,0.18,5800,0.20, 0.40,0.50,0.08, 1,0.42,0.55,
       0.60,0.25,0.52,0.88, 0.06,0.08,0,0.04,0.06,0.00,
       0.00,0.00,0.05,0.10,0.28, true,9.0,true,2.8, "PxPlus CompaqPort",16});

    // DEC Rainbow 100 (1982)
    // Hardware: VR201 monitor, P1 groen fosfor, 80×24, CP/M en DOS
    // Fosfor: P1 — DEC VR201 gebruikt P1 helder groen
    // Curvature: 0.18 — 12" monitor, minder dan 9", DEC-kwaliteitsglas
    // Scans: 0.38 — hogere kwaliteit dan IBM CGA, scherpere rasterweergave
    // Chroma: 0.00 — monochroomscherm
    // Font: PxPlus DEC Rainbow — authentieke Rainbow BIOS font, int10h pack
    //        https://int10h.org/oldschool-pc-fonts/download/ (CC BY-SA 4.0)
    p({"DEC Rainbow 100 (1982)","1982 — DEC's CP/M+DOS hybride, VR201 groene monitor",
       0,0.10,8200,0.14, 0.18,0.35,0.05, 1,0.38,0.62,
       0.52,0.18,0.55,0.86, 0.04,0.05,0,0.03,0.04,0.00,
       0.00,0.00,0.04,0.08,0.18, true,8.0,true,2.5, "PxPlus DEC Rainbow100-8x10",16});

    // TeleVideo 925 (1982)
    // Hardware: 12" green-phosphor CRT, 80×24, UNIX/CP/M kantoor-terminal
    // Fosfor: P1 — TeleVideo gebruikte standaard P1 groen in de 9xx-serie
    // Curvature: 0.20 — 12" monitor, vergelijkbaar met Wyse
    // Ageing: 0.07 — kantoor-terminals werden goed onderhouden
    // Chroma: 0.00 — monochroomscherm
    // Font: PxPlus TeleVideo TVI-925 — int10h pack
    //        https://int10h.org/oldschool-pc-fonts/download/ (CC BY-SA 4.0)
    p({"TeleVideo TVI-925 (1982)","1982 — Populaire UNIX-terminal, 12\" P1 groen",
       0,0.07,8300,0.12, 0.20,0.34,0.04, 1,0.36,0.66,
       0.50,0.16,0.56,0.87, 0.04,0.05,0,0.03,0.04,0.00,
       0.00,0.00,0.04,0.07,0.18, true,8.0,true,2.2, "PxPlus TeleVideo TVI-925",16});

    // Apple Lisa (1983)
    // Hardware: Sony 12" CRT, P4 wit fosfor, 720×364, eerste GUI-computer van Apple
    // Fosfor: P4 wit — Sony specificeerde P4 voor dit scherm
    // Curvature: 0.14 — Sony flat-face CRT, opvallend weinig curvature voor 1983
    // Scans: mode 0 (geen scanlines) — hoge resolutie voor zijn tijd, amper zichtbaar
    // Chroma: 0.00 — monochroom zwart-wit scherm
    // Font: LisaTerminal Paper — LisaTerminal bitmap font
    //        https://www.kreativekorp.com/swdownload/fonts/retro/lisa1.zip (gratis)
    p({"Apple Lisa (1983)","1983 — Eerste Apple GUI-computer, Sony 12\" b/w CRT",
       2,0.10,8800,0.06, 0.14,0.38,0.08, 0,0.15,0.70,
       0.38,0.08,0.62,0.91, 0.02,0.03,0,0.02,0.03,0.00,
       0.00,0.00,0.03,0.04,0.30, true,6.0,true,2.0, "LisaTerminal Paper",13});

    // Amstrad PC1512 (1986)
    // Hardware: CTM640 kleurenmonitor of GT65 groene monitor
    // Fosfor: P4 via shadow mask voor CTM640 kleur
    // Kleurtemperatuur: 6800K — CTM640 had een bekende groene tint bij wit
    // Curvature: 0.22 — 14" CGA-monitor, vergelijkbaar met IBM 5153
    // Chroma: 0.70 — kleurenmonitor, Amstrad had 16 CGA-kleuren
    // Font: PxPlus Amstrad PC — int10h pack
    //        https://int10h.org/oldschool-pc-fonts/download/ (CC BY-SA 4.0)
    p({"Amstrad PC1512 (1986)","1986 — Goedkope Britse IBM-kloon, CTM640 kleurenmonitor",
       2,0.16,6800,0.12, 0.22,0.36,0.06, 1,0.42,0.48,
       0.52,0.20,0.52,0.82, 0.07,0.09,0,0.06,0.08,0.00,
       0.70,0.42,0.08,0.14,0.18, true,8.0,true,2.5, "PxPlus Amstrad PC-2y",16});

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
       0.00,0.00,0.03,0.04,0.15, true,6.0,true,1.8, "Project Jason",14});

    // NEC APC III (1983)
    // Hardware: Japanse professionele PC, 12" groen-fosfor monitor, 640×400
    // Fosfor: P1 — NEC gebruikte P1 in hun professionele monitorserie
    // Curvature: 0.16 — 12" professionele NEC-monitor, laag bolrondheid
    // Scans: 0.32 — hoge resolutie, 640×400, minder zichtbare scanlijnen dan 200-lijn systemen
    // Chroma: 0.00 — monochroomscherm, professioneel gebruik
    // Font: PxPlus NEC APC3 8x16 — int10h pack
    //        https://int10h.org/oldschool-pc-fonts/download/ (CC BY-SA 4.0)
    p({"NEC APC III (1983)","1983 — Japanse professionele PC, 12\" P1 groen, 640×400",
       0,0.08,8400,0.10, 0.16,0.30,0.04, 1,0.32,0.68,
       0.46,0.14,0.58,0.88, 0.03,0.04,0,0.02,0.04,0.00,
       0.00,0.00,0.04,0.06,0.14, true,7.0,true,2.0, "PxPlus NEC APC3 8x16",16});

    // HP 150 Touchscreen (1983)
    // Hardware: ingebouwde 9" CRT, P4 wit fosfor, HP-kwaliteitsglas
    // Fosfor: P4 — HP specificeerde P4 wit voor de 150's ingebouwde monitor
    // Curvature: 0.22 — 9" maar HP gebruikte beter glas dan Kaypro/Compaq; minder bol
    // Scans: 0.40 — 80×24 tekst, zichtbare scanlijnen maar scherper dan consumer
    // Chroma: 0.00 — monochroom wit scherm
    // Font: PxPlus HP 150 — int10h pack
    //        https://int10h.org/oldschool-pc-fonts/download/ (CC BY-SA 4.0)
    p({"HP 150 Touchscreen (1983)","1983 — HP's eerste touchscreen PC, 9\" b/w CRT",
       2,0.08,8700,0.07, 0.22,0.40,0.06, 1,0.40,0.62,
       0.48,0.14,0.60,0.89, 0.03,0.04,0,0.02,0.03,0.00,
       0.00,0.00,0.04,0.06,0.24, true,7.0,true,2.0, "PxPlus HP 150",16});

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
       0.75,0.45,0.07,0.10,0.14, true,7.0,true,2.0, "Shaston 320",14});

    // Sharp MZ-700 (1982)
    // Hardware: ingebouwde 12" monitor (MZ-1D05), P4 wit of P1 groen afhankelijk van regio
    // Fosfor: P4 wit (Europese versie)
    // Curvature: 0.26 — 12" bolronde CRT, Japanse kwaliteit maar niet premium
    // Karakteristiek: monochroom maar scherp, Japans kantoor-gebruik
    // Font: Mizuno — Sharp MZ character ROM
    //        https://www.kreativekorp.com/swdownload/fonts/retro/mizuno.zip (gratis)
    p({"Sharp MZ-700 (1982)","1982 — Japanse Sharp, 12\" wit-fosfor CRT",
       2,0.12,8400,0.10, 0.26,0.40,0.06, 1,0.40,0.58,
       0.50,0.16,0.55,0.87, 0.05,0.06,0,0.03,0.05,0.00,
       0.00,0.00,0.04,0.08,0.22, true,8.0,true,2.5, "Mizuno",14});

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
       0.62,0.22,0.48,0.78, 0.18,0.18,1,0.14,0.14,0.00,
       0.55,0.38,0.16,0.35,0.24, true,10.0,true,3.0, "Antiquarius",16});

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

void RetroTermKCM::markChanged() { setNeedsSave(true); }

// ── Target mode ───────────────────────────────────────────────────────────────
void RetroTermKCM::setTargetMode(TargetMode mode)
{
    if (m_customRow)
        m_customRow->setVisible(mode == TargetMode::Custom);

    auto btn = [&]() -> QRadioButton * {
        switch (mode) {
            case TargetMode::Off:        return m_modeOff;
            case TargetMode::Terminals:  return m_modeTerminals;
            case TargetMode::AllWindows: return m_modeAll;
            case TargetMode::Custom:     return m_modeCustom;
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

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // 1. MODUS — meest prominente keuze, helemaal bovenaan
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    {
        auto *mgb = new QGroupBox(i18n("Op welke vensters werkt het effect?"));
        mgb->setStyleSheet(
            QStringLiteral("QGroupBox { font-weight:bold; border:2px solid palette(highlight);"
                           " border-radius:4px; margin-top:8px; padding-top:6px; }"
                           "QGroupBox::title { subcontrol-origin:margin; left:8px; }"));
        auto *mvbox = new QVBoxLayout(mgb);
        mvbox->setSpacing(8);

        m_modeGroup = new QButtonGroup(this);

        m_modeOff = new QRadioButton(
            i18n("Uit  —  geen venster krijgt het effect"));
        m_modeOff->setToolTip(i18n(
            "Het effect is geladen maar doet niets. "
            "Handig om tijdelijk uit te zetten zonder de plugin te deactiveren."));
        m_modeGroup->addButton(m_modeOff, static_cast<int>(TargetMode::Off));

        m_modeTerminals = new QRadioButton(
            i18n("Alleen terminals  —  Konsole, Yakuake, kitty, Alacritty, …"));
        m_modeTerminals->setToolTip(i18n(
            "Werkt op alle bekende terminal-emulators:\n%1")
            .arg(QString::fromLatin1(KNOWN_TERMINALS)));
        m_modeGroup->addButton(m_modeTerminals, static_cast<int>(TargetMode::Terminals));

        m_modeAll = new QRadioButton(
            i18n("Alle vensters  —  elk venster op het scherm wordt retro"));
        m_modeAll->setToolTip(i18n(
            "Elk venster dat KWin tekent krijgt het CRT-effect. "
            "Ziet er spectaculair uit, maar is zwaarder op de GPU."));
        m_modeGroup->addButton(m_modeAll, static_cast<int>(TargetMode::AllWindows));

        m_modeCustom = new QRadioButton(
            i18n("Aangepast  —  kies zelf welke applicaties"));
        m_modeCustom->setToolTip(i18n(
            "Geef een kommagescheiden lijst van WM_CLASS namen op.\n"
            "Gebruik 'xprop WM_CLASS' om de klasse van een venster te vinden."));
        m_modeGroup->addButton(m_modeCustom, static_cast<int>(TargetMode::Custom));

        mvbox->addWidget(m_modeOff);
        mvbox->addWidget(m_modeTerminals);
        mvbox->addWidget(m_modeAll);
        mvbox->addWidget(m_modeCustom);

        // Aangepast invoerveld — verborgen tenzij Custom geselecteerd
        m_customRow = new QWidget;
        auto *crow = new QHBoxLayout(m_customRow);
        crow->setContentsMargins(28, 2, 0, 2);
        crow->addWidget(new QLabel(i18n("Vensterklassen:")));
        m_targetClasses = new QLineEdit;
        m_targetClasses->setPlaceholderText(
            i18n("bijv. konsole,firefox,code  (kleine letters, kommagescheiden)"));
        m_targetClasses->setToolTip(i18n(
            "WM_CLASS namen. Zoek de naam op via:\n"
            "  xprop WM_CLASS  (klik daarna op het venster)\n"
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

        outerVBox->addWidget(mgb);
    }

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // 2. PRESET-SELECTOR
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    {
        auto *pgb = new QGroupBox(i18n("Historisch preset"));
        auto *pfl = new QFormLayout(pgb);

        m_presetCombo = new QComboBox;
        m_presetCombo->addItem(i18n("— Kies een preset —"));
        for (const auto &pv : m_presets)
            m_presetCombo->addItem(
                pv.era.isEmpty() ? pv.name
                                 : QStringLiteral("%1  (%2)").arg(pv.name, pv.era));

        m_applyPreset = new QPushButton(i18n("Preset laden"));
        m_applyPreset->setEnabled(false);

        m_applyKWin = new QPushButton(i18n("✓  Toepassen & KWin herladen"));
        m_applyKWin->setToolTip(i18n(
            "Slaat alle instellingen op en herlaadt KWin "
            "zodat het effect onmiddellijk actief wordt."));

        pfl->addRow(i18n("Preset:"), m_presetCombo);
        auto *btnRow = new QHBoxLayout;
        btnRow->addWidget(m_applyPreset);
        btnRow->addWidget(m_applyKWin);
        btnRow->addStretch();
        pfl->addRow(QString(), btnRow);

        connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int idx) { m_applyPreset->setEnabled(idx > 0); });
        connect(m_applyPreset, &QPushButton::clicked, this, [this] {
            const int idx = m_presetCombo->currentIndex();
            if (idx > 0) applyPreset(m_presets.at(idx - 1));
        });
        connect(m_applyKWin, &QPushButton::clicked, this, [this] {
            save();
            QDBusInterface kwin(QStringLiteral("org.kde.KWin"),
                                QStringLiteral("/KWin"),
                                QStringLiteral("org.kde.KWin"));
            kwin.call(QStringLiteral("reconfigure"));
        });

        outerVBox->addWidget(pgb);
    }

    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // 3. SHADER-PARAMETERS (scrollbaar)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    auto *sc   = new QWidget;
    auto *vbox = new QVBoxLayout(sc);
    vbox->setSpacing(10);
    scroll->setWidget(sc);
    outerVBox->addWidget(scroll, 1);

    { // Fosfory
        QFormLayout *fl = nullptr;
        auto *gb = makeGroup(i18n("Fosfory en kleur"), fl);
        auto *phc = new QComboBox;
        phc->addItem(i18n("P1 — Helder groen (VT100, Wyse)"));
        phc->addItem(i18n("P3 — Amber (IBM 3101, vroege terminals)"));
        phc->addItem(i18n("P4 — Wit/room (MDA, Mac, VGA)"));
        phc->addItem(i18n("P39 — Radar groen (lange nagloed)"));
        fl->addRow(i18n("Fosforytype:"), phc);
        m_combos["phosphorType"] = phc;
        connect(phc, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &RetroTermKCM::markChanged);
        addParam(fl, i18n("Vergeeling:"),        0.0,  1.0,  0.01, "phosphorAgeing",      i18n("0=nieuw, 1=verweerd geel/bruin"));
        addParam(fl, i18n("Kleurtemperatuur (K):"),3000,9300,50,   "colorTemperature",   i18n("3000=warm geel, 9300=koud blauw-wit"));
        addParam(fl, i18n("Nagloed:"),           0.0,  1.0,  0.01, "phosphorPersistence", i18n("Hoe lang tekens na weergave zichtbaar blijven"));
        vbox->addWidget(gb);
    }
    { // Geometrie
        QFormLayout *fl = nullptr;
        auto *gb = makeGroup(i18n("Schermgeometrie"), fl);
        addParam(fl, i18n("Barreldistorsie:"), 0.0, 1.0,  0.01, "screenCurvature",   i18n("0=vlak, 1=sterk gebogen"));
        addParam(fl, i18n("Vignette:"),        0.0, 1.0,  0.01, "vignetteIntensity", i18n("Randverduistering"));
        addParam(fl, i18n("Glasreflectie:"),   0.0, 0.30, 0.005,"ambientReflection", i18n("Spiegelreflectie van het schermglas"));
        vbox->addWidget(gb);
    }
    { // Scanlines
        QFormLayout *fl = nullptr;
        auto *gb = makeGroup(i18n("Scanlines / Rasterisatie"), fl);
        auto *rc = new QComboBox;
        rc->addItem(i18n("Geen")); rc->addItem(i18n("Scanlines (klassiek)"));
        rc->addItem(i18n("Pixelraster (shadow mask)")); rc->addItem(i18n("Sub-pixel RGB (aperture grille)"));
        fl->addRow(i18n("Modus:"), rc);
        m_combos["rasterizationMode"] = rc;
        connect(rc, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &RetroTermKCM::markChanged);
        addParam(fl, i18n("Intensiteit:"), 0.0, 1.0, 0.01, "scanlinesIntensity", i18n("Hoe donker de tussenruimten zijn"));
        addParam(fl, i18n("Scherpte:"),    0.0, 1.0, 0.01, "scanlinesSharpness", i18n("0=zacht, 1=scherp"));
        vbox->addWidget(gb);
    }
    { // Bloom
        QFormLayout *fl = nullptr;
        auto *gb = makeGroup(i18n("Bloom en gloed"), fl);
        addParam(fl, i18n("Bloom:"),      0.0, 1.0, 0.01, "bloom",       i18n("Gloedhalo (13-tap Gaussian)"));
        addParam(fl, i18n("Lijngloed:"), 0.0, 1.0, 0.01, "glowingLine", i18n("Horizontale lijngloed"));
        addParam(fl, i18n("Helderheid:"),0.0, 1.0, 0.01, "brightness",  i18n("Algehele helderheid"));
        addParam(fl, i18n("Contrast:"),  0.0, 1.0, 0.01, "contrast",    i18n("Contrast"));
        vbox->addWidget(gb);
    }
    { // Ruis
        QFormLayout *fl = nullptr;
        auto *gb = makeGroup(i18n("Ruis en synchronisatie-artefacten"), fl);
        addParam(fl, i18n("Statische ruis:"), 0.0, 1.0,  0.01, "staticNoise",       i18n("Granulair beeldruis"));
        addParam(fl, i18n("Jitter:"),         0.0, 1.0,  0.01, "jitter",            i18n("Per-pixel horizontale verschuiving"));
        auto *sc2 = new QComboBox;
        sc2->addItem(i18n("Stabiel")); sc2->addItem(i18n("Sinusdrift")); sc2->addItem(i18n("Rolling scan")); sc2->addItem(i18n("Ghosting"));
        fl->addRow(i18n("Sync-modus:"), sc2);
        m_combos["syncMode"] = sc2;
        connect(sc2, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &RetroTermKCM::markChanged);
        addParam(fl, i18n("Sync-intensiteit:"),  0.0, 1.0,  0.01, "horizontalSync",    i18n("Sterkte van het artefact"));
        addParam(fl, i18n("Flikkering:"),        0.0, 1.0,  0.01, "flickering",        i18n("50/60Hz helderheidsflikkering"));
        addParam(fl, i18n("Ghost-intensiteit:"), 0.0, 0.5,  0.005,"ghostingIntensity", i18n("Frame-echo (alleen bij syncMode Ghosting)"));
        vbox->addWidget(gb);
    }
    { // Kleur
        QFormLayout *fl = nullptr;
        auto *gb = makeGroup(i18n("Kleur en optische aberraties"), fl);
        addParam(fl, i18n("Kleurretentie:"),    0.0, 1.0, 0.01, "chromaColor",       i18n("0=grijsschaal, 1=volledige kleur"));
        addParam(fl, i18n("Verzadiging:"),      0.0, 1.0, 0.01, "saturationColor",   i18n("Extra kleurverzadiging"));
        addParam(fl, i18n("Chrom. aberratie:"), 0.0, 1.0, 0.01, "rbgShift",          i18n("RGB-kanalen horizontaal verschoven"));
        addParam(fl, i18n("Tekenvervaging:"),   0.0, 1.0, 0.01, "characterSmearing", i18n("Horizontale tekenvervaging"));
        addParam(fl, i18n("Burn-in:"),          0.0, 1.0, 0.01, "burnIn",            i18n("Schermcentrum iets helderder"));
        vbox->addWidget(gb);
    }
    { // Animaties
        QFormLayout *fl = nullptr;
        auto *gb = makeGroup(i18n("Animaties"), fl);
        auto *wuc = new QCheckBox(i18n("CRT warmup-animatie bij openen venster"));
        fl->addRow(wuc); m_checks["warmupEnabled"] = wuc;
        connect(wuc, &QCheckBox::checkStateChanged, this, &RetroTermKCM::markChanged);
        auto *wus = new QDoubleSpinBox; wus->setRange(0.5,30.0); wus->setSuffix(i18n(" sec")); wus->setSingleStep(0.5); wus->setDecimals(1);
        fl->addRow(i18n("Warmup-duur:"), wus); m_spins["warmupDuration"] = wus;
        connect(wus, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &RetroTermKCM::markChanged);
        auto *dgc = new QCheckBox(i18n("Degauss-animatie bij openen venster"));
        fl->addRow(dgc); m_checks["degaussOnStart"] = dgc;
        connect(dgc, &QCheckBox::checkStateChanged, this, &RetroTermKCM::markChanged);
        auto *dgs = new QDoubleSpinBox; dgs->setRange(0.5,10.0); dgs->setSuffix(i18n(" sec")); dgs->setSingleStep(0.5); dgs->setDecimals(1);
        fl->addRow(i18n("Degauss-duur:"), dgs); m_spins["degaussDuration"] = dgs;
        connect(dgs, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &RetroTermKCM::markChanged);
        vbox->addWidget(gb);
    }


    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // PIXEL SCALING
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    {
        QFormLayout *fl = nullptr;
        auto *gb = makeGroup(i18n("Pixel scaling — originele schermresolutie simuleren"), fl);
        gb->setToolTip(i18n(
            "Schaalt het vensterbeeld terug naar de originele pixelresolutie van het "
            "historische systeem en vergroot het weer op naar volledig scherm.\n"
            "0.0 = geen schaling (modern scherm)\n"
            "1.0 = pixel-exact origineel (blokpixels op ware grootte)\n"
            "Tussenwaarden mengen beide weergaven."));

        // Hoofdslider
        m_pixelScaleRow = new ParamRow(
            i18n("Pixel schaal:"), 0.0, 1.0, 0.01,
            i18n("0.0 = geen schaling  |  1.0 = pixel-exact origineel"),
            gb);
        fl->addRow(i18n("Pixel schaal:"), m_pixelScaleRow);
        connect(m_pixelScaleRow, &ParamRow::valueChanged,
                this, &RetroTermKCM::markChanged);

        // Sampling-modus combobox
        m_sampleModeCombo = new QComboBox;
        m_sampleModeCombo->addItem(i18n("Nearest-neighbour  —  harde blokpixels (klassiek)"));
        m_sampleModeCombo->addItem(i18n("Bilineair  —  zacht, goed voor tussenwaarden"));
        m_sampleModeCombo->addItem(i18n("Sharp bilineair  —  CRT-typisch: scherpe randen, geen aliasing"));
        m_sampleModeCombo->setCurrentIndex(2);
        m_sampleModeCombo->setToolTip(i18n(
            "Nearest: echte blokpixels zoals op de originele hardware.\n"
            "Bilineair: zachte interpolatie, goed bij pixelScale 0.3–0.7.\n"
            "Sharp bilineair: simuleert de Gaussiaanse elektronenbundel van een CRT — "
            "scherpe pixelgrenzen maar zonder harde tanding. Aanbevolen."));
        fl->addRow(i18n("Sampling:"), m_sampleModeCombo);
        connect(m_sampleModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &RetroTermKCM::markChanged);

        // Originele resolutie-invoer
        // Automatisch ingevuld bij preset laden, handmatig aanpasbaar
        m_targetResRow = new QWidget;
        auto *rhl = new QHBoxLayout(m_targetResRow);
        rhl->setContentsMargins(0, 0, 0, 0);
        rhl->addWidget(new QLabel(i18n("Breedte:")));
        m_targetResX = new QDoubleSpinBox;
        m_targetResX->setRange(40, 3840);
        m_targetResX->setDecimals(0);
        m_targetResX->setSingleStep(8);
        m_targetResX->setValue(320);
        m_targetResX->setToolTip(i18n("Originele horizontale resolutie van het historische systeem (pixels)"));
        rhl->addWidget(m_targetResX);
        rhl->addSpacing(12);
        rhl->addWidget(new QLabel(i18n("Hoogte:")));
        m_targetResY = new QDoubleSpinBox;
        m_targetResY->setRange(24, 2160);
        m_targetResY->setDecimals(0);
        m_targetResY->setSingleStep(8);
        m_targetResY->setValue(200);
        m_targetResY->setToolTip(i18n("Originele verticale resolutie van het historische systeem (pixels)"));
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

        fl->addRow(i18n("Originele res.:"), m_targetResRow);

        connect(m_targetResX, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &RetroTermKCM::markChanged);
        connect(m_targetResY, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &RetroTermKCM::markChanged);

        // Info-label
        auto *info = new QLabel(i18n(
            "<small><i>Tip: laad een preset — de originele resolutie wordt automatisch ingevuld.<br>"
            "Bij pixelScale = 0.0 heeft de resolutie geen effect.</i></small>"));
        info->setWordWrap(true);
        fl->addRow(QString(), info);

        vbox->addWidget(gb);
    }
    vbox->addStretch();
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

    // Pixel scaling: fill in original resolution if the preset specifies one
    if (p.targetResX > 0.0 && p.targetResY > 0.0) {
        if (m_targetResX) m_targetResX->setValue(p.targetResX);
        if (m_targetResY) m_targetResY->setValue(p.targetResY);
    }
    markChanged();
}

// ══════════════════════════════════════════════════════════════════════════════
// load / save / defaults
// ══════════════════════════════════════════════════════════════════════════════
void RetroTermKCM::load()
{
    KConfigGroup cfg = KSharedConfig::openConfig(QStringLiteral("kwinrc"))
                           ->group(QStringLiteral("Effect-retro-terminal"));

    // Modus
    const int savedMode = cfg.readEntry("targetMode",
                                        static_cast<int>(TargetMode::Terminals));
    setTargetMode(static_cast<TargetMode>(savedMode));
    if (m_targetClasses)
        m_targetClasses->setText(cfg.readEntry("targetClasses",
            QString::fromLatin1(KNOWN_TERMINALS)));

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
    if (m_pixelScaleRow)   m_pixelScaleRow->setValue(cfg.readEntry("pixelScale",  0.0));
    if (m_sampleModeCombo) m_sampleModeCombo->setCurrentIndex(cfg.readEntry("sampleMode", 2));
    if (m_targetResX)      m_targetResX->setValue(cfg.readEntry("targetResX", 320.0));
    if (m_targetResY)      m_targetResY->setValue(cfg.readEntry("targetResY", 200.0));

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
        case TargetMode::Terminals:  classes = QString::fromLatin1(KNOWN_TERMINALS); break;
        case TargetMode::AllWindows: classes = QStringLiteral("*"); break;   // effect-code herkent "*" als "alles"
        case TargetMode::Custom:     classes = m_targetClasses ? m_targetClasses->text() : QString(); break;
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
    if (m_pixelScaleRow)   grp.writeEntry("pixelScale",  m_pixelScaleRow->value());
    if (m_sampleModeCombo) grp.writeEntry("sampleMode",  m_sampleModeCombo->currentIndex());
    if (m_targetResX)      grp.writeEntry("targetResX",  m_targetResX->value());
    if (m_targetResY)      grp.writeEntry("targetResY",  m_targetResY->value());

    cfg->sync();
    setNeedsSave(false);
}

void RetroTermKCM::defaults()
{
    setTargetMode(TargetMode::Terminals);
    if (m_targetClasses)
        m_targetClasses->setText(QString::fromLatin1(KNOWN_TERMINALS));
    applyPreset(PresetValues{});
    setNeedsSave(true);
}

#include "retro_term_kcm.moc"
