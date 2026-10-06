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

#include <cmath>
#include <vector>

/**
    Demonstrates image meshes.

    On the left the logo sits on a grid mesh drawn with Graphics::drawImageMesh(): pick the
    grid size, drag its points to warp the image, or turn on the ripple. On the right,
    thousands of spinning logos bounce around, all drawn by one
    Graphics::drawImageMeshInstanced() call, with their count, alpha and blending adjustable.
*/
class ImageMeshDemo : public yup::Component
{
public:
    ImageMeshDemo()
        : yup::Component ("ImageMeshDemo")
    {
        loadLogo();

        resetButton.setButtonText ("Reset");
        resetButton.onClick = [this]
        {
            resetWarp();
        };
        addAndMakeVisible (resetButton);

        rippleToggle.setButtonText ("Ripple");
        addAndMakeVisible (rippleToggle);

        gridLabel.setText ("Grid", yup::dontSendNotification);
        addAndMakeVisible (gridLabel);

        gridSlider.setRange (1.0, 8.0, 1.0);
        gridSlider.setValue (static_cast<double> (gridCells), yup::dontSendNotification);
        gridSlider.setTextBoxStyle (yup::Slider::TextBoxRight, false, 56, 20);
        gridSlider.onValueChanged = [this] (double value)
        {
            gridCells = static_cast<int> (value);
            resetWarp();
        };
        addAndMakeVisible (gridSlider);

        countLabel.setText ("Sprites", yup::dontSendNotification);
        addAndMakeVisible (countLabel);

        countSlider.setRange (1.0, 5000.0, 1.0);
        countSlider.setValue (1000.0, yup::dontSendNotification);
        countSlider.setTextBoxStyle (yup::Slider::TextBoxRight, false, 56, 20);
        countSlider.onValueChanged = [this] (double value)
        {
            setSpriteCount (static_cast<int> (value));
        };
        addAndMakeVisible (countSlider);

        additiveToggle.setButtonText ("Additive");
        addAndMakeVisible (additiveToggle);

        alphaLabel.setText ("Alpha", yup::dontSendNotification);
        addAndMakeVisible (alphaLabel);

        alphaSlider.setRange (0.0, 1.0, 0.01);
        alphaSlider.setValue (static_cast<double> (spriteOpacity), yup::dontSendNotification);
        alphaSlider.setTextBoxStyle (yup::Slider::TextBoxRight, false, 56, 20);
        alphaSlider.onValueChanged = [this] (double value)
        {
            spriteOpacity = static_cast<float> (value);
        };
        addAndMakeVisible (alphaSlider);

        setSpriteCount (1000);
    }

    //==============================================================================
    void resized() override
    {
        auto area = getLocalBounds().reduced (16.0f);

        // Each panel has its controls above it, on two rows so the sliders get the panel's width
        auto controls = area.removeFromTop (rowHeight * 2.0f + rowGap);
        auto warpControls = controls.removeFromLeft (controls.getWidth() * 0.5f).withTrimmedRight (6.0f);
        auto spriteControls = controls.withTrimmedLeft (6.0f);

        auto warpButtons = warpControls.removeFromTop (rowHeight);
        resetButton.setBounds (warpButtons.removeFromLeft (80.0f));
        warpButtons.removeFromLeft (8.0f);
        rippleToggle.setBounds (warpButtons.removeFromLeft (90.0f));

        auto gridRow = warpControls.removeFromBottom (rowHeight);
        gridLabel.setBounds (gridRow.removeFromLeft (labelWidth));
        gridSlider.setBounds (gridRow);

        auto countRow = spriteControls.removeFromTop (rowHeight);
        countLabel.setBounds (countRow.removeFromLeft (labelWidth));
        countSlider.setBounds (countRow);

        auto alphaRow = spriteControls.removeFromBottom (rowHeight);
        additiveToggle.setBounds (alphaRow.removeFromRight (100.0f));
        alphaRow.removeFromRight (8.0f);
        alphaLabel.setBounds (alphaRow.removeFromLeft (labelWidth));
        alphaSlider.setBounds (alphaRow);

        area.removeFromTop (12.0f);
        warpPanel = area.removeFromLeft (area.getWidth() * 0.5f).withTrimmedRight (6.0f);
        spritePanel = area.withTrimmedLeft (6.0f);

        resetWarp();

        // Sprites created before the first layout had no panel to spread over
        if (! spritesPlaced && ! spritePanel.isEmpty())
        {
            for (auto& sprite : sprites)
                sprite.position = randomPositionInPanel();

            spritesPlaced = true;
        }
    }

    void refreshDisplay (double lastFrameTimeSeconds) override
    {
        const auto elapsed = static_cast<float> (yup::jlimit (0.0, 0.1, lastFrameTimeSeconds));
        time += elapsed;

        updateSprites (elapsed);
        repaint();
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (paletteColor (yup::ThemePalette::Role::background));
        g.fillAll();

        for (const auto& panel : { warpPanel, spritePanel })
        {
            g.setFillColor (paletteColor (yup::ThemePalette::Role::surface));
            g.fillRoundedRect (panel, 8.0f);
        }

        paintWarp (g);
        paintSprites (g);

        const auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (13.0f);
        g.setFillColor (paletteColor (yup::ThemePalette::Role::textMuted));
        g.fillFittedText ("drawImageMesh - drag the points", font, warpPanel.reduced (12.0f).removeFromTop (18.0f), yup::Justification::topLeft);
        g.fillFittedText ("drawImageMeshInstanced - " + yup::String (static_cast<int> (sprites.size())) + " sprites in one call",
                          font,
                          spritePanel.reduced (12.0f).removeFromTop (18.0f),
                          yup::Justification::topLeft);
    }

    //==============================================================================
    void mouseDown (const yup::MouseEvent& event) override
    {
        draggedVertex = findVertexAt (event.getPosition());
    }

    void mouseDrag (const yup::MouseEvent& event) override
    {
        if (draggedVertex >= 0)
            warpPositions[static_cast<std::size_t> (draggedVertex)] = event.getPosition();
    }

    void mouseUp (const yup::MouseEvent&) override
    {
        draggedVertex = -1;
    }

private:
    struct Sprite
    {
        yup::Point<float> position;
        yup::Point<float> velocity;
        float angle = 0.0f;
        float spin = 0.0f;
        float scale = 1.0f;
    };

    static constexpr float spriteWidth = 36.0f;
    static constexpr float rowHeight = 28.0f;
    static constexpr float rowGap = 6.0f;
    static constexpr float labelWidth = 60.0f;

    static yup::Color paletteColor (yup::ThemePalette::Role role)
    {
        return yup::ApplicationTheme::getGlobalTheme()->getPalette().getColor (role);
    }

    //==============================================================================
    void loadLogo()
    {
        yup::MemoryBlock bytes;
        if (! getAssetPath ("data/logo.png").loadFileAsData (bytes))
            return;

        if (auto loaded = yup::Image::loadFromData (bytes.asBytes()); loaded.wasOk())
            logo = std::move (loaded.getReference());
    }

    float getLogoAspect() const
    {
        return logo.getWidth() > 0 ? static_cast<float> (logo.getHeight()) / static_cast<float> (logo.getWidth()) : 1.0f;
    }

    //==============================================================================
    void resetWarp()
    {
        // The logo, fitted into the panel below its caption
        auto area = warpPanel.reduced (32.0f).withTrimmedTop (16.0f);
        const float width = yup::jmin (area.getWidth(), area.getHeight() / getLogoAspect());
        const auto imageArea = yup::Rectangle<float> (0.0f, 0.0f, width, width * getLogoAspect()).withCenter (area.getCenter());

        warpMesh = yup::ImageMesh::createGrid (imageArea, gridCells, gridCells);

        const auto restPositions = warpMesh.getVertices();
        warpPositions.assign (restPositions.begin(), restPositions.end());
        draggedVertex = -1;
    }

    std::vector<yup::Point<float>> getDisplayedPositions() const
    {
        auto positions = warpPositions;

        if (rippleToggle.getToggleState())
        {
            for (std::size_t i = 0; i < positions.size(); ++i)
            {
                // The dragged point stays under the cursor
                if (static_cast<int> (i) == draggedVertex)
                    continue;

                auto& position = positions[i];
                const float wave = std::sin (time * 3.0f + position.getX() * 0.03f + position.getY() * 0.02f);
                position = position.translated (wave * 6.0f, std::cos (time * 2.5f + position.getX() * 0.025f) * 6.0f);
            }
        }

        return positions;
    }

    int findVertexAt (yup::Point<float> position) const
    {
        const auto positions = getDisplayedPositions();

        for (std::size_t i = 0; i < positions.size(); ++i)
        {
            if (positions[i].distanceTo (position) <= 10.0f)
                return static_cast<int> (i);
        }

        return -1;
    }

    void paintWarp (yup::Graphics& g)
    {
        if (warpPositions.empty())
            return;

        const auto positions = getDisplayedPositions();
        warpMesh.setVertices (positions);
        g.drawImageMesh (logo, warpMesh);

        // The grid and its points, so it is clear what can be dragged
        const int stride = gridCells + 1;
        g.setStrokeColor (paletteColor (yup::ThemePalette::Role::accent).withAlpha (0.35f));
        g.setStrokeWidth (1.0f);

        for (int row = 0; row < stride; ++row)
        {
            for (int column = 0; column < stride; ++column)
            {
                const auto& point = positions[static_cast<std::size_t> (row * stride + column)];

                if (column + 1 < stride)
                    g.strokeLine (point, positions[static_cast<std::size_t> (row * stride + column + 1)]);

                if (row + 1 < stride)
                    g.strokeLine (point, positions[static_cast<std::size_t> ((row + 1) * stride + column)]);
            }
        }

        g.setFillColor (paletteColor (yup::ThemePalette::Role::accent));
        for (const auto& point : positions)
            g.fillEllipse (yup::Rectangle<float> (0.0f, 0.0f, 6.0f, 6.0f).withCenter (point));
    }

    //==============================================================================
    void setSpriteCount (int count)
    {
        auto& random = yup::Random::getSystemRandom();

        while (static_cast<int> (sprites.size()) < count)
        {
            Sprite sprite;
            sprite.position = randomPositionInPanel();
            sprite.velocity = { (random.nextFloat() - 0.5f) * 240.0f, (random.nextFloat() - 0.5f) * 240.0f };
            sprite.angle = random.nextFloat() * yup::MathConstants<float>::twoPi;
            sprite.spin = (random.nextFloat() - 0.5f) * 4.0f;
            sprite.scale = 0.5f + random.nextFloat();
            sprites.push_back (sprite);
        }

        sprites.resize (static_cast<std::size_t> (yup::jmax (0, count)));
        instances.resize (sprites.size());
    }

    yup::Point<float> randomPositionInPanel() const
    {
        auto& random = yup::Random::getSystemRandom();
        return { spritePanel.getX() + random.nextFloat() * spritePanel.getWidth(),
                 spritePanel.getY() + random.nextFloat() * spritePanel.getHeight() };
    }

    void updateSprites (float elapsed)
    {
        const auto bounds = spritePanel.reduced (spriteWidth * 0.5f);

        for (auto& sprite : sprites)
        {
            sprite.position += sprite.velocity * elapsed;
            sprite.angle += sprite.spin * elapsed;

            // Bounce off the panel's edges
            if (sprite.position.getX() < bounds.getX() || sprite.position.getX() > bounds.getRight())
                sprite.velocity.setX (-sprite.velocity.getX());

            if (sprite.position.getY() < bounds.getY() || sprite.position.getY() > bounds.getBottom())
                sprite.velocity.setY (-sprite.velocity.getY());

            sprite.position = { yup::jlimit (bounds.getX(), bounds.getRight(), sprite.position.getX()),
                                yup::jlimit (bounds.getY(), bounds.getBottom(), sprite.position.getY()) };
        }
    }

    void paintSprites (yup::Graphics& g)
    {
        if (sprites.empty())
            return;

        const float additiveAmount = additiveToggle.getToggleState() ? 1.0f : 0.0f;

        for (std::size_t i = 0; i < sprites.size(); ++i)
        {
            const auto& sprite = sprites[i];
            auto& instance = instances[i];

            instance.transform = yup::AffineTransform::scaling (spriteWidth * sprite.scale, spriteWidth * sprite.scale * getLogoAspect())
                                     .rotated (sprite.angle)
                                     .translated (sprite.position);
            instance.opacity = spriteOpacity;
            instance.additiveAmount = additiveAmount;
        }

        const auto state = g.saveState();
        g.setClipPath (spritePanel);
        g.drawImageMeshInstanced (logo, spriteQuad, instances);
    }

    //==============================================================================
    yup::Image logo;
    yup::ImageMesh warpMesh;
    int gridCells = 8;
    std::vector<yup::Point<float>> warpPositions;
    int draggedVertex = -1;

    // A unit quad centered on the origin: each instance scales and places it
    const yup::ImageMesh spriteQuad = yup::ImageMesh::createGrid ({ -0.5f, -0.5f, 1.0f, 1.0f }, 1, 1);
    std::vector<Sprite> sprites;
    std::vector<yup::ImageMeshInstance> instances;
    bool spritesPlaced = false;
    float spriteOpacity = 0.85f;

    yup::Rectangle<float> warpPanel;
    yup::Rectangle<float> spritePanel;
    float time = 0.0f;

    yup::TextButton resetButton;
    yup::ToggleButton rippleToggle;
    yup::Label gridLabel;
    yup::Slider gridSlider { yup::Slider::LinearHorizontal };
    yup::Label countLabel;
    yup::Slider countSlider { yup::Slider::LinearHorizontal };
    yup::ToggleButton additiveToggle;
    yup::Label alphaLabel;
    yup::Slider alphaSlider { yup::Slider::LinearHorizontal };
};
