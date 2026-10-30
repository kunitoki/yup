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

#include <yup_core/yup_core.h>

#include <algorithm>
#include <cmath>
#include <utility>

//==============================================================================
/** Small pieces the master effects share. */
namespace SynthEffectTools
{
/** How long a control takes to reach a new value, so automation and modulation never step. */
constexpr double smoothingSeconds = 0.02;

/** Returns the dry and wet gains of a mix for a wet signal unlike the dry one, such as an
    echo or a chorus: both play at full level in the middle, as on a mixer send. */
inline std::pair<float, float> sendGains (float mix) noexcept
{
    return { yup::jmin (1.0f, 2.0f * (1.0f - mix)), yup::jmin (1.0f, 2.0f * mix) };
}

/** Returns true if a value is too small to hear, so an effect can skip silence. */
inline bool isQuiet (float value) noexcept
{
    return std::abs (value) < 1.0e-6f;
}

/** Returns true if every sample of a buffer is too small to hear. */
inline bool isQuiet (const float* data, int numSamples) noexcept
{
    return std::all_of (data, data + numSamples, [] (float value) { return isQuiet (value); });
}

/** Advances a phase in [0, 1). */
inline void advance (double& phase, double increment) noexcept
{
    phase += increment;
    phase -= std::floor (phase);
}

/** Returns a triangle in [-1, 1] at a phase in [0, 1), 1 at the start of the cycle. */
inline float triangle (double phase) noexcept
{
    return static_cast<float> (4.0 * std::abs (phase - 0.5) - 1.0);
}

/** Returns a sine in [-1, 1] at a phase in [0, 1).

    A polynomial on the quarter wave, within 4e-6 of std::sin: plenty for an LFO and far
    cheaper than the library call it replaces on every sample.
*/
inline float sine (double phase) noexcept
{
    // Fold the phase into [-0.25, 0.25], where sin (2 pi t) is odd and monotonic.
    auto t = static_cast<float> (phase);

    if (t > 0.75f)
        t -= 1.0f;
    else if (t > 0.25f)
        t = 0.5f - t;

    const auto x = yup::MathConstants<float>::twoPi * t;
    const auto x2 = x * x;
    return x * (1.0f + x2 * (-1.0f / 6.0f + x2 * (1.0f / 120.0f + x2 * (-1.0f / 5040.0f + x2 * (1.0f / 362880.0f)))));
}
} // namespace SynthEffectTools
