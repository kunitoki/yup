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
class RbjFilter : public Biquad<SampleType, CoeffType>
{
    using BaseFilterType = Biquad<SampleType, CoeffType>;

public:
    //==============================================================================
    /** Filter type enumeration specific to RBJ cookbook */
    enum class Mode
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
    /** Default constructor */
    RbjFilter() noexcept
    {
        setParameters (Mode::lowpass, static_cast<CoeffType> (1000.0), static_cast<CoeffType> (0.707), static_cast<CoeffType> (0.0), 44100.0);
    }

    /** Constructor with optional initial parameters */
    explicit RbjFilter (Mode mode) noexcept
    {
        setParameters (mode, static_cast<CoeffType> (1000.0), static_cast<CoeffType> (0.707), static_cast<CoeffType> (0.0), 44100.0);
    }

    //==============================================================================
    /** 
        Sets all filter parameters.
        
        @param mode        The RBJ filter mode
        @param frequency   The center/cutoff frequency in Hz
        @param q          The Q factor (resonance/bandwidth control)
        @param gainDb     The gain in decibels (for peaking and shelving filters)
        @param sampleRate The sample rate in Hz
    */
    void setParameters (Mode mode, CoeffType frequency, CoeffType q, CoeffType gainDb, double sampleRate) noexcept
    {
        filterMode = mode;
        centerFreq = frequency;
        qFactor = q;
        gain = gainDb;

        BaseFilterType::sampleRate = sampleRate;

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
        Sets the filter mode.

        @param mode  The new RBJ filter mode
    */
    void setMode (Mode mode) noexcept
    {
        filterMode = mode;
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
        Gets the current filter mode.

        @returns  The RBJ filter mode
    */
    Mode getMode() const noexcept
    {
        return filterMode;
    }

    //==============================================================================
    /** @internal */
    void reset() noexcept override
    {
        BaseFilterType::reset();
    }

    /** @internal */
    void prepare (double sampleRate, int maximumBlockSize) noexcept override
    {
        BaseFilterType::prepare (sampleRate, maximumBlockSize);

        updateCoefficients();
    }

    /** @internal */
    SampleType processSample (SampleType inputSample) noexcept override
    {
        return BaseFilterType::processSample (inputSample);
    }

    /** @internal */
    void processBlock (const SampleType* inputBuffer, SampleType* outputBuffer, int numSamples) noexcept override
    {
        BaseFilterType::processBlock (inputBuffer, outputBuffer, numSamples);
    }

    /** @internal */
    DspMath::Complex<CoeffType> getComplexResponse (CoeffType frequency) const noexcept override
    {
        return BaseFilterType::getComplexResponse (frequency);
    }

private:
    //==============================================================================
    void updateCoefficients() noexcept
    {
        BiquadCoefficients<CoeffType> coeffs;

        switch (filterMode)
        {
            case Mode::lowpass:
                coeffs = FilterDesigner<CoeffType>::designRbjLowpass (centerFreq, qFactor, this->sampleRate);
                break;

            case Mode::highpass:
                coeffs = FilterDesigner<CoeffType>::designRbjHighpass (centerFreq, qFactor, this->sampleRate);
                break;

            case Mode::bandpassCsg:
            case Mode::bandpassCpg:
                coeffs = FilterDesigner<CoeffType>::designRbjBandpass (centerFreq, qFactor, this->sampleRate);
                break;

            case Mode::notch:
                coeffs = FilterDesigner<CoeffType>::designRbjBandstop (centerFreq, qFactor, this->sampleRate);
                break;

            case Mode::peaking:
                coeffs = FilterDesigner<CoeffType>::designRbjPeak (centerFreq, qFactor, gain, this->sampleRate);
                break;

            case Mode::lowshelf:
                coeffs = FilterDesigner<CoeffType>::designRbjLowShelf (centerFreq, qFactor, gain, this->sampleRate);
                break;

            case Mode::highshelf:
                coeffs = FilterDesigner<CoeffType>::designRbjHighShelf (centerFreq, qFactor, gain, this->sampleRate);
                break;

            case Mode::allpass:
                coeffs = FilterDesigner<CoeffType>::designRbjAllpass (centerFreq, qFactor, this->sampleRate);
                break;
        }

        BaseFilterType::setCoefficients (coeffs);
    }

    //==============================================================================
    CoeffType centerFreq = static_cast<CoeffType> (1000.0);
    CoeffType qFactor = static_cast<CoeffType> (0.707);
    CoeffType gain = static_cast<CoeffType> (0.0);
    Mode filterMode = Mode::lowpass;

    //==============================================================================
    YUP_LEAK_DETECTOR (RbjFilter)
};

//==============================================================================
/** Type aliases for convenience */
using RbjFilterFloat = RbjFilter<float>;   // float samples, double coefficients (default)
using RbjFilterDouble = RbjFilter<double>; // double samples, double coefficients (default)

} // namespace yup
