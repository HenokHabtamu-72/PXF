/*  PXF · SynthEngine.cpp
    Copyright (c) 2026 Henok Habtamu.
    Released under the GNU Affero General Public License v3.0 — see LICENSE.
*/

#include "SynthEngine.h"

namespace pxf
{
    //==============================================================================
    void SynthVoice::prepare (double sr)
    {
        sampleRate = sr;
        adsr.setSampleRate (sr);
        velGain.reset (sr, 0.005);
        reset();
    }

    void SynthVoice::reset()
    {
        active = held = false;
        adsr.reset();
        filterL.reset();
        filterR.reset();
        glideSamplesLeft = 0.0;
        phase1 = phase2 = phaseSub = 0.0f;
    }

    void SynthVoice::kill()
    {
        reset();
    }

    void SynthVoice::noteOn (float note, float velocity, bool glide, float glideFrom,
                             bool retriggerEnv, const SynthParams& p)
    {
        const bool freshStart = ! active;

        // ---- pitch: slide or jump ----
        if (glide && p.glideSeconds > 0.0f)
        {
            if (freshStart)
                noteCurrent = glideFrom;      // start from the last note we played

            noteTarget       = note;
            glideSamplesLeft = (double) p.glideSeconds * sampleRate;
        }
        else
        {
            noteCurrent = noteTarget = note;
            glideSamplesLeft = 0.0;
        }

        // ---- a voice taken over mid-flight keeps its phases and filter state,
        //      so stealing and legato retriggers never click ----
        if (freshStart)
        {
            phase1   = rng.nextFloat();
            phase2   = rng.nextFloat();
            phaseSub = rng.nextFloat();
            filterL.reset();
            filterR.reset();
            adsr.reset();
            velGain.setCurrentAndTargetValue (velocity);
        }
        else
        {
            velGain.setTargetValue (velocity);
        }

        if (retriggerEnv || freshStart)
            adsr.noteOn();

        midiNote = juce::roundToInt (note);
        active   = true;
        held     = true;
    }

    void SynthVoice::noteOff()
    {
        held = false;
        adsr.noteOff();
    }

    void SynthVoice::advanceGlide (int numSamples) noexcept
    {
        if (glideSamplesLeft <= 0.0)
        {
            noteCurrent = noteTarget;
            return;
        }

        // Constant-time glide: always lands exactly on the target.
        if (glideSamplesLeft <= (double) numSamples)
        {
            noteCurrent      = noteTarget;
            glideSamplesLeft = 0.0;
            return;
        }

        const float step = (noteTarget - noteCurrent) * (float) ((double) numSamples / glideSamplesLeft);
        noteCurrent      += step;
        glideSamplesLeft -= (double) numSamples;
    }

    void SynthVoice::render (juce::AudioBuffer<float>& out, int startSample, int numSamples, const SynthParams& p)
    {
        if (! active)
            return;

        adsr.setParameters ({ p.attack, p.decay, p.sustain, p.release });

        const bool  stereo = out.getNumChannels() > 1;
        float*      L      = out.getWritePointer (0);
        float*      R      = stereo ? out.getWritePointer (1) : L;
        const float sr     = (float) sampleRate;

        // Pitch, filter coefficients and gains update once per 16-sample chunk
        // (a ~3 kHz control rate — smooth glides, negligible CPU).
        constexpr int kChunk = 16;

        int   index     = startSample;
        int   remaining = numSamples;

        while (remaining > 0 && active)
        {
            const int n = juce::jmin (kChunk, remaining);

            advanceGlide (n);

            const float freq  = midiToFreq (juce::jlimit (0.0f, 127.0f, noteCurrent));
            const float cents = p.detuneCents;

            const float f1 = freq * std::exp2 (-cents / 1200.0f);
            const float f2 = freq * std::exp2 ( cents / 1200.0f);

            const float d1 = juce::jlimit (0.0f, 0.45f, f1 / sr);
            const float d2 = juce::jlimit (0.0f, 0.45f, f2 / sr);
            const float dS = juce::jlimit (0.0f, 0.45f, freq * 0.5f / sr);

            const float a1 = std::sqrt (1.0f - p.oscMix);
            const float a2 = std::sqrt (p.oscMix);

            // A touch of stereo spread that opens up as the oscillators detune.
            const float spread = 0.20f * (cents / 50.0f);
            const float pan1   = 0.5f - spread;
            const float pan2   = 0.5f + spread;
            const float gL1 = std::cos (pan1 * kPi * 0.5f), gR1 = std::sin (pan1 * kPi * 0.5f);
            const float gL2 = std::cos (pan2 * kPi * 0.5f), gR2 = std::sin (pan2 * kPi * 0.5f);

            SvfCoeffs coeffs;
            coeffs.set (p.cutoffHz, p.resoQ, sampleRate);

            for (int k = 0; k < n; ++k)
            {
                const float env = adsr.getNextSample();
                const float vel = velGain.getNextValue();

                const float o1  = renderWave (p.osc1, phase1, d1) * a1;
                const float o2  = renderWave (p.osc2, phase2, d2) * a2;
                const float sub = SineTable::get().lookup (phaseSub) * p.subLevel * 0.8f;

                phase1   += d1; if (phase1   >= 1.0f) phase1   -= 1.0f;
                phase2   += d2; if (phase2   >= 1.0f) phase2   -= 1.0f;
                phaseSub += dS; if (phaseSub >= 1.0f) phaseSub -= 1.0f;

                float l = o1 * gL1 + o2 * gL2 + sub * 0.7071f;
                float r = o1 * gR1 + o2 * gR2 + sub * 0.7071f;

                l = filterL.lowpass (l, coeffs);
                r = filterR.lowpass (r, coeffs);

                const float g = env * vel * 0.32f;

                if (stereo)
                {
                    L[index + k] += l * g;
                    R[index + k] += r * g;
                }
                else
                {
                    L[index + k] += 0.5f * (l + r) * g;
                }
            }

            index     += n;
            remaining -= n;

            if (! adsr.isActive())
            {
                active = false;
                held   = false;
            }
        }
    }

    //==============================================================================
    void SynthEngine::prepare (double sr)
    {
        sampleRate = sr;

        for (auto& v : voices)
            v.prepare (sr);

        reset();
    }

    void SynthEngine::reset()
    {
        for (auto& v : voices)
            v.kill();

        heldNotes.clearQuick();
        lastPlayedNote = -1;
        order = 0;
    }

    void SynthEngine::setParams (const SynthParams& p)
    {
        // Switching voice mode mid-phrase: release everything gracefully rather
        // than leaving a stale note stack behind. No click — it's a release, not a cut.
        if (p.mono != params.mono)
        {
            allNotesOff();
            heldNotes.clearQuick();
        }

        params = p;
    }

    SynthVoice* SynthEngine::allocateVoice()
    {
        SynthVoice* best = nullptr;

        // 1. an idle voice
        for (auto& v : voices)
            if (! v.isActive())
                return &v;

        // 2. the oldest voice already in its release tail
        for (auto& v : voices)
            if (v.isReleasing() && (best == nullptr || v.getStartOrder() < best->getStartOrder()))
                best = &v;

        if (best != nullptr)
            return best;

        // 3. the oldest voice overall
        best = &voices[0];
        for (auto& v : voices)
            if (v.getStartOrder() < best->getStartOrder())
                best = &v;

        return best;
    }

    void SynthEngine::noteOn (int note, float velocity)
    {
        note = juce::jlimit (0, 127, note);
        velocityOf[note] = velocity;

        if (params.mono)
        {
            const bool wasHeld = ! heldNotes.isEmpty();

            heldNotes.removeAllInstancesOf (note);
            heldNotes.add (note);

            auto& v = voices[0];

            // Slide only when notes overlap. Retrigger the envelope unless we're
            // playing legato into an already-sounding note.
            const bool slide     = wasHeld;
            const bool retrigger = (! params.legato) || (! wasHeld) || (! v.isActive());

            v.noteOn ((float) note, velocity, slide, v.getCurrentNote(), retrigger, params);
            v.setStartOrder (++order);

            // Silence any voices left over from a poly patch.
            for (int i = 1; i < kMaxVoices; ++i)
                if (voices[i].isActive() && voices[i].isHeld())
                    voices[i].noteOff();
        }
        else
        {
            auto* v = allocateVoice();

            const bool slide = (params.glideSeconds > 0.0f && lastPlayedNote >= 0);

            v->noteOn ((float) note, velocity, slide, (float) lastPlayedNote, true, params);
            v->setStartOrder (++order);
        }

        lastPlayedNote = note;
    }

    void SynthEngine::noteOff (int note)
    {
        note = juce::jlimit (0, 127, note);

        if (params.mono)
        {
            const bool wasOnTop = (! heldNotes.isEmpty() && heldNotes.getLast() == note);

            heldNotes.removeAllInstancesOf (note);

            auto& v = voices[0];

            if (heldNotes.isEmpty())
            {
                v.noteOff();
            }
            else if (wasOnTop)
            {
                // Fall back to the note still under our fingers, sliding as we go.
                const int back = heldNotes.getLast();
                v.noteOn ((float) back, velocityOf[back], true, v.getCurrentNote(),
                          ! params.legato, params);
            }

            return;
        }

        for (auto& v : voices)
            if (v.isActive() && v.isHeld() && v.getNoteNumber() == note)
                v.noteOff();
    }

    void SynthEngine::allNotesOff()
    {
        heldNotes.clearQuick();

        for (auto& v : voices)
            if (v.isActive() && v.isHeld())
                v.noteOff();
    }

    void SynthEngine::render (juce::AudioBuffer<float>& out, int startSample, int numSamples)
    {
        if (numSamples <= 0)
            return;

        for (auto& v : voices)
            v.render (out, startSample, numSamples, params);
    }

    bool SynthEngine::isActive() const
    {
        for (const auto& v : voices)
            if (v.isActive())
                return true;

        return false;
    }
}
