/*  PXF · EngineTests.cpp
    Copyright (c) 2026 Henok Habtamu.
    Released under the GNU Affero General Public License v3.0 — see LICENSE.

    Headless smoke tests for the two engines. Renders real audio and checks it,
    with pitch measured by zero-crossing rate. Exits non-zero on failure.
*/

#include <juce_audio_formats/juce_audio_formats.h>
#include "../Source/SynthEngine.h"
#include "../Source/GrainEngine.h"

#include <cstdio>

namespace
{
    constexpr double kRate = 48000.0;

    int failures = 0;

    void check (bool condition, const char* what)
    {
        std::printf ("%s  %s\n", condition ? "[ ok ]" : "[FAIL]", what);
        if (! condition)
            ++failures;
    }

    bool isFinite (const juce::AudioBuffer<float>& b)
    {
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
            for (int i = 0; i < b.getNumSamples(); ++i)
                if (! std::isfinite (b.getSample (ch, i)))
                    return false;
        return true;
    }

    float rms (const juce::AudioBuffer<float>& b, int start, int num)
    {
        double sum = 0.0;
        for (int i = 0; i < num; ++i)
        {
            const float s = b.getSample (0, start + i);
            sum += (double) s * s;
        }
        return (float) std::sqrt (sum / juce::jmax (1, num));
    }

    float peak (const juce::AudioBuffer<float>& b, int start, int num)
    {
        float m = 0.0f;
        for (int i = 0; i < num; ++i)
            m = juce::jmax (m, std::abs (b.getSample (0, start + i)));
        return m;
    }

    /** Estimated frequency from zero crossings over a window. */
    float zcFreq (const juce::AudioBuffer<float>& b, int start, int num)
    {
        int crossings = 0;
        for (int i = start + 1; i < start + num; ++i)
            if ((b.getSample (0, i - 1) < 0.0f) != (b.getSample (0, i) < 0.0f))
                ++crossings;

        return (float) crossings * 0.5f * (float) kRate / (float) num;
    }

    /** A synth params set that produces a near-pure sine, so zero-crossing
        pitch measurement is trustworthy. */
    pxf::SynthParams sineParams()
    {
        pxf::SynthParams p;
        p.osc1 = pxf::Wave::sine;
        p.osc2 = pxf::Wave::sine;
        p.oscMix      = 0.0f;      // all osc1
        p.detuneCents = 0.0f;
        p.subLevel    = 0.0f;
        p.cutoffHz    = 18000.0f;
        p.resoQ       = 0.707f;
        p.attack      = 0.003f;
        p.decay       = 0.05f;
        p.sustain     = 1.0f;
        p.release     = 0.05f;
        return p;
    }

    void render (pxf::SynthEngine& s, juce::AudioBuffer<float>& b, int start, int num)
    {
        s.render (b, start, num);
    }
}

//==============================================================================
static void testPolySynthBasics()
{
    std::printf ("\n-- poly synth --\n");

    pxf::SynthEngine synth;
    synth.prepare (kRate);
    synth.setParams (sineParams());

    juce::AudioBuffer<float> buffer (2, 24000);
    buffer.clear();

    synth.noteOn (60, 0.8f);
    synth.noteOn (64, 0.8f);
    synth.noteOn (67, 0.8f);
    render (synth, buffer, 0, 24000);

    check (isFinite (buffer), "poly output is finite");
    check (rms (buffer, 4000, 8000) > 0.01f, "poly chord is audible");

    synth.allNotesOff();

    juce::AudioBuffer<float> tail (2, 24000);
    tail.clear();
    render (synth, tail, 0, 24000);

    check (peak (tail, 16000, 8000) < 1.0e-3f, "voices fall silent after release");
}

//==============================================================================
static void testMonoLegatoSlide()
{
    std::printf ("\n-- mono/legato slide --\n");

    auto p = sineParams();
    p.mono = true;
    p.legato = true;
    p.glideSeconds = 0.15f;

    pxf::SynthEngine synth;
    synth.prepare (kRate);
    synth.setParams (p);

    // Hold C3 until the envelope settles, then overlap C4.
    juce::AudioBuffer<float> a (2, 24000);
    a.clear();
    synth.noteOn (48, 0.8f);
    render (synth, a, 0, 24000);

    const float f0 = zcFreq (a, 16000, 8000);
    check (std::abs (f0 - 130.8f) < 10.0f, "held C3 sits near 130.8 Hz");

    juce::AudioBuffer<float> b (2, 24000);
    b.clear();
    synth.noteOn (60, 0.8f);            // overlap: should SLIDE, not jump
    render (synth, b, 0, 24000);

    const float early = zcFreq (b, 0,     2400);   //   0- 50 ms
    const float mid   = zcFreq (b, 3600,  2400);   //  75-125 ms
    const float late  = zcFreq (b, 12000, 8000);   // 250 ms onward

    std::printf ("       slide: %.1f Hz -> %.1f Hz -> %.1f Hz\n", early, mid, late);

    check (early < 180.0f,                 "still near the old pitch right after the overlap");
    check (mid > early + 15.0f,            "pitch is rising mid-glide");
    check (std::abs (late - 261.6f) < 12.0f, "lands exactly on C4");

    // Legato: the envelope must not re-attack on the overlap. With sustain = 1
    // any retrigger would drag the level toward zero for a moment.
    check (peak (b, 0, 480) > 0.15f, "no envelope dip on the legato overlap");

    // Release the top note: mono falls back to the held C3, sliding down.
    juce::AudioBuffer<float> c (2, 24000);
    c.clear();
    synth.noteOff (60);
    render (synth, c, 0, 24000);

    const float back = zcFreq (c, 16000, 8000);
    std::printf ("       fall-back: %.1f Hz\n", back);
    check (std::abs (back - 130.8f) < 10.0f, "releasing the top note slides back to C3");

    check (isFinite (a) && isFinite (b) && isFinite (c), "mono output is finite");
}

//==============================================================================
static void testMonoRetrigger()
{
    std::printf ("\n-- mono retrigger vs legato --\n");

    // With sustain well below 1, a retrigger sends the envelope from the sustain
    // plateau back up to full peak. Legato leaves it sitting on the plateau.
    // (Retriggers climb from the current level, not zero — resetting to zero
    // mid-waveform would click.)
    auto p = sineParams();
    p.mono = true;
    p.glideSeconds = 0.0f;
    p.attack  = 0.05f;
    p.decay   = 0.15f;
    p.sustain = 0.30f;

    auto plateauThenOverlap = [&p] (bool legato) -> std::pair<float, float>
    {
        p.legato = legato;

        pxf::SynthEngine synth;
        synth.prepare (kRate);
        synth.setParams (p);

        juce::AudioBuffer<float> a (2, 48000);
        a.clear();
        synth.noteOn (48, 0.9f);
        render (synth, a, 0, 48000);                       // settle on the plateau

        const float plateau = peak (a, 43000, 4800);

        juce::AudioBuffer<float> b (2, 19200);
        b.clear();
        synth.noteOn (60, 0.9f);                           // the overlap
        render (synth, b, 0, 19200);

        return { plateau, peak (b, 0, 19200) };
    };

    const auto [plateauOff, afterOff] = plateauThenOverlap (false);
    std::printf ("       legato off: plateau %.3f -> peak %.3f after overlap\n", plateauOff, afterOff);
    check (afterOff > plateauOff * 1.8f, "legato off re-attacks to full level");

    const auto [plateauOn, afterOn] = plateauThenOverlap (true);
    std::printf ("       legato on:  plateau %.3f -> peak %.3f after overlap\n", plateauOn, afterOn);
    check (afterOn < plateauOn * 1.4f, "legato on stays on the sustain plateau");
}

//==============================================================================
static void testGrainEngine()
{
    std::printf ("\n-- grain engine --\n");

    // Write a 1 s / 440 Hz sine to a temp wav, then load it like a user would.
    const auto file = juce::File::getSpecialLocation (juce::File::tempDirectory)
                          .getChildFile ("pxf_test_tone.wav");
    file.deleteFile();

    {
        juce::AudioBuffer<float> tone (1, (int) kRate);
        for (int i = 0; i < tone.getNumSamples(); ++i)
            tone.setSample (0, i, 0.5f * std::sin (pxf::kTwoPi * 440.0f * (float) i / (float) kRate));

        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> writer (
            wav.createWriterFor (new juce::FileOutputStream (file), kRate, 1, 16, {}, 0));

        check (writer != nullptr, "test wav writer created");
        writer->writeFromAudioSampleBuffer (tone, 0, tone.getNumSamples());
    }

    juce::AudioFormatManager formats;
    formats.registerBasicFormats();

    pxf::GrainEngine grain;
    grain.prepare (kRate);

    juce::String error;
    check (grain.loadFile (file, formats, error), "loads the wav");
    check (grain.hasSample(), "sample flagged as ready");

    pxf::GrainParams gp;
    gp.sizeMs   = 80.0f;
    gp.density  = 30.0f;
    gp.position = 40.0f;
    gp.spray    = 0.0f;
    gp.shape    = 0.2f;
    grain.setParams (gp);

    juce::AudioBuffer<float> out (2, 24000);
    out.clear();
    grain.noteOn (60, 0.9f);             // C4 = original pitch
    grain.render (out, 0, 24000);

    check (isFinite (out), "grain output is finite");
    check (rms (out, 8000, 12000) > 0.005f, "grain stream is audible");

    const float fAt60 = zcFreq (out, 8000, 12000);
    std::printf ("       C4 grain pitch: %.1f Hz (source 440)\n", fAt60);
    check (std::abs (fAt60 - 440.0f) < 25.0f, "C4 plays the sample at its original pitch");

    grain.noteOff (60);

    juce::AudioBuffer<float> up (2, 24000);
    up.clear();
    grain.noteOn (72, 0.9f);             // one octave up
    grain.render (up, 0, 24000);

    const float fAt72 = zcFreq (up, 8000, 12000);
    std::printf ("       C5 grain pitch: %.1f Hz (expect ~880)\n", fAt72);
    check (std::abs (fAt72 - 880.0f) < 50.0f, "keyboard tracking transposes grains");

    grain.noteOff (72);

    juce::AudioBuffer<float> tail (2, 48000);
    tail.clear();
    grain.render (tail, 0, 48000);
    check (peak (tail, 24000, 24000) < 1.0e-3f, "grains fade out after note off");

    file.deleteFile();
}

//==============================================================================
int main()
{
    std::printf ("PXF engine tests @ %.0f Hz\n", kRate);

    testPolySynthBasics();
    testMonoLegatoSlide();
    testMonoRetrigger();
    testGrainEngine();

    std::printf ("\n%s (%d failure%s)\n",
                 failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED",
                 failures, failures == 1 ? "" : "s");

    return failures == 0 ? 0 : 1;
}
