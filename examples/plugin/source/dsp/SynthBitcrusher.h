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
/** A bitcrusher: quantizes to a fractional bit depth and holds samples at a lower rate.

    The bit depth and the hold rate are smoothed, so turning or modulating them never
    clicks. Optional TPDF dither trades the quantization buzz for noise.
*/
class SynthBitcrusher
{
public:
    /** Prepares the smoothing for a sample rate. */
    void prepare (double sampleRate);

    /** Clears the held samples and jumps the smoothing to its targets. */
    void reset() noexcept;

    /** Crushes a stereo chunk in place. */
    void process (float* left, float* right, int numSamples, const SynthBitcrusherValues& values) noexcept;

private:
    double sampleRate = 44100.0;
    yup::SmoothedValue<float> bits, rate, dither, mix;
    double holdPhase = 1.0;
    std::array<float, 2> held {};
    yup::Random random;
};
