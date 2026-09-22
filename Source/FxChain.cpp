/*  PXF · FxChain.cpp
    Copyright (c) 2026 Henok Habtamu.
    Released under the GNU Affero General Public License v3.0 — see LICENSE.
*/

#include "FxChain.h"
#include "Parameters.h"

namespace pxf
{
    void FxChain::prepare (double sr)
    {
        sampleRate = sr;

        const int maxDelay = (int) (sr * 4.5);   // 1/2 note at 30 BPM plus headroom
        delayL.prepare (maxDelay);
        delayR.prepare (maxDelay);

        baseWowSamples = (int) (0.004 * sr);     // 4 ms read offset
        maxWowDepth    = (float) (0.0009 * sr);  // +/- 0.9 ms of drift

        const int wowSize = baseWowSamples * 4 + 64;
        wowL.prepare (wowSize);
        wowR.prepare (wowSize);

        reverb.setSampleRate (sr);

        smoothedDelaySamples.reset (sr, 0.15);
        smoothedFeedback    .reset (sr, 0.05);
        smoothedDelayMix    .reset (sr, 0.05);
        smoothedDust        .reset (sr, 0.05);
        smoothedGain        .reset (sr, 0.02);
        smoothedNoiseGate   .reset (sr, 0.25);

        noiseFilter.setCutoff (5000.0f, sr);
        dampL.setCutoff (4000.0f, sr);
        dampR.setCutoff (4000.0f, sr);

        lastDustCutoff = -1.0f;

        reset();
    }

    void FxChain::reset()
    {
        delayL.clear();
        delayR.clear();
        wowL.clear();
        wowR.clear();

        dampL.reset();
        dampR.reset();
        highCutL.reset();
        highCutR.reset();
        noiseFilter.reset();

        reverb.reset();

        wowPhase = flutterPhase = 0.0f;
        crackle = 0.0f;

        smoothedDelaySamples.setCurrentAndTargetValue (juce::jmax (1.0f, (float) (sampleRate * 0.25)));
        smoothedFeedback .setCurrentAndTargetValue (params.delayFb);
        smoothedDelayMix .setCurrentAndTargetValue (params.delayMix);
        smoothedDust     .setCurrentAndTargetValue (params.dust);
        smoothedGain     .setCurrentAndTargetValue (juce::Decibels::decibelsToGain (params.volumeDb, -40.0f));
        smoothedNoiseGate.setCurrentAndTargetValue (0.0f);
    }

    void FxChain::process (juce::AudioBuffer<float>& buffer, double bpm, bool engineActive)
    {
        const int numSamples = buffer.getNumSamples();

        if (numSamples <= 0 || buffer.getNumChannels() < 1)
            return;

        const bool stereo = buffer.getNumChannels() > 1;
        float* L = buffer.getWritePointer (0);
        float* R = stereo ? buffer.getWritePointer (1) : L;

        // ---------------------------------------------------------------- targets
        const double beatSeconds  = 60.0 / juce::jlimit (20.0, 999.0, bpm);
        const double delaySeconds = beatSeconds * (double) delayDivInQuarters (params.delayDiv);
        const float  delaySamples = juce::jlimit (2.0f,
                                                  (float) (delayL.getSize() - 4),
                                                  (float) (delaySeconds * sampleRate));

        smoothedDelaySamples.setTargetValue (delaySamples);

        // The ping-pong loop crosses twice per cycle, so square-rooting the knob
        // makes one full L->R->L cycle decay by exactly the feedback amount.
        smoothedFeedback.setTargetValue (std::sqrt (juce::jlimit (0.0f, 0.95f, params.delayFb)));
        smoothedDelayMix.setTargetValue (juce::jlimit (0.0f, 1.0f, params.delayMix));
        smoothedDust    .setTargetValue (juce::jlimit (0.0f, 1.0f, params.dust));
        smoothedGain    .setTargetValue (juce::Decibels::decibelsToGain (params.volumeDb, -40.0f));
        smoothedNoiseGate.setTargetValue (engineActive ? 1.0f : 0.0f);

        // ---------------------------------------------------------------- delay
        for (int i = 0; i < numSamples; ++i)
        {
            const float d   = smoothedDelaySamples.getNextValue();
            const float fb  = smoothedFeedback.getNextValue();
            const float mix = smoothedDelayMix.getNextValue();

            const float tapL = delayL.read (d);
            const float tapR = delayR.read (d);

            const float inMono = 0.5f * (L[i] + R[i]);

            // Input lands on the left line; the left tap feeds the right, and the
            // right tap feeds back to the left. Echoes alternate across the field.
            delayL.write (inMono + dampR.process (tapR) * fb + 1.0e-20f);
            delayR.write (dampL.process (tapL) * fb + 1.0e-20f);

            L[i] = L[i] * (1.0f - mix) + tapL * mix;
            R[i] = R[i] * (1.0f - mix) + tapR * mix;
        }

        // ---------------------------------------------------------------- reverb
        {
            juce::Reverb::Parameters rp;
            rp.roomSize   = juce::jlimit (0.0f, 1.0f, 0.20f + 0.78f * juce::jlimit (0.0f, 1.0f, params.revSize));
            rp.damping    = 0.50f;                                  // warm tail
            rp.wetLevel   = juce::jlimit (0.0f, 1.0f, params.revMix) * 0.40f;
            rp.dryLevel   = (1.0f - juce::jlimit (0.0f, 1.0f, params.revMix)) * 0.50f;
            rp.width      = 1.0f;
            rp.freezeMode = 0.0f;

            reverb.setParameters (rp);

            if (stereo)
                reverb.processStereo (L, R, numSamples);
            else
                reverb.processMono (L, numSamples);
        }

        // ---------------------------------------------------------------- dust
        const float dustNow = juce::jlimit (0.0f, 1.0f, params.dust);
        const float cutoff  = 18000.0f * std::pow (0.28f, dustNow);

        if (std::abs (cutoff - lastDustCutoff) > 1.0f)
        {
            highCutL.setCutoff (cutoff, sampleRate);
            highCutR.setCutoff (cutoff, sampleRate);
            lastDustCutoff = cutoff;
        }

        const float wowInc     = (float) (0.6 / sampleRate);   // 0.6 Hz drift
        const float flutterInc = (float) (7.3 / sampleRate);   // 7.3 Hz flutter
        const float crackleP   = dustNow * 0.0006f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dust = smoothedDust.getNextValue();
            const float gate = smoothedNoiseGate.getNextValue();

            // -- wow & flutter --
            wowPhase     += wowInc;     if (wowPhase     >= 1.0f) wowPhase     -= 1.0f;
            flutterPhase += flutterInc; if (flutterPhase >= 1.0f) flutterPhase -= 1.0f;

            const float drift = SineTable::get().lookup (wowPhase)     * 0.80f
                              + SineTable::get().lookup (flutterPhase) * 0.20f;

            const float readHead = (float) baseWowSamples + drift * dust * maxWowDepth;

            wowL.write (L[i]);
            wowR.write (R[i]);

            float l = wowL.read (readHead);
            float r = wowR.read (readHead);

            // -- saturation --
            const float drive = 1.0f + dust * 2.5f;
            const float comp  = 1.0f / (1.0f + dust * 0.9f);

            l = softClip (l * drive) * comp;
            r = softClip (r * drive) * comp;

            // -- high cut --
            l = highCutL.process (l);
            r = highCutR.process (r);

            // -- vinyl noise + crackle --
            if (dust > 0.0f)
            {
                const float hiss = noiseFilter.process (rng.nextFloat() * 2.0f - 1.0f);

                if (rng.nextFloat() < crackleP)
                    crackle = (rng.nextFloat() * 2.0f - 1.0f) * 0.8f;

                crackle *= 0.85f;

                const float dirt = (hiss * 0.30f + crackle) * dust * gate * 0.030f;

                l += dirt;
                r += dirt * 0.85f;
            }

            // -- master volume + safety knee --
            const float g = smoothedGain.getNextValue();

            L[i] = softClip (l * g);

            if (stereo)
                R[i] = softClip (r * g);
        }
    }
}
