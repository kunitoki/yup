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
#include "SynthFilterResponse.h"
#include "SynthTheme.h"
#include "SynthUndoHistory.h"

#include <algorithm>

SynthFilterResponse::SynthFilterResponse (const SynthFilterParameters& parametersToShow)
    : parameters (parametersToShow)
    , cutoffGesture (*this, parametersToShow.cutoff)
    , resonanceGesture (*this, parametersToShow.resonance)
{
    labelFont = SynthTheme::captionFont (10.5f);

    setOpaque (false);
    setMouseCursor (yup::MouseCursor::Hand);
    stage.prepare (sampleRate);

    shownValues = parameters.read();
    stage.configureForDisplay (shownValues);
}

bool SynthFilterResponse::syncFromParameters (SynthParameterReader modulated)
{
    // While the handle is dragged the curve follows the hand, not the modulation.
    if (cutoffGesture.isActive())
        modulated = {};

    const auto values = parameters.read (modulated);

    if (values == shownValues)
        return false;

    shownValues = values;
    stage.configureForDisplay (values);
    repaint();
    return true;
}

void SynthFilterResponse::setSampleRate (double newSampleRate)
{
    if (newSampleRate <= 0.0 || newSampleRate == sampleRate)
        return;

    sampleRate = newSampleRate;
    stage.prepare (sampleRate);
    stage.configureForDisplay (shownValues);
    repaint();
}

float SynthFilterResponse::frequencyToX (float frequency) const
{
    const auto proportion = std::log (frequency / minimumFrequency) / std::log (maximumFrequency / minimumFrequency);
    return static_cast<float> (getWidth()) * proportion;
}

float SynthFilterResponse::xToFrequency (float x) const
{
    const auto proportion = yup::jlimit (0.0f, 1.0f, x / yup::jmax (1.0f, static_cast<float> (getWidth())));
    return minimumFrequency * std::pow (maximumFrequency / minimumFrequency, proportion);
}

float SynthFilterResponse::decibelsToY (float decibels) const
{
    const auto clamped = yup::jlimit (bottomDecibels, topDecibels, decibels);
    return static_cast<float> (getHeight()) * (topDecibels - clamped) / (topDecibels - bottomDecibels);
}

float SynthFilterResponse::getDecibelsAt (float frequency) const
{
    return 20.0f * std::log10 (yup::jmax (1.0e-5f, stage.getMagnitudeResponse (frequency)));
}

std::vector<yup::Point<float>> SynthFilterResponse::getCurveVertices() const
{
    const auto width = static_cast<float> (getWidth());
    const auto decibelsAtX = [this] (float x)
    {
        return getDecibelsAt (xToFrequency (x));
    };

    std::vector<float> xs, decibels;
    xs.reserve (static_cast<std::size_t> (points + 1));
    decibels.reserve (static_cast<std::size_t> (points + 1));

    for (int point = 0; point <= points; ++point)
    {
        xs.push_back (width * static_cast<float> (point) / static_cast<float> (points));
        decibels.push_back (decibelsAtX (xs.back()));
    }

    std::vector<yup::Point<float>> vertices;
    vertices.reserve (xs.size() + xs.size() / 2);

    for (std::size_t i = 0; i < xs.size(); ++i)
    {
        vertices.emplace_back (xs[i], decibelsToY (decibels[i]));

        if (i == 0 || i + 1 >= xs.size())
            continue;

        const auto rising = decibels[i] - decibels[i - 1];
        const auto falling = decibels[i + 1] - decibels[i];

        if (rising * falling >= 0.0f)
            continue;

        // A narrow peak or notch mostly falls between two samples, so its height would jump
        // as it moves: a golden section search finds the true extreme to draw.
        const auto sign = rising > 0.0f ? 1.0f : -1.0f;
        const auto ratio = 0.5f * (std::sqrt (5.0f) - 1.0f);
        auto low = xs[i - 1];
        auto high = xs[i + 1];

        for (int iteration = 0; iteration < refineIterations; ++iteration)
        {
            const auto left = high - ratio * (high - low);
            const auto right = low + ratio * (high - low);

            if (sign * decibelsAtX (left) > sign * decibelsAtX (right))
                high = right;
            else
                low = left;
        }

        const auto x = 0.5f * (low + high);
        vertices.emplace_back (x, decibelsToY (decibelsAtX (x)));
    }

    // Close teeth, as a comb's, refine into the same gap, so the extremes are put in order.
    std::sort (vertices.begin(), vertices.end(), [] (const auto& a, const auto& b)
    {
        return a.getX() < b.getX();
    });

    return vertices;
}

void SynthFilterResponse::paint (yup::Graphics& g)
{
    const auto bounds = getLocalBounds();
    SynthTheme::paintWell (g, *this, bounds);

    const auto gridColor = SynthTheme::textPrimary (*this).withAlpha (0.06f);

    for (const auto [frequency, text] : { std::pair { 20.0f, "20" }, std::pair { 200.0f, "200" }, std::pair { 2000.0f, "2K" }, std::pair { 20000.0f, "20K" } })
    {
        const auto x = frequencyToX (frequency);
        g.setFillColor (gridColor);
        g.fillRect (x, 0.0f, 1.0f, bounds.getHeight());
        g.setFillColor (SynthTheme::textSecondary (*this));
        g.fillFittedText (text, labelFont, { yup::jlimit (4.0f, bounds.getWidth() - 34.0f, x + 3.0f), bounds.getHeight() - 15.0f, 30.0f, 12.0f }, yup::Justification::centerLeft);
    }

    g.setFillColor (gridColor);
    g.fillRect (0.0f, decibelsToY (0.0f), bounds.getWidth(), 1.0f);

    if (! shownValues.enabled)
        return;

    const auto vertices = getCurveVertices();

    // Paths share their data when copied, so the curve and the area under it are built apart.
    yup::Path curve;
    yup::Path area;
    area.moveTo (0.0f, bounds.getHeight());

    for (const auto& vertex : vertices)
    {
        if (curve.isEmpty())
            curve.moveTo (vertex);
        else
            curve.lineTo (vertex);

        area.lineTo (vertex);
    }

    area.lineTo (bounds.getWidth(), bounds.getHeight());
    area.lineTo (0.0f, bounds.getHeight());
    area.closeSubPath();

    g.setFillColor (SynthTheme::accent (*this).withAlpha (0.14f));
    g.fillPath (area);

    // A narrow peak turns the curve almost back on itself, where a miter would spike out.
    g.setStrokeColor (SynthTheme::accent (*this));
    g.setStrokeWidth (2.0f);
    g.setStrokeJoin (yup::StrokeJoin::Round);
    g.strokePath (curve);

    const auto cutoff = yup::jlimit (minimumFrequency, maximumFrequency, shownValues.cutoff);
    const auto handle = yup::Point<float> (frequencyToX (cutoff), decibelsToY (getDecibelsAt (cutoff)));

    g.setFillColor (SynthTheme::textPrimary (*this));
    g.fillEllipse ({ handle.getX() - 5.0f, handle.getY() - 5.0f, 10.0f, 10.0f });
}

void SynthFilterResponse::mouseDown (const yup::MouseEvent&)
{
    cutoffGesture.begin();
    resonanceGesture.begin();
}

void SynthFilterResponse::mouseDrag (const yup::MouseEvent& event)
{
    if (! cutoffGesture.isActive())
        return;

    const auto& cutoff = parameters.cutoff;
    cutoffGesture.setNormalized (cutoff->convertToNormalizedValue (xToFrequency (event.getPosition().getX())));

    const auto rise = (event.getLastMouseDownPosition().getY() - event.getPosition().getY()) / pixelsPerResonance;
    resonanceGesture.setNormalized (resonanceGesture.getStartValue() + rise);

    syncFromParameters();
}

void SynthFilterResponse::mouseUp (const yup::MouseEvent&)
{
    // One drag moves both parameters, and is undone as one step.
    SynthUndoHistory::beginStep (*this);
    cutoffGesture.end (true);
    resonanceGesture.end (true);
}
