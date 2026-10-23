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

#include "SynthKnob.h"
#include "SynthTheme.h"

namespace
{
constexpr float startAngle = yup::degreesToRadians (135.0f);
constexpr float sweepAngle = yup::degreesToRadians (270.0f);
} // namespace

SynthKnob::SynthKnob()
    : yup::Slider (yup::Slider::RotaryVerticalDrag)
{
    setStyle (yup::ComponentStyle::createStyle<yup::Slider> (paintKnob));
}

void SynthKnob::setShowingValue (bool shouldShowValue)
{
    if (showingValue == shouldShowValue)
        return;

    showingValue = shouldShowValue;
    repaint();
}

void SynthKnob::setShowsValueInWell (bool shouldShowValueInWell)
{
    showsValueInWell = shouldShowValueInWell;
    repaint();
}

void SynthKnob::setAccent (yup::Color newAccent)
{
    accentOverride = newAccent;
    repaint();
}

void SynthKnob::setModulationArcs (std::vector<ModulationArc> newArcs)
{
    modulationArcs = std::move (newArcs);
    repaint();
}

void SynthKnob::paintKnob (yup::Graphics& g, const yup::ApplicationTheme& theme, const yup::Slider& slider)
{
    const auto* knob = dynamic_cast<const SynthKnob*> (&slider);
    const auto bounds = slider.getLocalBounds().reduced (2.0f);
    const auto size = yup::jmin (bounds.getWidth(), bounds.getHeight());

    if (size <= 4.0f)
        return;

    const auto center = bounds.getCenter();
    const auto ringWidth = yup::jmax (2.0f, size * 0.08f);
    const auto ringRadius = size * 0.5f - ringWidth;
    const auto wellRadius = ringRadius - ringWidth * 0.9f;
    const auto value = static_cast<float> (slider.getValueNormalised());
    const auto valueAngle = startAngle + sweepAngle * value;
    const auto enabled = slider.isEnabled();
    const auto accent = ! enabled                                         ? SynthTheme::textSecondary()
                        : knob != nullptr && knob->accentOverride.has_value() ? *knob->accentOverride
                                                                              : SynthTheme::accent();

    const auto strokeArc = [&] (float radius, float from, float to, float width, yup::Color color)
    {
        yup::Path arc;
        arc.addCenteredArc (center, radius, radius, 0.0f, from, to, true);
        g.setStrokeCap (yup::StrokeCap::Round);
        g.setStrokeWidth (width);
        g.setStrokeColor (color);
        g.strokePath (arc);
    };

    // The well, then the track and the value around it.
    g.setFillColor (SynthTheme::displayBackground());
    g.fillEllipse (yup::Rectangle<float> (center.getX() - wellRadius, center.getY() - wellRadius, wellRadius * 2.0f, wellRadius * 2.0f));

    strokeArc (ringRadius, startAngle, startAngle + sweepAngle, ringWidth, SynthTheme::panelBorder());

    if (value > 0.0f)
        strokeArc (ringRadius, startAngle, valueAngle, ringWidth, accent);

    // Modulation arcs run from the value, inside the ring, one lane each.
    if (knob != nullptr)
    {
        auto radius = ringRadius - ringWidth * 1.4f;

        for (const auto& arc : knob->getModulationArcs())
        {
            const auto to = startAngle + sweepAngle * yup::jlimit (0.0f, 1.0f, value + arc.depth);

            if (std::abs (to - valueAngle) > 1.0e-3f)
                strokeArc (radius, yup::jmin (valueAngle, to), yup::jmax (valueAngle, to), ringWidth * 0.6f, arc.color);

            radius -= ringWidth * 0.8f;
        }
    }

    if (knob != nullptr && knob->showsValueInWell)
    {
        if (knob->getValueText != nullptr)
        {
            g.setFillColor (value > 0.0f ? SynthTheme::textPrimary() : SynthTheme::textSecondary());
            g.fillFittedText (knob->getValueText(), SynthTheme::valueFont (wellRadius * 0.62f),
                              yup::Rectangle<float> (center.getX() - wellRadius, center.getY() - wellRadius, wellRadius * 2.0f, wellRadius * 2.0f),
                              yup::Justification::center);
        }

        return;
    }

    // The pointer, a dot on the well's edge.
    const auto dotRadius = ringWidth * 0.55f;
    const auto dot = center.getPointOnCircumference (wellRadius - dotRadius * 1.6f, valueAngle);
    g.setFillColor (enabled ? SynthTheme::textPrimary() : SynthTheme::textSecondary());
    g.fillEllipse (yup::Rectangle<float> (dot.getX() - dotRadius, dot.getY() - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f));

    if (knob == nullptr || ! knob->isShowingValue() || knob->getValueText == nullptr)
        return;

    const auto pill = yup::Rectangle<float> (center.getX() - size * 0.48f, center.getY() - size * 0.15f, size * 0.96f, size * 0.3f);
    g.setFillColor (accent);
    g.fillRoundedRect (pill, pill.getHeight() * 0.5f);

    g.setFillColor (SynthTheme::role (yup::ThemePalette::Role::onAccent));
    g.fillFittedText (knob->getValueText(), theme.getDefaultFont().withHeight (pill.getHeight() * 0.62f), pill.reduced (3.0f, 0.0f), yup::Justification::center);
}
