/*  PXF · SynthEngine.h
    Copyright (c) 2026 Henok Habtamu.
    Released under the GNU Affero General Public License v3.0 — see LICENSE.

    8-voice polyphonic synth: 2 oscillators + sub, one low-pass filter, one ADSR.

    VOICE MODES
    -----------
    POLY   (mono off)  Up to 8 voices. If Glide > 0, a new voice starts at the pitch
                       of the previously played note and slides to its own — classic
                       poly portamento. Glide = 0 means every note starts on pitch.

    MONO   (mono on)   One voice, last-note priority with a held-note stack. Playing a
                       new note while another is still held slides the pitch over the
                       Glide time. Releasing the top note slides back down to whatever
                       is still held.

    LEGATO (mono on)   Controls the envelope, not the pitch. With LEGATO on, the ADSR
                       is only retriggered by the first note of a phrase — overlapping
                       notes slide without re-attacking. With LEGATO off, every note
                       retriggers the envelope but the pitch still slides.

    A note played after full release always starts on pitch: you slide by overlapping
    notes, which is what a player expects from a legato slide.
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "DspUtils.h"

namespace pxf
{
    struct SynthParams
    {
        Wave  osc1 = Wave::saw;
        Wave  osc2 = Wave::triangle;
        float oscMix       = 0.5f;
        float detuneCents  = 8.0f;
        float subLevel     = 0.35f;

        float cutoffHz     = 2000.0f;
        float resoQ        = 0.707f;

        float attack       = 0.01f;
        float decay        = 0.40f;
        float sustain      = 0.70f;
        float release      = 0.50f;

        float glideSeconds = 0.0f;
        bool  mono         = false;
        bool  legato       = true;
    };

    //==============================================================================
    class SynthVoice
    {
    public:
        void prepare (double sampleRate);
        void reset();

        /** @param glide        slide from the current pitch instead of jumping
            @param glideFrom    pitch to slide from when the voice starts from silence
            @param retriggerEnv restart the ADSR (false = true legato) */
        void noteOn (float note, float velocity, bool glide, float glideFrom,
                     bool retriggerEnv, const SynthParams& p);

        void noteOff();
        void kill();

        bool isActive()    const noexcept { return active; }
        bool isHeld()      const noexcept { return held; }
        bool isReleasing() const noexcept { return active && ! held; }
        int  getNoteNumber()  const noexcept { return midiNote; }
        float getCurrentNote() const noexcept { return noteCurrent; }

        juce::uint32 getStartOrder() const noexcept { return startOrder; }
        void setStartOrder (juce::uint32 o) noexcept { startOrder = o; }

        void render (juce::AudioBuffer<float>& out, int startSample, int numSamples, const SynthParams& p);

    private:
        void advanceGlide (int numSamples) noexcept;

        double sampleRate = 44100.0;

        bool  active = false, held = false;
        int   midiNote = 60;
        juce::uint32 startOrder = 0;

        float  noteCurrent = 60.0f, noteTarget = 60.0f;
        double glideSamplesLeft = 0.0;

        float phase1 = 0.0f, phase2 = 0.0f, phaseSub = 0.0f;

        SvfState filterL, filterR;
        juce::ADSR adsr;
        juce::SmoothedValue<float> velGain { 1.0f };
        juce::Random rng;
    };

    //==============================================================================
    class SynthEngine
    {
    public:
        static constexpr int kMaxVoices = 8;

        void prepare (double sampleRate);
        void reset();

        void setParams (const SynthParams& p);
        const SynthParams& getParams() const noexcept { return params; }

        void noteOn (int note, float velocity);
        void noteOff (int note);
        void allNotesOff();

        void render (juce::AudioBuffer<float>& out, int startSample, int numSamples);
        bool isActive() const;

    private:
        SynthVoice* allocateVoice();

        SynthVoice voices[kMaxVoices];
        juce::Array<int> heldNotes;          // mono note stack, last-note priority
        float velocityOf[128] { };
        int   lastPlayedNote = -1;
        juce::uint32 order = 0;

        SynthParams params;
        double sampleRate = 44100.0;
    };
}
