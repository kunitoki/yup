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

#include <algorithm>
#include <array>
#include <cmath>
#include <initializer_list>

//==============================================================================
/**
    A settings panel that comes apart in depth as the pointer comes near.

    The panel is four transparent layers of a SurfaceView stacked exactly on top of
    each other: the card, the tabs, the controls and their labels. At rest they look
    like one ordinary panel; as it lifts they float toward the viewer one behind the
    other, like an exploded diagram, and the stack tilts toward the pointer. Clicks
    fall through the empty parts of a layer to the ones behind, so every widget
    keeps working, and the layers extend past the card so the combo box popup has
    room to open.

    While the demo is open it plays a pad over a pulsing drone (see LayersSound),
    shaped by where the pointer is over the card.
*/
class Layers : public Demo
{
public:
    //==============================================================================
    Layers()
        : Demo ("Layers")
    {
        tabs.addTab ("sound", "sound");
        tabs.addTab ("light", "light");
        tabs.addTab ("space", "space");

        glow.setButtonText ("enabled");

        mode.addItem ("warm", warmPalette);
        mode.addItem ("cold", coldPalette);
        mode.addItem ("bright", brightPalette);
        mode.addItem ("dark", darkPalette);
        mode.setSelectedId (coldPalette, yup::dontSendNotification);
        mode.onSelectedItemChanged = [this]
        {
            applyPalette (mode.getSelectedId());

            // The layers are cached textures: render them again in the new colors.
            for (auto& layer : layers)
                layer.repaint();
        };

        name.setText ("my patch", yup::dontSendNotification);

        glowLabel.setText ("glow", yup::dontSendNotification);
        powerLabel.setText ("power", yup::dontSendNotification);
        modeLabel.setText ("mode", yup::dontSendNotification);
        nameLabel.setText ("name", yup::dontSendNotification);

        layers[1].addAndMakeVisible (tabs);
        for (auto* control : std::initializer_list<yup::Component*> { &glow, &power, &mode, &name })
            layers[2].addAndMakeVisible (*control);
        for (auto* label : { &glowLabel, &powerLabel, &modeLabel, &nameLabel })
            layers[3].addAndMakeVisible (*label);

        addAndMakeVisible (view);

        // Back to front, the order SurfaceView draws them in.
        for (std::size_t level = 0; level < layers.size(); ++level)
            surfaces[level] = view.addSurface (layers[level]);
    }

    void resized() override
    {
        view.setBounds (getLocalBounds());
        // Fitted on the card alone: the room left around it for the popup may run off the screen.
        scale = readableScale (cardWidth, cardHeight);
        view.setContentScale (scale);

        const auto layerWidth = cardWidth + 2.0f * popupRoom;
        const auto layerHeight = cardHeight + 2.0f * popupRoom;
        const auto layerOnScreen = centeredContent (layerWidth, layerHeight, scale);
        cardInView = layerOnScreen.reduced (popupRoom * scale);

        for (auto& layer : layers)
            layer.setBounds (layerOnScreen.getX(), layerOnScreen.getY(), layerWidth, layerHeight);

        // Everything below is laid out in layer coordinates, where the card starts at popupRoom.
        layers[0].card = { popupRoom, popupRoom, cardWidth, cardHeight };

        auto area = layers[0].card.reduced (20.0f);
        tabs.setBounds (area.removeFromTop (34.0f));
        area.removeFromTop (18.0f);

        const auto row = [&] (yup::Label& label, yup::Component& control, float controlWidth)
        {
            auto line = area.removeFromTop (rowHeight);
            area.removeFromTop (rowGap);
            label.setBounds (line.removeFromLeft (labelWidth));
            control.setBounds (line.removeFromLeft (controlWidth).reduced (0.0f, 4.0f));
        };

        row (glowLabel, glow, 140.0f);
        row (powerLabel, power, 64.0f);
        row (modeLabel, mode, 200.0f);
        row (nameLabel, name, 240.0f);

        const auto viewport = getLocalBounds().getSize();
        for (std::size_t level = 0; level < layers.size(); ++level)
            view.setSurfaceMesh (surfaces[level], SurfaceView::flatQuad (layerOnScreen, viewport), SurfaceView::quadIndices());

        modelLift = -1.0f;
    }

private:
    //==============================================================================
    static constexpr float cardWidth = 420.0f;
    static constexpr float cardHeight = 320.0f;
    static constexpr float popupRoom = 140.0f;
    static constexpr float rowHeight = 40.0f;
    static constexpr float rowGap = 10.0f;
    static constexpr float labelWidth = 90.0f;
    static constexpr float layerDepth = 46.0f;

    //==============================================================================
    /** A transparent plane of the panel; the bottom one also paints the card. */
    class Layer : public yup::Component
    {
    public:
        Layer()
            : yup::Component ("LayersLayer")
        {
            setOpaque (false);
            setWantsMouseEvents (false, true); // empty parts let clicks through to the layers behind
        }

        void paint (yup::Graphics& g) override
        {
            if (card.isEmpty())
                return;

            const auto& palette = yup::ApplicationTheme::getGlobalTheme()->getPalette();

            g.setFillColor (palette.getColor (yup::ThemePalette::Role::surface));
            g.fillRoundedRect (card, 12.0f);
            g.setStrokeColor (palette.getColor (yup::ThemePalette::Role::outline));
            g.setStrokeWidth (1.5f);
            g.strokeRoundedRect (card.reduced (0.75f), 12.0f);
        }

        yup::Rectangle<float> card; // only set on the bottom layer
    };

    //==============================================================================
    yup::Component* getFocusTarget() override { return &tabs; }

    SoundSource* getSoundSource() override { return &sound; }

    void advance (float deltaSeconds) override
    {
        const auto lift = layersLift.update (pointer, cardInView, deltaSeconds);
        view.setLift (lift);

        const auto center = cardInView.getCenter();
        if (pointer.has_value())
        {
            pointerOffset = { yup::jlimit (-1.0f, 1.0f, (pointer->getX() - center.getX()) / (0.5f * cardInView.getWidth())),
                              yup::jlimit (-1.0f, 1.0f, (pointer->getY() - center.getY()) / (0.5f * cardInView.getHeight())) };

            sound.setPosition (0.5f + 0.5f * pointerOffset.getX(), 0.5f + 0.5f * pointerOffset.getY());
        }
        else
        {
            sound.setPosition (0.5f, 0.5f); // settles back to the middle once the pointer has gone
        }

        const auto yaw = 0.5f * lift * pointerOffset.getX();
        const auto pitch = 0.35f * lift * pointerOffset.getY();

        // The bass pulses breathe the layers apart, and the brighter the sound the warmer the light.
        // The layers are picked where they are drawn, so the breath holds still while a widget is
        // dragged, or the music would move it under the pointer.
        // Everything follows the sound at once and lets go slowly, so the stack breathes instead of twitching.
        if (! isDragging())
            breath = easeToward (breath, hearing.lowAccent, deltaSeconds, 0.02f, 0.9f);

        rimGlow = easeToward (rimGlow, 0.5f * hearing.hit, deltaSeconds, 0.01f, 0.5f);
        warmth = easeToward (warmth, std::clamp ((hearing.high - 0.3f) * 1.6f, 0.0f, 1.0f), deltaSeconds, 0.03f, 0.8f);

        const auto spread = lift * (1.0f + 0.8f * breath);
        view.setAudio (rimGlow, warmth);

        if (std::abs (spread - modelLift) < 0.0001f && std::abs (yaw - modelYaw) < 0.0001f && std::abs (pitch - modelPitch) < 0.0001f)
            return;

        modelLift = spread;
        modelYaw = yaw;
        modelPitch = pitch;

        const auto pivot = pixelToWorld (center, 0.0f, getLocalBounds().getSize());
        const auto tilt = yup::Matrix4::translation (-pivot)
                              .followedBy (yup::Matrix4::rotationY (yaw))
                              .followedBy (yup::Matrix4::rotationX (pitch))
                              .followedBy (yup::Matrix4::translation (pivot));

        for (std::size_t level = 0; level < layers.size(); ++level)
        {
            const auto rise = yup::Matrix4::translation ({ 0.0f, 0.0f, static_cast<float> (level) * layerDepth * scale * spread });
            view.setSurfaceTransform (surfaces[level], rise.followedBy (tilt));
        }
    }

    //==============================================================================
    SurfaceView view;
    std::array<Layer, 4> layers; // card, tabs, controls, labels
    std::array<int, 4> surfaces {};

    yup::TabBar tabs;
    yup::ToggleButton glow;
    yup::SwitchButton power;
    yup::ComboBox mode;
    yup::TextEditor name;
    yup::Label glowLabel, powerLabel, modeLabel, nameLabel;

    yup::Rectangle<float> cardInView;
    float scale = 1.0f;

    Lift layersLift;
    yup::Point<float> pointerOffset;
    float modelLift = -1.0f;
    float breath = 0.0f, rimGlow = 0.0f, warmth = 0.0f;
    float modelYaw = 0.0f;
    float modelPitch = 0.0f;

    LayersSound sound;
};
