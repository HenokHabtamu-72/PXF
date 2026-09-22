/*  PXF · Parameters.cpp
    Copyright (c) 2026 Henok Habtamu.
    Released under the GNU Affero General Public License v3.0 — see LICENSE.
*/

#include "Parameters.h"

namespace pxf
{
    juce::StringArray waveNames()
    {
        return { "Sine", "Triangle", "Saw", "Square" };
    }

    juce::StringArray delayDivNames()
    {
        return { "1/16", "1/8T", "1/8", "1/8D", "1/4", "1/4D", "1/2" };
    }

    float delayDivInQuarters (int index)
    {
        static const float quarters[] = { 0.25f, 1.0f / 3.0f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f };
        return quarters[juce::jlimit (0, 6, index)];
    }

    //==============================================================================
    namespace
    {
        juce::NormalisableRange<float> range (float lo, float hi, float centre)
        {
            juce::NormalisableRange<float> r (lo, hi);
            r.setSkewForCentre (centre);
            return r;
        }

        juce::NormalisableRange<float> range (float lo, float hi)
        {
            return { lo, hi };
        }

        using FloatParam  = juce::AudioParameterFloat;
        using ChoiceParam = juce::AudioParameterChoice;
        using BoolParam   = juce::AudioParameterBool;

        std::unique_ptr<FloatParam> makeFloat (const char* id, const char* name,
                                               juce::NormalisableRange<float> r, float def)
        {
            return std::make_unique<FloatParam> (juce::ParameterID { id, 1 }, name, r, def);
        }
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;

        // ------------------------------------------------------------------ SYNTH
        layout.add (std::make_unique<ChoiceParam> (juce::ParameterID { pid::osc1Wave, 1 }, "Osc 1 Wave",  waveNames(), 2));
        layout.add (std::make_unique<ChoiceParam> (juce::ParameterID { pid::osc2Wave, 1 }, "Osc 2 Wave",  waveNames(), 1));

        layout.add (makeFloat (pid::oscMix,   "Osc Mix",   range (0.0f, 1.0f),               0.5f));
        layout.add (makeFloat (pid::detune,   "Detune",    range (0.0f, 50.0f, 12.0f),       8.0f));
        layout.add (makeFloat (pid::subLevel, "Sub",       range (0.0f, 1.0f),               0.35f));

        layout.add (makeFloat (pid::cutoff,   "Cutoff",    range (30.0f, 18000.0f, 1200.0f), 2000.0f));
        layout.add (makeFloat (pid::reso,     "Resonance", range (0.0f, 1.0f),               0.15f));

        layout.add (makeFloat (pid::attack,   "Attack",    range (0.001f, 4.0f, 0.05f),      0.01f));
        layout.add (makeFloat (pid::decay,    "Decay",     range (0.005f, 4.0f, 0.30f),      0.40f));
        layout.add (makeFloat (pid::sustain,  "Sustain",   range (0.0f, 1.0f),               0.70f));
        layout.add (makeFloat (pid::release,  "Release",   range (0.005f, 8.0f, 0.40f),      0.50f));

        layout.add (makeFloat (pid::glide,    "Glide",     range (0.0f, 2000.0f, 150.0f),    0.0f));

        layout.add (std::make_unique<BoolParam> (juce::ParameterID { pid::mono,   1 }, "Mono",   false));
        layout.add (std::make_unique<BoolParam> (juce::ParameterID { pid::legato, 1 }, "Legato", true));

        // ------------------------------------------------------------------ GRAIN
        layout.add (makeFloat (pid::grainSize,     "Grain Size", range (10.0f, 500.0f, 120.0f), 90.0f));
        layout.add (makeFloat (pid::grainDensity,  "Density",    range (1.0f, 100.0f, 25.0f),   22.0f));
        layout.add (makeFloat (pid::grainPosition, "Position",   range (0.0f, 100.0f),          0.0f));
        layout.add (makeFloat (pid::grainSpray,    "Spray",      range (0.0f, 100.0f),          10.0f));
        layout.add (makeFloat (pid::grainPitch,    "Pitch",      range (-24.0f, 24.0f),         0.0f));
        layout.add (makeFloat (pid::grainShape,    "Shape",      range (0.0f, 1.0f),            0.30f));

        // --------------------------------------------------------------- MASTER FX
        layout.add (makeFloat (pid::revSize,  "Reverb Size", range (0.0f, 1.0f), 0.55f));
        layout.add (makeFloat (pid::revMix,   "Reverb Mix",  range (0.0f, 1.0f), 0.25f));

        layout.add (std::make_unique<ChoiceParam> (juce::ParameterID { pid::delayDiv, 1 }, "Delay Time", delayDivNames(), 2));

        layout.add (makeFloat (pid::delayFb,   "Delay Feedback", range (0.0f, 0.95f), 0.30f));
        layout.add (makeFloat (pid::delayMix,  "Delay Mix",      range (0.0f, 1.0f),  0.20f));
        layout.add (makeFloat (pid::dust,      "Dust",           range (0.0f, 1.0f),  0.30f));
        layout.add (makeFloat (pid::masterVol, "Volume",         range (-40.0f, 6.0f), -6.0f));

        return layout;
    }
}
