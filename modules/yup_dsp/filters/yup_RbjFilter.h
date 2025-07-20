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
    Robert Bristow-Johnson (RBJ) cookbook filters.
    
    This class implements the classic "Audio EQ Cookbook" biquad filters,
    widely used in audio applications for equalization and filtering.
    
    Features:
    - Peaking/bell filters with adjustable gain and Q
    - Low-shelf and high-shelf filters
    - Lowpass, highpass, bandpass, and notch filters
    - All filters based on analog prototypes with bilinear transform
    - Frequency, Q, and gain controls
    
    Reference: "Cookbook formulae for audio EQ biquad filter coefficients"
    by Robert Bristow-Johnson
    
    @see Biquad, FilterBase
*/
template <typename SampleType, typename CoeffType = double>
class RbjFilter : public FilterBase<SampleType, CoeffType>
{
public:
    //==============================================================================
    /** Filter type enumeration specific to RBJ cookbook */
    enum class Type
    {
        lowpass,     /**< Low-pass filter */
        highpass,    /**< High-pass filter */
        bandpassCsg, /**< Band-pass filter (constant skirt gain) */
        bandpassCpg, /**< Band-pass filter (constant peak gain) */
        notch,       /**< Notch filter */
        allpass,     /**< All-pass filter */
        peaking,     /**< Peaking filter */
        lowshelf,    /**< Low-shelf filter */
        highshelf    /**< High-shelf filter */
    };

    //==============================================================================
    /** Constructor with optional initial parameters */
    explicit RbjFilter (Type type = Type::peaking) noexcept
        : filterType (type)
    {
        setParameters (type, static_cast<CoeffType> (1000.0), static_cast<CoeffType> (0.707), static_cast<CoeffType> (0.0), 44100.0);
    }

    //==============================================================================
    /** @internal */
    void reset() noexcept override
    {
        biquad.reset();
    }

    /** @internal */
    void prepare (double sampleRate, int maximumBlockSize) noexcept override
    {
        this->sampleRate = sampleRate;
        this->maximumBlockSize = maximumBlockSize;
        biquad.prepare (sampleRate, maximumBlockSize);
        updateCoefficients();
    }

    /** @internal */
    SampleType processSample (SampleType inputSample) noexcept override
    {
        return biquad.processSample (inputSample);
    }

    /** @internal */
    void processBlock (const SampleType* inputBuffer, SampleType* outputBuffer, int numSamples) noexcept override
    {
        biquad.processBlock (inputBuffer, outputBuffer, numSamples);
    }

    /** @internal */
    DspMath::Complex<CoeffType> getComplexResponse (CoeffType frequency) const noexcept override
    {
        return biquad.getComplexResponse (frequency);
    }

    //==============================================================================
    /** 
        Sets all filter parameters.
        
        @param type        The RBJ filter type
        @param frequency   The center/cutoff frequency in Hz
        @param q          The Q factor (resonance/bandwidth control)
        @param gainDb     The gain in decibels (for peaking and shelving filters)
        @param sampleRate The sample rate in Hz
    */
    void setParameters (Type type, CoeffType frequency, CoeffType q, CoeffType gainDb, double sampleRate) noexcept
    {
        filterType = type;
        centerFreq = frequency;
        qFactor = q;
        gain = gainDb;
        this->sampleRate = sampleRate;
        updateCoefficients();
    }

    /** 
        Sets just the center/cutoff frequency.
        
        @param frequency  The new frequency in Hz
    */
    void setFrequency (CoeffType frequency) noexcept
    {
        centerFreq = frequency;
        updateCoefficients();
    }

    /** 
        Sets just the Q factor.
        
        @param q  The new Q factor
    */
    void setQ (CoeffType q) noexcept
    {
        qFactor = q;
        updateCoefficients();
    }

    /** 
        Sets just the gain (for peaking and shelving filters).
        
        @param gainDb  The new gain in decibels
    */
    void setGain (CoeffType gainDb) noexcept
    {
        gain = gainDb;
        updateCoefficients();
    }

    /** 
        Sets the filter type.
        
        @param type  The new RBJ filter type
    */
    void setType (Type type) noexcept
    {
        filterType = type;
        updateCoefficients();
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
        Gets the current filter type.
        
        @returns  The RBJ filter type
    */
    Type getType() const noexcept
    {
        return filterType;
    }

private:
    //==============================================================================
    void updateCoefficients() noexcept
    {
        BiquadCoefficients<CoeffType> coeffs;

        switch (filterType)
        {
            case Type::lowpass:
                coeffs = FilterDesigner::designRbjLowpass<CoeffType> (centerFreq, qFactor, this->sampleRate);
                break;
            case Type::highpass:
                coeffs = FilterDesigner::designRbjHighpass<CoeffType> (centerFreq, qFactor, this->sampleRate);
                break;
            case Type::bandpassCsg:
            case Type::bandpassCpg:
                coeffs = FilterDesigner::designRbjBandpass<CoeffType> (centerFreq, qFactor, this->sampleRate);
                break;
            case Type::notch:
                coeffs = FilterDesigner::designRbjBandstop<CoeffType> (centerFreq, qFactor, this->sampleRate);
                break;
            case Type::allpass:
                coeffs = FilterDesigner::designRbjAllpass<CoeffType> (centerFreq, qFactor, this->sampleRate);
                break;
            case Type::peaking:
                coeffs = FilterDesigner::designRbjPeak<CoeffType> (centerFreq, qFactor, gain, this->sampleRate);
                break;
            case Type::lowshelf:
                coeffs = FilterDesigner::designRbjLowShelf<CoeffType> (centerFreq, qFactor, gain, this->sampleRate);
                break;
            case Type::highshelf:
                coeffs = FilterDesigner::designRbjHighShelf<CoeffType> (centerFreq, qFactor, gain, this->sampleRate);
                break;
        }

        biquad.setCoefficients (coeffs);
    }

    //==============================================================================
    Biquad<SampleType> biquad;
    
    Type filterType = Type::peaking;
    CoeffType centerFreq = static_cast<CoeffType> (1000.0);
    CoeffType qFactor = static_cast<CoeffType> (0.707);
    CoeffType gain = static_cast<CoeffType> (0.0);

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RbjFilter)
};

//==============================================================================
/** Type aliases for convenience */
using RbjFilterFloat = RbjFilter<float>;      // float samples, double coefficients (default)
using RbjFilterDouble = RbjFilter<double>;    // double samples, double coefficients (default)

} // namespace yup
