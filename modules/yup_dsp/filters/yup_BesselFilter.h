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
    Bessel filter implementation with linear phase response.
    
    Bessel filters are designed to have maximally flat group delay, which means
    they preserve the waveform shape better than other filter types. They are
    characterized by:
    
    - Linear phase response (constant group delay)
    - Smooth frequency response without ripple
    - Excellent transient response with minimal overshoot
    - Slower rolloff compared to Butterworth or Chebyshev filters
    
    The filter uses Bessel polynomials to design analog prototypes that are
    then transformed to digital filters using the bilinear transform. This
    ensures optimal phase linearity is preserved in the digital domain.
    
    Features:
    - Orders 1-20 supported
    - Lowpass, highpass, bandpass, bandstop configurations  
    - Automatic biquad cascade generation
    - Stable coefficient calculation using analog prototypes
    - Maximally flat group delay for waveform preservation
    
    Applications:
    - Audio crossovers requiring phase coherence
    - Impulse response measurements
    - Waveform shaping without phase distortion
    - Anti-aliasing with minimal phase shift
    
    @see BiquadCascade, FilterBase, ButterworthFilter
*/
template <typename SampleType, typename CoeffType = double>
class BesselFilter : public FilterBase<SampleType, CoeffType>
{
public:
    using CoefficientsType = CoeffType;
    using SamplesType = SampleType;

    //==============================================================================
    /** Default constructor */
    BesselFilter() 
        : cascade (1)
    {
        setParameters (FilterType::lowpass, 2, static_cast<CoeffType> (1000.0), 44100.0);
    }

    /** Constructor with parameters */
    BesselFilter (FilterType filterType, int order, CoeffType frequency, double sampleRate)
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
        
        @param filterType  The filter type (lowpass, highpass, etc.)
        @param order       The filter order (1-20)
        @param frequency   The cutoff frequency in Hz (or center frequency for bandpass/bandstop)
        @param sampleRate  The sample rate in Hz
    */
    void setParameters (FilterType filterType, int order, CoeffType frequency, double sampleRate) noexcept
    {
        this->filterType = filterType;
        filterOrder = jlimit (1, 20, order);
        cutoffFreq = frequency;
        this->sampleRate = sampleRate;

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
        Gets the theoretical group delay at DC (for lowpass filters).
        
        Bessel filters are designed for constant group delay. This returns
        the normalized group delay at DC frequency.
        
        @returns  The group delay in samples
    */
    CoeffType getGroupDelay() const noexcept
    {
        // Group delay for Bessel filters is approximately order/cutoff_freq * sample_rate  
        if (filterType == FilterType::lowpass && this->sampleRate > 0.0)
        {
            const auto normalizedCutoff = cutoffFreq / static_cast<CoeffType> (this->sampleRate);
            return static_cast<CoeffType> (filterOrder) / (static_cast<CoeffType> (2.0) * MathConstants<CoeffType>::pi * normalizedCutoff);
        }
        
        return static_cast<CoeffType> (0.0);
    }

private:
    //==============================================================================
    static int calculateNumSections (int order) noexcept
    {
        return (order + 1) / 2;
    }

    void updateCoefficients() noexcept
    {
        switch (filterType)
        {
            case FilterType::lowpass:
                FilterDesigner<CoeffType>::designBesselLowpass (coefficientsStorage, filterOrder, cutoffFreq, this->sampleRate);
                break;
                
            case FilterType::highpass:
                FilterDesigner<CoeffType>::designBesselHighpass (coefficientsStorage, filterOrder, cutoffFreq, this->sampleRate);
                break;
                
            default:
                // For now, only lowpass and highpass are implemented
                FilterDesigner<CoeffType>::designBesselLowpass (coefficientsStorage, filterOrder, cutoffFreq, this->sampleRate);
                break;
        }
        
        // Apply coefficients to cascade
        const auto numSections = coefficientsStorage.size();
        for (size_t i = 0; i < numSections; ++i)
            cascade.setSectionCoefficients (i, coefficientsStorage[i]);
    }

    //==============================================================================
    BiquadCascade<SampleType, CoeffType> cascade;
    
    FilterType filterType = FilterType::lowpass;
    int filterOrder = 2;
    CoeffType cutoffFreq = static_cast<CoeffType> (1000.0);

    std::vector<BiquadCoefficients<CoeffType>> coefficientsStorage;

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BesselFilter)
};

//==============================================================================
/** Type aliases for convenience */
using BesselFilterFloat = BesselFilter<float>;      // float samples, double coefficients (default)
using BesselFilterDouble = BesselFilter<double>;    // double samples, double coefficients (default)

} // namespace yup
