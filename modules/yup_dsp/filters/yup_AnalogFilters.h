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
namespace detail
{
template <std::size_t numStates, typename CoeffType>
struct LinearStepResult
{
    CoeffType output = static_cast<CoeffType> (0);
    std::array<CoeffType, numStates> state {};
};

template <std::size_t numStates, typename CoeffType, typename StepFunction>
Complex<CoeffType> getLinearizedComplexResponse (
    StepFunction&& stepFunction,
    CoeffType frequency,
    double sampleRate) noexcept
{
    using ComplexType = Complex<CoeffType>;
    using State = std::array<CoeffType, numStates>;

    const auto zeroStep = stepFunction (static_cast<CoeffType> (0), State {});
    const auto inputStep = stepFunction (static_cast<CoeffType> (1), State {});

    std::array<std::array<CoeffType, numStates>, numStates> stateMatrix {};
    std::array<CoeffType, numStates> inputVector {};
    std::array<CoeffType, numStates> outputVector {};

    for (std::size_t row = 0; row < numStates; ++row)
        inputVector[row] = inputStep.state[row] - zeroStep.state[row];

    const auto directGain = inputStep.output - zeroStep.output;

    for (std::size_t column = 0; column < numStates; ++column)
    {
        State basis {};
        basis[column] = static_cast<CoeffType> (1);

        const auto basisStep = stepFunction (static_cast<CoeffType> (0), basis);

        for (std::size_t row = 0; row < numStates; ++row)
            stateMatrix[row][column] = basisStep.state[row] - zeroStep.state[row];

        outputVector[column] = basisStep.output - zeroStep.output;
    }

    const auto z = polar (static_cast<CoeffType> (1), frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate)));
    std::array<std::array<ComplexType, numStates>, numStates> matrix {};
    std::array<ComplexType, numStates> vector {};

    for (std::size_t row = 0; row < numStates; ++row)
    {
        for (std::size_t column = 0; column < numStates; ++column)
            matrix[row][column] = (row == column ? z : ComplexType {}) - stateMatrix[row][column];

        vector[row] = inputVector[row];
    }

    for (std::size_t pivot = 0; pivot < numStates; ++pivot)
    {
        auto pivotRow = pivot;
        auto pivotMagnitude = std::abs (matrix[pivot][pivot]);

        for (std::size_t row = pivot + 1; row < numStates; ++row)
        {
            const auto rowMagnitude = std::abs (matrix[row][pivot]);
            if (rowMagnitude > pivotMagnitude)
            {
                pivotMagnitude = rowMagnitude;
                pivotRow = row;
            }
        }

        if (pivotRow != pivot)
        {
            std::swap (matrix[pivot], matrix[pivotRow]);
            std::swap (vector[pivot], vector[pivotRow]);
        }

        if (std::abs (matrix[pivot][pivot]) <= std::numeric_limits<CoeffType>::epsilon())
            matrix[pivot][pivot] += ComplexType (std::numeric_limits<CoeffType>::epsilon());

        const auto pivotValue = matrix[pivot][pivot];

        for (std::size_t column = pivot; column < numStates; ++column)
            matrix[pivot][column] /= pivotValue;

        vector[pivot] /= pivotValue;

        for (std::size_t row = 0; row < numStates; ++row)
        {
            if (row == pivot)
                continue;

            const auto scale = matrix[row][pivot];
            if (std::abs (scale) <= std::numeric_limits<CoeffType>::epsilon())
                continue;

            for (std::size_t column = pivot; column < numStates; ++column)
                matrix[row][column] -= scale * matrix[pivot][column];

            vector[row] -= scale * vector[pivot];
        }
    }

    auto response = ComplexType (directGain);

    for (std::size_t column = 0; column < numStates; ++column)
        response += outputVector[column] * vector[column];

    return response;
}

template <typename CoeffType>
CoeffType clipAnalogResonance (CoeffType input) noexcept
{
    return std::tanh (input * static_cast<CoeffType> (0.25)) * static_cast<CoeffType> (4);
}

template <typename SampleType, typename CoeffType>
struct AnalogSaturator
{
    void setCoefficients (const AnalogSaturatorCoefficients<CoeffType>& newCoefficients) noexcept
    {
        coefficients = newCoefficients;
    }

    SampleType process (SampleType input) const noexcept
    {
        if (coefficients.drive <= static_cast<CoeffType> (0))
            return input;

        return static_cast<SampleType> (
            std::atan (static_cast<CoeffType> (input) * coefficients.preScale) * coefficients.postScale);
    }

    AnalogSaturatorCoefficients<CoeffType> coefficients;
};

template <typename CoeffType>
struct AnalogTwoPoleState
{
    CoeffType s1 = static_cast<CoeffType> (0);
    CoeffType s2 = static_cast<CoeffType> (0);

    void reset() noexcept
    {
        s1 = static_cast<CoeffType> (0);
        s2 = static_cast<CoeffType> (0);
    }
};

template <typename SampleType, typename CoeffType>
SampleType processAnalogTwoPole (
    SampleType input,
    const AnalogTwoPoleCoefficients<CoeffType>& coefficients,
    AnalogTwoPoleState<CoeffType>& state) noexcept
{
    const auto inputValue = static_cast<CoeffType> (input);
    const auto highpass = (inputValue - coefficients.r2 * state.s1 - coefficients.g * state.s1 - state.s2) * coefficients.h;
    const auto bandpass = coefficients.g * highpass + state.s1;
    const auto bandpassFeedback = clipAnalogResonance (bandpass);
    const auto lowpass = coefficients.g * bandpass + state.s2;

    state.s1 = coefficients.g * highpass + bandpassFeedback;
    state.s2 = coefficients.g * bandpassFeedback + lowpass;

    const auto output = (coefficients.lowOut * lowpass + coefficients.bandOut * bandpass + coefficients.highOut * highpass)
                      * coefficients.gainCorrection;

    return static_cast<SampleType> (output);
}

template <typename CoeffType>
Complex<CoeffType> getAnalogTwoPoleComplexResponse (
    const AnalogTwoPoleCoefficients<CoeffType>& coefficients,
    CoeffType frequency,
    double sampleRate) noexcept
{
    using ComplexType = Complex<CoeffType>;

    const auto highpassState1 = -coefficients.h * (coefficients.r2 + coefficients.g);
    const auto highpassState2 = -coefficients.h;
    const auto highpassInput = coefficients.h;

    const auto bandpassState1 = coefficients.g * highpassState1 + static_cast<CoeffType> (1);
    const auto bandpassState2 = coefficients.g * highpassState2;
    const auto bandpassInput = coefficients.g * highpassInput;

    const auto lowpassState1 = coefficients.g * bandpassState1;
    const auto lowpassState2 = coefficients.g * bandpassState2 + static_cast<CoeffType> (1);
    const auto lowpassInput = coefficients.g * bandpassInput;

    const auto state11 = coefficients.g * highpassState1 + bandpassState1;
    const auto state12 = coefficients.g * highpassState2 + bandpassState2;
    const auto state1Input = coefficients.g * highpassInput + bandpassInput;

    const auto state21 = coefficients.g * bandpassState1 + lowpassState1;
    const auto state22 = coefficients.g * bandpassState2 + lowpassState2;
    const auto state2Input = coefficients.g * bandpassInput + lowpassInput;

    const auto outputState1 = (coefficients.lowOut * lowpassState1 + coefficients.bandOut * bandpassState1 + coefficients.highOut * highpassState1)
                            * coefficients.gainCorrection;
    const auto outputState2 = (coefficients.lowOut * lowpassState2 + coefficients.bandOut * bandpassState2 + coefficients.highOut * highpassState2)
                            * coefficients.gainCorrection;
    const auto outputInput = (coefficients.lowOut * lowpassInput + coefficients.bandOut * bandpassInput + coefficients.highOut * highpassInput)
                           * coefficients.gainCorrection;

    const auto z = polar (static_cast<CoeffType> (1), frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate)));
    const auto determinant = (z - state11) * (z - state22) - state12 * state21;
    const auto responseState1 = ((z - state22) * state1Input + state12 * state2Input) / determinant;
    const auto responseState2 = (state21 * state1Input + (z - state11) * state2Input) / determinant;

    return ComplexType (outputInput) + outputState1 * responseState1 + outputState2 * responseState2;
}

template <typename CoeffType>
CoeffType getAnalogOnePoleLowPassOutput (CoeffType input, CoeffType state, const AnalogOnePoleCoefficients<CoeffType>& coefficients) noexcept
{
    return coefficients.alpha * input + (static_cast<CoeffType> (1) - coefficients.alpha) * state;
}

template <typename CoeffType>
CoeffType getAnalogOnePoleHighPassOutput (CoeffType input, CoeffType state, const AnalogOnePoleCoefficients<CoeffType>& coefficients) noexcept
{
    return (static_cast<CoeffType> (1) - coefficients.alpha) * (input - state);
}

template <typename CoeffType>
CoeffType getAnalogOnePoleNextState (CoeffType input, CoeffType state, const AnalogOnePoleCoefficients<CoeffType>& coefficients) noexcept
{
    return static_cast<CoeffType> (2) * coefficients.alpha * input
         + (static_cast<CoeffType> (1) - static_cast<CoeffType> (2) * coefficients.alpha) * state;
}

template <typename CoeffType>
Complex<CoeffType> getKorg35ComplexResponse (
    FilterModeType filterMode,
    const AnalogKorg35Coefficients<CoeffType>& coefficients,
    CoeffType frequency,
    double sampleRate) noexcept
{
    return getLinearizedComplexResponse<3, CoeffType> (
        [filterMode, &coefficients] (CoeffType input, std::array<CoeffType, 3> state) noexcept
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
        sampleRate);
}

template <typename CoeffType>
Complex<CoeffType> getMoogLadderComplexResponse (
    const AnalogMoogLadderCoefficients<CoeffType>& coefficients,
    CoeffType frequency,
    double sampleRate) noexcept
{
    return getLinearizedComplexResponse<4, CoeffType> (
        [&coefficients] (CoeffType input, std::array<CoeffType, 4> state) noexcept
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
        sampleRate);
}

template <typename CoeffType>
Complex<CoeffType> getRolandDiodeComplexResponse (
    const AnalogRolandDiodeCoefficients<CoeffType>& coefficients,
    CoeffType frequency,
    double sampleRate) noexcept
{
    return getLinearizedComplexResponse<5, CoeffType> (
        [&coefficients] (CoeffType input, std::array<CoeffType, 5> state) noexcept
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
        sampleRate);
}

template <typename CoeffType>
struct AnalogOnePoleState
{
    CoeffType z1 = static_cast<CoeffType> (0);

    void reset() noexcept
    {
        z1 = static_cast<CoeffType> (0);
    }

    CoeffType processLowPass (CoeffType input, const AnalogOnePoleCoefficients<CoeffType>& coefficients) noexcept
    {
        const auto v = (input - z1) * coefficients.alpha;
        const auto output = v + z1;
        z1 = output + v;

        return output;
    }

    CoeffType processHighPass (CoeffType input, const AnalogOnePoleCoefficients<CoeffType>& coefficients) noexcept
    {
        return input - processLowPass (input, coefficients);
    }

    CoeffType getFeedbackOutput (const AnalogOnePoleCoefficients<CoeffType>& coefficients) const noexcept
    {
        return coefficients.beta * z1;
    }
};

} // namespace detail

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

        auto output = detail::processAnalogTwoPole (input, coefficients, state);
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
        return detail::getAnalogTwoPoleComplexResponse (coefficients, frequency, this->sampleRate);
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
    detail::AnalogTwoPoleState<CoeffType> state;
    detail::AnalogSaturator<SampleType, CoeffType> saturator;

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
            output = detail::processAnalogTwoPole (output, coefficients.formants[i], states[i]);

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
            response *= detail::getAnalogTwoPoleComplexResponse (formant, frequency, this->sampleRate);

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
    std::array<detail::AnalogTwoPoleState<CoeffType>, 3> states;
    detail::AnalogSaturator<SampleType, CoeffType> saturator;

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
        u = detail::clipAnalogResonance (u);

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
        return detail::getKorg35ComplexResponse (filterMode, coefficients, frequency, this->sampleRate);
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
    std::array<detail::AnalogOnePoleState<CoeffType>, 3> poles;
    detail::AnalogSaturator<SampleType, CoeffType> saturator;

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
        u = detail::clipAnalogResonance (u);

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
        return detail::getMoogLadderComplexResponse (coefficients, frequency, this->sampleRate);
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
    std::array<detail::AnalogOnePoleState<CoeffType>, 4> poles;
    detail::AnalogSaturator<SampleType, CoeffType> saturator;

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
        const auto y0 = detail::clipAnalogResonance ((x - coefficients.feedback * y5) * static_cast<CoeffType> (2))
                      * static_cast<CoeffType> (0.5);
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
        return detail::getRolandDiodeComplexResponse (coefficients, frequency, this->sampleRate);
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
    detail::AnalogSaturator<SampleType, CoeffType> saturator;

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
