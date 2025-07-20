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

#include <array>
#include <memory>

namespace yup
{

//==============================================================================
/** 
    Linkwitz-Riley crossover filter implementation.
    
    Linkwitz-Riley filters are specialized crossover filters that provide
    perfect reconstruction when the lowpass and highpass outputs are summed.
    They are commonly used in speaker systems for frequency band separation.
    
    Key characteristics:
    - Even-order only (2nd, 4th, 6th, 8th order supported)
    - -6dB at crossover frequency for both LP and HP
    - Perfect magnitude and phase reconstruction when summed
    - Based on cascaded Butterworth sections
    
    The filter provides both lowpass and highpass outputs simultaneously,
    making it ideal for crossover applications.
    
    @see ButterworthFilter, BiquadCascade
*/
template <typename SampleType, typename CoeffType = double>
class LinkwitzRileyFilter : public FilterBase<SampleType, CoeffType>
{
public:
    //==============================================================================
    /** Filter output structure containing both lowpass and highpass */
    struct CrossoverOutputs
    {
        SampleType lowpass = 0;   /**< Lowpass output */
        SampleType highpass = 0;  /**< Highpass output */
    };

    //==============================================================================
    /** Constructor with optional parameters */
    explicit LinkwitzRileyFilter (int order = 4, SampleType frequency = static_cast<SampleType> (1000.0), double sampleRate = 44100.0)
        : lowpassFilter (FilterType::lowpass, order, frequency, sampleRate),
          highpassFilter (FilterType::highpass, order, frequency, sampleRate)
    {
        setParameters (order, frequency, sampleRate);
    }

    //==============================================================================
    /** @internal */
    void reset() noexcept override
    {
        lowpassFilter.reset();
        highpassFilter.reset();
    }

    /** @internal */
    void prepare (double sampleRate, int maximumBlockSize) noexcept override
    {
        this->sampleRate = sampleRate;
        this->maximumBlockSize = maximumBlockSize;
        
        lowpassFilter.prepare (sampleRate, maximumBlockSize);
        highpassFilter.prepare (sampleRate, maximumBlockSize);
    }

    /** @internal */
    SampleType processSample (SampleType inputSample) noexcept override
    {
        // Default returns lowpass output
        return lowpassFilter.processSample (inputSample);
    }

    /** @internal */
    void processBlock (const SampleType* inputBuffer, SampleType* outputBuffer, int numSamples) noexcept override
    {
        // Default processes lowpass output
        lowpassFilter.processBlock (inputBuffer, outputBuffer, numSamples);
    }

    /** @internal */
    DspMath::Complex<CoeffType> getComplexResponse (CoeffType frequency) const noexcept override
    {
        // Returns lowpass response by default
        return lowpassFilter.getComplexResponse (frequency);
    }

    //==============================================================================
    /** 
        Sets the crossover parameters.
        
        @param order       The filter order (must be even: 2, 4, 6, 8)
        @param frequency   The crossover frequency in Hz
        @param sampleRate  The sample rate in Hz
    */
    void setParameters (int order, SampleType frequency, double sampleRate) noexcept
    {
        // Ensure even order for Linkwitz-Riley
        filterOrder = jlimit (2, 8, (order + 1) & ~1); // Round to nearest even number
        crossoverFreq = frequency;
        this->sampleRate = sampleRate;

        lowpassFilter.setParameters (FilterType::lowpass, filterOrder, frequency, sampleRate);
        highpassFilter.setParameters (FilterType::highpass, filterOrder, frequency, sampleRate);
    }

    /** 
        Sets just the crossover frequency.
        
        @param frequency  The new crossover frequency in Hz
    */
    void setCrossoverFrequency (SampleType frequency) noexcept
    {
        crossoverFreq = frequency;
        lowpassFilter.setCutoffFrequency (frequency);
        highpassFilter.setCutoffFrequency (frequency);
    }

    /** 
        Sets just the filter order.
        
        @param order  The new filter order (will be rounded to nearest even number)
    */
    void setOrder (int order) noexcept
    {
        const auto newOrder = jlimit (2, 8, (order + 1) & ~1);
        if (filterOrder != newOrder)
        {
            filterOrder = newOrder;
            lowpassFilter.setOrder (filterOrder);
            highpassFilter.setOrder (filterOrder);
        }
    }

    /** 
        Gets the current crossover frequency.
        
        @returns  The crossover frequency in Hz
    */
    SampleType getCrossoverFrequency() const noexcept
    {
        return crossoverFreq;
    }

    /** 
        Gets the current filter order.
        
        @returns  The filter order
    */
    int getOrder() const noexcept
    {
        return filterOrder;
    }

    //==============================================================================
    /** 
        Processes a sample and returns both outputs.
        
        @param inputSample  The input sample
        @returns           Structure containing both lowpass and highpass outputs
    */
    CrossoverOutputs processCrossoverSample (SampleType inputSample) noexcept
    {
        CrossoverOutputs outputs;
        outputs.lowpass = lowpassFilter.processSample (inputSample);
        outputs.highpass = highpassFilter.processSample (inputSample);
        return outputs;
    }

    /** 
        Processes a block with separate outputs for lowpass and highpass.
        
        @param inputBuffer     The input buffer
        @param lowpassBuffer   Buffer for lowpass output (can be nullptr)
        @param highpassBuffer  Buffer for highpass output (can be nullptr)
        @param numSamples      Number of samples to process
    */
    void processCrossoverBlock (const SampleType* inputBuffer,
                                SampleType* lowpassBuffer,
                                SampleType* highpassBuffer,
                                int numSamples) noexcept
    {
        if (lowpassBuffer)
            lowpassFilter.processBlock (inputBuffer, lowpassBuffer, numSamples);
        
        if (highpassBuffer)
            highpassFilter.processBlock (inputBuffer, highpassBuffer, numSamples);
    }

    /** 
        Gets the lowpass frequency response.
        
        @param frequency  The frequency in Hz
        @returns         The complex frequency response of the lowpass section
    */
    DspMath::Complex<CoeffType> getLowpassResponse (CoeffType frequency) const noexcept
    {
        return lowpassFilter.getComplexResponse (frequency);
    }

    /** 
        Gets the highpass frequency response.
        
        @param frequency  The frequency in Hz
        @returns         The complex frequency response of the highpass section
    */
    DspMath::Complex<CoeffType> getHighpassResponse (CoeffType frequency) const noexcept
    {
        return highpassFilter.getComplexResponse (frequency);
    }

    /** 
        Verifies perfect reconstruction by checking the sum response.
        
        @param frequency  The frequency in Hz to test
        @returns         The magnitude of the summed response (should be close to 1.0)
    */
    CoeffType verifySummedResponse (CoeffType frequency) const noexcept
    {
        const auto lpResponse = getLowpassResponse (frequency);
        const auto hpResponse = getHighpassResponse (frequency);
        const auto summed = lpResponse + hpResponse;
        return std::abs (summed);
    }

private:
    //==============================================================================
    ButterworthFilter<SampleType> lowpassFilter;
    ButterworthFilter<SampleType> highpassFilter;

    int filterOrder = 4;
    SampleType crossoverFreq = static_cast<SampleType> (1000.0);

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LinkwitzRileyFilter)
};

//==============================================================================
/** 
    Stereo/Multi-channel Linkwitz-Riley crossover.
    
    Provides independent crossover processing for multiple channels while
    maintaining synchronized parameters across all channels.
    
    @see LinkwitzRileyFilter
*/
template <typename SampleType, int NumChannels = 2>
class LinkwitzRileyCrossover
{
public:
    //==============================================================================
    /** Constructor */
    LinkwitzRileyCrossover()
    {
        for (auto& filter : filters)
            filter = std::make_unique<LinkwitzRileyFilter<SampleType>>();
    }

    //==============================================================================
    /** Resets all channel filters */
    void reset() noexcept
    {
        for (auto& filter : filters)
            filter->reset();
    }

    /** Prepares all channel filters */
    void prepare (double sampleRate, int maximumBlockSize) noexcept
    {
        for (auto& filter : filters)
            filter->prepare (sampleRate, maximumBlockSize);
    }

    /** Sets parameters for all channels */
    void setParameters (int order, SampleType frequency, double sampleRate) noexcept
    {
        for (auto& filter : filters)
            filter->setParameters (order, frequency, sampleRate);
    }

    /** Sets crossover frequency for all channels */
    void setCrossoverFrequency (SampleType frequency) noexcept
    {
        for (auto& filter : filters)
            filter->setCrossoverFrequency (frequency);
    }

    /** 
        Processes interleaved audio with separate lowpass and highpass outputs.
        
        @param inputBuffer     Interleaved input samples [ch0, ch1, ch0, ch1, ...]
        @param lowpassBuffer   Interleaved lowpass output buffer
        @param highpassBuffer  Interleaved highpass output buffer
        @param numSamples      Number of samples per channel
    */
    void processInterleaved (const SampleType* inputBuffer,
                             SampleType* lowpassBuffer,
                             SampleType* highpassBuffer,
                             int numSamples) noexcept
    {
        for (int sample = 0; sample < numSamples; ++sample)
        {
            for (int ch = 0; ch < NumChannels; ++ch)
            {
                const auto inputIndex = sample * NumChannels + ch;
                const auto inputSample = inputBuffer[inputIndex];
                const auto outputs = filters[ch]->processCrossoverSample (inputSample);
                
                if (lowpassBuffer)
                    lowpassBuffer[inputIndex] = outputs.lowpass;
                
                if (highpassBuffer)
                    highpassBuffer[inputIndex] = outputs.highpass;
            }
        }
    }

    /** Gets a reference to a specific channel's filter */
    LinkwitzRileyFilter<SampleType>& getChannelFilter (int channel) noexcept
    {
        jassert (channel >= 0 && channel < NumChannels);
        return *filters[channel];
    }

private:
    //==============================================================================
    std::array<std::unique_ptr<LinkwitzRileyFilter<SampleType>>, NumChannels> filters;

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LinkwitzRileyCrossover)
};

//==============================================================================
/** Type aliases for convenience */
using LinkwitzRileyFilterFloat = LinkwitzRileyFilter<float>;      // float samples, double coefficients (default)
using LinkwitzRileyFilterDouble = LinkwitzRileyFilter<double>;    // double samples, double coefficients (default)
using LinkwitzRileyCrossoverStereoFloat = LinkwitzRileyCrossover<float, 2>;
using LinkwitzRileyCrossoverStereoDouble = LinkwitzRileyCrossover<double, 2>;

} // namespace yup
