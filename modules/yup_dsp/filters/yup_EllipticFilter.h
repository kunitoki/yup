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
    Elliptic (Cauer) filter implementation with steepest rolloff characteristics.
    
    Elliptic filters provide the steepest rolloff for any given filter order
    by allowing ripple in both the passband and stopband. They are optimal
    for applications requiring maximum frequency selectivity.
    
    Key characteristics:
    - Steepest possible rolloff for a given filter order
    - Equiripple behavior in both passband and stopband
    - Finite transmission zeros in the stopband
    - Configurable passband ripple and stopband attenuation
    - Complex design requiring elliptic integral calculations
    
    Elliptic filters are characterized by two parameters:
    - Passband ripple (typically 0.01 to 3.0 dB)
    - Stopband attenuation (typically 20 to 100 dB)
    
    Features:
    - Orders 1-20 supported
    - Lowpass, highpass, bandpass, bandstop configurations
    - Automatic biquad cascade generation
    - Stable coefficient calculation using elliptic integrals
    - Optimized frequency selectivity
    
    Applications:
    - Anti-aliasing with minimum transition band
    - Audio equalizers requiring sharp cutoffs
    - Communications filters with strict specifications
    - Any application prioritizing rolloff steepness over phase response
    
    Note: Elliptic filters have non-linear phase response and should not be
    used where phase linearity is important.
    
    @see BiquadCascade, FilterBase, ChebyshevFilter, BesselFilter
*/
template <typename SampleType, typename CoeffType = double>
class EllipticFilter : public FilterBase<SampleType, CoeffType>
{
public:
    //==============================================================================
    /** Default constructor */
    EllipticFilter() 
        : cascade (1)
    {
        setParameters (FilterType::lowpass, 2, static_cast<CoeffType> (1000.0), 44100.0, 
                      static_cast<CoeffType> (0.5), static_cast<CoeffType> (40.0));
    }

    /** Constructor with parameters */
    EllipticFilter (FilterType filterType, int order, CoeffType frequency, double sampleRate,
                   CoeffType passbandRipple = static_cast<CoeffType> (0.5), 
                   CoeffType stopbandAttenuation = static_cast<CoeffType> (40.0))
        : cascade (calculateNumSections (order))
    {
        setParameters (filterType, order, frequency, sampleRate, passbandRipple, stopbandAttenuation);
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
        
        @param filterType           The filter type (lowpass, highpass, etc.)
        @param order               The filter order (1-20)
        @param frequency           The cutoff frequency in Hz
        @param sampleRate          The sample rate in Hz
        @param passbandRipple      The passband ripple in dB (0.01 to 3.0)
        @param stopbandAttenuation The stopband attenuation in dB (20 to 100)
    */
    void setParameters (FilterType filterType, int order, CoeffType frequency, double sampleRate,
                       CoeffType passbandRipple, CoeffType stopbandAttenuation) noexcept
    {
        this->filterType = filterType;
        filterOrder = jlimit (1, 20, order);
        cutoffFreq = frequency;
        this->sampleRate = sampleRate;
        rippleAmount = jlimit (static_cast<CoeffType> (0.01), static_cast<CoeffType> (10.0), passbandRipple);
        stopbandAtten = jlimit (static_cast<CoeffType> (20.0), static_cast<CoeffType> (120.0), stopbandAttenuation);

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
        Sets the passband ripple amount.
        
        @param ripple  The passband ripple in dB (0.01 to 10.0)
    */
    void setPassbandRipple (CoeffType ripple) noexcept
    {
        rippleAmount = jlimit (static_cast<CoeffType> (0.01), static_cast<CoeffType> (10.0), ripple);
        updateCoefficients();
    }

    /** 
        Sets the stopband attenuation.
        
        @param attenuation  The stopband attenuation in dB (20.0 to 120.0)
    */
    void setStopbandAttenuation (CoeffType attenuation) noexcept
    {
        stopbandAtten = jlimit (static_cast<CoeffType> (20.0), static_cast<CoeffType> (120.0), attenuation);
        updateCoefficients();
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
        Gets the current passband ripple.
        
        @returns  The passband ripple in dB
    */
    CoeffType getPassbandRipple() const noexcept
    {
        return rippleAmount;
    }

    /** 
        Gets the current stopband attenuation.
        
        @returns  The stopband attenuation in dB
    */
    CoeffType getStopbandAttenuation() const noexcept
    {
        return stopbandAtten;
    }

    //==============================================================================
    /** 
        Gets the theoretical selectivity factor.
        
        This indicates how steep the transition from passband to stopband is.
        Higher values indicate sharper transitions.
        
        @returns  The selectivity factor
    */
    CoeffType getSelectivityFactor() const noexcept
    {
        // Selectivity factor k = 1/q where q is the transition ratio
        const auto epsilon = std::sqrt (std::pow (static_cast<CoeffType> (10.0), rippleAmount / static_cast<CoeffType> (10.0)) - static_cast<CoeffType> (1.0));
        const auto a = std::pow (static_cast<CoeffType> (10.0), stopbandAtten / static_cast<CoeffType> (20.0));
        
        return epsilon / std::sqrt (a * a - static_cast<CoeffType> (1.0));
    }

    /** 
        Gets the estimated transition bandwidth as a fraction of the cutoff frequency.
        
        @returns  The normalized transition bandwidth
    */
    CoeffType getTransitionBandwidth() const noexcept
    {
        // Empirical approximation for elliptic filter transition bandwidth
        const auto k = getSelectivityFactor();
        const auto orderFactor = static_cast<CoeffType> (1.0) / static_cast<CoeffType> (filterOrder);
        
        return k * orderFactor * static_cast<CoeffType> (0.5);
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
                coeffs = FilterDesigner<CoeffType>::designEllipticLowpass (filterOrder, cutoffFreq, this->sampleRate, rippleAmount, stopbandAtten);
                break;
                
            case FilterType::highpass:
                coeffs = FilterDesigner<CoeffType>::designEllipticHighpass (filterOrder, cutoffFreq, this->sampleRate, rippleAmount, stopbandAtten);
                break;
                
            case FilterType::allpass:
                coeffs = FilterDesigner<CoeffType>::designEllipticAllpass (filterOrder, this->sampleRate, rippleAmount, stopbandAtten);
                break;
                
            default:
                // For now, only lowpass, highpass, and allpass are implemented
                coeffs = FilterDesigner<CoeffType>::designEllipticLowpass (filterOrder, cutoffFreq, this->sampleRate, rippleAmount, stopbandAtten);
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
    CoeffType rippleAmount = static_cast<CoeffType> (0.5);
    CoeffType stopbandAtten = static_cast<CoeffType> (40.0);

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EllipticFilter)
};

//==============================================================================
/** Type aliases for convenience */
using EllipticFilterFloat = EllipticFilter<float>;      // float samples, double coefficients (default)
using EllipticFilterDouble = EllipticFilter<double>;    // double samples, double coefficients (default)

} // namespace yup
