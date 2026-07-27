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
    Udo Zoelzer biquad filters implementation.

    This class implements the biquad filters from "Digital Audio Signal Processing" by Udo Zoelzer. These filters
    use a different coefficient calculation approach compared to RBJ filters, based on the tangent of half the
    normalized frequency.

    Features:
    - Low-pass and high-pass filters
    - Peaking/bell filters with adjustable gain and Q
    - Low-shelf and high-shelf filters
    - Band-pass filters (constant skirt gain and constant peak gain variants)
    - Notch and all-pass filters
    - Based on K = tan(omega/2) where omega = 2*PI*frequency/sample_rate

    Reference: "Digital Audio Signal Processing" by Udo Zoelzer (John Wiley & Sons, ISBN 0 471 97226 6)

    @see Biquad, FilterBase, RbjFilter
*/
template <typename SampleType, typename CoeffType = double>
class ZoelzerFilter : public BiquadFilter<SampleType, CoeffType>
{
    using BaseFilterType = BiquadFilter<SampleType, CoeffType>;

public:
    //==============================================================================
    /** Default constructor */
    ZoelzerFilter() noexcept = default;

    /** Constructor with optional initial parameters */
    explicit ZoelzerFilter (FilterModeType mode) noexcept
        : BaseFilterType (mode)
    {
    }

private:
    //==============================================================================
    void updateCoefficients() override
    {
        auto coeffs = FilterDesigner<CoeffType>::designZoelzer (
            this->filterMode, this->centerFreq, this->qFactor, this->gain, this->sampleRate);

        BaseFilterType::setCoefficients (coeffs);
    }

    //==============================================================================
    YUP_LEAK_DETECTOR (ZoelzerFilter)
};

//==============================================================================
/** Type aliases for convenience */
using ZoelzerFilterFloat = ZoelzerFilter<float>;   // float samples, double coefficients (default)
using ZoelzerFilterDouble = ZoelzerFilter<double>; // double samples, double coefficients (default)

} // namespace yup
