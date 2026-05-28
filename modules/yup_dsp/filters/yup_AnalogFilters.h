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
    Two-pole topology-preserving analog-model filter.

    The filter supports low-pass, high-pass, two band-pass variants, peak, and
    band-stop modes. Resonance is normalized to 0..1 to match the analog-model
    designs.
*/
template <typename SampleType, typename CoeffType = double>
class AnalogTwoPoleFilter : public FilterBase<SampleType, CoeffType>
{
public:
    AnalogTwoPoleFilter()
    {
        setParameters (FilterMode::lowpass, static_cast<CoeffType> (1000), static_cast<CoeffType> (0), static_cast<CoeffType> (0), this->sampleRate);
    }

    explicit AnalogTwoPoleFilter (FilterModeType mode)
    {
        setParameters (mode, static_cast<CoeffType> (1000), static_cast<CoeffType> (0), static_cast<CoeffType> (0), this->sampleRate);
    }

    /** Sets all filter parameters. */
    void setParameters (
        FilterModeType mode,
        CoeffType frequency,
        CoeffType normalizedResonance,
        CoeffType saturation,
        double sampleRate) noexcept
    {
        mode = resolveFilterMode (mode, getSupportedModes());

        if (filterMode != mode
            || ! approximatelyEqual (centerFreq, frequency)
            || ! approximatelyEqual (resonance, normalizedResonance)
            || ! approximatelyEqual (drive, saturation)
            || ! approximatelyEqual (this->sampleRate, sampleRate))
        {
            filterMode = mode;
            centerFreq = frequency;
            resonance = normalizedResonance;
            drive = saturation;
            this->sampleRate = sampleRate;
            updateCoefficients();
        }
    }

    FilterModeType getSupportedModes() const noexcept override
    {
        return FilterMode::lowpass | FilterMode::highpass | FilterMode::bandpassCsg | FilterMode::bandpassCpg | FilterMode::bandstop | FilterMode::peak;
    }

    void setSignalRange (CoeffType range) noexcept
    {
        jassert (range > static_cast<CoeffType> (0));

        signalRange = jmax (range, std::numeric_limits<CoeffType>::min());
        inverseSignalRange = static_cast<CoeffType> (1) / signalRange;
    }

    CoeffType getSignalRange() const noexcept
    {
        return signalRange;
    }

    void reset() noexcept override
    {
        state.reset();
    }

    void prepare (double sampleRate, int maximumBlockSize) override
    {
        this->sampleRate = sampleRate;
        this->maximumBlockSize = maximumBlockSize;
        updateCoefficients();
        reset();
    }

    SampleType processSample (SampleType inputSample) noexcept override
    {
        auto input = static_cast<SampleType> (static_cast<CoeffType> (inputSample) * inverseSignalRange);
        input = saturator.process (input);

        auto output = processAnalogTwoPole (input, coefficients, state);
        output = saturator.process (output);

        return static_cast<SampleType> (static_cast<CoeffType> (output) * signalRange);
    }

    void processBlock (const SampleType* inputBuffer, SampleType* outputBuffer, int numSamples) noexcept override
    {
        for (int i = 0; i < numSamples; ++i)
            outputBuffer[i] = processSample (inputBuffer[i]);
    }

    Complex<CoeffType> getComplexResponse (CoeffType frequency) const override
    {
        return getAnalogTwoPoleComplexResponse (coefficients, frequency, this->sampleRate);
    }

private:
    void updateCoefficients() noexcept
    {
        coefficients = AnalogFilterDesigner<CoeffType>::designTwoPole (filterMode, centerFreq, this->sampleRate, resonance);
        saturator.setCoefficients (AnalogFilterDesigner<CoeffType>::designSaturator (drive));
    }

    FilterModeType filterMode = FilterMode::lowpass;
    CoeffType centerFreq = static_cast<CoeffType> (1000);
    CoeffType resonance = static_cast<CoeffType> (0);
    CoeffType drive = static_cast<CoeffType> (0);
    CoeffType signalRange = static_cast<CoeffType> (1);
    CoeffType inverseSignalRange = static_cast<CoeffType> (1);
    AnalogTwoPoleCoefficients<CoeffType> coefficients;
    AnalogTwoPoleState<CoeffType> state;
    AnalogSaturator<SampleType, CoeffType> saturator;

    YUP_LEAK_DETECTOR (AnalogTwoPoleFilter)
};

//==============================================================================
/**
    Three-formant vowel filter built from analog two-pole peak sections.
*/
template <typename SampleType, typename CoeffType = double>
class AnalogVowelFilter : public FilterBase<SampleType, CoeffType>
{
public:
    AnalogVowelFilter()
    {
        setParameters (static_cast<CoeffType> (0), static_cast<CoeffType> (0), static_cast<CoeffType> (0), this->sampleRate);
    }

    /** Sets vowel, resonance, saturation, and sample-rate parameters. */
    void setParameters (CoeffType vowelPosition, CoeffType normalizedResonance, CoeffType saturation, double sampleRate) noexcept
    {
        if (! approximatelyEqual (vowel, vowelPosition)
            || ! approximatelyEqual (resonance, normalizedResonance)
            || ! approximatelyEqual (drive, saturation)
            || ! approximatelyEqual (this->sampleRate, sampleRate))
        {
            vowel = vowelPosition;
            resonance = normalizedResonance;
            drive = saturation;
            this->sampleRate = sampleRate;
            updateCoefficients();
        }
    }

    FilterModeType getSupportedModes() const noexcept override
    {
        return FilterMode::peak;
    }

    void setSignalRange (CoeffType range) noexcept
    {
        jassert (range > static_cast<CoeffType> (0));

        signalRange = jmax (range, std::numeric_limits<CoeffType>::min());
        inverseSignalRange = static_cast<CoeffType> (1) / signalRange;
    }

    CoeffType getSignalRange() const noexcept
    {
        return signalRange;
    }

    void reset() noexcept override
    {
        for (auto& formantState : states)
            formantState.reset();
    }

    void prepare (double sampleRate, int maximumBlockSize) override
    {
        this->sampleRate = sampleRate;
        this->maximumBlockSize = maximumBlockSize;
        updateCoefficients();
        reset();
    }

    SampleType processSample (SampleType inputSample) noexcept override
    {
        auto output = saturator.process (static_cast<SampleType> (static_cast<CoeffType> (inputSample) * inverseSignalRange));

        for (std::size_t i = 0; i < states.size(); ++i)
            output = processAnalogTwoPole (output, coefficients.formants[i], states[i]);

        output = static_cast<SampleType> (static_cast<CoeffType> (output) * coefficients.gainCompensation);
        output = saturator.process (output);

        return static_cast<SampleType> (static_cast<CoeffType> (output) * signalRange);
    }

    void processBlock (const SampleType* inputBuffer, SampleType* outputBuffer, int numSamples) noexcept override
    {
        for (int i = 0; i < numSamples; ++i)
            outputBuffer[i] = processSample (inputBuffer[i]);
    }

    Complex<CoeffType> getComplexResponse (CoeffType frequency) const override
    {
        auto response = Complex<CoeffType> (coefficients.gainCompensation);

        for (const auto& formant : coefficients.formants)
            response *= getAnalogTwoPoleComplexResponse (formant, frequency, this->sampleRate);

        return response;
    }

private:
    void updateCoefficients() noexcept
    {
        coefficients = AnalogFilterDesigner<CoeffType>::designVowel (vowel, this->sampleRate, resonance);
        saturator.setCoefficients (AnalogFilterDesigner<CoeffType>::designSaturator (drive));
    }

    CoeffType vowel = static_cast<CoeffType> (0);
    CoeffType resonance = static_cast<CoeffType> (0);
    CoeffType drive = static_cast<CoeffType> (0);
    CoeffType signalRange = static_cast<CoeffType> (1);
    CoeffType inverseSignalRange = static_cast<CoeffType> (1);
    AnalogVowelCoefficients<CoeffType> coefficients;
    std::array<AnalogTwoPoleState<CoeffType>, 3> states;
    AnalogSaturator<SampleType, CoeffType> saturator;

    YUP_LEAK_DETECTOR (AnalogVowelFilter)
};

//==============================================================================
/**
    Korg35-inspired analog-model filter.
*/
template <typename SampleType, typename CoeffType = double>
class AnalogKorg35Filter : public FilterBase<SampleType, CoeffType>
{
public:
    AnalogKorg35Filter()
    {
        setParameters (FilterMode::lowpass, static_cast<CoeffType> (1000), static_cast<CoeffType> (0), static_cast<CoeffType> (0), this->sampleRate);
    }

    explicit AnalogKorg35Filter (FilterModeType mode)
    {
        setParameters (mode, static_cast<CoeffType> (1000), static_cast<CoeffType> (0), static_cast<CoeffType> (0), this->sampleRate);
    }

    /** Sets all filter parameters. */
    void setParameters (FilterModeType mode, CoeffType frequency, CoeffType normalizedResonance, CoeffType saturation, double sampleRate) noexcept
    {
        mode = resolveFilterMode (mode, getSupportedModes());

        if (filterMode != mode
            || ! approximatelyEqual (centerFreq, frequency)
            || ! approximatelyEqual (resonance, normalizedResonance)
            || ! approximatelyEqual (drive, saturation)
            || ! approximatelyEqual (this->sampleRate, sampleRate))
        {
            filterMode = mode;
            centerFreq = frequency;
            resonance = normalizedResonance;
            drive = saturation;
            this->sampleRate = sampleRate;
            updateCoefficients();
        }
    }

    FilterModeType getSupportedModes() const noexcept override
    {
        return FilterMode::lowpass | FilterMode::bandpassCsg | FilterMode::highpass;
    }

    void setSignalRange (CoeffType range) noexcept
    {
        jassert (range > static_cast<CoeffType> (0));

        signalRange = jmax (range, std::numeric_limits<CoeffType>::min());
        inverseSignalRange = static_cast<CoeffType> (1) / signalRange;
    }

    void reset() noexcept override
    {
        for (auto& pole : poles)
            pole.reset();
    }

    void prepare (double sampleRate, int maximumBlockSize) override
    {
        this->sampleRate = sampleRate;
        this->maximumBlockSize = maximumBlockSize;
        updateCoefficients();
        reset();
    }

    SampleType processSample (SampleType inputSample) noexcept override
    {
        auto input = saturator.process (static_cast<SampleType> (static_cast<CoeffType> (inputSample) * inverseSignalRange));
        const auto inputValue = static_cast<CoeffType> (input);
        const auto firstPole = filterMode.test (FilterMode::highpass)
                                 ? poles[0].processHighPass (inputValue, coefficients.poles[0])
                                 : poles[0].processLowPass (inputValue, coefficients.poles[0]);
        const auto feedback = poles[2].getFeedbackOutput (coefficients.poles[2])
                            + poles[1].getFeedbackOutput (coefficients.poles[1]);
        auto u = coefficients.alpha0 * (firstPole + feedback);
        u = clipAnalogResonance (u);

        auto output = coefficients.feedback * u;

        if (filterMode.test (FilterMode::lowpass))
        {
            output = coefficients.feedback * poles[1].processLowPass (u, coefficients.poles[1]);
            poles[2].processHighPass (output, coefficients.poles[2]);
        }
        else if (filterMode.test (FilterMode::bandpass))
        {
            output = poles[1].processLowPass (poles[2].processHighPass (output, coefficients.poles[2]), coefficients.poles[1]);
        }
        else
        {
            poles[1].processLowPass (poles[2].processHighPass (output, coefficients.poles[2]), coefficients.poles[1]);
        }

        output *= coefficients.gainCorrection;

        return static_cast<SampleType> (saturator.process (static_cast<SampleType> (output)) * signalRange);
    }

    void processBlock (const SampleType* inputBuffer, SampleType* outputBuffer, int numSamples) noexcept override
    {
        for (int i = 0; i < numSamples; ++i)
            outputBuffer[i] = processSample (inputBuffer[i]);
    }

    Complex<CoeffType> getComplexResponse (CoeffType frequency) const override
    {
        return getLinearizedComplexResponse<3, CoeffType> (
            [this] (CoeffType input, std::array<CoeffType, 3> state)
        {
            LinearStepResult<3, CoeffType> result;

            const auto firstPole = filterMode.test (FilterMode::highpass)
                                     ? getAnalogOnePoleHighPassOutput (input, state[0], coefficients.poles[0])
                                     : getAnalogOnePoleLowPassOutput (input, state[0], coefficients.poles[0]);
            const auto feedback = coefficients.poles[2].beta * state[2] + coefficients.poles[1].beta * state[1];
            const auto u = coefficients.alpha0 * (firstPole + feedback);
            const auto feedbackInput = coefficients.feedback * u;

            result.state[0] = getAnalogOnePoleNextState (input, state[0], coefficients.poles[0]);

            if (filterMode.test (FilterMode::lowpass))
            {
                const auto pole2Output = getAnalogOnePoleLowPassOutput (u, state[1], coefficients.poles[1]);
                const auto output = coefficients.feedback * pole2Output;

                result.state[1] = getAnalogOnePoleNextState (u, state[1], coefficients.poles[1]);
                result.state[2] = getAnalogOnePoleNextState (output, state[2], coefficients.poles[2]);
                result.output = output * coefficients.gainCorrection;
            }
            else
            {
                const auto pole3Output = getAnalogOnePoleHighPassOutput (feedbackInput, state[2], coefficients.poles[2]);
                const auto pole2Output = getAnalogOnePoleLowPassOutput (pole3Output, state[1], coefficients.poles[1]);

                result.state[1] = getAnalogOnePoleNextState (pole3Output, state[1], coefficients.poles[1]);
                result.state[2] = getAnalogOnePoleNextState (feedbackInput, state[2], coefficients.poles[2]);
                result.output = (filterMode.test (FilterMode::bandpass) ? pole2Output : feedbackInput) * coefficients.gainCorrection;
            }

            return result;
        },
            frequency,
            this->sampleRate);
    }

private:
    void updateCoefficients() noexcept
    {
        coefficients = AnalogFilterDesigner<CoeffType>::designKorg35 (filterMode, centerFreq, this->sampleRate, resonance, drive);
        saturator.setCoefficients (AnalogFilterDesigner<CoeffType>::designSaturator (drive));
    }

    FilterModeType filterMode = FilterMode::lowpass;
    CoeffType centerFreq = static_cast<CoeffType> (1000);
    CoeffType resonance = static_cast<CoeffType> (0);
    CoeffType drive = static_cast<CoeffType> (0);
    CoeffType signalRange = static_cast<CoeffType> (1);
    CoeffType inverseSignalRange = static_cast<CoeffType> (1);
    AnalogKorg35Coefficients<CoeffType> coefficients;
    std::array<AnalogOnePoleState<CoeffType>, 3> poles;
    AnalogSaturator<SampleType, CoeffType> saturator;

    YUP_LEAK_DETECTOR (AnalogKorg35Filter)
};

//==============================================================================
/**
    Four-pole Moog ladder-style analog-model filter.
*/
template <typename SampleType, typename CoeffType = double>
class AnalogMoogLadderFilter : public FilterBase<SampleType, CoeffType>
{
public:
    AnalogMoogLadderFilter()
    {
        setParameters (AnalogMoogLadderMode::lowpass24, static_cast<CoeffType> (1000), static_cast<CoeffType> (0), static_cast<CoeffType> (0), this->sampleRate);
    }

    /** Sets all filter parameters. */
    void setParameters (AnalogMoogLadderMode newMode, CoeffType frequency, CoeffType normalizedResonance, CoeffType saturation, double sampleRate) noexcept
    {
        if (mode != newMode
            || ! approximatelyEqual (centerFreq, frequency)
            || ! approximatelyEqual (resonance, normalizedResonance)
            || ! approximatelyEqual (drive, saturation)
            || ! approximatelyEqual (this->sampleRate, sampleRate))
        {
            mode = newMode;
            centerFreq = frequency;
            resonance = normalizedResonance;
            drive = saturation;
            this->sampleRate = sampleRate;
            updateCoefficients();
        }
    }

    FilterModeType getSupportedModes() const noexcept override
    {
        return FilterMode::lowpass | FilterMode::highpass | FilterMode::bandpassCsg;
    }

    void setSignalRange (CoeffType range) noexcept
    {
        jassert (range > static_cast<CoeffType> (0));

        signalRange = jmax (range, std::numeric_limits<CoeffType>::min());
        inverseSignalRange = static_cast<CoeffType> (1) / signalRange;
    }

    void reset() noexcept override
    {
        for (auto& pole : poles)
            pole.reset();
    }

    void prepare (double sampleRate, int maximumBlockSize) override
    {
        this->sampleRate = sampleRate;
        this->maximumBlockSize = maximumBlockSize;
        updateCoefficients();
        reset();
    }

    SampleType processSample (SampleType inputSample) noexcept override
    {
        auto input = saturator.process (static_cast<SampleType> (static_cast<CoeffType> (inputSample) * inverseSignalRange));
        const auto feedback = poles[0].getFeedbackOutput (coefficients.poles[0])
                            + poles[1].getFeedbackOutput (coefficients.poles[1])
                            + poles[2].getFeedbackOutput (coefficients.poles[2])
                            + poles[3].getFeedbackOutput (coefficients.poles[3]);
        auto u = (static_cast<CoeffType> (input) - coefficients.feedback * feedback) * coefficients.alpha0;
        u = clipAnalogResonance (u);

        const auto y1 = poles[0].processLowPass (u, coefficients.poles[0]);
        const auto y2 = poles[1].processLowPass (y1, coefficients.poles[1]);
        const auto y3 = poles[2].processLowPass (y2, coefficients.poles[2]);
        const auto y4 = poles[3].processLowPass (y3, coefficients.poles[3]);
        const auto output = (coefficients.outputs[0] * u
                             + coefficients.outputs[1] * y1
                             + coefficients.outputs[2] * y2
                             + coefficients.outputs[3] * y3
                             + coefficients.outputs[4] * y4)
                          * coefficients.gainCorrection;

        return static_cast<SampleType> (saturator.process (static_cast<SampleType> (output)) * signalRange);
    }

    void processBlock (const SampleType* inputBuffer, SampleType* outputBuffer, int numSamples) noexcept override
    {
        for (int i = 0; i < numSamples; ++i)
            outputBuffer[i] = processSample (inputBuffer[i]);
    }

    Complex<CoeffType> getComplexResponse (CoeffType frequency) const override
    {
        return getLinearizedComplexResponse<4, CoeffType> (
            [this] (CoeffType input, std::array<CoeffType, 4> state) noexcept
        {
            LinearStepResult<4, CoeffType> result;

            const auto feedback = coefficients.poles[0].beta * state[0]
                                + coefficients.poles[1].beta * state[1]
                                + coefficients.poles[2].beta * state[2]
                                + coefficients.poles[3].beta * state[3];
            const auto u = (input - coefficients.feedback * feedback) * coefficients.alpha0;
            const auto y1 = getAnalogOnePoleLowPassOutput (u, state[0], coefficients.poles[0]);
            const auto y2 = getAnalogOnePoleLowPassOutput (y1, state[1], coefficients.poles[1]);
            const auto y3 = getAnalogOnePoleLowPassOutput (y2, state[2], coefficients.poles[2]);
            const auto y4 = getAnalogOnePoleLowPassOutput (y3, state[3], coefficients.poles[3]);

            result.state[0] = getAnalogOnePoleNextState (u, state[0], coefficients.poles[0]);
            result.state[1] = getAnalogOnePoleNextState (y1, state[1], coefficients.poles[1]);
            result.state[2] = getAnalogOnePoleNextState (y2, state[2], coefficients.poles[2]);
            result.state[3] = getAnalogOnePoleNextState (y3, state[3], coefficients.poles[3]);
            result.output = (coefficients.outputs[0] * u
                             + coefficients.outputs[1] * y1
                             + coefficients.outputs[2] * y2
                             + coefficients.outputs[3] * y3
                             + coefficients.outputs[4] * y4)
                          * coefficients.gainCorrection;

            return result;
        },
            frequency,
            this->sampleRate);
    }

private:
    void updateCoefficients() noexcept
    {
        coefficients = AnalogFilterDesigner<CoeffType>::designMoogLadder (mode, centerFreq, this->sampleRate, resonance, drive);
        saturator.setCoefficients (AnalogFilterDesigner<CoeffType>::designSaturator (drive));
    }

    AnalogMoogLadderMode mode = AnalogMoogLadderMode::lowpass24;
    CoeffType centerFreq = static_cast<CoeffType> (1000);
    CoeffType resonance = static_cast<CoeffType> (0);
    CoeffType drive = static_cast<CoeffType> (0);
    CoeffType signalRange = static_cast<CoeffType> (1);
    CoeffType inverseSignalRange = static_cast<CoeffType> (1);
    AnalogMoogLadderCoefficients<CoeffType> coefficients;
    std::array<AnalogOnePoleState<CoeffType>, 4> poles;
    AnalogSaturator<SampleType, CoeffType> saturator;

    YUP_LEAK_DETECTOR (AnalogMoogLadderFilter)
};

//==============================================================================
/**
    Roland diode-ladder-inspired four-pole low-pass filter.
*/
template <typename SampleType, typename CoeffType = double>
class AnalogRolandDiodeFilter : public FilterBase<SampleType, CoeffType>
{
public:
    AnalogRolandDiodeFilter()
    {
        setParameters (static_cast<CoeffType> (1000), static_cast<CoeffType> (0), static_cast<CoeffType> (0), this->sampleRate);
    }

    /** Sets all filter parameters. */
    void setParameters (CoeffType frequency, CoeffType normalizedResonance, CoeffType saturation, double sampleRate) noexcept
    {
        if (! approximatelyEqual (centerFreq, frequency)
            || ! approximatelyEqual (resonance, normalizedResonance)
            || ! approximatelyEqual (drive, saturation)
            || ! approximatelyEqual (this->sampleRate, sampleRate))
        {
            centerFreq = frequency;
            resonance = normalizedResonance;
            drive = saturation;
            this->sampleRate = sampleRate;
            updateCoefficients();
        }
    }

    FilterModeType getSupportedModes() const noexcept override
    {
        return FilterMode::lowpass;
    }

    void setSignalRange (CoeffType range) noexcept
    {
        jassert (range > static_cast<CoeffType> (0));

        signalRange = jmax (range, std::numeric_limits<CoeffType>::min());
        inverseSignalRange = static_cast<CoeffType> (1) / signalRange;
    }

    void reset() noexcept override
    {
        z.fill (static_cast<CoeffType> (0));
    }

    void prepare (double sampleRate, int maximumBlockSize) override
    {
        this->sampleRate = sampleRate;
        this->maximumBlockSize = maximumBlockSize;
        updateCoefficients();
        reset();
    }

    SampleType processSample (SampleType inputSample) noexcept override
    {
        auto input = saturator.process (static_cast<SampleType> (static_cast<CoeffType> (inputSample) * inverseSignalRange));
        const auto x = static_cast<CoeffType> (input);
        auto s0 = (z[0] * coefficients.a2 * coefficients.a
                   + z[1] * coefficients.a2 * coefficients.b
                   + z[2] * (coefficients.b2 - static_cast<CoeffType> (2) * coefficients.a2) * coefficients.a
                   + z[3] * (coefficients.b2 - static_cast<CoeffType> (3) * coefficients.a2) * coefficients.b)
                * coefficients.c;
        const auto s = coefficients.highpassB * s0 - z[4];

        auto y5 = (coefficients.g * x + s) * coefficients.fg;
        const auto y0 = clipAnalogResonance ((x - coefficients.feedback * y5) * static_cast<CoeffType> (2)) * static_cast<CoeffType> (0.5);
        y5 = coefficients.g * y0 + s;

        const auto y4 = coefficients.g0 * y0 + s0;
        const auto y3 = (coefficients.b * y4 - z[3]) * coefficients.aInv;
        const auto y2 = (coefficients.b * y3 - coefficients.a * y4 - z[2]) * coefficients.aInv;
        const auto y1 = (coefficients.b * y2 - coefficients.a * y3 - z[1]) * coefficients.aInv;

        z[0] += static_cast<CoeffType> (4) * coefficients.a * (y0 - y1 + y2);
        z[1] += static_cast<CoeffType> (2) * coefficients.a * (y1 - static_cast<CoeffType> (2) * y2 + y3);
        z[2] += static_cast<CoeffType> (2) * coefficients.a * (y2 - static_cast<CoeffType> (2) * y3 + y4);
        z[3] += static_cast<CoeffType> (2) * coefficients.a * (y3 - static_cast<CoeffType> (2) * y4);
        z[4] = coefficients.highpassB * y4 + coefficients.highpassA * y5;

        const auto output = y4 * coefficients.gainCorrection;

        return static_cast<SampleType> (saturator.process (static_cast<SampleType> (output)) * signalRange);
    }

    void processBlock (const SampleType* inputBuffer, SampleType* outputBuffer, int numSamples) noexcept override
    {
        for (int i = 0; i < numSamples; ++i)
            outputBuffer[i] = processSample (inputBuffer[i]);
    }

    Complex<CoeffType> getComplexResponse (CoeffType frequency) const override
    {
        return getLinearizedComplexResponse<5, CoeffType> (
            [this] (CoeffType input, std::array<CoeffType, 5> state) noexcept
        {
            LinearStepResult<5, CoeffType> result;

            const auto s0 = (state[0] * coefficients.a2 * coefficients.a
                             + state[1] * coefficients.a2 * coefficients.b
                             + state[2] * (coefficients.b2 - static_cast<CoeffType> (2) * coefficients.a2) * coefficients.a
                             + state[3] * (coefficients.b2 - static_cast<CoeffType> (3) * coefficients.a2) * coefficients.b)
                          * coefficients.c;
            const auto s = coefficients.highpassB * s0 - state[4];
            auto y5 = (coefficients.g * input + s) * coefficients.fg;
            const auto y0 = input - coefficients.feedback * y5;
            y5 = coefficients.g * y0 + s;

            const auto y4 = coefficients.g0 * y0 + s0;
            const auto y3 = (coefficients.b * y4 - state[3]) * coefficients.aInv;
            const auto y2 = (coefficients.b * y3 - coefficients.a * y4 - state[2]) * coefficients.aInv;
            const auto y1 = (coefficients.b * y2 - coefficients.a * y3 - state[1]) * coefficients.aInv;

            result.state[0] = state[0] + static_cast<CoeffType> (4) * coefficients.a * (y0 - y1 + y2);
            result.state[1] = state[1] + static_cast<CoeffType> (2) * coefficients.a * (y1 - static_cast<CoeffType> (2) * y2 + y3);
            result.state[2] = state[2] + static_cast<CoeffType> (2) * coefficients.a * (y2 - static_cast<CoeffType> (2) * y3 + y4);
            result.state[3] = state[3] + static_cast<CoeffType> (2) * coefficients.a * (y3 - static_cast<CoeffType> (2) * y4);
            result.state[4] = coefficients.highpassB * y4 + coefficients.highpassA * y5;
            result.output = y4 * coefficients.gainCorrection;

            return result;
        },
            frequency,
            this->sampleRate);
    }

private:
    void updateCoefficients() noexcept
    {
        coefficients = AnalogFilterDesigner<CoeffType>::designRolandDiode (centerFreq, this->sampleRate, resonance, drive);
        saturator.setCoefficients (AnalogFilterDesigner<CoeffType>::designSaturator (drive));
    }

    CoeffType centerFreq = static_cast<CoeffType> (1000);
    CoeffType resonance = static_cast<CoeffType> (0);
    CoeffType drive = static_cast<CoeffType> (0);
    CoeffType signalRange = static_cast<CoeffType> (1);
    CoeffType inverseSignalRange = static_cast<CoeffType> (1);
    AnalogRolandDiodeCoefficients<CoeffType> coefficients;
    std::array<CoeffType, 5> z {};
    AnalogSaturator<SampleType, CoeffType> saturator;

    YUP_LEAK_DETECTOR (AnalogRolandDiodeFilter)
};

//==============================================================================
using AnalogTwoPoleFilterFloat = AnalogTwoPoleFilter<float>;
using AnalogTwoPoleFilterDouble = AnalogTwoPoleFilter<double>;
using AnalogVowelFilterFloat = AnalogVowelFilter<float>;
using AnalogVowelFilterDouble = AnalogVowelFilter<double>;
using AnalogKorg35FilterFloat = AnalogKorg35Filter<float>;
using AnalogKorg35FilterDouble = AnalogKorg35Filter<double>;
using AnalogMoogLadderFilterFloat = AnalogMoogLadderFilter<float>;
using AnalogMoogLadderFilterDouble = AnalogMoogLadderFilter<double>;
using AnalogRolandDiodeFilterFloat = AnalogRolandDiodeFilter<float>;
using AnalogRolandDiodeFilterDouble = AnalogRolandDiodeFilter<double>;

} // namespace yup
