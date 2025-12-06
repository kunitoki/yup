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

namespace yup
{

//==============================================================================

template <typename CoeffType>
FirstOrderCoefficients<CoeffType> FilterDesigner<CoeffType>::designFirstOrder (
    FilterModeType filterMode,
    CoeffType frequency,
    CoeffType gain,
    double sampleRate) noexcept
{
    const auto omega = frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate));
    const auto alpha = std::exp (-omega);

    FirstOrderCoefficients<CoeffType> coefficients;

    if (filterMode.test (FilterMode::lowpass))
    {
        coefficients.b0 = static_cast<CoeffType> (1.0) - alpha;
        coefficients.b1 = static_cast<CoeffType> (0.0);
        coefficients.a1 = -alpha;
    }
    else if (filterMode.test (FilterMode::highpass))
    {
        coefficients.b0 = (static_cast<CoeffType> (1.0) + alpha) / static_cast<CoeffType> (2.0);
        coefficients.b1 = -(static_cast<CoeffType> (1.0) + alpha) / static_cast<CoeffType> (2.0);
        coefficients.a1 = -alpha;
    }
    else if (filterMode.test (FilterMode::lowshelf))
    {
        const auto gainLinear = dbToGain (gain);
        const auto k = std::tan (omega / static_cast<CoeffType> (2.0));

        if (gain >= static_cast<CoeffType> (0.0))
        {
            const auto norm = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) + k);
            coefficients.b0 = (static_cast<CoeffType> (1.0) + gainLinear * k) * norm;
            coefficients.b1 = (gainLinear * k - static_cast<CoeffType> (1.0)) * norm;
            coefficients.a1 = (k - static_cast<CoeffType> (1.0)) * norm;
        }
        else
        {
            const auto norm = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) + k / gainLinear);
            coefficients.b0 = (static_cast<CoeffType> (1.0) + k) * norm;
            coefficients.b1 = (k - static_cast<CoeffType> (1.0)) * norm;
            coefficients.a1 = (k / gainLinear - static_cast<CoeffType> (1.0)) * norm;
        }
    }
    else if (filterMode.test (FilterMode::highshelf))
    {
        const auto A = dbToGain (gain);
        const auto k = std::tan (omega / static_cast<CoeffType> (2.0));

        if (gain >= static_cast<CoeffType> (0.0))
        {
            const auto norm = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) + k);
            coefficients.b0 = (A + k) * norm;
            coefficients.b1 = (k - A) * norm;
            coefficients.a1 = (k - static_cast<CoeffType> (1.0)) * norm;
        }
        else
        {
            const auto invA = static_cast<CoeffType> (1.0) / A;
            const auto norm = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) + k * invA);
            coefficients.b0 = (static_cast<CoeffType> (1.0) + k) * norm;
            coefficients.b1 = (k - static_cast<CoeffType> (1.0)) * norm;
            coefficients.a1 = (k * invA - static_cast<CoeffType> (1.0)) * norm;
        }
    }
    else if (filterMode.test (FilterMode::allpass))
    {
        const auto alpha = (static_cast<CoeffType> (1.0) - std::tan (omega / static_cast<CoeffType> (2.0)))
                         / (static_cast<CoeffType> (1.0) + std::tan (omega / static_cast<CoeffType> (2.0)));

        coefficients.b0 = alpha;
        coefficients.b1 = static_cast<CoeffType> (1.0);
        coefficients.a1 = alpha;
    }
    else
    {
        coefficients.b0 = static_cast<CoeffType> (1.0) - alpha;
        coefficients.b1 = static_cast<CoeffType> (0.0);
        coefficients.a1 = -alpha;
    }

    return coefficients;
}

//==============================================================================

template <typename CoeffType>
BiquadCoefficients<CoeffType> FilterDesigner<CoeffType>::designRbj (
    FilterModeType filterMode,
    CoeffType frequency,
    CoeffType q,
    CoeffType gain,
    double sampleRate) noexcept
{
    const auto omega = frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate));
    const auto cosOmega = std::cos (omega);
    const auto sinOmega = std::sin (omega);
    const auto alpha = sinOmega / (static_cast<CoeffType> (2.0) * q);
    const auto A = std::pow (static_cast<CoeffType> (10.0), gain / static_cast<CoeffType> (40.0));

    BiquadCoefficients<CoeffType> coeffs;

    if (filterMode.test (FilterMode::lowpass))
    {
        coeffs.b0 = (static_cast<CoeffType> (1.0) - cosOmega) / static_cast<CoeffType> (2.0);
        coeffs.b1 = static_cast<CoeffType> (1.0) - cosOmega;
        coeffs.b2 = (static_cast<CoeffType> (1.0) - cosOmega) / static_cast<CoeffType> (2.0);
        coeffs.a0 = static_cast<CoeffType> (1.0) + alpha;
        coeffs.a1 = static_cast<CoeffType> (-2.0) * cosOmega;
        coeffs.a2 = static_cast<CoeffType> (1.0) - alpha;
    }
    else if (filterMode.test (FilterMode::highpass))
    {
        coeffs.b0 = (static_cast<CoeffType> (1.0) + cosOmega) / static_cast<CoeffType> (2.0);
        coeffs.b1 = -(static_cast<CoeffType> (1.0) + cosOmega);
        coeffs.b2 = (static_cast<CoeffType> (1.0) + cosOmega) / static_cast<CoeffType> (2.0);
        coeffs.a0 = static_cast<CoeffType> (1.0) + alpha;
        coeffs.a1 = static_cast<CoeffType> (-2.0) * cosOmega;
        coeffs.a2 = static_cast<CoeffType> (1.0) - alpha;
    }
    else if (filterMode.test (FilterMode::bandpass))
    {
        // RBJ bandpass (constant skirt gain, peak gain = Q)
        // RBJ doesn't have a separate CPG variant, so use same as CSG
        coeffs.b0 = alpha;
        coeffs.b1 = static_cast<CoeffType> (0.0);
        coeffs.b2 = -alpha;
        coeffs.a0 = static_cast<CoeffType> (1.0) + alpha;
        coeffs.a1 = static_cast<CoeffType> (-2.0) * cosOmega;
        coeffs.a2 = static_cast<CoeffType> (1.0) - alpha;
    }
    else if (filterMode.test (FilterMode::bandstop))
    {
        coeffs.b0 = static_cast<CoeffType> (1.0);
        coeffs.b1 = static_cast<CoeffType> (-2.0) * cosOmega;
        coeffs.b2 = static_cast<CoeffType> (1.0);
        coeffs.a0 = static_cast<CoeffType> (1.0) + alpha;
        coeffs.a1 = static_cast<CoeffType> (-2.0) * cosOmega;
        coeffs.a2 = static_cast<CoeffType> (1.0) - alpha;
    }
    else if (filterMode.test (FilterMode::peak))
    {
        coeffs.b0 = static_cast<CoeffType> (1.0) + alpha * A;
        coeffs.b1 = static_cast<CoeffType> (-2.0) * cosOmega;
        coeffs.b2 = static_cast<CoeffType> (1.0) - alpha * A;
        coeffs.a0 = static_cast<CoeffType> (1.0) + alpha / A;
        coeffs.a1 = static_cast<CoeffType> (-2.0) * cosOmega;
        coeffs.a2 = static_cast<CoeffType> (1.0) - alpha / A;
    }
    else if (filterMode.test (FilterMode::lowshelf))
    {
        const auto S = static_cast<CoeffType> (1.0);
        const auto beta = std::sqrt (A) / q;

        coeffs.b0 = A * ((A + static_cast<CoeffType> (1.0)) - (A - static_cast<CoeffType> (1.0)) * cosOmega + beta * sinOmega);
        coeffs.b1 = static_cast<CoeffType> (2.0) * A * ((A - static_cast<CoeffType> (1.0)) - (A + static_cast<CoeffType> (1.0)) * cosOmega);
        coeffs.b2 = A * ((A + static_cast<CoeffType> (1.0)) - (A - static_cast<CoeffType> (1.0)) * cosOmega - beta * sinOmega);
        coeffs.a0 = (A + static_cast<CoeffType> (1.0)) + (A - static_cast<CoeffType> (1.0)) * cosOmega + beta * sinOmega;
        coeffs.a1 = static_cast<CoeffType> (-2.0) * ((A - static_cast<CoeffType> (1.0)) + (A + static_cast<CoeffType> (1.0)) * cosOmega);
        coeffs.a2 = (A + static_cast<CoeffType> (1.0)) + (A - static_cast<CoeffType> (1.0)) * cosOmega - beta * sinOmega;
    }
    else if (filterMode.test (FilterMode::highshelf))
    {
        const auto S = static_cast<CoeffType> (1.0);
        const auto beta = std::sqrt (A) / q;

        coeffs.b0 = A * ((A + static_cast<CoeffType> (1.0)) + (A - static_cast<CoeffType> (1.0)) * cosOmega + beta * sinOmega);
        coeffs.b1 = static_cast<CoeffType> (-2.0) * A * ((A - static_cast<CoeffType> (1.0)) + (A + static_cast<CoeffType> (1.0)) * cosOmega);
        coeffs.b2 = A * ((A + static_cast<CoeffType> (1.0)) + (A - static_cast<CoeffType> (1.0)) * cosOmega - beta * sinOmega);
        coeffs.a0 = (A + static_cast<CoeffType> (1.0)) - (A - static_cast<CoeffType> (1.0)) * cosOmega + beta * sinOmega;
        coeffs.a1 = static_cast<CoeffType> (2.0) * ((A - static_cast<CoeffType> (1.0)) - (A + static_cast<CoeffType> (1.0)) * cosOmega);
        coeffs.a2 = (A + static_cast<CoeffType> (1.0)) - (A - static_cast<CoeffType> (1.0)) * cosOmega - beta * sinOmega;
    }
    else if (filterMode.test (FilterMode::allpass))
    {
        coeffs.b0 = static_cast<CoeffType> (1.0) - alpha;
        coeffs.b1 = static_cast<CoeffType> (-2.0) * cosOmega;
        coeffs.b2 = static_cast<CoeffType> (1.0) + alpha;
        coeffs.a0 = static_cast<CoeffType> (1.0) + alpha;
        coeffs.a1 = static_cast<CoeffType> (-2.0) * cosOmega;
        coeffs.a2 = static_cast<CoeffType> (1.0) - alpha;
    }
    else
    {
        coeffs.b0 = (static_cast<CoeffType> (1.0) - cosOmega) / static_cast<CoeffType> (2.0);
        coeffs.b1 = static_cast<CoeffType> (1.0) - cosOmega;
        coeffs.b2 = (static_cast<CoeffType> (1.0) - cosOmega) / static_cast<CoeffType> (2.0);
        coeffs.a0 = static_cast<CoeffType> (1.0) + alpha;
        coeffs.a1 = static_cast<CoeffType> (-2.0) * cosOmega;
        coeffs.a2 = static_cast<CoeffType> (1.0) - alpha;
    }

    coeffs.normalize();
    return coeffs;
}

//==============================================================================
// Zoelzer Filter Implementations
//==============================================================================

template <typename CoeffType>
BiquadCoefficients<CoeffType> FilterDesigner<CoeffType>::designZoelzer (
    FilterModeType filterMode,
    CoeffType frequency,
    CoeffType q,
    CoeffType gain,
    double sampleRate) noexcept
{
    const auto omega = frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate));
    const auto K = std::tan (omega / static_cast<CoeffType> (2.0));
    const auto K2 = K * K;

    BiquadCoefficients<CoeffType> coeffs;

    if (filterMode.test (FilterMode::lowpass))
    {
        coeffs.b0 = K2;
        coeffs.b1 = static_cast<CoeffType> (2.0) * K2;
        coeffs.b2 = K2;
        coeffs.a0 = static_cast<CoeffType> (1.0) + K / q + K2;
        coeffs.a1 = static_cast<CoeffType> (2.0) * (K2 - static_cast<CoeffType> (1.0));
        coeffs.a2 = static_cast<CoeffType> (1.0) - K / q + K2;
    }
    else if (filterMode.test (FilterMode::highpass))
    {
        coeffs.b0 = static_cast<CoeffType> (1.0);
        coeffs.b1 = static_cast<CoeffType> (-2.0);
        coeffs.b2 = static_cast<CoeffType> (1.0);
        coeffs.a0 = static_cast<CoeffType> (1.0) + K / q + K2;
        coeffs.a1 = static_cast<CoeffType> (2.0) * (K2 - static_cast<CoeffType> (1.0));
        coeffs.a2 = static_cast<CoeffType> (1.0) - K / q + K2;
    }
    else if (filterMode.test (FilterMode::bandpassCsg))
    {
        coeffs.b0 = K;
        coeffs.b1 = static_cast<CoeffType> (0.0);
        coeffs.b2 = -K;
        coeffs.a0 = static_cast<CoeffType> (1.0) + K / q + K2;
        coeffs.a1 = static_cast<CoeffType> (2.0) * (K2 - static_cast<CoeffType> (1.0));
        coeffs.a2 = static_cast<CoeffType> (1.0) - K / q + K2;
    }
    else if (filterMode.test (FilterMode::bandpassCpg))
    {
        coeffs.b0 = K / q;
        coeffs.b1 = static_cast<CoeffType> (0.0);
        coeffs.b2 = -K / q;
        coeffs.a0 = static_cast<CoeffType> (1.0) + K / q + K2;
        coeffs.a1 = static_cast<CoeffType> (2.0) * (K2 - static_cast<CoeffType> (1.0));
        coeffs.a2 = static_cast<CoeffType> (1.0) - K / q + K2;
    }
    else if (filterMode.test (FilterMode::bandstop))
    {
        coeffs.b0 = static_cast<CoeffType> (1.0) + K2;
        coeffs.b1 = static_cast<CoeffType> (2.0) * (K2 - static_cast<CoeffType> (1.0));
        coeffs.b2 = static_cast<CoeffType> (1.0) + K2;
        coeffs.a0 = static_cast<CoeffType> (1.0) + K / q + K2;
        coeffs.a1 = static_cast<CoeffType> (2.0) * (K2 - static_cast<CoeffType> (1.0));
        coeffs.a2 = static_cast<CoeffType> (1.0) - K / q + K2;
    }
    else if (filterMode.test (FilterMode::peak))
    {
        const auto V = dbToGain (gain);

        if (gain >= static_cast<CoeffType> (0.0))
        {
            // Boost
            coeffs.b0 = static_cast<CoeffType> (1.0) + V * K / q + K2;
            coeffs.b1 = static_cast<CoeffType> (2.0) * (K2 - static_cast<CoeffType> (1.0));
            coeffs.b2 = static_cast<CoeffType> (1.0) - V * K / q + K2;
            coeffs.a0 = static_cast<CoeffType> (1.0) + K / q + K2;
            coeffs.a1 = static_cast<CoeffType> (2.0) * (K2 - static_cast<CoeffType> (1.0));
            coeffs.a2 = static_cast<CoeffType> (1.0) - K / q + K2;
        }
        else
        {
            // Cut
            coeffs.b0 = static_cast<CoeffType> (1.0) + K / q + K2;
            coeffs.b1 = static_cast<CoeffType> (2.0) * (K2 - static_cast<CoeffType> (1.0));
            coeffs.b2 = static_cast<CoeffType> (1.0) - K / q + K2;
            coeffs.a0 = static_cast<CoeffType> (1.0) + V * K / q + K2;
            coeffs.a1 = static_cast<CoeffType> (2.0) * (K2 - static_cast<CoeffType> (1.0));
            coeffs.a2 = static_cast<CoeffType> (1.0) - V * K / q + K2;
        }
    }
    else if (filterMode.test (FilterMode::lowshelf))
    {
        const auto V = dbToGain (gain);
        const auto sqrtV = std::sqrt (V);

        if (gain >= static_cast<CoeffType> (0.0))
        {
            // Boost
            coeffs.b0 = static_cast<CoeffType> (1.0) + sqrtV * K / q + V * K2;
            coeffs.b1 = static_cast<CoeffType> (2.0) * (V * K2 - static_cast<CoeffType> (1.0));
            coeffs.b2 = static_cast<CoeffType> (1.0) - sqrtV * K / q + V * K2;
            coeffs.a0 = static_cast<CoeffType> (1.0) + K / q + K2;
            coeffs.a1 = static_cast<CoeffType> (2.0) * (K2 - static_cast<CoeffType> (1.0));
            coeffs.a2 = static_cast<CoeffType> (1.0) - K / q + K2;
        }
        else
        {
            // Cut
            coeffs.b0 = static_cast<CoeffType> (1.0) + K / q + K2;
            coeffs.b1 = static_cast<CoeffType> (2.0) * (K2 - static_cast<CoeffType> (1.0));
            coeffs.b2 = static_cast<CoeffType> (1.0) - K / q + K2;
            coeffs.a0 = static_cast<CoeffType> (1.0) + sqrtV * K / q + V * K2;
            coeffs.a1 = static_cast<CoeffType> (2.0) * (V * K2 - static_cast<CoeffType> (1.0));
            coeffs.a2 = static_cast<CoeffType> (1.0) - sqrtV * K / q + V * K2;
        }
    }
    else if (filterMode.test (FilterMode::highshelf))
    {
        const auto V = dbToGain (gain);
        const auto sqrtV = std::sqrt (V);

        if (gain >= static_cast<CoeffType> (0.0))
        {
            // Boost - derived from reference comments
            coeffs.b0 = V * K2 + sqrtV * K / q + static_cast<CoeffType> (1.0);
            coeffs.b1 = static_cast<CoeffType> (2.0) * (V * K2 - static_cast<CoeffType> (1.0));
            coeffs.b2 = V * K2 - sqrtV * K / q + static_cast<CoeffType> (1.0);
            coeffs.a0 = K2 + K / q + static_cast<CoeffType> (1.0);
            coeffs.a1 = static_cast<CoeffType> (2.0) * (K2 - static_cast<CoeffType> (1.0));
            coeffs.a2 = K2 - K / q + static_cast<CoeffType> (1.0);
        }
        else
        {
            // Cut - derived from reference comments
            coeffs.b0 = K2 + K / q + static_cast<CoeffType> (1.0);
            coeffs.b1 = static_cast<CoeffType> (2.0) * (K2 - static_cast<CoeffType> (1.0));
            coeffs.b2 = K2 - K / q + static_cast<CoeffType> (1.0);
            coeffs.a0 = V * K2 + sqrtV * K / q + static_cast<CoeffType> (1.0);
            coeffs.a1 = static_cast<CoeffType> (2.0) * (V * K2 - static_cast<CoeffType> (1.0));
            coeffs.a2 = V * K2 - sqrtV * K / q + static_cast<CoeffType> (1.0);
        }
    }
    else if (filterMode.test (FilterMode::allpass))
    {
        coeffs.b0 = static_cast<CoeffType> (1.0) - K / q + K2;
        coeffs.b1 = static_cast<CoeffType> (2.0) * (K2 - static_cast<CoeffType> (1.0));
        coeffs.b2 = static_cast<CoeffType> (1.0) + K / q + K2;
        coeffs.a0 = static_cast<CoeffType> (1.0) + K / q + K2;
        coeffs.a1 = static_cast<CoeffType> (2.0) * (K2 - static_cast<CoeffType> (1.0));
        coeffs.a2 = static_cast<CoeffType> (1.0) - K / q + K2;
    }
    else
    {
        coeffs.b0 = K2;
        coeffs.b1 = static_cast<CoeffType> (2.0) * K2;
        coeffs.b2 = K2;
        coeffs.a0 = static_cast<CoeffType> (1.0) + K / q + K2;
        coeffs.a1 = static_cast<CoeffType> (2.0) * (K2 - static_cast<CoeffType> (1.0));
        coeffs.a2 = static_cast<CoeffType> (1.0) - K / q + K2;
    }

    coeffs.normalize();
    return coeffs;
}

//==============================================================================

template <typename CoeffType>
int FilterDesigner<CoeffType>::designButterworth (
    FilterModeType filterMode,
    int order,
    CoeffType frequency,
    CoeffType frequency2,
    double sampleRate,
    std::vector<BiquadCoefficients<CoeffType>>& coefficients) noexcept
{
    // Validate inputs
    jassert (order >= 2 && order <= 16);
    jassert (frequency > static_cast<CoeffType> (0.0));
    jassert (sampleRate > 0.0);

    if (filterMode.test (FilterMode::bandpass) || filterMode.test (FilterMode::bandstop))
        jassert (frequency2 > frequency);

    // Ensure order is valid (1 or power of 2) - limit to 16 for numerical stability
    order = jlimit (2, 16, nextEven (order));

    coefficients.clear();

    // Clip frequency to valid range
    frequency = yup::jlimit (static_cast<CoeffType> (0.0001 * sampleRate), static_cast<CoeffType> (0.49 * sampleRate), frequency);
    frequency2 = yup::jlimit (static_cast<CoeffType> (0.0001 * sampleRate), static_cast<CoeffType> (0.49 * sampleRate), frequency2);

    const int numStages = (order + 1) / 2;
    const CoeffType omega = static_cast<CoeffType> (2.0 * MathConstants<CoeffType>::pi * frequency / sampleRate);

    if (filterMode.test (FilterMode::lowpass) || filterMode.test (FilterMode::highpass))
    {
        // Lowpass and Highpass filters
        for (int s = 0; s < numStages; ++s)
        {
            const CoeffType d = static_cast<CoeffType> (2.0) * std::sin (((static_cast<CoeffType> (2 * (s + 1) - 1)) * MathConstants<CoeffType>::pi) / (static_cast<CoeffType> (2 * order)));

            const CoeffType beta = static_cast<CoeffType> (0.5) * ((static_cast<CoeffType> (1.0) - (d / static_cast<CoeffType> (2.0)) * std::sin (omega)) / (static_cast<CoeffType> (1.0) + (d / static_cast<CoeffType> (2.0)) * std::sin (omega)));

            const CoeffType gamma = (static_cast<CoeffType> (0.5) + beta) * std::cos (omega);

            BiquadCoefficients<CoeffType> coeffs;
            coeffs.a0 = static_cast<CoeffType> (1.0);
            coeffs.a1 = static_cast<CoeffType> (-2.0) * gamma;
            coeffs.a2 = static_cast<CoeffType> (2.0) * beta;

            if (filterMode.test (FilterMode::lowpass))
            {
                const CoeffType alpha = (static_cast<CoeffType> (0.5) + beta - gamma) / static_cast<CoeffType> (4.0);
                coeffs.b0 = static_cast<CoeffType> (2.0) * alpha;
                coeffs.b1 = static_cast<CoeffType> (4.0) * alpha;
                coeffs.b2 = static_cast<CoeffType> (2.0) * alpha;
            }
            else // highpass
            {
                const CoeffType alpha = (static_cast<CoeffType> (0.5) + beta + gamma) / static_cast<CoeffType> (4.0);
                coeffs.b0 = static_cast<CoeffType> (2.0) * alpha;
                coeffs.b1 = static_cast<CoeffType> (-4.0) * alpha;
                coeffs.b2 = static_cast<CoeffType> (2.0) * alpha;
            }

            coeffs.normalize();
            coefficients.push_back (coeffs);
        }
    }
    else if (filterMode.test (FilterMode::bandpass) || filterMode.test (FilterMode::bandstop))
    {
        // Bandpass and Bandstop filters
        const CoeffType centerFreq = std::sqrt (frequency * frequency2);
        const CoeffType omegaCenter = static_cast<CoeffType> (2.0 * MathConstants<CoeffType>::pi * centerFreq / sampleRate);
        CoeffType Q = centerFreq / (frequency2 - frequency);

        // Limit Q to prevent instability
        if (omegaCenter / Q > MathConstants<CoeffType>::pi / static_cast<CoeffType> (2.0))
        {
            Q = omegaCenter / (MathConstants<CoeffType>::pi / static_cast<CoeffType> (2.0));
        }

        // Clamp Q to reasonable range
        Q = yup::jlimit (static_cast<CoeffType> (0.08), static_cast<CoeffType> (20.0), Q);

        for (int s = 0; s < numStages; ++s)
        {
            const CoeffType dE = (static_cast<CoeffType> (2.0) * std::tan (omegaCenter / (static_cast<CoeffType> (2.0) * Q))) / std::sin (omegaCenter);
            const CoeffType Dk = static_cast<CoeffType> (2.0) * std::sin ((((static_cast<CoeffType> (2 * (s + 1))) - static_cast<CoeffType> (1.0)) * MathConstants<CoeffType>::pi) / (static_cast<CoeffType> (2 * numStages)));
            const CoeffType Ak = (static_cast<CoeffType> (1.0) + (dE / static_cast<CoeffType> (2.0)) * (dE / static_cast<CoeffType> (2.0))) / (Dk * dE / static_cast<CoeffType> (2.0));
            const CoeffType dk = std::sqrt ((dE * Dk) / (Ak + std::sqrt (Ak * Ak - static_cast<CoeffType> (1.0))));
            const CoeffType Bk = Dk * (dE / static_cast<CoeffType> (2.0)) / dk;
            const CoeffType Wk = Bk + std::sqrt (Bk * Bk - static_cast<CoeffType> (1.0));

            const CoeffType theta_k = ((s & 1) == 0)
                                        ? static_cast<CoeffType> (2.0) * std::atan ((std::tan (omegaCenter / static_cast<CoeffType> (2.0))) * Wk)
                                        : static_cast<CoeffType> (2.0) * std::atan ((std::tan (omegaCenter / static_cast<CoeffType> (2.0))) / Wk);

            const CoeffType beta = static_cast<CoeffType> (0.5) * (static_cast<CoeffType> (1.0) - (dk / static_cast<CoeffType> (2.0)) * std::sin (theta_k)) / (static_cast<CoeffType> (1.0) + (dk / static_cast<CoeffType> (2.0)) * std::sin (theta_k));

            const CoeffType gamma = (static_cast<CoeffType> (0.5) + beta) * std::cos (theta_k);

            BiquadCoefficients<CoeffType> coeffs;
            coeffs.a0 = static_cast<CoeffType> (1.0);
            coeffs.a1 = static_cast<CoeffType> (-2.0) * gamma;
            coeffs.a2 = static_cast<CoeffType> (2.0) * beta;

            if (filterMode.test (FilterMode::bandpass))
            {
                const CoeffType alpha = static_cast<CoeffType> (0.5) * (static_cast<CoeffType> (0.5) - beta) * std::sqrt (static_cast<CoeffType> (1.0) + (Wk - (static_cast<CoeffType> (1.0) / Wk)) * (Wk - (static_cast<CoeffType> (1.0) / Wk)) / (dk * dk));

                coeffs.b0 = static_cast<CoeffType> (2.0) * alpha;
                coeffs.b1 = static_cast<CoeffType> (0.0);
                coeffs.b2 = static_cast<CoeffType> (-2.0) * alpha;
            }
            else // bandstop
            {
                const CoeffType alpha = static_cast<CoeffType> (0.5) * (static_cast<CoeffType> (0.5) + beta) * ((static_cast<CoeffType> (1.0) - std::cos (theta_k)) / (static_cast<CoeffType> (1.0) - std::cos (omegaCenter)));

                coeffs.b0 = static_cast<CoeffType> (2.0) * alpha;
                coeffs.b1 = static_cast<CoeffType> (-4.0) * alpha * std::cos (omegaCenter);
                coeffs.b2 = static_cast<CoeffType> (2.0) * alpha;
            }

            coeffs.normalize();
            coefficients.push_back (coeffs);
        }
    }
    else if (filterMode.test (FilterMode::allpass))
    {
        // Allpass filters - use same structure as lowpass but with different coefficients
        for (int s = 0; s < numStages; ++s)
        {
            const CoeffType d = static_cast<CoeffType> (2.0) * std::sin (((static_cast<CoeffType> (2 * (s + 1) - 1)) * MathConstants<CoeffType>::pi) / (static_cast<CoeffType> (2 * order)));

            const CoeffType beta = static_cast<CoeffType> (0.5) * ((static_cast<CoeffType> (1.0) - (d / static_cast<CoeffType> (2.0)) * std::sin (omega)) / (static_cast<CoeffType> (1.0) + (d / static_cast<CoeffType> (2.0)) * std::sin (omega)));

            const CoeffType gamma = (static_cast<CoeffType> (0.5) + beta) * std::cos (omega);

            BiquadCoefficients<CoeffType> coeffs;
            // For allpass: numerator = reversed denominator
            coeffs.a0 = static_cast<CoeffType> (1.0);
            coeffs.a1 = static_cast<CoeffType> (-2.0) * gamma;
            coeffs.a2 = static_cast<CoeffType> (2.0) * beta;
            coeffs.b0 = static_cast<CoeffType> (2.0) * beta;
            coeffs.b1 = static_cast<CoeffType> (-2.0) * gamma;
            coeffs.b2 = static_cast<CoeffType> (1.0);

            coeffs.normalize();
            coefficients.push_back (coeffs);
        }
    }

    return static_cast<int> (coefficients.size());
}

//==============================================================================

template <typename CoeffType>
int FilterDesigner<CoeffType>::designLinkwitzRiley (
    int order,
    CoeffType crossoverFreq,
    double sampleRate,
    std::vector<BiquadCoefficients<CoeffType>>& lowCoeffs,
    std::vector<BiquadCoefficients<CoeffType>>& highCoeffs) noexcept
{
    jassert (order >= 2 && order <= 16);
    jassert ((order & 1) == 0); // Must be even
    jassert (crossoverFreq > static_cast<CoeffType> (0.0));
    jassert (sampleRate > 0.0);

    const int numStages = order / 2;

    // Clear output vectors
    lowCoeffs.clear();
    highCoeffs.clear();

    // Reserve space for two cascaded stages per biquad section
    lowCoeffs.reserve (numStages * 2);
    highCoeffs.reserve (numStages * 2);

    // Direct Linkwitz-Riley coefficient calculation matching inspiration code
    const auto omega = static_cast<CoeffType> (MathConstants<CoeffType>::twoPi * crossoverFreq / sampleRate);

    for (int stage = 0; stage < numStages; ++stage)
    {
        // Calculate pole angle for this stage (matching inspiration formula)
        const auto poleAngle = static_cast<CoeffType> ((2.0 * (stage + 1) - 1.0) * MathConstants<CoeffType>::pi / (2.0 * order));
        const auto d = static_cast<CoeffType> (2.0 * std::sin (poleAngle));

        const auto beta = static_cast<CoeffType> (0.5 * ((1.0 - (d / 2.0) * std::sin (omega)) / (1.0 + (d / 2.0) * std::sin (omega))));
        const auto gamma = static_cast<CoeffType> ((0.5 + beta) * std::cos (omega));

        // Lowpass coefficients (matching inspiration code lines 73-87)
        {
            const auto alpha = static_cast<CoeffType> ((0.5 + beta - gamma) / 4.0);

            const auto la0 = static_cast<CoeffType> (1.0);
            const auto la1 = static_cast<CoeffType> (-2.0 * gamma);
            const auto la2 = static_cast<CoeffType> (2.0 * beta);
            const auto lb0 = static_cast<CoeffType> (2.0 * alpha);
            const auto lb1 = static_cast<CoeffType> (4.0 * alpha);
            const auto lb2 = static_cast<CoeffType> (2.0 * alpha);

            BiquadCoefficients<CoeffType> lowCoeff;
            lowCoeff.a0 = la0;
            lowCoeff.a1 = la1 / la0;
            lowCoeff.a2 = la2 / la0;
            lowCoeff.b0 = lb0 / la0;
            lowCoeff.b1 = lb1 / la0;
            lowCoeff.b2 = lb2 / la0;

            // Add identical coefficients for both cascades (Linkwitz-Riley = 2x Butterworth)
            lowCoeffs.push_back (lowCoeff);
            lowCoeffs.push_back (lowCoeff);
        }

        // Highpass coefficients (matching inspiration code lines 92-107)
        {
            const auto alpha = static_cast<CoeffType> ((0.5 + beta + gamma) / 4.0);

            const auto ha0 = static_cast<CoeffType> (1.0);
            const auto ha1 = static_cast<CoeffType> (-2.0 * gamma);
            const auto ha2 = static_cast<CoeffType> (2.0 * beta);
            const auto hb0 = static_cast<CoeffType> (2.0 * alpha);
            const auto hb1 = static_cast<CoeffType> (-4.0 * alpha);
            const auto hb2 = static_cast<CoeffType> (2.0 * alpha);

            BiquadCoefficients<CoeffType> highCoeff;
            highCoeff.a0 = ha0;
            highCoeff.a1 = ha1 / ha0;
            highCoeff.a2 = ha2 / ha0;
            highCoeff.b0 = hb0 / ha0;
            highCoeff.b1 = hb1 / ha0;
            highCoeff.b2 = hb2 / ha0;

            // Add identical coefficients for both cascades (Linkwitz-Riley = 2x Butterworth)
            highCoeffs.push_back (highCoeff);
            highCoeffs.push_back (highCoeff);
        }
    }

    return static_cast<int> (lowCoeffs.size());
}

//==============================================================================
// FIR Filter Design Implementations
//==============================================================================

template <typename CoeffType>
void FilterDesigner<CoeffType>::designFIRLowpass (
    std::vector<CoeffType>& coefficients,
    int numCoefficients,
    CoeffType cutoffFreq,
    double sampleRate,
    WindowType windowType,
    CoeffType windowParameter) noexcept
{
    jassert (numCoefficients > 0);
    jassert (cutoffFreq > static_cast<CoeffType> (0.0));
    jassert (sampleRate > 0.0);
    jassert (cutoffFreq < static_cast<CoeffType> (sampleRate / 2.0));

    numCoefficients = nextOdd (numCoefficients);
    coefficients.resize (numCoefficients);

    const auto normalizedCutoff = static_cast<CoeffType> (2.0) * cutoffFreq / static_cast<CoeffType> (sampleRate);
    const int center = (numCoefficients - 1) / 2;

    // Generate ideal lowpass sinc function
    for (int i = 0; i < numCoefficients; ++i)
    {
        if (i == center)
        {
            coefficients[i] = normalizedCutoff;
        }
        else
        {
            const auto x = MathConstants<CoeffType>::pi * normalizedCutoff * static_cast<CoeffType> (i - center);
            coefficients[i] = std::sin (x) / (MathConstants<CoeffType>::pi * static_cast<CoeffType> (i - center));
        }
    }

    // Apply window function
    for (int i = 0; i < numCoefficients; ++i)
    {
        const auto windowValue = WindowFunctions<CoeffType>::getValue (windowType, i, numCoefficients, windowParameter);
        coefficients[i] *= windowValue;
    }

    // Normalization
    const auto sum = std::accumulate (coefficients.begin(), coefficients.end(), static_cast<CoeffType> (0.0));
    if (sum != static_cast<CoeffType> (0.0))
    {
        for (auto& c : coefficients)
            c /= sum;
    }
}

template <typename CoeffType>
void FilterDesigner<CoeffType>::designFIRHighpass (
    std::vector<CoeffType>& coefficients,
    int numCoefficients,
    CoeffType cutoffFreq,
    double sampleRate,
    WindowType windowType,
    CoeffType windowParameter) noexcept
{
    jassert (numCoefficients > 0);
    jassert (cutoffFreq > static_cast<CoeffType> (0.0));
    jassert (sampleRate > 0.0);
    jassert (cutoffFreq < static_cast<CoeffType> (sampleRate / 2.0));

    // Generate lowpass first
    numCoefficients = nextOdd (numCoefficients);
    designFIRLowpass (coefficients, numCoefficients, cutoffFreq, sampleRate, windowType);

    // Convert to highpass using spectral inversion
    const int center = (numCoefficients - 1) / 2;
    for (int i = 0; i < numCoefficients; ++i)
        coefficients[i] = -coefficients[i];

    // Add unit impulse at center
    coefficients[center] += static_cast<CoeffType> (1.0);

    // Normalization
    CoeffType hpi (0.0);
    for (int n = 0; n < numCoefficients; ++n)
        hpi += coefficients[n] * ((n & 1) ? static_cast<CoeffType> (-1.0) : static_cast<CoeffType> (1.0));

    if (hpi != static_cast<CoeffType> (0.0))
    {
        for (auto& c : coefficients)
            c /= hpi;
    }
}

template <typename CoeffType>
void FilterDesigner<CoeffType>::designFIRBandpass (
    std::vector<CoeffType>& coefficients,
    int numCoefficients,
    CoeffType lowCutoffFreq,
    CoeffType highCutoffFreq,
    double sampleRate,
    WindowType windowType,
    CoeffType windowParameter) noexcept
{
    jassert (numCoefficients > 0);
    jassert (lowCutoffFreq > static_cast<CoeffType> (0.0));
    jassert (highCutoffFreq > lowCutoffFreq);
    jassert (sampleRate > 0.0);
    jassert (highCutoffFreq < static_cast<CoeffType> (sampleRate / 2.0));

    numCoefficients = nextOdd (numCoefficients);
    coefficients.resize (numCoefficients);

    const auto normalizedLow = static_cast<CoeffType> (2.0) * lowCutoffFreq / static_cast<CoeffType> (sampleRate);
    const auto normalizedHigh = static_cast<CoeffType> (2.0) * highCutoffFreq / static_cast<CoeffType> (sampleRate);
    const int center = (numCoefficients - 1) / 2;

    // Generate ideal bandpass as difference of two sinc functions
    for (int i = 0; i < numCoefficients; ++i)
    {
        if (i == center)
        {
            coefficients[i] = normalizedHigh - normalizedLow;
        }
        else
        {
            const auto n = static_cast<CoeffType> (i - center);
            const auto xHigh = MathConstants<CoeffType>::pi * normalizedHigh * n;
            const auto xLow = MathConstants<CoeffType>::pi * normalizedLow * n;

            coefficients[i] = (std::sin (xHigh) - std::sin (xLow)) / (MathConstants<CoeffType>::pi * n);
        }
    }

    // Apply window function
    for (int i = 0; i < numCoefficients; ++i)
    {
        const auto windowValue = WindowFunctions<CoeffType>::getValue (windowType, i, numCoefficients, windowParameter);
        coefficients[i] *= windowValue;
    }
}

template <typename CoeffType>
void FilterDesigner<CoeffType>::designFIRBandstop (
    std::vector<CoeffType>& coefficients,
    int numCoefficients,
    CoeffType lowCutoffFreq,
    CoeffType highCutoffFreq,
    double sampleRate,
    WindowType windowType,
    CoeffType windowParameter) noexcept
{
    jassert (numCoefficients > 0);
    jassert (lowCutoffFreq > static_cast<CoeffType> (0.0));
    jassert (highCutoffFreq > lowCutoffFreq);
    jassert (sampleRate > 0.0);
    jassert (highCutoffFreq < static_cast<CoeffType> (sampleRate / 2.0));

    // Generate bandpass first
    numCoefficients = nextOdd (numCoefficients);
    designFIRBandpass (coefficients, numCoefficients, lowCutoffFreq, highCutoffFreq, sampleRate, windowType);

    // Convert to bandstop using spectral inversion
    const int center = (numCoefficients - 1) / 2;

    for (int i = 0; i < numCoefficients; ++i)
        coefficients[i] = -coefficients[i];

    // Add unit impulse at center
    coefficients[center] += static_cast<CoeffType> (1.0);
}

//==============================================================================

template class FilterDesigner<float>;
template class FilterDesigner<double>;

} // namespace yup
