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
#include "SynthSpinner.h"
#include "SynthTheme.h"

SynthSpinner::SynthSpinner (yup::AudioParameter::Ptr steppedParameter)
    : gesture (*this, std::move (steppedParameter))
{
    setOpaque (false);
    setMouseCursor (yup::MouseCursor::Hand);
    syncFromParameter();
}

bool SynthSpinner::syncFromParameter()
{
    const auto text = gesture.getParameter()->toString();

    if (text == shownText)
        return false;

    shownText = text;
    repaint();
    return true;
}

void SynthSpinner::paint (yup::Graphics& g)
{
    auto bounds = getLocalBounds();
    SynthTheme::paintWell (g, *this, bounds);

    auto arrows = bounds.removeFromRight (arrowWidth).reduced (2.0f, 4.0f);
    const auto color = isEnabled() ? SynthTheme::textSecondary (*this) : SynthTheme::textSecondary (*this).withAlpha (0.5f);

    for (const auto up : { true, false })
    {
        const auto box = (up ? arrows.removeFromTop (arrows.getHeight() * 0.5f) : arrows).withSizeKeepingCenter (6.0f, 3.5f);

        yup::Path triangle;
        triangle.moveTo (box.getX(), up ? box.getBottom() : box.getY());
        triangle.lineTo (box.getRight(), up ? box.getBottom() : box.getY());
        triangle.lineTo (box.getCenterX(), up ? box.getY() : box.getBottom());
        triangle.closeSubPath();

        g.setFillColor (color);
        g.fillPath (triangle);
    }

    g.setFillColor (isEnabled() ? SynthTheme::accent (*this) : SynthTheme::textSecondary (*this));
    g.fillFittedText (shownText, SynthTheme::valueFont (yup::jmin (13.5f, bounds.getHeight() * 0.65f)), bounds.reduced (4.0f, 0.0f), yup::Justification::center);
}

void SynthSpinner::stepTo (float value)
{
    const auto& parameter = gesture.getParameter();
    const auto clamped = yup::jlimit (parameter->getMinimumValue(), parameter->getMaximumValue(), static_cast<float> (yup::roundToInt (value)));

    gesture.setNormalized (parameter->convertToNormalizedValue (clamped));
    syncFromParameter();
}

void SynthSpinner::mouseDown (const yup::MouseEvent&)
{
    dragStartValue = gesture.getParameter()->getValue();
    dragged = false;
    gesture.begin();
}

void SynthSpinner::mouseDrag (const yup::MouseEvent& event)
{
    const auto steps = std::trunc ((event.getLastMouseDownPosition().getY() - event.getPosition().getY()) / pixelsPerStep);

    if (steps == 0.0f && ! dragged)
        return;

    dragged = true;
    stepTo (dragStartValue + steps);
}

void SynthSpinner::mouseUp (const yup::MouseEvent& event)
{
    gesture.end();

    if (dragged)
        return;

    const auto arrows = getLocalBounds().removeFromRight (arrowWidth + 4.0f);
    const auto position = event.getPosition();

    if (arrows.contains (position))
        stepTo (dragStartValue + (position.getY() < arrows.getCenterY() ? 1.0f : -1.0f));
}
