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

void EnvelopeDisplay::setCurve (const SynthCurveState::Curve& newCurve)
{
    if (newCurve == curve)
        return;

    curve = newCurve;
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

    if (area.getWidth() <= 0.0f || area.getHeight() <= 0.0f || curve.getNumPoints() < 2)
        return;

    const auto length = yup::jmax (1.0e-4f, curve.getLength());
    const auto sustain = curve.getSustainIndex();
    const auto sustainTime = sustain >= 0 ? curve.getPoint (sustain).time : length;
    const auto holdWidth = sustain >= 0 ? area.getWidth() * sustainShare : 0.0f;
    const auto timedWidth = area.getWidth() - holdWidth;

    const auto toPoint = [&] (float time, float level)
    {
        const auto x = area.getX() + time / length * timedWidth + (sustain >= 0 && time > sustainTime ? holdWidth : 0.0f);
        return yup::Point<float> { x, area.getBottom() - yup::jlimit (0.0f, 1.0f, level) * area.getHeight() };
    };

    path.clear();
    path.moveTo (toPoint (0.0f, curve.getPoint (0).level));

    for (int index = 1; index < curve.getNumPoints(); ++index)
    {
        const auto& from = curve.getPoint (index - 1);
        const auto& to = curve.getPoint (index);

        for (int step = 1; step <= stepsPerSegment; ++step)
        {
            const auto t = static_cast<float> (step) / static_cast<float> (stepsPerSegment);
            const auto time = from.time + (to.time - from.time) * t;
            path.lineTo (toPoint (time, SynthCurveState::Curve::segmentValue (from.level, to.level, to.curve, t)));
        }

        if (index == sustain)
            path.lineTo (toPoint (to.time, to.level).translated (holdWidth, 0.0f));
    }

    g.setStrokeColor (SynthTheme::accent());
    g.setStrokeWidth (1.5f);
    g.strokePath (path);

    g.setFillColor (SynthTheme::accent());

    for (int index = 0; index < curve.getNumPoints(); ++index)
    {
        const auto& point = curve.getPoint (index);
        const auto centre = toPoint (point.time, point.level);

        g.fillEllipse (yup::Rectangle<float> (centre.getX() - pointRadius, centre.getY() - pointRadius, pointRadius * 2.0f, pointRadius * 2.0f));
    }
}
