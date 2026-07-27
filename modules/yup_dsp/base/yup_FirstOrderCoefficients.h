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
    First-order filter coefficient storage.

    Stores coefficients for first-order IIR filters in the form:
    y[n] = b0*x[n] + b1*x[n-1] - a1*y[n-1]

    Uses CoeffType for internal precision (default double) while supporting
    different SampleType for audio processing.
*/
template <typename CoeffType = double>
struct FirstOrderCoefficients
{
    CoeffType a1 = 0;         // Feedback coefficient
    CoeffType b0 = 1, b1 = 0; // Feedforward coefficients

    FirstOrderCoefficients() = default;

    FirstOrderCoefficients (CoeffType b0_, CoeffType b1_, CoeffType a1_) noexcept
        : a1 (a1_)
        , b0 (b0_)
        , b1 (b1_)
    {
    }

    /** Returns the complex frequency response for these coefficients */
    Complex<CoeffType> getComplexResponse (CoeffType frequency, double sampleRate) const noexcept
    {
        const auto omega = frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate));
        const auto z = polar (static_cast<CoeffType> (1.0), -omega);

        auto numerator = Complex<CoeffType> (b0) + Complex<CoeffType> (b1) * z;
        auto denominator = Complex<CoeffType> (1.0) + Complex<CoeffType> (a1) * z;

        return numerator / denominator;
    }
};

} // namespace yup
