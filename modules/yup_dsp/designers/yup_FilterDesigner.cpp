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

// Explicit instantiations for FilterDesigner
template class FilterDesigner<float>;
template class FilterDesigner<double>;

} // namespace yup
