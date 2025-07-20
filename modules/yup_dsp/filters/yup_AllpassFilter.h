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
    First-order Allpass filter with programmable gain and delay.
    
    This filter implements a first-order allpass section of the form:
    G(z,n) = (a*z^n + 1)/(z^n + a)
    
    Where:
    - a is the allpass coefficient (gain parameter)
    - n is the delay in samples (programmable)
    
    Key characteristics:
    - Unity magnitude response at all frequencies
    - Frequency-dependent phase response
    - Programmable delay from 1 to multiple samples
    - Smooth phase transitions
    - No amplitude coloration
    
    Features:
    - Configurable gain coefficient (-1.0 to 1.0)
    - Variable delay length (1 to 32 samples)
    - Real-time coefficient updates
    - Efficient circular buffer implementation
    - Zero-latency processing with internal delay
    
    Applications:
    - Phase adjustment in crossovers
    - Reverb and delay effects
    - Phaser and chorus effects
    - Frequency-dependent time alignment
    - Creating complex phase responses
    
    The filter uses a dual-precision architecture where:
    - SampleType: for audio buffer processing (float/double)
    - CoeffType: for internal calculations (defaults to double for precision)
    
    @see FilterBase, SecondOrderAllpass, ButterworthAllpass
*/
template <typename SampleType, typename CoeffType = double>
class FirstOrderAllpass : public FilterBase<SampleType, CoeffType>
{
public:
    //==============================================================================
    /** Constructor with gain and delay parameters */
    explicit FirstOrderAllpass (CoeffType gain = static_cast<CoeffType> (0.5), int delaySamples = 1)
        : gainCoeff (gain), delayLength (delaySamples)
    {
        setParameters (gain, delaySamples);
    }

    //==============================================================================
    /** @internal */
    void reset() noexcept override
    {
        std::fill (multBuffer.begin(), multBuffer.end(), static_cast<CoeffType> (0.0));
        std::fill (sumBuffer.begin(), sumBuffer.end(), static_cast<CoeffType> (0.0));
        writeIndex = 0;
    }

    /** @internal */
    void prepare (double sampleRate, int maximumBlockSize) noexcept override
    {
        this->sampleRate = sampleRate;
        this->maximumBlockSize = maximumBlockSize;
        
        // Ensure buffers are sized correctly
        if (static_cast<int> (multBuffer.size()) != delayLength)
        {
            multBuffer.resize (static_cast<size_t> (delayLength));
            sumBuffer.resize (static_cast<size_t> (delayLength));
            reset();
        }
    }

    /** @internal */
    SampleType processSample (SampleType inputSample) noexcept override
    {
        // Convert input to coefficient precision
        const auto input = static_cast<CoeffType> (inputSample);
        
        // Calculate read index (delay samples ago)
        const auto readIndex = (writeIndex + delayLength - (delayLength - 1)) % delayLength;
        
        // Get delayed outputs
        const auto delayedSum = sumBuffer[readIndex];
        const auto delayedMult = multBuffer[readIndex];
        
        // Calculate current sum and multiplied value
        const auto currentSum = input + delayedMult;
        const auto currentMult = -gainCoeff * currentSum;
        
        // Calculate output
        const auto output = delayedSum - currentMult;
        
        // Store values in circular buffers
        multBuffer[writeIndex] = currentMult;
        sumBuffer[writeIndex] = currentSum;
        
        // Advance write index
        writeIndex = (writeIndex + 1) % delayLength;
        
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
        const auto z = DspMath::polar (static_cast<CoeffType> (1.0), -omega);
        
        // H(z) = (a*z^(-n) + 1) / (z^(-n) + a)
        const auto z_delayed = std::pow (z, -static_cast<CoeffType> (delayLength));
        const auto numerator = gainCoeff * z_delayed + static_cast<CoeffType> (1.0);
        const auto denominator = z_delayed + gainCoeff;
        
        return numerator / denominator;
    }

    //==============================================================================
    /** 
        Sets the allpass parameters.
        
        @param gain          The gain coefficient (-1.0 to 1.0)
        @param delaySamples  The delay in samples (1 to 32)
    */
    void setParameters (CoeffType gain, int delaySamples) noexcept
    {
        gainCoeff = jlimit (static_cast<CoeffType> (-1.0), static_cast<CoeffType> (1.0), gain);
        const auto newDelay = jlimit (1, 32, delaySamples);
        
        if (newDelay != delayLength)
        {
            delayLength = newDelay;
            multBuffer.resize (static_cast<size_t> (delayLength));
            sumBuffer.resize (static_cast<size_t> (delayLength));
            reset();
        }
    }

    /** 
        Sets just the gain coefficient.
        
        @param gain  The new gain coefficient (-1.0 to 1.0)
    */
    void setGain (CoeffType gain) noexcept
    {
        gainCoeff = jlimit (static_cast<CoeffType> (-1.0), static_cast<CoeffType> (1.0), gain);
    }

    /** 
        Sets just the delay length.
        
        @param delaySamples  The new delay in samples (1 to 32)
    */
    void setDelay (int delaySamples) noexcept
    {
        const auto newDelay = jlimit (1, 32, delaySamples);
        
        if (newDelay != delayLength)
        {
            delayLength = newDelay;
            multBuffer.resize (static_cast<size_t> (delayLength));
            sumBuffer.resize (static_cast<size_t> (delayLength));
            reset();
        }
    }

    /** 
        Gets the current gain coefficient.
        
        @returns  The gain coefficient
    */
    CoeffType getGain() const noexcept
    {
        return gainCoeff;
    }

    /** 
        Gets the current delay length.
        
        @returns  The delay in samples
    */
    int getDelay() const noexcept
    {
        return delayLength;
    }

    //==============================================================================
    /** 
        Gets the phase response at the given frequency.
        
        @param frequency  The frequency in Hz
        @returns         The phase response in radians
    */
    CoeffType getPhaseResponse (CoeffType frequency) const noexcept
    {
        const auto response = getComplexResponse (frequency);
        return std::arg (response);
    }

    /** 
        Gets the group delay at the given frequency.
        
        @param frequency  The frequency in Hz
        @returns         The group delay in samples
    */
    CoeffType getGroupDelay (CoeffType frequency) const noexcept
    {
        // For a first-order allpass, group delay is approximately:
        // τ = (1 - a²) / (1 + a² - 2a*cos(ωT))
        const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (this->sampleRate));
        const auto a2 = gainCoeff * gainCoeff;
        const auto cosOmega = std::cos (omega * static_cast<CoeffType> (delayLength));
        
        const auto numerator = static_cast<CoeffType> (1.0) - a2;
        const auto denominator = static_cast<CoeffType> (1.0) + a2 - static_cast<CoeffType> (2.0) * gainCoeff * cosOmega;
        
        return numerator / jmax (denominator, static_cast<CoeffType> (1e-12));
    }

private:
    //==============================================================================
    CoeffType gainCoeff = static_cast<CoeffType> (0.5);
    int delayLength = 1;
    
    // Circular buffers for delay implementation
    std::vector<CoeffType> multBuffer;  // Stores multiplied values
    std::vector<CoeffType> sumBuffer;   // Stores sum values
    int writeIndex = 0;

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FirstOrderAllpass)
};

//==============================================================================
/** 
    Second-order Allpass filter implementation.
    
    This filter implements a second-order allpass section of the form:
    G(z) = (z² + b*z + a) / (a*z² + b*z + 1)
    
    Key characteristics:
    - Unity magnitude response at all frequencies
    - Configurable phase response with two parameters
    - More complex phase behavior than first-order
    - Stable for |a| < 1 and appropriate b values
    
    Applications:
    - Advanced phase correction
    - Reverb diffusion networks
    - Complex phasing effects
    - Crossover phase alignment
    
    @see FirstOrderAllpass, ButterworthAllpass
*/
template <typename SampleType, typename CoeffType = double>
class SecondOrderAllpass : public FilterBase<SampleType, CoeffType>
{
public:
    //==============================================================================
    /** Constructor with coefficients */
    explicit SecondOrderAllpass (CoeffType aCoeff = static_cast<CoeffType> (0.5), 
                                CoeffType bCoeff = static_cast<CoeffType> (0.0))
        : a (aCoeff), b (bCoeff)
    {
        reset();
    }

    //==============================================================================
    /** @internal */
    void reset() noexcept override
    {
        std::fill (inputHistory.begin(), inputHistory.end(), static_cast<CoeffType> (0.0));
        std::fill (outputHistory.begin(), outputHistory.end(), static_cast<CoeffType> (0.0));
    }

    /** @internal */
    void prepare (double sampleRate, int maximumBlockSize) noexcept override
    {
        this->sampleRate = sampleRate;
        this->maximumBlockSize = maximumBlockSize;
    }

    /** @internal */
    SampleType processSample (SampleType inputSample) noexcept override
    {
        // Convert input to coefficient precision
        const auto input = static_cast<CoeffType> (inputSample);
        
        // Shift input history
        inputHistory[0] = inputHistory[1];
        inputHistory[1] = inputHistory[2];
        inputHistory[2] = input;
        
        // Shift output history
        outputHistory[0] = outputHistory[1];
        outputHistory[1] = outputHistory[2];
        
        // Calculate new output using the allpass difference equation
        // y[n] = a*(x[n-1] - y[n-1]) + b*(x[n] - y[n-2]) + x[n-2]
        const auto output = a * (inputHistory[1] - outputHistory[1]) + 
                          b * (inputHistory[2] - outputHistory[0]) + 
                          inputHistory[0];
        
        outputHistory[2] = output;
        
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
        const auto z = DspMath::polar (static_cast<CoeffType> (1.0), -omega);
        const auto z2 = z * z;
        
        // H(z) = (z² + b*z + a) / (a*z² + b*z + 1)
        const auto numerator = z2 + b * z + a;
        const auto denominator = a * z2 + b * z + static_cast<CoeffType> (1.0);
        
        return numerator / denominator;
    }

    //==============================================================================
    /** 
        Sets the allpass coefficients.
        
        @param aCoeff  The 'a' coefficient (should be |a| < 1 for stability)
        @param bCoeff  The 'b' coefficient
    */
    void setCoefficients (CoeffType aCoeff, CoeffType bCoeff) noexcept
    {
        a = jlimit (static_cast<CoeffType> (-0.99), static_cast<CoeffType> (0.99), aCoeff);
        b = bCoeff;
    }

    /** 
        Gets the 'a' coefficient.
        
        @returns  The 'a' coefficient
    */
    CoeffType getA() const noexcept
    {
        return a;
    }

    /** 
        Gets the 'b' coefficient.
        
        @returns  The 'b' coefficient
    */
    CoeffType getB() const noexcept
    {
        return b;
    }

    //==============================================================================
    /** 
        Gets the phase response at the given frequency.
        
        @param frequency  The frequency in Hz
        @returns         The phase response in radians
    */
    CoeffType getPhaseResponse (CoeffType frequency) const noexcept
    {
        const auto response = getComplexResponse (frequency);
        return std::arg (response);
    }

private:
    //==============================================================================
    CoeffType a = static_cast<CoeffType> (0.5);
    CoeffType b = static_cast<CoeffType> (0.0);
    
    // History buffers [n-2, n-1, n]
    std::array<CoeffType, 3> inputHistory = {};
    std::array<CoeffType, 3> outputHistory = {};

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SecondOrderAllpass)
};

//==============================================================================
/** Type aliases for convenience */
using FirstOrderAllpassFloat = FirstOrderAllpass<float>;            // float samples, double coefficients (default)
using FirstOrderAllpassDouble = FirstOrderAllpass<double>;          // double samples, double coefficients (default)

using SecondOrderAllpassFloat = SecondOrderAllpass<float>;          // float samples, double coefficients (default)
using SecondOrderAllpassDouble = SecondOrderAllpass<double>;        // double samples, double coefficients (default)

} // namespace yup