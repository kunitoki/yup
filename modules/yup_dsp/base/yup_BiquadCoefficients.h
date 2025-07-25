/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

namespace yup
{

//==============================================================================
/**
    Filter coefficient storage for biquad filters.

    Stores the coefficients for a second-order IIR filter in the form:
    y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]

    Uses CoeffType for internal precision (default double) while supporting
    different SampleType for audio processing.
*/
template <typename CoeffType = double>
struct BiquadCoefficients
{
    CoeffType a0 = 1, a1 = 0, a2 = 0; // Denominator coefficients (a0 is typically normalized to 1)
    CoeffType b0 = 1, b1 = 0, b2 = 0; // Numerator coefficients

    BiquadCoefficients() = default;

    BiquadCoefficients (CoeffType b0_, CoeffType b1_, CoeffType b2_, CoeffType a0_, CoeffType a1_) noexcept
        : a0 (a0_)
        , a1 (a1_)
        , a2 (0.0f)
        , b0 (b0_)
        , b1 (b1_)
        , b2 (b2_)
    {
    }

    BiquadCoefficients (CoeffType b0_, CoeffType b1_, CoeffType b2_, CoeffType a0_, CoeffType a1_, CoeffType a2_) noexcept
        : a0 (a0_)
        , a1 (a1_)
        , a2 (a2_)
        , b0 (b0_)
        , b1 (b1_)
        , b2 (b2_)
    {
    }

    /** Normalizes coefficients so that a0 = 1 */
    void normalize() noexcept
    {
        if (a0 != static_cast<CoeffType> (0.0))
        {
            b0 /= a0;
            b1 /= a0;
            b2 /= a0;
            a1 /= a0;
            a2 /= a0;
            a0 = static_cast<CoeffType> (1.0);
        }
    }

    /** Returns the complex frequency response for these coefficients */
    Complex<CoeffType> getComplexResponse (CoeffType frequency, double sampleRate) const noexcept
    {
        const auto omega = frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate));
        const auto z = polar (static_cast<CoeffType> (1.0), -omega);
        const auto z2 = z * z;

        auto numerator = Complex<CoeffType> (b0) + Complex<CoeffType> (b1) * z + Complex<CoeffType> (b2) * z2;
        auto denominator = Complex<CoeffType> (a0) + Complex<CoeffType> (a1) * z + Complex<CoeffType> (a2) * z2;

        return numerator / denominator;
    }
};

} // namespace yup
