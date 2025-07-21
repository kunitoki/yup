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
    SOAP (Second Order All Pass) filter implementation.
    
    This filter can simultaneously provide bandpass and bandreject outputs
    from the same input signal. It's based on Tom Erbe's design and is particularly
    useful for creating spectral effects and frequency domain manipulations.
    
    The filter implements a second-order allpass structure that inherently
    provides both bandpass and bandreject characteristics, making it efficient
    for dual-output filtering applications.
    
    Features:
    - Simultaneous bandpass and bandreject outputs
    - Adjustable center frequency and bandwidth
    - Phase relationships useful for spatial effects
    - Low computational overhead
    
    Applications:
    - Spectral filtering effects
    - Frequency domain splitting
    - Phase manipulation for stereo widening
    - Educational filter design demonstrations
    
    @see FilterBase, AllpassFilter
*/
template <typename SampleType, typename CoeffType = double>
class SoapFilter : public FilterBase<SampleType, CoeffType>
{
public:
    //==============================================================================
    /** Default constructor */
    SoapFilter() noexcept
    {
        setCenterFrequency (static_cast<CoeffType> (1000.0));
        setBandwidth (static_cast<CoeffType> (100.0));
    }

    //==============================================================================
    /** @internal */
    void reset() noexcept override
    {
        input0 = input1 = SampleType (0);
        output0 = output1 = SampleType (0);
        bandpassOutput = bandrejectOutput = SampleType (0);
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
        // Process through the allpass structure
        const auto output = a0 * inputSample + a1 * input0 + a2 * input1 - b1 * output0 - b2 * output1;
        
        // Update delay line
        input1 = input0;
        input0 = inputSample;
        output1 = output0;
        output0 = output;
        
        // Calculate bandpass and bandreject outputs
        bandpassOutput = static_cast<SampleType> (inputSample - output);
        bandrejectOutput = static_cast<SampleType> (inputSample + output);
        
        // Return the allpass output by default
        return static_cast<SampleType> (output);
    }

    /** @internal */
    void processBlock (const SampleType* inputBuffer, SampleType* outputBuffer, int numSamples) noexcept override
    {
        jassert (inputBuffer != nullptr && outputBuffer != nullptr);
        
        for (int i = 0; i < numSamples; ++i)
            outputBuffer[i] = processSample (inputBuffer[i]);
    }

    /** @internal */
    DspMath::Complex<CoeffType> getComplexResponse (CoeffType frequency) const noexcept override
    {
        const auto omega = DspMath::frequencyToAngular (frequency, this->sampleRate);
        const auto z = DspMath::polar (CoeffType (1), omega);
        const auto z2 = z * z;
        
        const auto numerator = a0 + a1 * z + a2 * z2;
        const auto denominator = CoeffType (1) + b1 * z + b2 * z2;
        
        return numerator / denominator;
    }

    //==============================================================================
    /** 
        Sets the center frequency of the filter.
        
        @param frequency  The center frequency in Hz
    */
    void setCenterFrequency (CoeffType frequency) noexcept
    {
        centerFreq = frequency;
        updateCoefficients();
    }

    /** 
        Sets the bandwidth of the filter.
        
        @param bandwidth  The bandwidth in Hz
    */
    void setBandwidth (CoeffType bandwidth) noexcept
    {
        filterBandwidth = bandwidth;
        updateCoefficients();
    }

    /** 
        Sets both center frequency and bandwidth.
        
        @param frequency  The center frequency in Hz
        @param bandwidth  The bandwidth in Hz
    */
    void setParameters (CoeffType frequency, CoeffType bandwidth) noexcept
    {
        centerFreq = frequency;
        filterBandwidth = bandwidth;
        updateCoefficients();
    }

    //==============================================================================
    /** 
        Gets the center frequency.
        
        @returns  The center frequency in Hz
    */
    CoeffType getCenterFrequency() const noexcept
    {
        return centerFreq;
    }

    /** 
        Gets the bandwidth.
        
        @returns  The bandwidth in Hz
    */
    CoeffType getBandwidth() const noexcept
    {
        return filterBandwidth;
    }

    //==============================================================================
    /** 
        Gets the bandpass output from the last processed sample.
        
        @returns  The bandpass filtered output
    */
    SampleType getBandpassOutput() const noexcept
    {
        return bandpassOutput;
    }

    /** 
        Gets the bandreject output from the last processed sample.
        
        @returns  The bandreject filtered output
    */
    SampleType getBandrejectOutput() const noexcept
    {
        return bandrejectOutput;
    }

    /** 
        Processes a sample and returns both outputs via references.
        
        @param inputSample      The input sample
        @param bandpassOut      Reference to store the bandpass output
        @param bandrejectOut    Reference to store the bandreject output
        @returns               The allpass output
    */
    SampleType processSample (SampleType inputSample, SampleType& bandpassOut, SampleType& bandrejectOut) noexcept
    {
        const auto allpassOut = processSample (inputSample);
        bandpassOut = bandpassOutput;
        bandrejectOut = bandrejectOutput;
        return allpassOut;
    }

private:
    //==============================================================================
    void updateCoefficients() noexcept
    {
        if (this->sampleRate <= 0.0)
            return;
        
        // Calculate normalized frequencies
        const auto nyquist = static_cast<CoeffType> (this->sampleRate * 0.5);
        const auto normalizedCenter = centerFreq / nyquist;
        const auto normalizedBandwidth = filterBandwidth / nyquist;
        
        // Prevent degenerate cases
        const auto clampedCenter = jlimit (static_cast<CoeffType> (0.001), 
                                          static_cast<CoeffType> (0.999), 
                                          normalizedCenter);
        const auto clampedBandwidth = jlimit (static_cast<CoeffType> (0.001), 
                                             static_cast<CoeffType> (1.0), 
                                             normalizedBandwidth);
        
        // Calculate Q factor from bandwidth
        const auto q = clampedCenter / clampedBandwidth;
        
        // Calculate angular frequency
        const auto omega = MathConstants<CoeffType>::twoPi * clampedCenter;
        const auto cosOmega = std::cos (omega);
        const auto sinOmega = std::sin (omega);
        const auto alpha = sinOmega / (CoeffType (2) * q);
        
        // Calculate allpass coefficients
        const auto norm = CoeffType (1) / (CoeffType (1) + alpha);
        
        a0 = (CoeffType (1) - alpha) * norm;
        a1 = -CoeffType (2) * cosOmega * norm;
        a2 = (CoeffType (1) + alpha) * norm;
        b1 = a1; // For allpass: b1 = a1
        b2 = a0; // For allpass: b2 = a0, b0 = a2 (but b0 = 1 after normalization)
    }

    //==============================================================================
    CoeffType centerFreq = static_cast<CoeffType> (1000.0);
    CoeffType filterBandwidth = static_cast<CoeffType> (100.0);
    
    // Filter coefficients
    CoeffType a0 = CoeffType (1), a1 = CoeffType (0), a2 = CoeffType (0);
    CoeffType b1 = CoeffType (0), b2 = CoeffType (0);
    
    // State variables
    SampleType input0 = SampleType (0), input1 = SampleType (0);
    SampleType output0 = SampleType (0), output1 = SampleType (0);
    
    // Output storage
    SampleType bandpassOutput = SampleType (0);
    SampleType bandrejectOutput = SampleType (0);

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoapFilter)
};

//==============================================================================
/** Type aliases for convenience */
using SoapFilterFloat = SoapFilter<float>;
using SoapFilterDouble = SoapFilter<double>;

} // namespace yup