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

LFODisplay::LFODisplay (std::shared_ptr<SynthWaveformShader> landscapeShader)
    : landscape (std::move (landscapeShader))
{
    gridFont = SynthTheme::captionFont (9.5f);
}

void LFODisplay::setValues (const SynthLFOValues& newValues)
{
    if (newValues == values)
        return;

    values = newValues;
    samples.clear();
    repaint();
}

void LFODisplay::setCurve (const SynthCurveState::Curve& newCurve)
{
    if (newCurve == curve)
        return;

    curve = newCurve;
    samples.clear();
    repaint();
}

void LFODisplay::setPhase (float newPhase)
{
    if (std::abs (newPhase - phase) < 1.0e-3f)
        return;

    // A wrap while playing starts a new period, which scrolls a random shape by one
    if (isRandom() && ! samples.empty() && phase >= 0.0f && newPhase >= 0.0f && newPhase < phase - 0.5f)
        appendPeriod();

    phase = newPhase;
    repaint();
}

void LFODisplay::setTempo (double bpm)
{
    if (bpm == tempo)
        return;

    tempo = bpm;
    samples.clear();
    repaint();
}

void LFODisplay::setTiming (const SynthTimeGrid::Timing& newTiming)
{
    if (newTiming == timing)
        return;

    timing = newTiming;
    repaint();
}

bool LFODisplay::isRandom() const
{
    return values.shape == yup::LFO<float>::Shape::sampleAndHold || values.shape == yup::LFO<float>::Shape::smoothRandom;
}

int LFODisplay::getPeriods() const
{
    return isRandom() ? randomPeriods : 1;
}

/** Reads the shape the way the engine reads it, at the LFO's own rate and smoothing, from
    a fixed seed. One period is run first to settle the smoother. */
void LFODisplay::rebuildSamples()
{
    const auto periods = getPeriods();
    const auto frequency = yup::jmax (0.01f, values.getFrequency (tempo));

    generator.prepare (static_cast<double> (points) * frequency / static_cast<double> (periods));
    generator.setFrequency (frequency);
    generator.setShape (values.shape);
    generator.setPhaseOffset (values.phase);
    generator.setSmoothing (values.smoothing);
    generator.setCustomCurve (&curve);
    generator.setSeed (displaySeed);
    generator.reset (0.0f);

    for (int index = 0; index < points / periods; ++index)
        generator.processSample();

    samples.resize (points + 1);

    for (auto& sample : samples)
        sample = generator.processSample();
}

void LFODisplay::appendPeriod()
{
    const auto perPeriod = static_cast<std::ptrdiff_t> (points / getPeriods());
    samples.erase (samples.begin(), samples.begin() + perPeriod);

    for (std::ptrdiff_t index = 0; index < perPeriod; ++index)
        samples.push_back (generator.processSample());
}

void LFODisplay::paint (yup::Graphics& g)
{
    const auto bounds = getLocalBounds();
    const auto area = bounds.reduced (8.0f);
    const auto hasArea = area.getWidth() > 0.0f && area.getHeight() > 0.0f;
    const auto color = SynthTheme::lfoAccent (*this);
    const auto periods = getPeriods();
    const auto polarity = values.inverted ? -1.0f : 1.0f;

    if (samples.empty())
        rebuildSamples();

    const auto toY = [&] (float value)
    {
        return area.getCenterY() - polarity * value * area.getHeight() * 0.45f;
    };

    // Reads the drawn samples between points, a proportion of the width from the area's left.
    const auto valueAt = [&] (float proportion)
    {
        const auto position = yup::jlimit (0.0f, static_cast<float> (points), proportion * static_cast<float> (points));
        const auto lower = yup::jmin (static_cast<int> (position), points - 1);
        return yup::jmap (position - static_cast<float> (lower), samples[static_cast<std::size_t> (lower)], samples[static_cast<std::size_t> (lower + 1)]);
    };

    // The marker runs through the newest period, reading the drawn samples so it carries
    // the same smoothing as the curve
    const auto shown = (static_cast<float> (periods - 1) + phase) / static_cast<float> (periods);
    const auto marker = yup::Point<float> (area.getX() + area.getWidth() * shown, toY (valueAt (shown)));

    if (hasArea && SynthLandscape::isEnabled())
    {
        landscape.setProfile (bounds.getWidth(), bounds.getHeight(), [&] (float x)
        {
            return toY (valueAt ((x - area.getX()) / area.getWidth()));
        });

        landscape.setPlayhead (phase >= 0.0f ? marker.getX() : -1.0f, trailWidth);
    }

    const auto onLandscape = hasArea && landscape.paint (g, *this, bounds, color);

    if (! onLandscape)
        SynthTheme::paintWell (g, *this, bounds);

    if (! hasArea)
        return;

    g.setFillColor (SynthTheme::textPrimary (*this).withAlpha (0.08f));
    g.fillRect (area.getX(), area.getCenterY(), area.getWidth(), 1.0f);

    SynthTimeGrid::paint (g, *this, SynthTimeGrid::getLines (timing, static_cast<float> (periods), area.getWidth()), area, gridFont, [&] (float time)
    {
        return std::optional<float> (area.getX() + area.getWidth() * time / static_cast<float> (periods));
    });

    // The landscape's ridge traces the shape, and its light marks the playing phase; the
    // line shows over it only under the mouse.
    if (onLandscape && ! hovered)
        return;

    path.clear();
    path.reserveSpace (points + 1);

    for (int index = 0; index <= points; ++index)
    {
        const auto x = area.getX() + area.getWidth() * static_cast<float> (index) / static_cast<float> (points);
        const auto y = toY (samples[static_cast<std::size_t> (index)]);

        if (index == 0)
            path.moveTo (x, y);
        else
            path.lineTo (x, y);
    }

    g.setStrokeColor (color);
    g.setStrokeWidth (2.0f);
    g.strokePath (path);

    if (phase < 0.0f || onLandscape)
        return;

    g.setFillColor (color.withAlpha (0.35f));
    g.fillEllipse (marker.getX() - 7.0f, marker.getY() - 7.0f, 14.0f, 14.0f);
    g.setFillColor (SynthTheme::textPrimary (*this));
    g.fillEllipse (marker.getX() - 3.5f, marker.getY() - 3.5f, 7.0f, 7.0f);
}

void LFODisplay::mouseEnter (const yup::MouseEvent&)
{
    hovered = true;
    repaint();
}

void LFODisplay::mouseExit (const yup::MouseEvent&)
{
    hovered = false;
    repaint();
}

void LFODisplay::refreshDisplay (double lastFrameTimeSeconds)
{
    if (isShowing() && landscape.advance (lastFrameTimeSeconds))
        repaint();
}
