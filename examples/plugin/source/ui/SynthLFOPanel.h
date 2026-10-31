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

#include "EnvelopeDisplay.h"
#include "LFODisplay.h"
#include "ParameterChoice.h"
#include "ParameterKnob.h"
#include "ParameterLed.h"
#include "ParameterSegmented.h"
#include "SynthModulationHandle.h"

//==============================================================================
/** One LFO: its shape and retrigger mode, its display, and rate, phase, smoothing and
    depth, its inversion, and a handle dragged onto a control to modulate it. The rate
    knob turns into the tempo division while the LFO is synced.

    The custom shape shows its curve for editing, one cycle snapping to the chosen step;
    the other shapes show what the LFO plays.

    @see SynthLFOParameters
*/
class SynthLFOPanel : public yup::Component
{
public:
    SynthLFOPanel (const SynthLFOParameters& parametersToEdit,
                   SynthCurveState& curvesToEdit,
                   int lfoIndex,
                   std::shared_ptr<SynthWaveformShader> landscapeShader);

    /** Shows changes made by the host, a preset or a state load. */
    void syncFromParameters();

    /** Moves the display marker to the engine's phase. */
    void setPhase (float phase);

    /** Lets the shape's landscape move, while something plays. */
    void setAnimating (bool shouldAnimate) noexcept
    {
        display.setAnimating (shouldAnimate);
        editor.setAnimating (shouldAnimate);
    }

    /** Follows the host's tempo and meter, for the grid and the synced rate. */
    void setTransport (const SynthTransport& newTransport);

    /** Returns the drag handle placed right of Sync in the section strip. */
    SynthModulationHandle& getModulationHandle() noexcept { return handle; }

    void resized() override;

private:
    static constexpr float lineHeight = 24.0f;
    static constexpr float knobRowHeight = 62.0f;
    static constexpr float retriggerWidth = 96.0f;
    static constexpr float optionsWidth = 76.0f;
    static constexpr int defaultSnap = 5;

    static std::vector<ParameterSegmented::Segment> makeShapeSegments();

    bool isCustom() const noexcept { return shownValues.shape == SynthLFOValues::Shape::custom; }
    void showValues();
    void updateTiming();

    const SynthLFOParameters& parameters;
    SynthCurveState& curves;
    int index = 0;
    int shownGeneration = -1;
    SynthLFOValues shownValues;
    SynthTransport transport;
    bool shownMusical = false;

    ParameterSegmented shapeSelector;
    ParameterChoice retriggerChoice;
    LFODisplay display;
    EnvelopeDisplay editor;
    ParameterLed invertLed;
    yup::ComboBox snapBox;
    SynthModulationHandle handle;
    ParameterKnob rateKnob;
    ParameterKnob phaseKnob;
    ParameterKnob smoothKnob;
    ParameterKnob depthKnob;
};
