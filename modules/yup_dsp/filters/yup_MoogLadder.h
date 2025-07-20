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
    Moog Ladder Filter implementation using Topology Preserving Transform (TPT).
    
    This filter emulates the classic Moog synthesizer ladder filter, providing
    the distinctive warm, creamy sound with characteristic resonance behavior.
    The implementation uses TPT for accurate analog circuit modeling with
    zero-delay feedback.
    
    Key features:
    - 4-pole lowpass characteristic (-24dB/octave)
    - Authentic Moog ladder topology
    - Resonance up to self-oscillation
    - Zero-delay feedback using TPT
    - Temperature compensation modeling
    - Drive/saturation modeling for analog warmth
    
    The filter uses a dual-precision architecture where:
    - SampleType: for audio buffer processing (float/double)
    - CoeffType: for internal calculations (defaults to double for precision)
    
    @see FilterBase, VirtualAnalogSvf
*/
template <typename SampleType, typename CoeffType = double>
class MoogLadder : public FilterBase<SampleType, CoeffType>
{
public:
    //==============================================================================
    /** Constructor with optional parameters */
    explicit MoogLadder (CoeffType frequency = static_cast<CoeffType> (1000.0), 
                         CoeffType resonance = static_cast<CoeffType> (0.1),
                         CoeffType drive = static_cast<CoeffType> (1.0))
        : cutoffFreq (frequency), resonanceAmount (resonance), driveAmount (drive)
    {
        updateCoefficients();
    }

    //==============================================================================
    /** @internal */
    void reset() noexcept override
    {
        V0 = V1 = V2 = V3 = static_cast<CoeffType> (0.0);
        s0 = s1 = s2 = s3 = static_cast<CoeffType> (0.0);
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
        // Convert input to coefficient precision
        auto input = static_cast<CoeffType> (inputSample);
        
        // Apply input drive/saturation
        input = applySaturation (input * driveAmount);
        
        // Compute the 4 node voltages
        const auto G0 = g / (static_cast<CoeffType> (1.0) + g);
        const auto G1 = g / (static_cast<CoeffType> (1.0) + g);
        const auto G2 = g / (static_cast<CoeffType> (1.0) + g);
        const auto G3 = g / (static_cast<CoeffType> (1.0) + g);
        
        // Calculate feedback amount with temperature compensation  
        const auto tempCompensatedK = k * (static_cast<CoeffType> (1.0) + static_cast<CoeffType> (0.0001) * cutoffFreq);
        
        // Input with feedback (correct Huovilainen model)
        const auto feedback = (s3 - passbandGain * input) * tempCompensatedK;
        const auto u = input - feedback;
        
        // 1st stage
        const auto v0 = u;
        const auto y0 = v0 * G0 + s0;
        s0 = static_cast<CoeffType> (2.0) * v0 * G0 - y0;
        
        // 2nd stage
        const auto v1 = y0;
        const auto y1 = v1 * G1 + s1;
        s1 = static_cast<CoeffType> (2.0) * v1 * G1 - y1;
        
        // 3rd stage
        const auto v2 = y1;
        const auto y2 = v2 * G2 + s2;
        s2 = static_cast<CoeffType> (2.0) * v2 * G2 - y2;
        
        // 4th stage
        const auto v3 = y2;
        const auto y3 = v3 * G3 + s3;
        s3 = static_cast<CoeffType> (2.0) * v3 * G3 - y3;
        
        // Store node voltages for other outputs if needed
        V0 = y0; V1 = y1; V2 = y2; V3 = y3;
        
        // Apply output compensation and convert back to SampleType
        return static_cast<SampleType> (y3 * outputGain);
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
        const auto s = DspMath::Complex<CoeffType> (0, omega);
        
        // 4-pole lowpass with resonance approximation
        const auto omega_c = DspMath::frequencyToAngular (cutoffFreq, static_cast<CoeffType> (this->sampleRate));
        const auto pole = DspMath::Complex<CoeffType> (-omega_c, 0);
        
        // 4th order response
        const auto response = static_cast<CoeffType> (1.0) / 
                             ((s - pole) * (s - pole) * (s - pole) * (s - pole));
        
        return response;
    }

    //==============================================================================
    /** 
        Sets the cutoff frequency.
        
        @param frequency  The cutoff frequency in Hz
    */
    void setCutoffFrequency (CoeffType frequency) noexcept
    {
        cutoffFreq = jmax (static_cast<CoeffType> (1.0), 
                          jmin (frequency, static_cast<CoeffType> (this->sampleRate * 0.49)));
        updateCoefficients();
    }

    /** 
        Sets the resonance amount.
        
        @param resonance  The resonance amount (0.0 to 1.0, where 1.0 approaches self-oscillation)
    */
    void setResonance (CoeffType resonance) noexcept
    {
        resonanceAmount = jlimit (static_cast<CoeffType> (0.0), static_cast<CoeffType> (0.999), resonance);
        updateCoefficients();
    }

    /** 
        Sets the input drive amount.
        
        @param drive  The drive amount (0.1 to 10.0, where 1.0 is unity gain)
    */
    void setDrive (CoeffType drive) noexcept
    {
        driveAmount = jlimit (static_cast<CoeffType> (0.1), static_cast<CoeffType> (10.0), drive);
    }

    /** 
        Sets all parameters simultaneously.
        
        @param frequency  The cutoff frequency in Hz
        @param resonance  The resonance amount (0.0 to 1.0)
        @param drive      The drive amount (0.1 to 10.0)
    */
    void setParameters (CoeffType frequency, CoeffType resonance, CoeffType drive = static_cast<CoeffType> (1.0)) noexcept
    {
        setCutoffFrequency (frequency);
        setResonance (resonance);
        setDrive (drive);
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
        Gets the current drive amount.
        
        @returns  The drive amount
    */
    CoeffType getDrive() const noexcept
    {
        return driveAmount;
    }

    /** 
        Sets the passband gain compensation factor.
        
        This helps compensate for energy loss in the passband at higher resonance values.
        
        @param gain  The passband gain (0.0 to 1.0)
    */
    void setPassbandGain (CoeffType gain) noexcept
    {
        passbandGain = jlimit (static_cast<CoeffType> (0.0), static_cast<CoeffType> (1.0), gain);
    }

    /** 
        Gets the current passband gain.
        
        @returns  The passband gain
    */
    CoeffType getPassbandGain() const noexcept
    {
        return passbandGain;
    }

    //==============================================================================
    /** 
        Gets the output from a specific stage of the ladder filter.
        
        This allows access to intermediate stages for different filter characteristics:
        - Stage 0: 1-pole lowpass (-6dB/octave)
        - Stage 1: 2-pole lowpass (-12dB/octave)
        - Stage 2: 3-pole lowpass (-18dB/octave)  
        - Stage 3: 4-pole lowpass (-24dB/octave) [default output]
        
        @param stage  The stage number (0-3)
        @returns     The output from the specified stage (requires processSample to be called first)
    */
    CoeffType getStageOutput (int stage) const noexcept
    {
        switch (stage)
        {
            case 0: return V0;
            case 1: return V1;
            case 2: return V2;
            case 3: return V3;
            default: return V3;
        }
    }

    /** 
        Processes a sample and returns outputs from all four stages.
        
        @param inputSample  The input sample
        @param outputs      Array of 4 elements to store stage outputs
        @returns           The main (4-pole) output
    */
    SampleType processMultiSample (SampleType inputSample, CoeffType outputs[4]) noexcept
    {
        const auto result = processSample (inputSample);
        outputs[0] = V0;
        outputs[1] = V1;
        outputs[2] = V2;
        outputs[3] = V3;
        return result;
    }

    //==============================================================================
    /** 
        Gets the magnitude response at the given frequency.
        
        @param frequency  The frequency in Hz
        @returns         The magnitude response (linear scale)
    */
    CoeffType getMagnitudeResponse (CoeffType frequency) const noexcept override
    {
        const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (this->sampleRate));
        const auto omega_c = DspMath::frequencyToAngular (cutoffFreq, static_cast<CoeffType> (this->sampleRate));
        
        // 4-pole Moog ladder approximation
        const auto ratio = omega / omega_c;
        const auto qFactor = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) - resonanceAmount * static_cast<CoeffType> (0.99));
        
        // 4th order lowpass with resonance
        const auto pole_real = static_cast<CoeffType> (-1.0) / qFactor;
        const auto magnitude_squared = static_cast<CoeffType> (1.0) / 
                                      (std::pow (static_cast<CoeffType> (1.0) + ratio * ratio, 2) + 
                                       std::pow (static_cast<CoeffType> (2.0) * ratio / qFactor, 2));
        
        return std::sqrt (magnitude_squared);
    }

private:
    //==============================================================================
    CoeffType cutoffFreq = static_cast<CoeffType> (1000.0);
    CoeffType resonanceAmount = static_cast<CoeffType> (0.1);
    CoeffType driveAmount = static_cast<CoeffType> (1.0);
    CoeffType passbandGain = static_cast<CoeffType> (0.5);

    // TPT coefficients
    CoeffType g = static_cast<CoeffType> (0.0);  // Warped frequency parameter
    CoeffType k = static_cast<CoeffType> (0.0);  // Feedback amount
    CoeffType outputGain = static_cast<CoeffType> (1.0);  // Output level compensation

    // State variables (node voltages and integrator states)
    CoeffType V0 = static_cast<CoeffType> (0.0);  // 1st stage output
    CoeffType V1 = static_cast<CoeffType> (0.0);  // 2nd stage output
    CoeffType V2 = static_cast<CoeffType> (0.0);  // 3rd stage output
    CoeffType V3 = static_cast<CoeffType> (0.0);  // 4th stage output (main output)
    
    CoeffType s0 = static_cast<CoeffType> (0.0);  // 1st integrator state
    CoeffType s1 = static_cast<CoeffType> (0.0);  // 2nd integrator state
    CoeffType s2 = static_cast<CoeffType> (0.0);  // 3rd integrator state
    CoeffType s3 = static_cast<CoeffType> (0.0);  // 4th integrator state

    //==============================================================================
    /** Updates the filter coefficients based on current parameters */
    void updateCoefficients() noexcept
    {
        const auto coeffs = FilterDesigner::designMoogLadder<CoeffType> (cutoffFreq, resonanceAmount, this->sampleRate);
        
        // Extract coefficients from designer
        g = coeffs[0];
        k = coeffs[1];
        outputGain = coeffs[2];
    }

    /** 
        Applies soft saturation modeling analog circuit behavior.
        
        @param input  The input value
        @returns     The saturated output
    */
    CoeffType applySaturation (CoeffType input) noexcept
    {
        // Soft saturation using tanh approximation
        // This models the non-linear behavior of analog circuits
        if (driveAmount <= static_cast<CoeffType> (1.0))
            return input;
        
        const auto x = input * static_cast<CoeffType> (2.0);
        const auto x2 = x * x;
        
        // Fast tanh approximation: tanh(x) ≈ x * (27 + x²) / (27 + 9*x²)
        return (x * (static_cast<CoeffType> (27.0) + x2)) / 
               (static_cast<CoeffType> (27.0) + static_cast<CoeffType> (9.0) * x2) * 
               static_cast<CoeffType> (0.5);
    }

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MoogLadder)
};

//==============================================================================
/** Type aliases for convenience */
using MoogLadderFloat = MoogLadder<float>;      // float samples, double coefficients (default)
using MoogLadderDouble = MoogLadder<double>;    // double samples, double coefficients (default)

} // namespace yup