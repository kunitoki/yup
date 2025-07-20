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
    First-order IIR filter implementation.
    
    This class implements various first-order filters including:
    - One-pole lowpass and highpass filters
    - First-order shelving filters
    - Allpass filters
    
    The filter implements the difference equation:
    y[n] = b0*x[n] + b1*x[n-1] - a1*y[n-1]
    
    @see FilterBase, FirstOrderCoefficients, FirstOrderState
*/
template <typename SampleType, typename CoeffType = double>
class FirstOrderFilter : public FilterBase<SampleType, CoeffType>
{
public:
    //==============================================================================
    /** Default constructor */
    FirstOrderFilter() = default;

    //==============================================================================
    /** @internal */
    void reset() noexcept override
    {
        state.reset();
    }

    /** @internal */
    void prepare (double sampleRate, int maximumBlockSize) noexcept override
    {
        this->sampleRate = sampleRate;
        this->maximumBlockSize = maximumBlockSize;
        reset();
    }

    /** @internal */
    SampleType processSample (SampleType inputSample) noexcept override
    {
        return state.processSample (inputSample, coefficients);
    }

    /** @internal */
    void processBlock (const SampleType* inputBuffer, SampleType* outputBuffer, int numSamples) noexcept override
    {
        auto x1 = state.x1;
        auto y1 = state.y1;
        const auto b0 = coefficients.b0;
        const auto b1 = coefficients.b1;
        const auto a1 = coefficients.a1;

        for (int i = 0; i < numSamples; ++i)
        {
            const auto input = inputBuffer[i];
            const auto output = b0 * input + b1 * x1 - a1 * y1;

            x1 = input;
            y1 = output;
            outputBuffer[i] = output;
        }

        state.x1 = x1;
        state.y1 = y1;
    }

    /** @internal */
    DspMath::Complex<CoeffType> getComplexResponse (CoeffType frequency) const noexcept override
    {
        return coefficients.getComplexResponse (frequency, this->sampleRate);
    }

    //==============================================================================
    /** 
        Sets the filter coefficients.
        
        @param newCoefficients  The new first-order coefficients
    */
    void setCoefficients (const FirstOrderCoefficients<CoeffType>& newCoefficients) noexcept
    {
        coefficients = newCoefficients;
    }

    /** 
        Gets the current filter coefficients.
        
        @returns  The current first-order coefficients
    */
    const FirstOrderCoefficients<CoeffType>& getCoefficients() const noexcept
    {
        return coefficients;
    }

    //==============================================================================
    /** 
        Configures the filter as a one-pole lowpass.
        
        @param frequency   The cutoff frequency in Hz
        @param sampleRate  The sample rate in Hz
    */
    void makeLowpass (CoeffType frequency, double sampleRate) noexcept
    {
        const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate));
        const auto alpha = std::exp (-omega);
        
        coefficients.b0 = static_cast<CoeffType> (1.0) - alpha;
        coefficients.b1 = static_cast<CoeffType> (0.0);
        coefficients.a1 = -alpha;
    }

    /** 
        Configures the filter as a one-pole highpass.
        
        @param frequency   The cutoff frequency in Hz
        @param sampleRate  The sample rate in Hz
    */
    void makeHighpass (CoeffType frequency, double sampleRate) noexcept
    {
        const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate));
        const auto alpha = std::exp (-omega);
        
        coefficients.b0 = (static_cast<CoeffType> (1.0) + alpha) / static_cast<CoeffType> (2.0);
        coefficients.b1 = -(static_cast<CoeffType> (1.0) + alpha) / static_cast<CoeffType> (2.0);
        coefficients.a1 = -alpha;
    }

    /** 
        Configures the filter as a first-order allpass.
        
        @param frequency   The characteristic frequency in Hz
        @param sampleRate  The sample rate in Hz
    */
    void makeAllpass (CoeffType frequency, double sampleRate) noexcept
    {
        const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate));
        const auto alpha = (static_cast<CoeffType> (1.0) - std::tan (omega / static_cast<CoeffType> (2.0))) /
                          (static_cast<CoeffType> (1.0) + std::tan (omega / static_cast<CoeffType> (2.0)));
        
        coefficients.b0 = alpha;
        coefficients.b1 = static_cast<CoeffType> (1.0);
        coefficients.a1 = alpha;
    }

    /** 
        Configures the filter as a low-shelf.
        
        @param frequency   The shelf frequency in Hz
        @param gainDb      The shelf gain in decibels
        @param sampleRate  The sample rate in Hz
    */
    void makeLowShelf (CoeffType frequency, CoeffType gainDb, double sampleRate) noexcept
    {
        const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate));
        const auto gain = DspMath::dbToGain (gainDb);
        const auto k = std::tan (omega / static_cast<CoeffType> (2.0));
        
        if (gainDb >= static_cast<CoeffType> (0.0))
        {
            const auto norm = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) + k);
            coefficients.b0 = (static_cast<CoeffType> (1.0) + gain * k) * norm;
            coefficients.b1 = (gain * k - static_cast<CoeffType> (1.0)) * norm;
            coefficients.a1 = (k - static_cast<CoeffType> (1.0)) * norm;
        }
        else
        {
            const auto norm = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) + k / gain);
            coefficients.b0 = (static_cast<CoeffType> (1.0) + k) * norm;
            coefficients.b1 = (k - static_cast<CoeffType> (1.0)) * norm;
            coefficients.a1 = (k / gain - static_cast<CoeffType> (1.0)) * norm;
        }
    }

    /** 
        Configures the filter as a high-shelf.
        
        @param frequency   The shelf frequency in Hz
        @param gainDb      The shelf gain in decibels
        @param sampleRate  The sample rate in Hz
    */
    void makeHighShelf (CoeffType frequency, CoeffType gainDb, double sampleRate) noexcept
    {
        const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate));
        const auto gain = DspMath::dbToGain (gainDb);
        const auto k = std::tan (omega / static_cast<CoeffType> (2.0));
        
        if (gainDb >= static_cast<CoeffType> (0.0))
        {
            const auto norm = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) + k);
            coefficients.b0 = (gain + k) * norm;
            coefficients.b1 = (k - gain) * norm;
            coefficients.a1 = (k - static_cast<CoeffType> (1.0)) * norm;
        }
        else
        {
            const auto norm = static_cast<CoeffType> (1.0) / (gain + k);
            coefficients.b0 = (static_cast<CoeffType> (1.0) + k) * norm;
            coefficients.b1 = (k - static_cast<CoeffType> (1.0)) * norm;
            coefficients.a1 = (k - gain) * norm;
        }
    }

private:
    //==============================================================================
    FirstOrderCoefficients<CoeffType> coefficients;
    FirstOrderState<CoeffType> state;

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FirstOrderFilter)
};

//==============================================================================
/** Type aliases for convenience */
using FirstOrderFilterFloat = FirstOrderFilter<float>;      // float samples, double coefficients (default)
using FirstOrderFilterDouble = FirstOrderFilter<double>;    // double samples, double coefficients (default)

} // namespace yup
