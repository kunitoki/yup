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

namespace yup
{

//==============================================================================
/** Clips the input value to the range [-4, 4] using a hyperbolic tangent function.

    This function is used to limit the feedback in analog-model filters to prevent
    instability while preserving the character of the resonance.

    @param input The input value to clip

    @return The clipped output value
*/
template <typename CoeffType>
CoeffType clipAnalogResonance (CoeffType input) noexcept
{
    return std::tanh (input * static_cast<CoeffType> (0.25)) * static_cast<CoeffType> (4);
}

/** Analog saturator structure.
 
    This structure implements a simple symmetric atan-based waveshaper for analog
    saturation effects. The drive parameter controls the amount of saturation, while
    preScale and postScale allow for adjusting the input and output levels to
    achieve the desired tonal characteristics.
*/
template <typename SampleType, typename CoeffType>
struct AnalogSaturator
{
    void setCoefficients (const AnalogSaturatorCoefficients<CoeffType>& newCoefficients) noexcept
    {
        coefficients = newCoefficients;
    }

    SampleType process (SampleType input) const noexcept
    {
        if (coefficients.drive <= static_cast<CoeffType> (0))
            return input;

        return static_cast<SampleType> (
            std::atan (static_cast<CoeffType> (input) * coefficients.preScale) * coefficients.postScale);
    }

    AnalogSaturatorCoefficients<CoeffType> coefficients;
};

} // namespace yup
