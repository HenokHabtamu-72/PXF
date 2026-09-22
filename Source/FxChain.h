/*  PXF · FxChain.h
    Copyright (c) 2026 Henok Habtamu.
    Released under the GNU Affero General Public License v3.0 — see LICENSE.

    Shared by both engines. Signal order:

        engine -> BPM-synced ping-pong delay -> reverb -> DUST -> master volume

    DUST is the lo-fi character control. One knob, four things at once:
        wow & flutter   a slowly modulated read head (tape drift)
        saturation      gentle drive into a soft knee
        high-cut        a one-pole roll-off that closes as it turns up
        vinyl noise     filtered hiss plus sparse crackle, gated to playing

    At Dust = 0 the whole stage is transparent apart from a fixed 4 ms read offset,
    which is reported to the host as latency.
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "DspUtils.h"

namespace pxf
{
    struct FxParams
    {
        float revSize  = 0.55f;
        float revMix   = 0.25f;
        int   delayDiv = 2;
        float delayFb  = 0.30f;
        float delayMix = 0.20f;
        float dust     = 0.30f;
        float volumeDb = -6.0f;
    };

    class FxChain
    {
    public:
        void prepare (double sampleRate);
        void reset();

        void setParams (const FxParams& p) noexcept { params = p; }

        /** @param engineActive gates the vinyl noise so an idle instrument stays silent. */
        void process (juce::AudioBuffer<float>& buffer, double bpm, bool engineActive);

        int getLatencySamples() const noexcept { return baseWowSamples; }

    private:
        double sampleRate = 44100.0;

        juce::Reverb reverb;

        DelayLine delayL, delayR;
        OnePoleLP dampL, dampR;

        DelayLine wowL, wowR;
        OnePoleLP highCutL, highCutR, noiseFilter;

        juce::SmoothedValue<float> smoothedDelaySamples { 1000.0f };
        juce::SmoothedValue<float> smoothedFeedback     { 0.0f };
        juce::SmoothedValue<float> smoothedDelayMix     { 0.0f };
        juce::SmoothedValue<float> smoothedDust         { 0.0f };
        juce::SmoothedValue<float> smoothedGain         { 0.0f };
        juce::SmoothedValue<float> smoothedNoiseGate    { 0.0f };

        float wowPhase = 0.0f, flutterPhase = 0.0f;
        float crackle  = 0.0f;

        int   baseWowSamples = 0;
        float maxWowDepth    = 0.0f;
        float lastDustCutoff = -1.0f;

        juce::Random rng;
        FxParams params;
    };
}
