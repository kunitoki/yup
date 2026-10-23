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
#pragma once

#include <yup_gui/yup_gui.h>

#include "../dsp/SynthCurveState.h"

#include <functional>

//==============================================================================
/** Draws and edits the curve an envelope traces.

    Dragging a point moves it in time and level; dragging the handle in the middle of a
    segment bends that segment. A double-click adds a point, or removes the one under
    the mouse. The time axis keeps its scale while a drag runs and leaves some room on
    the right, so the last segment can be lengthened.

    The sustain hold has no duration of its own, so it is drawn with a fixed share of
    the width after the sustain point; what comes after the sustain point by order, not
    by time, is drawn after the hold, so a point dragged onto the sustain time stays on
    its side of it.
*/
class EnvelopeDisplay : public yup::Component
{
public:
    EnvelopeDisplay();

    /** Called with the curve after every edit. */
    std::function<void (const SynthCurveState::Curve&)> onCurveEdited;

    /** Called once a drag or a double click finished editing, with the curve it started from. */
    std::function<void (const SynthCurveState::Curve&)> onEditFinished;

    /** Locks the start of an amplitude envelope, which begins silent, at time and level 0.
        Every envelope ends at level 0: its last point only moves in time. */
    void setAmplitude (bool isAmplitudeEnvelope);

    /** Shows where the most recent note is along the curve's points, with a band of light
        trailing it; a negative position hides it. Called every frame.

        The position counts points, not seconds (see yup::CurveEnvelope::getPointPosition()),
        so it lands on the curve shown even while a drag moves the points ahead of the
        curve the voices play.
    */
    void setPlayhead (float position);

    /** Updates the drawn curve. Called from the message thread. */
    void setCurve (const SynthCurveState::Curve& newCurve);

    void paint (yup::Graphics& g) override;
    void mouseDown (const yup::MouseEvent& event) override;
    void mouseDrag (const yup::MouseEvent& event) override;
    void mouseUp (const yup::MouseEvent& event) override;
    void mouseDoubleClick (const yup::MouseEvent& event) override;

private:
    enum class DragKind
    {
        none,
        point,
        slope
    };

    static constexpr float sustainShare = 0.22f;
    static constexpr float pointRadius = 4.0f;
    static constexpr float handleRadius = 3.5f;
    static constexpr float hitRadius = 8.0f;
    static constexpr float headroom = 1.2f;
    static constexpr float trailWidth = 90.0f;
    static constexpr int stepsPerSegment = 24;

    yup::Rectangle<float> getArea() const;
    float getShownLength() const;
    float getHoldWidth() const;
    yup::Point<float> toScreen (float time, float level, bool afterHold) const;
    float toTime (float x, bool afterHold) const;
    bool isAfterHold (int index) const;
    float toLevel (float y) const;
    yup::Point<float> getHandle (int segment) const;
    int findPoint (yup::Point<float> position) const;
    int findHandle (yup::Point<float> position) const;
    void publish();

    SynthCurveState::Curve curve;
    yup::Path path;
    bool amplitude = false;
    float playhead = -1.0f;
    DragKind dragKind = DragKind::none;
    int dragIndex = -1;
    float frozenLength = 0.0f;
    SynthCurveState::Curve editStart;
    yup::Point<float> dragPosition;
};
