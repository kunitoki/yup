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

#include <algorithm>

//==============================================================================
/** An antialiased distortion: soft, cubic or hard clipping of the driven signal.

    The curve runs through yup::AaIirAntialiaser, which integrates it against an
    antialiasing filter, so heavy drive stays clean without oversampling. The dry
    signal goes through the same filter with a straight line as the curve, so dry
    and wet stay phase aligned and the mix never combs.

    Bias offsets the signal before the curve for asymmetric, even harmonics; the offset
    it leaves is removed afterwards. The output follows the input's loudness, so the
    drive changes the character and not the level, then the tone lowpass and the output
    gain apply.
*/
class SynthDistortion
{
public:
    /** Prepares the filters and the smoothing for a sample rate. */
    void prepare (double sampleRate);

    /** Clears the filters and jumps the smoothing to its targets. */
    void reset() noexcept;

    /** Distorts a stereo chunk in place. */
    void process (float* left, float* right, int numSamples, const SynthDistortionValues& values) noexcept;

private:
    /** Cubic soft clipping, 1.5 (x - x^3 / 3), flat beyond +-1. */
    struct CubicTraits
    {
        template <typename T>
        static T f (T x) noexcept
        {
            const auto clamped = std::clamp (x, T (-1), T (1));
            return T (1.5) * (clamped - clamped * clamped * clamped / T (3));
        }

        template <typename T>
        static void fillBreakpoints (T x0, T delta, T* points, int& numPoints) noexcept
        {
            yup::HardClipperTraits::fillBreakpoints (x0, delta, points, numPoints);
        }
    };

    struct LinearTraits
    {
        template <typename T>
        static T f (T x) noexcept { return x; }
    };

    template <typename Traits>
    using Shaper = yup::AaIirAntialiaser<float, Traits>;

    struct Channel
    {
        Shaper<yup::TanhClipperTraits> soft;
        Shaper<CubicTraits> cubic;
        Shaper<yup::HardClipperTraits> hard;
        Shaper<LinearTraits> dry;
        yup::FirstOrderFilter<float> dcBlocker, tone;
    };

    float shape (Channel& channel, float input) noexcept;
    static float curve (SynthDistortionType type, float x) noexcept;

    double sampleRate = 44100.0;
    SynthDistortionType type = SynthDistortionType::soft;
    std::array<Channel, 2> channels;
    yup::SmoothedValue<float> drive, bias, tone, output, mix;
    float levelCoefficient = 0.0f, dryPower = 0.0f, wetPower = 0.0f;
    bool outputQuiet = false;
};
