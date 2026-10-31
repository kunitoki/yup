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

//==============================================================================
/** A three band equalizer: a low shelf, a peak and a high shelf.

    The controls are smoothed and the coefficients follow them once per chunk, so
    sweeping a band never steps.
*/
class SynthEqualizer
{
public:
    /** Prepares the filters and the smoothing for a sample rate. */
    void prepare (double sampleRate);

    /** Clears the filters and jumps the smoothing to its targets. */
    void reset() noexcept;

    /** Equalizes a stereo chunk in place. */
    void process (float* left, float* right, int numSamples, const SynthEqualizerValues& values) noexcept;

private:
    struct Band
    {
        std::array<yup::RbjFilter<float>, 2> filters;
        yup::SmoothedValue<float> gain, frequency, q;
    };

    double sampleRate = 44100.0;
    Band low, mid, high;
};
