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
    explicit ZoelzerFilter (FilterMode mode) noexcept
        : BaseFilterType (mode)
    {
    }

private:
    //==============================================================================
    void updateCoefficients() override
    {
        BiquadCoefficients<CoeffType> coeffs;

        switch (this->filterMode)
        {
            case FilterMode::lowpass:
                coeffs = FilterDesigner<CoeffType>::designZoelzerLowpass (this->centerFreq, this->qFactor, this->sampleRate);
                break;

            case FilterMode::highpass:
                coeffs = FilterDesigner<CoeffType>::designZoelzerHighpass (this->centerFreq, this->qFactor, this->sampleRate);
                break;

            case FilterMode::bandpass:
                coeffs = FilterDesigner<CoeffType>::designZoelzerBandpassCsg (this->centerFreq, this->qFactor, this->sampleRate);
                break;

            //case Mode::bandpassCpg:
            //    coeffs = FilterDesigner<CoeffType>::designZoelzerBandpassCpg (this->centerFreq, this->qFactor, this->sampleRate);
            //    break;

            case FilterMode::bandstop:
                coeffs = FilterDesigner<CoeffType>::designZoelzerNotch (this->centerFreq, this->qFactor, this->sampleRate);
                break;

            case FilterMode::peak:
                coeffs = FilterDesigner<CoeffType>::designZoelzerPeaking (this->centerFreq, this->qFactor, this->gain, this->sampleRate);
                break;

            case FilterMode::lowshelf:
                coeffs = FilterDesigner<CoeffType>::designZoelzerLowShelf (this->centerFreq, this->qFactor, this->gain, this->sampleRate);
                break;

            case FilterMode::highshelf:
                coeffs = FilterDesigner<CoeffType>::designZoelzerHighShelf (this->centerFreq, this->qFactor, this->gain, this->sampleRate);
                break;

            case FilterMode::allpass:
                coeffs = FilterDesigner<CoeffType>::designZoelzerAllpass (this->centerFreq, this->qFactor, this->sampleRate);
                break;
        }

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
