/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

   YUP is an open source library subject to open-source licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#pragma once

#include <array>
#include <cmath>
#include <vector>

/**
    Demonstrates color emoji.

    Text shaping falls back to Font::getColorEmojiFallbackFont() for every character the text
    font lacks, so the theme font below draws emoji without knowing about them. Switch
    between the system emoji font and the bundled Twemoji font (the only choice on the web),
    change the size, or type in the editor at the bottom.

    Emoji are drawn by the renderer like any other text, so the effects strip at the top
    treats them as artwork: tinted, shadowed, glowing with additive blending, used as the
    mask of a gradient layer, squashed by a transform and leaving a tinted trail.
*/
class EmojiDemo : public yup::Component
{
public:
    EmojiDemo()
        : yup::Component ("EmojiDemo")
        , previousFallbackFont (yup::Font::getColorEmojiFallbackFont())
    {
        if (auto systemFont = yup::Font::loadColorEmojiSystemFont(); systemFont.wasOk())
        {
            emojiFonts.push_back (systemFont.getValue());
            fontCombo.addItem ("System", static_cast<int> (emojiFonts.size()));
        }

        if (auto twemoji = yup::Font::loadFontFromFile (getAssetPath ("data/fonts/Twemoji.ttf")); twemoji.wasOk())
        {
            emojiFonts.push_back (twemoji.getValue());
            fontCombo.addItem ("Twemoji", static_cast<int> (emojiFonts.size()));
        }

        fontLabel.setText ("Emoji font", yup::dontSendNotification);
        addAndMakeVisible (fontLabel);

        fontCombo.onSelectedItemChanged = [this]
        {
            selectEmojiFont (fontCombo.getSelectedId());
        };
        addAndMakeVisible (fontCombo);

        sizeLabel.setText ("Size", yup::dontSendNotification);
        addAndMakeVisible (sizeLabel);

        sizeSlider.setRange (12.0, 96.0, 1.0);
        sizeSlider.setValue (static_cast<double> (textSize), yup::dontSendNotification);
        sizeSlider.setTextBoxStyle (yup::Slider::TextBoxRight, false, 56, 20);
        sizeSlider.onValueChanged = [this] (double value)
        {
            textSize = static_cast<float> (value);
            shapeSamples();
        };
        addAndMakeVisible (sizeSlider);

        editor.setText (utf8 (u8"Type or paste emoji here \U0001F680"), yup::dontSendNotification);
        addAndMakeVisible (editor);

        if (! emojiFonts.empty())
        {
            fontCombo.setSelectedId (1, yup::dontSendNotification);
            selectEmojiFont (1);
        }
    }

    ~EmojiDemo() override
    {
        // The fallback font is process-wide: leave it as the demo found it
        yup::Font::setColorEmojiFallbackFont (previousFallbackFont);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (16.0f);

        auto controls = area.removeFromTop (28.0f);
        fontLabel.setBounds (controls.removeFromLeft (90.0f));
        fontCombo.setBounds (controls.removeFromLeft (140.0f));
        controls.removeFromLeft (24.0f);
        sizeLabel.setBounds (controls.removeFromLeft (44.0f));
        sizeSlider.setBounds (controls.removeFromLeft (260.0f));

        editor.setBounds (area.removeFromBottom (36.0f));
        area.removeFromBottom (12.0f);

        area.removeFromTop (12.0f);
        effectsArea = area.removeFromTop (yup::jmin (170.0f, area.getHeight() * 0.4f));
        samplesArea = area.withTrimmedTop (12.0f);

        shapeSamples();
    }

    void refreshDisplay (double lastFrameTimeSeconds) override
    {
        time += static_cast<float> (yup::jlimit (0.0, 0.1, lastFrameTimeSeconds));
        repaint (effectsArea);
    }

    void paint (yup::Graphics& g) override
    {
        const auto& palette = yup::ApplicationTheme::getGlobalTheme()->getPalette();

        g.setFillColor (palette.getColor (yup::ThemePalette::Role::background));
        g.fillAll();

        if (emojiFonts.empty())
        {
            g.setFillColor (palette.getColor (yup::ThemePalette::Role::textMuted));
            g.fillFittedText ("No color emoji font available", captionFont(), samplesArea, yup::Justification::center);
            return;
        }

        paintEffects (g, palette);

        auto area = samplesArea;

        for (std::size_t i = 0; i < sampleTexts.size() && area.getHeight() > 0.0f; ++i)
        {
            g.setFillColor (palette.getColor (yup::ThemePalette::Role::textMuted));
            g.fillFittedText (getSamples()[i].caption, captionFont(), area.removeFromTop (20.0f), yup::Justification::topLeft);

            g.setFillColor (palette.getColor (yup::ThemePalette::Role::text));
            g.fillFittedText (sampleTexts[i], area.removeFromTop (sampleTexts[i].getComputedTextBounds().getHeight()));

            area.removeFromTop (12.0f);
        }
    }

private:
    struct Sample
    {
        yup::String caption;
        yup::String text;
    };

    // Text is shaped ahead of time: shaping it on every frame would redo the emoji work each time
    static constexpr float emojiBaseSize = 64.0f;
    static constexpr float emojiBox = emojiBaseSize * 1.6f;

    static yup::String utf8 (const char8_t* text)
    {
        return yup::String::fromUTF8 (reinterpret_cast<const char*> (text));
    }

    static const std::vector<Sample>& getSamples()
    {
        static const std::vector<Sample> samples {
            { "Mixed with text", utf8 (u8"Hello \U0001F44B world \U0001F30D, emoji sit inside the text \U0001F600\U0001F389") },
            { "Skin tones", utf8 (u8"\U0001F44D \U0001F44D\U0001F3FB \U0001F44D\U0001F3FC \U0001F44D\U0001F3FD \U0001F44D\U0001F3FE \U0001F44D\U0001F3FF") },
            { "Flags", utf8 (u8"\U0001F1EE\U0001F1F9 \U0001F1EF\U0001F1F5 \U0001F1E7\U0001F1F7 \U0001F1F0\U0001F1EA \U0001F1FA\U0001F1E6") },
            { "Keycaps and emoji presentation", utf8 (u8"1\uFE0F\u20E3 #\uFE0F\u20E3 \u2764 \u2764\uFE0F \u2600 \u2600\uFE0F") },
            { "Zero width joiner sequences (split when the text font has a ZWJ glyph)", utf8 (u8"\U0001F469\u200D\U0001F4BB \U0001F468\u200D\U0001F469\u200D\U0001F467 \U0001F3F3\uFE0F\u200D\U0001F308") },
            { "Wrapping", utf8 (u8"Every character the text font lacks \U0001F50D falls back to the emoji font, so labels, "
                                u8"editors and Rive text all draw color emoji \U0001F3A8 without extra code. Resize the "
                                u8"window to watch them wrap with the words \U0001F4DD\U0001F4DA\U0001F58B\uFE0F.") }
        };

        return samples;
    }

    enum Effect
    {
        tintEffect,
        shadowEffect,
        glowEffect,
        maskEffect,
        transformEffect,
        trailEffect,
        numEffects
    };

    void shapeSamples()
    {
        const auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont();
        const auto& samples = getSamples();

        sampleTexts.resize (samples.size());
        for (std::size_t i = 0; i < samples.size(); ++i)
        {
            auto modifier = sampleTexts[i].startUpdate();
            modifier.clear();
            modifier.setMaxSize ({ samplesArea.isEmpty() ? -1.0f : samplesArea.getWidth(), -1.0f });
            modifier.appendText (samples[i].text, font.withHeight (textSize));
        }

        static constexpr std::array<const char8_t*, numEffects> effectEmoji {
            u8"\U0001F984", u8"\U0001F419", u8"\U0001F31F", u8"\U0001F3B8", u8"\U0001F354", u8"\U0001F680"
        };

        for (std::size_t i = 0; i < effectEmoji.size(); ++i)
        {
            auto modifier = effectTexts[i].startUpdate();
            modifier.clear();
            modifier.setMaxSize ({ emojiBox, emojiBox });
            modifier.setHorizontalAlign (yup::StyledText::center);
            modifier.setVerticalAlign (yup::StyledText::middle);
            modifier.appendText (utf8 (effectEmoji[i]), font.withHeight (emojiBaseSize));
        }

        repaint();
    }

    /** Draws an effect's emoji centered on a point, scaled from its shaped size. */
    void drawEmoji (yup::Graphics& g, Effect effect, yup::Point<float> center, float size) const
    {
        const auto state = g.saveState();
        g.addTransform (yup::AffineTransform::scaling (size / emojiBaseSize).translated (center));
        g.fillFittedText (effectTexts[static_cast<std::size_t> (effect)], { -emojiBox * 0.5f, -emojiBox * 0.5f, emojiBox, emojiBox });
    }

    static yup::Color hue (float h)
    {
        return yup::Color::fromHSL (h - std::floor (h), 0.9f, 0.6f);
    }

    void paintEffects (yup::Graphics& g, const yup::ThemePalette& palette)
    {
        static constexpr std::array<const char*, numEffects> captions { "Tint", "Shadow", "Additive glow", "Gradient through a mask", "Transform", "Tinted trail" };

        const auto cellWidth = effectsArea.getWidth() / static_cast<float> (captions.size());

        for (std::size_t i = 0; i < captions.size(); ++i)
        {
            auto cell = effectsArea.withX (effectsArea.getX() + cellWidth * static_cast<float> (i)).withWidth (cellWidth).reduced (4.0f);

            g.setFillColor (palette.getColor (yup::ThemePalette::Role::surface));
            g.fillRoundedRect (cell, 8.0f);

            g.setFillColor (palette.getColor (yup::ThemePalette::Role::textMuted));
            g.fillFittedText (captions[i], captionFont(), cell.removeFromBottom (24.0f), yup::Justification::center);

            const auto state = g.saveState();
            g.setClipPath (cell);
            paintEffect (g, static_cast<Effect> (i), cell);
        }
    }

    void paintEffect (yup::Graphics& g, Effect effect, yup::Rectangle<float> cell)
    {
        const auto center = cell.getCenter();
        const auto size = yup::jmin (cell.getWidth(), cell.getHeight()) * 0.5f;

        g.setFillColor (yup::Colors::white);

        switch (effect)
        {
            case tintEffect:
                // Tint multiplies every color of the emoji
                g.setTint (hue (time * 0.15f));
                drawEmoji (g, effect, center, size);
                break;

            case shadowEffect:
            {
                // A black tint turns the emoji into its own silhouette
                const auto lift = (std::sin (time * 2.0f) * 0.5f + 0.5f) * size * 0.3f;
                {
                    const auto state = g.saveState();
                    g.setTint (yup::Colors::black);
                    g.setOpacity (0.5f - 0.3f * lift / size);
                    drawEmoji (g, effect, center.translated (size * 0.1f + lift * 0.4f, size * 0.12f + lift * 0.3f), size);
                }
                drawEmoji (g, effect, center.translated (0.0f, -lift), size);
                break;
            }

            case glowEffect:
            {
                // Enlarged tinted copies, grouped in a layer that is added onto the background
                const auto pulse = std::sin (time * 3.0f) * 0.5f + 0.5f;
                {
                    const auto state = g.saveState();
                    auto layer = g.beginTransparencyLayer (cell, 0.4f + 0.5f * pulse);
                    if (layer.isValid())
                    {
                        auto& layerGraphics = layer.getGraphics();
                        layerGraphics.setFillColor (yup::Colors::white);
                        layerGraphics.setTint (yup::Color (0xffff9a3c));

                        for (int ring = 3; ring >= 1; --ring)
                        {
                            layerGraphics.setOpacity (0.6f / static_cast<float> (ring));
                            drawEmoji (layerGraphics, effect, center - cell.getPosition(), size * (1.0f + 0.15f * static_cast<float> (ring) * (0.5f + pulse)));
                        }

                        g.setBlendMode (yup::BlendMode::Additive);
                        g.setAdditiveAmount (1.0f);
                        layer.commit();
                    }
                }
                drawEmoji (g, effect, center, size);
                break;
            }

            case maskEffect:
            {
                // The emoji is the alpha mask of a layer filled with a turning gradient
                auto layer = g.beginTransparencyLayer (cell);
                if (! layer.isValid())
                    break;

                const auto localCenter = cell.getCenter() - cell.getPosition();
                const auto direction = yup::Point<float> (std::cos (time), std::sin (time)) * size;

                auto& layerGraphics = layer.getGraphics();
                layerGraphics.setFillColorGradient (yup::ColorGradient (hue (time * 0.1f), localCenter - direction, hue (time * 0.1f + 0.5f), localCenter + direction));
                layerGraphics.fillAll();

                if (auto* mask = layer.addMask (yup::LayerMaskMode::Alpha))
                {
                    mask->setFillColor (yup::Colors::white);
                    drawEmoji (*mask, effect, localCenter, size);
                }

                layer.commit();
                break;
            }

            case transformEffect:
            {
                // Vector emoji (Twemoji) stay sharp under any transform, bitmap ones are resampled
                const auto squash = 0.15f * std::sin (time * 6.0f);
                g.addTransform (yup::AffineTransform::translation (-center.getX(), -center.getY())
                                    .scaled (1.0f + squash, 1.0f - squash)
                                    .sheared (0.2f * std::sin (time * 3.0f), 0.0f)
                                    .translated (center.getX(), center.getY()));
                drawEmoji (g, effect, center, size);
                break;
            }

            case trailEffect:
            {
                // Older positions drawn smaller, fainter and tinted
                constexpr int trailLength = 8;
                for (int step = trailLength - 1; step >= 0; --step)
                {
                    const auto t = time - static_cast<float> (step) * 0.08f;
                    const auto position = center.translated (std::cos (t * 1.3f) * cell.getWidth() * 0.28f, std::sin (t * 2.6f) * cell.getHeight() * 0.2f);
                    const auto fade = static_cast<float> (step) / static_cast<float> (trailLength);

                    const auto state = g.saveState();
                    if (step > 0)
                    {
                        g.setTint (hue (fade + time * 0.2f));
                        g.setOpacity (0.6f * (1.0f - fade));
                    }

                    drawEmoji (g, effect, position, size * (0.7f - 0.3f * fade));
                }
                break;
            }

            case numEffects:
                break;
        }
    }

    static yup::Font captionFont()
    {
        return yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (13.0f);
    }

    void selectEmojiFont (int itemId)
    {
        if (itemId < 1 || itemId > static_cast<int> (emojiFonts.size()))
            return;

        yup::Font::setColorEmojiFallbackFont (emojiFonts[static_cast<std::size_t> (itemId - 1)]);

        // Shaped text keeps its old emoji until it is shaped again, and setting the same text is a no-op
        const auto text = editor.getText();
        editor.setText ({}, yup::dontSendNotification);
        editor.setText (text, yup::dontSendNotification);

        shapeSamples();
    }

    yup::Font previousFallbackFont;
    std::vector<yup::Font> emojiFonts;
    yup::Label fontLabel;
    yup::ComboBox fontCombo;
    yup::Label sizeLabel;
    yup::Slider sizeSlider { yup::Slider::LinearHorizontal };
    yup::TextEditor editor;
    yup::Rectangle<float> effectsArea;
    yup::Rectangle<float> samplesArea;
    std::vector<yup::StyledText> sampleTexts;
    std::array<yup::StyledText, numEffects> effectTexts;
    float textSize = 18.0f;
    float time = 0.0f;
};
