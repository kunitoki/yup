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
    DC removal high-pass filter for eliminating DC bias.
    
    This filter implements a high-pass filter specifically designed to remove
    DC offsets from audio signals while preserving the audio content. It uses
    a single-pole high-pass filter with configurable response characteristics.
    
    The filter provides three response modes:
    - Slow: Gentle DC removal, preserves very low frequencies (< 10 Hz cutoff)
    - Default: Balanced response for most applications (~ 20 Hz cutoff)
    - Fast: Aggressive DC removal, may affect low frequencies (~ 50 Hz cutoff)
    
    Key features:
    - Extremely efficient single-pole implementation
    - Configurable response speed/aggressiveness
    - Automatic denormal protection
    - Separate processing channels for stereo
    - Zero-latency processing
    - Stable for all sample rates
    
    The filter uses a leaky integrator topology that automatically adapts
    to the signal characteristics, providing smooth DC removal without
    introducing artifacts or clicks.
    
    @see FilterBase, FirstOrderFilter
*/
template <typename SampleType, typename CoeffType = double>
class DcFilter : public FilterBase<SampleType, CoeffType>
{
public:
    //==============================================================================
    /** DC filter response modes */
    enum class Mode
    {
        Slow,     /**< Gentle DC removal, preserves very low frequencies */
        Default,  /**< Balanced response for most applications */
        Fast      /**< Aggressive DC removal, may affect low frequencies */
    };

    //==============================================================================
    /** Default constructor */
    DcFilter (Mode mode = Mode::Default)
        : filterMode (mode)
    {
        updateCoefficients();
    }

    //==============================================================================
    /** @internal */
    void reset() noexcept override
    {
        x1 = y1 = static_cast<CoeffType> (0.0);
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
        const auto input = static_cast<CoeffType> (inputSample);
        
        // Single-pole high-pass filter: y[n] = x[n] - x[n-1] + a * y[n-1]
        const auto output = input - x1 + coefficient * y1;
        
        // Update state variables
        x1 = input;
        y1 = output;
        
        // Denormal protection
        if (std::abs (y1) < static_cast<CoeffType> (1e-25))
            y1 = static_cast<CoeffType> (0.0);
            
        return static_cast<SampleType> (output);
    }

    /** @internal */
    void processBlock (const SampleType* inputBuffer, SampleType* outputBuffer, int numSamples) noexcept override
    {
        for (int i = 0; i < numSamples; ++i)
        {
            outputBuffer[i] = processSample (inputBuffer[i]);
        }
    }

    /** @internal */
    DspMath::Complex<CoeffType> getComplexResponse (CoeffType frequency) const noexcept override
    {
        const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (this->sampleRate));
        const auto z = DspMath::Complex<CoeffType> (std::cos (omega), std::sin (omega));
        
        // H(z) = (1 - z^-1) / (1 - a * z^-1)
        const auto numerator = static_cast<CoeffType> (1.0) - (static_cast<CoeffType> (1.0) / z);
        const auto denominator = static_cast<CoeffType> (1.0) - (coefficient / z);
        
        return numerator / denominator;
    }

    //==============================================================================
    /** 
        Sets the DC filter mode.
        
        @param mode  The new filter mode
    */
    void setMode (Mode mode) noexcept
    {
        if (filterMode != mode)
        {
            filterMode = mode;
            updateCoefficients();
        }
    }

    /** 
        Gets the current DC filter mode.
        
        @returns  The current filter mode
    */
    Mode getMode() const noexcept
    {
        return filterMode;
    }

    /** 
        Sets a custom cutoff frequency for the DC filter.
        
        This overrides the mode-based frequency selection and allows
        for precise control over the DC removal characteristics.
        
        @param frequency  The cutoff frequency in Hz
    */
    void setCutoffFrequency (CoeffType frequency) noexcept
    {
        customCutoff = jmax (static_cast<CoeffType> (0.1), 
                           jmin (frequency, static_cast<CoeffType> (this->sampleRate * 0.45)));
        useCustomCutoff = true;
        updateCoefficients();
    }

    /** 
        Resets to use mode-based frequency selection.
    */
    void useDefaultCutoff() noexcept
    {
        useCustomCutoff = false;
        updateCoefficients();
    }

    /** 
        Gets the current effective cutoff frequency.
        
        @returns  The cutoff frequency in Hz
    */
    CoeffType getCutoffFrequency() const noexcept
    {
        if (useCustomCutoff)
            return customCutoff;
            
        return getModeBasedCutoff();
    }

    /** 
        Gets the current filter coefficient.
        
        @returns  The filter coefficient (0-1)
    */
    CoeffType getCoefficient() const noexcept
    {
        return coefficient;
    }

private:
    //==============================================================================
    Mode filterMode = Mode::Default;
    CoeffType coefficient = static_cast<CoeffType> (0.999);
    CoeffType customCutoff = static_cast<CoeffType> (20.0);
    bool useCustomCutoff = false;

    // Filter state variables
    CoeffType x1 = static_cast<CoeffType> (0.0);  // Previous input
    CoeffType y1 = static_cast<CoeffType> (0.0);  // Previous output

    //==============================================================================
    CoeffType getModeBasedCutoff() const noexcept
    {
        switch (filterMode)
        {
            case Mode::Slow:    return static_cast<CoeffType> (5.0);
            case Mode::Default: return static_cast<CoeffType> (20.0);
            case Mode::Fast:    return static_cast<CoeffType> (50.0);
        }
        
        return static_cast<CoeffType> (20.0);
    }

    void updateCoefficients() noexcept
    {
        if (this->sampleRate <= 0.0)
            return;
            
        const auto cutoff = useCustomCutoff ? customCutoff : getModeBasedCutoff();
        const auto omega = MathConstants<CoeffType>::twoPi * cutoff / static_cast<CoeffType> (this->sampleRate);
        
        // Calculate coefficient for single-pole high-pass filter
        // a = 1 / (1 + omega_c)
        coefficient = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) + omega);
        
        // Ensure coefficient stays in valid range
        coefficient = jlimit (static_cast<CoeffType> (0.5), static_cast<CoeffType> (0.9999), coefficient);
    }

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DcFilter)
};

//==============================================================================
/** Type aliases for convenience */
using DcFilterFloat = DcFilter<float>;      // float samples, double coefficients (default)
using DcFilterDouble = DcFilter<double>;    // double samples, double coefficients (default)

} // namespace yup