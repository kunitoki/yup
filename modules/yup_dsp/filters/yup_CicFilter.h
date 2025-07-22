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
#include <algorithm>

namespace yup
{

//==============================================================================
/**
    Cascaded Integrator-Comb (CIC) filter for efficient sample rate conversion.

    CIC filters are computationally efficient digital filters used for sample
    rate conversion, particularly effective for large integer conversion ratios.
    They require no multiplications, only additions and subtractions, making them
    ideal for FPGA implementations and real-time processing with limited resources.

    Key Characteristics:
    - **No multipliers required**: Only additions, subtractions, and delays
    - **Linear phase response**: Constant group delay across frequency
    - **Efficient for large rate changes**: Particularly effective for factors ≥ 8
    - **Cascaded structure**: Multiple stages improve stopband attenuation
    - **Configurable stages**: Typically 3-5 stages for good performance

    Mathematical Foundation:
    CIC filters implement a (sin(x)/x)^N frequency response, where N is the number
    of stages. For decimation, the order is: Integrators → Downsampler → Combs.
    For interpolation, the order is: Combs → Upsampler → Integrators.

    Applications:
    - Digital down converters (DDC) and up converters (DUC)
    - Anti-aliasing for high decimation ratios (≥ 8x)
    - Multi-stage sample rate conversion (CIC + FIR compensation)
    - Software defined radio (SDR) applications
    - FPGA-based signal processing

    Limitations:
    - Significant droop in passband (compensated with FIR equalizer)
    - Limited stopband attenuation compared to FIR filters
    - Fixed frequency response shape
    - Potential for arithmetic overflow with high decimation ratios

    The filter uses a dual-precision architecture where:
    - SampleType: for audio buffer processing (float/double)
    - CoeffType: for internal calculations (defaults to double for precision)

    @see FilterDesigner, FirFilter, ButterworthFilter
*/
template <typename SampleType, typename CoeffType = double>
class CicFilter : public FilterBase<SampleType, CoeffType>
{
public:
    //==============================================================================
    /** Operation modes for CIC filter */
    enum class Mode
    {
        decimation,     /** Decimation mode: input rate > output rate */
        interpolation   /** Interpolation mode: input rate < output rate */
    };

    //==============================================================================
    /** Default constructor */
    CicFilter()
        : mode (Mode::decimation)
        , stages (3)
        , rate (2)
        , sampleCount (0)
    {
        resize (stages);
        setParameters (Mode::decimation, 3, 2);
    }

    /** Constructor with parameters */
    CicFilter (Mode filterMode, int numStages, int conversionRate)
        : mode (filterMode)
        , stages (numStages)
        , rate (conversionRate)
        , sampleCount (0)
    {
        resize (stages);
        setParameters (filterMode, numStages, conversionRate);
    }

    //==============================================================================
    /** @internal */
    void reset() noexcept override
    {
        std::fill (accumulators.begin(), accumulators.end(), static_cast<CoeffType> (0.0));
        std::fill (differentiators.begin(), differentiators.end(), static_cast<CoeffType> (0.0));
        std::fill (previousValues.begin(), previousValues.end(), static_cast<CoeffType> (0.0));
        sampleCount = 0;
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
        switch (mode)
        {
            case Mode::decimation:
                return processDecimation (inputSample);

            case Mode::interpolation:
                return processInterpolation (inputSample);
        }

        return static_cast<SampleType> (0.0);
    }

    /** @internal */
    void processBlock (const SampleType* inputBuffer, SampleType* outputBuffer, int numSamples) noexcept override
    {
        if (mode == Mode::decimation)
        {
            processDecimationBlock (inputBuffer, outputBuffer, numSamples);
        }
        else
        {
            processInterpolationBlock (inputBuffer, outputBuffer, numSamples);
        }
    }

    /** @internal */
    DspMath::Complex<CoeffType> getComplexResponse (CoeffType frequency) const noexcept override
    {
        // CIC frequency response: (sin(π*f*R) / (π*f*R))^N
        if (this->sampleRate <= 0.0)
            return DspMath::Complex<CoeffType> (static_cast<CoeffType> (1.0), static_cast<CoeffType> (0.0));

        const auto normalizedFreq = frequency / this->sampleRate;
        const auto x = MathConstants<CoeffType>::pi * normalizedFreq * static_cast<CoeffType> (rate);

        if (std::abs (x) < static_cast<CoeffType> (1e-10))
        {
            // Use limit as x approaches 0: sinc(x) = 1
            return DspMath::Complex<CoeffType> (static_cast<CoeffType> (1.0), static_cast<CoeffType> (0.0));
        }

        const auto sinc = std::sin (x) / x;
        const auto magnitude = std::pow (sinc, static_cast<CoeffType> (stages));

        // CIC filters have linear phase (real-valued response)
        return DspMath::Complex<CoeffType> (magnitude, static_cast<CoeffType> (0.0));
    }

    //==============================================================================
    /**
        Sets all filter parameters.

        @param filterMode      The operation mode (decimation or interpolation)
        @param numStages       The number of CIC stages (typically 3-5)
        @param conversionRate  The integer conversion rate (≥ 2)
    */
    void setParameters (Mode filterMode, int numStages, int conversionRate) noexcept
    {
        jassert (numStages >= 1 && numStages <= 10);
        jassert (conversionRate >= 2);

        const auto newStages = jlimit (1, 10, numStages);
        const auto newRate = jmax (2, conversionRate);

        if (stages != newStages)
        {
            stages = newStages;
            resize (stages);
        }

        mode = filterMode;
        rate = newRate;
        reset();
    }

    /**
        Sets the number of CIC stages.

        @param numStages  The number of stages (1-10, typically 3-5)
    */
    void setStages (int numStages) noexcept
    {
        const auto newStages = jlimit (1, 10, numStages);
        if (stages != newStages)
        {
            stages = newStages;
            resize (stages);
            reset();
        }
    }

    /**
        Sets the conversion rate.

        @param conversionRate  The integer conversion rate (≥ 2)
    */
    void setRate (int conversionRate) noexcept
    {
        rate = jmax (2, conversionRate);
        reset();
    }

    /**
        Sets the operation mode.

        @param filterMode  The operation mode
    */
    void setMode (Mode filterMode) noexcept
    {
        mode = filterMode;
        reset();
    }

    //==============================================================================
    /** Gets the current number of stages */
    int getStages() const noexcept { return stages; }

    /** Gets the current conversion rate */
    int getRate() const noexcept { return rate; }

    /** Gets the current operation mode */
    Mode getMode() const noexcept { return mode; }

    /**
        Calculates the DC gain of the CIC filter.

        @returns The DC gain (rate^stages)
    */
    CoeffType getDcGain() const noexcept
    {
        return std::pow (static_cast<CoeffType> (rate), static_cast<CoeffType> (stages));
    }

    /**
        Calculates the passband droop at a given frequency.

        @param frequency  The frequency in Hz
        @returns         The magnitude response (0.0 to 1.0)
    */
    CoeffType getPassbandResponse (CoeffType frequency) const noexcept
    {
        if (this->sampleRate <= 0.0)
            return static_cast<CoeffType> (1.0);

        const auto response = getComplexResponse (frequency);
        return std::abs (response);
    }

    /**
        Estimates the equivalent noise bandwidth of the CIC filter.

        @returns The noise bandwidth factor relative to sample rate
    */
    CoeffType getEquivalentNoiseBandwidth() const noexcept
    {
        // Approximation for CIC filter noise bandwidth
        return static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (2.0) * static_cast<CoeffType> (stages) + static_cast<CoeffType> (1.0));
    }

private:
    //==============================================================================
    void resize (int numStages) noexcept
    {
        accumulators.resize (static_cast<size_t> (numStages), static_cast<CoeffType> (0.0));
        differentiators.resize (static_cast<size_t> (numStages), static_cast<CoeffType> (0.0));
        previousValues.resize (static_cast<size_t> (numStages), static_cast<CoeffType> (0.0));
    }

    //==============================================================================
    SampleType processDecimation (SampleType input) noexcept
    {
        // Decimation: Integrate → Downsample → Differentiate

        // Integrator stages (run at high sample rate)
        accumulators[0] += static_cast<CoeffType> (input);
        for (int i = 1; i < stages; ++i)
        {
            accumulators[i] += accumulators[i - 1];
        }

        ++sampleCount;

        // Downsample: only output every 'rate' samples
        if (sampleCount >= rate)
        {
            sampleCount = 0;

            // Differentiator stages (run at low sample rate)
            differentiators[0] = accumulators[stages - 1] - previousValues[0];
            previousValues[0] = accumulators[stages - 1];

            for (int i = 1; i < stages; ++i)
            {
                differentiators[i] = differentiators[i - 1] - previousValues[i];
                previousValues[i] = differentiators[i - 1];
            }

            return static_cast<SampleType> (differentiators[stages - 1]);
        }

        return static_cast<SampleType> (0.0);
    }

    SampleType processInterpolation (SampleType input) noexcept
    {
        // Interpolation: Differentiate → Upsample → Integrate

        if (sampleCount == 0)
        {
            // Process input through differentiator stages (run at low sample rate)
            differentiators[0] = static_cast<CoeffType> (input) - previousValues[0];
            previousValues[0] = static_cast<CoeffType> (input);

            for (int i = 1; i < stages; ++i)
            {
                differentiators[i] = differentiators[i - 1] - previousValues[i];
                previousValues[i] = differentiators[i - 1];
            }

            accumulators[0] += differentiators[stages - 1];
        }
        else
        {
            // Zero-stuff (no new input during upsampling)
            accumulators[0] += static_cast<CoeffType> (0.0);
        }

        // Integrator stages (run at high sample rate)
        for (int i = 1; i < stages; ++i)
        {
            accumulators[i] += accumulators[i - 1];
        }

        ++sampleCount;
        if (sampleCount >= rate)
            sampleCount = 0;

        return static_cast<SampleType> (accumulators[stages - 1]);
    }

    //==============================================================================
    void processDecimationBlock (const SampleType* inputBuffer, SampleType* outputBuffer, int numSamples) noexcept
    {
        int outputIndex = 0;

        for (int i = 0; i < numSamples; ++i)
        {
            const auto output = processDecimation (inputBuffer[i]);

            // Only store output when decimation produces a sample
            if (sampleCount == 0) // Just reset, meaning we produced an output
            {
                outputBuffer[outputIndex++] = output;
            }
        }

        // Note: outputBuffer should be sized appropriately by caller
        // Output length ≈ numSamples / rate
    }

    void processInterpolationBlock (const SampleType* inputBuffer, SampleType* outputBuffer, int numSamples) noexcept
    {
        int outputIndex = 0;

        for (int i = 0; i < numSamples; ++i)
        {
            for (int j = 0; j < rate; ++j)
            {
                const auto input = (j == 0) ? inputBuffer[i] : static_cast<SampleType> (0.0);
                outputBuffer[outputIndex++] = processInterpolation (input);
            }
        }

        // Note: outputBuffer should be sized as numSamples * rate
    }

    //==============================================================================
    Mode mode;
    int stages;
    int rate;
    int sampleCount;

    std::vector<CoeffType> accumulators;
    std::vector<CoeffType> differentiators;
    std::vector<CoeffType> previousValues;

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CicFilter)
};

//==============================================================================
/** Type aliases for convenience */
using CicFilterFloat = CicFilter<float>;       // float samples, double coefficients (default)
using CicFilterDouble = CicFilter<double>;     // double samples, double coefficients (default)

} // namespace yup