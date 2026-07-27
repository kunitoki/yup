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
    Robert Bristow-Johnson (RBJ) cookbook filters.

    This class implements the classic "Audio EQ Cookbook" biquad filters,
    widely used in audio applications for equalization and filtering.

    Features:
    - Peaking/bell filters with adjustable gain and Q
    - Low-shelf and high-shelf filters
    - Lowpass, highpass, bandpass, and notch filters
    - All filters based on analog prototypes with bilinear transform
    - Frequency, Q, and gain controls

    Reference: "Cookbook formulae for audio EQ biquad filter coefficients"
    by Robert Bristow-Johnson

    @see Biquad, FilterBase
*/
template <typename SampleType, typename CoeffType = double>
class RbjFilter : public BiquadFilter<SampleType, CoeffType>
{
    using BaseFilterType = BiquadFilter<SampleType, CoeffType>;

public:
    //==============================================================================
    /** Default constructor */
    RbjFilter() noexcept = default;

    /** Constructor with optional initial parameters */
    explicit RbjFilter (FilterModeType mode) noexcept
        : BaseFilterType (mode)
    {
    }

private:
    //==============================================================================
    void updateCoefficients() override
    {
        auto coeffs = FilterDesigner<CoeffType>::designRbj (
            this->filterMode, this->centerFreq, this->qFactor, this->gain, this->sampleRate);

        BaseFilterType::setCoefficients (coeffs);
    }

    //==============================================================================
    YUP_LEAK_DETECTOR (RbjFilter)
};

//==============================================================================
/** Type aliases for convenience */
using RbjFilterFloat = RbjFilter<float>;   // float samples, double coefficients (default)
using RbjFilterDouble = RbjFilter<double>; // double samples, double coefficients (default)

} // namespace yup
