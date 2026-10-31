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
/** A stereo chorus with a generic mode and the three Roland Juno-60 modes.

    Each channel reads a modulated yup::FractionallyAddressedDelay, so the delay sweep is a
    smooth pitch glide. The generic mode runs a sine LFO in quadrature between the channels,
    with rate, depth, delay and feedback from the controls. The Juno modes play the fixed
    triangle sweeps of the original bucket brigade chorus, inverted on the right channel,
    through a darker wet path, and ignore those controls.
*/
class SynthChorus
{
public:
    /** Allocates the delays for a sample rate. */
    void prepare (double sampleRate);

    /** Clears the delays and the wet tone filter. */
    void reset() noexcept;

    /** Choruses a stereo chunk in place. */
    void process (float* left, float* right, int numSamples, const SynthChorusValues& values) noexcept;

private:
    double sampleRate = 44100.0;
    double phase = 0.0;
    std::array<yup::FractionallyAddressedDelay<float>, 2> delays;
    std::array<yup::FirstOrderFilter<float>, 2> tone;
    std::array<float, 2> lastWet {};
    yup::SmoothedValue<float> rate, centerMs, depthMs, feedback, mix, toneAmount;
};
