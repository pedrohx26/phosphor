#!/usr/bin/env python3
"""Consistency checker for Phosphor's preset table.

Four separate audits of this preset table each turned up invented or
self-contradictory data: wrong years and resolutions, pixel grids for machines
that never published one, palettes attributed to vendors who never specified
them, and sine-drift sync on every home computer on the theory that "a TV
picture wobbles". The pattern is always the same — a plausible-looking value
that no source supports, sitting next to correct ones and indistinguishable
from them by eye.

This script is the mechanical half of the defence. It cannot check history (a
year is right or wrong regardless of what the other fields say), but it can
check that the fields agree *with each other* — and most of the errors found
so far did leave a detectable inconsistency behind. It is deliberately noisy
about intent: everything it reports is a question worth answering, not proof
of a bug.

Usage:  tools/check-presets.py [--quiet]
Exit code 1 if any ERROR-level finding is present, so it can gate CI.
"""

import re
import sys
import pathlib

SRC = pathlib.Path(__file__).resolve().parent.parent / "src" / "retro_term_kcm.cpp"

# Field order mirrors PresetValues in retro_term_kcm.h. Kept as a list rather
# than parsed from the header so a silent reordering there shows up here as
# nonsense values instead of quietly shifting every check by one.
FIELDS = [
    "name", "era", "phosphorType", "phosphorAgeing", "colorTemperature",
    "phosphorPersistence", "screenCurvature", "vignetteIntensity",
    "ambientReflection", "rasterizationMode", "scanlinesIntensity",
    "scanlinesSharpness", "bloom", "glowingLine", "brightness", "contrast",
    "staticNoise", "jitter", "syncMode", "horizontalSync", "flickering",
    "ghostingIntensity", "chromaColor", "saturationColor", "rbgShift",
    "characterSmearing", "burnIn", "warmupEnabled", "warmupDuration",
    "degaussOnStart", "degaussDuration", "font", "fontSize",
    "targetResX", "targetResY", "targetCols", "targetRows", "scheme",
]

# In-class defaults from PresetValues, for fields omitted at the end of an
# aggregate initialiser. Only the tail is ever omissible in practice.
DEFAULTS = {"scheme": '""', "targetCols": "0", "targetRows": "0",
            "targetResX": "0.0", "targetResY": "0.0", "fontSize": "14"}

PHOSPHOR = {0: "P1 green", 1: "P3 amber", 2: "P4 white", 3: "P39 long-persistence"}
RASTER = {0: "none", 1: "scanlines", 2: "shadow mask", 3: "aperture grille"}
SYNC = {0: "stable", 1: "sine drift", 2: "rolling", 3: "ghosting"}

# Schemes that mean "this machine drew in one colour"; the phosphor simulation
# supplies the tint, so colour-retention fields must stay near zero or the
# monochrome look is contaminated by whatever the terminal palette holds.
MONO_SCHEMES = {"mono", "paper"}

# Presets that deliberately make no hardware claim, so era-consistency checks
# do not apply to them.
GENERIC = {"Default (amber)", "Minimaal (laag GPU)"}


def split_fields(inner: str):
    """Split one p({...}) body on top-level commas (quotes/brackets aware)."""
    out, depth, in_quote, start, i = [], 0, False, 0, 0
    while i < len(inner):
        c = inner[i]
        if in_quote:
            if c == "\\":
                i += 2
                continue
            if c == '"':
                in_quote = False
        elif c == '"':
            in_quote = True
        elif c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
        elif c == "," and depth == 0:
            out.append(inner[start:i].strip())
            start = i + 1
        i += 1
    out.append(inner[start:].strip())
    return out


def parse(path: pathlib.Path):
    src = path.read_text()
    presets = []
    for m in re.finditer(r"p\(\{(.*?)\}\);", src, re.DOTALL):
        raw = split_fields(m.group(1))
        # C++ aggregate initialisation lets trailing fields be omitted, taking
        # PresetValues' in-class defaults. Mirror that rather than rejecting
        # such entries — but only ever pad the tail, since a *shorter* list
        # anywhere else would mean the fields have silently shifted.
        if len(raw) < len(FIELDS):
            raw = raw + [DEFAULTS[k] for k in FIELDS[len(raw):]]
        if len(raw) != len(FIELDS):
            raise SystemExit(
                f"field-count mismatch: got {len(raw)}, expected {len(FIELDS)}\n"
                f"  in: {raw[0] if raw else '?'}\n"
                "  PresetValues and FIELDS in this script have drifted apart."
            )
        p = {}
        for key, val in zip(FIELDS, raw):
            v = val.strip()
            if v.startswith('"'):
                p[key] = v[1:-1]
            elif v in ("true", "false"):
                p[key] = v == "true"
            elif re.fullmatch(r"-?\d+", v):
                p[key] = int(v)
            else:
                p[key] = float(v)
        presets.append(p)
    return presets


def check(p, err, warn, note):
    n = p["name"]
    mono = p["scheme"] in MONO_SCHEMES
    generic = n in GENERIC

    # ── Colour vs. monochrome ────────────────────────────────────────────────
    # A monochrome machine that retains terminal colour shows a green-tinted
    # rainbow; a colour machine with the colour drained shows a palette that
    # was researched and then thrown away.
    if mono and not generic:
        if p["chromaColor"] > 0.05:
            err(n, f"monochrome scheme '{p['scheme']}' but chromaColor="
                   f"{p['chromaColor']} — colour leaks into a mono machine")
        if p["saturationColor"] > 0.05:
            err(n, f"monochrome scheme but saturationColor={p['saturationColor']}")
    if not mono and not generic and p["scheme"]:
        if p["chromaColor"] < 0.5:
            err(n, f"colour scheme '{p['scheme']}' but chromaColor="
                   f"{p['chromaColor']} — the researched palette is half discarded")

    # A colour machine tinted through a coloured phosphor cannot show its own
    # palette: P4 (white) is the only neutral base.
    if not mono and not generic and p["scheme"] and p["phosphorType"] != 2:
        err(n, f"colour scheme but phosphorType={p['phosphorType']} "
               f"({PHOSPHOR.get(p['phosphorType'])}) — tints the palette away; "
               "P4 white is the neutral choice")

    # ── Phosphor semantics ───────────────────────────────────────────────────
    # P39 is *defined* by its long afterglow; a P39 preset that decays fast is
    # claiming a phosphor it isn't simulating.
    if p["phosphorType"] == 3 and p["phosphorPersistence"] < 0.35:
        warn(n, f"P39 (long persistence) but phosphorPersistence="
                f"{p['phosphorPersistence']} — that is a short-decay phosphor")
    if p["phosphorType"] != 3 and p["phosphorPersistence"] > 0.5:
        warn(n, f"phosphorPersistence={p['phosphorPersistence']} is P39-like "
                f"but phosphorType={PHOSPHOR.get(p['phosphorType'])}")

    # ── Pixel grid ───────────────────────────────────────────────────────────
    cols, rows = p["targetCols"], p["targetRows"]
    rx, ry = p["targetResX"], p["targetResY"]
    if (cols > 0) != (rows > 0):
        err(n, f"half a grid: cols={cols} rows={rows}")
    if cols > 0 and rx > 0:
        if int(rx) % cols or int(ry) % rows:
            # Not necessarily wrong — the HP 150 genuinely had a non-integer
            # cell — but it silently disables integer zoom, so it must be a
            # conscious choice rather than a typo.
            note(n, f"cell not integral: {int(rx)}x{int(ry)} / {cols}x{rows} = "
                    f"{rx/cols:.2f}x{ry/rows:.2f} — integer zoom stays off")
        else:
            cw, ch = int(rx) // cols, int(ry) // rows
            if not (4 <= cw <= 16 and 6 <= ch <= 24):
                warn(n, f"implausible character cell {cw}x{ch}px")
    if cols > 0 and rx <= 0:
        note(n, f"grid {cols}x{rows} but no resolution — integer zoom stays off")

    # ── Raster / era ─────────────────────────────────────────────────────────
    # Aperture grille is a Trinitron-family construction, not a generic look.
    if p["rasterizationMode"] == 3 and "Trinitron" not in n:
        warn(n, "rasterizationMode=aperture grille on a non-Trinitron machine")
    if p["ghostingIntensity"] > 0.001 and p["syncMode"] != 3:
        note(n, f"ghostingIntensity={p['ghostingIntensity']} has no effect "
                f"outside syncMode=ghosting (is {SYNC.get(p['syncMode'])})")
    if p["syncMode"] != 0 and p["horizontalSync"] < 0.001:
        note(n, f"syncMode={SYNC.get(p['syncMode'])} but horizontalSync=0 — inert")

    # Degaussing demagnetises a shadow/aperture mask. A monochrome tube has no
    # mask, so the animation is depicting hardware that isn't there.
    if mono and not generic and p["degaussOnStart"]:
        warn(n, "degauss animation on a monochrome tube — degaussing is a "
                "colour-mask procedure")

    # ── Ranges ───────────────────────────────────────────────────────────────
    if not (3000 <= p["colorTemperature"] <= 9300):
        err(n, f"colorTemperature={p['colorTemperature']} outside the UI range")
    for k in ("phosphorAgeing", "phosphorPersistence", "screenCurvature",
              "vignetteIntensity", "scanlinesIntensity", "scanlinesSharpness",
              "bloom", "glowingLine", "brightness", "contrast", "staticNoise",
              "jitter", "horizontalSync", "flickering", "chromaColor",
              "saturationColor", "rbgShift", "characterSmearing", "burnIn"):
        if not (0.0 <= p[k] <= 1.0):
            err(n, f"{k}={p[k]} outside 0..1")
    if not (0.0 <= p["ambientReflection"] <= 0.30):
        err(n, f"ambientReflection={p['ambientReflection']} outside 0..0.30")
    if not (0.0 <= p["ghostingIntensity"] <= 0.5):
        err(n, f"ghostingIntensity={p['ghostingIntensity']} outside 0..0.5")
    if not p["font"]:
        err(n, "no font")
    if not (8 <= p["fontSize"] <= 32):
        warn(n, f"fontSize={p['fontSize']} is unusual")
    if not p["scheme"] and not generic:
        warn(n, "no colour scheme — the terminal keeps whatever palette it had")


def main():
    quiet = "--quiet" in sys.argv
    presets = parse(SRC)

    errors, warnings, notes = [], [], []
    check_fns = (lambda n, m: errors.append((n, m)),
                 lambda n, m: warnings.append((n, m)),
                 lambda n, m: notes.append((n, m)))
    for p in presets:
        check(p, *check_fns)

    # Cross-preset: schemes and fonts should be reused deliberately, not by
    # accident. Reuse is legitimate (four DOS machines really did share the
    # CGA palette) — this only surfaces it for review.
    if not quiet:
        by_font = {}
        for p in presets:
            by_font.setdefault(p["font"], []).append(p["name"])
        shared = {f: ns for f, ns in by_font.items() if len(ns) > 1}
        if shared:
            print("Fonts used by more than one preset (check substitutions):")
            for f, ns in sorted(shared.items()):
                print(f"  {f:34} {', '.join(ns)}")
            print()

    for label, items in (("ERROR", errors), ("WARN", warnings), ("NOTE", notes)):
        if items and not (quiet and label == "NOTE"):
            print(f"{label} ({len(items)}):")
            for name, msg in items:
                print(f"  {name:32} {msg}")
            print()

    print(f"{len(presets)} presets checked — "
          f"{len(errors)} errors, {len(warnings)} warnings, {len(notes)} notes")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
