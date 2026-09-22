/*  PXF · GrainEngine.cpp
    Copyright (c) 2026 Henok Habtamu.
    Released under the GNU Affero General Public License v3.0 — see LICENSE.
*/

#include "GrainEngine.h"

namespace pxf
{
    GrainEngine::GrainEngine()
    {
        for (auto& v : vizPos)
            v.store (0.0f);

        buildEnvTable (0.30f);
    }

    //==============================================================================
    void GrainEngine::prepare (double sr)
    {
        sampleRate = sr;
        reset();

        if (original.getNumSamples() > 0)
            rebuildWorkBuffer();
    }

    void GrainEngine::reset()
    {
        const juce::SpinLock::ScopedLockType lock (bufferLock);

        for (auto& v : voices)
        {
            v.active = v.held = false;
            v.grainCount = 0;
            v.spawnAccumulator = 0.0;
        }

        globalGrains = 0;
        vizCount.store (0);
    }

    //==============================================================================
    void GrainEngine::buildEnvTable (float shape)
    {
        // shape 0 -> Hann (soft both ends). shape 1 -> fast attack, long smooth decay.
        const float attackFrac = juce::jmap (juce::jlimit (0.0f, 1.0f, shape), 0.5f, 0.02f);

        for (int i = 0; i < kEnvSize; ++i)
        {
            const float p = (float) i / (float) (kEnvSize - 1);

            envTable[i] = (p < attackFrac)
                            ? 0.5f * (1.0f - std::cos (kPi * p / attackFrac))
                            : 0.5f * (1.0f + std::cos (kPi * (p - attackFrac) / (1.0f - attackFrac)));
        }

        envTable[kEnvSize] = 0.0f;   // guard slot for the interpolator
        currentShape = shape;
    }

    //==============================================================================
    bool GrainEngine::loadFile (const juce::File& file, juce::AudioFormatManager& formats, juce::String& errorOut)
    {
        std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));

        if (reader == nullptr)
        {
            errorOut = "Can't read that file. Try WAV, AIFF, FLAC or MP3.";
            return false;
        }

        const juce::int64 maxLen = (juce::int64) (reader->sampleRate * kMaxSeconds);
        const int numSamples = (int) juce::jmin (maxLen, reader->lengthInSamples);

        if (numSamples < 16)
        {
            errorOut = "That file is empty.";
            return false;
        }

        const int numChannels = (int) juce::jmin ((juce::uint32) 2, reader->numChannels);

        juce::AudioBuffer<float> decoded (numChannels, numSamples);
        reader->read (&decoded, 0, numSamples, 0, true, numChannels > 1);

        original     = std::move (decoded);
        originalRate = reader->sampleRate;
        sourceFile   = file;
        sourceName   = file.getFileName();

        rebuildWorkBuffer();
        return true;
    }

    void GrainEngine::clearSample()
    {
        {
            const juce::SpinLock::ScopedLockType lock (bufferLock);

            work.setSize (0, 0);
            sampleLoaded.store (false);

            for (auto& v : voices)
            {
                v.active = v.held = false;
                v.grainCount = 0;
            }

            globalGrains = 0;
        }

        original.setSize (0, 0);
        sourceFile = juce::File();
        sourceName = {};

        {
            const juce::ScopedLock sl (peakLock);
            peakMin.clear();
            peakMax.clear();
        }

        ++version;
    }

    void GrainEngine::rebuildWorkBuffer()
    {
        if (original.getNumSamples() < 16 || sampleRate <= 0.0)
            return;

        juce::AudioBuffer<float> resampled;

        const double ratio = originalRate / sampleRate;   // input samples per output sample

        if (std::abs (ratio - 1.0) < 1.0e-9)
        {
            resampled.makeCopyOf (original);
        }
        else
        {
            const int outLength = (int) std::ceil ((double) original.getNumSamples() / ratio);
            resampled.setSize (original.getNumChannels(), juce::jmax (16, outLength), false, true, false);

            for (int ch = 0; ch < original.getNumChannels(); ++ch)
            {
                juce::LagrangeInterpolator interp;
                interp.reset();
                interp.process (ratio,
                                original.getReadPointer (ch),
                                resampled.getWritePointer (ch),
                                resampled.getNumSamples());
            }
        }

        // Level the sample so grain density behaves the same for every one-shot.
        const float mag = resampled.getMagnitude (0, resampled.getNumSamples());
        if (mag > 1.0e-4f)
            resampled.applyGain (0.9f / mag);

        computePeaks (resampled);

        {
            const juce::SpinLock::ScopedLockType lock (bufferLock);

            work = std::move (resampled);

            // Live grains hold read positions into the old buffer — drop them.
            // Held notes stay held, so they simply start spawning into the new sample.
            for (auto& v : voices)
            {
                v.grainCount = 0;
                v.spawnAccumulator = 0.0;
                v.active = v.held;
            }

            globalGrains = 0;
            sampleLoaded.store (work.getNumSamples() > 16);
        }

        ++version;
    }

    void GrainEngine::computePeaks (const juce::AudioBuffer<float>& buffer)
    {
        std::vector<float> mins, maxs;

        const int n = buffer.getNumSamples();
        const int channels = buffer.getNumChannels();

        if (n > 0 && channels > 0)
        {
            mins.resize (kPeakBuckets, 0.0f);
            maxs.resize (kPeakBuckets, 0.0f);

            for (int b = 0; b < kPeakBuckets; ++b)
            {
                const int start = (int) ((juce::int64) b       * n / kPeakBuckets);
                const int end   = (int) ((juce::int64) (b + 1) * n / kPeakBuckets);

                float lo = 0.0f, hi = 0.0f;

                for (int i = start; i < juce::jmax (start + 1, end) && i < n; ++i)
                {
                    float s = 0.0f;
                    for (int ch = 0; ch < channels; ++ch)
                        s += buffer.getSample (ch, i);

                    s /= (float) channels;

                    lo = juce::jmin (lo, s);
                    hi = juce::jmax (hi, s);
                }

                mins[(size_t) b] = lo;
                maxs[(size_t) b] = hi;
            }
        }

        const juce::ScopedLock sl (peakLock);
        peakMin = std::move (mins);
        peakMax = std::move (maxs);
    }

    void GrainEngine::getPeaks (std::vector<float>& mins, std::vector<float>& maxs) const
    {
        const juce::ScopedLock sl (peakLock);
        mins = peakMin;
        maxs = peakMax;
    }

    juce::String GrainEngine::getFileName() const { return sourceName; }
    juce::File   GrainEngine::getFile()     const { return sourceFile; }

    //==============================================================================
    void GrainEngine::noteOn (int note, float velocity)
    {
        if (! sampleLoaded.load())
            return;

        note = juce::jlimit (0, 127, note);

        Voice* target = nullptr;

        for (auto& v : voices)
            if (v.active && v.held && v.note == note)
                target = &v;                                    // retrigger the same note

        if (target == nullptr)
            for (auto& v : voices)
                if (! v.active)
                {
                    target = &v;
                    break;
                }

        if (target == nullptr)
            for (auto& v : voices)
                if (! v.held)
                {
                    target = &v;                                // steal a fading stream
                    break;
                }

        if (target == nullptr)
            target = &voices[0];

        target->active = true;
        target->held   = true;
        target->note   = note;
        target->vel    = juce::jlimit (0.05f, 1.0f, velocity);
    }

    void GrainEngine::noteOff (int note)
    {
        note = juce::jlimit (0, 127, note);

        for (auto& v : voices)
            if (v.active && v.held && v.note == note)
                v.held = false;    // stop spawning; live grains finish and fade out
    }

    void GrainEngine::allNotesOff()
    {
        for (auto& v : voices)
            v.held = false;
    }

    bool GrainEngine::isActive() const
    {
        for (const auto& v : voices)
            if (v.active)
                return true;

        return false;
    }

    int GrainEngine::getVizPositions (float* dest, int maxCount) const
    {
        const int n = juce::jlimit (0, juce::jmin (maxCount, (int) kVizSlots), vizCount.load());

        for (int i = 0; i < n; ++i)
            dest[i] = vizPos[i].load();

        return n;
    }

    //==============================================================================
    void GrainEngine::render (juce::AudioBuffer<float>& out, int startSample, int numSamples)
    {
        if (numSamples <= 0)
            return;

        if (std::abs (params.shape - currentShape) > 1.0e-4f)
            buildEnvTable (params.shape);

        const juce::SpinLock::ScopedTryLockType lock (bufferLock);

        if (! lock.isLocked() || ! sampleLoaded.load())
            return;

        const int n = work.getNumSamples();
        const int channels = work.getNumChannels();

        if (n < 16 || channels < 1)
            return;

        const float* src0 = work.getReadPointer (0);
        const float* src1 = (channels > 1) ? work.getReadPointer (1) : src0;

        const bool stereo = out.getNumChannels() > 1;
        float* L = out.getWritePointer (0);
        float* R = stereo ? out.getWritePointer (1) : L;

        // ---- per-block grain settings ----
        const double sizeSec   = (double) juce::jlimit (10.0f, 500.0f, params.sizeMs) * 0.001;
        const int    grainLen  = juce::jmax (8, (int) (sizeSec * sampleRate));
        const double density   = (double) juce::jlimit (1.0f, 100.0f, params.density);
        const double spawnInc  = density / sampleRate;

        const float  overlap   = (float) (density * sizeSec);
        const float  grainGain = 1.0f / std::sqrt (juce::jmax (1.0f, overlap));

        const float  posNorm   = juce::jlimit (0.0f, 1.0f, params.position * 0.01f);
        const float  sprayNorm = juce::jlimit (0.0f, 1.0f, params.spray    * 0.01f);
        const float  pitch     = juce::jlimit (-24.0f, 24.0f, params.pitchSemis);

        for (auto& v : voices)
        {
            if (! v.active)
                continue;

            // C4 (MIDI 60) plays the sample at its original pitch.
            v.ratio = (double) std::exp2 (((float) (v.note - 60) + pitch) / 12.0f);

            const double span = (double) grainLen * v.ratio;
            const bool   fits = span < (double) n - 2.0;

            for (int k = 0; k < numSamples; ++k)
            {
                // ---- spawn ----
                if (v.held)
                {
                    v.spawnAccumulator += spawnInc;

                    while (v.spawnAccumulator >= 1.0)
                    {
                        v.spawnAccumulator -= 1.0;

                        if (v.grainCount < kGrainsPerVoice && globalGrains < kGlobalGrainCap)
                        {
                            auto& g = v.grains[v.grainCount++];
                            ++globalGrains;

                            double start = (double) posNorm * (double) (n - 1);

                            if (sprayNorm > 0.0f)
                                start += (double) ((v.rng.nextFloat() * 2.0f - 1.0f) * sprayNorm * 0.5f) * (double) n;

                            if (fits)
                            {
                                // Keep the whole grain inside the sample: no wrap, no click.
                                start = juce::jlimit (0.0, (double) n - span - 2.0, start);
                            }
                            else
                            {
                                while (start <  0.0)        start += (double) n;
                                while (start >= (double) n) start -= (double) n;
                            }

                            g.pos    = start;
                            g.inc    = v.ratio;
                            g.length = grainLen;
                            g.invLen = 1.0f / (float) grainLen;
                            g.age    = 0;
                            g.amp    = v.vel * grainGain;
                        }
                    }
                }

                // ---- render live grains ----
                float sl = 0.0f, sr = 0.0f;

                for (int gi = 0; gi < v.grainCount; )
                {
                    auto& g = v.grains[gi];

                    const float  ep = (float) g.age * g.invLen;
                    const float  ex = ep * (float) (kEnvSize - 1);
                    const int    ei = juce::jlimit (0, kEnvSize - 1, (int) ex);
                    const float  ef = ex - (float) ei;
                    const float  env = envTable[ei] + (envTable[ei + 1] - envTable[ei]) * ef;

                    const int   i0 = juce::jlimit (0, n - 1, (int) g.pos);
                    const float fr = (float) (g.pos - (double) i0);
                    const int   i1 = (i0 + 1 < n) ? i0 + 1 : 0;

                    const float a = env * g.amp;

                    sl += (src0[i0] + (src0[i1] - src0[i0]) * fr) * a;
                    sr += (src1[i0] + (src1[i1] - src1[i0]) * fr) * a;

                    g.pos += g.inc;
                    if      (g.pos >= (double) n) g.pos -= (double) n;
                    else if (g.pos <  0.0)        g.pos += (double) n;

                    if (++g.age >= g.length)
                    {
                        v.grains[gi] = v.grains[--v.grainCount];   // swap-remove
                        --globalGrains;
                    }
                    else
                    {
                        ++gi;
                    }
                }

                if (stereo)
                {
                    L[startSample + k] += sl;
                    R[startSample + k] += sr;
                }
                else
                {
                    L[startSample + k] += 0.5f * (sl + sr);
                }
            }

            if (! v.held && v.grainCount == 0)
                v.active = false;
        }

        // ---- snapshot for the waveform display ----
        int count = 0;

        for (const auto& v : voices)
        {
            if (! v.active)
                continue;

            for (int gi = 0; gi < v.grainCount && count < kVizSlots; ++gi)
                vizPos[count++].store ((float) (v.grains[gi].pos / (double) n));
        }

        vizCount.store (count);
    }
}
