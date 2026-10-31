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
/** A stereo flanger: a short swept delay with bipolar feedback.

    Each channel reads a yup::FractionallyAddressedDelay swept by a triangle LFO, from the
    base delay down towards zero and up to twice it at full depth. The right channel's LFO
    runs up to half a cycle ahead.
*/
class SynthFlanger
{
public:
    /** Allocates the delays for a sample rate. */
    void prepare (double sampleRate);

    /** Clears the delays. */
    void reset() noexcept;

    /** Flanges a stereo chunk in place. */
    void process (float* left, float* right, int numSamples, const SynthFlangerValues& values) noexcept;

private:
    double sampleRate = 44100.0;
    double phase = 0.0;
    std::array<yup::FractionallyAddressedDelay<float>, 2> delays;
    std::array<float, 2> lastWet {};
    yup::SmoothedValue<float> rate, depth, delayMs, feedback, stereo, mix;
};
