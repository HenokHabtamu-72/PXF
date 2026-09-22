/*  PXF · Presets.cpp
    Copyright (c) 2026 Henok Habtamu.
    Released under the GNU Affero General Public License v3.0 — see LICENSE.
*/

#include "Presets.h"

namespace pxf
{
    /*  Waves: 0 sine  1 triangle  2 saw  3 square
        Delay: 0 = 1/16, 1 = 1/8T, 2 = 1/8, 3 = 1/8D, 4 = 1/4, 5 = 1/4D, 6 = 1/2

        Basses, flutes and leads run MONO + LEGATO so overlapping notes slide.
        Everything polyphonic keeps Glide at 0, so chords never swoop unless asked.

        cat      name              o1 o2   mix   det   sub    cut    res     A      D      S     R    glide  mono  leg   rvSz  rvMx  dly   fb    dmx   dust  vol  */
    const PresetDef presets[kNumPresets] =
    {
        { "BASS",  "Midnight Sub",   0, 1, 0.30f,  5.0f, 0.85f,  320.0f, 0.12f, 0.006f, 0.40f, 0.60f, 0.30f,  45.0f, true,  true,  0.35f, 0.08f, 2, 0.20f, 0.05f, 0.28f, -5.0f },
        { "BASS",  "Purple Bass",    2, 3, 0.35f,  9.0f, 0.60f,  520.0f, 0.30f, 0.005f, 0.50f, 0.50f, 0.28f,  30.0f, true,  true,  0.30f, 0.10f, 2, 0.18f, 0.07f, 0.38f, -6.0f },
        { "BASS",  "Deep End",       0, 0, 0.40f,  3.0f, 1.00f,  240.0f, 0.08f, 0.012f, 0.80f, 0.78f, 0.45f,  70.0f, true,  true,  0.40f, 0.10f, 4, 0.15f, 0.05f, 0.22f, -5.0f },

        { "PLUCK", "Rain Pluck",     1, 2, 0.40f,  8.0f, 0.25f, 1700.0f, 0.35f, 0.002f, 0.28f, 0.00f, 0.25f,   0.0f, false, true,  0.55f, 0.28f, 2, 0.38f, 0.30f, 0.35f, -7.0f },
        { "PLUCK", "Neon Drip",      3, 1, 0.45f, 12.0f, 0.20f, 2400.0f, 0.45f, 0.002f, 0.22f, 0.05f, 0.30f,   0.0f, false, true,  0.50f, 0.25f, 0, 0.42f, 0.32f, 0.30f, -7.0f },
        { "PLUCK", "Glass Pluck",    0, 1, 0.50f,  6.0f, 0.15f, 3800.0f, 0.25f, 0.001f, 0.35f, 0.00f, 0.40f,   0.0f, false, true,  0.65f, 0.32f, 2, 0.30f, 0.22f, 0.25f, -7.0f },

        { "SYNTH", "Dusk Synth",     2, 2, 0.50f, 14.0f, 0.30f, 1200.0f, 0.22f, 0.050f, 0.60f, 0.55f, 0.50f,   0.0f, false, true,  0.55f, 0.25f, 2, 0.28f, 0.18f, 0.40f, -7.0f },
        { "SYNTH", "Haze",           1, 2, 0.45f, 18.0f, 0.25f,  900.0f, 0.18f, 0.250f, 1.00f, 0.60f, 0.80f,   0.0f, false, true,  0.70f, 0.35f, 4, 0.30f, 0.20f, 0.50f, -7.0f },

        { "PAD",   "2AM Pad",        2, 1, 0.50f, 18.0f, 0.20f, 1000.0f, 0.15f, 0.900f, 1.50f, 0.70f, 1.60f,   0.0f, false, true,  0.80f, 0.45f, 4, 0.32f, 0.22f, 0.40f, -8.0f },
        { "PAD",   "Violet Sky",     2, 2, 0.50f, 24.0f, 0.15f, 1400.0f, 0.20f, 1.400f, 2.00f, 0.80f, 2.20f,   0.0f, false, true,  0.85f, 0.50f, 6, 0.28f, 0.18f, 0.35f, -8.0f },
        { "PAD",   "Slow Fade",      1, 0, 0.45f, 10.0f, 0.25f,  700.0f, 0.10f, 2.000f, 2.50f, 0.65f, 3.00f,   0.0f, false, true,  0.90f, 0.55f, 6, 0.25f, 0.15f, 0.45f, -8.0f },

        { "BELL",  "Night Bell",     0, 0, 0.45f, 22.0f, 0.10f, 5000.0f, 0.20f, 0.001f, 1.20f, 0.00f, 1.40f,   0.0f, false, true,  0.70f, 0.40f, 2, 0.32f, 0.22f, 0.30f, -8.0f },
        { "BELL",  "Chime Glow",     0, 1, 0.40f, 16.0f, 0.08f, 6500.0f, 0.15f, 0.002f, 1.60f, 0.00f, 1.80f,   0.0f, false, true,  0.80f, 0.45f, 0, 0.35f, 0.25f, 0.25f, -8.0f },

        { "FLUTE", "Air Flute",      1, 0, 0.35f,  4.0f, 0.10f, 2200.0f, 0.10f, 0.120f, 0.30f, 0.85f, 0.30f,  35.0f, true,  true,  0.55f, 0.30f, 2, 0.25f, 0.15f, 0.30f, -6.0f },
        { "FLUTE", "Lofi Flute",     1, 1, 0.40f,  7.0f, 0.12f, 1700.0f, 0.14f, 0.090f, 0.25f, 0.80f, 0.35f,  50.0f, true,  true,  0.50f, 0.28f, 2, 0.28f, 0.18f, 0.60f, -6.0f },

        { "PIANO", "Dusty Keys",     1, 2, 0.28f,  6.0f, 0.30f, 2600.0f, 0.18f, 0.002f, 0.90f, 0.18f, 0.35f,   0.0f, false, true,  0.50f, 0.25f, 2, 0.25f, 0.12f, 0.55f, -7.0f },
        { "PIANO", "Felt Piano",     0, 1, 0.35f,  4.0f, 0.35f, 1900.0f, 0.12f, 0.004f, 1.10f, 0.12f, 0.45f,   0.0f, false, true,  0.60f, 0.30f, 4, 0.20f, 0.10f, 0.45f, -7.0f },

        { "LEAD",  "Neon Lead",      2, 3, 0.40f, 10.0f, 0.25f, 2100.0f, 0.30f, 0.010f, 0.30f, 0.75f, 0.30f,  65.0f, true,  true,  0.50f, 0.25f, 3, 0.40f, 0.30f, 0.30f, -7.0f },
        { "LEAD",  "Skyline",        2, 2, 0.50f, 16.0f, 0.20f, 2600.0f, 0.25f, 0.020f, 0.40f, 0.80f, 0.50f,  55.0f, true,  true,  0.60f, 0.30f, 4, 0.35f, 0.28f, 0.35f, -7.0f },
        { "LEAD",  "Cruise Control", 3, 1, 0.45f,  8.0f, 0.30f, 1800.0f, 0.28f, 0.030f, 0.50f, 0.70f, 0.60f, 120.0f, true,  true,  0.55f, 0.28f, 2, 0.34f, 0.26f, 0.40f, -7.0f }
    };
}
