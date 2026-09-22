/*  PXF · Theme.h
    Copyright (c) 2026 Henok Habtamu.
    Released under the GNU Affero General Public License v3.0 — see LICENSE.

    Late Night Chill: neon purple on near-black navy. 2 a.m. studio, dim room,
    a neon sign in the window.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace pxf::theme
{
    // ---- palette (from the spec, verbatim) ----
    const juce::Colour bgTop       { 0xff0a0e27 };
    const juce::Colour bgBottom    { 0xff131a3d };
    const juce::Colour purple      { 0xffb04cf7 };
    const juce::Colour purpleDeep  { 0xff8b5cf6 };
    const juce::Colour blue        { 0xff3d5afe };
    const juce::Colour text        { 0xffc9c4e8 };
    const juce::Colour textMuted   { 0xff6e6a8f };

    // ---- derived surfaces ----
    const juce::Colour cardFill    { 0xff0d1230 };
    const juce::Colour knobBody    { 0xff080b1f };
    const juce::Colour trackDim    { 0xff262b52 };

    //==============================================================================
    /** Clean geometric sans, whatever the machine actually has. */
    inline juce::String familyName()
    {
        static const juce::String chosen = []
        {
            const juce::StringArray preferred
            {
                "Inter", "Inter Tight", "Montserrat", "Poppins",
                "Segoe UI Variable Display", "Segoe UI",
                "Avenir Next", "Helvetica Neue", "DejaVu Sans", "Arial"
            };

            const auto available = juce::Font::findAllTypefaceNames();

            for (const auto& name : preferred)
                if (available.contains (name))
                    return name;

            return juce::Font::getDefaultSansSerifFontName();
        }();

        return chosen;
    }

    inline juce::Font font (float height, bool bold = false)
    {
        const int style = bold ? juce::Font::bold : juce::Font::plain;

       #if JUCE_MAJOR_VERSION >= 8
        return juce::Font (juce::FontOptions (familyName(), height, style));
       #else
        return juce::Font (familyName(), height, style);
       #endif
    }

    //==============================================================================
    /** Uppercase micro-labels with wide letter-spacing — the house style. */
    inline void drawTracked (juce::Graphics& g,
                             const juce::String& label,
                             juce::Rectangle<int> area,
                             float tracking,
                             juce::Justification justification,
                             const juce::Font& f)
    {
        if (label.isEmpty())
            return;

        g.setFont (f);

        auto measure = [&f] (const juce::String& s)
        {
            juce::GlyphArrangement glyphs;
            glyphs.addLineOfText (f, s, 0.0f, 0.0f);
            return glyphs.getBoundingBox (0, -1, true).getWidth();
        };

        juce::Array<float> widths;
        float total = 0.0f;

        for (auto character : label)
        {
            const float w = measure (juce::String::charToString (character));
            widths.add (w);
            total += w + tracking;
        }

        if (! widths.isEmpty())
            total -= tracking;

        float x = (float) area.getX();

        if (justification.testFlags (juce::Justification::horizontallyCentred))
            x = (float) area.getCentreX() - total * 0.5f;
        else if (justification.testFlags (juce::Justification::right))
            x = (float) area.getRight() - total;

        const float baseline = (float) area.getCentreY() + (f.getAscent() - f.getDescent()) * 0.5f;

        int i = 0;

        for (auto character : label)
        {
            g.drawSingleLineText (juce::String::charToString (character),
                                  juce::roundToInt (x),
                                  juce::roundToInt (baseline));
            x += widths[i++] + tracking;
        }
    }

    /** Layered strokes: cheaper than a real blur, and it reads as neon. */
    inline void glowPath (juce::Graphics& g, const juce::Path& path, juce::Colour colour,
                          float width, float amount = 1.0f)
    {
        g.setColour (colour.withAlpha (0.10f * amount));
        g.strokePath (path, juce::PathStrokeType (width + 7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour (colour.withAlpha (0.18f * amount));
        g.strokePath (path, juce::PathStrokeType (width + 3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour (colour.withAlpha (juce::jlimit (0.0f, 1.0f, amount)));
        g.strokePath (path, juce::PathStrokeType (width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    /** A soft neon bloom behind an element. */
    inline void glowEllipse (juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour colour, float amount = 1.0f)
    {
        juce::ColourGradient grad (colour.withAlpha (0.28f * amount), bounds.getCentreX(), bounds.getCentreY(),
                                   colour.withAlpha (0.0f),          bounds.getCentreX(), bounds.getY(), true);
        g.setGradientFill (grad);
        g.fillEllipse (bounds);
    }
}
