/*  PXF · PluginProcessor.h
    Copyright (c) 2026 Henok Habtamu.
    Released under the GNU Affero General Public License v3.0 — see LICENSE.

*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>

#include "Parameters.h"
#include "Presets.h"
#include "SynthEngine.h"
#include "GrainEngine.h"
#include "FxChain.h"

namespace pxf
{
    enum Tab { synthTab = 0, grainTab = 1 };

    class PxfAudioProcessor : public juce::AudioProcessor
    {
    public:
        PxfAudioProcessor();
        ~PxfAudioProcessor() override = default;

        void prepareToPlay (double sampleRate, int samplesPerBlock) override;
        void releaseResources() override {}
        bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

        juce::AudioProcessorEditor* createEditor() override;
        bool hasEditor() const override                  { return true; }

        const juce::String getName() const override      { return "PXF"; }
        bool acceptsMidi() const override                { return true; }
        bool producesMidi() const override               { return false; }
        bool isMidiEffect() const override               { return false; }
        double getTailLengthSeconds() const override     { return 4.0; }

        int getNumPrograms() override                    { return 1; }
        int getCurrentProgram() override                 { return 0; }
        void setCurrentProgram (int) override            {}
        const juce::String getProgramName (int) override { return "PXF"; }
        void changeProgramName (int, const juce::String&) override {}

        void getStateInformation (juce::MemoryBlock&) override;
        void setStateInformation (const void*, int) override;

        // ---- preset browser ----
        int  getPresetIndex() const noexcept { return presetIndex.load(); }
        void loadPreset (int index);
        void stepPreset (int delta);

        // ---- tabs ----
        int  getActiveTab() const noexcept { return activeTab.load(); }
        void setActiveTab (int tab);

        // ---- grain sample (message thread) ----
        bool loadSample (const juce::File& file, juce::String& errorOut);

        juce::AudioProcessorValueTreeState apvts;
        GrainEngine grain;

    private:
        void renderEngine (juce::AudioBuffer<float>& buffer, int start, int num, int tab);
        void pullParameters();

        std::atomic<float>* raw (const char* id) const;

        juce::AudioFormatManager formatManager;

        SynthEngine synth;
        FxChain     fx;

        std::atomic<int> presetIndex { 0 };
        std::atomic<int> activeTab   { synthTab };
        int lastTab = synthTab;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PxfAudioProcessor)
    };
}
