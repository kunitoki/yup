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
    Virtual Analog State Variable Filter using Topology Preserving Transform (TPT).
    
    This filter provides excellent analog circuit emulation characteristics with
    simultaneous lowpass, highpass, bandpass, and notch outputs. The TPT method
    ensures zero-delay feedback and maintains the filter's character across all
    sample rates.
    
    Key features:
    - Zero-delay feedback topology
    - Simultaneous multi-mode outputs
    - Resonance up to self-oscillation
    - Excellent frequency response matching analog circuits
    - Stable across all frequencies and resonance settings
    
    The filter uses a dual-precision architecture where:
    - SampleType: for audio buffer processing (float/double)
    - CoeffType: for internal calculations (defaults to double for precision)
    
    @see FilterBase, StateVariableFilter
*/
template <typename SampleType, typename CoeffType = double>
class VirtualAnalogSvf : public FilterBase<SampleType, CoeffType>
{
public:
    //==============================================================================
    /** Filter output structure containing all simultaneous outputs */
    struct FilterOutputs
    {
        SampleType lowpass = 0;    /**< Lowpass output */
        SampleType highpass = 0;   /**< Highpass output */
        SampleType bandpass = 0;   /**< Bandpass output */
        SampleType notch = 0;      /**< Notch (band-reject) output */
        SampleType allpass = 0;    /**< Allpass output */
        SampleType peak = 0;       /**< Peak output (bandpass with gain compensation) */
    };

    /** Filter mode enumeration for single-output processing */
    enum class Mode
    {
        lowpass,   /**< Lowpass mode */
        highpass,  /**< Highpass mode */
        bandpass,  /**< Bandpass mode */
        notch,     /**< Notch mode */
        allpass,   /**< Allpass mode */
        peak       /**< Peak mode */
    };

    //==============================================================================
    /** Constructor with optional parameters */
    explicit VirtualAnalogSvf (CoeffType frequency = static_cast<CoeffType> (1000.0), 
                               CoeffType resonance = static_cast<CoeffType> (0.1),
                               Mode mode = Mode::lowpass)
        : cutoffFreq (frequency), resonanceAmount (resonance), filterMode (mode)
    {
        updateCoefficients();
    }

    //==============================================================================
    /** @internal */
    void reset() noexcept override
    {
        ic1eq = static_cast<CoeffType> (0.0);
        ic2eq = static_cast<CoeffType> (0.0);
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
        const auto outputs = processMultiSample (inputSample);
        
        switch (filterMode)
        {
            case Mode::lowpass:  return outputs.lowpass;
            case Mode::highpass: return outputs.highpass;
            case Mode::bandpass: return outputs.bandpass;
            case Mode::notch:    return outputs.notch;
            case Mode::allpass:  return outputs.allpass;
            case Mode::peak:     return outputs.peak;
            default:             return outputs.lowpass;
        }
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
        const auto z = DspMath::polar (static_cast<CoeffType> (1.0), -omega);
        
        // TPT SVF transfer function approximation for lowpass
        const auto s = (z - static_cast<CoeffType> (1.0)) / (z + static_cast<CoeffType> (1.0));
        const auto omega_c = DspMath::frequencyToAngular (cutoffFreq, static_cast<CoeffType> (this->sampleRate));
        const auto g_norm = std::tan (omega_c / static_cast<CoeffType> (2.0));
        
        const auto denominator = static_cast<CoeffType> (1.0) + g_norm * (static_cast<CoeffType> (1.0) + k) + g_norm * g_norm;
        const auto numerator = g_norm * g_norm;
        
        return numerator / denominator;
    }

    //==============================================================================
    /** 
        Sets the cutoff frequency.
        
        @param frequency  The cutoff frequency in Hz
    */
    void setCutoffFrequency (CoeffType frequency) noexcept
    {
        cutoffFreq = jmax (static_cast<CoeffType> (1.0), frequency);
        updateCoefficients();
    }

    /** 
        Sets the resonance amount.
        
        @param resonance  The resonance amount (0.0 to 1.0, where 1.0 approaches self-oscillation)
    */
    void setResonance (CoeffType resonance) noexcept
    {
        resonanceAmount = jlimit (static_cast<CoeffType> (0.0), static_cast<CoeffType> (0.99), resonance);
        updateCoefficients();
    }

    /** 
        Sets the filter mode for single-output processing.
        
        @param mode  The desired filter mode
    */
    void setMode (Mode mode) noexcept
    {
        filterMode = mode;
    }

    /** 
        Sets all parameters simultaneously.
        
        @param frequency  The cutoff frequency in Hz
        @param resonance  The resonance amount (0.0 to 1.0)
        @param mode       The filter mode
    */
    void setParameters (CoeffType frequency, CoeffType resonance, Mode mode = Mode::lowpass) noexcept
    {
        cutoffFreq = jmax (static_cast<CoeffType> (1.0), frequency);
        resonanceAmount = jlimit (static_cast<CoeffType> (0.0), static_cast<CoeffType> (0.99), resonance);
        filterMode = mode;
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
        Gets the current resonance amount.
        
        @returns  The resonance amount
    */
    CoeffType getResonance() const noexcept
    {
        return resonanceAmount;
    }

    /** 
        Gets the current filter mode.
        
        @returns  The current filter mode
    */
    Mode getMode() const noexcept
    {
        return filterMode;
    }

    //==============================================================================
    /** 
        Processes a sample and returns all filter outputs simultaneously.
        
        This is the most efficient way to get multiple outputs from the filter.
        
        @param inputSample  The input sample
        @returns           Structure containing all filter outputs
    */
    FilterOutputs processMultiSample (SampleType inputSample) noexcept
    {
        // Convert input to coefficient precision
        const auto input = static_cast<CoeffType> (inputSample);
        
        // TPT State Variable Filter implementation
        const auto v3 = input - ic2eq;
        const auto v1 = a1 * ic1eq + a2 * v3;
        const auto v2 = ic2eq + a2 * v1;
        
        // Update state variables
        ic1eq = static_cast<CoeffType> (2.0) * v1 - ic1eq;
        ic2eq = static_cast<CoeffType> (2.0) * v2 - ic2eq;
        
        // Generate all outputs
        FilterOutputs outputs;
        outputs.lowpass = static_cast<SampleType> (v2);
        outputs.bandpass = static_cast<SampleType> (v1);
        outputs.highpass = static_cast<SampleType> (v3);
        outputs.notch = static_cast<SampleType> (input - k * v1);
        outputs.allpass = static_cast<SampleType> (input - static_cast<CoeffType> (2.0) * k * v1);
        outputs.peak = static_cast<SampleType> (input - k * v1 - v2); // Peak with gain compensation
        
        return outputs;
    }

    /** 
        Processes a block with separate outputs for each filter type.
        
        @param inputBuffer     The input buffer
        @param lowpassBuffer   Buffer for lowpass output (can be nullptr)
        @param highpassBuffer  Buffer for highpass output (can be nullptr)
        @param bandpassBuffer  Buffer for bandpass output (can be nullptr)
        @param notchBuffer     Buffer for notch output (can be nullptr)
        @param numSamples      Number of samples to process
    */
    void processMultiBlock (const SampleType* inputBuffer,
                           SampleType* lowpassBuffer,
                           SampleType* highpassBuffer,
                           SampleType* bandpassBuffer,
                           SampleType* notchBuffer,
                           int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto outputs = processMultiSample (inputBuffer[i]);
            
            if (lowpassBuffer)  lowpassBuffer[i] = outputs.lowpass;
            if (highpassBuffer) highpassBuffer[i] = outputs.highpass;
            if (bandpassBuffer) bandpassBuffer[i] = outputs.bandpass;
            if (notchBuffer)    notchBuffer[i] = outputs.notch;
        }
    }

    //==============================================================================
    /** 
        Gets the lowpass frequency response at the given frequency.
        
        @param frequency  The frequency in Hz
        @returns         The magnitude response
    */
    CoeffType getLowpassMagnitudeResponse (CoeffType frequency) const noexcept
    {
        const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (this->sampleRate));
        const auto omega_c = DspMath::frequencyToAngular (cutoffFreq, static_cast<CoeffType> (this->sampleRate));
        
        // Analog SVF lowpass response approximation
        const auto ratio = omega / omega_c;
        const auto denominator = static_cast<CoeffType> (1.0) + 
                               static_cast<CoeffType> (2.0) * (static_cast<CoeffType> (1.0) - resonanceAmount) * ratio + 
                               ratio * ratio;
        
        return static_cast<CoeffType> (1.0) / std::sqrt (denominator);
    }

    /** 
        Gets the highpass frequency response at the given frequency.
        
        @param frequency  The frequency in Hz
        @returns         The magnitude response
    */
    CoeffType getHighpassMagnitudeResponse (CoeffType frequency) const noexcept
    {
        const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (this->sampleRate));
        const auto omega_c = DspMath::frequencyToAngular (cutoffFreq, static_cast<CoeffType> (this->sampleRate));
        
        // Analog SVF highpass response approximation
        const auto ratio = omega / omega_c;
        const auto denominator = static_cast<CoeffType> (1.0) + 
                               static_cast<CoeffType> (2.0) * (static_cast<CoeffType> (1.0) - resonanceAmount) * ratio + 
                               ratio * ratio;
        
        return (ratio * ratio) / std::sqrt (denominator);
    }

    /** 
        Gets the bandpass frequency response at the given frequency.
        
        @param frequency  The frequency in Hz
        @returns         The magnitude response
    */
    CoeffType getBandpassMagnitudeResponse (CoeffType frequency) const noexcept
    {
        const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (this->sampleRate));
        const auto omega_c = DspMath::frequencyToAngular (cutoffFreq, static_cast<CoeffType> (this->sampleRate));
        
        // Analog SVF bandpass response approximation
        const auto ratio = omega / omega_c;
        const auto qFactor = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (2.0) * (static_cast<CoeffType> (1.0) - resonanceAmount));
        const auto denominator = static_cast<CoeffType> (1.0) + 
                               (ratio / qFactor) * (ratio / qFactor) + 
                               ratio * ratio;
        
        return (ratio / qFactor) / std::sqrt (denominator);
    }

private:
    //==============================================================================
    CoeffType cutoffFreq = static_cast<CoeffType> (1000.0);
    CoeffType resonanceAmount = static_cast<CoeffType> (0.1);
    Mode filterMode = Mode::lowpass;

    // TPT coefficients
    CoeffType g = static_cast<CoeffType> (0.0);  // Warped frequency parameter
    CoeffType k = static_cast<CoeffType> (0.0);  // Resonance parameter
    CoeffType a1 = static_cast<CoeffType> (0.0); // Coefficient 1
    CoeffType a2 = static_cast<CoeffType> (0.0); // Coefficient 2

    // State variables (integrator states)
    CoeffType ic1eq = static_cast<CoeffType> (0.0);  // First integrator state
    CoeffType ic2eq = static_cast<CoeffType> (0.0);  // Second integrator state

    //==============================================================================
    /** Updates the filter coefficients based on current parameters */
    void updateCoefficients() noexcept
    {
        const auto coeffs = FilterDesigner<CoeffType>::designTptSvf (cutoffFreq, resonanceAmount, this->sampleRate);
        
        // Extract coefficients from designer
        g = coeffs[0];
        k = coeffs[1];
        const auto G = coeffs[2];
        
        // Compute derived coefficients
        a1 = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) + g * (g + k));
        a2 = g * a1;
    }

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VirtualAnalogSvf)
};

//==============================================================================
/** Type aliases for convenience */
using VirtualAnalogSvfFloat = VirtualAnalogSvf<float>;       // float samples, double coefficients (default)
using VirtualAnalogSvfDouble = VirtualAnalogSvf<double>;     // double samples, double coefficients (default)

} // namespace yup
