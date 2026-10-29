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

void LFODisplay::setTempo (double bpm)
{
    if (bpm == tempo)
        return;

    tempo = bpm;
    repaint();
}

void LFODisplay::setTiming (const SynthTimeGrid::Timing& newTiming)
{
    if (newTiming == timing)
        return;

    timing = newTiming;
    repaint();
}

void LFODisplay::paint (yup::Graphics& g)
{
    const auto bounds = getLocalBounds();

    SynthTheme::paintWell (g, *this, bounds);

    const auto area = bounds.reduced (8.0f);

    if (area.getWidth() <= 0.0f || area.getHeight() <= 0.0f)
        return;

    g.setFillColor (SynthTheme::textPrimary (*this).withAlpha (0.08f));
    g.fillRect (area.getX(), area.getCenterY(), area.getWidth(), 1.0f);

    // The shape is read the way the engine reads it, at the LFO's own rate and smoothing,
    // from a fixed seed so the random shapes hold still. Random shapes draw a new value per
    // period, so they show several periods; one period is run first to settle the smoother.
    const auto random = values.shape == yup::LFO<float>::Shape::sampleAndHold || values.shape == yup::LFO<float>::Shape::smoothRandom;
    const auto periods = random ? randomPeriods : 1;
    const auto frequency = yup::jmax (0.01f, values.getFrequency (tempo > 0.0 ? tempo : displayTempo));
    const auto polarity = values.inverted ? -1.0f : 1.0f;

    SynthTimeGrid::paint (g, *this, SynthTimeGrid::getLines (timing, static_cast<float> (periods), area.getWidth()), area, [&] (float time)
    {
        return std::optional<float> (area.getX() + area.getWidth() * time / static_cast<float> (periods));
    });

    yup::LFO<float> shape;
    shape.prepare (static_cast<double> (points) * frequency / static_cast<double> (periods));
    shape.setFrequency (frequency);
    shape.setShape (values.shape);
    shape.setPhaseOffset (values.phase);
    shape.setSmoothing (values.smoothing);
    shape.setCustomCurve (&curve);
    shape.setSeed (displaySeed);
    shape.reset (0.0f);

    for (int index = 0; index < points / periods; ++index)
        shape.processSample();

    path.clear();
    path.reserveSpace (points + 1);

    for (int index = 0; index <= points; ++index)
    {
        const auto x = area.getX() + area.getWidth() * static_cast<float> (index) / static_cast<float> (points);
        const auto y = area.getCenterY() - polarity * shape.processSample() * area.getHeight() * 0.45f;

        if (index == 0)
            path.moveTo (x, y);
        else
            path.lineTo (x, y);
    }

    const auto color = SynthTheme::lfoAccent (*this);
    const auto playing = ! random && phase >= 0.0f;
    const auto markerX = area.getX() + area.getWidth() * phase;

    g.setStrokeColor (color);
    g.setStrokeWidth (2.0f);
    g.strokePath (path);

    if (! playing)
        return;

    shape.reset (phase);
    const auto markerY = area.getCenterY() - polarity * shape.getValue() * area.getHeight() * 0.45f;

    g.setFillColor (color.withAlpha (0.35f));
    g.fillEllipse (markerX - 7.0f, markerY - 7.0f, 14.0f, 14.0f);
    g.setFillColor (SynthTheme::textPrimary (*this));
    g.fillEllipse (markerX - 3.5f, markerY - 3.5f, 7.0f, 7.0f);
}
