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
#include <iterator>

SynthFilterResponse::SynthFilterResponse (const SynthFilterParameters& parametersToShow, std::shared_ptr<SynthWaveformShader> landscapeShader)
    : parameters (parametersToShow)
    , cutoffGesture (*this, parametersToShow.cutoff)
    , resonanceGesture (*this, parametersToShow.resonance)
    , landscape (std::move (landscapeShader))
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

float SynthFilterResponse::findExtremeFrequency (float lowFrequency, float highFrequency, bool peak) const
{
    const auto sign = peak ? 1.0f : -1.0f;
    const auto ratio = 0.5f * (std::sqrt (5.0f) - 1.0f);

    for (int iteration = 0; iteration < refineIterations; ++iteration)
    {
        const auto left = highFrequency - ratio * (highFrequency - lowFrequency);
        const auto right = lowFrequency + ratio * (highFrequency - lowFrequency);

        if (sign * stage.getMagnitudeResponse (left) > sign * stage.getMagnitudeResponse (right))
            highFrequency = right;
        else
            lowFrequency = left;
    }

    return 0.5f * (lowFrequency + highFrequency);
}

std::vector<yup::Point<float>> SynthFilterResponse::getCurveVertices() const
{
    const auto width = static_cast<float> (getWidth());
    const auto spacing = stage.getToothSpacing();

    // A comb's teeth are found exactly while far apart, drawn from their envelopes once a few
    // pixels apart, and alternate between those envelopes on every pixel once closer still.
    const auto pixelsPerLog = width / std::log (maximumFrequency / minimumFrequency);
    const auto frequencyWhereTeethAre = [&] (float pixelsApart)
    {
        return spacing > 0.0f ? spacing / std::expm1 (pixelsApart / pixelsPerLog) : 2.0f * maximumFrequency;
    };

    const auto exactBelow = frequencyWhereTeethAre (exactToothPixels);
    const auto teethBelow = frequencyWhereTeethAre (denseToothPixels);

    std::vector<yup::Point<float>> vertices;
    vertices.reserve (static_cast<std::size_t> (2 * points) + static_cast<std::size_t> (width));

    const auto addVertex = [&] (float frequency, float magnitude)
    {
        if (frequency >= 0.999f * minimumFrequency && frequency <= 1.001f * maximumFrequency)
            vertices.emplace_back (frequencyToX (frequency), decibelsToY (20.0f * std::log10 (yup::jmax (1.0e-5f, magnitude))));
    };

    std::vector<float> frequencies, magnitudes;
    frequencies.reserve (static_cast<std::size_t> (points + 1));
    magnitudes.reserve (static_cast<std::size_t> (points + 1));

    for (int point = 0; point <= points; ++point)
    {
        const auto frequency = xToFrequency (width * static_cast<float> (point) / static_cast<float> (points));

        if (frequency >= exactBelow)
            break;

        frequencies.push_back (frequency);
        magnitudes.push_back (stage.getMagnitudeResponse (frequency));
    }

    for (std::size_t i = 0; i < frequencies.size(); ++i)
    {
        addVertex (frequencies[i], magnitudes[i]);

        if (spacing > 0.0f || i == 0 || i + 1 >= frequencies.size())
            continue;

        const auto rising = magnitudes[i] - magnitudes[i - 1];
        const auto falling = magnitudes[i + 1] - magnitudes[i];

        // A narrow peak or notch mostly falls between two points, so its height would jump
        // as it moves: the true extreme is found and drawn too.
        if (rising * falling < 0.0f)
        {
            const auto extreme = findExtremeFrequency (frequencies[i - 1], frequencies[i + 1], rising > 0.0f);
            addVertex (extreme, stage.getMagnitudeResponse (extreme));
        }
    }

    // A tooth peaks once around each multiple of the spacing, with a trough between.
    for (auto tooth = 0.0f; spacing > 0.0f && tooth * spacing < yup::jmin (teethBelow, maximumFrequency); tooth += 1.0f)
    {
        const auto top = tooth * spacing;
        const auto bottom = top + 0.5f * spacing;

        if (top < exactBelow)
        {
            if (tooth > 0.0f)
            {
                const auto peak = findExtremeFrequency (top - 0.5f * spacing, bottom, true);
                addVertex (peak, stage.getMagnitudeResponse (peak));
            }

            const auto trough = findExtremeFrequency (top, top + spacing, false);
            addVertex (trough, stage.getMagnitudeResponse (trough));
        }
        else
        {
            addVertex (top, stage.getToothMagnitudeRange (top).getEnd());
            addVertex (bottom, stage.getToothMagnitudeRange (bottom).getStart());
        }
    }

    for (auto x = std::ceil (yup::jmax (0.0f, frequencyToX (teethBelow))); spacing > 0.0f && teethBelow < maximumFrequency && x <= width; x += 1.0f)
    {
        const auto frequency = xToFrequency (x);
        const auto range = stage.getToothMagnitudeRange (frequency);
        addVertex (frequency, static_cast<int> (x) % 2 == 0 ? range.getEnd() : range.getStart());
    }

    if (spacing > 0.0f && teethBelow >= maximumFrequency)
        addVertex (maximumFrequency, stage.getMagnitudeResponse (maximumFrequency));

    std::sort (vertices.begin(), vertices.end(), [] (const auto& a, const auto& b)
    {
        return a.getX() < b.getX();
    });

    return vertices;
}

void SynthFilterResponse::paint (yup::Graphics& g)
{
    const auto bounds = getLocalBounds();
    const auto vertices = shownValues.enabled ? getCurveVertices() : std::vector<yup::Point<float>>();

    if (! vertices.empty() && SynthLandscape::isEnabled())
    {
        landscape.setProfile (bounds.getWidth(), bounds.getHeight(), [&vertices] (float x)
        {
            const auto next = std::lower_bound (vertices.begin(), vertices.end(), x, [] (const auto& vertex, float position)
            {
                return vertex.getX() < position;
            });

            if (next == vertices.begin())
                return next->getY();

            if (next == vertices.end())
                return vertices.back().getY();

            const auto& previous = *std::prev (next);
            const auto span = next->getX() - previous.getX();
            return span > 0.0f ? yup::jmap (x, previous.getX(), next->getX(), previous.getY(), next->getY()) : next->getY();
        });
    }

    const auto onLandscape = ! vertices.empty() && landscape.paint (g, *this, bounds, SynthTheme::accent (*this));

    if (! onLandscape)
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

    if (vertices.empty())
        return;

    // The landscape's ridge already traces the curve.
    if (! onLandscape)
    {
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
    }

    // Over the landscape the handle only shows under the mouse, or while it is dragged.
    if (onLandscape && ! hovered && ! cutoffGesture.isActive())
        return;

    const auto cutoff = yup::jlimit (minimumFrequency, maximumFrequency, shownValues.cutoff);
    const auto handle = yup::Point<float> (frequencyToX (cutoff), decibelsToY (getDecibelsAt (cutoff)));

    g.setFillColor (SynthTheme::textPrimary (*this));
    g.fillEllipse ({ handle.getX() - 5.0f, handle.getY() - 5.0f, 10.0f, 10.0f });
}

void SynthFilterResponse::refreshDisplay (double lastFrameTimeSeconds)
{
    if (isShowing() && landscape.advance (lastFrameTimeSeconds))
        repaint();
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
    repaint();
}

void SynthFilterResponse::mouseEnter (const yup::MouseEvent&)
{
    hovered = true;
    repaint();
}

void SynthFilterResponse::mouseExit (const yup::MouseEvent&)
{
    hovered = false;
    repaint();
}
