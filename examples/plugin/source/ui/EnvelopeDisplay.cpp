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

#include "../dsp/SynthFormat.h"

EnvelopeDisplay::EnvelopeDisplay()
{
    setOpaque (false);
}

void EnvelopeDisplay::setAmplitude (bool isAmplitudeEnvelope)
{
    amplitude = isAmplitudeEnvelope;
}

void EnvelopeDisplay::setTiming (const SynthTimeGrid::Timing& newTiming)
{
    if (newTiming == timing)
        return;

    timing = newTiming;
    repaint();
}

void EnvelopeDisplay::setCycle (bool isCycle)
{
    cycle = isCycle;
    repaint();
}

void EnvelopeDisplay::setInverted (bool shouldBeInverted)
{
    if (shouldBeInverted == inverted)
        return;

    inverted = shouldBeInverted;
    repaint();
}

void EnvelopeDisplay::setPlayhead (float position)
{
    if (std::abs (position - playhead) < 1.0e-4f)
        return;

    playhead = position;
    repaint();
}

void EnvelopeDisplay::setCurve (const SynthCurveState::Curve& newCurve)
{
    if (newCurve == curve)
        return;

    curve = newCurve;
    repaint();
}

//==============================================================================
yup::Rectangle<float> EnvelopeDisplay::getArea() const
{
    return getLocalBounds().reduced (10.0f);
}

float EnvelopeDisplay::getShownLength() const
{
    if (cycle)
        return 1.0f;

    return dragKind != DragKind::none ? frozenLength : yup::jmax (1.0e-3f, curve.getLength() * headroom);
}

float EnvelopeDisplay::getHoldWidth() const
{
    return ! cycle && curve.getSustainIndex() >= 0 ? getArea().getWidth() * sustainShare : 0.0f;
}

bool EnvelopeDisplay::isAfterHold (int index) const
{
    const auto sustain = curve.getSustainIndex();
    return ! cycle && sustain >= 0 && index > sustain;
}

yup::Point<float> EnvelopeDisplay::toScreen (float time, float level, bool afterHold) const
{
    const auto area = getArea();
    const auto timedWidth = area.getWidth() - getHoldWidth();
    const auto x = area.getX() + time / getShownLength() * timedWidth + (afterHold ? getHoldWidth() : 0.0f);

    const auto shown = yup::jlimit (0.0f, 1.0f, inverted ? 1.0f - level : level);

    return { x, area.getBottom() - shown * area.getHeight() };
}

float EnvelopeDisplay::toTime (float x, bool afterHold) const
{
    const auto area = getArea();
    const auto timedWidth = yup::jmax (1.0f, area.getWidth() - getHoldWidth());
    const auto offset = x - area.getX() - (afterHold ? getHoldWidth() : 0.0f);

    return yup::jmax (0.0f, offset / timedWidth * getShownLength());
}

float EnvelopeDisplay::toLevel (float y) const
{
    const auto area = getArea();
    const auto level = yup::jlimit (0.0f, 1.0f, (area.getBottom() - y) / yup::jmax (1.0f, area.getHeight()));

    return inverted ? 1.0f - level : level;
}

float EnvelopeDisplay::snapTime (float time, const yup::MouseEvent& event) const
{
    return event.getModifiers().isShiftDown() ? time : SynthTimeGrid::snap (time, timing, snapChoice);
}

yup::Point<float> EnvelopeDisplay::getHandle (int segment) const
{
    const auto& from = curve.getPoint (segment - 1);
    const auto& to = curve.getPoint (segment);
    const auto time = (from.time + to.time) * 0.5f;
    const auto level = SynthCurveState::Curve::segmentValue (from.level, to.level, to.curve, 0.5f);

    return toScreen (time, level, isAfterHold (segment));
}

int EnvelopeDisplay::findPoint (yup::Point<float> position) const
{
    for (int index = 0; index < curve.getNumPoints(); ++index)
        if (toScreen (curve.getPoint (index).time, curve.getPoint (index).level, isAfterHold (index)).distanceTo (position) <= hitRadius)
            return index;

    return -1;
}

int EnvelopeDisplay::findHandle (yup::Point<float> position) const
{
    for (int segment = 1; segment < curve.getNumPoints(); ++segment)
        if (getHandle (segment).distanceTo (position) <= hitRadius)
            return segment;

    return -1;
}

void EnvelopeDisplay::publish()
{
    // Every envelope ends silent, whichever point an edit left last.
    if (const auto last = curve.getNumPoints() - 1; ! cycle && last > 0 && curve.getPoint (last).level != 0.0f)
    {
        auto point = curve.getPoint (last);
        point.level = 0.0f;
        curve.setPoint (last, point);
    }

    repaint();

    if (onCurveEdited != nullptr)
        onCurveEdited (curve);
}

//==============================================================================
void EnvelopeDisplay::mouseMove (const yup::MouseEvent& event)
{
    const auto position = event.getPosition();
    const auto point = findPoint (position);
    const auto draggable = (point >= 0 && ! (amplitude && ! cycle && point == 0)) || findHandle (position) >= 0;
    setMouseCursor (draggable ? yup::MouseCursor::Hand : yup::MouseCursor::Default);
}

void EnvelopeDisplay::mouseDown (const yup::MouseEvent& event)
{
    const auto position = event.getPosition();
    frozenLength = getShownLength();
    dragPosition = position;
    editStart = curve;

    if (dragIndex = findPoint (position); dragIndex >= 0)
        dragKind = amplitude && ! cycle && dragIndex == 0 ? DragKind::none : DragKind::point;
    else if (dragIndex = findHandle (position); dragIndex >= 0)
        dragKind = DragKind::slope;
    else
        dragKind = DragKind::none;

    repaint();
}

void EnvelopeDisplay::mouseDrag (const yup::MouseEvent& event)
{
    if (dragKind == DragKind::none)
        return;

    dragPosition = event.getPosition();
    auto point = curve.getPoint (dragIndex);

    if (dragKind == DragKind::point)
    {
        const auto isLast = dragIndex == curve.getNumPoints() - 1;
        const auto time = snapTime (toTime (dragPosition.getX(), isAfterHold (dragIndex)), event);

        // A cycle's ends stay at its start and end; an envelope's last point ends silent.
        if (dragIndex == 0)
            point.time = 0.0f;
        else if (cycle)
            point.time = isLast ? 1.0f : yup::jlimit (0.0f, 1.0f, time);
        else
            point.time = time;

        point.level = isLast && ! cycle ? 0.0f : toLevel (dragPosition.getY());
        curve.setPoint (dragIndex, point);
        publish();
        return;
    }

    const auto& from = curve.getPoint (dragIndex - 1);
    const auto span = point.level - from.level;

    if (std::abs (span) < 1.0e-4f)
        return;

    point.curve = SynthCurveState::Curve::curveForMidpoint ((toLevel (dragPosition.getY()) - from.level) / span);
    curve.setPoint (dragIndex, point);
    publish();
}

void EnvelopeDisplay::mouseUp (const yup::MouseEvent&)
{
    if (dragKind != DragKind::none && onEditFinished != nullptr)
        onEditFinished (editStart);

    dragKind = DragKind::none;
    dragIndex = -1;
    repaint();
}

void EnvelopeDisplay::mouseDoubleClick (const yup::MouseEvent& event)
{
    const auto position = event.getPosition();
    const auto before = curve;
    dragKind = DragKind::none;

    if (const auto index = findPoint (position); index >= 0)
    {
        if (index == 0 || curve.getNumPoints() <= 2 || (cycle && index == curve.getNumPoints() - 1))
            return;

        curve.removePoint (index);
        publish();

        if (onEditFinished != nullptr)
            onEditFinished (before);

        return;
    }

    if (curve.getNumPoints() >= SynthCurveState::Curve::maxPoints)
        return;

    // A point added right of the sustain point, the hold included, belongs after it: its time
    // is read past the hold and never earlier than the sustain point's.
    const auto sustain = curve.getSustainIndex();
    const auto sustainTime = sustain >= 0 ? curve.getPoint (sustain).time : 0.0f;
    const auto afterHold = sustain >= 0 && position.getX() > toScreen (sustainTime, 0.0f, false).getX();
    const auto time = snapTime (toTime (position.getX(), afterHold), event);

    // A point added to a cycle stays strictly inside it, so its ends keep their places.
    if (cycle)
        curve.addPoint (yup::jlimit (cycleMargin, 1.0f - cycleMargin, time), toLevel (position.getY()));
    else
        curve.addPoint (afterHold ? yup::jmax (time, sustainTime) : time, toLevel (position.getY()));
    publish();

    if (onEditFinished != nullptr)
        onEditFinished (before);
}

//==============================================================================
void EnvelopeDisplay::paint (yup::Graphics& g)
{
    const auto bounds = getLocalBounds();
    SynthTheme::paintWell (g, *this, bounds);

    const auto area = getArea();

    if (area.getWidth() <= 0.0f || area.getHeight() <= 0.0f || curve.getNumPoints() < 2)
        return;

    const auto sustain = cycle ? -1 : curve.getSustainIndex();

    if (cycle)
    {
        g.setFillColor (SynthTheme::textPrimary (*this).withAlpha (0.08f));
        g.fillRect (area.getX(), area.getCenterY(), area.getWidth(), 1.0f);
    }

    const auto baseline = cycle ? area.getCenterY() : area.getBottom();
    auto previous = toScreen (0.0f, curve.getPoint (0).level, false);
    yup::Path fill;
    fill.moveTo (previous.getX(), baseline);
    fill.lineTo (previous);

    path.clear();
    path.moveTo (previous);

    const auto appendPoint = [&] (yup::Point<float> point)
    {
        // Close each bipolar lobe at zero instead of making a self-intersecting fill.
        if (cycle && (previous.getY() - baseline) * (point.getY() - baseline) < 0.0f)
        {
            const auto fraction = (baseline - previous.getY()) / (point.getY() - previous.getY());
            const auto x = previous.getX() + fraction * (point.getX() - previous.getX());
            fill.lineTo (x, baseline);
            fill.closeSubPath();
            fill.moveTo (x, baseline);
        }

        path.lineTo (point);
        fill.lineTo (point);
        previous = point;
    };

    for (int index = 1; index < curve.getNumPoints(); ++index)
    {
        const auto& from = curve.getPoint (index - 1);
        const auto& to = curve.getPoint (index);

        for (int step = 1; step <= stepsPerSegment; ++step)
        {
            const auto t = static_cast<float> (step) / static_cast<float> (stepsPerSegment);
            const auto time = from.time + (to.time - from.time) * t;
            appendPoint (toScreen (time, SynthCurveState::Curve::segmentValue (from.level, to.level, to.curve, t), isAfterHold (index)));
        }

        if (index == sustain)
            appendPoint (toScreen (to.time, to.level, true));
    }

    const auto color = cycle ? SynthTheme::lfoAccent (*this) : SynthTheme::envelopeAccent (*this);
    fill.lineTo (previous.getX(), baseline);
    fill.closeSubPath();
    g.setFillColor (color.withAlpha (0.13f));
    g.fillPath (fill);

    // The playhead keeps moving while a point is dragged; the time axis holds its scale
    // then, so a playhead past the shown length is left out rather than drawn outside.
    auto playheadPosition = yup::Point<float>();
    auto playing = false;

    if (playhead >= 0.0f)
    {
        const auto last = curve.getNumPoints() - 1;
        const auto index = yup::jlimit (0, last, static_cast<int> (std::floor (playhead)));
        const auto t = index < last ? playhead - static_cast<float> (index) : 0.0f;
        const auto& from = curve.getPoint (index);
        const auto& to = curve.getPoint (yup::jmin (index + 1, last));

        // On a point the playhead sits where that point is drawn, before the hold for the
        // sustain point; inside a segment it belongs to the segment's end point.
        playheadPosition = toScreen (from.time + (to.time - from.time) * t,
                                     SynthCurveState::Curve::segmentValue (from.level, to.level, to.curve, t),
                                     isAfterHold (t > 0.0f ? index + 1 : index));
        playing = playheadPosition.getX() <= area.getRight();
    }

    if (playing)
        SynthTheme::paintTrail (g, fill, playheadPosition.getX(), trailWidth, color);

    // Keep the grid above the fill and trail, with no elapsed time inside the sustain hold.
    const auto sustainTime = sustain >= 0 ? curve.getPoint (sustain).time : 0.0f;
    SynthTimeGrid::paint (g, *this, SynthTimeGrid::getLines (timing, getShownLength(), area.getWidth() - getHoldWidth()), area, [&] (float time)
    {
        return std::optional<float> (toScreen (time, 0.0f, sustain >= 0 && time > sustainTime).getX());
    });

    g.setStrokeColor (color);
    g.setStrokeWidth (2.0f);
    g.strokePath (path);

    for (int segment = 1; segment < curve.getNumPoints(); ++segment)
    {
        const auto handle = getHandle (segment);
        const auto active = dragKind == DragKind::slope && dragIndex == segment;

        g.setFillColor (SynthTheme::wellColor (*this));
        g.fillEllipse (handle.getX() - handleRadius, handle.getY() - handleRadius, handleRadius * 2.0f, handleRadius * 2.0f);
        g.setStrokeColor (active ? SynthTheme::textPrimary (*this) : color);
        g.setStrokeWidth (1.5f);
        g.strokeEllipse (handle.getX() - handleRadius, handle.getY() - handleRadius, handleRadius * 2.0f, handleRadius * 2.0f);
    }

    for (int index = 0; index < curve.getNumPoints(); ++index)
    {
        const auto& point = curve.getPoint (index);
        const auto center = toScreen (point.time, point.level, isAfterHold (index));
        const auto radius = dragKind == DragKind::point && dragIndex == index ? pointRadius + 1.5f : pointRadius;

        g.setFillColor (index == sustain ? color : SynthTheme::textPrimary (*this));
        g.fillEllipse (center.getX() - radius, center.getY() - radius, radius * 2.0f, radius * 2.0f);
    }

    if (playing)
    {
        g.setFillColor (color.withAlpha (0.35f));
        g.fillEllipse (playheadPosition.getX() - 7.0f, playheadPosition.getY() - 7.0f, 14.0f, 14.0f);
        g.setFillColor (SynthTheme::textPrimary (*this));
        g.fillEllipse (playheadPosition.getX() - 3.5f, playheadPosition.getY() - 3.5f, 7.0f, 7.0f);
    }

    if (dragKind == DragKind::none || dragIndex < 0)
        return;

    const auto& point = curve.getPoint (dragIndex);
    const auto level = cycle ? SynthFormat::bipolar ((inverted ? -1.0 : 1.0) * (2.0 * point.level - 1.0)) : SynthFormat::percent (point.level);
    const auto text = dragKind == DragKind::point ? SynthTimeGrid::format (point.time, timing) + "  " + level
                                                  : "CURVE " + yup::String (point.curve, 2);
    const auto font = SynthTheme::valueFont (12.0f);
    const auto width = SynthTheme::textWidth (text, font) + 14.0f;
    const auto pill = yup::Rectangle<float> (yup::jlimit (area.getX(), area.getRight() - width, dragPosition.getX() - width * 0.5f),
                                             yup::jmax (area.getY(), dragPosition.getY() - 30.0f),
                                             width,
                                             18.0f);

    g.setFillColor (color);
    g.fillRoundedRect (pill, 9.0f);
    g.setFillColor (SynthTheme::role (*this, yup::ThemePalette::Role::onAccent));
    g.fillFittedText (text, font, pill, yup::Justification::center);
}
