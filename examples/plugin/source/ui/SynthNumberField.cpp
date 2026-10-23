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
    setOpaque (false);
    syncFromParameter();
}

void SynthNumberField::setParameter (yup::AudioParameter::Ptr newParameter)
{
    if (newParameter == gesture.getParameter())
        return;

    gesture.setParameter (std::move (newParameter));
    syncFromParameter();
}

bool SynthNumberField::syncFromParameter()
{
    const auto text = gesture.getParameter()->toString();

    if (text == shownText)
        return false;

    shownText = text;
    repaint();
    return true;
}

void SynthNumberField::paint (yup::Graphics& g)
{
    const auto bounds = getLocalBounds();
    SynthTheme::paintWell (g, *this, bounds);

    auto content = bounds.reduced (7.0f, 0.0f);
    const auto height = yup::jmin (13.5f, bounds.getHeight() * 0.62f);

    if (caption.isNotEmpty())
    {
        const auto font = SynthTheme::captionFont (height * 0.85f);
        const auto captionWidth = yup::jmin (content.getWidth() * 0.5f, SynthTheme::textWidth (caption, font) + 6.0f);

        g.setFillColor (SynthTheme::textSecondary (*this));
        g.fillFittedText (caption, font, content.removeFromLeft (captionWidth), yup::Justification::centerLeft);
    }

    g.setFillColor (isEnabled() ? SynthTheme::accent (*this) : SynthTheme::textSecondary (*this));
    g.fillFittedText (shownText, SynthTheme::valueFont (height), content, caption.isNotEmpty() ? yup::Justification::centerRight : yup::Justification::center);
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
