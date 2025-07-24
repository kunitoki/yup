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

/** Mathematical constants and utility functions for DSP operations. */
namespace DspMath
{

//==============================================================================

/** Complex number type alias using std::complex */
template <typename FloatType>
using Complex = std::complex<FloatType>;

/** Creates a complex number from magnitude and phase */
template <typename FloatType>
constexpr Complex<FloatType> polar (FloatType magnitude, FloatType phase) noexcept
{
    return std::polar (magnitude, phase);
}

//==============================================================================

template <typename FloatType>
using ComplexVector = std::vector<DspMath::Complex<FloatType>>;

//==============================================================================

/** Converts frequency to angular frequency (radians per sample) */
template <typename FloatType>
constexpr FloatType frequencyToAngular (FloatType frequency, FloatType sampleRate) noexcept
{
    return MathConstants<FloatType>::twoPi * frequency / sampleRate;
}

/** Converts angular frequency (radians per sample) to frequency */
template <typename FloatType>
constexpr FloatType angularToFrequency (FloatType omega, FloatType sampleRate) noexcept
{
    return omega * sampleRate / MathConstants<FloatType>::twoPi;
}

//==============================================================================

/** Converts Q factor to bandwidth (octaves) */
template <typename FloatType>
constexpr FloatType qToBandwidth (FloatType q) noexcept
{
    return static_cast<FloatType> (2.0) * std::asinh (static_cast<FloatType> (1.0) / (static_cast<FloatType> (2.0) * q)) / MathConstants<FloatType>::ln2;
}

/** Converts bandwidth (octaves) to Q factor */
template <typename FloatType>
constexpr FloatType bandwidthToQ (FloatType bandwidth) noexcept
{
    return static_cast<FloatType> (1.0) / (static_cast<FloatType> (2.0) * std::sinh (bandwidth * MathConstants<FloatType>::ln2 / static_cast<FloatType> (2.0)));
}

//==============================================================================

/** Converts decibels to linear gain */
template <typename FloatType>
constexpr FloatType dbToGain (FloatType decibels) noexcept
{
    return std::pow (static_cast<FloatType> (10.0), decibels / static_cast<FloatType> (20.0));
}

/** Converts linear gain to decibels */
template <typename FloatType>
constexpr FloatType gainToDb (FloatType gain) noexcept
{
    return static_cast<FloatType> (20.0) * std::log10 (gain);
}

//==============================================================================

/** Fast approximation of sin(x) using Taylor series for small angles */
template <typename FloatType>
FloatType fastSin (FloatType x) noexcept
{
    const auto x2 = x * x;
    return x * (static_cast<FloatType> (1.0) - x2 / static_cast<FloatType> (6.0) *
               (static_cast<FloatType> (1.0) - x2 / static_cast<FloatType> (20.0)));
}

/** Fast approximation of cos(x) using Taylor series for small angles */
template <typename FloatType>
FloatType fastCos (FloatType x) noexcept
{
    const auto x2 = x * x;
    return static_cast<FloatType> (1.0) - x2 / static_cast<FloatType> (2.0) *
           (static_cast<FloatType> (1.0) - x2 / static_cast<FloatType> (12.0));
}

//==============================================================================

/** Bilinear transform from s-plane to z-plane with frequency warping */
template <typename FloatType>
void bilinearTransform (FloatType& a0, FloatType& a1, FloatType& a2,
                        FloatType& b0, FloatType& b1, FloatType& b2,
                        FloatType frequency, FloatType sampleRate) noexcept
{
    const auto warpedFreq = static_cast<FloatType> (2.0) * sampleRate * std::tan (frequencyToAngular (frequency, sampleRate) / static_cast<FloatType> (2.0));
    const auto k = warpedFreq / sampleRate;
    const auto k2 = k * k;
    const auto norm = static_cast<FloatType> (1.0) / (a0 + a1 * k + a2 * k2);

    const auto newB0 = (b0 + b1 * k + b2 * k2) * norm;
    const auto newB1 = (static_cast<FloatType> (2.0) * (b2 * k2 - b0)) * norm;
    const auto newB2 = (b0 - b1 * k + b2 * k2) * norm;
    const auto newA1 = (static_cast<FloatType> (2.0) * (a2 * k2 - a0)) * norm;
    const auto newA2 = (a0 - a1 * k + a2 * k2) * norm;

    a0 = static_cast<FloatType> (1.0);
    a1 = newA1;
    a2 = newA2;
    b0 = newB0;
    b1 = newB1;
    b2 = newB2;
}

//==============================================================================

template <typename FloatType>
void extractPolesZerosFromSecondOrderBiquad (FloatType b0, FloatType b1, FloatType b2,
                                             FloatType a0, FloatType a1, FloatType a2,
                                             DspMath::ComplexVector<FloatType>& poles,
                                             DspMath::ComplexVector<FloatType>& zeros)
{
    const auto epsilon = static_cast<FloatType> (1e-12);

    // Calculate poles from denominator: 1 + a1*z^-1 + a2*z^-2 = 0
    // Multiplying by z^2: z^2 + a1*z + a2 = 0
    // Using quadratic formula: z = (-a1 ± √(a1² - 4*a2)) / 2
    if (std::abs (a2) > epsilon)
    {
        auto discriminant = a1 * a1 - 4 * a2;
        if (discriminant >= 0)
        {
            // Real poles
            auto sqrtDisc = std::sqrt (discriminant);
            poles.push_back (DspMath::Complex<FloatType> ((-a1 + sqrtDisc) / 2, 0));
            poles.push_back (DspMath::Complex<FloatType> ((-a1 - sqrtDisc) / 2, 0));
        }
        else
        {
            // Complex conjugate poles
            auto real = -a1 / 2;
            auto imag = std::sqrt (-discriminant) / 2;
            poles.push_back (DspMath::Complex<FloatType> (real, imag));
            poles.push_back (DspMath::Complex<FloatType> (real, -imag));
        }
    }
    else if (std::abs (a1) > epsilon)
    {
        // First-order: 1 + a1*z^-1 = 0 -> z = -1/a1
        poles.push_back (DspMath::Complex<FloatType> (-1 / a1, 0));
    }

    // Calculate zeros from numerator: b0 + b1*z^-1 + b2*z^-2 = 0
    // Multiplying by z^2: b0*z^2 + b1*z + b2 = 0
    // Using quadratic formula: z = (-b1 ± √(b1² - 4*b0*b2)) / (2*b0)
    if (std::abs (b0) > epsilon && std::abs (b2) > epsilon)
    {
        auto discriminant = b1 * b1 - 4 * b0 * b2;
        if (discriminant >= 0)
        {
            // Real zeros
            auto sqrtDisc = std::sqrt (discriminant);
            zeros.push_back (DspMath::Complex<FloatType> ((-b1 + sqrtDisc) / (2 * b0), 0));
            zeros.push_back (DspMath::Complex<FloatType> ((-b1 - sqrtDisc) / (2 * b0), 0));
        }
        else
        {
            // Complex conjugate zeros
            auto real = -b1 / (2 * b0);
            auto imag = std::sqrt (-discriminant) / (2 * b0);
            zeros.push_back (DspMath::Complex<FloatType> (real, imag));
            zeros.push_back (DspMath::Complex<FloatType> (real, -imag));
        }
    }
    else if (std::abs (b1) > epsilon && std::abs (b0) > epsilon)
    {
        // First-order: b0 + b1*z^-1 = 0 -> z = -b0/b1
        zeros.push_back (DspMath::Complex<FloatType> (-b0 / b1, 0));
    }
    else if (std::abs (b2) > epsilon)
    {
        // Zero at origin (b0 = 0): b1*z^-1 + b2*z^-2 = 0 -> z*(b1 + b2*z^-1) = 0
        // One zero at z = 0, another at z = -b1/b2
        zeros.push_back (DspMath::Complex<FloatType> (0, 0));
        if (std::abs (b1) > epsilon)
            zeros.push_back (DspMath::Complex<FloatType> (-b1 / b2, 0));
    }
}

/** Extract poles and zeros from fourth-order section coefficients */
template <typename FloatType>
void extractPolesZerosFromFourthOrderBiquad (FloatType b0, FloatType b1, FloatType b2, FloatType b3, FloatType b4,
                                             FloatType a0, FloatType a1, FloatType a2, FloatType a3, FloatType a4,
                                             DspMath::ComplexVector<FloatType>& poles,
                                             DspMath::ComplexVector<FloatType>& zeros)
{
    // For fourth-order polynomials, we can try to factor them into quadratic pairs
    // This is a simplified approach - for full accuracy, a robust polynomial root finder would be needed

    // First, try to factor the denominator polynomial (poles)
    // a4*z^4 + a3*z^3 + a2*z^2 + a1*z + a0 = 0

    // For Butterworth filters designed using our method, we can often decompose this way:
    // Split into two biquads with shared characteristics

    // Simple approach: assume it can be factored as (z^2 + p1*z + q1)(z^2 + p2*z + q2)

    const auto epsilon = static_cast<FloatType> (1e-12);

    if (std::abs (a4) > epsilon)
    {
        // Attempt to find characteristic polynomial roots
        // This is a simplified extraction - in practice, you'd want a full polynomial solver

        // Try to extract first biquad-like section
        auto a1_norm = a1 / a4;
        auto a2_norm = a2 / a4;
        auto a3_norm = a3 / a4;
        auto a0_norm = a0 / a4;

        // Use approximation method for 4th order Butterworth characteristics
        // Extract two approximate biquad sections
        auto q1 = std::sqrt (std::abs (a0_norm));
        auto p1 = a1_norm / 2;

        if (q1 > epsilon)
        {
            auto discriminant1 = p1 * p1 - 4 * q1;
            if (discriminant1 >= 0)
            {
                auto sqrtDisc = std::sqrt (discriminant1);
                poles.push_back (DspMath::Complex<FloatType> ((-p1 + sqrtDisc) / 2, 0));
                poles.push_back (DspMath::Complex<FloatType> ((-p1 - sqrtDisc) / 2, 0));
            }
            else
            {
                auto real = -p1 / 2;
                auto imag = std::sqrt (-discriminant1) / 2;
                poles.push_back (DspMath::Complex<FloatType> (real, imag));
                poles.push_back (DspMath::Complex<FloatType> (real, -imag));
            }
        }

        // Second pair (approximation)
        auto p2 = a3_norm / 2;
        auto q2 = a2_norm - q1;

        if (std::abs (q2) > epsilon)
        {
            auto discriminant2 = p2 * p2 - 4 * q2;
            if (discriminant2 >= 0)
            {
                auto sqrtDisc = std::sqrt (discriminant2);
                poles.push_back (DspMath::Complex<FloatType> ((-p2 + sqrtDisc) / 2, 0));
                poles.push_back (DspMath::Complex<FloatType> ((-p2 - sqrtDisc) / 2, 0));
            }
            else
            {
                auto real = -p2 / 2;
                auto imag = std::sqrt (-discriminant2) / 2;
                poles.push_back (DspMath::Complex<FloatType> (real, imag));
                poles.push_back (DspMath::Complex<FloatType> (real, -imag));
            }
        }
    }

    // Similar approach for zeros (numerator polynomial)
    if (std::abs (b4) > epsilon)
    {
        auto b1_norm = b1 / b4;
        auto b2_norm = b2 / b4;
        auto b3_norm = b3 / b4;
        auto b0_norm = b0 / b4;

        auto q1 = std::sqrt (std::abs (b0_norm));
        auto p1 = b1_norm / 2;

        if (q1 > epsilon)
        {
            auto discriminant1 = p1 * p1 - 4 * q1;
            if (discriminant1 >= 0)
            {
                auto sqrtDisc = std::sqrt (discriminant1);
                zeros.push_back (DspMath::Complex<FloatType> ((-p1 + sqrtDisc) / 2, 0));
                zeros.push_back (DspMath::Complex<FloatType> ((-p1 - sqrtDisc) / 2, 0));
            }
            else
            {
                auto real = -p1 / 2;
                auto imag = std::sqrt (-discriminant1) / 2;
                zeros.push_back (DspMath::Complex<FloatType> (real, imag));
                zeros.push_back (DspMath::Complex<FloatType> (real, -imag));
            }
        }

        auto p2 = b3_norm / 2;
        auto q2 = b2_norm - q1;

        if (std::abs (q2) > epsilon)
        {
            auto discriminant2 = p2 * p2 - 4 * q2;
            if (discriminant2 >= 0)
            {
                auto sqrtDisc = std::sqrt (discriminant2);
                zeros.push_back (DspMath::Complex<FloatType> ((-p2 + sqrtDisc) / 2, 0));
                zeros.push_back (DspMath::Complex<FloatType> ((-p2 - sqrtDisc) / 2, 0));
            }
            else
            {
                auto real = -p2 / 2;
                auto imag = std::sqrt (-discriminant2) / 2;
                zeros.push_back (DspMath::Complex<FloatType> (real, imag));
                zeros.push_back (DspMath::Complex<FloatType> (real, -imag));
            }
        }
    }
}

} // namespace DspMath
} // namespace yup
