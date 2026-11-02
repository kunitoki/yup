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
    wellFont = SynthTheme::valueFont (13.5f);
}

void SynthKnob::setSourceAccent (int family)
{
    accentFamily = family;
    repaint();
}

void SynthKnob::setModulationArcs (std::vector<ModulationArc> newArcs)
{
    if (newArcs == modulationArcs)
        return;

    modulationArcs = std::move (newArcs);
    repaint();
}

void SynthKnob::setModulatedValue (std::optional<float> newModulatedValue)
{
    if (newModulatedValue.has_value() == modulatedValue.has_value()
        && (! newModulatedValue.has_value() || std::abs (*newModulatedValue - *modulatedValue) < 1.0e-4f))
        return;

    modulatedValue = newModulatedValue;
    repaint();
}

void SynthKnob::mouseEnter (const yup::MouseEvent& event)
{
    yup::Slider::mouseEnter (event);

    if (onHoverChanged != nullptr)
        onHoverChanged();
}

void SynthKnob::mouseExit (const yup::MouseEvent& event)
{
    yup::Slider::mouseExit (event);

    if (onHoverChanged != nullptr)
        onHoverChanged();
}

void SynthKnob::mouseDown (const yup::MouseEvent& event)
{
    if (event.isRightButtonDown() && onContextClick != nullptr)
    {
        onContextClick();
        return;
    }

    if (isResetClick (event))
    {
        setValue (getDefaultValue());
        return;
    }

    yup::Slider::mouseDown (event);

    dragAnchor = event.getPosition();
    dragAnchorValue = getValueNormalised();
    fineDrag = event.getModifiers().isShiftDown();
}

void SynthKnob::mouseDrag (const yup::MouseEvent& event)
{
    if (! isCurrentlyBeingDragged())
        return;

    // Shift pressed or released mid drag carries on from where the value is, without a jump.
    if (const auto fine = event.getModifiers().isShiftDown(); fine != fineDrag)
    {
        dragAnchor = event.getPosition();
        dragAnchorValue = getValueNormalised();
        fineDrag = fine;
    }

    const auto height = yup::jmax (1.0f, getHeight());
    const auto travel = static_cast<double> ((dragAnchor.getY() - event.getPosition().getY()) / height);

    setValueNormalised (dragAnchorValue + travel * getMouseDragSensitivity() * (fineDrag ? fineDragScale : 1.0));
}

void SynthKnob::mouseUp (const yup::MouseEvent& event)
{
    yup::Slider::mouseUp (event);

    if (onHoverChanged != nullptr)
        onHoverChanged();
}

void SynthKnob::mouseDoubleClick (const yup::MouseEvent& event)
{
    if (onDoubleClick == nullptr)
        yup::Slider::mouseDoubleClick (event);
    else if (isEnabled())
        onDoubleClick();
}

bool SynthKnob::isResetClick (const yup::MouseEvent& event)
{
    const auto modifiers = event.getModifiers();
    return event.isLeftButtonDown() && (modifiers.isCommandDown() || modifiers.isControlDown());
}

void SynthKnob::paintKnob (yup::Graphics& g, const yup::ApplicationTheme&, const yup::Slider& slider)
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
    const auto accent = ! enabled                                         ? SynthTheme::textSecondary (slider)
                        : knob != nullptr && knob->accentFamily.has_value() ? SynthTheme::sourceAccent (slider, *knob->accentFamily)
                                                                              : SynthTheme::accent (slider);

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
    g.setFillColor (SynthTheme::wellColor (slider));
    g.fillEllipse (yup::Rectangle<float> (center.getX() - wellRadius, center.getY() - wellRadius, wellRadius * 2.0f, wellRadius * 2.0f));

    strokeArc (ringRadius, startAngle, startAngle + sweepAngle, ringWidth, SynthTheme::panelBorder (slider));

    if (value > 0.0f)
        strokeArc (ringRadius, startAngle, valueAngle, ringWidth, accent);

    // Modulation arcs run from the value, inside the ring, one lane each.
    if (knob != nullptr)
    {
        auto radius = ringRadius - ringWidth * 1.4f;

        for (const auto& arc : knob->getModulationArcs())
        {
            const auto angleOf = [&] (float normalized) { return startAngle + sweepAngle * yup::jlimit (0.0f, 1.0f, normalized); };
            const auto from = arc.bipolar ? angleOf (value - arc.depth) : valueAngle;
            const auto to = angleOf (value + arc.depth);

            if (std::abs (to - from) > 1.0e-3f)
                strokeArc (radius, yup::jmin (from, to), yup::jmax (from, to), ringWidth * 0.6f, arc.color);

            radius -= ringWidth * 0.8f;
        }

        // The value the modulation plays now, a bright tick across the ring.
        if (knob->modulatedValue.has_value())
        {
            const auto angle = startAngle + sweepAngle * yup::jlimit (0.0f, 1.0f, *knob->modulatedValue);
            strokeArc (ringRadius, angle - 0.06f, angle + 0.06f, ringWidth * 1.6f, SynthTheme::textPrimary (slider));
        }
    }

    if (knob != nullptr && knob->getWellText != nullptr)
    {
        g.setFillColor (value > 0.0f ? SynthTheme::textPrimary (slider) : SynthTheme::textSecondary (slider));
        g.fillFittedText (knob->getWellText(), knob->wellFont.withHeight (wellRadius * 0.62f),
                          yup::Rectangle<float> (center.getX() - wellRadius, center.getY() - wellRadius, wellRadius * 2.0f, wellRadius * 2.0f),
                          yup::Justification::center);
        return;
    }

    // The pointer, a dot on the well's edge.
    const auto dotRadius = ringWidth * 0.55f;
    const auto dot = center.getPointOnCircumference (wellRadius - dotRadius * 1.6f, valueAngle);
    g.setFillColor (enabled ? SynthTheme::textPrimary (slider) : SynthTheme::textSecondary (slider));
    g.fillEllipse (yup::Rectangle<float> (dot.getX() - dotRadius, dot.getY() - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f));
}
