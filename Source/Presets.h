/*  PXF · Presets.h
    Copyright (c) 2026 Henok Habtamu.
    Released under the GNU Affero General Public License v3.0 — see LICENSE.

    Exactly 20 presets, compiled into the binary. No user preset saving.
*/

#pragma once

namespace pxf
{
    struct PresetDef
    {
        const char* category;
        const char* name;

        // synth
        int   osc1, osc2;          // 0 sine, 1 triangle, 2 saw, 3 square
        float oscMix, detune, sub;
        float cutoff, reso;
        float attack, decay, sustain, release;
        float glideMs;
        bool  mono, legato;

        // master fx
        float revSize, revMix;
        int   delayDiv;
        float delayFb, delayMix;
        float dust;
        float volumeDb;
    };

    inline constexpr int kNumPresets = 20;

    extern const PresetDef presets[kNumPresets];
}
