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
#include "ParameterLed.h"
#include "SynthTheme.h"

ParameterLed::ParameterLed (yup::AudioParameter::Ptr toggleParameter, const yup::String& captionText)
    : gesture (*this, std::move (toggleParameter))
    , caption (captionText)
{
    setOpaque (false);
    setMouseCursor (yup::MouseCursor::Hand);
    shownOn = isOn();
}

bool ParameterLed::isOn() const
{
    const auto& parameter = gesture.getParameter();
    return parameter == nullptr || parameter->getNormalizedValue() >= 0.5f;
}

bool ParameterLed::syncFromParameter()
{
    if (isOn() == shownOn)
        return false;

    shownOn = isOn();
    repaint();
    return true;
}

void ParameterLed::paint (yup::Graphics& g)
{
    auto bounds = getLocalBounds();
    const auto led = caption.isEmpty() ? bounds.withSizeKeepingCenter (ledSize, ledSize)
                                       : bounds.removeFromRight (ledSize).withSizeKeepingCenter (ledSize, ledSize);

    if (caption.isNotEmpty())
    {
        g.setFillColor (SynthTheme::textPrimary (*this));
        g.fillFittedText (caption.toUpperCase(), SynthTheme::captionFont (12.0f), bounds.withTrimmedRight (6.0f), yup::Justification::centerRight);
    }

    const auto lit = SynthTheme::accent (*this);

    if (! isOn())
    {
        SynthTheme::paintWell (g, *this, led, 2.0f);
        return;
    }

    g.setFillColor (lit.withAlpha (0.35f));
    g.fillRoundedRect (led.reduced (-2.0f), 4.0f);
    g.setFillColor (isEnabled() ? lit : SynthTheme::textSecondary (*this));
    g.fillRoundedRect (led, 2.0f);
}

void ParameterLed::mouseDown (const yup::MouseEvent&)
{
    if (gesture.getParameter() == nullptr)
        return;

    gesture.setNormalized (isOn() ? 0.0f : 1.0f);
    syncFromParameter();
}
