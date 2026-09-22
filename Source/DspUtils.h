/*  PXF · DspUtils.h
    Copyright (c) 2026 Henok Habtamu.
    Released under the GNU Affero General Public License v3.0 — see LICENSE.

    Small, self-contained DSP building blocks. No dependencies beyond juce_core.
*/

#pragma once

#include <juce_core/juce_core.h>
#include <cmath>
#include <vector>
#include <algorithm>

namespace pxf
{
    inline constexpr float kPi    = 3.14159265358979323846f;
    inline constexpr float kTwoPi = 6.28318530717958647692f;

    inline float midiToFreq (float noteNumber) noexcept
    {
        return 440.0f * std::exp2 ((noteNumber - 69.0f) / 12.0f);
    }

    //==============================================================================
    /** Cheap, accurate sine via a 2048-point table with linear interpolation.
        Phase must be wrapped into [0, 1). */
    class SineTable
    {
    public:
        static const SineTable& get() noexcept
        {
            static const SineTable instance;
            return instance;
        }

        inline float lookup (float phase) const noexcept
        {
            const float x  = phase * (float) kSize;
            const int   i  = (int) x;
            const float f  = x - (float) i;
            const int   i0 = i & (kSize - 1);
            return table[i0] + (table[i0 + 1] - table[i0]) * f;
        }

    private:
        static constexpr int kSize = 2048;
        float table[kSize + 1];

        SineTable() noexcept
        {
            for (int i = 0; i <= kSize; ++i)
                table[i] = std::sin (kTwoPi * (float) i / (float) kSize);
        }
    };

    //==============================================================================
    // Band-limiting correction terms.

    inline float polyBlep (float t, float dt) noexcept
    {
        if (dt <= 0.0f) return 0.0f;

        if (t < dt)             { const float x = t / dt;          return x + x - x * x - 1.0f; }
        if (t > 1.0f - dt)      { const float x = (t - 1.0f) / dt; return x * x + x + x + 1.0f; }
        return 0.0f;
    }

    inline float polyBlamp (float t, float dt) noexcept
    {
        if (dt <= 0.0f) return 0.0f;

        if (t < dt)        { const float x = t / dt - 1.0f;          return -0.333333333f * x * x * x; }
        if (t > 1.0f - dt) { const float x = (t - 1.0f) / dt + 1.0f; return  0.333333333f * x * x * x; }
        return 0.0f;
    }

    enum class Wave { sine = 0, triangle = 1, saw = 2, square = 3 };

    inline Wave waveFromIndex (int i) noexcept
    {
        return static_cast<Wave> (juce::jlimit (0, 3, i));
    }

    /** phase in [0,1), dt = normalised frequency (freq / sampleRate). */
    inline float renderWave (Wave w, float phase, float dt) noexcept
    {
        switch (w)
        {
            case Wave::sine:
                return SineTable::get().lookup (phase);

            case Wave::saw:
            {
                float s = 2.0f * phase - 1.0f;
                s -= polyBlep (phase, dt);
                return s * 0.92f;
            }

            case Wave::square:
            {
                float s = phase < 0.5f ? 1.0f : -1.0f;
                s += polyBlep (phase, dt);

                float t2 = phase + 0.5f;
                if (t2 >= 1.0f) t2 -= 1.0f;
                s -= polyBlep (t2, dt);

                return s * 0.80f;
            }

            case Wave::triangle:
            default:
            {
                float t1 = phase + 0.25f; if (t1 >= 1.0f) t1 -= 1.0f;
                float t2 = phase + 0.75f; if (t2 >= 1.0f) t2 -= 1.0f;

                float y = phase * 4.0f;
                if      (y >= 3.0f) y -= 4.0f;
                else if (y >  1.0f) y  = 2.0f - y;

                y += 4.0f * dt * (polyBlamp (t1, dt) - polyBlamp (t2, dt));
                return y;
            }
        }
    }

    //==============================================================================
    /** Transparent below the knee, gently rounded above it. */
    inline float softClip (float x) noexcept
    {
        constexpr float knee = 0.85f;
        constexpr float rest = 1.0f - knee;

        if (x >  knee) return  knee + rest * std::tanh ((x - knee) / rest);
        if (x < -knee) return -knee - rest * std::tanh ((-x - knee) / rest);
        return x;
    }

    //==============================================================================
    /** Zavalishin TPT state-variable filter. Coefficients are shared between
        channels; each channel keeps its own state. */
    struct SvfCoeffs
    {
        float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;

        void set (float cutoffHz, float q, double sampleRate) noexcept
        {
            const float sr = (float) sampleRate;
            const float fc = juce::jlimit (20.0f, sr * 0.45f, cutoffHz);
            const float g  = std::tan (kPi * fc / sr);
            const float k  = 1.0f / juce::jmax (0.05f, q);

            a1 = 1.0f / (1.0f + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;
        }
    };

    struct SvfState
    {
        float ic1 = 0.0f, ic2 = 0.0f;

        inline float lowpass (float v0, const SvfCoeffs& c) noexcept
        {
            const float v3 = v0 - ic2;
            const float v1 = c.a1 * ic1 + c.a2 * v3;
            const float v2 = ic2 + c.a2 * ic1 + c.a3 * v3;

            ic1 = 2.0f * v1 - ic1;
            ic2 = 2.0f * v2 - ic2;
            return v2;
        }

        void reset() noexcept { ic1 = ic2 = 0.0f; }
    };

    //==============================================================================
    struct OnePoleLP
    {
        float z = 0.0f, a = 1.0f;

        void setCutoff (float hz, double sampleRate) noexcept
        {
            const float sr = (float) sampleRate;
            const float f  = juce::jlimit (10.0f, sr * 0.49f, hz);
            a = 1.0f - std::exp (-kTwoPi * f / sr);
        }

        inline float process (float x) noexcept
        {
            z += a * (x - z);
            return z;
        }

        void reset() noexcept { z = 0.0f; }
    };

    //==============================================================================
    /** Fractional-delay circular buffer. Call read() before write() for a plain
        delay, or write() before read() when the delay is a modulated read head. */
    class DelayLine
    {
    public:
        void prepare (int maxSamples)
        {
            size = juce::jmax (8, maxSamples);
            buffer.assign ((size_t) size, 0.0f);
            writePos = 0;
        }

        void clear() noexcept
        {
            std::fill (buffer.begin(), buffer.end(), 0.0f);
            writePos = 0;
        }

        inline void write (float x) noexcept
        {
            buffer[(size_t) writePos] = x;
            if (++writePos >= size)
                writePos = 0;
        }

        inline float read (float delaySamples) const noexcept
        {
            const float d = juce::jlimit (1.0f, (float) (size - 2), delaySamples);

            float rp = (float) writePos - d;
            while (rp < 0.0f)
                rp += (float) size;

            const int   i0 = (int) rp;
            const float f  = rp - (float) i0;
            const int   i1 = (i0 + 1 < size) ? i0 + 1 : 0;

            return buffer[(size_t) i0] + (buffer[(size_t) i1] - buffer[(size_t) i0]) * f;
        }

        int getSize() const noexcept { return size; }

    private:
        std::vector<float> buffer;
        int size = 0, writePos = 0;
    };
}
