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
#include <cmath>

namespace yup
{

//==============================================================================
/** 
    Interpolation type for delay line readback.
*/
enum class DelayInterpolationType
{
    none,      /**< No interpolation (nearest sample) */
    linear,    /**< Linear interpolation */
    cubic,     /**< Cubic Hermite interpolation */
    allpass    /**< Thiran allpass interpolation */
};

//==============================================================================
/** 
    High-quality interpolated delay line with fractional delay support.
    
    This delay line supports fractional delay times with various interpolation
    methods for smooth, artifact-free delays. It's optimized for real-time
    audio processing and supports dynamic delay time changes.
    
    Features:
    - Fractional delay times with sub-sample accuracy
    - Multiple interpolation methods (linear, cubic, allpass)
    - Smooth delay time modulation without artifacts
    - Efficient circular buffer implementation
    - Optional feedback with saturation protection
    
    Applications:
    - Chorus, flanger, and phaser effects
    - Pitch shifting and time stretching
    - Reverb and echo effects
    - Physical modeling synthesis
    - Digital waveguides
    
    @see DelayInterpolationType
*/
template <typename SampleType>
class InterpolatedDelayLine
{
public:
    //==============================================================================
    /** Default constructor */
    InterpolatedDelayLine() = default;
    
    /** Constructor with maximum delay time */
    explicit InterpolatedDelayLine (SampleType maxDelayInSamples)
    {
        setMaximumDelayInSamples (maxDelayInSamples);
    }

    //==============================================================================
    /** 
        Sets the maximum delay time in samples.
        
        @param newDelayInSamples  The maximum delay time in samples
    */
    void setMaximumDelayInSamples (SampleType newDelayInSamples)
    {
        jassert (newDelayInSamples > SampleType (0));
        
        maxDelayInSamples = newDelayInSamples;
        
        // Add extra samples for interpolation
        const auto bufferSize = static_cast<size_t> (std::ceil (maxDelayInSamples)) + 4;
        
        if (buffer.size() != bufferSize)
        {
            buffer.resize (bufferSize);
            reset();
        }
    }

    /** 
        Gets the maximum delay time in samples.
        
        @returns  The maximum delay time in samples
    */
    SampleType getMaximumDelayInSamples() const noexcept
    {
        return maxDelayInSamples;
    }

    //==============================================================================
    /** 
        Resets the delay line, clearing all stored samples.
    */
    void reset() noexcept
    {
        std::fill (buffer.begin(), buffer.end(), SampleType (0));
        writeIndex = 0;
        
        // Reset allpass filters for allpass interpolation
        allpassState1 = allpassState2 = SampleType (0);
    }

    //==============================================================================
    /** 
        Sets the interpolation type for fractional delays.
        
        @param type  The interpolation type to use
    */
    void setInterpolationType (DelayInterpolationType type) noexcept
    {
        interpolationType = type;
    }

    /** 
        Gets the current interpolation type.
        
        @returns  The current interpolation type
    */
    DelayInterpolationType getInterpolationType() const noexcept
    {
        return interpolationType;
    }

    //==============================================================================
    /** 
        Writes a sample to the delay line.
        
        @param inputSample  The sample to write
    */
    void write (SampleType inputSample) noexcept
    {
        buffer[writeIndex] = inputSample;
        writeIndex = (writeIndex + 1) % buffer.size();
    }

    /** 
        Reads a sample from the delay line with the specified delay.
        
        @param delayInSamples  The delay time in samples (can be fractional)
        @returns              The delayed sample
    */
    SampleType read (SampleType delayInSamples) const noexcept
    {
        jassert (delayInSamples >= SampleType (0) && delayInSamples <= maxDelayInSamples);
        
        switch (interpolationType)
        {
            case DelayInterpolationType::none:
                return readWithoutInterpolation (delayInSamples);
            
            case DelayInterpolationType::linear:
                return readWithLinearInterpolation (delayInSamples);
            
            case DelayInterpolationType::cubic:
                return readWithCubicInterpolation (delayInSamples);
            
            case DelayInterpolationType::allpass:
                return readWithAllpassInterpolation (delayInSamples);
            
            default:
                return readWithLinearInterpolation (delayInSamples);
        }
    }

    /** 
        Processes a sample through the delay line with the specified delay.
        This combines write() and read() in one operation.
        
        @param inputSample     The input sample to write
        @param delayInSamples  The delay time in samples (can be fractional)
        @returns              The delayed sample
    */
    SampleType process (SampleType inputSample, SampleType delayInSamples) noexcept
    {
        const auto output = read (delayInSamples);
        write (inputSample);
        return output;
    }

    /** 
        Processes a sample through the delay line with feedback.
        
        @param inputSample     The input sample
        @param delayInSamples  The delay time in samples
        @param feedback        The feedback amount (-1 to 1)
        @returns              The processed sample
    */
    SampleType processWithFeedback (SampleType inputSample, SampleType delayInSamples, SampleType feedback) noexcept
    {
        const auto delayedSample = read (delayInSamples);
        const auto feedbackSample = softClip (delayedSample * feedback);
        write (inputSample + feedbackSample);
        return delayedSample;
    }

private:
    //==============================================================================
    SampleType readWithoutInterpolation (SampleType delayInSamples) const noexcept
    {
        const auto delaySamples = static_cast<int> (std::round (delayInSamples));
        const auto readIndex = (writeIndex - delaySamples - 1 + static_cast<int> (buffer.size())) % static_cast<int> (buffer.size());
        return buffer[static_cast<size_t> (readIndex)];
    }

    SampleType readWithLinearInterpolation (SampleType delayInSamples) const noexcept
    {
        const auto delaySamplesFloor = std::floor (delayInSamples);
        const auto fraction = delayInSamples - delaySamplesFloor;
        
        const auto index1 = static_cast<int> (delaySamplesFloor);
        const auto index2 = index1 + 1;
        
        const auto readIndex1 = (writeIndex - index1 - 1 + static_cast<int> (buffer.size())) % static_cast<int> (buffer.size());
        const auto readIndex2 = (writeIndex - index2 - 1 + static_cast<int> (buffer.size())) % static_cast<int> (buffer.size());
        
        const auto sample1 = buffer[static_cast<size_t> (readIndex1)];
        const auto sample2 = buffer[static_cast<size_t> (readIndex2)];
        
        return sample1 + fraction * (sample2 - sample1);
    }

    SampleType readWithCubicInterpolation (SampleType delayInSamples) const noexcept
    {
        const auto delaySamplesFloor = std::floor (delayInSamples);
        const auto fraction = delayInSamples - delaySamplesFloor;
        
        const auto index = static_cast<int> (delaySamplesFloor);
        
        // Get four samples for cubic interpolation
        const auto getBufferSample = [this] (int offset) -> SampleType
        {
            const auto readIndex = (writeIndex - offset - 1 + static_cast<int> (buffer.size())) % static_cast<int> (buffer.size());
            return buffer[static_cast<size_t> (readIndex)];
        };
        
        const auto y0 = getBufferSample (index - 1);
        const auto y1 = getBufferSample (index);
        const auto y2 = getBufferSample (index + 1);
        const auto y3 = getBufferSample (index + 2);
        
        // Cubic Hermite interpolation
        const auto c0 = y1;
        const auto c1 = (y2 - y0) * SampleType (0.5);
        const auto c2 = y0 - SampleType (2.5) * y1 + SampleType (2) * y2 - SampleType (0.5) * y3;
        const auto c3 = SampleType (1.5) * (y1 - y2) + SampleType (0.5) * (y3 - y0);
        
        return ((c3 * fraction + c2) * fraction + c1) * fraction + c0;
    }

    SampleType readWithAllpassInterpolation (SampleType delayInSamples) const noexcept
    {
        // Thiran allpass interpolation for fractional delays
        const auto integerDelay = std::floor (delayInSamples);
        const auto fractionalDelay = delayInSamples - integerDelay;
        
        // Read integer delayed sample
        const auto delaySamples = static_cast<int> (integerDelay);
        const auto readIndex = (writeIndex - delaySamples - 1 + static_cast<int> (buffer.size())) % static_cast<int> (buffer.size());
        auto sample = buffer[static_cast<size_t> (readIndex)];
        
        if (fractionalDelay > SampleType (0))
        {
            // Apply Thiran allpass filter for fractional delay
            const auto alpha = (SampleType (1) - fractionalDelay) / (SampleType (1) + fractionalDelay);
            
            // First-order allpass
            const auto temp1 = sample + alpha * allpassState1;
            const_cast<InterpolatedDelayLine*>(this)->allpassState1 = sample - alpha * temp1;
            sample = temp1;
            
            // Second-order for better approximation
            const auto temp2 = sample + alpha * allpassState2;
            const_cast<InterpolatedDelayLine*>(this)->allpassState2 = sample - alpha * temp2;
            sample = temp2;
        }
        
        return sample;
    }

    /** Soft clipping function to prevent feedback explosion */
    SampleType softClip (SampleType input) const noexcept
    {
        const auto threshold = SampleType (0.95);
        if (std::abs (input) <= threshold)
            return input;
        
        const auto sign = (input >= SampleType (0)) ? SampleType (1) : SampleType (-1);
        const auto excess = std::abs (input) - threshold;
        const auto clipped = threshold + excess / (SampleType (1) + excess);
        
        return sign * clipped;
    }

    //==============================================================================
    std::vector<SampleType> buffer;
    SampleType maxDelayInSamples = SampleType (1000);
    size_t writeIndex = 0;
    
    DelayInterpolationType interpolationType = DelayInterpolationType::linear;
    
    // State for allpass interpolation
    mutable SampleType allpassState1 = SampleType (0);
    mutable SampleType allpassState2 = SampleType (0);

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InterpolatedDelayLine)
};

//==============================================================================
/** Type aliases for convenience */
using InterpolatedDelayLineFloat = InterpolatedDelayLine<float>;
using InterpolatedDelayLineDouble = InterpolatedDelayLine<double>;

} // namespace yup