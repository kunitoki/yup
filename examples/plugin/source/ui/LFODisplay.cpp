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

#include "LFODisplay.h"
#include "SynthTheme.h"

void LFODisplay::setValues (const SynthLFOValues& newValues)
{
    values = newValues;
    repaint();
}

void LFODisplay::setCurve (const SynthCurveState::Curve& newCurve)
{
    if (newCurve == curve)
        return;

    curve = newCurve;
    repaint();
}

void LFODisplay::setPhase (float newPhase)
{
    if (std::abs (newPhase - phase) < 1.0e-3f)
        return;

    phase = newPhase;
    repaint();
}

void LFODisplay::paint (yup::Graphics& g)
{
    const auto bounds = getLocalBounds();

    SynthTheme::paintWell (g, bounds);

    const auto area = bounds.reduced (8.0f);

    if (area.getWidth() <= 0.0f || area.getHeight() <= 0.0f)
        return;

    g.setFillColor (SynthTheme::textPrimary().withAlpha (0.08f));
    g.fillRect (area.getX(), area.getCenterY(), area.getWidth(), 1.0f);

    // One period at one sample per point: the shape is read the way the engine reads it.
    yup::LFO<float> shape;
    shape.prepare (static_cast<double> (points));
    shape.setFrequency (1.0f);
    shape.setShape (values.shape);
    shape.setPhaseOffset (values.phase);
    shape.setCustomCurve (&curve);

    path.clear();
    path.reserveSpace (points + 1);

    for (int index = 0; index <= points; ++index)
    {
        const auto x = area.getX() + area.getWidth() * static_cast<float> (index) / static_cast<float> (points);
        const auto y = area.getCenterY() - shape.processSample() * area.getHeight() * 0.45f;

        if (index == 0)
            path.moveTo (x, y);
        else
            path.lineTo (x, y);
    }

    g.setStrokeColor (SynthTheme::lfoAccent());
    g.setStrokeWidth (2.0f);
    g.strokePath (path);

    shape.reset (phase);

    const auto markerX = area.getX() + area.getWidth() * phase;
    const auto markerY = area.getCenterY() - shape.getValue() * area.getHeight() * 0.45f;

    g.setFillColor (SynthTheme::textPrimary());
    g.fillEllipse (yup::Rectangle<float> (markerX - 3.0f, markerY - 3.0f, 6.0f, 6.0f));
}
