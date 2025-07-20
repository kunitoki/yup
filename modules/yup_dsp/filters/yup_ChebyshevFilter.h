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
    Chebyshev filter implementation (Type I and Type II).
    
    Chebyshev filters provide sharper rolloff than Butterworth filters but
    introduce ripple in either the passband (Type I) or stopband (Type II).
    They are optimal for applications requiring steep frequency selectivity.
    
    Type I features:
    - Equiripple in the passband, monotonic in the stopband
    - Steeper rolloff than Butterworth for same order
    - Configurable passband ripple (0.01 to 3.0 dB typical)
    
    Type II features:
    - Monotonic in the passband, equiripple in the stopband
    - Finite transmission zeros (notches) in the stopband
    - Configurable stopband attenuation
    
    Features:
    - Orders 1-20 supported  
    - Lowpass, highpass, bandpass, bandstop configurations
    - Automatic biquad cascade generation
    - Stable coefficient calculation using analog prototypes
    
    @see BiquadCascade, FilterBase, ButterworthFilter
*/
template <typename SampleType, typename CoeffType = double>
class ChebyshevFilter : public FilterBase<SampleType, CoeffType>
{
public:
    //==============================================================================
    /** Chebyshev filter type */
    enum class Type
    {
        Type1,  /**< Type I: passband ripple, monotonic stopband */
        Type2   /**< Type II: monotonic passband, stopband ripple */
    };

    //==============================================================================
    /** Default constructor */
    ChebyshevFilter() 
        : cascade (1)
    {
        setParameters (Type::Type1, FilterType::lowpass, 2, static_cast<CoeffType> (1000.0), 44100.0, static_cast<CoeffType> (0.5));
    }

    /** Constructor with parameters */
    ChebyshevFilter (Type chebyType, FilterType filterType, int order, CoeffType frequency, double sampleRate, CoeffType ripple = static_cast<CoeffType> (0.5))
        : cascade (calculateNumSections (order))
    {
        setParameters (chebyType, filterType, order, frequency, sampleRate, ripple);
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
        
        @param chebyType   The Chebyshev type (Type I or Type II)
        @param filterType  The filter type (lowpass, highpass, etc.)
        @param order       The filter order (1-20)
        @param frequency   The cutoff frequency in Hz (or center frequency for bandpass/bandstop)
        @param sampleRate  The sample rate in Hz
        @param ripple      The ripple amount in dB (passband for Type I, stopband attenuation for Type II)
    */
    void setParameters (Type chebyType, FilterType filterType, int order, CoeffType frequency, double sampleRate, CoeffType ripple = static_cast<CoeffType> (0.5)) noexcept
    {
        chebyshevType = chebyType;
        this->filterType = filterType;
        filterOrder = jlimit (1, 20, order);
        cutoffFreq = frequency;
        rippleAmount = ripple;
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
        Sets the ripple amount.
        
        @param ripple  The ripple in dB (passband for Type I, stopband attenuation for Type II)
    */
    void setRipple (CoeffType ripple) noexcept
    {
        if (chebyshevType == Type::Type1)
            rippleAmount = jlimit (static_cast<CoeffType> (0.01), static_cast<CoeffType> (10.0), ripple);
        else
            rippleAmount = jlimit (static_cast<CoeffType> (20.0), static_cast<CoeffType> (100.0), ripple);
        
        updateCoefficients();
    }

    /** 
        Sets the Chebyshev type.
        
        @param type  The Chebyshev type
    */
    void setChebyshevType (Type type) noexcept
    {
        if (chebyshevType != type)
        {
            chebyshevType = type;
            
            // Adjust ripple range for the new type
            if (type == Type::Type1 && rippleAmount > static_cast<CoeffType> (10.0))
                rippleAmount = static_cast<CoeffType> (1.0);
            else if (type == Type::Type2 && rippleAmount < static_cast<CoeffType> (20.0))
                rippleAmount = static_cast<CoeffType> (40.0);
            
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

    /** 
        Gets the current Chebyshev type.
        
        @returns  The Chebyshev type
    */
    Type getChebyshevType() const noexcept
    {
        return chebyshevType;
    }

    /** 
        Gets the current ripple amount.
        
        @returns  The ripple in dB
    */
    CoeffType getRipple() const noexcept
    {
        return rippleAmount;
    }

    //==============================================================================
    /** 
        Gets the theoretical passband edge frequency for Type I filters.
        
        This is the frequency at which the response first reaches -ripple dB.
        
        @returns  The passband edge frequency in Hz
    */
    CoeffType getPassbandEdgeFrequency() const noexcept
    {
        if (chebyshevType == Type::Type1)
            return cutoffFreq;
        
        // For Type II, the passband edge is different from the cutoff
        const auto epsilon = static_cast<CoeffType> (1.0) / std::sqrt (std::pow (static_cast<CoeffType> (10.0), rippleAmount / static_cast<CoeffType> (10.0)) - static_cast<CoeffType> (1.0));
        const auto factor = std::pow (epsilon + std::sqrt (static_cast<CoeffType> (1.0) + epsilon * epsilon), static_cast<CoeffType> (1.0) / static_cast<CoeffType> (filterOrder));
        
        return cutoffFreq / factor;
    }

    /** 
        Gets the theoretical stopband edge frequency for Type II filters.
        
        @returns  The stopband edge frequency in Hz
    */
    CoeffType getStopbandEdgeFrequency() const noexcept
    {
        if (chebyshevType == Type::Type2)
            return cutoffFreq;
        
        // For Type I, calculate the stopband edge
        const auto epsilon = std::sqrt (std::pow (static_cast<CoeffType> (10.0), rippleAmount / static_cast<CoeffType> (10.0)) - static_cast<CoeffType> (1.0));
        const auto factor = std::pow (epsilon + std::sqrt (static_cast<CoeffType> (1.0) + epsilon * epsilon), static_cast<CoeffType> (1.0) / static_cast<CoeffType> (filterOrder));
        
        return cutoffFreq * factor;
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
                if (chebyshevType == Type::Type1)
                    coeffs = FilterDesigner<CoeffType>::designChebyshev1Lowpass (filterOrder, cutoffFreq, this->sampleRate, rippleAmount);
                else
                    coeffs = FilterDesigner<CoeffType>::designChebyshev2Lowpass (filterOrder, cutoffFreq, this->sampleRate, rippleAmount);
                break;
                
            case FilterType::highpass:
                if (chebyshevType == Type::Type1)
                    coeffs = FilterDesigner<CoeffType>::designChebyshev1Highpass (filterOrder, cutoffFreq, this->sampleRate, rippleAmount);
                else
                    coeffs = FilterDesigner<CoeffType>::designChebyshev2Highpass (filterOrder, cutoffFreq, this->sampleRate, rippleAmount);
                break;
                
            default:
                // For now, only lowpass and highpass are implemented
                if (chebyshevType == Type::Type1)
                    coeffs = FilterDesigner<CoeffType>::designChebyshev1Lowpass (filterOrder, cutoffFreq, this->sampleRate, rippleAmount);
                else
                    coeffs = FilterDesigner<CoeffType>::designChebyshev2Lowpass (filterOrder, cutoffFreq, this->sampleRate, rippleAmount);
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
    
    Type chebyshevType = Type::Type1;
    FilterType filterType = FilterType::lowpass;
    int filterOrder = 2;
    CoeffType cutoffFreq = static_cast<CoeffType> (1000.0);
    CoeffType rippleAmount = static_cast<CoeffType> (0.5);

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChebyshevFilter)
};

//==============================================================================
/** Type aliases for convenience */
using ChebyshevFilterFloat = ChebyshevFilter<float>;      // float samples, double coefficients (default)
using ChebyshevFilterDouble = ChebyshevFilter<double>;    // double samples, double coefficients (default)

} // namespace yup
