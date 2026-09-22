/*  PXF · PxfLookAndFeel.cpp
    Copyright (c) 2026 Henok Habtamu.
    Released under the GNU Affero General Public License v3.0 — see LICENSE.
*/

#include "PxfLookAndFeel.h"

namespace pxf
{
    PxfLookAndFeel::PxfLookAndFeel (juce::Colour accentColour)
        : accent (accentColour)
    {
        setColour (juce::ComboBox::backgroundColourId,     theme::knobBody);
        setColour (juce::ComboBox::textColourId,           theme::text);
        setColour (juce::ComboBox::outlineColourId,        accent.withAlpha (0.35f));
        setColour (juce::ComboBox::arrowColourId,          accent);
        setColour (juce::ComboBox::focusedOutlineColourId, accent);

        setColour (juce::PopupMenu::backgroundColourId,            theme::bgTop);
        setColour (juce::PopupMenu::textColourId,                  theme::text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, accent.withAlpha (0.30f));
        setColour (juce::PopupMenu::highlightedTextColourId,       juce::Colours::white);
        setColour (juce::PopupMenu::headerTextColourId,            theme::textMuted);

        setColour (juce::Label::textColourId,               theme::text);
        setColour (juce::TooltipWindow::backgroundColourId, theme::bgTop);
        setColour (juce::TooltipWindow::textColourId,       theme::text);
    }

    juce::Font PxfLookAndFeel::getComboBoxFont (juce::ComboBox&)      { return theme::font (12.5f); }
    juce::Font PxfLookAndFeel::getPopupMenuFont()                     { return theme::font (13.5f); }

    void PxfLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
    {
        label.setBounds (10, 1, box.getWidth() - 24, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
        label.setJustificationType (juce::Justification::centredLeft);
    }

    void PxfLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                       int, int, int, int, juce::ComboBox& box)
    {
        auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
        const float radius = 6.0f;

        g.setColour (theme::knobBody);
        g.fillRoundedRectangle (bounds, radius);

        const bool lit = box.isMouseOver() || box.isPopupActive();

        g.setColour (accent.withAlpha (lit ? 0.75f : 0.30f));
        g.drawRoundedRectangle (bounds, radius, 1.0f);

        // chevron
        juce::Path arrow;
        const float cx = (float) width - 13.0f;
        const float cy = (float) height * 0.5f;

        arrow.startNewSubPath (cx - 3.5f, cy - 1.8f);
        arrow.lineTo (cx, cy + 2.2f);
        arrow.lineTo (cx + 3.5f, cy - 1.8f);

        g.setColour (accent.withAlpha (lit ? 1.0f : 0.7f));
        g.strokePath (arrow, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void PxfLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float startAngle, float endAngle,
                                           juce::Slider& slider)
    {
        const auto area   = juce::Rectangle<int> (x, y, width, height).toFloat();
        const float size  = juce::jmin (area.getWidth(), area.getHeight());
        const auto  box   = juce::Rectangle<float> (size, size).withCentre (area.getCentre());

        const float radius = size * 0.5f;
        const float cx     = box.getCentreX();
        const float cy     = box.getCentreY();

        const float arcRadius   = radius - 3.5f;
        const float bodyRadius  = radius - 10.0f;
        const float angle       = startAngle + sliderPos * (endAngle - startAngle);

        const bool lit = slider.isMouseOverOrDragging() && slider.isEnabled();
        const float alpha = slider.isEnabled() ? 1.0f : 0.35f;

        // ---- ambient bloom ----
        if (sliderPos > 0.001f)
            theme::glowEllipse (g, box.expanded (4.0f), accent, (lit ? 0.55f : 0.30f) * alpha);

        // ---- knob body ----
        juce::ColourGradient body (juce::Colour (0xff1b2148), cx, cy - bodyRadius,
                                   theme::knobBody,           cx, cy + bodyRadius, false);
        g.setGradientFill (body);
        g.fillEllipse (juce::Rectangle<float> (bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre ({ cx, cy }));

        g.setColour (juce::Colours::white.withAlpha (0.06f * alpha));
        g.drawEllipse (juce::Rectangle<float> (bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre ({ cx, cy }), 1.0f);

        // ---- dim track ----
        juce::Path track;
        track.addCentredArc (cx, cy, arcRadius, arcRadius, 0.0f, startAngle, endAngle, true);

        g.setColour (theme::trackDim.withAlpha (0.9f * alpha));
        g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // ---- glowing value arc ----
        if (std::abs (angle - startAngle) > 0.01f)
        {
            juce::Path value;
            value.addCentredArc (cx, cy, arcRadius, arcRadius, 0.0f, startAngle, angle, true);

            theme::glowPath (g, value, accent, 3.0f, (lit ? 1.0f : 0.85f) * alpha);
        }

        // ---- pointer ----
        const float sinA = std::sin (angle);
        const float cosA = std::cos (angle);

        juce::Path pointer;
        pointer.startNewSubPath (cx + sinA * (bodyRadius * 0.30f), cy - cosA * (bodyRadius * 0.30f));
        pointer.lineTo          (cx + sinA * (bodyRadius * 0.86f), cy - cosA * (bodyRadius * 0.86f));

        g.setColour (juce::Colours::white.withAlpha (0.85f * alpha));
        g.strokePath (pointer, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // ---- tip dot on the arc ----
        const float dotSize = 4.0f;
        const juce::Point<float> tip (cx + sinA * arcRadius, cy - cosA * arcRadius);

        theme::glowEllipse (g, juce::Rectangle<float> (14.0f, 14.0f).withCentre (tip), accent, alpha);

        g.setColour (accent.brighter (0.35f).withAlpha (alpha));
        g.fillEllipse (juce::Rectangle<float> (dotSize, dotSize).withCentre (tip));
    }
}
