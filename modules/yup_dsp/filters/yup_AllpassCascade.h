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
    Variable IIR Allpass cascade filter for halfband applications.
    
    This filter implements a halfband IIR filter with two branches of cascaded
    first-order allpass sections. It's designed for efficient halfband filtering
    with configurable allpass characteristics (Butterworth or Elliptic).
    
    Key characteristics:
    - Two-branch architecture with independent allpass cascades
    - Configurable delay compensation between branches
    - Support for both lowpass and highpass outputs
    - Elliptic or Butterworth coefficient generation
    - Optimal for halfband filter applications
    - Real-time coefficient updates
    
    Mathematical Foundation:
    The filter processes input through two parallel allpass cascades (A0 and A1),
    then combines the outputs with optional delay:
    - Lowpass output: (A0 + delayed_A1) / 2
    - Highpass output: (A0 - delayed_A1) / 2
    
    Features:
    - Variable number of stages (1 to 20)
    - Configurable delay between branches (1 to 8 samples)
    - Both elliptic and Butterworth coefficient modes
    - Simultaneous lowpass and highpass outputs
    - Efficient circular buffer delay implementation
    - Real-time parameter updates
    
    Applications:
    - Halfband filter design
    - Multirate signal processing
    - Perfect reconstruction filter banks
    - Quadrature mirror filters
    - Polyphase decomposition
    - Audio sample rate conversion
    
    The filter uses a dual-precision architecture where:
    - SampleType: for audio buffer processing (float/double)
    - CoeffType: for internal calculations (defaults to double for precision)
    
    @see FirstOrderAllpass, ButterworthAllpass, EllipticAllpass
*/
template <typename SampleType, typename CoeffType = double>
class AllpassCascade : public FilterBase<SampleType, CoeffType>
{
public:
    //==============================================================================
    /** Allpass cascade design type */
    enum class DesignType
    {
        butterworth,  /**< Butterworth allpass design */
        elliptic      /**< Elliptic allpass design */
    };

    /** Filter output structure containing both lowpass and highpass outputs */
    struct CascadeOutputs
    {
        SampleType lowpass = 0;   /**< Lowpass output */
        SampleType highpass = 0;  /**< Highpass output */
    };

    //==============================================================================
    /** Constructor with parameters */
    explicit AllpassCascade (DesignType design = DesignType::elliptic,
                            CoeffType passbandFreq = static_cast<CoeffType> (0.4),
                            int stages = 4,
                            int delaySamples = 2)
        : designType (design), fp (passbandFreq), numStages (stages), delayLength (delaySamples)
    {
        setParameters (design, passbandFreq, stages, delaySamples);
    }

    //==============================================================================
    /** @internal */
    void reset() noexcept override
    {
        // Reset all allpass sections
        for (auto& section : a0Cascade)
            section.reset();
        for (auto& section : a1Cascade)
            section.reset();
        
        // Reset delay buffer
        std::fill (delayBuffer.begin(), delayBuffer.end(), static_cast<CoeffType> (0.0));
        delayIndex = 0;
    }

    /** @internal */
    void prepare (double sampleRate, int maximumBlockSize) noexcept override
    {
        this->sampleRate = sampleRate;
        this->maximumBlockSize = maximumBlockSize;
        
        // Prepare all allpass sections
        for (auto& section : a0Cascade)
            section.prepare (sampleRate, maximumBlockSize);
        for (auto& section : a1Cascade)
            section.prepare (sampleRate, maximumBlockSize);
    }

    /** @internal */
    SampleType processSample (SampleType inputSample) noexcept override
    {
        const auto outputs = processMultiSample (inputSample);
        return outputs.lowpass;  // Default to lowpass output
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
        // Combined response of both branches with delay
        auto response0 = DspMath::Complex<CoeffType> (static_cast<CoeffType> (1.0), static_cast<CoeffType> (0.0));
        for (const auto& section : a0Cascade)
            response0 *= section.getComplexResponse (frequency);
            
        auto response1 = DspMath::Complex<CoeffType> (static_cast<CoeffType> (1.0), static_cast<CoeffType> (0.0));
        for (const auto& section : a1Cascade)
            response1 *= section.getComplexResponse (frequency);
            
        // Apply delay to second branch
        const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (this->sampleRate));
        const auto delayResponse = DspMath::polar (static_cast<CoeffType> (1.0), -omega * static_cast<CoeffType> (delayLength / 2));
        response1 *= delayResponse;
        
        // Return lowpass response (sum of branches)
        return (response0 + response1) / static_cast<CoeffType> (2.0);
    }

    //==============================================================================
    /** 
        Sets all filter parameters and recalculates coefficients.
        
        @param design         The allpass design type
        @param passbandFreq   The passband frequency parameter (0.005 to 0.495)
        @param stages         The number of allpass stages (1 to 20)
        @param delaySamples   The delay between branches (1 to 8)
    */
    void setParameters (DesignType design, CoeffType passbandFreq, int stages, int delaySamples) noexcept
    {
        designType = design;
        fp = jlimit (static_cast<CoeffType> (0.005), static_cast<CoeffType> (0.495), passbandFreq);
        numStages = jlimit (1, 20, stages);
        
        const auto newDelay = jlimit (1, 8, delaySamples);
        if (newDelay != delayLength)
        {
            delayLength = newDelay;
            delayBuffer.resize (static_cast<size_t> (delayLength / 2));
            std::fill (delayBuffer.begin(), delayBuffer.end(), static_cast<CoeffType> (0.0));
            delayIndex = 0;
        }
        
        updateCoefficients();
    }

    /** 
        Sets just the passband frequency parameter.
        
        @param passbandFreq  The passband frequency parameter (0.005 to 0.495)
    */
    void setPassbandFrequency (CoeffType passbandFreq) noexcept
    {
        fp = jlimit (static_cast<CoeffType> (0.005), static_cast<CoeffType> (0.495), passbandFreq);
        updateCoefficients();
    }

    /** 
        Sets just the number of stages.
        
        @param stages  The number of allpass stages (1 to 20)
    */
    void setStages (int stages) noexcept
    {
        numStages = jlimit (1, 20, stages);
        updateCoefficients();
    }

    /** 
        Sets just the design type.
        
        @param design  The allpass design type
    */
    void setDesignType (DesignType design) noexcept
    {
        designType = design;
        updateCoefficients();
    }

    //==============================================================================
    /** 
        Processes a sample and returns both lowpass and highpass outputs.
        
        @param inputSample  The input sample
        @returns           Structure containing both outputs
    */
    CascadeOutputs processMultiSample (SampleType inputSample) noexcept
    {
        // Convert input to coefficient precision
        const auto input = static_cast<CoeffType> (inputSample);
        
        // Process through A0 cascade (upper branch)
        auto out0 = input;
        for (auto& section : a0Cascade)
            out0 = static_cast<CoeffType> (section.processSample (static_cast<SampleType> (out0)));
        
        // Process through A1 cascade (lower branch)
        auto out1 = input;
        for (auto& section : a1Cascade)
            out1 = static_cast<CoeffType> (section.processSample (static_cast<SampleType> (out1)));
        
        // Apply delay to A1 output
        delayBuffer[delayIndex] = out1;
        const auto delayedOut1 = delayBuffer[(delayIndex + 1) % delayBuffer.size()];
        delayIndex = (delayIndex + 1) % delayBuffer.size();
        
        // Generate outputs
        CascadeOutputs outputs;
        outputs.lowpass = static_cast<SampleType> ((out0 + delayedOut1) / static_cast<CoeffType> (2.0));
        outputs.highpass = static_cast<SampleType> ((out0 - delayedOut1) / static_cast<CoeffType> (2.0));
        
        return outputs;
    }

    /** 
        Gets just the highpass output from the last processed sample.
        
        @returns  The highpass output
    */
    SampleType getHighpassOutput (SampleType inputSample) noexcept
    {
        const auto outputs = processMultiSample (inputSample);
        return outputs.highpass;
    }

    //==============================================================================
    /** 
        Gets the current design type.
        
        @returns  The design type
    */
    DesignType getDesignType() const noexcept
    {
        return designType;
    }

    /** 
        Gets the current passband frequency parameter.
        
        @returns  The passband frequency parameter
    */
    CoeffType getPassbandFrequency() const noexcept
    {
        return fp;
    }

    /** 
        Gets the current number of stages.
        
        @returns  The number of stages
    */
    int getStages() const noexcept
    {
        return numStages;
    }

    /** 
        Gets the current delay length.
        
        @returns  The delay length
    */
    int getDelayLength() const noexcept
    {
        return delayLength;
    }

    /** 
        Gets the number of A0 cascade sections.
        
        @returns  Number of A0 sections
    */
    int getNumA0Sections() const noexcept
    {
        return static_cast<int> (a0Cascade.size());
    }

    /** 
        Gets the number of A1 cascade sections.
        
        @returns  Number of A1 sections
    */
    int getNumA1Sections() const noexcept
    {
        return static_cast<int> (a1Cascade.size());
    }

private:
    //==============================================================================
    void updateCoefficients() noexcept
    {
        // Clear existing cascades
        a0Cascade.clear();
        a1Cascade.clear();
        
        if (numStages <= 0)
            return;
        
        // Generate coefficients based on design type
        std::vector<CoeffType> a0Coeffs, a1Coeffs;
        
        if (designType == DesignType::butterworth)
        {
            generateButterworthCoefficients (a0Coeffs, a1Coeffs);
        }
        else
        {
            generateEllipticCoefficients (a0Coeffs, a1Coeffs);
        }
        
        // Calculate section distribution
        const int j = (numStages + 1) / 2;  // Number of A0 sections
        const int k = numStages - j;        // Number of A1 sections
        
        // Create A0 cascade sections
        for (int i = 0; i < j && i < static_cast<int> (a0Coeffs.size()); ++i)
        {
            a0Cascade.emplace_back (a0Coeffs[i], 1);
        }
        
        // Create A1 cascade sections
        for (int i = 0; i < k && i < static_cast<int> (a1Coeffs.size()); ++i)
        {
            a1Cascade.emplace_back (a1Coeffs[i], 1);
        }
        
        // Prepare sections if we're already prepared
        if (this->sampleRate > 0.0)
        {
            for (auto& section : a0Cascade)
                section.prepare (this->sampleRate, this->maximumBlockSize);
            for (auto& section : a1Cascade)
                section.prepare (this->sampleRate, this->maximumBlockSize);
        }
    }

    void generateButterworthCoefficients (std::vector<CoeffType>& a0Coeffs, std::vector<CoeffType>& a1Coeffs) noexcept
    {
        // Butterworth allpass coefficient generation (from spuce butterworth_allpass)
        const int N = 2 * numStages + 1;
        const int J = numStages / 2;
        
        a0Coeffs.clear();
        a1Coeffs.clear();
        
        // Generate a1 coefficients
        for (int l = 1; l <= J; ++l)
        {
            const auto d = std::tan (MathConstants<CoeffType>::pi * static_cast<CoeffType> (l) / static_cast<CoeffType> (N));
            a1Coeffs.push_back (d * d);
        }
        
        // Generate a0 coefficients
        for (int l = J + 1; l <= numStages; ++l)
        {
            const auto d = static_cast<CoeffType> (1.0) / std::tan (MathConstants<CoeffType>::pi * static_cast<CoeffType> (l) / static_cast<CoeffType> (N));
            a0Coeffs.push_back (d * d);
        }
    }

    void generateEllipticCoefficients (std::vector<CoeffType>& a0Coeffs, std::vector<CoeffType>& a1Coeffs) noexcept
    {
        // Simplified elliptic allpass coefficient generation
        const int N = 2 * numStages + 1;
        const auto k = static_cast<CoeffType> (2.0) * fp;
        const auto zeta = static_cast<CoeffType> (1.0) / k;
        const auto zeta2 = zeta * zeta;
        
        a0Coeffs.clear();
        a1Coeffs.clear();
        
        const bool odd = (numStages % 2) != 0;
        
        // Generate coefficients for each stage
        for (int l = 1; l <= numStages; ++l)
        {
            // Simplified elliptic coefficient calculation
            const auto angle = MathConstants<CoeffType>::pi * static_cast<CoeffType> (l) / static_cast<CoeffType> (N);
            const auto sn_approx = std::sin (angle);
            const auto sn2 = sn_approx * sn_approx;
            
            const auto lambda = static_cast<CoeffType> (1.0);
            const auto sqrt_term = std::sqrt ((static_cast<CoeffType> (1.0) - sn2) * (zeta2 - sn2));
            const auto numerator = zeta + sn2 - lambda * sqrt_term;
            const auto denominator = zeta + sn2 + lambda * sqrt_term;
            
            auto beta = numerator / jmax (denominator, static_cast<CoeffType> (1e-12));
            beta = jlimit (static_cast<CoeffType> (-0.99), static_cast<CoeffType> (0.99), beta);
            
            // Distribute coefficients between branches
            if ((l % 2) != (odd ? 1 : 0))
            {
                a1Coeffs.push_back (beta);
            }
            else
            {
                a0Coeffs.push_back (beta);
            }
        }
    }

    //==============================================================================
    DesignType designType = DesignType::elliptic;
    CoeffType fp = static_cast<CoeffType> (0.4);
    int numStages = 4;
    int delayLength = 2;
    
    // Allpass cascades
    std::vector<FirstOrderAllpass<SampleType, CoeffType>> a0Cascade;  // Upper branch
    std::vector<FirstOrderAllpass<SampleType, CoeffType>> a1Cascade;  // Lower branch
    
    // Delay buffer for branch compensation
    std::vector<CoeffType> delayBuffer;
    int delayIndex = 0;

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AllpassCascade)
};

//==============================================================================
/** Type aliases for convenience */
using AllpassCascadeFloat = AllpassCascade<float>;        // float samples, double coefficients (default)
using AllpassCascadeDouble = AllpassCascade<double>;      // double samples, double coefficients (default)

} // namespace yup