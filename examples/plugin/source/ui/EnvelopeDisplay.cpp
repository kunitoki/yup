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

EnvelopeDisplay::EnvelopeDisplay (std::shared_ptr<SynthWaveformShader> landscapeShader)
    : landscape (std::move (landscapeShader))
{
    gridFont = SynthTheme::captionFont (9.5f);
    valueFont = SynthTheme::valueFont (12.0f);

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
    updateShown();
    repaint();
}

void EnvelopeDisplay::setTransfer (bool isTransfer)
{
    transfer = isTransfer;
    setCycle (isTransfer || cycle);
}

void EnvelopeDisplay::setStages (const SynthEnvelopeStages& newStages)
{
    if (newStages == stages)
        return;

    stages = newStages;
    updateShown();
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
    updateShown();
    repaint();
}

void EnvelopeDisplay::updateShown()
{
    shown = cycle ? curve : stages.apply (curve);
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

    return dragKind != DragKind::none ? frozenLength : yup::jmax (1.0e-3f, shown.getLength() * headroom);
}

yup::Point<float> EnvelopeDisplay::toScreen (float time, float level) const
{
    const auto area = getArea();
    const auto x = area.getX() + time / getShownLength() * area.getWidth();

    const auto shown = yup::jlimit (0.0f, 1.0f, inverted ? 1.0f - level : level);

    return { x, area.getBottom() - shown * area.getHeight() };
}

float EnvelopeDisplay::toTime (float x) const
{
    const auto area = getArea();
    return yup::jmax (0.0f, (x - area.getX()) / yup::jmax (1.0f, area.getWidth()) * getShownLength());
}

float EnvelopeDisplay::toLevel (float y) const
{
    const auto area = getArea();
    const auto level = yup::jlimit (0.0f, 1.0f, (area.getBottom() - y) / yup::jmax (1.0f, area.getHeight()));

    return inverted ? 1.0f - level : level;
}

float EnvelopeDisplay::toCurveTime (float shownTime, int segment) const
{
    if (segment <= 0)
        return shownTime;

    // The segment's stage stretched its duration, which the curve keeps unstretched.
    const auto scale = cycle ? 1.0f : yup::jmax (1.0e-3f, stages.getScale (curve, segment));
    return curve.getPoint (segment - 1).time + (shownTime - shown.getPoint (segment - 1).time) / scale;
}

float EnvelopeDisplay::snapTime (float time, const yup::MouseEvent& event) const
{
    return event.getModifiers().isShiftDown() ? time : SynthTimeGrid::snap (time, timing, snapChoice);
}

float EnvelopeDisplay::getLevelAt (float shownTime) const
{
    for (int index = 1; index < shown.getNumPoints(); ++index)
    {
        const auto& from = shown.getPoint (index - 1);
        const auto& to = shown.getPoint (index);

        if (shownTime > to.time)
            continue;

        const auto t = to.time > from.time ? yup::jlimit (0.0f, 1.0f, (shownTime - from.time) / (to.time - from.time)) : 1.0f;
        return SynthCurveState::Curve::segmentValue (from.level, to.level, to.curve, t);
    }

    return shown.getPoint (shown.getNumPoints() - 1).level;
}

/** Returns where the playhead is drawn, or nothing while it is hidden or past the shown length. */
std::optional<yup::Point<float>> EnvelopeDisplay::getPlayheadPosition() const
{
    if (playhead < 0.0f)
        return std::nullopt;

    // The playhead keeps moving while a point is dragged; the time axis holds its scale
    // then, so a playhead past the shown length is left out rather than drawn outside.
    const auto last = shown.getNumPoints() - 1;
    const auto index = yup::jlimit (0, last, static_cast<int> (std::floor (playhead)));
    const auto t = index < last ? playhead - static_cast<float> (index) : 0.0f;
    const auto& from = shown.getPoint (index);
    const auto& to = shown.getPoint (yup::jmin (index + 1, last));

    const auto position = toScreen (from.time + (to.time - from.time) * t,
                                    SynthCurveState::Curve::segmentValue (from.level, to.level, to.curve, t));

    if (position.getX() > getArea().getRight())
        return std::nullopt;

    return position;
}

yup::Point<float> EnvelopeDisplay::getHandle (int segment) const
{
    const auto& from = shown.getPoint (segment - 1);
    const auto& to = shown.getPoint (segment);
    const auto time = (from.time + to.time) * 0.5f;
    const auto level = SynthCurveState::Curve::segmentValue (from.level, to.level, to.curve, 0.5f);

    return toScreen (time, level);
}

int EnvelopeDisplay::findPoint (yup::Point<float> position) const
{
    for (int index = 0; index < shown.getNumPoints(); ++index)
        if (toScreen (shown.getPoint (index).time, shown.getPoint (index).level).distanceTo (position) <= hitRadius)
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

    updateShown();
    repaint();

    if (onCurveEdited != nullptr)
        onCurveEdited (curve);
}

//==============================================================================
void EnvelopeDisplay::mouseEnter (const yup::MouseEvent&)
{
    hovered = true;
    repaint();
}

void EnvelopeDisplay::mouseExit (const yup::MouseEvent&)
{
    hovered = false;
    repaint();
}

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
        const auto time = toCurveTime (snapTime (toTime (dragPosition.getX()), event), dragIndex);

        // A cycle's ends stay at its start and end; an envelope's last point ends silent.
        if (dragIndex == 0)
            point.time = 0.0f;
        else if (cycle)
            point.time = isLast ? 1.0f : yup::jlimit (0.0f, 1.0f, time);
        else
            point.time = time;

        // The sustain point is shown at the level its stage scales it to.
        auto level = toLevel (dragPosition.getY());

        if (! cycle && dragIndex == curve.getSustainIndex())
            level = stages.sustain > 0.0f ? level / stages.sustain : point.level;

        point.level = isLast && ! cycle ? 0.0f : level;
        curve.setPoint (dragIndex, point);
        publish();
        return;
    }

    const auto& from = shown.getPoint (dragIndex - 1);
    const auto span = shown.getPoint (dragIndex).level - from.level;

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

    const auto shownTime = snapTime (toTime (position.getX()), event);
    auto segment = 1;

    while (segment < shown.getNumPoints() - 1 && shown.getPoint (segment).time < shownTime)
        ++segment;

    const auto time = toCurveTime (shownTime, segment);

    // A point added to a cycle stays strictly inside it, so its ends keep their places.
    if (cycle)
        curve.addPoint (yup::jlimit (cycleMargin, 1.0f - cycleMargin, time), toLevel (position.getY()));
    else
        curve.addPoint (time, toLevel (position.getY()));
    publish();

    if (onEditFinished != nullptr)
        onEditFinished (before);
}

//==============================================================================
void EnvelopeDisplay::paint (yup::Graphics& g)
{
    const auto bounds = getLocalBounds();
    const auto area = getArea();
    const auto color = cycle ? SynthTheme::lfoAccent (*this) : SynthTheme::envelopeAccent (*this);
    const auto hasCurve = area.getWidth() > 0.0f && area.getHeight() > 0.0f && shown.getNumPoints() >= 2;
    const auto playheadPosition = hasCurve ? getPlayheadPosition() : std::nullopt;

    if (hasCurve && SynthLandscape::isEnabled())
    {
        landscape.setProfile (bounds.getWidth(), bounds.getHeight(), [this] (float x)
        {
            const auto time = toTime (x);
            return toScreen (time, getLevelAt (time)).getY();
        });

        landscape.setPlayhead (playheadPosition.has_value() ? playheadPosition->getX() : -1.0f, trailWidth);
    }

    const auto onLandscape = hasCurve && landscape.paint (g, *this, bounds, color);

    if (! onLandscape)
        SynthTheme::paintWell (g, *this, bounds);

    if (! hasCurve)
        return;

    const auto sustain = cycle ? -1 : curve.getSustainIndex();

    if (cycle)
    {
        g.setFillColor (SynthTheme::textPrimary (*this).withAlpha (0.08f));
        g.fillRect (area.getX(), area.getCenterY(), area.getWidth(), 1.0f);

        if (transfer)
            g.fillRect (area.getCenterX(), area.getY(), 1.0f, area.getHeight());
    }

    const auto baseline = cycle ? area.getCenterY() : area.getBottom();
    auto previous = toScreen (0.0f, shown.getPoint (0).level);
    yup::Path fill;
    fill.moveTo (previous.getX(), baseline);
    fill.lineTo (previous);

    path.clear();
    path.moveTo (previous);

    const auto appendPoint = [&] (yup::Point<float> point)
    {
        path.lineTo (point);
        fill.lineTo (point);
        previous = point;
    };

    for (int index = 1; index < shown.getNumPoints(); ++index)
    {
        const auto& from = shown.getPoint (index - 1);
        const auto& to = shown.getPoint (index);

        for (int step = 1; step <= stepsPerSegment; ++step)
        {
            const auto t = static_cast<float> (step) / static_cast<float> (stepsPerSegment);
            const auto time = from.time + (to.time - from.time) * t;
            appendPoint (toScreen (time, SynthCurveState::Curve::segmentValue (from.level, to.level, to.curve, t)));
        }
    }

    fill.lineTo (previous.getX(), baseline);
    fill.closeSubPath();

    // A cycle is a bipolar line only, an envelope fills the area under it; the landscape
    // shows both itself, and lights the playhead's trail.
    if (! cycle && ! onLandscape)
    {
        g.setFillColor (color.withAlpha (0.13f));
        g.fillPath (fill);

        if (playheadPosition.has_value())
            SynthTheme::paintTrail (g, fill, playheadPosition->getX(), trailWidth, color);
    }

    // Keep the grid above the fill and trail.
    if (! transfer)
    {
        SynthTimeGrid::paint (g, *this, SynthTimeGrid::getLines (timing, getShownLength(), area.getWidth()), area, gridFont, [&] (float time)
        {
            return std::optional<float> (toScreen (time, 0.0f).getX());
        });
    }

    // Over the landscape the stroke and its points only show under the mouse, or follow a drag ahead of the ridge.
    if (onLandscape && ! hovered && dragKind == DragKind::none)
        return;

    g.setStrokeColor (color);
    g.setStrokeWidth (2.0f);
    g.strokePath (path);

    for (int segment = 1; segment < shown.getNumPoints(); ++segment)
    {
        const auto handle = getHandle (segment);
        const auto active = dragKind == DragKind::slope && dragIndex == segment;

        g.setFillColor (SynthTheme::wellColor (*this));
        g.fillEllipse (handle.getX() - handleRadius, handle.getY() - handleRadius, handleRadius * 2.0f, handleRadius * 2.0f);
        g.setStrokeColor (active ? SynthTheme::textPrimary (*this) : color);
        g.setStrokeWidth (1.5f);
        g.strokeEllipse (handle.getX() - handleRadius, handle.getY() - handleRadius, handleRadius * 2.0f, handleRadius * 2.0f);
    }

    for (int index = 0; index < shown.getNumPoints(); ++index)
    {
        const auto& point = shown.getPoint (index);
        const auto center = toScreen (point.time, point.level);
        const auto radius = dragKind == DragKind::point && dragIndex == index ? pointRadius + 1.5f : pointRadius;

        g.setFillColor (index == sustain ? color : SynthTheme::textPrimary (*this));
        g.fillEllipse (center.getX() - radius, center.getY() - radius, radius * 2.0f, radius * 2.0f);
    }

    if (playheadPosition.has_value() && ! onLandscape)
    {
        g.setFillColor (color.withAlpha (0.35f));
        g.fillEllipse (playheadPosition->getX() - 7.0f, playheadPosition->getY() - 7.0f, 14.0f, 14.0f);
        g.setFillColor (SynthTheme::textPrimary (*this));
        g.fillEllipse (playheadPosition->getX() - 3.5f, playheadPosition->getY() - 3.5f, 7.0f, 7.0f);
    }

    if (dragKind == DragKind::none || dragIndex < 0)
        return;

    const auto& point = shown.getPoint (dragIndex);
    const auto level = cycle ? SynthFormat::bipolar ((inverted ? -1.0 : 1.0) * (2.0 * point.level - 1.0)) : SynthFormat::percent (point.level);
    const auto position = transfer ? SynthFormat::bipolar (2.0 * point.time - 1.0) + " ->" : SynthTimeGrid::format (point.time, timing);
    const auto text = dragKind == DragKind::point ? position + "  " + level
                                                  : "CURVE " + yup::String (point.curve, 2);
    const auto width = SynthTheme::textWidth (text, valueFont) + 14.0f;
    const auto pill = yup::Rectangle<float> (yup::jlimit (area.getX(), area.getRight() - width, dragPosition.getX() - width * 0.5f),
                                             yup::jmax (area.getY(), dragPosition.getY() - 30.0f),
                                             width,
                                             18.0f);

    g.setFillColor (color);
    g.fillRoundedRect (pill, 9.0f);
    g.setFillColor (SynthTheme::role (*this, yup::ThemePalette::Role::onAccent));
    g.fillFittedText (text, valueFont, pill, yup::Justification::center);
}

void EnvelopeDisplay::refreshDisplay (double lastFrameTimeSeconds)
{
    if (isShowing() && landscape.advance (lastFrameTimeSeconds))
        repaint();
}
