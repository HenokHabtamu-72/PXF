/*  PXF · GrainEngine.h
    Copyright (c) 2026 Henok Habtamu.
    Released under the GNU Affero General Public License v3.0 — see LICENSE.

    Granulises a user-loaded one-shot. Each held MIDI note spawns its own stream of
    overlapping grains, transposed to that note (C4 = original pitch). Every grain is
    windowed to zero at both ends, so grain boundaries never click.

    Threading: the file is decoded and resampled on the message thread, then swapped
    into the audio thread's working buffer under a spin lock. The audio thread only
    ever try-locks, so it can never be blocked by disk I/O.
*/

#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include "DspUtils.h"

namespace pxf
{
    struct GrainParams
    {
        float sizeMs     = 90.0f;    // 10 .. 500
        float density    = 22.0f;    // 1 .. 100 grains/sec
        float position   = 0.0f;     // 0 .. 100 %
        float spray      = 10.0f;    // 0 .. 100 %
        float pitchSemis = 0.0f;     // -24 .. +24
        float shape      = 0.30f;    // 0 = Hann .. 1 = sharp attack
    };

    class GrainEngine
    {
    public:
        static constexpr int kMaxVoices      = 8;
        static constexpr int kGrainsPerVoice = 48;
        static constexpr int kGlobalGrainCap = 220;   // bounds worst-case CPU
        static constexpr int kEnvSize        = 512;
        static constexpr int kVizSlots       = 32;
        static constexpr int kPeakBuckets    = 1024;
        static constexpr double kMaxSeconds  = 30.0;

        GrainEngine();

        void prepare (double sampleRate);
        void reset();

        // ---- sample loading (message thread) ----
        bool loadFile (const juce::File& file, juce::AudioFormatManager& formats, juce::String& errorOut);
        void clearSample();

        bool         hasSample()     const noexcept { return sampleLoaded.load(); }
        int          getVersion()    const noexcept { return version.load(); }
        juce::String getFileName()   const;
        juce::File   getFile()       const;
        void         getPeaks (std::vector<float>& mins, std::vector<float>& maxs) const;

        // ---- audio thread ----
        void setParams (const GrainParams& p) noexcept { params = p; }

        void noteOn (int note, float velocity);
        void noteOff (int note);
        void allNotesOff();

        void render (juce::AudioBuffer<float>& out, int startSample, int numSamples);
        bool isActive() const;

        /** Normalised read positions of live grains, for the waveform display. */
        int getVizPositions (float* dest, int maxCount) const;

    private:
        struct Grain
        {
            double pos    = 0.0;
            double inc    = 1.0;
            int    length = 0;
            int    age    = 0;
            float  invLen = 0.0f;
            float  amp    = 0.0f;
        };

        struct Voice
        {
            bool   active = false;
            bool   held   = false;
            int    note   = 60;
            float  vel    = 1.0f;
            double ratio  = 1.0;
            double spawnAccumulator = 0.0;
            int    grainCount = 0;
            Grain  grains[kGrainsPerVoice];
            juce::Random rng;
        };

        void rebuildWorkBuffer();
        void buildEnvTable (float shape);
        void computePeaks (const juce::AudioBuffer<float>& buffer);

        double sampleRate = 44100.0;

        juce::AudioBuffer<float> original;   // as decoded, at the file's own rate
        double originalRate = 44100.0;

        juce::AudioBuffer<float> work;       // resampled to the device rate
        juce::SpinLock bufferLock;
        std::atomic<bool> sampleLoaded { false };
        std::atomic<int>  version { 0 };

        juce::File   sourceFile;
        juce::String sourceName;
        juce::CriticalSection peakLock;
        std::vector<float> peakMin, peakMax;

        Voice voices[kMaxVoices];
        int   globalGrains = 0;

        float envTable[kEnvSize + 1] { };
        float currentShape = -1.0f;

        GrainParams params;

        std::atomic<float> vizPos[kVizSlots];
        std::atomic<int>   vizCount { 0 };
    };
}
