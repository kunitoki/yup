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
/** A stereo phaser: up to twelve first order allpass stages swept by a sine LFO.

    The sweep is exponential around the center frequency, three octaves each way at full
    depth, and the right channel's LFO runs up to half a cycle ahead for a wider image.
*/
class SynthPhaser
{
public:
    /** Prepares the smoothing for a sample rate. */
    void prepare (double sampleRate);

    /** Clears the allpass memory. */
    void reset() noexcept;

    /** Phases a stereo chunk in place. */
    void process (float* left, float* right, int numSamples, const SynthPhaserValues& values) noexcept;

private:
    static constexpr int maximumStages = 12;

    double sampleRate = 44100.0;
    double phase = 0.0;
    std::array<std::array<float, maximumStages>, 2> states {};
    std::array<float, 2> lastWet {};
    yup::SmoothedValue<float> rate, depth, center, feedback, stereo, mix;
};
