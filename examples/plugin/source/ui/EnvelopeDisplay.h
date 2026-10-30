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
#include "../dsp/SynthEnvelopeParameters.h"
#include "SynthLandscape.h"
#include "SynthTimeGrid.h"

#include <functional>
#include <optional>

//==============================================================================
/** Draws and edits the curve an envelope traces.

    Dragging a point moves it in time and level; dragging the handle in the middle of a
    segment bends that segment. A double-click adds a point, or removes the one under
    the mouse. The time axis keeps its scale while a drag runs and leaves some room on
    the right, so the last segment can be lengthened.

    The curve is drawn as it plays, reshaped by the envelope's stages (see setStages()),
    so the grid, the snapping and the readout of a dragged point count the time it takes;
    edits are carried back to the curve itself.

    A grid marks the curve's time, in seconds or in bars and beats, and dragged or added
    points snap to the chosen step unless shift is held. The same editor edits a custom
    LFO cycle, see setCycle(), and a waveshaper's transfer curve, see setTransfer().

    With landscape displays on, the curve is the ridge of a shader landscape and the
    playhead lights it as it runs; the stroke only shows while a drag edits it.
*/
class EnvelopeDisplay : public yup::Component
{
public:
    explicit EnvelopeDisplay (std::shared_ptr<SynthWaveformShader> landscapeShader);

    /** Called with the curve after every edit. */
    std::function<void (const SynthCurveState::Curve&)> onCurveEdited;

    /** Called once a drag or a double click finished editing, with the curve it started from. */
    std::function<void (const SynthCurveState::Curve&)> onEditFinished;

    /** Locks the start of an amplitude envelope, which begins silent, at time and level 0.
        Every envelope ends at level 0: its last point only moves in time. */
    void setAmplitude (bool isAmplitudeEnvelope);

    /** Sets how the curve's times read, for the grid, the snapping and the readout. */
    void setTiming (const SynthTimeGrid::Timing& newTiming);

    /** Sets the stages that reshape the curve as it plays; cycles ignore them. */
    void setStages (const SynthEnvelopeStages& newStages);

    /** Picks a snapping choice of SynthTimeGrid::getSnapNames(); 0 is off. */
    void setSnap (int newSnapChoice) { snapChoice = newSnapChoice; }

    /** Edits one cycle of a custom LFO instead of an envelope: the curve spans times 0 to 1,
        its ends keep those times but their levels move freely, levels read bipolar and no
        sustain point is marked. */
    void setCycle (bool isCycle);

    /** Edits a waveshaper's transfer curve: a cycle whose times read as the input from
        -1 to 1, so no time grid is drawn and a dragged point shows its input and output. */
    void setTransfer (bool isTransfer);

    /** Draws the levels upside down, as an inverted LFO plays them, without changing the curve. */
    void setInverted (bool shouldBeInverted);

    /** Shows where the most recent note is along the curve's points, with a band of light
        trailing it; a negative position hides it. Called every frame.

        The position counts points, not seconds (see yup::CurveEnvelope::getPointPosition()),
        so it lands on the curve shown even while a drag moves the points ahead of the
        curve the voices play.
    */
    void setPlayhead (float position);

    /** Updates the drawn curve. Called from the message thread. */
    void setCurve (const SynthCurveState::Curve& newCurve);

    /** Lets the landscape move, while something plays. */
    void setAnimating (bool shouldAnimate) noexcept { landscape.setAnimating (shouldAnimate); }

    void paint (yup::Graphics& g) override;
    void refreshDisplay (double lastFrameTimeSeconds) override;
    void mouseEnter (const yup::MouseEvent& event) override;
    void mouseExit (const yup::MouseEvent& event) override;
    void mouseMove (const yup::MouseEvent& event) override;
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

    static constexpr float pointRadius = 4.0f;
    static constexpr float handleRadius = 3.5f;
    static constexpr float hitRadius = 8.0f;
    static constexpr float headroom = 1.2f;
    static constexpr float cycleMargin = 1.0e-3f;
    static constexpr float trailWidth = 90.0f;
    static constexpr int stepsPerSegment = 24;

    bool hovered = false;

    yup::Rectangle<float> getArea() const;
    float getShownLength() const;
    yup::Point<float> toScreen (float time, float level) const;
    float toTime (float x) const;
    float toLevel (float y) const;
    float toCurveTime (float shownTime, int segment) const;
    void updateShown();
    float snapTime (float time, const yup::MouseEvent& event) const;
    float getLevelAt (float shownTime) const;
    std::optional<yup::Point<float>> getPlayheadPosition() const;
    yup::Point<float> getHandle (int segment) const;
    int findPoint (yup::Point<float> position) const;
    int findHandle (yup::Point<float> position) const;
    void publish();

    SynthCurveState::Curve curve;
    SynthCurveState::Curve shown;
    SynthEnvelopeStages stages;
    yup::Path path;
    bool amplitude = false;
    bool cycle = false;
    bool transfer = false;
    bool inverted = false;
    SynthTimeGrid::Timing timing;
    int snapChoice = 0;
    float playhead = -1.0f;
    DragKind dragKind = DragKind::none;
    int dragIndex = -1;
    float frozenLength = 0.0f;
    SynthCurveState::Curve editStart;
    yup::Point<float> dragPosition;
    yup::Font gridFont;
    yup::Font valueFont;
    SynthLandscape landscape;
};
