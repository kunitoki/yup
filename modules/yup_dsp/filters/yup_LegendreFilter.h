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

#include <vector>

namespace yup
{

//==============================================================================
/** 
    Legendre (Optimum-L) filter implementation with optimal monotonic response.
    
    Legendre filters, also known as "Optimum-L" filters, provide the steepest
    monotonic rolloff for a given filter order. They offer an optimal compromise
    between Butterworth and Chebyshev characteristics:
    
    Key characteristics:
    - Steepest possible monotonic rolloff (no ripple in passband or stopband)
    - Faster rolloff than Butterworth filters
    - No overshoot or ringing in time domain
    - Optimal compromise between magnitude and phase response
    - Maximally flat response up to the transition region
    
    Mathematical Foundation:
    Legendre filters are based on Legendre polynomials of the first kind, designed
    using the Papoulis method for optimal monotonic response. The poles are calculated
    to provide maximum rolloff steepness while maintaining monotonic behavior.
    
    Features:
    - Orders 1-20 supported
    - Lowpass, highpass, bandpass, bandstop configurations
    - Automatic biquad cascade generation
    - Stable coefficient calculation using pre-computed poles
    - Optimized for both magnitude and phase characteristics
    
    Applications:
    - Audio applications requiring steep rolloff without overshoot
    - Anti-aliasing filters with optimal transition characteristics
    - Control systems requiring monotonic response
    - Communications filters with linear phase requirements
    - Any application where Butterworth is too slow and Chebyshev has too much ripple
    
    The filter uses a dual-precision architecture where:
    - SampleType: for audio buffer processing (float/double)
    - CoeffType: for internal calculations (defaults to double for precision)
    
    @see ButterworthFilter, ChebyshevFilter, BesselFilter
*/
template <typename SampleType, typename CoeffType = double>
class LegendreFilter : public FilterBase<SampleType, CoeffType>
{
public:
    //==============================================================================
    /** Default constructor */
    LegendreFilter() 
        : cascade (1)
    {
        setParameters (FilterType::lowpass, 2, static_cast<CoeffType> (1000.0), 44100.0);
    }

    /** Constructor with parameters */
    LegendreFilter (FilterType filterType, int order, CoeffType frequency, double sampleRate)
        : cascade (calculateNumSections (order))
    {
        setParameters (filterType, order, frequency, sampleRate);
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
        
        @param filterType  The filter type (lowpass, highpass, bandpass, bandstop)
        @param order       The filter order (1-20)
        @param frequency   The cutoff frequency in Hz (or center frequency for bandpass/bandstop)
        @param sampleRate  The sample rate in Hz
        @param bandwidth   The bandwidth for bandpass/bandstop filters (default 1 octave)
    */
    void setParameters (FilterType filterType, int order, CoeffType frequency, double sampleRate, CoeffType bandwidth = static_cast<CoeffType> (1.0)) noexcept
    {
        this->filterType = filterType;
        filterOrder = jlimit (1, 20, order);
        cutoffFreq = frequency;
        this->sampleRate = sampleRate;
        bandwidthOctaves = bandwidth;

        const auto numSections = calculateNumSections (filterOrder);
        if (cascade.getNumSections() != static_cast<size_t> (numSections))
            cascade.setNumSections (numSections);

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

    //==============================================================================
    /** 
        Gets the theoretical rolloff steepness compared to Butterworth.
        
        Legendre filters provide steeper rolloff than Butterworth filters
        while maintaining monotonic response.
        
        @returns  The approximate steepness factor (> 1.0)
    */
    CoeffType getSteepnessFactor() const noexcept
    {
        // Legendre filters provide approximately 15-20% steeper rolloff than Butterworth
        return static_cast<CoeffType> (1.0) + static_cast<CoeffType> (0.2) * static_cast<CoeffType> (filterOrder) / static_cast<CoeffType> (10.0);
    }

    /** 
        Gets the estimated 3dB bandwidth for the filter.
        
        @returns  The 3dB bandwidth in Hz
    */
    CoeffType getBandwidth3dB() const noexcept
    {
        if (filterType == FilterType::bandpass || filterType == FilterType::bandstop)
        {
            return cutoffFreq * bandwidthOctaves * static_cast<CoeffType> (0.693);  // Convert octaves to linear
        }
        return cutoffFreq;
    }

private:
    //==============================================================================
    static int calculateNumSections (int order) noexcept
    {
        return (order + 1) / 2;
    }

    void updateCoefficients() noexcept
    {
        std::vector<BiquadCoefficients<CoeffType>> coeffs;
        
        switch (filterType)
        {
            case FilterType::lowpass:
                coeffs = FilterDesigner<CoeffType>::designLegendreLowpass (filterOrder, cutoffFreq, this->sampleRate);
                break;

            case FilterType::highpass:
                coeffs = FilterDesigner<CoeffType>::designLegendreHighpass (filterOrder, cutoffFreq, this->sampleRate);
                break;

            case FilterType::bandpass:
                coeffs = FilterDesigner<CoeffType>::designLegendreBandpass (filterOrder, cutoffFreq, bandwidthOctaves, this->sampleRate);
                break;

            case FilterType::bandstop:
                coeffs = FilterDesigner<CoeffType>::designLegendreBandstop (filterOrder, cutoffFreq, bandwidthOctaves, this->sampleRate);
                break;

            default:
                coeffs = FilterDesigner<CoeffType>::designLegendreLowpass (filterOrder, cutoffFreq, this->sampleRate);
                break;
        }
        
        // Apply coefficients to cascade
        const auto numSections = coeffs.size();
        for (size_t i = 0; i < numSections; ++i)
        {
            cascade.setSectionCoefficients (i, coeffs[i]);
        }
    }

    //==============================================================================
    BiquadCascade<SampleType, CoeffType> cascade;
    
    FilterType filterType = FilterType::lowpass;
    int filterOrder = 2;
    CoeffType cutoffFreq = static_cast<CoeffType> (1000.0);
    CoeffType bandwidthOctaves = static_cast<CoeffType> (1.0);

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LegendreFilter)
};

//==============================================================================
/** Type aliases for convenience */
using LegendreFilterFloat = LegendreFilter<float>;       // float samples, double coefficients (default)
using LegendreFilterDouble = LegendreFilter<double>;     // double samples, double coefficients (default)

} // namespace yup
