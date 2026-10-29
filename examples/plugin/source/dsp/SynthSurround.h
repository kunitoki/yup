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
/** A mid/side stereo widener with a short delayed ambience.

    The sides are scaled by the width, then a delayed copy of the mid is added to them,
    out of phase between the channels, so a mono source opens up Haas style while
    summing back to mono unchanged. Below the mono frequency the sides are cut, keeping
    the bass centered.
*/
class SynthSurround
{
public:
    /** Prepares the delay line and the smoothing for a sample rate. */
    void prepare (double sampleRate);

    /** Clears the delay line and jumps the smoothing to its targets. */
    void reset() noexcept;

    /** Widens a stereo chunk in place. */
    void process (float* left, float* right, int numSamples, const SynthSurroundValues& values) noexcept;

private:
    float readDelayed (float delaySamples) const noexcept;

    double sampleRate = 44100.0;
    std::vector<float> delayLine;
    int writeIndex = 0;
    yup::FirstOrderFilter<float> sideHighpass;
    yup::SmoothedValue<float> width, depth, delaySamples, monoBelow, mix;
};
