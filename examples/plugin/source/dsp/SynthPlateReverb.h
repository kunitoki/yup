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

#include "SynthEffectsParameters.h"

#include <vector>

//==============================================================================
/** A plate reverb after Jon Dattorro's "Effect Design, Part 1" (JAES, 1997).

    A mono sum runs through a predelay and four input diffusers into a figure-eight tank of
    two modulated allpasses, delays, damping filters and decay allpasses; the stereo output
    is tapped from inside the tank. Delay lengths are scaled from the paper's 29761 Hz to
    the host rate. Decay sets the tank gain, damping the tank's lowpass, and width scales
    the side signal of the output.
*/
class SynthPlateReverb
{
public:
    /** Allocates the delays for a sample rate. */
    void prepare (double sampleRate);

    /** Clears the predelay and the tank. */
    void reset() noexcept;

    /** Reverberates a stereo chunk in place. */
    void process (float* left, float* right, int numSamples, const SynthReverbValues& values) noexcept;

private:
    struct DelayBuffer
    {
        void allocate (int maximumDelay);
        void clear() noexcept;
        void push (float sample) noexcept;

        /** Returns the sample pushed delay pushes ago, 0 being the latest. */
        float read (int delay) const noexcept;
        float readFractional (float delay) const noexcept;

        std::vector<float> data;
        int mask = 0;
        int writeIndex = 0;
    };

    struct Allpass
    {
        float process (float input, float coefficient, float delay) noexcept;

        DelayBuffer buffer;
    };

    int scaled (int samplesAtReferenceRate) const noexcept;

    double sampleRate = 44100.0;
    double modulationPhase = 0.0;
    float inputPole = 0.0f;
    float bandwidthState = 0.0f;
    std::array<float, 2> dampingStates {};
    float lastLeftTank = 0.0f, lastRightTank = 0.0f;

    DelayBuffer predelay;
    std::array<Allpass, 4> diffusers;
    std::array<Allpass, 2> modulatedAllpasses, decayAllpasses;
    std::array<DelayBuffer, 2> firstDelays, secondDelays;

    yup::SmoothedValue<float> predelaySamples, decay, dampingPole, width, mix;
};
