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
    the width after the sustain point.
*/
class EnvelopeDisplay : public yup::Component
{
public:
    EnvelopeDisplay();

    /** Called with the curve after every edit. */
    std::function<void (const SynthCurveState::Curve&)> onCurveEdited;

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
    static constexpr int stepsPerSegment = 24;

    yup::Rectangle<float> getArea() const;
    float getShownLength() const;
    float getHoldWidth() const;
    yup::Point<float> toScreen (float time, float level) const;
    float toTime (float x) const;
    float toLevel (float y) const;
    yup::Point<float> getHandle (int segment) const;
    int findPoint (yup::Point<float> position) const;
    int findHandle (yup::Point<float> position) const;
    void publish();

    SynthCurveState::Curve curve;
    yup::Path path;
    DragKind dragKind = DragKind::none;
    int dragIndex = -1;
    float frozenLength = 0.0f;
    yup::Point<float> dragPosition;
};
