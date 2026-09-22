/*  PXF · Parameters.h
    Copyright (c) 2026 Henok Habtamu.
    Released under the GNU Affero General Public License v3.0 — see LICENSE.

    Every automatable parameter lives here.
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace pxf
{
    namespace pid
    {
        // ---- SYNTH ----
        inline constexpr const char* osc1Wave = "osc1Wave";
        inline constexpr const char* osc2Wave = "osc2Wave";
        inline constexpr const char* oscMix   = "oscMix";
        inline constexpr const char* detune   = "detune";
        inline constexpr const char* subLevel = "subLevel";
        inline constexpr const char* cutoff   = "cutoff";
        inline constexpr const char* reso     = "reso";
        inline constexpr const char* attack   = "attack";
        inline constexpr const char* decay    = "decay";
        inline constexpr const char* sustain  = "sustain";
        inline constexpr const char* release  = "release";
        inline constexpr const char* glide    = "glide";
        inline constexpr const char* mono     = "mono";
        inline constexpr const char* legato   = "legato";

        // ---- GRAIN ----
        inline constexpr const char* grainSize     = "grainSize";
        inline constexpr const char* grainDensity  = "grainDensity";
        inline constexpr const char* grainPosition = "grainPosition";
        inline constexpr const char* grainSpray    = "grainSpray";
        inline constexpr const char* grainPitch    = "grainPitch";
        inline constexpr const char* grainShape    = "grainShape";

        // ---- MASTER FX ----
        inline constexpr const char* revSize   = "revSize";
        inline constexpr const char* revMix    = "revMix";
        inline constexpr const char* delayDiv  = "delayDiv";
        inline constexpr const char* delayFb   = "delayFb";
        inline constexpr const char* delayMix  = "delayMix";
        inline constexpr const char* dust      = "dust";
        inline constexpr const char* masterVol = "masterVol";
    }

    juce::StringArray waveNames();
    juce::StringArray delayDivNames();

    /** Delay division expressed in quarter notes. */
    float delayDivInQuarters (int index);

    /** Maps the 0..1 resonance knob to a filter Q. */
    inline float resoToQ (float reso) noexcept
    {
        return 0.707f * std::pow (11.0f, juce::jlimit (0.0f, 1.0f, reso));
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
}
