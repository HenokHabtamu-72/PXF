/*  PXF · UIComponents.cpp
    Copyright (c) 2026 Henok Habtamu.
    Released under the GNU Affero General Public License v3.0 — see LICENSE.
*/

#include "UIComponents.h"

namespace pxf
{
    //==============================================================================
    juce::String formatHz (double v)
    {
        if (v >= 1000.0)
            return juce::String (v / 1000.0, v >= 10000.0 ? 1 : 2) + " kHz";

        return juce::String (juce::roundToInt (v)) + " Hz";
    }

    juce::String formatMs (double seconds)
    {
        if (seconds >= 1.0)
            return juce::String (seconds, 2) + " s";

        return juce::String (juce::roundToInt (seconds * 1000.0)) + " ms";
    }

    juce::String formatPercent (double v01)
    {
        return juce::String (juce::roundToInt (v01 * 100.0)) + "%";
    }

    juce::String formatRaw (double v, int decimals, const juce::String& suffix)
    {
        return juce::String (v, decimals) + suffix;
    }

    //==============================================================================
    CardPanel::CardPanel (juce::String cardTitle, juce::Colour accentColour)
        : title (std::move (cardTitle)), accent (accentColour)
    {
        setInterceptsMouseClicks (false, true);
    }

    juce::Rectangle<int> CardPanel::getContentArea() const
    {
        auto area = getLocalBounds().reduced (12, 10);
        area.removeFromTop (title.isEmpty() ? 0 : 20);
        return area;
    }

    void CardPanel::paint (juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat().reduced (0.5f);
        const float radius = 10.0f;

        g.setColour (theme::cardFill.withAlpha (0.80f));
        g.fillRoundedRectangle (bounds, radius);

        // A hairline of accent light along the top edge.
        juce::ColourGradient edge (accent.withAlpha (0.22f), bounds.getCentreX(), bounds.getY(),
                                   juce::Colours::white.withAlpha (0.04f), bounds.getCentreX(), bounds.getBottom(), false);
        g.setGradientFill (edge);
        g.drawRoundedRectangle (bounds, radius, 1.0f);

        if (title.isNotEmpty())
        {
            auto titleArea = getLocalBounds().reduced (12, 10).removeFromTop (18);

            const auto dot = juce::Rectangle<float> (5.0f, 5.0f)
                                .withCentre ({ (float) titleArea.getX() + 2.0f, (float) titleArea.getCentreY() });

            theme::glowEllipse (g, dot.expanded (5.0f), accent, 0.9f);
            g.setColour (accent);
            g.fillEllipse (dot);

            titleArea.removeFromLeft (13);

            g.setColour (theme::textMuted);
            theme::drawTracked (g, title.toUpperCase(), titleArea, 1.6f,
                                juce::Justification::centredLeft, theme::font (10.0f, true));
        }
    }

    //==============================================================================
    KnobCell::KnobCell (juce::AudioProcessorValueTreeState& state,
                        const juce::String& parameterID,
                        const juce::String& labelText,
                        Formatter formatter)
        : label (labelText), format (std::move (formatter))
    {
        slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                    juce::MathConstants<float>::pi * 2.75f, true);
        slider.setVelocityBasedMode (false);
        slider.onValueChange = [this] { repaint(); };

        addAndMakeVisible (slider);
        slider.addMouseListener (this, false);

        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, parameterID, slider);
    }

    void KnobCell::resized()
    {
        auto area = getLocalBounds();
        area.removeFromBottom (16);                       // room for the label
        const int size = juce::jmin (area.getWidth(), area.getHeight());
        slider.setBounds (area.withSizeKeepingCentre (size, size));
    }

    void KnobCell::paint (juce::Graphics& g)
    {
        auto textArea = getLocalBounds().removeFromBottom (15);

        const bool live = showValue && format != nullptr;

        g.setColour (live ? theme::text : theme::textMuted);

        theme::drawTracked (g,
                            live ? format (slider.getValue()) : label.toUpperCase(),
                            textArea,
                            live ? 0.4f : 1.4f,
                            juce::Justification::centred,
                            theme::font (live ? 11.0f : 9.5f, live));
    }

    void KnobCell::mouseEnter (const juce::MouseEvent&) { showValue = true;  repaint(); }
    void KnobCell::mouseDrag  (const juce::MouseEvent&) { showValue = true;  repaint(); }
    void KnobCell::mouseExit  (const juce::MouseEvent&) { showValue = slider.isMouseButtonDown(); repaint(); }
    void KnobCell::mouseUp    (const juce::MouseEvent&) { showValue = slider.isMouseOver();       repaint(); }

    //==============================================================================
    ChipButton::ChipButton (const juce::String& chipText, juce::Colour accentColour)
        : juce::Button (chipText), accent (accentColour)
    {
        setClickingTogglesState (true);
    }

    void ChipButton::paintButton (juce::Graphics& g, bool isMouseOver, bool)
    {
        auto bounds = getLocalBounds().toFloat().reduced (0.5f);
        const float radius = bounds.getHeight() * 0.5f;

        const bool on = getToggleState();
        const float alpha = isEnabled() ? 1.0f : 0.30f;

        if (on)
        {
            juce::Path pill;
            pill.addRoundedRectangle (bounds, radius);
            theme::glowPath (g, pill, accent, 1.2f, 0.9f * alpha);

            g.setColour (accent.withAlpha (0.22f * alpha));
            g.fillRoundedRectangle (bounds, radius);
        }
        else
        {
            g.setColour (theme::knobBody.withAlpha (0.85f * alpha));
            g.fillRoundedRectangle (bounds, radius);

            g.setColour (juce::Colours::white.withAlpha ((isMouseOver ? 0.20f : 0.10f) * alpha));
            g.drawRoundedRectangle (bounds, radius, 1.0f);
        }

        g.setColour ((on ? juce::Colours::white : theme::textMuted).withAlpha (alpha));

        theme::drawTracked (g, getButtonText().toUpperCase(), getLocalBounds(), 1.8f,
                            juce::Justification::centred, theme::font (10.0f, on));
    }

    //==============================================================================
    TabChip::TabChip (const juce::String& chipText, juce::Colour accentColour)
        : juce::Button (chipText), accent (accentColour)
    {
        setClickingTogglesState (false);
    }

    void TabChip::paintButton (juce::Graphics& g, bool isMouseOver, bool)
    {
        auto bounds = getLocalBounds().toFloat().reduced (0.5f);
        const float radius = 7.0f;

        const bool on = getToggleState();

        if (on)
        {
            juce::Path shape;
            shape.addRoundedRectangle (bounds, radius);
            theme::glowPath (g, shape, accent, 1.3f, 1.0f);

            g.setColour (accent.withAlpha (0.20f));
            g.fillRoundedRectangle (bounds, radius);
        }
        else if (isMouseOver)
        {
            g.setColour (juce::Colours::white.withAlpha (0.06f));
            g.fillRoundedRectangle (bounds, radius);
        }

        g.setColour (on ? juce::Colours::white : theme::textMuted);

        theme::drawTracked (g, getButtonText().toUpperCase(), getLocalBounds(), 2.0f,
                            juce::Justification::centred, theme::font (10.5f, on));
    }

    //==============================================================================
    ArrowChip::ArrowChip (bool pointsRight, juce::Colour accentColour)
        : juce::Button (pointsRight ? "next" : "prev"), right (pointsRight), accent (accentColour)
    {
    }

    void ArrowChip::paintButton (juce::Graphics& g, bool isMouseOver, bool isMouseDown)
    {
        auto bounds = getLocalBounds().toFloat();
        const float cx = bounds.getCentreX();
        const float cy = bounds.getCentreY();
        const float w  = 3.6f;
        const float h  = 5.4f;

        juce::Path arrow;

        if (right)
        {
            arrow.startNewSubPath (cx - w * 0.5f, cy - h);
            arrow.lineTo (cx + w * 0.5f, cy);
            arrow.lineTo (cx - w * 0.5f, cy + h);
        }
        else
        {
            arrow.startNewSubPath (cx + w * 0.5f, cy - h);
            arrow.lineTo (cx - w * 0.5f, cy);
            arrow.lineTo (cx + w * 0.5f, cy + h);
        }

        const float amount = isMouseDown ? 1.0f : (isMouseOver ? 0.85f : 0.5f);

        if (isMouseOver)
            theme::glowPath (g, arrow, accent, 1.8f, amount);
        else
        {
            g.setColour (theme::textMuted);
            g.strokePath (arrow, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    //==============================================================================
    WaveformDisplay::WaveformDisplay()
    {
        setInterceptsMouseClicks (true, false);
    }

    void WaveformDisplay::setPeaks (std::vector<float> mins, std::vector<float> maxs)
    {
        peakMin = std::move (mins);
        peakMax = std::move (maxs);
        repaint();
    }

    void WaveformDisplay::setPosition (float normalised)
    {
        const float clamped = juce::jlimit (0.0f, 1.0f, normalised);

        if (std::abs (clamped - position) > 0.0005f)
        {
            position = clamped;
            repaint();
        }
    }

    void WaveformDisplay::setGrainDots (const std::vector<float>& positions)
    {
        grainDots = positions;
        repaint();
    }

    void WaveformDisplay::setPlaceholder (juce::String message)
    {
        placeholder = std::move (message);
        repaint();
    }

    void WaveformDisplay::positionFromMouse (const juce::MouseEvent& e)
    {
        if (peakMin.empty() || onPositionDragged == nullptr)
            return;

        const float x = juce::jlimit (0.0f, 1.0f, (float) e.position.x / (float) juce::jmax (1, getWidth()));
        onPositionDragged (x);
    }

    void WaveformDisplay::mouseDown (const juce::MouseEvent& e) { positionFromMouse (e); }
    void WaveformDisplay::mouseDrag (const juce::MouseEvent& e) { positionFromMouse (e); }

    bool WaveformDisplay::isInterestedInFileDrag (const juce::StringArray& files)
    {
        for (const auto& f : files)
            if (f.endsWithIgnoreCase (".wav")  || f.endsWithIgnoreCase (".aiff")
             || f.endsWithIgnoreCase (".aif")  || f.endsWithIgnoreCase (".mp3")
             || f.endsWithIgnoreCase (".flac") || f.endsWithIgnoreCase (".ogg"))
                return true;

        return false;
    }

    void WaveformDisplay::fileDragEnter (const juce::StringArray&, int, int) { dragOver = true;  repaint(); }
    void WaveformDisplay::fileDragExit  (const juce::StringArray&)           { dragOver = false; repaint(); }

    void WaveformDisplay::filesDropped (const juce::StringArray& files, int, int)
    {
        dragOver = false;
        repaint();

        if (! files.isEmpty() && onFileDropped != nullptr)
            onFileDropped (juce::File (files[0]));
    }

    void WaveformDisplay::paint (juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat().reduced (0.5f);
        const float radius = 8.0f;

        g.setColour (theme::knobBody.withAlpha (0.75f));
        g.fillRoundedRectangle (bounds, radius);

        const bool empty = peakMin.empty();

        // ---- waveform, purple on the left bleeding to blue on the right ----
        if (! empty)
        {
            const int   n  = (int) peakMin.size();
            const float w  = bounds.getWidth();
            const float cy = bounds.getCentreY();
            const float half = bounds.getHeight() * 0.42f;

            juce::Path wave;
            wave.startNewSubPath (bounds.getX(), cy);

            for (int i = 0; i < n; ++i)
            {
                const float x = bounds.getX() + w * (float) i / (float) (n - 1);
                wave.lineTo (x, cy - peakMax[(size_t) i] * half);
            }

            for (int i = n - 1; i >= 0; --i)
            {
                const float x = bounds.getX() + w * (float) i / (float) (n - 1);
                wave.lineTo (x, cy - peakMin[(size_t) i] * half);
            }

            wave.closeSubPath();

            juce::ColourGradient grad (theme::purple.withAlpha (0.75f), bounds.getX(),     cy,
                                       theme::blue  .withAlpha (0.75f), bounds.getRight(), cy, false);
            g.setGradientFill (grad);
            g.fillPath (wave);

            // faint bloom over the whole shape
            g.setColour (theme::blue.withAlpha (0.10f));
            g.strokePath (wave, juce::PathStrokeType (2.5f));

            // ---- live grain read-heads ----
            for (const auto dot : grainDots)
            {
                const float x = bounds.getX() + w * juce::jlimit (0.0f, 1.0f, dot);
                const auto  spot = juce::Rectangle<float> (7.0f, 7.0f).withCentre ({ x, cy });

                theme::glowEllipse (g, spot.expanded (5.0f), theme::blue, 0.8f);
                g.setColour (theme::blue.brighter (0.4f).withAlpha (0.65f));
                g.fillEllipse (spot.reduced (2.0f));
            }

            // ---- position marker ----
            const float px = bounds.getX() + w * position;

            juce::Path marker;
            marker.startNewSubPath (px, bounds.getY() + 4.0f);
            marker.lineTo (px, bounds.getBottom() - 4.0f);

            theme::glowPath (g, marker, theme::purple, 1.6f, 1.0f);

            const auto head = juce::Rectangle<float> (7.0f, 7.0f).withCentre ({ px, bounds.getY() + 5.0f });
            theme::glowEllipse (g, head.expanded (6.0f), theme::purple, 1.0f);
            g.setColour (juce::Colours::white);
            g.fillEllipse (head.reduced (1.5f));
        }
        else
        {
            g.setColour (theme::textMuted);
            theme::drawTracked (g, placeholder, getLocalBounds(), 2.4f,
                                juce::Justification::centred, theme::font (11.0f, true));
        }

        // ---- border / drop highlight ----
        if (dragOver)
        {
            juce::Path outline;
            outline.addRoundedRectangle (bounds, radius);
            theme::glowPath (g, outline, theme::blue, 1.5f, 1.0f);
        }
        else
        {
            g.setColour (theme::blue.withAlpha (empty ? 0.28f : 0.18f));
            g.drawRoundedRectangle (bounds, radius, 1.0f);
        }
    }
}
