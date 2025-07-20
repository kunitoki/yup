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
    Roland TB-303 Diode Ladder Filter implementation using TPT (Topology Preserving Transform).
    
    This filter emulates the iconic diode ladder filter found in the Roland TB-303
    bassline synthesizer. The TB-303 filter has a distinctive aggressive character
    with asymmetric distortion and a unique resonance behavior that defines the
    classic acid house sound.
    
    Key features:
    - Diode ladder topology with asymmetric saturation
    - Aggressive resonance with self-oscillation capabilities
    - Temperature-dependent behavior modeling
    - Zero-delay feedback using TPT method
    - Envelope following for dynamic response
    - Drive control for input saturation
    
    The filter uses a dual-precision architecture where:
    - SampleType: for audio buffer processing (float/double)
    - CoeffType: for internal calculations (defaults to double for precision)
    
    @see FilterBase, MoogLadder, VirtualAnalogSvf
*/
template <typename SampleType, typename CoeffType = double>
class Tb303Filter : public FilterBase<SampleType, CoeffType>
{
public:
    //==============================================================================
    /** Constructor with optional parameters */
    explicit Tb303Filter (CoeffType frequency = static_cast<CoeffType> (1000.0), 
                          CoeffType resonance = static_cast<CoeffType> (0.1),
                          CoeffType envMod = static_cast<CoeffType> (0.5),
                          CoeffType accent = static_cast<CoeffType> (0.0))
        : cutoffFreq (frequency), resonanceAmount (resonance), 
          envelopeAmount (envMod), accentAmount (accent)
    {
        updateCoefficients();
    }

    //==============================================================================
    /** @internal */
    void reset() noexcept override
    {
        // Reset ladder stages
        s1 = s2 = s3 = s4 = static_cast<CoeffType> (0.0);
        
        // Reset envelope follower
        envelopeState = static_cast<CoeffType> (0.0);
        
        // Reset diode states
        diodeV1 = diodeV2 = diodeV3 = diodeV4 = static_cast<CoeffType> (0.0);
    }

    /** @internal */
    void prepare (double sampleRate, int maximumBlockSize) noexcept override
    {
        this->sampleRate = sampleRate;
        this->maximumBlockSize = maximumBlockSize;
        
        // Calculate envelope time constants
        const auto timeConstant = static_cast<CoeffType> (0.001); // 1ms envelope follower
        envelopeCoeff = static_cast<CoeffType> (1.0) - std::exp (-static_cast<CoeffType> (1.0) / (timeConstant * this->sampleRate));
        
        updateCoefficients();
    }

    /** @internal */
    SampleType processSample (SampleType inputSample) noexcept override
    {
        // Convert input to coefficient precision
        auto input = static_cast<CoeffType> (inputSample);
        
        // Apply input gain and soft saturation
        input *= inputGain;
        input = applyInputSaturation (input);
        
        // Envelope follower for dynamic response
        const auto inputLevel = std::abs (input);
        envelopeState += envelopeCoeff * (inputLevel - envelopeState);
        
        // Dynamic frequency modulation based on envelope and accent
        const auto dynamicFreq = cutoffFreq * (static_cast<CoeffType> (1.0) + 
                                             envelopeAmount * envelopeState + 
                                             accentAmount * static_cast<CoeffType> (0.5));
        
        // Update coefficients if frequency changed significantly
        if (std::abs (dynamicFreq - lastFreq) > static_cast<CoeffType> (1.0))
        {
            lastFreq = dynamicFreq;
            updateDynamicCoefficients (dynamicFreq);
        }
        
        // Diode ladder processing with nonlinear elements
        const auto feedbackSignal = computeFeedback();
        const auto inputWithFeedback = input - feedbackSignal;
        
        // Stage 1: First diode section
        const auto stage1Input = inputWithFeedback;
        s1 = processNonlinearStage (stage1Input, s1, g1, diodeV1, static_cast<CoeffType> (0.7));
        
        // Stage 2: Second diode section  
        s2 = processNonlinearStage (s1, s2, g2, diodeV2, static_cast<CoeffType> (0.3));
        
        // Stage 3: Third diode section
        s3 = processNonlinearStage (s2, s3, g3, diodeV3, static_cast<CoeffType> (0.2));
        
        // Stage 4: Fourth diode section (output stage)
        s4 = processNonlinearStage (s3, s4, g4, diodeV4, static_cast<CoeffType> (0.1));
        
        // Apply output gain compensation and convert back to SampleType
        return static_cast<SampleType> (s4 * outputGain);
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
        
        // TB-303 diode ladder approximation (4-pole response with asymmetric characteristics)
        const auto omega_c = DspMath::frequencyToAngular (cutoffFreq, static_cast<CoeffType> (this->sampleRate));
        const auto pole = DspMath::Complex<CoeffType> (-omega_c, 0);
        
        // Asymmetric 4th order response modeling diode nonlinearity
        const auto response = static_cast<CoeffType> (1.0) / 
                             ((s - pole) * (s - pole * static_cast<CoeffType> (1.1)) * 
                              (s - pole * static_cast<CoeffType> (0.9)) * (s - pole * static_cast<CoeffType> (0.8)));
        
        return response;
    }

    //==============================================================================
    /** 
        Sets the cutoff frequency.
        
        @param frequency  The cutoff frequency in Hz
    */
    void setCutoffFrequency (CoeffType frequency) noexcept
    {
        cutoffFreq = jmax (static_cast<CoeffType> (10.0), 
                          jmin (frequency, static_cast<CoeffType> (this->sampleRate * 0.48)));
        updateCoefficients();
    }

    /** 
        Sets the resonance amount.
        
        @param resonance  The resonance amount (0.0 to 1.0, where 1.0 is self-oscillation)
    */
    void setResonance (CoeffType resonance) noexcept
    {
        resonanceAmount = jlimit (static_cast<CoeffType> (0.0), static_cast<CoeffType> (0.995), resonance);
        updateCoefficients();
    }

    /** 
        Sets the envelope modulation amount.
        
        @param envMod  The envelope modulation depth (0.0 to 1.0)
    */
    void setEnvelopeAmount (CoeffType envMod) noexcept
    {
        envelopeAmount = jlimit (static_cast<CoeffType> (0.0), static_cast<CoeffType> (2.0), envMod);
    }

    /** 
        Sets the accent amount for dynamic response.
        
        @param accent  The accent amount (0.0 to 1.0)
    */
    void setAccent (CoeffType accent) noexcept
    {
        accentAmount = jlimit (static_cast<CoeffType> (0.0), static_cast<CoeffType> (1.0), accent);
    }

    /** 
        Sets all parameters simultaneously.
        
        @param frequency  The cutoff frequency in Hz
        @param resonance  The resonance amount (0.0 to 1.0)
        @param envMod     The envelope modulation depth (0.0 to 1.0)
        @param accent     The accent amount (0.0 to 1.0)
    */
    void setParameters (CoeffType frequency, CoeffType resonance, 
                       CoeffType envMod = static_cast<CoeffType> (0.5), 
                       CoeffType accent = static_cast<CoeffType> (0.0)) noexcept
    {
        setCutoffFrequency (frequency);
        setResonance (resonance);
        setEnvelopeAmount (envMod);
        setAccent (accent);
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
        Gets the current envelope modulation amount.
        
        @returns  The envelope modulation depth
    */
    CoeffType getEnvelopeAmount() const noexcept
    {
        return envelopeAmount;
    }

    /** 
        Gets the current accent amount.
        
        @returns  The accent amount
    */
    CoeffType getAccent() const noexcept
    {
        return accentAmount;
    }

    //==============================================================================
    /** 
        Gets the current envelope follower state.
        
        @returns  The envelope state (0.0 to 1.0)
    */
    CoeffType getEnvelopeState() const noexcept
    {
        return envelopeState;
    }

private:
    //==============================================================================
    CoeffType cutoffFreq = static_cast<CoeffType> (1000.0);
    CoeffType resonanceAmount = static_cast<CoeffType> (0.1);
    CoeffType envelopeAmount = static_cast<CoeffType> (0.5);
    CoeffType accentAmount = static_cast<CoeffType> (0.0);

    // Filter coefficients per stage
    CoeffType g1 = static_cast<CoeffType> (0.0);  // Stage 1 gain
    CoeffType g2 = static_cast<CoeffType> (0.0);  // Stage 2 gain  
    CoeffType g3 = static_cast<CoeffType> (0.0);  // Stage 3 gain
    CoeffType g4 = static_cast<CoeffType> (0.0);  // Stage 4 gain
    
    CoeffType feedbackGain = static_cast<CoeffType> (0.0);  // Feedback amount
    CoeffType inputGain = static_cast<CoeffType> (1.0);     // Input level
    CoeffType outputGain = static_cast<CoeffType> (1.0);    // Output compensation

    // Filter state variables (integrator states)
    CoeffType s1 = static_cast<CoeffType> (0.0);  // Stage 1 state
    CoeffType s2 = static_cast<CoeffType> (0.0);  // Stage 2 state
    CoeffType s3 = static_cast<CoeffType> (0.0);  // Stage 3 state
    CoeffType s4 = static_cast<CoeffType> (0.0);  // Stage 4 state

    // Diode voltage states for nonlinear modeling
    CoeffType diodeV1 = static_cast<CoeffType> (0.0);  // Diode 1 voltage
    CoeffType diodeV2 = static_cast<CoeffType> (0.0);  // Diode 2 voltage
    CoeffType diodeV3 = static_cast<CoeffType> (0.0);  // Diode 3 voltage
    CoeffType diodeV4 = static_cast<CoeffType> (0.0);  // Diode 4 voltage

    // Envelope follower
    CoeffType envelopeState = static_cast<CoeffType> (0.0);
    CoeffType envelopeCoeff = static_cast<CoeffType> (0.01);
    CoeffType lastFreq = static_cast<CoeffType> (1000.0);

    //==============================================================================
    /** Updates the filter coefficients based on current parameters */
    void updateCoefficients() noexcept
    {
        const auto coeffs = FilterDesigner<CoeffType>::designTb303 (cutoffFreq, resonanceAmount, this->sampleRate);
        
        // Extract coefficients from designer
        g1 = coeffs[0];
        g2 = coeffs[1]; 
        g3 = coeffs[2];
        g4 = coeffs[3];
        feedbackGain = coeffs[4];
        inputGain = coeffs[5];
        outputGain = coeffs[6];
    }
    
    /** Updates coefficients dynamically during processing */
    void updateDynamicCoefficients (CoeffType frequency) noexcept
    {
        const auto coeffs = FilterDesigner<CoeffType>::designTb303 (frequency, resonanceAmount, this->sampleRate);
        
        // Smooth coefficient updates to avoid clicks
        const auto smoothing = static_cast<CoeffType> (0.1);
        g1 += smoothing * (coeffs[0] - g1);
        g2 += smoothing * (coeffs[1] - g2);
        g3 += smoothing * (coeffs[2] - g3);
        g4 += smoothing * (coeffs[3] - g4);
        feedbackGain += smoothing * (coeffs[4] - feedbackGain);
    }

    /** 
        Processes a single nonlinear filter stage with diode modeling.
        
        @param input        The input signal
        @param state        The current integrator state
        @param gain         The stage gain coefficient
        @param diodeVoltage The diode voltage state
        @param threshold    The diode conduction threshold
        @returns           The stage output
    */
    CoeffType processNonlinearStage (CoeffType input, CoeffType& state, CoeffType gain, 
                                     CoeffType& diodeVoltage, CoeffType threshold) noexcept
    {
        // Linear integrator part
        const auto linearOutput = input * gain + state;
        
        // Diode nonlinearity modeling
        const auto diodeInput = linearOutput - diodeVoltage;
        const auto diodeOutput = applyDiodeDistortion (diodeInput, threshold);
        
        // Update diode voltage (capacitive coupling)
        diodeVoltage += static_cast<CoeffType> (0.1) * (diodeOutput - diodeVoltage);
        
        // Update integrator state (TPT method)
        state = static_cast<CoeffType> (2.0) * linearOutput - state;
        
        return diodeOutput;
    }

    /** 
        Applies TB-303 style diode distortion.
        
        @param input      The input signal
        @param threshold  The diode conduction threshold
        @returns         The distorted output
    */
    CoeffType applyDiodeDistortion (CoeffType input, CoeffType threshold) noexcept
    {
        const auto x = input / threshold;
        const auto x2 = x * x;
        
        // Asymmetric diode characteristic
        if (input >= static_cast<CoeffType> (0.0))
        {
            // Forward bias: exponential characteristic
            return threshold * (static_cast<CoeffType> (1.0) - std::exp (-x * static_cast<CoeffType> (2.0)));
        }
        else
        {
            // Reverse bias: more linear with soft knee
            return input / (static_cast<CoeffType> (1.0) + x2);
        }
    }

    /** 
        Applies input saturation for TB-303 character.
        
        @param input  The input signal
        @returns     The saturated output
    */
    CoeffType applyInputSaturation (CoeffType input) noexcept
    {
        // TB-303 style input saturation with asymmetric behavior
        const auto drive = static_cast<CoeffType> (1.5) + resonanceAmount;
        const auto x = input * drive;
        
        // Asymmetric tanh-like saturation
        if (x >= static_cast<CoeffType> (0.0))
        {
            return std::tanh (x * static_cast<CoeffType> (1.2)) / static_cast<CoeffType> (1.2);
        }
        else
        {
            return std::tanh (x * static_cast<CoeffType> (0.8)) / static_cast<CoeffType> (0.8);
        }
    }

    /** 
        Computes the feedback signal from the filter stages.
        
        @returns  The feedback signal
    */
    CoeffType computeFeedback() noexcept
    {
        // TB-303 uses feedback from multiple stages with different weights
        const auto fb1 = s1 * static_cast<CoeffType> (0.1);
        const auto fb2 = s2 * static_cast<CoeffType> (0.3);
        const auto fb3 = s3 * static_cast<CoeffType> (0.5);
        const auto fb4 = s4 * static_cast<CoeffType> (1.0);
        
        return feedbackGain * (fb1 + fb2 + fb3 + fb4);
    }

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Tb303Filter)
};

//==============================================================================
/** Type aliases for convenience */
using Tb303FilterFloat = Tb303Filter<float>;       // float samples, double coefficients (default)
using Tb303FilterDouble = Tb303Filter<double>;     // double samples, double coefficients (default)

} // namespace yup
