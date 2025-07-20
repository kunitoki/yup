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
    Notch filter implementation with multiple algorithm options.
    
    A notch filter creates a deep attenuation (notch) at a specific frequency
    while leaving other frequencies relatively unaffected. This implementation
    provides several algorithm options optimized for different use cases:
    
    Algorithm Types:
    - **Allpass-based**: Uses a 2nd-order allpass section for excellent phase characteristics
    - **Biquad-based**: Traditional IIR biquad implementation for efficient processing
    - **Cut/Boost**: Can function as either notch (cut) or peak (boost) filter
    
    Key Features:
    - Independent frequency and depth control
    - Multiple algorithm options for different phase/magnitude trade-offs
    - Real-time parameter changes without artifacts
    - Optimized for audio and signal processing applications
    
    Applications:
    - Removing specific frequency interference (50/60Hz hum, whistles)
    - Audio feedback suppression
    - Spectral shaping and equalization
    - Creating resonant effects
    - Parametric EQ building blocks
    
    The filter uses a dual-precision architecture where:
    - SampleType: for audio buffer processing (float/double)
    - CoeffType: for internal calculations (defaults to double for precision)
    
    @see RbjFilter, StateVariableFilter, ButterworthFilter
*/
template <typename SampleType, typename CoeffType = double>
class NotchFilter : public FilterBase<SampleType, CoeffType>
{
public:
    //==============================================================================
    /** Algorithm types for notch filter implementation */
    enum class Algorithm
    {
        allpass,    /** Allpass-based notch with excellent phase characteristics */
        biquad,     /** Traditional biquad implementation for efficiency */
        cutboost    /** Cut/boost filter that can notch or peak */
    };

    //==============================================================================
    /** Default constructor */
    NotchFilter()
        : algorithm (Algorithm::allpass)
        , notchFreq (static_cast<CoeffType> (1000.0))
        , depth (static_cast<CoeffType> (0.9))
        , boost (static_cast<CoeffType> (0.0))
    {
        setParameters (notchFreq, depth, 44100.0);
    }

    /** Constructor with parameters */
    NotchFilter (CoeffType frequency, CoeffType notchDepth, double sampleRate, Algorithm alg = Algorithm::allpass)
        : algorithm (alg)
        , notchFreq (frequency)
        , depth (notchDepth)
        , boost (static_cast<CoeffType> (0.0))
    {
        setParameters (frequency, notchDepth, sampleRate);
    }

    //==============================================================================
    /** @internal */
    void reset() noexcept override
    {
        switch (algorithm)
        {
            case Algorithm::allpass:
                resetAllpass();
                break;
            case Algorithm::biquad:
                resetBiquad();
                break;
            case Algorithm::cutboost:
                resetCutBoost();
                break;
        }
    }

    /** @internal */
    void prepare (double sampleRate, int maximumBlockSize) noexcept override
    {
        this->sampleRate = sampleRate;
        this->maximumBlockSize = maximumBlockSize;
        updateCoefficients();
    }

    /** @internal */
    SampleType processSample (SampleType inputSample) noexcept override
    {
        switch (algorithm)
        {
            case Algorithm::allpass:
                return processAllpass (inputSample);
            case Algorithm::biquad:
                return processBiquad (inputSample);
            case Algorithm::cutboost:
                return processCutBoost (inputSample);
        }
        return inputSample;
    }

    /** @internal */
    void processBlock (const SampleType* inputBuffer, SampleType* outputBuffer, int numSamples) noexcept override
    {
        for (int i = 0; i < numSamples; ++i)
            outputBuffer[i] = processSample (inputBuffer[i]);
    }

    /** @internal */
    DspMath::Complex<CoeffType> getComplexResponse (CoeffType frequency) const noexcept override
    {
        const auto omega = DspMath::frequencyToAngular (frequency, this->sampleRate);
        const auto z = DspMath::Complex<CoeffType> (std::cos (omega), std::sin (omega));
        
        switch (algorithm)
        {
            case Algorithm::allpass:
                return getComplexResponseAllpass (z);
            case Algorithm::biquad:
                return getComplexResponseBiquad (z);
            case Algorithm::cutboost:
                return getComplexResponseCutBoost (z);
        }
        return DspMath::Complex<CoeffType> (static_cast<CoeffType> (1.0), static_cast<CoeffType> (0.0));
    }

    //==============================================================================
    /** 
        Sets all filter parameters.
        
        @param frequency   The notch frequency in Hz
        @param notchDepth  The depth of the notch (0.0 to 1.0, where 1.0 is deepest)
        @param sampleRate  The sample rate in Hz
        @param alg         The algorithm to use (optional, defaults to current)
    */
    void setParameters (CoeffType frequency, CoeffType notchDepth, double sampleRate, Algorithm alg = Algorithm::allpass) noexcept
    {
        if (alg != algorithm)
        {
            algorithm = alg;
            reset();
        }
        
        notchFreq = frequency;
        depth = jlimit (static_cast<CoeffType> (0.0), static_cast<CoeffType> (1.0), notchDepth);
        this->sampleRate = sampleRate;
        
        updateCoefficients();
    }

    /** 
        Sets the notch frequency.
        
        @param frequency  The new notch frequency in Hz
    */
    void setFrequency (CoeffType frequency) noexcept
    {
        notchFreq = frequency;
        updateCoefficients();
    }

    /** 
        Sets the notch depth.
        
        @param notchDepth  The depth of the notch (0.0 to 1.0)
    */
    void setDepth (CoeffType notchDepth) noexcept
    {
        depth = jlimit (static_cast<CoeffType> (0.0), static_cast<CoeffType> (1.0), notchDepth);
        updateCoefficients();
    }

    /** 
        Sets the boost amount (for cut/boost algorithm only).
        
        @param boostAmount  The boost amount (-1.0 to 1.0, negative values cut, positive boost)
    */
    void setBoost (CoeffType boostAmount) noexcept
    {
        boost = jlimit (static_cast<CoeffType> (-1.0), static_cast<CoeffType> (1.0), boostAmount);
        if (algorithm == Algorithm::cutboost)
            updateCoefficients();
    }

    /** 
        Changes the algorithm used.
        
        @param alg  The new algorithm to use
    */
    void setAlgorithm (Algorithm alg) noexcept
    {
        if (algorithm != alg)
        {
            algorithm = alg;
            reset();
            updateCoefficients();
        }
    }

    //==============================================================================
    /** Gets the current notch frequency */
    CoeffType getFrequency() const noexcept { return notchFreq; }

    /** Gets the current notch depth */
    CoeffType getDepth() const noexcept { return depth; }

    /** Gets the current boost amount */
    CoeffType getBoost() const noexcept { return boost; }

    /** Gets the current algorithm */
    Algorithm getAlgorithm() const noexcept { return algorithm; }

    /** Gets the estimated -3dB bandwidth of the notch */
    CoeffType getBandwidth3dB() const noexcept
    {
        // Approximation based on depth - deeper notches are narrower
        return notchFreq * (static_cast<CoeffType> (0.1) + static_cast<CoeffType> (0.4) * (static_cast<CoeffType> (1.0) - depth));
    }

private:
    //==============================================================================
    Algorithm algorithm;
    CoeffType notchFreq;
    CoeffType depth;
    CoeffType boost;
    
    // Allpass-based implementation
    struct AllpassData
    {
        CoeffType a = static_cast<CoeffType> (0.9);
        CoeffType b = static_cast<CoeffType> (0.0);
        SampleType z1 = static_cast<SampleType> (0.0);
        SampleType z2 = static_cast<SampleType> (0.0);
        SampleType y1 = static_cast<SampleType> (0.0);
        SampleType y2 = static_cast<SampleType> (0.0);
    } allpassData;
    
    // Biquad-based implementation
    struct BiquadData
    {
        CoeffType b0 = static_cast<CoeffType> (1.0);
        CoeffType b1 = static_cast<CoeffType> (0.0);
        CoeffType b2 = static_cast<CoeffType> (1.0);
        CoeffType a1 = static_cast<CoeffType> (0.0);
        CoeffType a2 = static_cast<CoeffType> (0.0);
        CoeffType gain = static_cast<CoeffType> (1.0);
        SampleType x1 = static_cast<SampleType> (0.0);
        SampleType x2 = static_cast<SampleType> (0.0);
        SampleType y1 = static_cast<SampleType> (0.0);
        SampleType y2 = static_cast<SampleType> (0.0);
    } biquadData;
    
    // Cut/boost implementation
    struct CutBoostData
    {
        CoeffType k = static_cast<CoeffType> (1.0);
        CoeffType g = static_cast<CoeffType> (0.5);
        CoeffType a = static_cast<CoeffType> (0.9);
        AllpassData allpass;
    } cutBoostData;

    //==============================================================================
    void updateCoefficients() noexcept
    {
        if (this->sampleRate <= 0.0)
            return;

        const auto normalizedFreq = notchFreq / this->sampleRate;
        
        switch (algorithm)
        {
            case Algorithm::allpass:
                updateAllpassCoeffs (normalizedFreq);
                break;
            case Algorithm::biquad:
                updateBiquadCoeffs (normalizedFreq);
                break;
            case Algorithm::cutboost:
                updateCutBoostCoeffs (normalizedFreq);
                break;
        }
    }

    void updateAllpassCoeffs (CoeffType normalizedFreq) noexcept
    {
        // Based on spuce notch_allpass implementation
        const auto k2 = depth * static_cast<CoeffType> (0.95); // Limit to avoid instability
        const auto cosine = std::cos (MathConstants<CoeffType>::twoPi * normalizedFreq);
        
        allpassData.a = k2;
        allpassData.b = -cosine * (static_cast<CoeffType> (1.0) + k2);
    }

    void updateBiquadCoeffs (CoeffType normalizedFreq) noexcept
    {
        // Based on spuce notch_iir implementation with improvements
        const auto Y = depth * static_cast<CoeffType> (0.9); // Depth control
        const auto B = -std::cos (MathConstants<CoeffType>::twoPi * normalizedFreq); // Frequency control
        
        biquadData.b0 = static_cast<CoeffType> (1.0);
        biquadData.b1 = Y * (static_cast<CoeffType> (1.0) + B);
        biquadData.b2 = B;
        biquadData.a1 = static_cast<CoeffType> (2.0) * Y;
        biquadData.a2 = static_cast<CoeffType> (1.0);
        biquadData.gain = (static_cast<CoeffType> (1.0) + B) * static_cast<CoeffType> (0.5);
    }

    void updateCutBoostCoeffs (CoeffType normalizedFreq) noexcept
    {
        // Based on spuce cutboost implementation
        const auto k2 = depth * static_cast<CoeffType> (0.95);
        const auto cosine = std::cos (MathConstants<CoeffType>::twoPi * normalizedFreq);
        
        cutBoostData.a = k2;
        cutBoostData.allpass.a = k2;
        cutBoostData.allpass.b = -cosine * (static_cast<CoeffType> (1.0) + k2);
        
        // Cut/boost control
        const auto k0 = boost;
        cutBoostData.k = (static_cast<CoeffType> (1.0) - k0) / (static_cast<CoeffType> (1.0) + k0);
        cutBoostData.g = static_cast<CoeffType> (0.5) * (static_cast<CoeffType> (1.0) + k0);
    }

    //==============================================================================
    void resetAllpass() noexcept
    {
        allpassData.z1 = allpassData.z2 = static_cast<SampleType> (0.0);
        allpassData.y1 = allpassData.y2 = static_cast<SampleType> (0.0);
    }

    void resetBiquad() noexcept
    {
        biquadData.x1 = biquadData.x2 = static_cast<SampleType> (0.0);
        biquadData.y1 = biquadData.y2 = static_cast<SampleType> (0.0);
    }

    void resetCutBoost() noexcept
    {
        cutBoostData.allpass.z1 = cutBoostData.allpass.z2 = static_cast<SampleType> (0.0);
        cutBoostData.allpass.y1 = cutBoostData.allpass.y2 = static_cast<SampleType> (0.0);
    }

    //==============================================================================
    SampleType processAllpass (SampleType input) noexcept
    {
        // 2nd order allpass: G(z) = (z^2 + b*z + a) / (a*z^2 + b*z + 1)
        auto& ap = allpassData;
        
        const auto allpassOut = static_cast<SampleType> (ap.a) * (input - ap.y1) + 
                                static_cast<SampleType> (ap.b) * (ap.z1 - ap.y2) + ap.z2;
        
        // Shift delays
        ap.z2 = ap.z1;
        ap.z1 = input;
        ap.y2 = ap.y1;
        ap.y1 = allpassOut;
        
        // Notch output: 0.5 * (input + allpass_output)
        return static_cast<SampleType> (0.5) * (input + allpassOut);
    }

    SampleType processBiquad (SampleType input) noexcept
    {
        auto& bq = biquadData;
        
        const auto scaledInput = static_cast<SampleType> (bq.gain) * input;
        const auto output = static_cast<SampleType> (bq.b0) * scaledInput + 
                           static_cast<SampleType> (bq.b1) * bq.x1 + 
                           static_cast<SampleType> (bq.b2) * bq.x2 -
                           static_cast<SampleType> (bq.a1) * bq.y1 - 
                           static_cast<SampleType> (bq.a2) * bq.y2;
        
        // Shift delays
        bq.x2 = bq.x1;
        bq.x1 = scaledInput;
        bq.y2 = bq.y1;
        bq.y1 = output;
        
        return output;
    }

    SampleType processCutBoost (SampleType input) noexcept
    {
        auto& cb = cutBoostData;
        auto& ap = cb.allpass;
        
        // Process through allpass
        const auto allpassOut = static_cast<SampleType> (ap.a) * (input - ap.y1) + 
                                static_cast<SampleType> (ap.b) * (ap.z1 - ap.y2) + ap.z2;
        
        // Shift allpass delays
        ap.z2 = ap.z1;
        ap.z1 = input;
        ap.y2 = ap.y1;
        ap.y1 = allpassOut;
        
        // Cut/boost output: g * (input + k * allpass_output)
        return static_cast<SampleType> (cb.g) * (input + static_cast<SampleType> (cb.k) * allpassOut);
    }

    //==============================================================================
    DspMath::Complex<CoeffType> getComplexResponseAllpass (const DspMath::Complex<CoeffType>& z) const noexcept
    {
        const auto& ap = allpassData;
        
        // Allpass: H(z) = (z^2 + b*z + a) / (a*z^2 + b*z + 1)
        const auto z2 = z * z;
        const auto num = z2 + static_cast<CoeffType> (ap.b) * z + static_cast<CoeffType> (ap.a);
        const auto den = static_cast<CoeffType> (ap.a) * z2 + static_cast<CoeffType> (ap.b) * z + static_cast<CoeffType> (1.0);
        
        const auto allpassResponse = num / den;
        
        // Notch: H(z) = 0.5 * (1 + H_allpass(z))
        return static_cast<CoeffType> (0.5) * (DspMath::Complex<CoeffType> (static_cast<CoeffType> (1.0), static_cast<CoeffType> (0.0)) + allpassResponse);
    }

    DspMath::Complex<CoeffType> getComplexResponseBiquad (const DspMath::Complex<CoeffType>& z) const noexcept
    {
        const auto& bq = biquadData;
        
        const auto z_inv = static_cast<CoeffType> (1.0) / z;
        const auto z_inv2 = z_inv * z_inv;
        
        const auto num = static_cast<CoeffType> (bq.b0) + static_cast<CoeffType> (bq.b1) * z_inv + static_cast<CoeffType> (bq.b2) * z_inv2;
        const auto den = static_cast<CoeffType> (1.0) + static_cast<CoeffType> (bq.a1) * z_inv + static_cast<CoeffType> (bq.a2) * z_inv2;
        
        return static_cast<CoeffType> (bq.gain) * (num / den);
    }

    DspMath::Complex<CoeffType> getComplexResponseCutBoost (const DspMath::Complex<CoeffType>& z) const noexcept
    {
        const auto allpassResponse = getComplexResponseAllpass (z);
        const auto& cb = cutBoostData;
        
        // Cut/boost: H(z) = g * (1 + k * H_allpass(z))
        return static_cast<CoeffType> (cb.g) * (DspMath::Complex<CoeffType> (static_cast<CoeffType> (1.0), static_cast<CoeffType> (0.0)) + 
                                                static_cast<CoeffType> (cb.k) * allpassResponse);
    }

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NotchFilter)
};

//==============================================================================
/** Type aliases for convenience */
using NotchFilterFloat = NotchFilter<float>;       // float samples, double coefficients (default)
using NotchFilterDouble = NotchFilter<double>;     // double samples, double coefficients (default)

} // namespace yup