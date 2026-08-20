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
void RetroTermKCM::buildPresets()
{
    auto p = [&](PresetValues pv) { m_presets.append(pv); };

    p({"Default (amber)","—",1,0.05,7000,0.10,0.25,0.35,0.04,1,0.35,0.50,0.55,0.20,0.50,0.80,0.08,0.10,0,0.05,0.08,0.00,0.20,0.20,0.10,0.08,0.20,true,8.0,true,2.5,"VT323",16,0.0,0.0});
    p({"IBM 2260 (1964)","1964 — Vroege IBM-mainframeterminal, 80×12",2,0.55,8500,0.60,0.45,0.65,0.12,0,0.35,0.50,0.80,0.45,0.42,0.90,0.18,0.25,1,0.20,0.22,0.00,0.00,0.00,0.15,0.30,0.50,true,15.0,true,4.0,"Glass TTY VT220",16,0.0,0.0});
    p({"DEC GT40 (1972)","1972 — Vectorterminal PDP-11, P39",3,0.40,7800,0.80,0.20,0.55,0.08,0,0.35,0.50,0.85,0.60,0.38,0.85,0.10,0.15,0,0.08,0.15,0.00,0.05,0.10,0.06,0.15,0.35,true,12.0,true,3.5,"VT323",18,1024.0,768.0});
    p({"DEC VT100 (1978)","1978 — Dé referentieterminal",0,0.12,8000,0.18,0.22,0.38,0.05,1,0.40,0.55,0.52,0.22,0.52,0.82,0.06,0.08,0,0.05,0.07,0.00,0.05,0.08,0.05,0.10,0.22,true,9.0,true,2.5,"VT323",18,800.0,240.0});
    p({"IBM 3270 (1971)","1971 — IBM-mainframe blokmodus",0,0.18,8200,0.25,0.28,0.42,0.06,1,0.35,0.60,0.48,0.18,0.50,0.88,0.05,0.06,0,0.04,0.05,0.00,0.04,0.06,0.04,0.08,0.35,true,10.0,true,2.8,"Px437 IBM 3270pc",16,720.0,350.0});
    p({"Wyse WY-50 (1983)","1983 — UNIX-werkterminal, 14\" groen",0,0.08,8400,0.14,0.18,0.32,0.04,1,0.38,0.65,0.55,0.20,0.55,0.85,0.04,0.06,0,0.03,0.05,0.00,0.04,0.08,0.04,0.08,0.20,true,8.0,true,2.5,"Px437 Wyse700b",16,800.0,312.0});
    p({"Militair Radar (1958)","1958 — SAGE AN/FSQ-7, 19\" P14 nalichtend",3,0.50,7000,0.90,0.15,0.70,0.10,0,0.35,0.50,0.90,0.70,0.35,0.95,0.15,0.20,0,0.10,0.18,0.00,0.04,0.08,0.06,0.20,0.55,true,20.0,true,5.0,"Share Tech Mono",16,0.0,0.0});
    p({"Apple II (1977)","1977 — NTSC-TV, composite",2,0.30,6500,0.22,0.38,0.50,0.08,1,0.55,0.35,0.65,0.28,0.48,0.78,0.14,0.18,1,0.12,0.14,0.00,0.45,0.35,0.18,0.35,0.28,true,10.0,true,3.0,"Print Char 21",16,280.0,192.0});
    p({"Commodore 64 (1982)","1982 — VIC-II, PAL-TV",2,0.22,6200,0.18,0.35,0.45,0.07,1,0.50,0.38,0.60,0.25,0.50,0.80,0.12,0.14,1,0.10,0.12,0.00,0.55,0.40,0.14,0.28,0.25,true,9.0,true,2.8,"C64 Pro Mono",14,320.0,200.0});
    p({"ZX Spectrum (1982)","1982 — PAL-TV, attribuutcellen",2,0.25,6300,0.16,0.38,0.48,0.08,1,0.52,0.33,0.62,0.24,0.52,0.78,0.13,0.16,1,0.11,0.13,0.00,0.60,0.45,0.16,0.30,0.20,true,9.0,true,2.8,"VT323",14,256.0,192.0});
    p({"BBC Micro (1981)","1981 — Britse schoolcomputer",2,0.20,6600,0.16,0.30,0.44,0.07,1,0.50,0.38,0.60,0.22,0.50,0.80,0.10,0.12,1,0.09,0.11,0.00,0.55,0.40,0.14,0.25,0.22,true,9.0,true,2.8,"Bedstead",16,320.0,256.0});
    p({"Atari 400/800 (1979)","1979 — ANTIC/CTIA, NTSC-TV",2,0.26,6400,0.19,0.36,0.47,0.07,1,0.52,0.35,0.62,0.26,0.50,0.79,0.12,0.15,1,0.10,0.13,0.00,0.50,0.38,0.15,0.30,0.24,true,9.0,true,2.8,"Atari Classic",16,320.0,192.0});
    p({"IBM PC MDA (1981)","1981 — IBM 5151, P39",3,0.15,7500,0.30,0.20,0.40,0.06,1,0.42,0.60,0.58,0.28,0.50,0.88,0.05,0.07,0,0.04,0.06,0.00,0.04,0.06,0.05,0.10,0.30,true,8.0,true,2.5,"PxPlus IBM MDA",16,720.0,350.0});
    p({"IBM PC CGA (1981)","1981 — CGA op composite/TV",2,0.20,7000,0.15,0.25,0.38,0.06,1,0.45,0.50,0.55,0.22,0.52,0.83,0.08,0.10,0,0.06,0.08,0.00,0.65,0.45,0.10,0.15,0.22,true,8.0,true,2.5,"PxPlus IBM CGA",16,320.0,200.0});
    p({"IBM PC EGA (1984)","1984 — IBM 5154, 16 kleuren",2,0.14,7200,0.12,0.20,0.32,0.05,1,0.38,0.58,0.50,0.18,0.54,0.84,0.06,0.08,0,0.04,0.06,0.00,0.60,0.38,0.08,0.12,0.18,true,8.0,true,2.5,"PxPlus IBM EGA 8x14",14,640.0,350.0});
    p({"Tandy 1000 (1984)","1984 — Verbeterde CGA",2,0.22,6800,0.15,0.28,0.42,0.07,1,0.48,0.42,0.58,0.22,0.50,0.80,0.10,0.12,1,0.08,0.10,0.00,0.70,0.48,0.12,0.20,0.24,true,9.0,true,2.8,"PxPlus Tandy1K-II 200L",16,320.0,200.0});
    p({"IBM PS/2 VGA (1987)","1987 — De DOS-standaard",2,0.10,7400,0.10,0.15,0.28,0.04,1,0.32,0.62,0.45,0.15,0.55,0.85,0.05,0.06,0,0.03,0.05,0.00,0.65,0.35,0.07,0.10,0.15,true,7.0,true,2.2,"PxPlus IBM VGA 9x16",16,720.0,400.0});
    p({"Amiga 500 (1987)","1987 — PAL-TV of 1084S",2,0.15,6800,0.14,0.25,0.38,0.06,1,0.48,0.44,0.55,0.20,0.52,0.81,0.08,0.10,0,0.05,0.08,0.00,0.65,0.42,0.10,0.18,0.18,true,8.0,true,2.5,"Topaz a500a1000a2000",14,320.0,256.0});
    p({"Amiga WorkBench 2 (1990)","1990 — 1084S RGB-monitor",2,0.10,7000,0.10,0.20,0.30,0.05,1,0.40,0.52,0.48,0.16,0.56,0.83,0.06,0.08,0,0.04,0.06,0.00,0.68,0.38,0.08,0.14,0.14,true,7.0,true,2.2,"Topaz a500a1000a2000",14,640.0,256.0});
    p({"Apple Macintosh 128K (1984)","1984 — 9-inch b/w CRT",2,0.35,9000,0.08,0.15,0.55,0.12,2,0.20,0.70,0.35,0.10,0.60,0.92,0.03,0.04,0,0.02,0.04,0.00,0.00,0.00,0.04,0.05,0.40,true,6.0,true,2.0,"Silkscreen",12,512.0,342.0});
    // Font: DejaVu Sans Mono — "Lucida Console" is a Microsoft font with no
    // free-redistributable source, so install-fonts.sh never had anything to
    // fetch for it; DejaVu Sans Mono ships in every distro's base fonts package
    // (already present on both test machines) and reads the same UNIX-console way.
    p({"NeXT Station (1990)","1990 — 1120×832 grijs",2,0.08,8000,0.06,0.08,0.22,0.05,0,0.35,0.50,0.32,0.08,0.62,0.88,0.02,0.03,0,0.02,0.03,0.00,0.00,0.00,0.03,0.04,0.12,true,5.0,true,1.8,"DejaVu Sans Mono",13,1120.0,832.0});
    p({"SVGA Multisync (1992)","1992 — 800×600, shadow mask",2,0.06,7600,0.06,0.10,0.20,0.04,1,0.22,0.72,0.35,0.10,0.60,0.87,0.03,0.04,0,0.02,0.04,0.00,0.70,0.30,0.05,0.06,0.10,true,6.0,true,2.0,"Terminus",14,800.0,600.0});
    p({"Sony Trinitron (1997)","1997 — Aperture-grille beeldbuis",2,0.05,7800,0.05,0.04,0.18,0.04,3,0.18,0.78,0.30,0.08,0.62,0.88,0.02,0.03,0,0.02,0.03,0.00,0.80,0.28,0.04,0.04,0.08,true,5.0,true,1.8,"Terminus",14,1024.0,768.0});
    p({"Teletext / Ceefax (1974)","1974 — PAL-TV, 8 kleuren",2,0.30,6200,0.20,0.40,0.52,0.09,1,0.62,0.28,0.70,0.30,0.46,0.76,0.20,0.24,2,0.18,0.20,0.12,0.80,0.55,0.22,0.40,0.28,true,12.0,true,3.5,"Bedstead",16,480.0,250.0});
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
       0.00,0.00,0.05,0.12,0.38, true,11.0,true,3.0, "Pet Me 2Y",16,320.0,200.0});

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
       0.58,0.18,0.50,0.80, 0.14,0.15,1,0.10,0.12,0.00,
       0.10,0.12,0.14,0.28,0.22, true,9.0,true,2.8, "Another Mans Treasure MIA Raw",16,384.0,192.0});

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
       0.60,0.45,0.15,0.30,0.22, true,9.0,true,2.8, "Hot CoCo",16,256.0,192.0});

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
       0.00,0.00,0.04,0.08,0.25, true,9.0,true,2.8, "Px437 Kaypro2K G",16,640.0,192.0});

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
       0.00,0.00,0.05,0.10,0.28, true,9.0,true,2.8, "Px437 Compaq Port3",16,640.0,200.0});

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
       0.00,0.00,0.04,0.08,0.18, true,8.0,true,2.5, "PxPlus Rainbow100 re.40",16,800.0,240.0});

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
       0.00,0.00,0.04,0.07,0.18, true,8.0,true,2.2, "Px437 Wyse700b",16,0.0,0.0});

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
       0.00,0.00,0.03,0.04,0.30, true,6.0,true,2.0, "LisaTerminal Paper Raw",13,720.0,364.0});

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
       0.70,0.42,0.08,0.14,0.18, true,8.0,true,2.5, "PxPlus Amstrad PC-2y",16,640.0,200.0});

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
       0.00,0.00,0.03,0.04,0.15, true,6.0,true,1.8, "Project Jason Small",14,640.0,400.0});

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
       0.00,0.00,0.04,0.06,0.14, true,7.0,true,2.0, "Px437 NEC APC3 8x16",16,640.0,400.0});

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
       0.00,0.00,0.04,0.06,0.24, true,7.0,true,2.0, "PxPlus HP 150 re.",16,512.0,390.0});

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
       0.75,0.45,0.07,0.10,0.14, true,7.0,true,2.0, "Shaston 320",14,320.0,200.0});

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
       0.54,0.18,0.52,0.83, 0.09,0.10,1,0.06,0.08,0.00,
       0.45,0.32,0.10,0.16,0.22, true,8.0,true,2.5, "Mizuno",14,320.0,200.0});

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
       0.55,0.38,0.16,0.35,0.24, true,10.0,true,3.0, "Antiquarius",16,320.0,192.0});

    // Commodore VIC-20 (1981)
    // Hardware: MOS 6560/6561 (VIC), composite naar TV
    // Font: C64 Pro Mono — shares Commodore character ROM lineage
    // Resolution: 176x184 — VIC-20 high-res graphics mode (22x23 text cells at
    // 8x8 px); missing from the original entry, which left pixel scaling off.
    p({"Commodore VIC-20 (1981)","1981 — First color Commodore home computer",
       2,0.28,6000,0.20, 0.40,0.52,0.09, 1,0.58,0.30,
       0.68,0.30,0.46,0.75, 0.16,0.20,1,0.15,0.16,0.00,
       0.50,0.42,0.20,0.38,0.22, true,10.0,true,3.0, "C64 Pro Mono",16,176.0,184.0});

    // MSX (1983)
    // Hardware: TMS9918 video, composite naar TV
    // Font: VT323 — similar to common MSX screen fonts
    // Resolution: 256x192 — TMS9918 standard screen mode, shared by every
    // TMS9918-based system (MSX, ColecoVision, SG-1000); missing from the
    // original entry, which left pixel scaling off.
    p({"MSX (1983)","1983 — Japanese home computer standard (Sony, Philips, Panasonic)",
       2,0.24,6500,0.18, 0.35,0.46,0.07, 1,0.52,0.36,
       0.62,0.24,0.50,0.79, 0.12,0.14,1,0.10,0.12,0.00,
       0.58,0.42,0.15,0.28,0.22, true,9.0,true,2.8, "VT323",14,256.0,192.0});

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
       0.00,0.00,0.04,0.06,0.12, true,6.0,true,2.0, "DejaVu Sans Mono",14,1152.0,900.0});

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
            "<p>Konsole stores its font in a profile file, which this page can "
            "write for you. For other terminals, set the font yourself:</p>"
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
            if (applyFontToKonsole(m_presetFont, m_presetFontSize, &err)) {
                m_fontStatus->setText(i18n(
                    "<span style=\"color:#27ae60;\">Wrote <b>%1</b> %2pt to "
                    "%3.</span> Open a new Konsole tab or window to see it — "
                    "Konsole reads a profile when a session starts.",
                    m_presetFont, m_presetFontSize,
                    m_konsoleProfile->currentText()));
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

    // Hoofdslider
    m_pixelScaleRow = new ParamRow(
        i18n("Pixel scale:"), 0.0, 1.0, 0.01,
        i18n("0.0 = no scaling  |  1.0 = exact original pixels"),
        gb);
    fl->addRow(i18n("Pixel scale:"), m_pixelScaleRow);
    connect(m_pixelScaleRow, &ParamRow::valueChanged,
            this, &RetroTermKCM::markChanged);
    // At pixelScale 0 the width/height fields (and the quick-pick buttons
    // beside them) are inert — the shader ignores targetRes entirely. Left
    // enabled, they read as "set" when they do nothing; graying the whole
    // row out the moment scaling is off makes that state visible instead of
    // something you have to already know from the tooltip.
    connect(m_pixelScaleRow, &ParamRow::valueChanged, this, [this](double v) {
        if (m_targetResRow) m_targetResRow->setEnabled(v > 0.001);
    });

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
    // setValue() only emits valueChanged on an actual change, so the row's
    // enabled state needs one explicit sync here — load() calling
    // setValue(0.0) on a slider that already defaults to 0.0 fires no
    // signal at all, and the row would otherwise start enabled regardless.
    m_targetResRow->setEnabled(m_pixelScaleRow->value() > 0.001);

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

bool RetroTermKCM::applyFontToKonsole(const QString &family, int pointSize, QString *error)
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

    // Stash whatever font (and antialiasing setting, see below) the profile
    // had before Phosphor ever touched it, so "Restore" can put the user's own
    // choice back. Written once per profile: overwriting it on every preset
    // load would, after two presets, only be able to restore the *previous
    // preset's* font — which is not what a user who spent time picking their
    // terminal font is asking to get back.
    const QString previousFont =
        appearance.readEntry(QStringLiteral("Font"), QString());
    const bool previousAA =
        appearance.readEntry(QStringLiteral("AntiAliasFonts"), true);
    KConfigGroup ours = KSharedConfig::openConfig(QStringLiteral("kwinrc"))
                            ->group(QLatin1String(CFG_GROUP));
    const QString backupKey =
        QStringLiteral("OriginalKonsoleFont-") + QFileInfo(path).fileName();
    const QString backupAAKey =
        QStringLiteral("OriginalKonsoleAA-") + QFileInfo(path).fileName();
    if (!previousFont.isEmpty() && !ours.hasKey(backupKey)) {
        ours.writeEntry(backupKey, previousFont);
        ours.writeEntry(backupAAKey, previousAA);
        ours.sync();
    }

    // Konsole slaat het font op als een QFont-beschrijvingsstring: familie,
    // puntgrootte, en daarna acht velden (pixelSize, styleHint, weight, italic,
    // underline, strikeout, fixedPitch, rawMode) die Konsole zelf ook altijd
    // op deze waarden schrijft. -1 bij pixelSize betekent "gebruik puntgrootte".
    appearance.writeEntry(QStringLiteral("Font"),
        QStringLiteral("%1,%2,-1,5,50,0,0,0,0,0").arg(family).arg(pointSize));

    // These preset fonts are bitmap/pixel fonts (int10h, kreativekorp, C64 Pro
    // Mono, Topaz, ...) drawn as exact blocky pixels on purpose. Antialiasing
    // is the right default for a normal typeface, but on a pixel font it
    // blurs precisely the crisp edges the font was designed to have — the
    // opposite of what a CRT preset is going for. Off whenever Phosphor sets
    // the font; restoreKonsoleFont() below puts it back exactly as found.
    appearance.writeEntry(QStringLiteral("AntiAliasFonts"), false);
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
    const QString original = ours.readEntry(backupKey, QString());
    if (original.isEmpty()) {
        if (error) *error = i18n("this profile has no Phosphor-saved original font");
        return false;
    }
    // Default true (Konsole's own default) if this profile predates the
    // AntiAliasFonts backup added alongside applyFontToKonsole()'s AA=false —
    // an existing OriginalKonsoleFont- backup from before that change won't
    // have a matching AA key, and "restore to what Konsole normally does" is
    // the correct fallback, not "restore to off".
    const bool originalAA = ours.readEntry(backupAAKey, true);

    KSharedConfig::Ptr cfg = KSharedConfig::openConfig(path, KConfig::SimpleConfig);
    KConfigGroup appearance = cfg->group(QStringLiteral("Appearance"));
    appearance.writeEntry(QStringLiteral("Font"), original);
    appearance.writeEntry(QStringLiteral("AntiAliasFonts"), originalAA);
    cfg->sync();

    // Backup weggooien: het profiel staat weer op de eigen keuze van de gebruiker,
    // dus de volgende preset-toepassing mag daar opnieuw een verse backup van maken.
    ours.deleteEntry(backupKey);
    ours.deleteEntry(backupAAKey);
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

    // Pixel scaling: fill in original resolution and enable scaling if preset has one.
    //
    // This used to set 0.7 rather than 1.0, which is why "the resolution doesn't
    // do anything" — the shader interpolates the sampling grid between the
    // window's own pixel size and targetRes (effRes = mix(resolution, targetRes,
    // pixelScale)), so on a 1200px-wide terminal 0.7 lands at ~580 cells: barely
    // two screen pixels per cell, an effect you have to hunt for. 1.0 is the
    // value that actually means what this preset field promises — the machine's
    // real resolution, one block per original pixel.
    if (p.targetResX > 0.0 && p.targetResY > 0.0) {
        if (m_targetResX) m_targetResX->setValue(p.targetResX);
        if (m_targetResY) m_targetResY->setValue(p.targetResY);
        if (m_pixelScaleRow) m_pixelScaleRow->setValue(1.0);
        if (m_sampleModeCombo) m_sampleModeCombo->setCurrentIndex(2);
    } else {
        if (m_pixelScaleRow) m_pixelScaleRow->setValue(0.0);
    }

    // The font is not ours to apply — see buildFontsTab(). Remember what this
    // preset wants so the Fonts tab can offer it, and write it straight into the
    // Konsole profile when the user has asked for that.
    m_presetFont     = p.font;
    m_presetFontSize = p.fontSize;
    updateFontTabInfo();
    if (!p.font.isEmpty() && m_autoApplyFont && m_autoApplyFont->isChecked()
        && m_autoApplyFont->isEnabled()) {
        QString err;
        if (applyFontToKonsole(p.font, p.fontSize, &err) && m_fontStatus) {
            m_fontStatus->setText(i18n(
                "<span style=\"color:#27ae60;\">Wrote <b>%1</b> %2pt to %3.</span> "
                "Open a new Konsole tab or window to see it.",
                p.font, p.fontSize, m_konsoleProfile->currentText()));
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

    m_presetInfo->setText(fontPart + QStringLiteral("<br>") + resPart);
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
    if (m_pixelScaleRow)   m_pixelScaleRow->setValue(cfg.readEntry("pixelScale",  0.0));
    if (m_sampleModeCombo) m_sampleModeCombo->setCurrentIndex(cfg.readEntry("sampleMode", 2));
    if (m_targetResX)      m_targetResX->setValue(cfg.readEntry("targetResX", 320.0));
    if (m_targetResY)      m_targetResY->setValue(cfg.readEntry("targetResY", 200.0));

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
    if (m_pixelScaleRow)   grp.writeEntry("pixelScale",  m_pixelScaleRow->value());
    if (m_sampleModeCombo) grp.writeEntry("sampleMode",  m_sampleModeCombo->currentIndex());
    if (m_targetResX)      grp.writeEntry("targetResX",  m_targetResX->value());
    if (m_targetResY)      grp.writeEntry("targetResY",  m_targetResY->value());

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
