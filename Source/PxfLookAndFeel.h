/*  PXF · PxfLookAndFeel.h
    Copyright (c) 2026 Henok Habtamu.
    Released under the GNU Affero General Public License v3.0 — see LICENSE.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme.h"

namespace pxf
{
    /** One instance per accent colour: purple for the synth and master, blue for grain. */
    class PxfLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        explicit PxfLookAndFeel (juce::Colour accentColour);

        void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                               float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                               juce::Slider&) override;

        void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                           int buttonX, int buttonY, int buttonW, int buttonH,
                           juce::ComboBox&) override;

        juce::Font getComboBoxFont (juce::ComboBox&) override;
        void positionComboBoxText (juce::ComboBox&, juce::Label&) override;

        juce::Font getPopupMenuFont() override;

        juce::Colour accent;
    };
}
