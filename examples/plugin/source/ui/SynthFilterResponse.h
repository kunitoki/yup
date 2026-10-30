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

#include "../dsp/SynthFilterStage.h"
#include "ParameterGesture.h"

#include <yup_gui/yup_gui.h>

#include <vector>

//==============================================================================
/** The magnitude response of a voice filter from 20 Hz to 20 kHz, with a handle at the
    cutoff: dragging it sideways moves the cutoff, up and down the resonance.

    The curve comes from a filter stage of its own set to the filter's controls, the
    same filters the voices play.

    @see SynthFilterPanel, SynthFilterStage
*/
class SynthFilterResponse : public yup::Component
{
public:
    explicit SynthFilterResponse (const SynthFilterParameters& parametersToShow);

    /** Redraws the curve when the filter's controls changed.

        @param modulated  Where the values come from: the parameters, or a table with the
                          most recent note's modulation, so the curve follows a modulated cutoff.
        @returns true when they did.
    */
    bool syncFromParameters (SynthParameterReader modulated = {});

    /** Draws the response at the rate the voices play at; ignored until it is known. */
    void setSampleRate (double newSampleRate);

    void paint (yup::Graphics& g) override;
    void mouseDown (const yup::MouseEvent& event) override;
    void mouseDrag (const yup::MouseEvent& event) override;
    void mouseUp (const yup::MouseEvent& event) override;

private:
    static constexpr float minimumFrequency = 20.0f;
    static constexpr float maximumFrequency = 20000.0f;
    static constexpr float topDecibels = 24.0f;
    static constexpr float bottomDecibels = -48.0f;
    static constexpr float pixelsPerResonance = 160.0f;
    static constexpr int points = 160;
    static constexpr int refineIterations = 20;
    static constexpr float exactToothPixels = 12.0f;
    static constexpr float denseToothPixels = 2.0f;

    float frequencyToX (float frequency) const;
    float xToFrequency (float x) const;
    float decibelsToY (float decibels) const;
    float getDecibelsAt (float frequency) const;
    float findExtremeFrequency (float lowFrequency, float highFrequency, bool peak) const;
    std::vector<yup::Point<float>> getCurveVertices() const;

    const SynthFilterParameters& parameters;
    SynthFilterStage stage;
    SynthFilterValues shownValues;
    double sampleRate = 48000.0;
    ParameterGesture cutoffGesture;
    ParameterGesture resonanceGesture;
    yup::Font labelFont;
};
