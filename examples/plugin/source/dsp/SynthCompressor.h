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
/** A stereo linked compressor with a soft knee.

    The detector follows the louder channel's peak in decibels, the gain reduction above
    the threshold rises with the attack and falls with the release, and makeup gain
    applies after. The mix blends in the uncompressed signal for parallel compression.
*/
class SynthCompressor
{
public:
    /** Prepares the smoothing for a sample rate. */
    void prepare (double sampleRate);

    /** Releases the gain reduction and jumps the smoothing to its targets. */
    void reset() noexcept;

    /** Compresses a stereo chunk in place. */
    void process (float* left, float* right, int numSamples, const SynthCompressorValues& values) noexcept;

private:
    double sampleRate = 44100.0;
    yup::SmoothedValue<float> threshold, ratio, makeup, mix;
    float reductionDb = 0.0f;
};
