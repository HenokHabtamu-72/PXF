/*  PXF · PluginEditor.cpp
    Copyright (c) 2026 Henok Habtamu.
    Released under the GNU Affero General Public License v3.0 — see LICENSE.
*/

#include "PluginEditor.h"

namespace pxf
{
    namespace
    {
        constexpr int kWidth   = 920;
        constexpr int kHeight  = 580;
        constexpr int kHeaderH = 66;
        constexpr int kFxH     = 142;
        constexpr int kMargin  = 14;
        constexpr int kGap     = 10;
    }

    //==============================================================================
    PxfAudioProcessorEditor::PxfAudioProcessorEditor (PxfAudioProcessor& p)
        : juce::AudioProcessorEditor (&p), pxfProcessor (p)
    {
        setLookAndFeel (&purpleLnf);

        filmGrain = makeGrainImage();

        // ---- header ----
        addAndMakeVisible (synthTabButton);
        addAndMakeVisible (grainTabButton);
        addAndMakeVisible (prevPreset);
        addAndMakeVisible (nextPreset);

        synthTabButton.onClick = [this] { showTab (synthTab); };
        grainTabButton.onClick = [this] { showTab (grainTab); };

        prevPreset.onClick = [this] { pxfProcessor.stepPreset (-1); refreshPresetLabel(); };
        nextPreset.onClick = [this] { pxfProcessor.stepPreset ( 1); refreshPresetLabel(); };

        addAndMakeVisible (synthPanel);
        addAndMakeVisible (grainPanel);
        addAndMakeVisible (fxStrip);

        grainPanel.setLookAndFeel (&blueLnf);

        buildSynthPanel();
        buildGrainPanel();
        buildFxStrip();

        showTab (pxfProcessor.getActiveTab());
        refreshSample();
        syncMonoLegato();

        setSize (kWidth, kHeight);
        setResizable (false, false);

        startTimerHz (30);
    }

    PxfAudioProcessorEditor::~PxfAudioProcessorEditor()
    {
        stopTimer();
        grainPanel.setLookAndFeel (nullptr);
        setLookAndFeel (nullptr);
    }

    //==============================================================================
    juce::Image PxfAudioProcessorEditor::makeGrainImage() const
    {
        constexpr int size = 128;

        juce::Image image (juce::Image::ARGB, size, size, true);
        juce::Random rng (0x9E3779B9);

        juce::Image::BitmapData pixels (image, juce::Image::BitmapData::writeOnly);

        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
            {
                const auto v = (juce::uint8) rng.nextInt (256);
                pixels.setPixelColour (x, y, juce::Colour (v, v, v).withAlpha (0.035f));
            }

        return image;
    }

    void PxfAudioProcessorEditor::paint (juce::Graphics& g)
    {
        // ---- vertical navy gradient ----
        g.setGradientFill (juce::ColourGradient::vertical (theme::bgTop, 0.0f,
                                                           theme::bgBottom, (float) getHeight()));
        g.fillAll();

        // ---- neon bloom: purple top-left, blue bottom-right ----
        {
            juce::ColourGradient bloom (theme::purpleDeep.withAlpha (0.16f), 120.0f, 20.0f,
                                        theme::purpleDeep.withAlpha (0.0f),  120.0f, 360.0f, true);
            g.setGradientFill (bloom);
            g.fillRect (getLocalBounds());
        }

        {
            juce::ColourGradient bloom (theme::blue.withAlpha (0.13f), (float) getWidth() - 90.0f, (float) getHeight() - 30.0f,
                                        theme::blue.withAlpha (0.0f),  (float) getWidth() - 90.0f, (float) getHeight() - 380.0f, true);
            g.setGradientFill (bloom);
            g.fillRect (getLocalBounds());
        }

        // ---- film grain ----
        g.setTiledImageFill (filmGrain, 0, 0, 1.0f);
        g.fillRect (getLocalBounds());

        paintHeader (g);
    }

    void PxfAudioProcessorEditor::paintHeader (juce::Graphics& g)
    {
        auto header = getLocalBounds().removeFromTop (kHeaderH);

        // hairline under the header
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.fillRect (header.getX() + kMargin, header.getBottom() - 1, header.getWidth() - kMargin * 2, 1);

        // ---- wordmark ----
        auto mark = header.reduced (kMargin, 0).removeFromLeft (150);

        const auto bloom = juce::Rectangle<float> (110.0f, 60.0f)
                              .withCentre ({ (float) mark.getX() + 34.0f, (float) mark.getCentreY() });
        theme::glowEllipse (g, bloom, theme::purple, 0.85f);

        g.setColour (theme::purple.brighter (0.30f));
        theme::drawTracked (g, "PXF", mark, 5.0f, juce::Justification::centredLeft, theme::font (26.0f, true));

        // ---- preset browser ----
        const auto& preset = presets[pxfProcessor.getPresetIndex()];

        auto category = presetNameArea.withHeight (14).withY (presetNameArea.getY() + 8);
        auto name     = presetNameArea.withTrimmedTop (20);

        g.setColour (theme::textMuted);
        theme::drawTracked (g, juce::String (preset.category), category, 2.2f,
                            juce::Justification::centred, theme::font (9.0f, true));

        g.setColour (theme::text);
        theme::drawTracked (g, juce::String (preset.name), name, 0.6f,
                            juce::Justification::centred, theme::font (15.0f));
    }

    //==============================================================================
    void PxfAudioProcessorEditor::buildSynthPanel()
    {
        synthPanel.addAndMakeVisible (oscCard);
        synthPanel.addAndMakeVisible (filterCard);
        synthPanel.addAndMakeVisible (ampCard);
        synthPanel.addAndMakeVisible (voiceCard);

        auto& state = pxfProcessor.apvts;

        // ---- oscillators ----
        osc1Box.addItemList (waveNames(), 1);
        osc2Box.addItemList (waveNames(), 1);
        synthPanel.addAndMakeVisible (osc1Box);
        synthPanel.addAndMakeVisible (osc2Box);

        osc1Attach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (state, pid::osc1Wave, osc1Box);
        osc2Attach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (state, pid::osc2Wave, osc2Box);

        for (auto* label : { &osc1Label, &osc2Label })
        {
            label->setFont (theme::font (9.5f, true));
            label->setColour (juce::Label::textColourId, theme::textMuted);
            label->setJustificationType (juce::Justification::centredRight);
            synthPanel.addAndMakeVisible (*label);
        }

        osc1Label.setText ("OSC 1", juce::dontSendNotification);
        osc2Label.setText ("OSC 2", juce::dontSendNotification);

        mixKnob    = std::make_unique<KnobCell> (state, pid::oscMix,   "Mix",
                                                 [] (double v) { return juce::String (juce::roundToInt ((1.0 - v) * 100.0)) + ":"
                                                                      + juce::String (juce::roundToInt (v * 100.0)); });
        detuneKnob = std::make_unique<KnobCell> (state, pid::detune,   "Detune",
                                                 [] (double v) { return formatRaw (v, 1, " ct"); });
        subKnob    = std::make_unique<KnobCell> (state, pid::subLevel, "Sub",
                                                 [] (double v) { return formatPercent (v); });

        for (auto* k : { mixKnob.get(), detuneKnob.get(), subKnob.get() })
            synthPanel.addAndMakeVisible (*k);

        // ---- filter ----
        cutoffKnob = std::make_unique<KnobCell> (state, pid::cutoff, "Cutoff", [] (double v) { return formatHz (v); });
        resoKnob   = std::make_unique<KnobCell> (state, pid::reso,   "Reso",   [] (double v) { return formatPercent (v); });

        synthPanel.addAndMakeVisible (*cutoffKnob);
        synthPanel.addAndMakeVisible (*resoKnob);

        // ---- amp envelope ----
        attackKnob  = std::make_unique<KnobCell> (state, pid::attack,  "Attack",  [] (double v) { return formatMs (v); });
        decayKnob   = std::make_unique<KnobCell> (state, pid::decay,   "Decay",   [] (double v) { return formatMs (v); });
        sustainKnob = std::make_unique<KnobCell> (state, pid::sustain, "Sustain", [] (double v) { return formatPercent (v); });
        releaseKnob = std::make_unique<KnobCell> (state, pid::release, "Release", [] (double v) { return formatMs (v); });

        for (auto* k : { attackKnob.get(), decayKnob.get(), sustainKnob.get(), releaseKnob.get() })
            synthPanel.addAndMakeVisible (*k);

        // ---- voice: glide + mono + legato ----
        glideKnob = std::make_unique<KnobCell> (state, pid::glide, "Glide",
                                                [] (double v) { return juce::String (juce::roundToInt (v)) + " ms"; });
        synthPanel.addAndMakeVisible (*glideKnob);

        synthPanel.addAndMakeVisible (monoButton);
        synthPanel.addAndMakeVisible (legatoButton);

        monoAttach   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (state, pid::mono,   monoButton);
        legatoAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (state, pid::legato, legatoButton);

        monoButton.onStateChange = [this] { syncMonoLegato(); };

        monoButton  .setTooltip ("One voice, last-note priority. Overlapping notes slide over the Glide time.");
        legatoButton.setTooltip ("Overlapping notes slide without retriggering the envelope.");
        glideKnob->slider.setTooltip ("Time taken to slide between notes.");
    }

    void PxfAudioProcessorEditor::syncMonoLegato()
    {
        // Legato is a mono behaviour: it shapes the envelope when notes overlap.
        legatoButton.setEnabled (monoButton.getToggleState());
        legatoButton.repaint();
    }

    void PxfAudioProcessorEditor::buildGrainPanel()
    {
        grainPanel.addAndMakeVisible (sampleCard);
        grainPanel.addAndMakeVisible (grainCard);

        auto& state = pxfProcessor.apvts;

        loadButton.setButtonText ("LOAD");
        loadButton.setColour (juce::TextButton::buttonColourId,   theme::knobBody);
        loadButton.setColour (juce::TextButton::buttonOnColourId, theme::blue);
        loadButton.setColour (juce::TextButton::textColourOffId,  theme::text);
        loadButton.onClick = [this] { chooseFile(); };
        grainPanel.addAndMakeVisible (loadButton);

        fileLabel.setFont (theme::font (11.0f));
        fileLabel.setColour (juce::Label::textColourId, theme::textMuted);
        fileLabel.setJustificationType (juce::Justification::centredLeft);
        grainPanel.addAndMakeVisible (fileLabel);

        waveform.onFileDropped = [this] (const juce::File& f) { loadFile (f); };

        waveform.onPositionDragged = [this] (float x)
        {
            if (auto* param = pxfProcessor.apvts.getParameter (pid::grainPosition))
                param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, x));
        };

        grainPanel.addAndMakeVisible (waveform);

        sizeKnob     = std::make_unique<KnobCell> (state, pid::grainSize,     "Size",
                                                   [] (double v) { return juce::String (juce::roundToInt (v)) + " ms"; });
        densityKnob  = std::make_unique<KnobCell> (state, pid::grainDensity,  "Density",
                                                   [] (double v) { return juce::String (juce::roundToInt (v)) + "/s"; });
        positionKnob = std::make_unique<KnobCell> (state, pid::grainPosition, "Position",
                                                   [] (double v) { return juce::String (juce::roundToInt (v)) + "%"; });
        sprayKnob    = std::make_unique<KnobCell> (state, pid::grainSpray,    "Spray",
                                                   [] (double v) { return juce::String (juce::roundToInt (v)) + "%"; });
        pitchKnob    = std::make_unique<KnobCell> (state, pid::grainPitch,    "Pitch",
                                                   [] (double v) { return (v > 0 ? "+" : "") + juce::String (v, 1) + " st"; });
        shapeKnob    = std::make_unique<KnobCell> (state, pid::grainShape,    "Shape",
                                                   [] (double v) { return formatPercent (v); });

        for (auto* k : { sizeKnob.get(), densityKnob.get(), positionKnob.get(),
                         sprayKnob.get(), pitchKnob.get(), shapeKnob.get() })
            grainPanel.addAndMakeVisible (*k);

        pitchKnob->slider.setTooltip ("Grain transposition. C4 plays the sample at its original pitch.");
    }

    void PxfAudioProcessorEditor::buildFxStrip()
    {
        fxStrip.addAndMakeVisible (reverbCard);
        fxStrip.addAndMakeVisible (delayCard);
        fxStrip.addAndMakeVisible (dustCard);
        fxStrip.addAndMakeVisible (masterCard);

        auto& state = pxfProcessor.apvts;

        revSizeKnob = std::make_unique<KnobCell> (state, pid::revSize, "Size", [] (double v) { return formatPercent (v); });
        revMixKnob  = std::make_unique<KnobCell> (state, pid::revMix,  "Mix",  [] (double v) { return formatPercent (v); });

        delayDivBox.addItemList (delayDivNames(), 1);
        fxStrip.addAndMakeVisible (delayDivBox);
        delayDivAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (state, pid::delayDiv, delayDivBox);
        delayDivBox.setTooltip ("Delay time, synced to the host tempo.");

        delayFbKnob  = std::make_unique<KnobCell> (state, pid::delayFb,   "Feedback", [] (double v) { return formatPercent (v); });
        delayMixKnob = std::make_unique<KnobCell> (state, pid::delayMix,  "Mix",      [] (double v) { return formatPercent (v); });
        dustKnob     = std::make_unique<KnobCell> (state, pid::dust,      "Dust",     [] (double v) { return formatPercent (v); });
        volumeKnob   = std::make_unique<KnobCell> (state, pid::masterVol, "Volume",
                                                   [] (double v) { return juce::String (v, 1) + " dB"; });

        dustKnob->slider.setTooltip ("Saturation, tape wow, high-cut and vinyl noise, all on one knob.");

        for (auto* k : { revSizeKnob.get(), revMixKnob.get(), delayFbKnob.get(),
                         delayMixKnob.get(), dustKnob.get(), volumeKnob.get() })
            fxStrip.addAndMakeVisible (*k);
    }

    //==============================================================================
    void PxfAudioProcessorEditor::resized()
    {
        auto area = getLocalBounds();

        // ---- header ----
        auto header = area.removeFromTop (kHeaderH).reduced (kMargin, 0);
        header.removeFromLeft (150);                       // wordmark

        auto tabs = header.removeFromRight (150);
        tabs = tabs.withSizeKeepingCentre (150, 26);
        synthTabButton.setBounds (tabs.removeFromLeft (72));
        tabs.removeFromLeft (6);
        grainTabButton.setBounds (tabs);

        auto browser = header.withSizeKeepingCentre (juce::jmin (330, header.getWidth()), 44);
        prevPreset.setBounds (browser.removeFromLeft (26));
        nextPreset.setBounds (browser.removeFromRight (26));
        presetNameArea = browser;

        // ---- fx strip at the bottom ----
        auto fx = area.removeFromBottom (kFxH);
        fxStrip.setBounds (fx);

        // ---- tab content ----
        synthPanel.setBounds (area);
        grainPanel.setBounds (area);

        // ================= SYNTH =================
        {
            auto content = synthPanel.getLocalBounds().reduced (kMargin, 10);

            auto osc    = content.removeFromLeft (330); content.removeFromLeft (kGap);
            auto filter = content.removeFromLeft (186); content.removeFromLeft (kGap);
            auto amp    = content.removeFromLeft (204); content.removeFromLeft (kGap);
            auto voice  = content;

            oscCard   .setBounds (osc);
            filterCard.setBounds (filter);
            ampCard   .setBounds (amp);
            voiceCard .setBounds (voice);

            // oscillators: two wave selectors, then mix / detune / sub
            {
                auto inner = oscCard.getContentArea() + oscCard.getPosition();

                auto row1 = inner.removeFromTop (28);
                osc1Label.setBounds (row1.removeFromLeft (48));
                row1.removeFromLeft (6);
                osc1Box.setBounds (row1.reduced (0, 1));

                inner.removeFromTop (6);

                auto row2 = inner.removeFromTop (28);
                osc2Label.setBounds (row2.removeFromLeft (48));
                row2.removeFromLeft (6);
                osc2Box.setBounds (row2.reduced (0, 1));

                inner.removeFromTop (10);

                const int cell = inner.getWidth() / 3;
                mixKnob   ->setBounds (inner.removeFromLeft (cell));
                detuneKnob->setBounds (inner.removeFromLeft (cell));
                subKnob   ->setBounds (inner);
            }

            // filter
            {
                auto inner = filterCard.getContentArea() + filterCard.getPosition();
                inner = inner.withSizeKeepingCentre (inner.getWidth(), juce::jmin (inner.getHeight(), 150));

                const int cell = inner.getWidth() / 2;
                cutoffKnob->setBounds (inner.removeFromLeft (cell));
                resoKnob  ->setBounds (inner);
            }

            // amp envelope: 2 x 2
            {
                auto inner = ampCard.getContentArea() + ampCard.getPosition();

                auto top    = inner.removeFromTop (inner.getHeight() / 2);
                auto bottom = inner;

                const int cell = top.getWidth() / 2;
                attackKnob ->setBounds (top.removeFromLeft (cell));
                decayKnob  ->setBounds (top);
                sustainKnob->setBounds (bottom.removeFromLeft (cell));
                releaseKnob->setBounds (bottom);
            }

            // voice: glide over the two chips
            {
                auto inner = voiceCard.getContentArea() + voiceCard.getPosition();

                auto chips = inner.removeFromBottom (70);
                glideKnob->setBounds (inner.reduced (4, 0));

                monoButton  .setBounds (chips.removeFromTop (28));
                chips.removeFromTop (8);
                legatoButton.setBounds (chips.removeFromTop (28));
            }
        }

        // ================= GRAIN =================
        {
            auto content = grainPanel.getLocalBounds().reduced (kMargin, 10);

            auto sample = content.removeFromTop (196);
            content.removeFromTop (kGap);
            auto grains = content;

            sampleCard.setBounds (sample);
            grainCard .setBounds (grains);

            {
                auto inner = sampleCard.getContentArea() + sampleCard.getPosition();

                auto row = inner.removeFromTop (28);
                loadButton.setBounds (row.removeFromLeft (86).reduced (0, 1));
                row.removeFromLeft (10);
                fileLabel.setBounds (row);

                inner.removeFromTop (8);
                waveform.setBounds (inner);
            }

            {
                auto inner = grainCard.getContentArea() + grainCard.getPosition();

                const int cell = inner.getWidth() / 6;

                sizeKnob    ->setBounds (inner.removeFromLeft (cell));
                densityKnob ->setBounds (inner.removeFromLeft (cell));
                positionKnob->setBounds (inner.removeFromLeft (cell));
                sprayKnob   ->setBounds (inner.removeFromLeft (cell));
                pitchKnob   ->setBounds (inner.removeFromLeft (cell));
                shapeKnob   ->setBounds (inner);
            }
        }

        // ================= MASTER FX =================
        {
            auto content = fxStrip.getLocalBounds().reduced (kMargin, 8);
            content.removeFromBottom (4);

            auto reverb = content.removeFromLeft (210); content.removeFromLeft (kGap);
            auto delay  = content.removeFromLeft (306); content.removeFromLeft (kGap);
            auto dust   = content.removeFromLeft (176); content.removeFromLeft (kGap);
            auto master = content;

            reverbCard.setBounds (reverb);
            delayCard .setBounds (delay);
            dustCard  .setBounds (dust);
            masterCard.setBounds (master);

            {
                auto inner = reverbCard.getContentArea() + reverbCard.getPosition();
                const int cell = inner.getWidth() / 2;
                revSizeKnob->setBounds (inner.removeFromLeft (cell));
                revMixKnob ->setBounds (inner);
            }

            {
                auto inner = delayCard.getContentArea() + delayCard.getPosition();

                auto timeColumn = inner.removeFromLeft (92);
                delayDivBox.setBounds (timeColumn.withSizeKeepingCentre (86, 26));

                const int cell = inner.getWidth() / 2;
                delayFbKnob ->setBounds (inner.removeFromLeft (cell));
                delayMixKnob->setBounds (inner);
            }

            dustKnob  ->setBounds ((dustCard  .getContentArea() + dustCard  .getPosition()).reduced (10, 0));
            volumeKnob->setBounds ((masterCard.getContentArea() + masterCard.getPosition()).reduced (10, 0));
        }
    }

    //==============================================================================
    void PxfAudioProcessorEditor::showTab (int tab)
    {
        pxfProcessor.setActiveTab (tab);

        synthPanel.setVisible (tab == synthTab);
        grainPanel.setVisible (tab == grainTab);

        synthTabButton.setToggleState (tab == synthTab, juce::dontSendNotification);
        grainTabButton.setToggleState (tab == grainTab, juce::dontSendNotification);

        synthTabButton.repaint();
        grainTabButton.repaint();
    }

    void PxfAudioProcessorEditor::showPresetMenu()
    {
        juce::PopupMenu menu;
        menu.setLookAndFeel (&purpleLnf);

        juce::String currentCategory;

        for (int i = 0; i < kNumPresets; ++i)
        {
            const auto& preset = presets[i];

            if (juce::String (preset.category) != currentCategory)
            {
                currentCategory = preset.category;
                menu.addSectionHeader (currentCategory);
            }

            menu.addItem (i + 1, preset.name, true, i == pxfProcessor.getPresetIndex());
        }

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this)
                                                      .withTargetScreenArea (localAreaToGlobal (presetNameArea)),
                            [this] (int result)
                            {
                                if (result > 0)
                                {
                                    pxfProcessor.loadPreset (result - 1);
                                    refreshPresetLabel();
                                }
                            });
    }

    void PxfAudioProcessorEditor::refreshPresetLabel()
    {
        syncMonoLegato();
        repaint (getLocalBounds().removeFromTop (kHeaderH));
    }

    void PxfAudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
    {
        if (presetNameArea.contains (e.getPosition()))
            showPresetMenu();
    }

    //==============================================================================
    void PxfAudioProcessorEditor::chooseFile()
    {
        chooser = std::make_unique<juce::FileChooser> ("Load a one-shot",
                                                       juce::File::getSpecialLocation (juce::File::userMusicDirectory),
                                                       "*.wav;*.aiff;*.aif;*.mp3;*.flac;*.ogg");

        const auto browserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync (browserFlags, [this] (const juce::FileChooser& fc)
        {
            const auto file = fc.getResult();

            if (file.existsAsFile())
                loadFile (file);
        });
    }

    void PxfAudioProcessorEditor::loadFile (const juce::File& file)
    {
        juce::String error;

        if (! pxfProcessor.loadSample (file, error))
        {
            waveform.setPlaceholder (error.toUpperCase());
            return;
        }

        refreshSample();
    }

    void PxfAudioProcessorEditor::refreshSample()
    {
        lastSampleVersion = pxfProcessor.grain.getVersion();

        if (pxfProcessor.grain.hasSample())
        {
            std::vector<float> mins, maxs;
            pxfProcessor.grain.getPeaks (mins, maxs);
            waveform.setPeaks (std::move (mins), std::move (maxs));

            fileLabel.setText (pxfProcessor.grain.getFileName(), juce::dontSendNotification);
        }
        else
        {
            waveform.setPeaks ({}, {});
            waveform.setPlaceholder ("DROP AN AUDIO FILE");
            fileLabel.setText ("No sample loaded", juce::dontSendNotification);
        }
    }

    //==============================================================================
    void PxfAudioProcessorEditor::timerCallback()
    {
        // Header follows the host: presets can change from automation too.
        if (shownPresetIndex != pxfProcessor.getPresetIndex())
        {
            shownPresetIndex = pxfProcessor.getPresetIndex();
            repaint (getLocalBounds().removeFromTop (kHeaderH));
        }

        if (monoButton.getToggleState() != legatoButton.isEnabled())
            syncMonoLegato();

        if (pxfProcessor.getActiveTab() != (synthPanel.isVisible() ? synthTab : grainTab))
            showTab (pxfProcessor.getActiveTab());

        if (! grainPanel.isVisible())
            return;

        if (lastSampleVersion != pxfProcessor.grain.getVersion())
            refreshSample();

        if (auto* param = pxfProcessor.apvts.getRawParameterValue (pid::grainPosition))
            waveform.setPosition (param->load() * 0.01f);

        float positions[GrainEngine::kVizSlots];
        const int count = pxfProcessor.grain.getVizPositions (positions, GrainEngine::kVizSlots);

        waveform.setGrainDots (std::vector<float> (positions, positions + count));
    }
}
