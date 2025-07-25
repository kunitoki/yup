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

template <typename FloatType, typename FilterType>
void calculateFilterMagnitudeResponse (FilterType& filter, Span<Complex<FloatType>> buffer, double minFreq, double maxFreq)
{
    for (std::size_t i = 0; i < buffer.size(); ++i)
    {
        // Logarithmic frequency sweep
        const double ratio = static_cast<double> (i) / (buffer.size() - 1);
        const double freq = minFreq * std::pow (maxFreq / minFreq, ratio);

        // Get complex response
        auto response = filter.getComplexResponse (freq);

        // Calculate magnitude in dB
        double magnitude = std::abs (response);
        double magnitudeDb = 20.0 * std::log10 (yup::jmax (magnitude, 1e-12));

        buffer[i] = { static_cast<FloatType> (freq), static_cast<FloatType> (magnitudeDb) };
    }
}

//==============================================================================

template <typename FloatType, typename FilterType>
void calculateFilterPhaseResponse (FilterType& filter, Span<Complex<FloatType>> buffer, double minFreq, double maxFreq)
{
    for (std::size_t i = 0; i < buffer.size(); ++i)
    {
        // Logarithmic frequency sweep
        const double ratio = static_cast<double> (i) / (buffer.size() - 1);
        const double freq = minFreq * std::pow (maxFreq / minFreq, ratio);

        // Get complex response
        auto response = filter.getComplexResponse (freq);

        // Calculate phase in degrees
        double phaseRad = std::arg (response);
        double phaseDeg = phaseRad * 180.0 / yup::MathConstants<double>::pi;

        buffer[i] = { static_cast<FloatType> (freq), static_cast<FloatType> (phaseDeg) };
    }
}

//==============================================================================

template <typename FloatType, typename FilterType>
void calculateFilterGroupDelay (FilterType& filter, Span<Complex<FloatType>> buffer, double minFreq, double maxFreq, double sampleRate)
{
    for (std::size_t i = 0; i < buffer.size(); ++i)
    {
        // Logarithmic frequency sweep
        const double ratio = static_cast<double> (i) / (buffer.size() - 1);
        const double freq = minFreq * std::pow (maxFreq / minFreq, ratio);

        // Calculate group delay (numerical derivative of phase)
        double groupDelay = 0.0;
        if (i > 0 && i < buffer.size() - 1)
        {
            const double deltaFreq = freq * 0.01; // Small frequency step
            auto responseLow = filter.getComplexResponse (freq - deltaFreq);
            auto responseHigh = filter.getComplexResponse (freq + deltaFreq);

            double phaseLow = std::arg (responseLow);
            double phaseHigh = std::arg (responseHigh);

            // Unwrap phase difference
            double phaseDiff = phaseHigh - phaseLow;
            while (phaseDiff > yup::MathConstants<double>::pi)
                phaseDiff -= 2.0 * yup::MathConstants<double>::pi;
            while (phaseDiff < -yup::MathConstants<double>::pi)
                phaseDiff += 2.0 * yup::MathConstants<double>::pi;

            groupDelay = -phaseDiff / (2.0 * deltaFreq * 2.0 * yup::MathConstants<double>::pi) * sampleRate;
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
