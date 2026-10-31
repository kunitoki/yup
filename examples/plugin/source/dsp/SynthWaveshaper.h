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

#include "SynthCurveState.h"
#include "SynthEffectsParameters.h"

#include <algorithm>
#include <array>

//==============================================================================
/** An antialiased waveshaper whose transfer curve is drawn, like a custom LFO cycle.

    The curve maps the input from -1 to 1 onto the output from -1 to 1 and holds its end
    levels beyond that range. It is sampled into a table whenever it changes, and the
    table runs through yup::AaIirAntialiaser, which integrates it against an
    antialiasing filter, so steep or folded curves stay clean without oversampling. The
    dry signal goes through the same filter with a straight line as the curve, so dry
    and wet stay phase aligned and the mix never combs.

    Drive sets how far into the curve the signal reaches and bias offsets it for
    asymmetric, even harmonics; the offset it leaves is removed afterwards. Unlike
    SynthDistortion the level is not matched: the drawn curve decides it, and the output
    gain trims it.

    @see SynthCurveState::getShaper()
*/
class SynthWaveshaper
{
public:
    SynthWaveshaper();

    /** Prepares the filters and the smoothing for a sample rate. */
    void prepare (double sampleRate);

    /** Clears the filters and jumps the smoothing to its targets. */
    void reset() noexcept;

    /** Picks up the drawn curve when it changed. Audio thread, never waits. */
    void updateCurve (const SynthCurveState& curves) noexcept;

    /** Shapes a stereo chunk in place. */
    void process (float* left, float* right, int numSamples, const SynthWaveshaperValues& values) noexcept;

private:
    static constexpr int tableSegments = 1024;
    using Table = std::array<float, tableSegments + 1>;

    /** The drawn curve as a piecewise linear function of a table; a straight line until one is set. */
    struct CurveTraits
    {
        const Table* table = nullptr;

        template <typename T>
        T f (T x) const noexcept
        {
            if (table == nullptr)
                return x;

            const auto position = (std::clamp (x, T (-1), T (1)) + T (1)) * T (0.5 * tableSegments);
            const auto index = yup::jmin (static_cast<int> (position), tableSegments - 1);
            const auto from = static_cast<T> ((*table)[static_cast<std::size_t> (index)]);
            const auto to = static_cast<T> ((*table)[static_cast<std::size_t> (index + 1)]);
            return from + (to - from) * (position - static_cast<T> (index));
        }
    };

    struct LinearTraits
    {
        template <typename T>
        static T f (T x) noexcept { return x; }
    };

    struct Channel
    {
        yup::AaIirAntialiaser<float, CurveTraits> wet;
        yup::AaIirAntialiaser<float, LinearTraits> dry;
        yup::FirstOrderFilter<float> dcBlocker;
    };

    void fillTable (const SynthCurveState::Curve& curve) noexcept;

    Table table {};
    SynthCurveState::Curve curve;
    int curveGeneration = -1;
    std::array<Channel, 2> channels;
    yup::SmoothedValue<float> drive, bias, output, mix;
    bool outputQuiet = false;
};
