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
// Butterworth Filter Design Implementation - Based on zpk approach
//==============================================================================

namespace
{
    template <typename CoeffType>
    void normalizeFrequencies (const std::vector<CoeffType>& freqs, double sampleRate,
                             ButterworthWorkspace<CoeffType>& workspace)
    {
        workspace.normalizedFreqs.clear();
        for (CoeffType f : freqs)
        {
            CoeffType w = f / (static_cast<CoeffType> (sampleRate) / static_cast<CoeffType> (2.0));
            jassert (w > static_cast<CoeffType> (0.0) && w < static_cast<CoeffType> (1.0));
            workspace.normalizedFreqs.push_back (w);
        }
    }

    template <typename CoeffType>
    void calculateAnalogPrototype (int order, ButterworthWorkspace<CoeffType>& workspace)
    {
        workspace.zpkPoles.clear();
        workspace.zpkZeros.clear();
        workspace.gain = static_cast<CoeffType> (1.0);

        for (int k = -order + 1; k < order; k += 2)
        {
            const auto angle = (static_cast<CoeffType> (k) * MathConstants<CoeffType>::pi) /
                (static_cast<CoeffType> (2) * static_cast<CoeffType> (order));

            workspace.zpkPoles.emplace_back (-Complex<CoeffType> (std::cos(angle), std::sin(angle)));
        }
    }

    template <typename CoeffType>
    void prewarpFrequencies (double sampleRate, ButterworthWorkspace<CoeffType>& workspace)
    {
        workspace.prewarpedFreqs.clear();
        for (CoeffType w : workspace.normalizedFreqs)
        {
            CoeffType warped = static_cast<CoeffType> (2.0 * sampleRate) *
                std::tan (MathConstants<CoeffType>::pi * w / static_cast<CoeffType> (2.0));
            workspace.prewarpedFreqs.push_back (warped);
        }
    }

    template <typename CoeffType>
    void frequencyTransformLowpass (ButterworthWorkspace<CoeffType>& workspace)
    {
        if (workspace.prewarpedFreqs.empty()) return;

        CoeffType wo = workspace.prewarpedFreqs[0];
        int degree = static_cast<int> (workspace.zpkPoles.size()) - static_cast<int> (workspace.zpkZeros.size());

        // Transform poles and zeros
        workspace.tempPoles1.clear();
        workspace.tempZeros1.clear();

        for (const auto& p : workspace.zpkPoles)
            workspace.tempPoles1.push_back (wo * p);
        workspace.zpkPoles = workspace.tempPoles1;

        for (const auto& z : workspace.zpkZeros)
            workspace.tempZeros1.push_back (wo * z);
        workspace.zpkZeros = workspace.tempZeros1;

        workspace.gain *= std::pow (wo, degree);
    }

    template <typename CoeffType>
    void frequencyTransformHighpass (ButterworthWorkspace<CoeffType>& workspace)
    {
        if (workspace.prewarpedFreqs.empty()) return;

        CoeffType wo = workspace.prewarpedFreqs[0];
        int degree = static_cast<int> (workspace.zpkPoles.size()) - static_cast<int> (workspace.zpkZeros.size());

        // Transform: s -> wo/s
        workspace.tempPoles1.clear();
        workspace.tempZeros1.clear();

        for (const auto& p : workspace.zpkPoles)
            workspace.tempPoles1.push_back (wo / p);
        workspace.zpkPoles = workspace.tempPoles1;

        for (const auto& z : workspace.zpkZeros)
            workspace.tempZeros1.push_back (wo / z);
        for (int i = 0; i < static_cast<int>(workspace.zpkPoles.size()); ++i)
        {
            workspace.tempZeros1.emplace_back (static_cast<CoeffType> (0.0));
        }
        workspace.zpkZeros = workspace.tempZeros1;

    }

    template <typename CoeffType>
    void frequencyTransformBandpass (ButterworthWorkspace<CoeffType>& workspace)
    {
        if (workspace.prewarpedFreqs.size() < 2) return;

        CoeffType wo = std::sqrt (workspace.prewarpedFreqs[0] * workspace.prewarpedFreqs[1]);
        CoeffType bw = std::abs (workspace.prewarpedFreqs[1] - workspace.prewarpedFreqs[0]);
        int degree = static_cast<int> (workspace.zpkPoles.size()) - static_cast<int> (workspace.zpkZeros.size());

        // Transform each pole/zero using correct bandpass transformation
        workspace.tempPoles1.clear();
        workspace.tempZeros1.clear();

        for (const auto& p : workspace.zpkPoles)
        {
            // Correct bandpass transformation: bp_S = 0.5 * lp_S * BW + 0.5 * sqrt(BW^2 * lp_S^2 - 4*Wc^2)
            Complex<CoeffType> lpS = p;
            Complex<CoeffType> term1 = static_cast<CoeffType> (0.5) * lpS * bw;
            Complex<CoeffType> discriminant = std::sqrt (bw * bw * lpS * lpS - static_cast<CoeffType> (4.0) * wo * wo);
            workspace.tempPoles1.push_back (term1 + static_cast<CoeffType> (0.5) * discriminant);
            workspace.tempPoles1.push_back (term1 - static_cast<CoeffType> (0.5) * discriminant);
        }

        workspace.zpkPoles = workspace.tempPoles1;

        for (const auto& z : workspace.zpkZeros)
        {
            Complex<CoeffType> lpS = z;
            Complex<CoeffType> term1 = static_cast<CoeffType> (0.5) * lpS * bw;
            Complex<CoeffType> discriminant = std::sqrt (bw * bw * lpS * lpS - static_cast<CoeffType> (4.0) * wo * wo);
            workspace.tempZeros1.push_back (term1 + static_cast<CoeffType> (0.5) * discriminant);
            workspace.tempZeros1.push_back (term1 - static_cast<CoeffType> (0.5) * discriminant);
        }

        // Add zeros at origin for degree difference
        for (int i = 0; i < degree; ++i)
            workspace.tempZeros1.emplace_back (static_cast<CoeffType> (0.0), static_cast<CoeffType> (0.0));

        workspace.zpkZeros = workspace.tempZeros1;

        // Correct gain adjustment for bandpass
        workspace.gain *= std::pow (bw, degree);
    }

    template <typename CoeffType>
    void frequencyTransformBandstop (ButterworthWorkspace<CoeffType>& workspace)
    {
        if (workspace.prewarpedFreqs.size() < 2) return;

        CoeffType wo = std::sqrt (workspace.prewarpedFreqs[0] * workspace.prewarpedFreqs[1]);
        CoeffType bw = std::abs (workspace.prewarpedFreqs[1] - workspace.prewarpedFreqs[0]);
        int degree = static_cast<int> (workspace.zpkPoles.size()) - static_cast<int> (workspace.zpkZeros.size());

        // Transform each pole/zero using correct bandstop transformation
        workspace.tempPoles1.clear();
        workspace.tempZeros1.clear();

        for (const auto& p : workspace.zpkPoles)
        {
            // Correct bandstop transformation: bs_S = 0.5 * BW / lp_S + 0.5 * sqrt(BW^2 / lp_S^2 - 4*Wc^2)
            Complex<CoeffType> lpS = p;
            Complex<CoeffType> term1 = static_cast<CoeffType> (0.5) * bw / lpS;
            Complex<CoeffType> discriminant = std::sqrt (bw * bw / (lpS * lpS) - static_cast<CoeffType> (4.0) * wo * wo);
            workspace.tempPoles1.push_back (term1 + static_cast<CoeffType> (0.5) * discriminant);
            workspace.tempPoles1.push_back (term1 - static_cast<CoeffType> (0.5) * discriminant);
        }

        workspace.zpkPoles = workspace.tempPoles1;

        for (const auto& z : workspace.zpkZeros)
        {
            Complex<CoeffType> lpS = z;
            Complex<CoeffType> term1 = static_cast<CoeffType> (0.5) * bw / lpS;
            Complex<CoeffType> discriminant = std::sqrt (bw * bw / (lpS * lpS) - static_cast<CoeffType> (4.0) * wo * wo);
            workspace.tempZeros1.push_back (term1 + static_cast<CoeffType> (0.5) * discriminant);
            workspace.tempZeros1.push_back (term1 - static_cast<CoeffType> (0.5) * discriminant);
        }

        // Add zeros at ±jwo for bandstop characteristic
        for (int i = 0; i < degree; ++i)
        {
            workspace.tempZeros1.emplace_back (static_cast<CoeffType> (0.0), wo);
            workspace.tempZeros1.emplace_back (static_cast<CoeffType> (0.0), -wo);
        }

        workspace.zpkZeros = workspace.tempZeros1;

        // Correct gain calculation for bandstop
        Complex<CoeffType> gainProduct (1, 0);
        for (const auto& p : workspace.zpkPoles) gainProduct *= p;
        for (const auto& z : workspace.zpkZeros) gainProduct /= z;
        workspace.gain *= std::abs (gainProduct);
    }

    template <typename CoeffType>
    void applyBilinearTransform (double sampleRate, ButterworthWorkspace<CoeffType>& workspace)
    {
        CoeffType fs = static_cast<CoeffType> (sampleRate);
        int degree = static_cast<int> (workspace.zpkPoles.size()) - static_cast<int> (workspace.zpkZeros.size());

        workspace.tempPoles1.clear();
        workspace.tempZeros1.clear();

        // Transform zeros
        for (auto& z : workspace.zpkZeros)
            workspace.tempZeros1.emplace_back ((static_cast<CoeffType> (2.0) * fs + z) / (static_cast<CoeffType> (2.0) * fs - z));

        // Add -1 zeros for the degree difference
        for (int i = 0; i < degree; ++i)
            workspace.tempZeros1.emplace_back (static_cast<CoeffType> (-1.0));

        workspace.zpkZeros = workspace.tempZeros1;

        // Transform poles
        for (auto& p : workspace.zpkPoles)
            workspace.tempPoles1.emplace_back ((static_cast<CoeffType> (2.0) * fs + p) / (static_cast<CoeffType> (2.0) * fs - p));

        workspace.zpkPoles = workspace.tempPoles1;

        Complex<CoeffType> Z (1, 0), P (1, 0);
        for (const auto& z : workspace.zpkZeros) Z *= (static_cast<CoeffType> (2.0) * fs - z);
        for (const auto& p : workspace.zpkPoles) P *= (static_cast<CoeffType> (2.0) * fs - p);
        workspace.gain *= (Z / P).real();
    }

    template <typename CoeffType>
    void zpkToSos (ButterworthWorkspace<CoeffType>& workspace)
    {
        workspace.biquadCoeffs.clear();

        auto& z = workspace.zpkZeros;
        auto& p = workspace.zpkPoles;
        size_t n = std::max (z.size(), p.size());

        // Ensure even number of zeros and poles for biquad pairing
        while (z.size() < n)
            z.emplace_back (static_cast<CoeffType> (0.0));
        while (p.size() < n)
            p.emplace_back (static_cast<CoeffType> (0.0));

        std::sort (z.begin(), z.end(), [](const auto& a, const auto& b) { return std::abs (a) < std::abs (b); });
        std::sort (p.begin(), p.end(), [](const auto& a, const auto& b) { return std::abs (a) < std::abs (b); });

        CoeffType g = workspace.gain;
        for (size_t i = 0; i < n; i += 2)
        {
            Complex<CoeffType> z1 = z[i];
            Complex<CoeffType> z2 = z[i + 1];
            Complex<CoeffType> p1 = p[i];
            Complex<CoeffType> p2 = p[i + 1];

            BiquadCoefficients<CoeffType> coeffs;
            coeffs.b0 = g;
            coeffs.b1 = -g * (z1 + z2).real();
            coeffs.b2 = g * (z1 * z2).real();
            coeffs.a0 = static_cast<CoeffType> (1.0);
            coeffs.a1 = -(p1 + p2).real();
            coeffs.a2 = (p1 * p2).real();

            workspace.biquadCoeffs.push_back (coeffs);
            g = static_cast<CoeffType> (1.0);
        }
    }

    template <typename CoeffType>
    void normalizeDcGain (ButterworthWorkspace<CoeffType>& workspace)
    {
        if (workspace.biquadCoeffs.empty()) return;

        // Calculate DC gain (H(z=1) = 1)
        CoeffType dcGain = static_cast<CoeffType> (1.0);
        for (const auto& s : workspace.biquadCoeffs)
        {
            CoeffType num = s.b0 + s.b1 + s.b2;
            CoeffType den = s.a0 + s.a1 + s.a2;
            if (std::abs (den) > static_cast<CoeffType> (1e-10))
                dcGain *= (num / den);
        }

        // Scale first section
        if (std::abs (dcGain) > static_cast<CoeffType> (1e-10))
        {
            workspace.biquadCoeffs[0].b0 /= dcGain;
            workspace.biquadCoeffs[0].b1 /= dcGain;
            workspace.biquadCoeffs[0].b2 /= dcGain;
        }
    }

    template <typename CoeffType>
    void normalizeGain (ButterworthWorkspace<CoeffType>& workspace, FilterModeType filterMode)
    {
        if (workspace.biquadCoeffs.empty()) return;

        CoeffType targetGain = static_cast<CoeffType> (1.0);
        
        if (filterMode.test (FilterMode::lowpass))
        {
            // For lowpass: normalize DC gain H(z=1) = 1
            for (const auto& s : workspace.biquadCoeffs)
            {
                CoeffType num = s.b0 + s.b1 + s.b2;
                CoeffType den = s.a0 + s.a1 + s.a2;
                if (std::abs (den) > static_cast<CoeffType> (1e-10))
                    targetGain *= (num / den);
            }
        }
        else if (filterMode.test (FilterMode::highpass))
        {
            // For highpass: normalize high-frequency gain H(z=-1) = 1
            for (const auto& s : workspace.biquadCoeffs)
            {
                CoeffType num = s.b0 - s.b1 + s.b2;
                CoeffType den = s.a0 - s.a1 + s.a2;
                if (std::abs (den) > static_cast<CoeffType> (1e-10))
                    targetGain *= (num / den);
            }
        }
        else if (filterMode.test (FilterMode::bandpass))
        {
            // For bandpass: normalize to peak gain = 1 at center frequency
            // Use geometric mean of cutoff frequencies for center frequency
            if (workspace.normalizedFreqs.size() >= 2)
            {
                CoeffType wc = std::sqrt (workspace.normalizedFreqs[0] * workspace.normalizedFreqs[1]);
                CoeffType omega = MathConstants<CoeffType>::pi * wc;
                Complex<CoeffType> z (std::cos (omega), std::sin (omega));
                
                Complex<CoeffType> H (1, 0);
                for (const auto& s : workspace.biquadCoeffs)
                {
                    Complex<CoeffType> num = s.b0 + s.b1 * z + s.b2 * z * z;
                    Complex<CoeffType> den = s.a0 + s.a1 * z + s.a2 * z * z;
                    if (std::abs (den) > static_cast<CoeffType> (1e-10))
                        H *= (num / den);
                }
                targetGain = std::abs (H);
            }
        }
        else if (filterMode.test (FilterMode::bandstop))
        {
            // For bandstop: normalize DC or high-frequency gain
            for (const auto& s : workspace.biquadCoeffs)
            {
                CoeffType num = s.b0 + s.b1 + s.b2;
                CoeffType den = s.a0 + s.a1 + s.a2;
                if (std::abs (den) > static_cast<CoeffType> (1e-10))
                    targetGain *= (num / den);
            }
        }
        else
        {
            // Default: normalize DC gain
            for (const auto& s : workspace.biquadCoeffs)
            {
                CoeffType num = s.b0 + s.b1 + s.b2;
                CoeffType den = s.a0 + s.a1 + s.a2;
                if (std::abs (den) > static_cast<CoeffType> (1e-10))
                    targetGain *= (num / den);
            }
        }

        // Scale first section
        if (std::abs (targetGain) > static_cast<CoeffType> (1e-10))
        {
            workspace.biquadCoeffs[0].b0 /= targetGain;
            workspace.biquadCoeffs[0].b1 /= targetGain;
            workspace.biquadCoeffs[0].b2 /= targetGain;
        }
    }

} // namespace

template <typename CoeffType>
int FilterDesigner<CoeffType>::designButterworth (
    FilterModeType filterMode,
    int order,
    CoeffType frequency,
    CoeffType frequency2,
    double sampleRate,
    ButterworthWorkspace<CoeffType>& workspace,
    std::vector<BiquadCoefficients<CoeffType>>& coefficients) noexcept
{
    // Validate inputs
    jassert (order >= 2 && order <= 32);
    jassert (frequency > static_cast<CoeffType> (0.0));
    jassert (sampleRate > 0.0);

    if (filterMode.test (FilterMode::bandpass) || filterMode.test (FilterMode::bandstop))
        jassert (frequency2 > frequency);

    // Ensure order is valid (1 or power of 2)
    order = jlimit (2, 32, nextPowerOfTwo (order));

    workspace.clear();
    coefficients.clear();

    // Build frequency vector
    std::vector<CoeffType> freqs;
    freqs.push_back (frequency);
    if (filterMode.test (FilterMode::bandpass) || filterMode.test (FilterMode::bandstop))
        freqs.push_back (frequency2);

    // Follow the zpk design sequence
    normalizeFrequencies (freqs, sampleRate, workspace);
    calculateAnalogPrototype (order, workspace);
    prewarpFrequencies (sampleRate, workspace);

    // Apply frequency transformations
    if (filterMode.test (FilterMode::lowpass))
        frequencyTransformLowpass (workspace);
    else if (filterMode.test (FilterMode::highpass))
        frequencyTransformHighpass (workspace);
    else if (filterMode.test (FilterMode::bandpass))
        frequencyTransformBandpass (workspace);
    else if (filterMode.test (FilterMode::bandstop))
        frequencyTransformBandstop (workspace);
    else
        frequencyTransformLowpass (workspace); // Default to lowpass

    // Transform to digital domain and convert to SOS
    applyBilinearTransform (sampleRate, workspace);
    zpkToSos (workspace);
    // Normalize gain appropriately for filter type
    normalizeGain (workspace, filterMode);

    // Copy to output
    coefficients = workspace.biquadCoeffs;

    return static_cast<int> (coefficients.size());
}

//==============================================================================
// Explicit template instantiations
//==============================================================================

template class FilterDesigner<float>;
template class FilterDesigner<double>;

} // namespace yup
