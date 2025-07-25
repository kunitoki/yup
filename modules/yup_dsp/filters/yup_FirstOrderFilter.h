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
    First-order IIR filter implementation.

    This class implements various first-order filters including:
    - One-pole lowpass and highpass filters
    - First-order shelving filters
    - Allpass filters

    @see FirstOrder
*/
template <typename SampleType, typename CoeffType = double>
class FirstOrderFilter : public FirstOrder<SampleType, CoeffType>
{
    using BaseFilterType = FirstOrder<SampleType, CoeffType>;

public:
    //==============================================================================
    /** Default constructor */
    FirstOrderFilter() = default;

    //==============================================================================
    /**
        Sets the filter parameters.

        @param frequency   The cutoff frequency in Hz
        @param q          The Q factor (resonance)
        @param sampleRate The sample rate in Hz
    */
    void setParameters (FilterModeType mode, CoeffType frequency, CoeffType gainDb, double sampleRate) noexcept
    {
        mode = resolveFilterMode (mode, getSupportedModes());

        if (filterMode != mode
            || ! approximatelyEqual (centerFreq, frequency)
            || ! approximatelyEqual (gain, gainDb)
            || ! approximatelyEqual (this->sampleRate, sampleRate))
        {
            filterMode = mode;
            centerFreq = frequency;
            gain = gainDb;

            this->sampleRate = sampleRate;

            updateCoefficients();
        }
    }

    /**
        Sets just the cutoff frequency.

        @param frequency  The new cutoff frequency in Hz
    */
    void setCutoffFrequency (CoeffType frequency) noexcept
    {
        if (! approximatelyEqual (centerFreq, frequency))
        {
            centerFreq = frequency;

            updateCoefficients();
        }
    }

    /**
        Sets just the gain (for peaking and shelving filters).

        @param gainDb  The new gain in decibels
    */
    void setGain (CoeffType gainDb) noexcept
    {
        if (! approximatelyEqual (gain, gainDb))
        {
            gain = gainDb;

            updateCoefficients();
        }
    }

    /**
        Sets the filter mode.

        @param mode  The new RBJ filter mode
    */
    void setMode (FilterModeType mode) noexcept
    {
        if (filterMode != mode)
        {
            filterMode = mode;

            updateCoefficients();
        }
    }

    /**
        Gets the current frequency.

        @returns  The center/cutoff frequency in Hz
    */
    CoeffType getFrequency() const noexcept
    {
        return centerFreq;
    }

    /**
        Gets the current gain.

        @returns  The gain in decibels
    */
    CoeffType getGain() const noexcept
    {
        return gain;
    }

    /**
        Gets the current filter mode.

        @returns  The RBJ filter mode
    */
    FilterModeType getMode() const noexcept
    {
        return filterMode;
    }

    //==============================================================================
    /** @internal */
    FilterModeType getSupportedModes() const noexcept override
    {
        return FilterMode::lowpass | FilterMode::highpass | FilterMode::lowshelf | FilterMode::highshelf | FilterMode::allpass;
    }

protected:
    //==============================================================================
    virtual void updateCoefficients()
    {
        FirstOrderCoefficients<CoeffType> coeffs;

        if (this->filterMode.test (FilterMode::lowpass))
            coeffs = FilterDesigner<CoeffType>::designFirstOrderLowpass (centerFreq, this->sampleRate);

        else if (this->filterMode.test (FilterMode::highpass))
            coeffs = FilterDesigner<CoeffType>::designFirstOrderHighpass (centerFreq, this->sampleRate);

        else if (this->filterMode.test (FilterMode::lowshelf))
            coeffs = FilterDesigner<CoeffType>::designFirstOrderLowShelf (centerFreq, gain, this->sampleRate);

        else if (this->filterMode.test (FilterMode::highshelf))
            coeffs = FilterDesigner<CoeffType>::designFirstOrderHighShelf (centerFreq, gain, this->sampleRate);

        else if (this->filterMode.test (FilterMode::allpass))
            coeffs = FilterDesigner<CoeffType>::designFirstOrderAllpass (centerFreq, this->sampleRate);

        else
            coeffs = FilterDesigner<CoeffType>::designFirstOrderLowpass (centerFreq, this->sampleRate);

        BaseFilterType::setCoefficients (coeffs);
    }

    //==============================================================================
    FilterModeType filterMode = FilterMode::lowpass;
    CoeffType centerFreq = static_cast<CoeffType> (1000.0);
    CoeffType gain = static_cast<CoeffType> (0.0);

private:
    //==============================================================================
    YUP_LEAK_DETECTOR (FirstOrderFilter)
};

//==============================================================================
/** Type aliases for convenience */
using FirstOrderFilterFloat = FirstOrderFilter<float>;   // float samples, double coefficients (default)
using FirstOrderFilterDouble = FirstOrderFilter<double>; // double samples, double coefficients (default)

} // namespace yup
