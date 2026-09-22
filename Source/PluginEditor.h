/*  PXF · PluginEditor.h
    Copyright (c) 2026 Henok Habtamu.
    Released under the GNU Affero General Public License v3.0 — see LICENSE.
*/

#pragma once

#include "PluginProcessor.h"
#include "PxfLookAndFeel.h"
#include "UIComponents.h"

namespace pxf
{
    class PxfAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
    {
    public:
        explicit PxfAudioProcessorEditor (PxfAudioProcessor&);
        ~PxfAudioProcessorEditor() override;

        void paint (juce::Graphics&) override;
        void resized() override;
        void mouseDown (const juce::MouseEvent&) override;

    private:
        void timerCallback() override;

        void buildSynthPanel();
        void buildGrainPanel();
        void buildFxStrip();

        void showTab (int tab);
        void showPresetMenu();
        void refreshPresetLabel();
        void refreshSample();
        void chooseFile();
        void loadFile (const juce::File&);
        void syncMonoLegato();

        void paintHeader (juce::Graphics&);
        juce::Image makeGrainImage() const;

        // Not just 'processor': AudioProcessorEditor already has a member by that name.
        PxfAudioProcessor& pxfProcessor;

        // Declared first so they outlive every component that borrows them.
        PxfLookAndFeel purpleLnf { theme::purple };
        PxfLookAndFeel blueLnf   { theme::blue };

        juce::Image filmGrain;

        // ---- header ----
        TabChip   synthTabButton { "Synth", theme::purple };
        TabChip   grainTabButton { "Grain", theme::blue };
        ArrowChip prevPreset { false, theme::purple };
        ArrowChip nextPreset { true,  theme::purple };
        juce::Rectangle<int> presetNameArea;

        // ---- panels ----
        juce::Component synthPanel, grainPanel, fxStrip;

        CardPanel oscCard    { "Oscillators", theme::purple };
        CardPanel filterCard { "Filter",      theme::purple };
        CardPanel ampCard    { "Amp Envelope",theme::purple };
        CardPanel voiceCard  { "Voice",       theme::purple };

        CardPanel grainCard  { "Grain",  theme::blue };
        CardPanel sampleCard { "Sample", theme::blue };

        CardPanel reverbCard { "Reverb", theme::purple };
        CardPanel delayCard  { "Delay",  theme::purple };
        CardPanel dustCard   { "Dust",   theme::purple };
        CardPanel masterCard { "Master", theme::purple };

        // ---- synth controls ----
        juce::ComboBox osc1Box, osc2Box;
        juce::Label    osc1Label { {}, "1" }, osc2Label { {}, "2" };

        std::unique_ptr<KnobCell> mixKnob, detuneKnob, subKnob;
        std::unique_ptr<KnobCell> cutoffKnob, resoKnob;
        std::unique_ptr<KnobCell> attackKnob, decayKnob, sustainKnob, releaseKnob;
        std::unique_ptr<KnobCell> glideKnob;

        ChipButton monoButton   { "Mono",   theme::purple };
        ChipButton legatoButton { "Legato", theme::purple };

        // ---- grain controls ----
        juce::TextButton loadButton { "LOAD" };
        WaveformDisplay  waveform;
        juce::Label      fileLabel;

        std::unique_ptr<KnobCell> sizeKnob, densityKnob, positionKnob, sprayKnob, pitchKnob, shapeKnob;

        // ---- fx controls ----
        std::unique_ptr<KnobCell> revSizeKnob, revMixKnob, delayFbKnob, delayMixKnob, dustKnob, volumeKnob;
        juce::ComboBox delayDivBox;

        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> osc1Attach, osc2Attach, delayDivAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   monoAttach, legatoAttach;

        std::unique_ptr<juce::FileChooser> chooser;

        int lastSampleVersion = -1;
        int shownPresetIndex  = -1;   // per-editor, so two instances never fight over it

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PxfAudioProcessorEditor)
    };
}
