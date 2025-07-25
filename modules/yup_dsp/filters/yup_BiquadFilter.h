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
    Biquad filter base.

    @see Biquad, FilterBase
*/
template <typename SampleType, typename CoeffType = double>
class BiquadFilter : public Biquad<SampleType, CoeffType>
{
    using BaseFilterType = Biquad<SampleType, CoeffType>;

public:
    //==============================================================================
    /** Default constructor */
    BiquadFilter()
    {
        setParameters (FilterMode::lowpass, static_cast<CoeffType> (1000.0), static_cast<CoeffType> (0.707), static_cast<CoeffType> (0.0), 44100.0);
    }

    /** Constructor with optional initial parameters */
    explicit BiquadFilter (FilterModeType mode)
    {
        setParameters (mode, static_cast<CoeffType> (1000.0), static_cast<CoeffType> (0.707), static_cast<CoeffType> (0.0), 44100.0);
    }

    //==============================================================================
    /**
        Sets all filter parameters.

        @param mode        The filter mode
        @param frequency   The center/cutoff frequency in Hz
        @param q           The Q factor (resonance/bandwidth control)
        @param gainDb      The gain in decibels (for peaking and shelving filters)
        @param sampleRate  The sample rate in Hz
    */
    void setParameters (FilterModeType mode, CoeffType frequency, CoeffType q, CoeffType gainDb, double sampleRate) noexcept
    {
        mode = resolveFilterMode (mode, this->getSupportedModes());

        if (filterMode != mode
            || ! approximatelyEqual (centerFreq, frequency)
            || ! approximatelyEqual (qFactor, q)
            || ! approximatelyEqual (gain, gainDb)
            || ! approximatelyEqual (this->sampleRate, sampleRate))
        {
            filterMode = mode;
            centerFreq = frequency;
            qFactor = q;
            gain = gainDb;

            this->sampleRate = sampleRate;

            updateCoefficients();
        }
    }

    /**
        Sets just the center/cutoff frequency.

        @param frequency  The new frequency in Hz
    */
    void setFrequency (CoeffType frequency) noexcept
    {
        if (! approximatelyEqual (centerFreq, frequency))
        {
            centerFreq = frequency;

            updateCoefficients();
        }
    }

    /**
        Sets just the Q factor.

        @param q  The new Q factor
    */
    void setQ (CoeffType q) noexcept
    {
        if (! approximatelyEqual (qFactor, q))
        {
            qFactor = q;

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

        @param mode  The filter mode
    */
    void setMode (FilterModeType mode) noexcept
    {
        mode = resolveFilterMode (mode, this->getSupportedModes());

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
        Gets the current Q factor.

        @returns  The Q factor
    */
    CoeffType getQ() const noexcept
    {
        return qFactor;
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

        @returns  The filter mode
    */
    FilterModeType getMode() const noexcept
    {
        return filterMode;
    }

    //==============================================================================
    /** @internal */
    void prepare (double sampleRate, int maximumBlockSize) override
    {
        BaseFilterType::prepare (sampleRate, maximumBlockSize);

        updateCoefficients();
    }

protected:
    //==============================================================================
    virtual void updateCoefficients() = 0;

    //==============================================================================
    FilterModeType filterMode = FilterMode::lowpass;
    CoeffType centerFreq = static_cast<CoeffType> (1000.0);
    CoeffType qFactor = static_cast<CoeffType> (0.707);
    CoeffType gain = static_cast<CoeffType> (0.0);

private:
    //==============================================================================
    YUP_LEAK_DETECTOR (BiquadFilter)
};

//==============================================================================
/** Type aliases for convenience */
using BiquadFilterFloat = BiquadFilter<float>;   // float samples, double coefficients (default)
using BiquadFilterDouble = BiquadFilter<double>; // double samples, double coefficients (default)

} // namespace yup
