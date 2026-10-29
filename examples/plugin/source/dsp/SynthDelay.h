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
/** A clickless stereo or ping-pong delay with filtered feedback.

    Each channel runs a yup::FractionallyAddressedDelay. The delay time eases towards its
    target and the delay turns every change into a tape-like pitch glide, so sweeping or
    modulating the time, or changing the tempo, never clicks. The low and high cut filter
    every repeat, inside the feedback loop.
*/
class SynthDelay
{
public:
    /** Allocates the delays for a sample rate. */
    void prepare (double sampleRate);

    /** Clears the delays and the filters, and jumps the time to its next target. */
    void reset() noexcept;

    /** Delays a stereo chunk in place.

        @param bpm  The host tempo used when synced, or zero when unknown
    */
    void process (float* left, float* right, int numSamples, const SynthDelayValues& values, double bpm) noexcept;

private:
    double sampleRate = 44100.0;
    double timeSamples = 0.0;
    double timeCoefficient = 0.0;
    bool jumpTime = true;
    std::array<yup::FractionallyAddressedDelay<float>, 2> delays;
    std::array<yup::FirstOrderFilter<float>, 2> lowCuts, highCuts;
    std::array<float, 2> lastWet {};
    yup::SmoothedValue<float> feedback, pingPong, mix;
};
