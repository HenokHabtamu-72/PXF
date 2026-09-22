/*  PXF · UIComponents.h
    Copyright (c) 2026 Henok Habtamu.
    Released under the GNU Affero General Public License v3.0 — see LICENSE.
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Theme.h"

namespace pxf
{
    //==============================================================================
    /** A titled panel. Everything on the front sits inside one of these. */
    class CardPanel : public juce::Component
    {
    public:
        CardPanel (juce::String cardTitle, juce::Colour accentColour);

        void paint (juce::Graphics&) override;

        /** Content area, i.e. the card minus its padding and title row. */
        juce::Rectangle<int> getContentArea() const;

        juce::String title;
        juce::Colour accent;
    };

    //==============================================================================
    /** Knob + micro-label. The label swaps to a live value readout on hover or drag. */
    class KnobCell : public juce::Component
    {
    public:
        using Formatter = std::function<juce::String (double)>;

        KnobCell (juce::AudioProcessorValueTreeState& state,
                  const juce::String& parameterID,
                  const juce::String& labelText,
                  Formatter formatter);

        void paint (juce::Graphics&) override;
        void resized() override;

        void mouseEnter (const juce::MouseEvent&) override;
        void mouseExit  (const juce::MouseEvent&) override;
        void mouseDrag  (const juce::MouseEvent&) override;
        void mouseUp    (const juce::MouseEvent&) override;

        juce::Slider slider;

    private:
        juce::String label;
        Formatter format;
        bool showValue = false;

        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KnobCell)
    };

    //==============================================================================
    /** Small pill toggle — MONO, LEGATO. */
    class ChipButton : public juce::Button
    {
    public:
        ChipButton (const juce::String& text, juce::Colour accentColour);

        void paintButton (juce::Graphics&, bool isMouseOver, bool isMouseDown) override;

        juce::Colour accent;
    };

    //==============================================================================
    /** SYNTH / GRAIN switch in the header. */
    class TabChip : public juce::Button
    {
    public:
        TabChip (const juce::String& text, juce::Colour accentColour);

        void paintButton (juce::Graphics&, bool isMouseOver, bool isMouseDown) override;

        juce::Colour accent;
    };

    //==============================================================================
    /** Preset browser arrow. */
    class ArrowChip : public juce::Button
    {
    public:
        ArrowChip (bool pointsRight, juce::Colour accentColour);

        void paintButton (juce::Graphics&, bool isMouseOver, bool isMouseDown) override;

    private:
        bool right;
        juce::Colour accent;
    };

    //==============================================================================
    /** Waveform + glowing position marker. Click or drag to set Position;
        drop an audio file anywhere on it to load. */
    class WaveformDisplay : public juce::Component,
                            public juce::FileDragAndDropTarget
    {
    public:
        WaveformDisplay();

        void paint (juce::Graphics&) override;

        void setPeaks (std::vector<float> mins, std::vector<float> maxs);
        void setPosition (float normalised);
        void setGrainDots (const std::vector<float>& normalisedPositions);
        void setPlaceholder (juce::String message);

        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;

        bool isInterestedInFileDrag (const juce::StringArray& files) override;
        void fileDragEnter (const juce::StringArray&, int, int) override;
        void fileDragExit  (const juce::StringArray&) override;
        void filesDropped  (const juce::StringArray& files, int, int) override;

        std::function<void (const juce::File&)> onFileDropped;
        std::function<void (float)> onPositionDragged;

    private:
        void positionFromMouse (const juce::MouseEvent&);

        std::vector<float> peakMin, peakMax;
        std::vector<float> grainDots;
        float position = 0.0f;
        bool  dragOver = false;
        juce::String placeholder { "DROP AN AUDIO FILE" };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformDisplay)
    };

    //==============================================================================
    juce::String formatHz (double v);
    juce::String formatMs (double seconds);
    juce::String formatPercent (double v01);
    juce::String formatRaw (double v, int decimals, const juce::String& suffix);
}
