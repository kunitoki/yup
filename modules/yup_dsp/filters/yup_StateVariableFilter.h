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
    /** Filter mode enumeration for single-output processing */
    enum class Mode
    {
        lowpass,   /**< Low-pass output only */
        bandpass,  /**< Band-pass output only */
        highpass,  /**< High-pass output only */
        notch      /**< Notch (band-stop) output only */
    };

    /** Structure containing all filter outputs */
    struct Outputs
    {
        SampleType lowpass = 0;   /**< Low-pass output */
        SampleType bandpass = 0;  /**< Band-pass output */
        SampleType highpass = 0;  /**< High-pass output */
        SampleType notch = 0;     /**< Notch output */
    };

    //==============================================================================
    /** Default constructor */
    StateVariableFilter() noexcept
    {
        setParameters (Mode::lowpass, static_cast<CoeffType> (1000.0), static_cast<CoeffType> (0.707), 44100.0);
    }

    /** Constructor with initial mode */
    explicit StateVariableFilter (Mode initialMode) noexcept
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
    void setParameters (Mode mode, CoeffType frequency, CoeffType q, double sampleRate) noexcept
    {
        cutoffFreq = frequency;
        qFactor = q;
        filterMode = mode;
        this->sampleRate = sampleRate;

        updateCoefficients();
    }

    /**
        Sets just the cutoff frequency.

        @param frequency  The new cutoff frequency in Hz
    */
    void setCutoffFrequency (CoeffType frequency) noexcept
    {
        cutoffFreq = frequency;
        updateCoefficients();
    }

    /**
        Sets just the Q factor.

        @param q  The new Q factor
    */
    void setQFactor (CoeffType q) noexcept
    {
        qFactor = q;
        updateCoefficients();
    }

    /**
        Sets the filter mode for single-output processing.

        @param newMode  The new filter mode
    */
    void setMode (Mode newMode) noexcept
    {
        filterMode = newMode;
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
        Gets the current Q factor.

        @returns  The Q factor
    */
    CoeffType getQFactor() const noexcept
    {
        return qFactor;
    }

    /**
        Gets the current filter mode.

        @returns  The current filter mode
    */
    Mode getMode() const noexcept
    {
        return filterMode;
    }

    //==============================================================================

    void getPolesZeros (std::vector<std::complex<double>>& poles,
                        std::vector<std::complex<double>>& zeros)
    {
        double f0 = getCutoffFrequency();
        double q  = yup::jlimit (0.707, 20.0, getQFactor());
        double fs = yup::jmax (0.1, this->sampleRate);
        double T = 1.0 / fs;
        double wc = 2.0 * yup::MathConstants<double>::pi * f0;

        // Analog prototype poles: s^2 + (wc/Q) s + wc^2 = 0
        double realPart = -wc / (2.0 * q);
        double imagPart = wc * std::sqrt (std::max (0.0, 1.0 - 1.0 / (4.0 * q * q)));
        std::complex<double> pa (realPart, imagPart);
        std::complex<double> pb (realPart, -imagPart);

        // Bilinear map helper: z = (2 + s T) / (2 - s T)
        auto bilinear = [T](const std::complex<double>& s) -> std::complex<double> { return (2.0 + s * T) / (2.0 - s * T); };

        // Map poles
        poles.clear();
        poles.reserve (2);
        poles.push_back (bilinear(pa));
        poles.push_back (bilinear(pb));

        // Map zeros depending on filter mode
        zeros.clear();
        zeros.reserve(2);

        switch (filterMode)
        {
            case Mode::lowpass: // analog zeros at s = ∞ (=> z = -1 double)
                zeros.push_back (-1.0);
                zeros.push_back (-1.0);
                break;

            case Mode::highpass: // analog zeros at s = 0 => z = (2+0)/(2-0) = +1 (double)
                zeros.push_back (1.0);
                zeros.push_back (1.0);
                break;

            case Mode::bandpass: // zeros at s = 0 => z=+1, and s=∞=>z=-1
                zeros.push_back (1.0);
                zeros.push_back (-1.0);
                break;

            case Mode::notch: // analog zeros at s = ±j wc
                zeros.push_back (bilinear (std::complex<double> (0.0, wc)));
                zeros.push_back (bilinear (std::complex<double> (0.0, -wc)));
                break;
        }
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

        outputs.highpass = (inputSample - damping * state1 - state2) * g;
        outputs.bandpass = outputs.highpass * k + state1;
        outputs.lowpass = outputs.bandpass * k + state2;
        outputs.notch = outputs.highpass + outputs.lowpass;

        state1 = outputs.bandpass;
        state2 = outputs.lowpass;

        return outputs;
    }

    /**
        Processes a block and fills separate buffers for each output.

        @param inputBuffer    The input buffer
        @param lowpassBuffer  Buffer for lowpass output (can be nullptr)
        @param bandpassBuffer Buffer for bandpass output (can be nullptr)
        @param highpassBuffer Buffer for highpass output (can be nullptr)
        @param notchBuffer    Buffer for notch output (can be nullptr)
        @param numSamples     Number of samples to process
    */
    void processMultipleOutputs (const SampleType* inputBuffer,
                                 SampleType* lowpassBuffer,
                                 SampleType* bandpassBuffer,
                                 SampleType* highpassBuffer,
                                 SampleType* notchBuffer,
                                 int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto outputs = processAllOutputs (inputBuffer[i]);

            if (lowpassBuffer)
                lowpassBuffer[i] = outputs.lowpass;

            if (bandpassBuffer)
                bandpassBuffer[i] = outputs.bandpass;

            if (highpassBuffer)
                highpassBuffer[i] = outputs.highpass;

            if (notchBuffer)
                notchBuffer[i] = outputs.notch;
        }
    }

    //==============================================================================
    /** @internal */
    void reset() noexcept override
    {
        state1 = state2 = static_cast<CoeffType> (0.0);
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

        switch (filterMode)
        {
            case Mode::lowpass:  return outputs.lowpass;
            case Mode::bandpass: return outputs.bandpass;
            case Mode::highpass: return outputs.highpass;
            case Mode::notch:    return outputs.notch;
            default:             return outputs.lowpass;
        }
    }

    /** @internal */
    void processBlock (const SampleType* inputBuffer, SampleType* outputBuffer, int numSamples) noexcept override
    {
        switch (filterMode)
        {
            case Mode::lowpass:
                processBlockLowpass (inputBuffer, outputBuffer, numSamples);
                break;

            case Mode::bandpass:
                processBlockBandpass (inputBuffer, outputBuffer, numSamples);
                break;

            case Mode::highpass:
                processBlockHighpass (inputBuffer, outputBuffer, numSamples);
                break;

            case Mode::notch:
                processBlockNotch (inputBuffer, outputBuffer, numSamples);
                break;
        }
    }

    /** @internal */
    DspMath::Complex<CoeffType> getComplexResponse (CoeffType frequency) const noexcept override
    {
        const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (this->sampleRate));
        const auto s = DspMath::Complex<CoeffType> (static_cast<SampleType> (0.0), omega);
        const auto s2 = s * s;
        const auto wc = DspMath::frequencyToAngular (cutoffFreq, static_cast<CoeffType> (this->sampleRate));
        const auto wc2 = wc * wc;
        const auto k = jlimit (0.707, 20.0, qFactor);

        auto denominator = s2 + DspMath::Complex<CoeffType> (wc / k) * s + DspMath::Complex<CoeffType> (wc2) + 1e-6;

        switch (filterMode)
        {
            case Mode::lowpass:
                return DspMath::Complex<CoeffType> (wc2) / denominator;

            case Mode::bandpass:
                return (DspMath::Complex<CoeffType> (wc / qFactor) * s) / denominator;

            case Mode::highpass:
                return s2 / denominator;

            case Mode::notch:
                return (s2 + DspMath::Complex<CoeffType> (wc2)) / denominator;

            default:
                return DspMath::Complex<CoeffType> (1.0);
        }
    }

private:
    //==============================================================================
    void updateCoefficients() noexcept
    {
        k = static_cast<CoeffType> (1.0) / jlimit (0.707, 20.0, qFactor);
        const auto omega = DspMath::frequencyToAngular (cutoffFreq, static_cast<CoeffType> (this->sampleRate));
        g = std::tan (omega / static_cast<CoeffType> (2.0));
        damping = k + g;
        g = g / (static_cast<CoeffType> (1.0) + g * damping);
    }

    void processBlockLowpass (const SampleType* input, SampleType* output, int numSamples) noexcept
    {
        auto s1 = state1;
        auto s2 = state2;

        for (int i = 0; i < numSamples; ++i)
        {
            const auto hp = (input[i] - damping * s1 - s2) * g;
            const auto bp = hp * k + s1;
            const auto lp = bp * k + s2;

            s1 = bp;
            s2 = lp;
            output[i] = lp;
        }

        state1 = s1;
        state2 = s2;
    }

    void processBlockBandpass (const SampleType* input, SampleType* output, int numSamples) noexcept
    {
        auto s1 = state1;
        auto s2 = state2;

        for (int i = 0; i < numSamples; ++i)
        {
            const auto hp = (input[i] - damping * s1 - s2) * g;
            const auto bp = hp * k + s1;
            const auto lp = bp * k + s2;

            s1 = bp;
            s2 = lp;
            output[i] = bp;
        }

        state1 = s1;
        state2 = s2;
    }

    void processBlockHighpass (const SampleType* input, SampleType* output, int numSamples) noexcept
    {
        auto s1 = state1;
        auto s2 = state2;

        for (int i = 0; i < numSamples; ++i)
        {
            const auto hp = (input[i] - damping * s1 - s2) * g;
            const auto bp = hp * k + s1;
            const auto lp = bp * k + s2;

            s1 = bp;
            s2 = lp;
            output[i] = hp;
        }

        state1 = s1;
        state2 = s2;
    }

    void processBlockNotch (const SampleType* input, SampleType* output, int numSamples) noexcept
    {
        auto s1 = state1;
        auto s2 = state2;

        for (int i = 0; i < numSamples; ++i)
        {
            const auto inputSample = input[i];
            const auto hp = (inputSample - damping * s1 - s2) * g;
            const auto bp = hp * k + s1;
            const auto lp = bp * k + s2;

            s1 = bp;
            s2 = lp;
            output[i] = inputSample - damping * s1;
        }

        state1 = s1;
        state2 = s2;
    }

    //==============================================================================
    CoeffType cutoffFreq = static_cast<CoeffType> (1000.0);
    CoeffType qFactor = static_cast<CoeffType> (0.707);
    Mode filterMode = Mode::lowpass;

    CoeffType k = static_cast<CoeffType> (1.0);
    CoeffType g = static_cast<CoeffType> (1.0);
    CoeffType damping = static_cast<CoeffType> (1.0);

    CoeffType state1 = static_cast<CoeffType> (0.0);
    CoeffType state2 = static_cast<CoeffType> (0.0);

    //==============================================================================
    YUP_LEAK_DETECTOR (StateVariableFilter)
};

//==============================================================================
/** Type aliases for convenience */
using StateVariableFilterFloat = StateVariableFilter<float>;      // float samples, double coefficients (default)
using StateVariableFilterDouble = StateVariableFilter<double>;    // double samples, double coefficients (default)

} // namespace yup
