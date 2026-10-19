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
#include <memory>
#include <optional>

//==============================================================================
/**
    A small mixer under liquid glass that ripples where the pointer moves.

    The panel is ordinary widgets (faders, knobs, a switch) with a RippleEffect:
    near the pointer a lens magnifies them, and moving the pointer leaves rings
    that spread across the panel and fade, lit like water. The widgets keep
    working through the warp.
*/
class Ripple : public Demo
{
public:
    //==============================================================================
    Ripple()
        : Demo ("Ripple")
    {
        static constexpr std::array<const char*, numFaders> faderNames { "low", "mid", "high", "air" };
        static constexpr std::array<const char*, numKnobs> knobNames { "drive", "space" };

        for (std::size_t i = 0; i < faders.size(); ++i)
        {
            faders[i] = std::make_unique<yup::Slider> (yup::Slider::LinearVertical, "fader");
            faders[i]->setRange (0.0, 1.0);
            faders[i]->setValue (0.3 + 0.15 * static_cast<double> (i));
            faderLabels[i].setText (faderNames[i], yup::dontSendNotification);
            panel.addAndMakeVisible (*faders[i]);
            panel.addAndMakeVisible (faderLabels[i]);
        }

        for (std::size_t i = 0; i < knobs.size(); ++i)
        {
            knobs[i] = std::make_unique<yup::Slider> (yup::Slider::RotaryVerticalDrag, "knob");
            knobs[i]->setRange (0.0, 1.0);
            knobs[i]->setMouseDragSensitivity (knobDragSensitivity);
            knobs[i]->setValue (0.6 - 0.25 * static_cast<double> (i));
            knobLabels[i].setText (knobNames[i], yup::dontSendNotification);
            panel.addAndMakeVisible (*knobs[i]);
            panel.addAndMakeVisible (knobLabels[i]);
        }

        powerLabel.setText ("power", yup::dontSendNotification);
        panel.addAndMakeVisible (power);
        panel.addAndMakeVisible (powerLabel);

        panel.setComponentEffect (yup::ComponentEffect::Ptr (effect.get()));
        addAndMakeVisible (panel);
    }

    void resized() override
    {
        const auto scale = contentScale();

        // The panel reaches past the card, so the warp can bend the card's own edge outward too.
        cardBounds = centeredContent (panelWidth, panelHeight);
        panel.setCardInset (rippleRoom * scale);
        panel.setBounds (cardBounds.enlarged (rippleRoom * scale));

        const auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (13.0f * scale);
        auto area = panel.getLocalBounds().reduced ((rippleRoom + 28.0f) * scale);
        auto faderArea = area.removeFromLeft (area.getWidth() * 0.58f);
        area.removeFromLeft (16.0f * scale);

        const auto faderSlot = faderArea.getWidth() / numFaders;
        for (std::size_t i = 0; i < faders.size(); ++i)
        {
            auto slot = faderArea.removeFromLeft (faderSlot);
            faderLabels[i].setFont (font);
            faderLabels[i].setBounds (slot.removeFromBottom (22.0f * scale));
            faders[i]->setBounds (slot.withSizeKeepingCenter (34.0f * scale, slot.getHeight()));
        }

        auto switchRow = area.removeFromBottom (40.0f * scale);
        powerLabel.setFont (font);
        powerLabel.setBounds (switchRow.removeFromLeft (70.0f * scale));
        power.setBounds (switchRow.removeFromLeft (64.0f * scale).reduced (0.0f, 6.0f * scale));

        const auto knobSlot = area.getHeight() / numKnobs;
        for (std::size_t i = 0; i < knobs.size(); ++i)
        {
            auto slot = area.removeFromTop (knobSlot);
            knobLabels[i].setFont (font);
            knobLabels[i].setBounds (slot.removeFromBottom (22.0f * scale));
            knobs[i]->setBounds (slot.withSizeKeepingCenter (slot.getHeight(), slot.getHeight()));
        }
    }

    yup::Component* getFocusTarget() override { return faders.front().get(); }

private:
    //==============================================================================
    static constexpr float panelWidth = 560.0f;
    static constexpr float panelHeight = 360.0f;
    static constexpr int numFaders = 4;
    static constexpr int numKnobs = 2;
    static constexpr double knobDragSensitivity = 1.0; // a drag as tall as the knob sweeps its whole range
    static constexpr float ringSpacing = 0.06f;    // pointer travel between two rings, as a fraction of the panel height
    static constexpr float ringSpeed = 0.35f;      // fractions of the height per second
    static constexpr float fullRingSpeed = 1.5f;   // pointer speed giving the strongest rings, in heights per second
    static constexpr float strongestRing = 0.014f; // ring amplitude, in uv
    static constexpr float rippleRoom = 28.0f;     // transparent margin around the card for its edge to ripple into

    //==============================================================================
    class Panel : public yup::Component
    {
    public:
        Panel()
            : yup::Component ("RipplePanel")
        {
            setOpaque (false);
        }

        void setCardInset (float newInset)
        {
            cardInset = newInset;
            repaint();
        }

        void paint (yup::Graphics& g) override
        {
            const auto& palette = yup::ApplicationTheme::getGlobalTheme()->getPalette();
            const auto card = getLocalBounds().reduced (cardInset);

            g.setFillColor (palette.getColor (yup::ThemePalette::Role::surface));
            g.fillRoundedRect (card, 14.0f);
            g.setStrokeColor (palette.getColor (yup::ThemePalette::Role::outline));
            g.setStrokeWidth (1.5f);
            g.strokeRoundedRect (card.reduced (0.75f), 14.0f);
        }

    private:
        float cardInset = 0.0f;
    };

    //==============================================================================
    void advance (float deltaSeconds) override
    {
        const auto lift = rippleLift.update (pointer, cardBounds, deltaSeconds);
        const auto size = yup::Point<float> (panel.getWidth(), panel.getHeight());

        if (pointer.has_value() && panel.getHeight() > 0.0f)
        {
            const auto uv = (*pointer - panel.getBounds().getTopLeft()) / size;
            lensCenter = lensCenter + (uv - lensCenter) * yup::jmin (1.0f, deltaSeconds * 12.0f);

            // A ring every so often along the pointer's path, stronger the faster it moves.
            if (! lastRingAt.has_value())
                lastRingAt = uv;

            const auto travelled = (uv - *lastRingAt).distanceTo ({}) * size.getX() / size.getY();
            if (travelled > ringSpacing)
            {
                const auto speed = travelled / yup::jmax (deltaSeconds, 0.001f);
                addRing (uv, strongestRing * yup::jlimit (0.25f, 1.0f, speed / fullRingSpeed) * lift);
                lastRingAt = uv;
            }
        }
        else
        {
            lastRingAt.reset();
        }

        for (int i = 0; i < state.numRings; ++i)
        {
            auto& ring = state.rings[static_cast<std::size_t> (i)];
            ring.radius += deltaSeconds * ringSpeed;
            ring.amplitude *= std::exp (-deltaSeconds * 1.4f);
        }

        // Drop the rings that faded out, keeping the order
        int kept = 0;
        for (int i = 0; i < state.numRings; ++i)
        {
            if (state.rings[static_cast<std::size_t> (i)].amplitude > 0.0003f)
                state.rings[static_cast<std::size_t> (kept++)] = state.rings[static_cast<std::size_t> (i)];
        }
        state.numRings = kept;

        state.lensCenter = lensCenter;
        state.lensStrength = 0.35f * lift;
        effect->setState (state);

        const auto active = lift > 0.0f || state.numRings > 0;
        if (active || wasActive)
            panel.repaint();

        wasActive = active;
    }

    void addRing (yup::Point<float> center, float amplitude)
    {
        if (amplitude <= 0.0f)
            return;

        if (state.numRings == static_cast<int> (state.rings.size()))
        {
            for (std::size_t i = 1; i < state.rings.size(); ++i)
                state.rings[i - 1] = state.rings[i];

            --state.numRings;
        }

        state.rings[static_cast<std::size_t> (state.numRings++)] = { center, 0.0f, amplitude };
    }

    //==============================================================================
    Panel panel;
    yup::Rectangle<float> cardBounds;
    std::array<std::unique_ptr<yup::Slider>, numFaders> faders;
    std::array<yup::Label, numFaders> faderLabels;
    std::array<std::unique_ptr<yup::Slider>, numKnobs> knobs;
    std::array<yup::Label, numKnobs> knobLabels;
    yup::SwitchButton power;
    yup::Label powerLabel;

    yup::ReferenceCountedObjectPtr<RippleEffect> effect { new RippleEffect() };
    RippleEffect::State state;

    Lift rippleLift;
    yup::Point<float> lensCenter { 0.5f, 0.5f };
    std::optional<yup::Point<float>> lastRingAt;
    bool wasActive = false;
};
