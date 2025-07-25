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
        BiquadCoefficients<CoeffType> coeffs;

        if (this->filterMode.test (FilterMode::lowpass))
        {
            coeffs = FilterDesigner<CoeffType>::designZoelzerLowpass (this->centerFreq, this->qFactor, this->sampleRate);
        }
        else if (this->filterMode.test (FilterMode::highpass))
        {
            coeffs = FilterDesigner<CoeffType>::designZoelzerHighpass (this->centerFreq, this->qFactor, this->sampleRate);
        }
        else if (this->filterMode.test (FilterMode::bandpassCsg))
        {
            coeffs = FilterDesigner<CoeffType>::designZoelzerBandpassCsg (this->centerFreq, this->qFactor, this->sampleRate);
        }
        else if (this->filterMode.test (FilterMode::bandpassCpg))
        {
            coeffs = FilterDesigner<CoeffType>::designZoelzerBandpassCpg (this->centerFreq, this->qFactor, this->sampleRate);
        }
        else if (this->filterMode.test (FilterMode::bandstop))
        {
            coeffs = FilterDesigner<CoeffType>::designZoelzerNotch (this->centerFreq, this->qFactor, this->sampleRate);
        }
        else if (this->filterMode.test (FilterMode::peak))
        {
            coeffs = FilterDesigner<CoeffType>::designZoelzerPeaking (this->centerFreq, this->qFactor, this->gain, this->sampleRate);
        }
        else if (this->filterMode.test (FilterMode::lowshelf))
        {
            coeffs = FilterDesigner<CoeffType>::designZoelzerLowShelf (this->centerFreq, this->qFactor, this->gain, this->sampleRate);
        }
        else if (this->filterMode.test (FilterMode::highshelf))
        {
            coeffs = FilterDesigner<CoeffType>::designZoelzerHighShelf (this->centerFreq, this->qFactor, this->gain, this->sampleRate);
        }
        else if (this->filterMode.test (FilterMode::allpass))
        {
            coeffs = FilterDesigner<CoeffType>::designZoelzerAllpass (this->centerFreq, this->qFactor, this->sampleRate);
        }
        else if (this->filterMode.test (FilterMode::bandpass))
        {
            // Handle composite bandpass mode by defaulting to CSG variant
            // Choose the most appropriate bandpass variant
            if (FilterCapabilities<ZoelzerFilter>::supportedModes.test (FilterMode::bandpassCsg))
                coeffs = FilterDesigner<CoeffType>::designZoelzerBandpassCsg (this->centerFreq, this->qFactor, this->sampleRate);
            else
                coeffs = FilterDesigner<CoeffType>::designZoelzerBandpassCpg (this->centerFreq, this->qFactor, this->sampleRate);
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

//==============================================================================
/** Zoelzer Filter capabilities specialization - supports both bandpass variants */
template <>
struct FilterCapabilities<ZoelzerFilter<float>>
{
    static constexpr auto supportedModes =
        FilterMode::lowpass | FilterMode::highpass | FilterMode::bandpassCsg | FilterMode::bandpassCpg | FilterMode::bandstop |
        FilterMode::peak | FilterMode::lowshelf | FilterMode::highshelf | FilterMode::allpass;
};

template <>
struct FilterCapabilities<ZoelzerFilter<double>>
{
    static constexpr auto supportedModes = 
        FilterMode::lowpass | FilterMode::highpass | FilterMode::bandpassCsg | FilterMode::bandpassCpg | FilterMode::bandstop |
        FilterMode::peak | FilterMode::lowshelf | FilterMode::highshelf | FilterMode::allpass;
};

} // namespace yup
