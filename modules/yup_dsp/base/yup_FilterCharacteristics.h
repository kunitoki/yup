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
    Calculate the magnitude response of a filter.

    @param filter  The filter to calculate the magnitude response of.
    @param buffer  The buffer to store the magnitude response in.
    @param minFreq The minimum frequency to calculate the response at.
    @param maxFreq The maximum frequency to calculate the response at.
*/
template <typename FloatType, typename FilterType>
void calculateFilterMagnitudeResponse (FilterType& filter, Span<Complex<FloatType>> buffer, double minFreq, double maxFreq)
{
    for (std::size_t i = 0; i < buffer.size(); ++i)
    {
        // Logarithmic frequency sweep
        const double ratio = static_cast<double> (i) / (buffer.size() - 1);
        const double freq = minFreq * std::pow (maxFreq / minFreq, ratio);

        // Get complex response
        auto magnitude = filter.getMagnitudeResponse (freq);

        // Calculate magnitude in dB
        double magnitudeDb = 20.0 * std::log10 (yup::jmax (magnitude, 1e-12));

        buffer[i] = { static_cast<FloatType> (freq), static_cast<FloatType> (magnitudeDb) };
    }
}

//==============================================================================

/**
    Calculate the phase response of a filter.

    @param filter  The filter to calculate the phase response of.
    @param buffer  The buffer to store the phase response in.
    @param minFreq The minimum frequency to calculate the response at.
    @param maxFreq The maximum frequency to calculate the response at.
*/

template <typename FloatType, typename FilterType>
void calculateFilterPhaseResponse (FilterType& filter, Span<Complex<FloatType>> buffer, double minFreq, double maxFreq)
{
    for (std::size_t i = 0; i < buffer.size(); ++i)
    {
        // Logarithmic frequency sweep
        const double ratio = static_cast<double> (i) / (buffer.size() - 1);
        const double freq = minFreq * std::pow (maxFreq / minFreq, ratio);

        // Get complex response
        auto phaseRad = filter.getPhaseResponse (freq);

        // Calculate phase in degrees
        double phaseDeg = phaseRad * 180.0 / yup::MathConstants<double>::pi;

        buffer[i] = { static_cast<FloatType> (freq), static_cast<FloatType> (phaseDeg) };
    }
}

//==============================================================================

/**
    Calculate the group delay of a filter.

    @param filter  The filter to calculate the group delay of.
    @param buffer  The buffer to store the group delay in.
    @param minFreq The minimum frequency to calculate the response at.
    @param maxFreq The maximum frequency to calculate the response at.
    @param sampleRate The sample rate of the filter.
*/
template <typename FloatType, typename FilterType>
void calculateFilterGroupDelay (FilterType& filter, Span<Complex<FloatType>> buffer, double minFreq, double maxFreq, double sampleRate)
{
    for (std::size_t i = 0; i < buffer.size(); ++i)
    {
        // Logarithmic frequency sweep
        const double ratio = static_cast<double> (i) / (buffer.size() - 1);
        const double freq = minFreq * std::pow (maxFreq / minFreq, ratio);
        const double deltaFreq = freq * 0.01; // Small frequency step

        // Calculate group delay (numerical derivative of phase)
        double groupDelay = 0.0;
        if (i > 0 && i < buffer.size() - 1)
        {
            auto phaseLow = filter.getPhaseResponse (freq - deltaFreq);
            auto phaseHigh = filter.getPhaseResponse (freq + deltaFreq);

            // Unwrap phase difference
            double phaseDiff = phaseHigh - phaseLow;
            while (phaseDiff > yup::MathConstants<double>::pi)
                phaseDiff -= yup::MathConstants<double>::twoPi;
            while (phaseDiff < -yup::MathConstants<double>::pi)
                phaseDiff += yup::MathConstants<double>::twoPi;

            groupDelay = -phaseDiff / (2.0 * deltaFreq * yup::MathConstants<double>::twoPi) * sampleRate;
        }

        buffer[i] = { static_cast<FloatType> (freq), static_cast<FloatType> (groupDelay) };
    }
}

//==============================================================================

/**
    Calculate the step response of a filter.

    @param filter  The filter to calculate the step response of.
    @param buffer  The buffer to store the step response in.
*/
template <typename FloatType, typename FilterType>
void calculateFilterStepResponse (FilterType& filter, Span<Complex<FloatType>> buffer)
{
    filter.reset();

    using SampleType = typename FilterType::SamplesType;

    for (std::size_t i = 0; i < buffer.size(); ++i)
    {
        const auto input = (i == 0) ? static_cast<SampleType> (1.0) : static_cast<SampleType> (0.0);
        const auto output = filter.processSample (input);

        buffer[i] = { static_cast<FloatType> (i), static_cast<FloatType> (output) };
    }

    filter.reset();
}

} // namespace yup
