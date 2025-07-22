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
    Butterworth filter coefficient calculator and implementation.

    Butterworth filters provide maximally flat passband response with no ripple
    in either passband or stopband. They offer the best phase response among
    classical filter types but have the gentlest rolloff.

    Features:
    - Orders 1-20 supported
    - Lowpass, highpass, bandpass, bandstop configurations
    - Automatic biquad cascade generation
    - Stable coefficient calculation using analog prototypes

    @see BiquadCascade, FilterBase
*/
template <typename SampleType, typename CoeffType = double>
class ButterworthFilter : public FilterBase<SampleType, CoeffType>
{
public:
    //==============================================================================
    /** Default constructor */
    ButterworthFilter()
        : cascade (1)
    {
        setParameters (FilterType::lowpass, 2, static_cast<CoeffType> (1000.0), 44100.0);
    }

    /** Constructor with parameters */
    ButterworthFilter (FilterType type, int order, CoeffType frequency, double sampleRate)
        : cascade (calculateNumSections (order))
    {
        setParameters (type, order, frequency, sampleRate);
    }

    //==============================================================================
    /** @internal */
    void reset() noexcept override
    {
        cascade.reset();
    }

    /** @internal */
    void prepare (double sampleRate, int maximumBlockSize) noexcept override
    {
        this->sampleRate = sampleRate;
        this->maximumBlockSize = maximumBlockSize;
        cascade.prepare (sampleRate, maximumBlockSize);
        updateCoefficients();
    }

    /** @internal */
    SampleType processSample (SampleType inputSample) noexcept override
    {
        return cascade.processSample (inputSample);
    }

    /** @internal */
    void processBlock (const SampleType* inputBuffer, SampleType* outputBuffer, int numSamples) noexcept override
    {
        cascade.processBlock (inputBuffer, outputBuffer, numSamples);
    }

    /** @internal */
    DspMath::Complex<CoeffType> getComplexResponse (CoeffType frequency) const noexcept override
    {
        return cascade.getComplexResponse (frequency);
    }

    //==============================================================================
    /**
        Sets all filter parameters.

        @param type        The filter type (lowpass, highpass, etc.)
        @param order       The filter order (1-20)
        @param frequency   The cutoff frequency in Hz (or center frequency for bandpass/bandstop)
        @param sampleRate  The sample rate in Hz
        @param bandwidth   The bandwidth for bandpass/bandstop filters (default 1 octave)
    */
    void setParameters (FilterType type, int order, CoeffType frequency, double sampleRate, CoeffType bandwidth = static_cast<CoeffType> (1.0)) noexcept
    {
        filterType = type;
        filterOrder = jlimit (1, 20, order);
        cutoffFreq = frequency;
        bandwidthOctaves = bandwidth;
        this->sampleRate = sampleRate;

        const auto numSections = calculateNumSections (filterOrder);
        if (cascade.getNumSections() != static_cast<size_t> (numSections))
            cascade.setNumSections (numSections);

        // Pre-size coefficient storage to avoid allocation during updateCoefficients
        if (coefficientsStorage.size() != static_cast<size_t> (numSections))
            coefficientsStorage.resize (numSections);

        updateCoefficients();
    }

    /**
        Sets just the cutoff frequency.

        @param frequency  The new cutoff frequency in Hz
    */
    void setCutoffFrequency (CoeffType frequency) noexcept
    {
        cutoffFreq = frequency;
        updateCoefficients();
    }

    /**
        Sets just the filter order.

        @param order  The new filter order (1-20)
    */
    void setOrder (int order) noexcept
    {
        const auto newOrder = jlimit (1, 20, order);
        if (filterOrder != newOrder)
        {
            filterOrder = newOrder;
            const auto numSections = calculateNumSections (filterOrder);
            cascade.setNumSections (numSections);

            // Pre-size coefficient storage to avoid allocation during updateCoefficients
            if (coefficientsStorage.size() != static_cast<size_t> (numSections))
                coefficientsStorage.resize (numSections);

            updateCoefficients();
        }
    }

    /**
        Gets the current cutoff frequency.

        @returns  The cutoff frequency in Hz
    */
    CoeffType getCutoffFrequency() const noexcept
    {
        return cutoffFreq;
    }

    /**
        Gets the current filter order.

        @returns  The filter order
    */
    int getOrder() const noexcept
    {
        return filterOrder;
    }

    /**
        Gets the current filter type.

        @returns  The filter type
    */
    FilterType getFilterType() const noexcept
    {
        return filterType;
    }

private:
    //==============================================================================
    static int calculateNumSections (int order) noexcept
    {
        return (order + 1) / 2;
    }

    void updateCoefficients() noexcept
    {
        // Use vector reference versions to write to pre-allocated storage
        switch (filterType)
        {
            case FilterType::lowpass:
                FilterDesigner<CoeffType>::designButterworthLowpass (coefficientsStorage, filterOrder, cutoffFreq, this->sampleRate);
                break;

            case FilterType::highpass:
                FilterDesigner<CoeffType>::designButterworthHighpass (coefficientsStorage, filterOrder, cutoffFreq, this->sampleRate);
                break;

                /* TODO - Keep this for future implementation
            case FilterType::bandpass:
                FilterDesigner<CoeffType>::designButterworthBandpass (coefficientsStorage, filterOrder, cutoffFreq, bandwidthOctaves, this->sampleRate);
                break;

            case FilterType::bandstop:
                FilterDesigner<CoeffType>::designButterworthBandstop (coefficientsStorage, filterOrder, cutoffFreq, bandwidthOctaves, this->sampleRate);
                break;
            */

            case FilterType::allpass:
                FilterDesigner<CoeffType>::designButterworthAllpass (coefficientsStorage, filterOrder, this->sampleRate);
                break;

            default:
                FilterDesigner<CoeffType>::designButterworthLowpass (coefficientsStorage, filterOrder, cutoffFreq, this->sampleRate);
                break;
        }

        // Apply coefficients to cascade
        for (size_t i = 0; i < coefficientsStorage.size(); ++i)
            cascade.setSectionCoefficients (i, coefficientsStorage[i]);
    }

    //==============================================================================
    BiquadCascade<SampleType, CoeffType> cascade;

    FilterType filterType = FilterType::lowpass;
    int filterOrder = 2;
    CoeffType cutoffFreq = static_cast<CoeffType> (1000.0);
    CoeffType bandwidthOctaves = static_cast<CoeffType> (1.0);

    std::vector<BiquadCoefficients<CoeffType>> coefficientsStorage;

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ButterworthFilter)
};

//==============================================================================
/** Type aliases for convenience */
using ButterworthFilterFloat = ButterworthFilter<float>;   // float samples, double coefficients (default)
using ButterworthFilterDouble = ButterworthFilter<double>; // double samples, double coefficients (default)

} // namespace yup
