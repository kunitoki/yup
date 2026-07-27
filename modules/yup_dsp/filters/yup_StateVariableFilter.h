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
    State Variable Filter (SVF) implementation.

    This filter simultaneously produces lowpass, bandpass, highpass, and notch
    outputs from a single input. It's particularly useful for real-time parameter
    changes as it maintains stability and smooth response updates.

    The SVF uses a topology based on integrators that mimics analog filter behavior,
    providing excellent frequency response characteristics and efficient computation.

    Features:
    - Simultaneous LP/BP/HP/Notch outputs
    - Smooth parameter updates
    - Stable across the full frequency range
    - Resonance control via Q parameter

    @see FilterBase
*/
template <typename SampleType, typename CoeffType = double>
class StateVariableFilter : public FilterBase<SampleType, CoeffType>
{
public:
    //==============================================================================
    /** Structure containing all filter outputs */
    struct Outputs
    {
        SampleType lowpass = 0;  /**< Low-pass output */
        SampleType highpass = 0; /**< High-pass output */
        SampleType bandpass = 0; /**< Band-pass output */
        SampleType bandstop = 0; /**< Notch output */
    };

    //==============================================================================
    /** Default constructor */
    StateVariableFilter()
    {
        setParameters (FilterMode::lowpass, static_cast<CoeffType> (1000.0), static_cast<CoeffType> (0.707), 44100.0);
    }

    /** Constructor with initial mode */
    explicit StateVariableFilter (FilterModeType initialMode)
    {
        setParameters (initialMode, static_cast<CoeffType> (1000.0), static_cast<CoeffType> (0.707), 44100.0);
    }

    //==============================================================================
    /**
        Sets the filter parameters.

        @param frequency   The cutoff frequency in Hz
        @param q          The Q factor (resonance)
        @param sampleRate The sample rate in Hz
    */
    void setParameters (FilterModeType mode, CoeffType frequency, CoeffType q, double sampleRate) noexcept
    {
        mode = resolveFilterMode (mode, this->getSupportedModes());

        if (filterMode != mode
            || ! approximatelyEqual (centerFreq, frequency)
            || ! approximatelyEqual (qFactor, q)
            || ! approximatelyEqual (this->sampleRate, sampleRate))
        {
            filterMode = mode;
            centerFreq = frequency;
            qFactor = q;

            this->sampleRate = sampleRate;

            updateCoefficients();
        }
    }

    /**
        Sets just the cutoff frequency.

        @param frequency  The new cutoff frequency in Hz
    */
    void setCutoffFrequency (CoeffType frequency) noexcept
    {
        if (! approximatelyEqual (centerFreq, frequency))
        {
            centerFreq = frequency;

            updateCoefficients();
        }
    }

    /**
        Sets just the Q factor.

        @param q  The new Q factor
    */
    void setQ (CoeffType q) noexcept
    {
        if (! approximatelyEqual (qFactor, q))
        {
            qFactor = q;

            updateCoefficients();
        }
    }

    /**
        Sets the filter mode for single-output processing.

        @param newMode  The new filter mode
    */
    void setMode (FilterModeType mode) noexcept
    {
        mode = resolveFilterMode (mode, this->getSupportedModes());

        if (filterMode != mode)
        {
            filterMode = mode;

            updateCoefficients();
        }
    }

    /**
        Gets the current cutoff frequency.

        @returns  The cutoff frequency in Hz
    */
    CoeffType getFrequency() const noexcept
    {
        return centerFreq;
    }

    /**
        Gets the current Q factor.

        @returns  The Q factor
    */
    CoeffType getQ() const noexcept
    {
        return qFactor;
    }

    /**
        Gets the current filter mode.

        @returns  The current filter mode
    */
    FilterModeType getMode() const noexcept
    {
        return filterMode;
    }

    //==============================================================================
    /**
        Processes a sample and returns all outputs.

        @param inputSample  The input sample
        @returns           Structure containing all filter outputs
    */
    Outputs processAllOutputs (SampleType inputSample) noexcept
    {
        Outputs outputs;

        outputs.highpass = (inputSample - coefficients.damping * state.s1 - state.s2) * coefficients.g;
        outputs.bandpass = outputs.highpass * coefficients.k + state.s1;
        outputs.lowpass = outputs.bandpass * coefficients.k + state.s2;
        outputs.bandstop = outputs.highpass + outputs.lowpass;

        state.s1 = outputs.bandpass;
        state.s2 = outputs.lowpass;

        return outputs;
    }

    /**
        Processes a block and fills separate buffers for each output.

        @param inputBuffer    The input buffer
        @param lowpassBuffer  Buffer for lowpass output (can be nullptr)
        @param highpassBuffer Buffer for highpass output (can be nullptr)
        @param bandpassBuffer Buffer for bandpass output (can be nullptr)
        @param bandstopBuffer    Buffer for notch output (can be nullptr)
        @param numSamples     Number of samples to process
    */
    void processMultipleOutputs (const SampleType* inputBuffer,
                                 SampleType* lowpassBuffer,
                                 SampleType* highpassBuffer,
                                 SampleType* bandpassBuffer,
                                 SampleType* bandstopBuffer,
                                 int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto outputs = processAllOutputs (inputBuffer[i]);

            if (lowpassBuffer)
                lowpassBuffer[i] = outputs.lowpass;

            if (highpassBuffer)
                highpassBuffer[i] = outputs.highpass;

            if (bandpassBuffer)
                bandpassBuffer[i] = outputs.bandpass;

            if (bandstopBuffer)
                bandstopBuffer[i] = outputs.bandstop;
        }
    }

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

        updateCoefficients();

        reset();
    }

    /** @internal */
    SampleType processSample (SampleType inputSample) noexcept override
    {
        const auto outputs = processAllOutputs (inputSample);

        if (filterMode.test (FilterMode::lowpass))
            return outputs.lowpass;

        if (filterMode.test (FilterMode::highpass))
            return outputs.highpass;

        if (filterMode.test (FilterMode::bandpass))
            return outputs.bandpass;

        if (filterMode.test (FilterMode::bandstop))
            return outputs.bandstop;

        return outputs.lowpass;
    }

    /** @internal */
    void processBlock (const SampleType* inputBuffer, SampleType* outputBuffer, int numSamples) noexcept override
    {
        if (filterMode.test (FilterMode::lowpass))
            processBlockLowpass (inputBuffer, outputBuffer, numSamples);

        else if (filterMode.test (FilterMode::highpass))
            processBlockHighpass (inputBuffer, outputBuffer, numSamples);

        else if (filterMode.test (FilterMode::bandpass))
            processBlockBandpass (inputBuffer, outputBuffer, numSamples);

        else if (filterMode.test (FilterMode::bandstop))
            processBlockBandstop (inputBuffer, outputBuffer, numSamples);
    }

    /** @internal */
    Complex<CoeffType> getComplexResponse (CoeffType frequency) const noexcept override
    {
        const auto omega = frequencyToAngular (frequency, static_cast<CoeffType> (this->sampleRate));
        const auto s = Complex<CoeffType> (static_cast<SampleType> (0.0), omega);
        const auto s2 = s * s;
        const auto wc = frequencyToAngular (centerFreq, static_cast<CoeffType> (this->sampleRate));
        const auto wc2 = wc * wc;
        const auto k = jlimit (0.707, 20.0, qFactor);

        auto denominator = s2 + Complex<CoeffType> (wc / k) * s + Complex<CoeffType> (wc2) + 1e-6;

        if (filterMode.test (FilterMode::lowpass))
            return Complex<CoeffType> (wc2) / denominator;

        if (filterMode.test (FilterMode::highpass))
            return s2 / denominator;

        if (filterMode.test (FilterMode::bandpass))
            return (Complex<CoeffType> (wc / qFactor) * s) / denominator;

        if (filterMode.test (FilterMode::bandstop))
            return (s2 + Complex<CoeffType> (wc2)) / denominator;

        return Complex<CoeffType> (1.0);
    }

    /** @internal */
    void getPolesZeros (
        ComplexVector<CoeffType>& poles,
        ComplexVector<CoeffType>& zeros) const override
    {
        CoeffType f0 = centerFreq;
        CoeffType q = yup::jlimit (0.707, 20.0, qFactor);
        CoeffType fs = yup::jmax (0.1, this->sampleRate);
        CoeffType T = 1.0 / fs;
        CoeffType wc = 2.0 * yup::MathConstants<CoeffType>::pi * f0;

        // Analog prototype poles: s^2 + (wc/Q) s + wc^2 = 0
        CoeffType realPart = -wc / (2.0 * q);
        CoeffType imagPart = wc * std::sqrt (std::max (0.0, 1.0 - 1.0 / (4.0 * q * q)));
        Complex<CoeffType> pa (realPart, imagPart);
        Complex<CoeffType> pb (realPart, -imagPart);

        // Bilinear map helper: z = (2 + s T) / (2 - s T)
        auto bilinear = [T] (const Complex<CoeffType>& s) -> Complex<CoeffType>
        {
            return (2.0 + s * T) / (2.0 - s * T);
        };

        // Map poles
        poles.reserve (2);
        poles.push_back (bilinear (pa));
        poles.push_back (bilinear (pb));

        // Map zeros depending on filter mode
        zeros.reserve (2);

        if (filterMode.test (FilterMode::lowpass)) // analog zeros at s = ∞ (=> z = -1 double)
        {
            zeros.push_back (-1.0);
            zeros.push_back (-1.0);
        }
        else if (filterMode.test (FilterMode::highpass)) // analog zeros at s = 0 => z = (2+0)/(2-0) = +1 (double)
        {
            zeros.push_back (1.0);
            zeros.push_back (1.0);
        }
        else if (filterMode.test (FilterMode::bandpass)) // zeros at s = 0 => z=+1, and s=∞=>z=-1
        {
            zeros.push_back (1.0);
            zeros.push_back (-1.0);
        }
        else if (filterMode.test (FilterMode::bandstop)) // analog zeros at s = ±j wc
        {
            zeros.push_back (bilinear (Complex<CoeffType> (0.0, wc)));
            zeros.push_back (bilinear (Complex<CoeffType> (0.0, -wc)));
        }
    }

private:
    //==============================================================================
    struct StateVariableState
    {
        CoeffType s1 = static_cast<CoeffType> (0.0);
        CoeffType s2 = static_cast<CoeffType> (0.0);

        /** Resets all state variables to zero */
        void reset() noexcept
        {
            s1 = s2 = static_cast<CoeffType> (0.0);
        }
    };

    //==============================================================================
    void updateCoefficients() noexcept
    {
        coefficients.k = static_cast<CoeffType> (1.0) / jlimit (0.707, 20.0, qFactor);
        const auto omega = frequencyToAngular (centerFreq, static_cast<CoeffType> (this->sampleRate));
        coefficients.g = std::tan (omega / static_cast<CoeffType> (2.0));
        coefficients.damping = coefficients.k + coefficients.g;
        coefficients.g = coefficients.g / (static_cast<CoeffType> (1.0) + coefficients.g * coefficients.damping);
    }

    void processBlockLowpass (const SampleType* input, SampleType* output, int numSamples) noexcept
    {
        auto s1 = state.s1;
        auto s2 = state.s2;

        for (int i = 0; i < numSamples; ++i)
        {
            const auto hp = (input[i] - coefficients.damping * s1 - s2) * coefficients.g;
            const auto bp = hp * coefficients.k + s1;
            const auto lp = bp * coefficients.k + s2;

            s1 = bp;
            s2 = lp;
            output[i] = lp;
        }

        state.s1 = s1;
        state.s2 = s2;
    }

    void processBlockHighpass (const SampleType* input, SampleType* output, int numSamples) noexcept
    {
        auto s1 = state.s1;
        auto s2 = state.s2;

        for (int i = 0; i < numSamples; ++i)
        {
            const auto hp = (input[i] - coefficients.damping * s1 - s2) * coefficients.g;
            const auto bp = hp * coefficients.k + s1;
            const auto lp = bp * coefficients.k + s2;

            s1 = bp;
            s2 = lp;
            output[i] = hp;
        }

        state.s1 = s1;
        state.s2 = s2;
    }

    void processBlockBandpass (const SampleType* input, SampleType* output, int numSamples) noexcept
    {
        auto s1 = state.s1;
        auto s2 = state.s2;

        for (int i = 0; i < numSamples; ++i)
        {
            const auto hp = (input[i] - coefficients.damping * s1 - s2) * coefficients.g;
            const auto bp = hp * coefficients.k + s1;
            const auto lp = bp * coefficients.k + s2;

            s1 = bp;
            s2 = lp;
            output[i] = bp;
        }

        state.s1 = s1;
        state.s2 = s2;
    }

    void processBlockBandstop (const SampleType* input, SampleType* output, int numSamples) noexcept
    {
        auto s1 = state.s1;
        auto s2 = state.s2;

        for (int i = 0; i < numSamples; ++i)
        {
            const auto inputSample = input[i];
            const auto hp = (inputSample - coefficients.damping * s1 - s2) * coefficients.g;
            const auto bp = hp * coefficients.k + s1;
            const auto lp = bp * coefficients.k + s2;

            s1 = bp;
            s2 = lp;
            output[i] = inputSample - coefficients.damping * s1;
        }

        state.s1 = s1;
        state.s2 = s2;
    }

    //==============================================================================
    FilterModeType filterMode = FilterMode::lowpass;
    CoeffType centerFreq = static_cast<CoeffType> (1000.0);
    CoeffType qFactor = static_cast<CoeffType> (0.707);

    StateVariableCoefficients<CoeffType> coefficients;
    StateVariableState state;

    //==============================================================================
    YUP_LEAK_DETECTOR (StateVariableFilter)
};

//==============================================================================
/** Type aliases for convenience */
using StateVariableFilterFloat = StateVariableFilter<float>;   // float samples, double coefficients (default)
using StateVariableFilterDouble = StateVariableFilter<double>; // double samples, double coefficients (default)

} // namespace yup
