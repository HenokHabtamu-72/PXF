/*  PXF · PluginProcessor.cpp
    Copyright (c) 2026 Henok Habtamu.
    Released under the GNU Affero General Public License v3.0 — see LICENSE.
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace pxf
{
    PxfAudioProcessor::PxfAudioProcessor()
        : juce::AudioProcessor (BusesProperties()
              .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
          apvts (*this, nullptr, "PXF", createParameterLayout())
    {
        formatManager.registerBasicFormats();

       #if JUCE_USE_MP3AUDIOFORMAT
        formatManager.registerFormat (new juce::MP3AudioFormat(), false);
       #endif

        // A fresh instance opens on preset 0 ("Midnight Sub"), exactly as the
        // header says. Hosts restoring a session overwrite this via setState.
        loadPreset (0);
    }

    std::atomic<float>* PxfAudioProcessor::raw (const char* id) const
    {
        return apvts.getRawParameterValue (id);
    }

    //==============================================================================
    void PxfAudioProcessor::prepareToPlay (double sampleRate, int)
    {
        synth.prepare (sampleRate);
        grain.prepare (sampleRate);
        fx.prepare (sampleRate);

        setLatencySamples (fx.getLatencySamples());

        lastTab = activeTab.load();
    }

    bool PxfAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
    {
        return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
    }

    //==============================================================================
    void PxfAudioProcessor::pullParameters()
    {
        SynthParams sp;
        sp.osc1         = waveFromIndex ((int) raw (pid::osc1Wave)->load());
        sp.osc2         = waveFromIndex ((int) raw (pid::osc2Wave)->load());
        sp.oscMix       = juce::jlimit (0.0f, 1.0f, raw (pid::oscMix)->load());
        sp.detuneCents  = raw (pid::detune)->load();
        sp.subLevel     = raw (pid::subLevel)->load();
        sp.cutoffHz     = raw (pid::cutoff)->load();
        sp.resoQ        = resoToQ (raw (pid::reso)->load());
        sp.attack       = raw (pid::attack)->load();
        sp.decay        = raw (pid::decay)->load();
        sp.sustain      = raw (pid::sustain)->load();
        sp.release      = raw (pid::release)->load();
        sp.glideSeconds = raw (pid::glide)->load() * 0.001f;
        sp.mono         = raw (pid::mono)->load()   > 0.5f;
        sp.legato       = raw (pid::legato)->load() > 0.5f;
        synth.setParams (sp);

        GrainParams gp;
        gp.sizeMs     = raw (pid::grainSize)->load();
        gp.density    = raw (pid::grainDensity)->load();
        gp.position   = raw (pid::grainPosition)->load();
        gp.spray      = raw (pid::grainSpray)->load();
        gp.pitchSemis = raw (pid::grainPitch)->load();
        gp.shape      = raw (pid::grainShape)->load();
        grain.setParams (gp);

        FxParams fp;
        fp.revSize  = raw (pid::revSize)->load();
        fp.revMix   = raw (pid::revMix)->load();
        fp.delayDiv = (int) raw (pid::delayDiv)->load();
        fp.delayFb  = raw (pid::delayFb)->load();
        fp.delayMix = raw (pid::delayMix)->load();
        fp.dust     = raw (pid::dust)->load();
        fp.volumeDb = raw (pid::masterVol)->load();
        fx.setParams (fp);
    }

    void PxfAudioProcessor::renderEngine (juce::AudioBuffer<float>& buffer, int start, int num, int tab)
    {
        if (num <= 0)
            return;

        if (tab == synthTab)
            synth.render (buffer, start, num);
        else
            grain.render (buffer, start, num);
    }

    void PxfAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
    {
        juce::ScopedNoDenormals noDenormals;

        const int numSamples = buffer.getNumSamples();
        buffer.clear();

        pullParameters();

        // Only the active tab's engine is audible; the other one lets go of its notes.
        const int tab = activeTab.load();

        if (tab != lastTab)
        {
            synth.allNotesOff();
            grain.allNotesOff();
            lastTab = tab;
        }

        double bpm = 120.0;

        if (auto* head = getPlayHead())
            if (auto position = head->getPosition())
                if (auto hostBpm = position->getBpm())
                    bpm = *hostBpm;

        // ---- sample-accurate MIDI ----
        int cursor = 0;

        for (const auto metadata : midi)
        {
            const int timestamp = juce::jlimit (0, numSamples, metadata.samplePosition);

            if (timestamp > cursor)
            {
                renderEngine (buffer, cursor, timestamp - cursor, tab);
                cursor = timestamp;
            }

            const auto message = metadata.getMessage();

            if (message.isNoteOn())
            {
                if (tab == synthTab) synth.noteOn (message.getNoteNumber(), message.getFloatVelocity());
                else                 grain.noteOn (message.getNoteNumber(), message.getFloatVelocity());
            }
            else if (message.isNoteOff())
            {
                if (tab == synthTab) synth.noteOff (message.getNoteNumber());
                else                 grain.noteOff (message.getNoteNumber());
            }
            else if (message.isAllNotesOff() || message.isAllSoundOff())
            {
                synth.allNotesOff();
                grain.allNotesOff();
            }
        }

        renderEngine (buffer, cursor, numSamples - cursor, tab);

        const bool engineActive = (tab == synthTab) ? synth.isActive() : grain.isActive();

        fx.process (buffer, bpm, engineActive);
    }

    //==============================================================================
    void PxfAudioProcessor::setActiveTab (int tab)
    {
        activeTab.store (juce::jlimit (0, 1, tab));
    }

    void PxfAudioProcessor::loadPreset (int index)
    {
        index = juce::jlimit (0, kNumPresets - 1, index);
        const auto& p = presets[index];

        auto set = [this] (const char* id, float value)
        {
            if (auto* param = apvts.getParameter (id))
                param->setValueNotifyingHost (param->convertTo0to1 (value));
        };

        set (pid::osc1Wave, (float) p.osc1);
        set (pid::osc2Wave, (float) p.osc2);
        set (pid::oscMix,   p.oscMix);
        set (pid::detune,   p.detune);
        set (pid::subLevel, p.sub);
        set (pid::cutoff,   p.cutoff);
        set (pid::reso,     p.reso);
        set (pid::attack,   p.attack);
        set (pid::decay,    p.decay);
        set (pid::sustain,  p.sustain);
        set (pid::release,  p.release);
        set (pid::glide,    p.glideMs);
        set (pid::mono,     p.mono   ? 1.0f : 0.0f);
        set (pid::legato,   p.legato ? 1.0f : 0.0f);

        set (pid::revSize,   p.revSize);
        set (pid::revMix,    p.revMix);
        set (pid::delayDiv,  (float) p.delayDiv);
        set (pid::delayFb,   p.delayFb);
        set (pid::delayMix,  p.delayMix);
        set (pid::dust,      p.dust);
        set (pid::masterVol, p.volumeDb);

        presetIndex.store (index);
    }

    void PxfAudioProcessor::stepPreset (int delta)
    {
        const int next = (presetIndex.load() + delta + kNumPresets) % kNumPresets;
        loadPreset (next);
    }

    bool PxfAudioProcessor::loadSample (const juce::File& file, juce::String& errorOut)
    {
        return grain.loadFile (file, formatManager, errorOut);
    }

    //==============================================================================
    void PxfAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
    {
        auto state = apvts.copyState();

        state.setProperty ("presetIndex", presetIndex.load(), nullptr);
        state.setProperty ("activeTab",   activeTab.load(),   nullptr);
        state.setProperty ("samplePath",  grain.getFile().getFullPathName(), nullptr);

        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
    }

    void PxfAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
    {
        auto xml = getXmlFromBinary (data, sizeInBytes);

        if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
            return;

        const auto tree = juce::ValueTree::fromXml (*xml);
        apvts.replaceState (tree);

        presetIndex.store (juce::jlimit (0, kNumPresets - 1, (int) tree.getProperty ("presetIndex", 0)));
        activeTab  .store (juce::jlimit (0, 1,               (int) tree.getProperty ("activeTab",   0)));

        const juce::String path = tree.getProperty ("samplePath", juce::String());

        if (path.isNotEmpty())
        {
            const juce::File file (path);

            if (file.existsAsFile())
            {
                juce::String error;
                grain.loadFile (file, formatManager, error);
            }
        }
    }

    juce::AudioProcessorEditor* PxfAudioProcessor::createEditor()
    {
        return new PxfAudioProcessorEditor (*this);
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new pxf::PxfAudioProcessor();
}
