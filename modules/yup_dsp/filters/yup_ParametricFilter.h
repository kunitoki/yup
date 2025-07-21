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
    Parametric filter implementation for audio equalization and signal shaping.
    
    A parametric filter provides precise control over frequency response with
    independent adjustments for frequency, gain, and bandwidth (Q factor).
    This implementation supports multiple filter types optimized for different
    equalization scenarios:
    
    Filter Types:
    - **Bell/Peak**: Traditional parametric EQ band with symmetric boost/cut
    - **Low Shelf**: High/low frequency shelving with adjustable slope
    - **High Shelf**: High frequency shelving with adjustable slope  
    - **Notch**: Deep cut at specific frequency (similar to NotchFilter)
    - **Cut/Boost**: Asymmetric cut/boost filter based on allpass structure
    
    Key Features:
    - Independent frequency, gain, and Q/bandwidth control
    - Multiple filter topologies for different EQ applications
    - Real-time parameter changes without artifacts
    - Optimized coefficient calculation for audio rates
    - Stable over wide parameter ranges
    
    Applications:
    - Multi-band parametric equalizers
    - Audio mixing and mastering
    - Live sound feedback suppression
    - Tone shaping and sound design
    - Crossover network design
    - Room correction systems
    
    The filter uses a dual-precision architecture where:
    - SampleType: for audio buffer processing (float/double)
    - CoeffType: for internal calculations (defaults to double for precision)
    
    @see NotchFilter, StateVariableFilter, RbjFilter
*/
template <typename SampleType, typename CoeffType = double>
class ParametricFilter : public FilterBase<SampleType, CoeffType>
{
public:
    //==============================================================================
    /** Filter types for different parametric EQ applications */
    enum class Type
    {
        bell,       /** Bell/peak filter - traditional parametric EQ band */
        lowShelf,   /** Low frequency shelf filter */
        highShelf,  /** High frequency shelf filter */
        notch,      /** Deep notch filter */
        cutBoost    /** Cut/boost filter with allpass structure */
    };

    //==============================================================================
    /** Default constructor */
    ParametricFilter()
        : filterType (Type::bell)
        , centerFreq (static_cast<CoeffType> (1000.0))
        , gainDb (static_cast<CoeffType> (0.0))
        , qFactor (static_cast<CoeffType> (1.0))
    {
        setParameters (centerFreq, gainDb, qFactor, 44100.0);
    }

    /** Constructor with parameters */
    ParametricFilter (Type type, CoeffType frequency, CoeffType gain, CoeffType Q, double sampleRate)
        : filterType (type)
        , centerFreq (frequency)
        , gainDb (gain)
        , qFactor (Q)
    {
        setParameters (frequency, gain, Q, sampleRate);
    }

    //==============================================================================
    /** @internal */
    void reset() noexcept override
    {
        x1 = x2 = static_cast<SampleType> (0.0);
        y1 = y2 = static_cast<SampleType> (0.0);
        
        // Reset shelf filter state
        shelfPrevIn = shelfPrevOut = static_cast<SampleType> (0.0);
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
        if (filterType == Type::lowShelf || filterType == Type::highShelf)
        {
            return processShelf (inputSample);
        }
        else
        {
            return processBiquad (inputSample);
        }
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
        
        if (filterType == Type::lowShelf || filterType == Type::highShelf)
        {
            return getComplexResponseShelf (z);
        }
        else
        {
            return getComplexResponseBiquad (z);
        }
    }

    //==============================================================================
    /** 
        Sets all filter parameters.
        
        @param frequency   The center frequency in Hz (or cutoff for shelf filters)
        @param gain        The gain in dB (positive = boost, negative = cut)
        @param Q           The Q factor (higher Q = narrower band)
        @param sampleRate  The sample rate in Hz
        @param type        The filter type (optional, defaults to current)
    */
    void setParameters (CoeffType frequency, CoeffType gain, CoeffType Q, double sampleRate, Type type = Type::bell) noexcept
    {
        if (type != filterType)
        {
            filterType = type;
            reset();
        }
        
        centerFreq = frequency;
        gainDb = jlimit (static_cast<CoeffType> (-40.0), static_cast<CoeffType> (40.0), gain);
        qFactor = jmax (static_cast<CoeffType> (0.1), Q);
        this->sampleRate = sampleRate;
        
        updateCoefficients();
    }

    /** 
        Sets the center frequency.
        
        @param frequency  The new center frequency in Hz
    */
    void setFrequency (CoeffType frequency) noexcept
    {
        centerFreq = frequency;
        updateCoefficients();
    }

    /** 
        Sets the gain in dB.
        
        @param gain  The gain in dB (positive = boost, negative = cut)
    */
    void setGain (CoeffType gain) noexcept
    {
        gainDb = jlimit (static_cast<CoeffType> (-40.0), static_cast<CoeffType> (40.0), gain);
        updateCoefficients();
    }

    /** 
        Sets the Q factor.
        
        @param Q  The Q factor (higher Q = narrower band)
    */
    void setQ (CoeffType Q) noexcept
    {
        qFactor = jmax (static_cast<CoeffType> (0.1), Q);
        updateCoefficients();
    }

    /** 
        Sets the bandwidth in octaves (alternative to Q).
        
        @param bandwidth  The bandwidth in octaves
    */
    void setBandwidth (CoeffType bandwidth) noexcept
    {
        // Convert bandwidth to Q: Q = 1 / (2 * sinh(ln(2)/2 * BW))
        const auto bw = jmax (static_cast<CoeffType> (0.1), bandwidth);
        qFactor = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (2.0) * 
                   std::sinh (MathConstants<CoeffType>::ln2 * bw * static_cast<CoeffType> (0.5)));
        updateCoefficients();
    }

    /** 
        Changes the filter type.
        
        @param type  The new filter type
    */
    void setType (Type type) noexcept
    {
        if (filterType != type)
        {
            filterType = type;
            reset();
            updateCoefficients();
        }
    }

    //==============================================================================
    /** Gets the current center frequency */
    CoeffType getFrequency() const noexcept { return centerFreq; }

    /** Gets the current gain in dB */
    CoeffType getGain() const noexcept { return gainDb; }

    /** Gets the current Q factor */
    CoeffType getQ() const noexcept { return qFactor; }

    /** Gets the current bandwidth in octaves */
    CoeffType getBandwidth() const noexcept
    {
        // Convert Q to bandwidth: BW = (2 / ln(2)) * asinh(1 / (2*Q))
        return (static_cast<CoeffType> (2.0) / MathConstants<CoeffType>::ln2) * 
               std::asinh (static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (2.0) * qFactor));
    }

    /** Gets the current filter type */
    Type getType() const noexcept { return filterType; }

    /** Gets whether the filter is currently boosting (gain > 0) */
    bool isBoosting() const noexcept { return gainDb > static_cast<CoeffType> (0.0); }

    /** Gets whether the filter is currently cutting (gain < 0) */
    bool isCutting() const noexcept { return gainDb < static_cast<CoeffType> (0.0); }

private:
    //==============================================================================
    Type filterType;
    CoeffType centerFreq;
    CoeffType gainDb;
    CoeffType qFactor;
    
    // Biquad state variables
    CoeffType b0 = static_cast<CoeffType> (1.0);
    CoeffType b1 = static_cast<CoeffType> (0.0);
    CoeffType b2 = static_cast<CoeffType> (0.0);
    CoeffType a1 = static_cast<CoeffType> (0.0);
    CoeffType a2 = static_cast<CoeffType> (0.0);
    
    SampleType x1 = static_cast<SampleType> (0.0);
    SampleType x2 = static_cast<SampleType> (0.0);
    SampleType y1 = static_cast<SampleType> (0.0);
    SampleType y2 = static_cast<SampleType> (0.0);
    
    // Shelf filter state variables (1st order)
    CoeffType shelfA0 = static_cast<CoeffType> (1.0);
    CoeffType shelfA1 = static_cast<CoeffType> (0.0);
    CoeffType shelfB = static_cast<CoeffType> (0.0);
    
    SampleType shelfPrevIn = static_cast<SampleType> (0.0);
    SampleType shelfPrevOut = static_cast<SampleType> (0.0);

    //==============================================================================
    void updateCoefficients() noexcept
    {
        if (this->sampleRate <= 0.0)
            return;

        switch (filterType)
        {
            case Type::bell:
                updateBellCoeffs();
                break;
            case Type::lowShelf:
                updateLowShelfCoeffs();
                break;
            case Type::highShelf:
                updateHighShelfCoeffs();
                break;
            case Type::notch:
                updateNotchCoeffs();
                break;
            case Type::cutBoost:
                updateCutBoostCoeffs();
                break;
        }
    }

    void updateBellCoeffs() noexcept
    {
        // RBJ parametric/peaking EQ
        const auto omega = MathConstants<CoeffType>::twoPi * centerFreq / this->sampleRate;
        const auto sin_omega = std::sin (omega);
        const auto cos_omega = std::cos (omega);
        const auto A = std::pow (static_cast<CoeffType> (10.0), gainDb / static_cast<CoeffType> (40.0));
        const auto alpha = sin_omega / (static_cast<CoeffType> (2.0) * qFactor);
        
        const auto b0_raw = static_cast<CoeffType> (1.0) + alpha * A;
        const auto b1_raw = static_cast<CoeffType> (-2.0) * cos_omega;
        const auto b2_raw = static_cast<CoeffType> (1.0) - alpha * A;
        const auto a0_raw = static_cast<CoeffType> (1.0) + alpha / A;
        const auto a1_raw = static_cast<CoeffType> (-2.0) * cos_omega;
        const auto a2_raw = static_cast<CoeffType> (1.0) - alpha / A;
        
        // Normalize by a0
        b0 = b0_raw / a0_raw;
        b1 = b1_raw / a0_raw;
        b2 = b2_raw / a0_raw;
        a1 = a1_raw / a0_raw;
        a2 = a2_raw / a0_raw;
    }

    void updateLowShelfCoeffs() noexcept
    {
        // Based on spuce iir_shelf implementation
        const auto omega = MathConstants<CoeffType>::twoPi * centerFreq / this->sampleRate;
        const auto A = std::pow (static_cast<CoeffType> (10.0), gainDb / static_cast<CoeffType> (20.0));
        
        // Calculate shelf parameters
        const auto ca = std::tan (omega * static_cast<CoeffType> (0.5)) * A;
        const auto cb = std::tan (omega * static_cast<CoeffType> (0.5));
        
        shelfB = (static_cast<CoeffType> (1.0) - cb) / (static_cast<CoeffType> (1.0) + cb);
        shelfA0 = (ca + static_cast<CoeffType> (1.0)) / (cb + static_cast<CoeffType> (1.0));
        shelfA1 = (static_cast<CoeffType> (1.0) - ca) / (static_cast<CoeffType> (1.0) + shelfB);
    }

    void updateHighShelfCoeffs() noexcept
    {
        // High shelf is similar to low shelf but with inverted frequency mapping
        const auto omega = MathConstants<CoeffType>::twoPi * centerFreq / this->sampleRate;
        const auto A = std::pow (static_cast<CoeffType> (10.0), gainDb / static_cast<CoeffType> (20.0));
        
        // For high shelf, use complementary frequency mapping
        const auto ca = std::tan ((MathConstants<CoeffType>::pi - omega) * static_cast<CoeffType> (0.5)) / A;
        const auto cb = std::tan ((MathConstants<CoeffType>::pi - omega) * static_cast<CoeffType> (0.5));
        
        shelfB = (static_cast<CoeffType> (1.0) - cb) / (static_cast<CoeffType> (1.0) + cb);
        shelfA0 = (ca + static_cast<CoeffType> (1.0)) / (cb + static_cast<CoeffType> (1.0));
        shelfA1 = (static_cast<CoeffType> (1.0) - ca) / (static_cast<CoeffType> (1.0) + shelfB);
    }

    void updateNotchCoeffs() noexcept
    {
        // RBJ notch filter
        const auto omega = MathConstants<CoeffType>::twoPi * centerFreq / this->sampleRate;
        const auto sin_omega = std::sin (omega);
        const auto cos_omega = std::cos (omega);
        const auto alpha = sin_omega / (static_cast<CoeffType> (2.0) * qFactor);
        
        const auto a0_raw = static_cast<CoeffType> (1.0) + alpha;
        
        b0 = static_cast<CoeffType> (1.0) / a0_raw;
        b1 = static_cast<CoeffType> (-2.0) * cos_omega / a0_raw;
        b2 = static_cast<CoeffType> (1.0) / a0_raw;
        a1 = static_cast<CoeffType> (-2.0) * cos_omega / a0_raw;
        a2 = (static_cast<CoeffType> (1.0) - alpha) / a0_raw;
    }

    void updateCutBoostCoeffs() noexcept
    {
        // Based on spuce cutboost implementation
        const auto normalizedFreq = centerFreq / this->sampleRate;
        const auto depth = static_cast<CoeffType> (1.0) / (qFactor + static_cast<CoeffType> (1.0)); // Convert Q to depth
        const auto k2 = depth * static_cast<CoeffType> (0.95);
        const auto cosine = std::cos (MathConstants<CoeffType>::twoPi * normalizedFreq);
        const auto b_coeff = -cosine * (static_cast<CoeffType> (1.0) + k2);
        
        // Cut/boost control from gain
        const auto k0 = std::tanh (gainDb / static_cast<CoeffType> (20.0)); // Convert dB to linear factor
        const auto k = (static_cast<CoeffType> (1.0) - k0) / (static_cast<CoeffType> (1.0) + k0);
        const auto g = static_cast<CoeffType> (0.5) * (static_cast<CoeffType> (1.0) + k0);
        
        // Convert to biquad form: H(z) = g * (1 + k * G_allpass(z))
        b0 = g * (static_cast<CoeffType> (1.0) + k * k2);
        b1 = g * k * b_coeff;
        b2 = g * (static_cast<CoeffType> (1.0) + k * k2);
        a1 = b_coeff;
        a2 = k2;
    }

    //==============================================================================
    SampleType processBiquad (SampleType input) noexcept
    {
        const auto output = static_cast<SampleType> (b0) * input + 
                           static_cast<SampleType> (b1) * x1 + 
                           static_cast<SampleType> (b2) * x2 -
                           static_cast<SampleType> (a1) * y1 - 
                           static_cast<SampleType> (a2) * y2;
        
        // Shift delays
        x2 = x1;
        x1 = input;
        y2 = y1;
        y1 = output;
        
        return output;
    }

    SampleType processShelf (SampleType input) noexcept
    {
        // First-order shelf filter: H(z) = (a0 + a1*z^-1) / (1 + b*z^-1)
        const auto output = static_cast<SampleType> (shelfA0) * input - 
                           static_cast<SampleType> (shelfA1) * shelfPrevIn + 
                           static_cast<SampleType> (shelfB) * shelfPrevOut;
        
        // Shift delays
        shelfPrevIn = input;
        shelfPrevOut = output;
        
        return output;
    }

    //==============================================================================
    DspMath::Complex<CoeffType> getComplexResponseBiquad (const DspMath::Complex<CoeffType>& z) const noexcept
    {
        const auto z_inv = static_cast<CoeffType> (1.0) / z;
        const auto z_inv2 = z_inv * z_inv;
        
        const auto num = b0 + b1 * z_inv + b2 * z_inv2;
        const auto den = static_cast<CoeffType> (1.0) + a1 * z_inv + a2 * z_inv2;
        
        return num / den;
    }

    DspMath::Complex<CoeffType> getComplexResponseShelf (const DspMath::Complex<CoeffType>& z) const noexcept
    {
        const auto z_inv = static_cast<CoeffType> (1.0) / z;
        
        const auto num = shelfA0 - shelfA1 * z_inv;
        const auto den = static_cast<CoeffType> (1.0) + shelfB * z_inv;
        
        return num / den;
    }

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParametricFilter)
};

//==============================================================================
/** Type aliases for convenience */
using ParametricFilterFloat = ParametricFilter<float>;       // float samples, double coefficients (default)
using ParametricFilterDouble = ParametricFilter<double>;     // double samples, double coefficients (default)

} // namespace yup
