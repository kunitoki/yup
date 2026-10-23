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
#include "SynthKeyboardLook.h"
#include "SynthTheme.h"

namespace SynthKeyboardLook
{
namespace
{
yup::ColorGradient makeGradient (yup::Point<float> from, yup::Point<float> to, std::initializer_list<std::pair<float, yup::Color>> stops)
{
    yup::ColorGradient gradient;

    for (const auto& [position, color] : stops)
        gradient.addColorStop (color, from + (to - from) * position, position);

    return gradient;
}

void paintWhiteKey (yup::Graphics& g, yup::Rectangle<float> key, bool pressed, bool over)
{
    const auto accent = SynthTheme::accent();
    const auto tint = [&] (yup::Color color) { return pressed ? color.mixedWith (accent, 0.55f, yup::ColorSpace::SRGB) : (over ? color.mixedWith (accent, 0.12f, yup::ColorSpace::SRGB) : color); };
    const auto lip = pressed ? 3.0f : 7.0f;
    const auto body = key.reduced (0.5f, 0.0f);

    g.setFillColor (yup::Colors::black.withAlpha (0.6f));
    g.fillRoundedRect (body.withTrimmedTop (2.0f), 0.0f, 0.0f, 3.0f, 3.0f);

    g.setFillColorGradient (makeGradient (body.getTopLeft(), body.getBottomLeft(), { { 0.0f, tint (yup::Color (0xffffffff)) }, { 0.7f, tint (yup::Color (0xffeef1f4)) }, { 1.0f, tint (yup::Color (0xffd3d9df)) } }));
    g.fillRoundedRect (body.withTrimmedBottom (1.0f), 0.0f, 0.0f, 3.0f, 3.0f);

    const auto front = body.withTrimmedBottom (1.0f).withTrimmedTop (body.getHeight() - 1.0f - lip);
    g.setFillColor (tint (yup::Color (0xffb8c1ca)));
    g.fillRoundedRect (front, 0.0f, 0.0f, 3.0f, 3.0f);
    g.setFillColor (tint (yup::Color (0xffe9edf1)));
    g.fillRect (front.getX(), front.getY(), front.getWidth(), 1.0f);

    g.setFillColor (yup::Colors::black.withAlpha (0.12f));
    g.fillRect (body.getRight() - 1.0f, body.getY(), 1.0f, body.getHeight() - lip);
}

void paintBlackKey (yup::Graphics& g, yup::Rectangle<float> key, bool pressed, bool over)
{
    const auto accent = SynthTheme::accent();
    const auto tint = [&] (yup::Color color) { return pressed ? color.mixedWith (accent, 0.5f, yup::ColorSpace::SRGB) : (over ? color.mixedWith (accent, 0.15f, yup::ColorSpace::SRGB) : color); };

    g.setFillColor (yup::Colors::black.withAlpha (0.55f));
    g.fillRoundedRect (key.withY (key.getY() + 1.0f).withHeight (key.getHeight() + 2.0f), 0.0f, 0.0f, 3.0f, 3.0f);

    g.setFillColorGradient (makeGradient (key.getTopLeft(), key.getTopRight(), { { 0.0f, tint (yup::Color (0xff0c0f13)) }, { 0.45f, tint (yup::Color (0xff2a3139)) }, { 1.0f, tint (yup::Color (0xff12171c)) } }));
    g.fillRoundedRect (key, 0.0f, 0.0f, 2.5f, 2.5f);

    const auto face = key.reduced (key.getWidth() * 0.14f, 0.0f).withTrimmedTop (key.getHeight() * (pressed ? 0.82f : 0.72f)).withTrimmedBottom (key.getHeight() * 0.06f);
    g.setFillColorGradient (makeGradient (face.getTopLeft(), face.getBottomLeft(), { { 0.0f, tint (yup::Color (0xff2c343c)) }, { 1.0f, tint (yup::Color (0xff4a545e)) } }));
    g.fillRoundedRect (face, 0.0f, 0.0f, 2.0f, 2.0f);
}

void paintKeyboard (yup::Graphics& g, const yup::ApplicationTheme&, const yup::MidiKeyboardComponent& keyboard)
{
    const auto bounds = keyboard.getLocalBounds();

    if (bounds.isEmpty())
        return;

    const auto keyWidth = keyboard.getKeyWidth();

    for (const auto black : { false, true })
    {
        for (int note = keyboard.getLowestVisibleKey(); note <= keyboard.getHighestVisibleKey(); ++note)
        {
            if (keyboard.isBlackKey (note) != black)
                continue;

            auto isBlack = false;
            yup::Rectangle<float> key;
            keyboard.getKeyPosition (note, keyWidth, key, isBlack);

            if (black)
                paintBlackKey (g, key, keyboard.isNoteOn (note), keyboard.isMouseOverNote (note));
            else
                paintWhiteKey (g, key, keyboard.isNoteOn (note), keyboard.isMouseOverNote (note));
        }
    }

    g.setFillColorGradient (makeGradient (bounds.getTopLeft(), bounds.getTopLeft().translated (0.0f, 6.0f), { { 0.0f, yup::Colors::black.withAlpha (0.7f) }, { 1.0f, yup::Colors::black.withAlpha (0.0f) } }));
    g.fillRect (bounds.withHeight (6.0f));
}

void paintWheel (yup::Graphics& g, yup::Rectangle<float> bounds, float normalized, bool active)
{
    if (bounds.isEmpty())
        return;

    SynthTheme::paintWell (g, bounds, 7.0f);

    const auto cylinder = bounds.reduced (bounds.getWidth() * 0.2f, 5.0f);
    const auto corner = cylinder.getWidth() * 0.25f;

    g.setFillColorGradient (makeGradient (cylinder.getTopLeft(), cylinder.getBottomLeft(),
                                          { { 0.0f, yup::Color (0xff05080b) }, { 0.22f, yup::Color (0xff232b33) }, { 0.5f, yup::Color (0xff3a444e) }, { 0.78f, yup::Color (0xff232b33) }, { 1.0f, yup::Color (0xff05080b) } }));
    g.fillRoundedRect (cylinder, corner);

    const auto travelTop = cylinder.getY() + 4.0f;
    const auto travel = cylinder.getHeight() - 8.0f;
    const auto markerY = travelTop + (1.0f - yup::jlimit (0.0f, 1.0f, normalized)) * travel;
    const auto spacing = 3.6f;
    const auto roll = std::fmod (markerY - cylinder.getY(), spacing);

    for (auto y = cylinder.getY() + roll; y < cylinder.getBottom(); y += spacing)
    {
        const auto distance = std::abs (y - cylinder.getCenterY()) / (cylinder.getHeight() * 0.5f);
        g.setFillColor (yup::Colors::black.withAlpha (0.55f * (1.0f - distance * 0.6f)));
        g.fillRect (cylinder.getX() + 1.0f, y, cylinder.getWidth() - 2.0f, 1.0f);
        g.setFillColor (yup::Colors::white.withAlpha (0.07f * (1.0f - distance)));
        g.fillRect (cylinder.getX() + 1.0f, y + 1.0f, cylinder.getWidth() - 2.0f, 1.0f);
    }

    const auto edge = 1.0f - std::abs (markerY - cylinder.getCenterY()) / (cylinder.getHeight() * 0.5f);
    g.setFillColor (yup::Colors::black.withAlpha (0.5f));
    g.fillRect (cylinder.getX(), markerY - 3.0f, cylinder.getWidth(), 6.0f);
    g.setFillColor ((active ? SynthTheme::accent() : yup::Color (0xffdfe7ee)).withAlpha (0.35f + 0.65f * edge));
    g.fillRect (cylinder.getX(), markerY - 1.5f, cylinder.getWidth(), 3.0f);

    g.setFillColorGradient (makeGradient (cylinder.getTopLeft(), cylinder.getTopRight(),
                                          { { 0.0f, yup::Colors::black.withAlpha (0.55f) }, { 0.3f, yup::Colors::black.withAlpha (0.0f) }, { 0.7f, yup::Colors::black.withAlpha (0.0f) }, { 1.0f, yup::Colors::black.withAlpha (0.6f) } }));
    g.fillRoundedRect (cylinder, corner);
}
} // namespace

void apply (yup::MidiKeyboardComponent& keyboard)
{
    keyboard.setStyle (yup::ComponentStyle::createStyle<yup::MidiKeyboardComponent> (paintKeyboard));
}

void apply (yup::PitchWheelComponent& wheel)
{
    wheel.setStyle (yup::ComponentStyle::createStyle<yup::PitchWheelComponent> ([] (yup::Graphics& g, const yup::ApplicationTheme&, const yup::PitchWheelComponent& w)
    {
        paintWheel (g, w.getLocalBounds(), static_cast<float> ((w.getValue() + 1.0) * 0.5), w.isCurrentlyBeingDragged());
    }));
}

void apply (yup::ModWheelComponent& wheel)
{
    wheel.setStyle (yup::ComponentStyle::createStyle<yup::ModWheelComponent> ([] (yup::Graphics& g, const yup::ApplicationTheme&, const yup::ModWheelComponent& w)
    {
        paintWheel (g, w.getLocalBounds(), static_cast<float> (w.getValue()), w.isCurrentlyBeingDragged());
    }));
}
} // namespace SynthKeyboardLook
