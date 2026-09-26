/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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
    Topology preserving transform state variable filter with eight simultaneous outputs.

    A port of Zavalishin's *The Art of VA Filter Design* State Variable Filter using
    Topology Preserving Transform. The trapezoidal integrators give a bilinear transform
    with the cutoff prewarped, and the zero-delay feedback loop is solved for the highpass
    path, so every other output follows from it in one pass.

    Compared with StateVariableFilter this adds the unity gain bandpass, band shelf,
    allpass and lowpass-minus-highpass outputs, a resonance control in 0..1 and an
    unclamped Q. Modes map onto FilterMode as follows:

    | FilterMode    | output                                   |
    |---------------|------------------------------------------|
    | lowpass       | LP                                       |
    | highpass      | HP                                       |
    | bandpassCsg   | BP, peak gain Q                          |
    | bandpassCpg   | 2 R BP, unity peak gain                  |
    | bandstop      | input - 2 R BP                           |
    | allpass       | input - 4 R BP                           |
    | peak          | input + K 2 R BP, K from the shelf gain  |

    The LP - HP output has no FilterMode and is reachable through Outputs::peak.

    @see StateVariableFilter, FilterBase
*/
template <typename SampleType, typename CoeffType = double>
class VAStateVariableFilter : public FilterBase<SampleType, CoeffType>
{
public:
    //==============================================================================
    /** Every output of one processing step. */
    struct Outputs
    {
        SampleType lowpass = 0;
        SampleType highpass = 0;
        SampleType bandpass = 0;          /**< Constant skirt gain bandpass, peak gain Q. */
        SampleType unityGainBandpass = 0; /**< Bandpass with unity gain at the cutoff. */
        SampleType bandShelf = 0;         /**< Input plus the shelf gain times the unity bandpass. */
        SampleType notch = 0;
        SampleType allpass = 0;
        SampleType peak = 0;              /**< Lowpass minus highpass. */
    };

    /** Largest resonance resonanceToQ() accepts, so that Q stays finite. */
    static constexpr CoeffType maxResonance = static_cast<CoeffType> (0.999);

    //==============================================================================
    /** Creates a lowpass at 1 kHz with a resonance of 0.5. */
    VAStateVariableFilter()
    {
        setParameters (FilterMode::lowpass, static_cast<CoeffType> (1000), resonanceToQ (static_cast<CoeffType> (0.5)), CoeffType (0), 44100.0);
    }

    /** Creates a filter in the given mode at 1 kHz with a resonance of 0.5. */
    explicit VAStateVariableFilter (FilterModeType initialMode)
    {
        setParameters (initialMode, static_cast<CoeffType> (1000), resonanceToQ (static_cast<CoeffType> (0.5)), CoeffType (0), 44100.0);
    }

    //==============================================================================
    /** Converts a resonance in 0..1 to Q as 1 / (2 (1 - resonance)), clamped at maxResonance. */
    static CoeffType resonanceToQ (CoeffType resonance) noexcept
    {
        const auto clamped = jlimit (CoeffType (0), maxResonance, resonance);

        return CoeffType (1) / (CoeffType (2) * (CoeffType (1) - clamped));
    }

    //==============================================================================
    /**
        Sets every parameter at once.

        @param mode         The output to select; composite modes resolve to a supported one.
        @param cutoffHz     Cutoff in Hz, clamped below Nyquist.
        @param q            Quality factor; the damping is R = 1 / (2 Q).
        @param shelfGainDb  Gain of the band shelf in dB; 0 dB bypasses it.
        @param sampleRate   Sample rate in Hz.
    */
    void setParameters (FilterModeType mode, CoeffType cutoffHz, CoeffType q, CoeffType shelfGainDb, double sampleRate) noexcept
    {
        mode = resolveFilterMode (mode, getSupportedModes());

        if (filterMode != mode
            || ! approximatelyEqual (cutoffFrequency, cutoffHz)
            || ! approximatelyEqual (qFactor, q)
            || ! approximatelyEqual (shelfGainDecibels, shelfGainDb)
            || ! approximatelyEqual (this->sampleRate, sampleRate))
        {
            filterMode = mode;
            cutoffFrequency = cutoffHz;
            qFactor = q;
            shelfGainDecibels = shelfGainDb;
            this->sampleRate = sampleRate;

            updateCoefficients();
        }
    }

    /** Selects the output; composite modes resolve to a supported one. */
    void setMode (FilterModeType mode) noexcept
    {
        filterMode = resolveFilterMode (mode, getSupportedModes());
    }

    /** Sets the cutoff in Hz. */
    void setCutoffFrequency (CoeffType cutoffHz) noexcept
    {
        if (approximatelyEqual (cutoffFrequency, cutoffHz))
            return;

        cutoffFrequency = cutoffHz;
        updateCoefficients();
    }

    /** Sets the cutoff as a MIDI note number, 440 Hz at 69. */
    void setCutoffPitch (CoeffType midiNote) noexcept
    {
        setCutoffFrequency (static_cast<CoeffType> (440) * std::exp2 ((midiNote - static_cast<CoeffType> (69)) / static_cast<CoeffType> (12)));
    }

    /** Sets the quality factor. */
    void setQ (CoeffType q) noexcept
    {
        if (approximatelyEqual (qFactor, q))
            return;

        qFactor = q;
        updateCoefficients();
    }

    /** Sets the resonance in 0..1, see resonanceToQ(). */
    void setResonance (CoeffType resonance) noexcept
    {
        setQ (resonanceToQ (resonance));
    }

    /** Sets the gain of the band shelf in dB. Only the peak mode reads it. */
    void setShelfGain (CoeffType shelfGainDb) noexcept
    {
        if (approximatelyEqual (shelfGainDecibels, shelfGainDb))
            return;

        shelfGainDecibels = shelfGainDb;
        updateCoefficients();
    }

    /** Returns the selected mode. */
    FilterModeType getMode() const noexcept { return filterMode; }

    /** Returns the cutoff in Hz. */
    CoeffType getCutoffFrequency() const noexcept { return cutoffFrequency; }

    /** Returns the quality factor. */
    CoeffType getQ() const noexcept { return qFactor; }

    /** Returns the band shelf gain in dB. */
    CoeffType getShelfGain() const noexcept { return shelfGainDecibels; }

    //==============================================================================
    /** @internal */
    FilterModeType getSupportedModes() const noexcept override
    {
        return FilterMode::lowpass | FilterMode::highpass | FilterMode::bandpassCsg | FilterMode::bandpassCpg
             | FilterMode::bandstop | FilterMode::allpass | FilterMode::peak;
    }

    /** @internal */
    void reset() noexcept override
    {
        s1 = CoeffType (0);
        s2 = CoeffType (0);
    }

    /** @internal */
    void prepare (double sampleRate, int maximumBlockSize) override
    {
        this->sampleRate = sampleRate;
        this->maximumBlockSize = maximumBlockSize;

        updateCoefficients();
        reset();
    }

    //==============================================================================
    /** Runs one step and returns every output. */
    Outputs processAllOutputs (SampleType inputSample) noexcept
    {
        const auto input = static_cast<CoeffType> (inputSample);

        const auto hp = (input - (CoeffType (2) * r + g) * s1 - s2) * h;
        const auto bp = g * hp + s1;
        const auto lp = g * bp + s2;
        const auto ubp = CoeffType (2) * r * bp;

        s1 = g * hp + bp;
        s2 = g * bp + lp;

        Outputs outputs;
        outputs.lowpass = static_cast<SampleType> (lp);
        outputs.highpass = static_cast<SampleType> (hp);
        outputs.bandpass = static_cast<SampleType> (bp);
        outputs.unityGainBandpass = static_cast<SampleType> (ubp);
        outputs.bandShelf = static_cast<SampleType> (input + k * ubp);
        outputs.notch = static_cast<SampleType> (input - ubp);
        outputs.allpass = static_cast<SampleType> (input - CoeffType (2) * ubp);
        outputs.peak = static_cast<SampleType> (lp - hp);
        return outputs;
    }

    /** @internal */
    SampleType processSample (SampleType inputSample) noexcept override
    {
        return select (processAllOutputs (inputSample));
    }

    /** @internal */
    void processBlock (const SampleType* inputBuffer, SampleType* outputBuffer, int numSamples) noexcept override
    {
        for (int i = 0; i < numSamples; ++i)
            outputBuffer[i] = select (processAllOutputs (inputBuffer[i]));
    }

    //==============================================================================
    /** @internal */
    Complex<CoeffType> getComplexResponse (CoeffType frequency) const override
    {
        const auto omega = frequencyToAngular (frequency, static_cast<CoeffType> (this->sampleRate));
        const Complex<CoeffType> s (CoeffType (0), std::tan (omega / CoeffType (2)) / g);
        const auto s2 = s * s;
        const auto twoRs = Complex<CoeffType> (CoeffType (2) * r) * s;
        const auto one = Complex<CoeffType> (CoeffType (1));
        const auto denominator = s2 + twoRs + one;

        if (filterMode.test (FilterMode::lowpass))
            return one / denominator;

        if (filterMode.test (FilterMode::highpass))
            return s2 / denominator;

        if (filterMode.test (FilterMode::bandpassCsg))
            return s / denominator;

        if (filterMode.test (FilterMode::bandpassCpg))
            return twoRs / denominator;

        if (filterMode.test (FilterMode::bandstop))
            return (s2 + one) / denominator;

        if (filterMode.test (FilterMode::allpass))
            return (s2 - twoRs + one) / denominator;

        if (filterMode.test (FilterMode::peak))
            return one + Complex<CoeffType> (k) * twoRs / denominator;

        return one;
    }

private:
    //==============================================================================
    SampleType select (const Outputs& outputs) const noexcept
    {
        if (filterMode.test (FilterMode::lowpass))
            return outputs.lowpass;

        if (filterMode.test (FilterMode::highpass))
            return outputs.highpass;

        if (filterMode.test (FilterMode::bandpassCsg))
            return outputs.bandpass;

        if (filterMode.test (FilterMode::bandpassCpg))
            return outputs.unityGainBandpass;

        if (filterMode.test (FilterMode::bandstop))
            return outputs.notch;

        if (filterMode.test (FilterMode::allpass))
            return outputs.allpass;

        if (filterMode.test (FilterMode::peak))
            return outputs.bandShelf;

        return outputs.lowpass;
    }

    void updateCoefficients() noexcept
    {
        const auto rate = static_cast<CoeffType> (jmax (1.0, this->sampleRate));
        const auto cutoff = jlimit (static_cast<CoeffType> (1e-3), rate * static_cast<CoeffType> (0.49), cutoffFrequency);
        const auto q = jmax (static_cast<CoeffType> (1e-3), qFactor);

        g = std::tan (MathConstants<CoeffType>::pi * cutoff / rate);
        r = CoeffType (1) / (CoeffType (2) * q);
        k = Decibels::decibelsToGain (shelfGainDecibels) - CoeffType (1);
        h = CoeffType (1) / (CoeffType (1) + CoeffType (2) * r * g + g * g);
    }

    //==============================================================================
    FilterModeType filterMode = FilterMode::lowpass;
    CoeffType cutoffFrequency = static_cast<CoeffType> (1000);
    CoeffType qFactor = CoeffType (1);
    CoeffType shelfGainDecibels = CoeffType (0);

    CoeffType g = CoeffType (0);
    CoeffType r = CoeffType (1);
    CoeffType k = CoeffType (0);
    CoeffType h = CoeffType (1);
    CoeffType s1 = CoeffType (0);
    CoeffType s2 = CoeffType (0);

    //==============================================================================
    YUP_LEAK_DETECTOR (VAStateVariableFilter)
};

//==============================================================================
/** Type aliases for convenience */
using VAStateVariableFilterFloat = VAStateVariableFilter<float>;   // float samples, double coefficients (default)
using VAStateVariableFilterDouble = VAStateVariableFilter<double>; // double samples, double coefficients (default)

} // namespace yup
