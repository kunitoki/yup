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

#include "../dsp/SynthEnvelopeParameters.h"
#include "EnvelopeDisplay.h"
#include "ParameterKnob.h"
#include "ParameterLed.h"
#include "ParameterSegmented.h"
#include "SynthModulationHandle.h"

//==============================================================================
/** One envelope: a toolbar with its polarity as a modulation source, its editable curve
    over its stage, time, velocity and depth macros, its tempo sync, the curve's snapping,
    and a handle dragged onto a control to modulate it. The amplitude envelope only
    shapes the level, so its polarity shows unipolar and disabled, next to a caption
    saying it drives the amplitude.

    The curve is drawn as the most recent note plays it, with its stages and time scale
    modulated. Synced, the curve's times are quarter note beats and the grid counts bars
    and beats in the host's meter; otherwise they are seconds.

    @see SynthEnvelopeParameters, SynthCurveState
*/
class SynthEnvelopePanel : public yup::Component
{
public:
    SynthEnvelopePanel (const SynthEnvelopeParameters& parametersToEdit, SynthCurveState& curvesToEdit, int envelopeIndex);

    /** Shows where the most recent note is along the curve's points, or hides it when negative. */
    void setPlayhead (float position) { display.setPlayhead (position); }

    /** Shows changes made by the host, a preset or a state load, and the curve as the
        modulated values read play it. */
    void syncFromParameters (SynthParameterReader modulated = {});

    /** Follows the host's tempo and meter, for the grid. */
    void setTransport (const SynthTransport& newTransport);

    /** Returns the Sync control placed in the section strip. */
    ParameterLed& getSyncLed() noexcept { return syncLed; }

    /** Returns the drag handle placed right of Sync in the section strip. */
    SynthModulationHandle& getModulationHandle() noexcept { return handle; }

    void paint (yup::Graphics& g) override;
    void resized() override;

private:
    static constexpr float lineHeight = 24.0f;
    static constexpr float polarityWidth = 140.0f;
    static constexpr float ampCaptionWidth = 48.0f;
    static constexpr float knobRowHeight = 62.0f;
    static constexpr int numKnobs = 7;
    static constexpr float optionsWidth = 76.0f;
    static constexpr int defaultSnap = 5;

    void updateTiming (const SynthEnvelopeValues& values);

    std::array<ParameterKnob*, numKnobs> getKnobs() noexcept
    {
        return { &attackKnob, &decayKnob, &sustainKnob, &releaseKnob, &timeScaleKnob, &velocityKnob, &depthKnob };
    }

    const SynthEnvelopeParameters& parameters;
    SynthCurveState& curves;
    int index = 0;
    int shownGeneration = -1;
    SynthTransport transport;
    SynthEnvelopeValues shownValues;
    bool shownMusical = false;
    yup::Rectangle<float> ampCaptionArea;
    yup::Font captionFont;

    EnvelopeDisplay display;
    ParameterLed syncLed;
    std::unique_ptr<SynthSegmented> polaritySelector;
    yup::ComboBox snapBox;
    SynthModulationHandle handle;

    ParameterKnob attackKnob;
    ParameterKnob decayKnob;
    ParameterKnob sustainKnob;
    ParameterKnob releaseKnob;
    ParameterKnob timeScaleKnob;
    ParameterKnob velocityKnob;
    ParameterKnob depthKnob;
};
