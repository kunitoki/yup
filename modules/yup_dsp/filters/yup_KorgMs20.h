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
    Korg MS-20 Filter emulation using Topology Preserving Transform (TPT).

    This filter emulates the distinctive sound of the Korg MS-20 synthesizer's
    dual filter design. The MS-20 is famous for its aggressive, screaming filter
    sound with characteristic non-linear behavior and unique resonance response.

    Key features:
    - Dual-mode operation (lowpass and highpass)
    - Aggressive resonance character
    - Non-linear saturation modeling
    - Zero-delay feedback using TPT
    - Characteristic MS-20 frequency response
    - Drive-dependent harmonic content

    The filter uses a dual-precision architecture where:
    - SampleType: for audio buffer processing (float/double)
    - CoeffType: for internal calculations (defaults to double for precision)

    @see FilterBase, MoogLadder, VirtualAnalogSvf
*/
template <typename SampleType, typename CoeffType = double>
class KorgMs20 : public FilterBase<SampleType, CoeffType>
{
public:
    //==============================================================================
    /** Filter mode enumeration */
    enum class Mode
    {
        lowpass,   /**< Lowpass mode (MS-20 main filter) */
        highpass   /**< Highpass mode (MS-20 secondary filter) */
    };

    //==============================================================================
    /** Constructor with optional parameters */
    explicit KorgMs20 (CoeffType frequency = static_cast<CoeffType> (1000.0),
                       CoeffType resonance = static_cast<CoeffType> (0.1),
                       Mode mode = Mode::lowpass)
        : cutoffFreq (frequency), resonanceAmount (resonance), filterMode (mode)
    {
        updateCoefficients();
    }

    //==============================================================================
    /** @internal */
    void reset() noexcept override
    {
        s1 = s2 = static_cast<CoeffType> (0.0);
        z1 = z2 = static_cast<CoeffType> (0.0);
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

        // Apply pre-filter saturation (MS-20 characteristic)
        input = applyPreSaturation (input);

        // Calculate feedback signal
        const auto feedback = k * (s1 + s2);

        // Input with feedback and non-linear processing
        const auto inputWithFeedback = input - feedback;
        const auto processedInput = applyNonLinearProcessing (inputWithFeedback);

        // 2-pole filter implementation (simplified MS-20 topology)
        const auto v1 = (processedInput - s1) * g;
        const auto y1 = v1 + s1;
        s1 = y1 + v1;

        const auto v2 = (y1 - s2) * g;
        const auto y2 = v2 + s2;
        s2 = y2 + v2;

        // Mode selection and output processing
        CoeffType output;
        if (filterMode == Mode::lowpass)
        {
            output = y2 * nonLinearGain;
        }
        else
        {
            // Highpass mode
            output = (processedInput - k * y1) * nonLinearGain;
        }

        // Apply post-filter saturation
        output = applyPostSaturation (output);

        // Store intermediate values for multi-mode output
        z1 = y1;
        z2 = y2;

        return static_cast<SampleType> (output);
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

        // 2-pole response approximation
        const auto omega_c = DspMath::frequencyToAngular (cutoffFreq, static_cast<CoeffType> (this->sampleRate));
        const auto pole = DspMath::Complex<CoeffType> (-omega_c, 0);

        if (filterMode == Mode::lowpass)
        {
            return static_cast<CoeffType> (1.0) / ((s - pole) * (s - pole));
        }
        else
        {
            return (s * s) / ((s - pole) * (s - pole));
        }
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

        @param resonance  The resonance amount (0.0 to 1.0, where 1.0 approaches self-oscillation)
    */
    void setResonance (CoeffType resonance) noexcept
    {
        resonanceAmount = jlimit (static_cast<CoeffType> (0.0), static_cast<CoeffType> (0.99), resonance);
        updateCoefficients();
    }

    /**
        Sets the filter mode.

        @param mode  The desired filter mode (lowpass or highpass)
    */
    void setMode (Mode mode) noexcept
    {
        filterMode = mode;
    }

    /**
        Sets all parameters simultaneously.

        @param frequency  The cutoff frequency in Hz
        @param resonance  The resonance amount (0.0 to 1.0)
        @param mode       The filter mode
    */
    void setParameters (CoeffType frequency, CoeffType resonance, Mode mode = Mode::lowpass) noexcept
    {
        setCutoffFrequency (frequency);
        setResonance (resonance);
        setMode (mode);
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
        Gets the current filter mode.

        @returns  The current filter mode
    */
    Mode getMode() const noexcept
    {
        return filterMode;
    }

    //==============================================================================
    /**
        Gets the intermediate lowpass output (useful for dual-mode operation).

        @returns  The lowpass stage output (requires processSample to be called first)
    */
    CoeffType getLowpassOutput() const noexcept
    {
        return z2;
    }

    /**
        Gets the intermediate bandpass output.

        @returns  The bandpass stage output (requires processSample to be called first)
    */
    CoeffType getBandpassOutput() const noexcept
    {
        return z1;
    }

    /**
        Processes a sample and returns both lowpass and highpass outputs.

        This emulates the dual-filter design of the MS-20.

        @param inputSample  The input sample
        @param lpOutput     Reference to store lowpass output
        @param hpOutput     Reference to store highpass output
        @returns           The main output (depends on current mode)
    */
    SampleType processDualSample (SampleType inputSample, CoeffType& lpOutput, CoeffType& hpOutput) noexcept
    {
        const auto result = processSample (inputSample);
        lpOutput = z2 * nonLinearGain;
        hpOutput = (static_cast<CoeffType> (inputSample) - k * z1) * nonLinearGain;
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

        const auto ratio = omega / omega_c;
        const auto qFactor = k / static_cast<CoeffType> (2.0);

        if (filterMode == Mode::lowpass)
        {
            // 2-pole lowpass with resonance
            const auto denominator = std::sqrt (std::pow (static_cast<CoeffType> (1.0) - ratio * ratio, 2) +
                                              std::pow (ratio / qFactor, 2));
            return static_cast<CoeffType> (1.0) / jmax (denominator, static_cast<CoeffType> (0.001));
        }
        else
        {
            // 2-pole highpass with resonance
            const auto numerator = ratio * ratio;
            const auto denominator = std::sqrt (std::pow (static_cast<CoeffType> (1.0) - ratio * ratio, 2) +
                                              std::pow (ratio / qFactor, 2));
            return numerator / jmax (denominator, static_cast<CoeffType> (0.001));
        }
    }

private:
    /**
        Applies pre-filter saturation (input stage modeling).

        @param input  The input value
        @returns     The saturated input
    */
   CoeffType applyPreSaturation (CoeffType input) noexcept
   {
       if (saturationAmount < static_cast<CoeffType> (0.01))
           return input;

       // Asymmetric saturation characteristic of MS-20
       const auto drive = static_cast<CoeffType> (1.0) + saturationAmount * static_cast<CoeffType> (2.0);
       const auto x = input * drive;

       // Asymmetric clipping (more aggressive on positive swings)
       if (x > static_cast<CoeffType> (0.0))
       {
           return std::tanh (x * static_cast<CoeffType> (1.3)) / drive;
       }
       else
       {
           return std::tanh (x * static_cast<CoeffType> (0.9)) / drive;
       }
   }

   /**
       Applies non-linear processing in the filter loop.

       @param input  The input value
       @returns     The processed value
   */
   CoeffType applyNonLinearProcessing (CoeffType input) noexcept
   {
       // MS-20 characteristic non-linearity
       const auto threshold = static_cast<CoeffType> (0.7);
       const auto ratio = static_cast<CoeffType> (0.3);

       if (std::abs (input) > threshold)
       {
           const auto excess = std::abs (input) - threshold;
           const auto compressed = threshold + excess * ratio;
           return (input >= static_cast<CoeffType> (0.0)) ? compressed : -compressed;
       }

       return input;
   }

   /**
       Applies post-filter saturation (output stage modeling).

       @param input  The input value
       @returns     The saturated output
   */
   CoeffType applyPostSaturation (CoeffType input) noexcept
   {
       if (saturationAmount < static_cast<CoeffType> (0.01))
           return input;

       // Gentle output saturation
       const auto drive = static_cast<CoeffType> (1.0) + saturationAmount * static_cast<CoeffType> (0.5);
       return std::tanh (input * drive) / drive;
   }

    //==============================================================================
    CoeffType cutoffFreq = static_cast<CoeffType> (1000.0);
    CoeffType resonanceAmount = static_cast<CoeffType> (0.1);
    Mode filterMode = Mode::lowpass;

    // Filter coefficients from designer
    CoeffType g = static_cast<CoeffType> (0.0);               // Frequency parameter
    CoeffType k = static_cast<CoeffType> (0.0);               // Resonance parameter
    CoeffType nonLinearGain = static_cast<CoeffType> (1.0);   // Non-linear gain
    CoeffType saturationAmount = static_cast<CoeffType> (0.0); // Saturation amount

    // State variables
    CoeffType s1 = static_cast<CoeffType> (0.0);  // First integrator state
    CoeffType s2 = static_cast<CoeffType> (0.0);  // Second integrator state
    CoeffType z1 = static_cast<CoeffType> (0.0);  // First stage output
    CoeffType z2 = static_cast<CoeffType> (0.0);  // Second stage output

    //==============================================================================
    /** Updates the filter coefficients based on current parameters */
    void updateCoefficients() noexcept
    {
        const auto coeffs = FilterDesigner<CoeffType>::designKorgMs20 (cutoffFreq, resonanceAmount, this->sampleRate);

        // Extract coefficients from designer
        g = coeffs[0];
        k = coeffs[1];
        nonLinearGain = coeffs[2];
        saturationAmount = coeffs[3];
    }

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KorgMs20)
};

//==============================================================================
/** Type aliases for convenience */
using KorgMs20Float = KorgMs20<float>;      // float samples, double coefficients (default)
using KorgMs20Double = KorgMs20<double>;    // double samples, double coefficients (default)

} // namespace yup
