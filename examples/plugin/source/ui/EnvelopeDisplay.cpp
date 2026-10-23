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
    return dragKind != DragKind::none ? frozenLength : yup::jmax (1.0e-3f, curve.getLength() * headroom);
}

float EnvelopeDisplay::getHoldWidth() const
{
    return curve.getSustainIndex() >= 0 ? getArea().getWidth() * sustainShare : 0.0f;
}

yup::Point<float> EnvelopeDisplay::toScreen (float time, float level) const
{
    const auto area = getArea();
    const auto sustain = curve.getSustainIndex();
    const auto sustainTime = sustain >= 0 ? curve.getPoint (sustain).time : getShownLength();
    const auto timedWidth = area.getWidth() - getHoldWidth();
    const auto x = area.getX() + time / getShownLength() * timedWidth + (sustain >= 0 && time > sustainTime ? getHoldWidth() : 0.0f);

    return { x, area.getBottom() - yup::jlimit (0.0f, 1.0f, level) * area.getHeight() };
}

float EnvelopeDisplay::toTime (float x) const
{
    const auto area = getArea();
    const auto sustain = curve.getSustainIndex();
    const auto timedWidth = yup::jmax (1.0f, area.getWidth() - getHoldWidth());
    auto offset = x - area.getX();

    if (sustain >= 0)
    {
        const auto sustainX = curve.getPoint (sustain).time / getShownLength() * timedWidth;

        if (offset > sustainX + getHoldWidth())
            offset -= getHoldWidth();
        else if (offset > sustainX)
            offset = sustainX;
    }

    return yup::jmax (0.0f, offset / timedWidth * getShownLength());
}

float EnvelopeDisplay::toLevel (float y) const
{
    const auto area = getArea();
    return yup::jlimit (0.0f, 1.0f, (area.getBottom() - y) / yup::jmax (1.0f, area.getHeight()));
}

yup::Point<float> EnvelopeDisplay::getHandle (int segment) const
{
    const auto& from = curve.getPoint (segment - 1);
    const auto& to = curve.getPoint (segment);
    const auto time = (from.time + to.time) * 0.5f;
    const auto level = SynthCurveState::Curve::segmentValue (from.level, to.level, to.curve, 0.5f);

    return toScreen (time, level);
}

int EnvelopeDisplay::findPoint (yup::Point<float> position) const
{
    for (int index = 0; index < curve.getNumPoints(); ++index)
        if (toScreen (curve.getPoint (index).time, curve.getPoint (index).level).distanceTo (position) <= hitRadius)
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
    repaint();

    if (onCurveEdited != nullptr)
        onCurveEdited (curve);
}

//==============================================================================
void EnvelopeDisplay::mouseDown (const yup::MouseEvent& event)
{
    const auto position = event.getPosition();
    frozenLength = getShownLength();
    dragPosition = position;

    if (dragIndex = findPoint (position); dragIndex >= 0)
        dragKind = DragKind::point;
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
        point.time = dragIndex == 0 ? 0.0f : toTime (dragPosition.getX());
        point.level = toLevel (dragPosition.getY());
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
    dragKind = DragKind::none;
    dragIndex = -1;
    repaint();
}

void EnvelopeDisplay::mouseDoubleClick (const yup::MouseEvent& event)
{
    const auto position = event.getPosition();
    dragKind = DragKind::none;

    if (const auto index = findPoint (position); index >= 0)
    {
        if (index == 0 || curve.getNumPoints() <= 2)
            return;

        curve.removePoint (index);
        publish();
        return;
    }

    if (curve.getNumPoints() >= SynthCurveState::Curve::maxPoints)
        return;

    curve.addPoint (toTime (position.getX()), toLevel (position.getY()));
    publish();
}

//==============================================================================
void EnvelopeDisplay::paint (yup::Graphics& g)
{
    const auto bounds = getLocalBounds();
    SynthTheme::paintWell (g, *this, bounds);

    const auto area = getArea();

    if (area.getWidth() <= 0.0f || area.getHeight() <= 0.0f || curve.getNumPoints() < 2)
        return;

    const auto sustain = curve.getSustainIndex();
    const auto holdWidth = getHoldWidth();

    path.clear();
    path.moveTo (toScreen (0.0f, curve.getPoint (0).level));

    for (int index = 1; index < curve.getNumPoints(); ++index)
    {
        const auto& from = curve.getPoint (index - 1);
        const auto& to = curve.getPoint (index);

        for (int step = 1; step <= stepsPerSegment; ++step)
        {
            const auto t = static_cast<float> (step) / static_cast<float> (stepsPerSegment);
            const auto time = from.time + (to.time - from.time) * t;
            path.lineTo (toScreen (time, SynthCurveState::Curve::segmentValue (from.level, to.level, to.curve, t)));
        }

        if (index == sustain)
            path.lineTo (toScreen (to.time, to.level).translated (holdWidth, 0.0f));
    }

    const auto color = SynthTheme::envelopeAccent();

    // Paths share their data when copied, so the area under the curve gets a path of its own.
    yup::Path fill;
    fill.appendPath (path);
    fill.lineTo (toScreen (curve.getPoint (curve.getNumPoints() - 1).time, 0.0f));
    fill.lineTo (area.getX(), area.getBottom());
    fill.closeSubPath();
    g.setFillColor (color.withAlpha (0.13f));
    g.fillPath (fill);

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
        const auto center = toScreen (point.time, point.level);
        const auto radius = dragKind == DragKind::point && dragIndex == index ? pointRadius + 1.5f : pointRadius;

        g.setFillColor (index == sustain ? color : SynthTheme::textPrimary (*this));
        g.fillEllipse (center.getX() - radius, center.getY() - radius, radius * 2.0f, radius * 2.0f);
    }

    if (dragKind == DragKind::none || dragIndex < 0)
        return;

    const auto& point = curve.getPoint (dragIndex);
    const auto text = dragKind == DragKind::point ? SynthFormat::milliseconds (point.time) + "  " + SynthFormat::percent (point.level)
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
