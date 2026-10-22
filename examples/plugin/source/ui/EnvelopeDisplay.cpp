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

#include "EnvelopeDisplay.h"
#include "SynthTheme.h"

void EnvelopeDisplay::setValues (const SynthEnvelopeValues& newValues)
{
    values = newValues;
    repaint();
}

void EnvelopeDisplay::paint (yup::Graphics& g)
{
    const auto bounds = getLocalBounds();

    g.setFillColor (SynthTheme::displayBackground());
    g.fillRoundedRect (bounds, 4.0f);

    g.setStrokeColor (SynthTheme::panelBorder());
    g.setStrokeWidth (1.0f);
    g.strokeRoundedRect (bounds.reduced (0.5f), 4.0f);

    const auto area = bounds.reduced (8.0f);

    if (area.getWidth() <= 0.0f || area.getHeight() <= 0.0f)
        return;

    // The sustain stage has no duration of its own, so it is given a fixed share of
    // the width and the timed stages share what is left.
    const auto totalSeconds = yup::jmax (1.0e-4f, values.delay + values.attack + values.hold + values.decay + values.release);
    const auto timedWidth = area.getWidth() * (1.0f - sustainShare);
    const auto secondsToPixels = timedWidth / totalSeconds;

    const auto levelToY = [area] (float level)
    {
        return area.getBottom() - yup::jlimit (0.0f, 1.0f, level) * area.getHeight();
    };

    constexpr int numPoints = 7;

    const yup::Point<float> points[numPoints] = {
        { area.getX(), levelToY (0.0f) },
        { area.getX() + values.delay * secondsToPixels, levelToY (0.0f) },
        { area.getX() + (values.delay + values.attack) * secondsToPixels, levelToY (1.0f) },
        { area.getX() + (values.delay + values.attack + values.hold) * secondsToPixels, levelToY (1.0f) },
        { area.getX() + (values.delay + values.attack + values.hold + values.decay) * secondsToPixels, levelToY (values.sustain) },
        { area.getX() + (values.delay + values.attack + values.hold + values.decay) * secondsToPixels + area.getWidth() * sustainShare, levelToY (values.sustain) },
        { area.getRight(), levelToY (0.0f) }
    };

    path.clear();
    path.reserveSpace (numPoints + 2);
    path.moveTo (points[0]);

    for (int index = 1; index < numPoints; ++index)
        path.lineTo (points[index]);

    g.setStrokeColor (SynthTheme::accent());
    g.setStrokeWidth (1.5f);
    g.strokePath (path);

    for (int index = 1; index < numPoints - 1; ++index)
    {
        g.setFillColor (SynthTheme::accent());
        g.fillEllipse (yup::Rectangle<float> (points[index].getX() - pointRadius,
                                              points[index].getY() - pointRadius,
                                              pointRadius * 2.0f,
                                              pointRadius * 2.0f));
    }
}
