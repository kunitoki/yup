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
#include "SynthNumberField.h"
#include "SynthTheme.h"

SynthNumberField::SynthNumberField (const yup::String& captionText, yup::AudioParameter::Ptr parameterToControl)
    : gesture (*this, std::move (parameterToControl))
    , caption (captionText)
{
    captionFont = SynthTheme::captionFont (11.5f);
    valueFont = SynthTheme::valueFont (13.5f);

    setOpaque (false);
    setMouseCursor (yup::MouseCursor::Hand);
    syncFromParameter();
}

void SynthNumberField::setParameter (yup::AudioParameter::Ptr newParameter)
{
    if (newParameter == gesture.getParameter())
        return;

    gesture.setParameter (std::move (newParameter));
    shownValue.reset();
    syncFromParameter();
}

bool SynthNumberField::syncFromParameter()
{
    const auto& parameter = gesture.getParameter();

    // Formatting is costly at the interface rate, so only a new value is formatted.
    if (const auto value = parameter->getValue(); std::exchange (shownValue, value) == value)
        return false;

    const auto text = parameter->toString();

    if (text == shownText)
        return false;

    shownText = text;
    repaint();
    return true;
}

void SynthNumberField::showModulation (std::vector<SynthKnob::ModulationArc> arcs, std::optional<float> newModulatedValue)
{
    const auto sameValue = newModulatedValue.has_value() == modulatedValue.has_value()
                        && (! newModulatedValue.has_value() || std::abs (*newModulatedValue - *modulatedValue) < 1.0e-4f);

    if (arcs == modulationArcs && sameValue)
        return;

    modulationArcs = std::move (arcs);
    modulatedValue = newModulatedValue;
    repaint();
}

void SynthNumberField::paint (yup::Graphics& g)
{
    const auto bounds = getLocalBounds();
    SynthTheme::paintWell (g, *this, bounds);

    auto content = bounds.reduced (7.0f, 0.0f);
    const auto height = yup::jmin (13.5f, bounds.getHeight() * 0.62f);

    if (caption.isNotEmpty())
    {
        const auto font = captionFont.withHeight (height * 0.85f);
        const auto captionWidth = yup::jmin (content.getWidth() * 0.5f, SynthTheme::textWidth (caption, font) + 6.0f);

        g.setFillColor (SynthTheme::textSecondary (*this));
        g.fillFittedText (caption, font, content.removeFromLeft (captionWidth), yup::Justification::centerLeft);
    }

    const auto& parameter = gesture.getParameter();
    const auto text = modulatedValue.has_value() ? parameter->convertToString (parameter->convertToDenormalizedValue (*modulatedValue)) : shownText;
    const auto color = ! isEnabled()                 ? SynthTheme::textSecondary (*this)
                       : modulatedValue.has_value() ? SynthTheme::textPrimary (*this)
                                                    : SynthTheme::accent (*this);

    g.setFillColor (color);
    g.fillFittedText (text, valueFont.withHeight (height), content, caption.isNotEmpty() ? yup::Justification::centerRight : yup::Justification::center);

    // Route ranges along the bottom edge, across the parameter's whole range.
    const auto track = bounds.reduced (3.0f, 0.0f);
    const auto positionOf = [&track] (float normalized) { return track.getX() + track.getWidth() * yup::jlimit (0.0f, 1.0f, normalized); };
    const auto value = parameter->getNormalizedValue();
    auto lane = bounds.getBottom() - 3.0f;

    for (const auto& arc : modulationArcs)
    {
        const auto from = positionOf (arc.bipolar ? value - arc.depth : value);
        const auto to = positionOf (value + arc.depth);
        g.setFillColor (arc.color);
        g.fillRect (yup::jmin (from, to), lane, yup::jmax (1.0f, std::abs (to - from)), 1.5f);
        lane -= 2.0f;
    }

    if (modulatedValue.has_value())
    {
        g.setFillColor (SynthTheme::textPrimary (*this));
        g.fillRect (positionOf (*modulatedValue) - 1.0f, lane + 0.5f, 2.0f, bounds.getBottom() - lane - 1.5f);
    }
}

void SynthNumberField::mouseDown (const yup::MouseEvent&)
{
    gesture.begin();
}

void SynthNumberField::mouseDrag (const yup::MouseEvent& event)
{
    if (! gesture.isActive())
        return;

    const auto travel = event.getLastMouseDownPosition().getY() - event.getPosition().getY();
    const auto scale = event.getModifiers().isShiftDown() ? 0.1f : 1.0f;

    gesture.setNormalized (gesture.getStartValue() + travel / pixelsPerRange * scale);
    syncFromParameter();
}

void SynthNumberField::mouseUp (const yup::MouseEvent&)
{
    gesture.end();
}

void SynthNumberField::mouseDoubleClick (const yup::MouseEvent&)
{
    gesture.resetToDefault();
    syncFromParameter();
}
