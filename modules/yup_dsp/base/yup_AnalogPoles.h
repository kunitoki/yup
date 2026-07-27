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
/** State for a one-pole analog-model filter.

    The state is designed to be used with the coefficients defined in AnalogOnePoleCoefficients.
*/
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

//==============================================================================
/** State for a two-pole analog-model filter.

    The state is designed to be used with the coefficients defined in AnalogTwoPoleCoefficients.
*/
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

//==============================================================================
/** Computes the low-pass output of a one-pole analog-model filter.
 
    @param input The input sample to process
    @param state The current state of the filter
    @param coefficients The coefficients defining the filter behavior

    @return The low-pass output sample
*/
template <typename CoeffType>
CoeffType getAnalogOnePoleLowPassOutput (CoeffType input, CoeffType state, const AnalogOnePoleCoefficients<CoeffType>& coefficients) noexcept
{
    return coefficients.alpha * input + (static_cast<CoeffType> (1) - coefficients.alpha) * state;
}

/** Computes the high-pass output of a one-pole analog-model filter.
 
    @param input The input sample to process
    @param state The current state of the filter
    @param coefficients The coefficients defining the filter behavior
  
    @return The high-pass output sample
*/
template <typename CoeffType>
CoeffType getAnalogOnePoleHighPassOutput (CoeffType input, CoeffType state, const AnalogOnePoleCoefficients<CoeffType>& coefficients) noexcept
{
    return (static_cast<CoeffType> (1) - coefficients.alpha) * (input - state);
}

/** Computes the next state of a one-pole analog-model filter.
 
    @param input The input sample to process
    @param state The current state of the filter
    @param coefficients The coefficients defining the filter behavior

    @return The next state of the filter
*/
template <typename CoeffType>
CoeffType getAnalogOnePoleNextState (CoeffType input, CoeffType state, const AnalogOnePoleCoefficients<CoeffType>& coefficients) noexcept
{
    return static_cast<CoeffType> (2) * coefficients.alpha * input
         + (static_cast<CoeffType> (1) - static_cast<CoeffType> (2) * coefficients.alpha) * state;
}

//==============================================================================
/** Processes a single sample through a two-pole analog-model filter.

    This function implements the processing for a trapezoidal-integrator two-pole state-variable filter.
    The output is a combination of low-pass, high-pass, and band-pass responses based on the coefficients provided.

    @param input The input sample to process
    @param coefficients The coefficients defining the filter behavior
    @param state The state of the filter, which will be updated during processing

    @return The processed output sample
 */
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

//==============================================================================
/** Computes the complex frequency response of a two-pole analog-model filter.

    @param coefficients The coefficients defining the filter behavior
    @param frequency The frequency at which to evaluate the response
    @param sampleRate The sample rate of the system

    @return The complex frequency response
 */
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

} // namespace yup
