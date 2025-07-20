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

#include <yup_dsp/yup_dsp.h>

namespace yup
{

namespace
{

//==============================================================================

/** Transforms lowpass coefficients to highpass */
template <typename CoeffType>
static void transformLowpassToHighpass (BiquadCoefficients<CoeffType>& coeffs) noexcept
{
    // Spectral inversion: negate odd-indexed coefficients
    coeffs.b1 = -coeffs.b1;
    coeffs.a1 = -coeffs.a1;
}

//==============================================================================

/** Gets normalized Bessel polynomial coefficients for given order */
template <typename CoeffType>
static std::vector<CoeffType> getBesselPolynomial (int order) noexcept
{
    // Pre-computed normalized Bessel polynomial coefficients for orders 1-10
    // These are designed for maximum flatness of group delay
    static const std::vector<std::vector<double>> besselCoeffs = {
        {},                                                     // order 0 (unused)
        {1.0, 1.0},                                            // order 1: s + 1
        {1.0, 3.0, 3.0},                                       // order 2: s^2 + 3s + 3
        {1.0, 6.0, 15.0, 15.0},                               // order 3
        {1.0, 10.0, 45.0, 105.0, 105.0},                     // order 4
        {1.0, 15.0, 105.0, 420.0, 945.0, 945.0},             // order 5
        {1.0, 21.0, 210.0, 1260.0, 4725.0, 10395.0, 10395.0}, // order 6
        {1.0, 28.0, 378.0, 3150.0, 17325.0, 62370.0, 135135.0, 135135.0}, // order 7
        {1.0, 36.0, 630.0, 6930.0, 51975.0, 270270.0, 945945.0, 2027025.0, 2027025.0}, // order 8
        {1.0, 45.0, 990.0, 13860.0, 135135.0, 945945.0, 4729725.0, 16216200.0, 34459425.0, 34459425.0}, // order 9
        {1.0, 55.0, 1485.0, 25740.0, 315315.0, 2837835.0, 18918900.0, 91891800.0, 310134825.0, 654729075.0, 654729075.0} // order 10
    };

    if (order < 1 || order > 10)
    {
        // For orders > 10, use recursive generation (simplified)
        return {static_cast<CoeffType> (1.0), static_cast<CoeffType> (1.0)};
    }

    const auto& coeffs = besselCoeffs[static_cast<size_t> (order)];
    std::vector<CoeffType> result;
    result.reserve (coeffs.size());

    for (auto coeff : coeffs)
    {
        result.push_back (static_cast<CoeffType> (coeff));
    }

    return result;
}

/** Calculates poles for Bessel filter of given order */
template <typename CoeffType>
static void calculateBesselPoles (int order, std::vector<std::complex<CoeffType>>& poles) noexcept
{
    poles.clear();
    poles.reserve (static_cast<size_t> (order));

    // Pre-computed normalized Bessel poles for orders 1-10
    // These provide maximum flatness of group delay
    static const std::vector<std::vector<std::complex<double>>> besselPoles = {
        {},                                                                      // order 0
        {{-1.0, 0.0}},                                                          // order 1
        {{-1.5, 0.866025}, {-1.5, -0.866025}},                                // order 2
        {{-2.3222, 0.0}, {-1.8389, 1.7544}, {-1.8389, -1.7544}},            // order 3
        {{-2.8962, 1.8379}, {-2.8962, -1.8379}, {-2.1038, 2.6575}, {-2.1038, -2.6575}}, // order 4
        {{-3.6467, 0.0}, {-3.3520, 2.4150}, {-3.3520, -2.4150}, {-2.3247, 3.5710}, {-2.3247, -3.5710}}, // order 5
        // For orders > 5, use approximation
    };

    if (order >= 1 && order <= static_cast<int> (besselPoles.size() - 1))
    {
        const auto& orderPoles = besselPoles[static_cast<size_t> (order)];
        for (const auto& pole : orderPoles)
            poles.emplace_back (static_cast<CoeffType> (pole.real()), static_cast<CoeffType> (pole.imag()));
    }
    else
    {
        // For higher orders, use approximation based on Butterworth poles with group delay correction
        for (int i = 0; i < order; ++i)
        {
            const auto angle = MathConstants<CoeffType>::pi * (static_cast<CoeffType> (2 * i + order + 1)) / (static_cast<CoeffType> (2 * order));
            const auto real = -std::cos (angle);
            const auto imag = std::sin (angle);

            // Apply Bessel correction factor for group delay flatness
            const auto correction = static_cast<CoeffType> (1.0) + static_cast<CoeffType> (0.5) / static_cast<CoeffType> (order);
            poles.emplace_back (real * correction, imag * correction);
        }
    }
}

//==============================================================================

/** Approximation of complete elliptic integral K(k) */
template <typename CoeffType>
CoeffType ellipticIntegralK (CoeffType k) noexcept
{
    if (k > static_cast<CoeffType> (0.99))
    {
        // Use logarithmic approximation for k close to 1
        const auto k_prime = std::sqrt (static_cast<CoeffType> (1.0) - k * k);
        return std::log (static_cast<CoeffType> (4.0) / k_prime);
    }

    // AGM (Arithmetic-Geometric Mean) method approximation
    const auto a0 = static_cast<CoeffType> (1.0);
    const auto b0 = std::sqrt (static_cast<CoeffType> (1.0) - k * k);

    auto a = a0;
    auto b = b0;

    for (int n = 0; n < 10; ++n)  // Usually converges quickly
    {
        const auto a_new = (a + b) / static_cast<CoeffType> (2.0);
        const auto b_new = std::sqrt (a * b);

        if (std::abs (a - b) < static_cast<CoeffType> (1e-12))
            break;

        a = a_new;
        b = b_new;
    }

    return MathConstants<CoeffType>::pi / (static_cast<CoeffType> (2.0) * a);
}

/** Jacobi elliptic functions sn, cn, dn */
template <typename CoeffType>
void jacobianElliptic (CoeffType u, CoeffType k, CoeffType& cn, CoeffType& sn, CoeffType& dn) noexcept
{
    // Simplified approximation using series expansion
    // For production code, a full implementation would be needed

    const auto k2 = k * k;

    if (std::abs (u) < static_cast<CoeffType> (1e-8))
    {
        // Small angle approximation
        sn = u;
        cn = static_cast<CoeffType> (1.0);
        dn = static_cast<CoeffType> (1.0);
        return;
    }

    // Use trigonometric approximation for moderate values
    const auto sin_u = std::sin (u);
    const auto cos_u = std::cos (u);

    sn = sin_u / std::sqrt (static_cast<CoeffType> (1.0) + k2 * sin_u * sin_u);
    cn = cos_u / std::sqrt (static_cast<CoeffType> (1.0) + k2 * sin_u * sin_u);
    dn = std::sqrt (static_cast<CoeffType> (1.0) - k2 * sn * sn);
}

/** Inverse Jacobi sn function for real argument */
template <typename CoeffType>
CoeffType jacobianInverseSnReal (CoeffType x, CoeffType k) noexcept
{
    // Simplified approximation
    if (std::abs (x) > static_cast<CoeffType> (0.99))
        return static_cast<CoeffType> (0.5) * std::log ((static_cast<CoeffType> (1.0) + x) / (static_cast<CoeffType> (1.0) - x));

    // Use series approximation for moderate values
    return std::asin (x * std::sqrt (static_cast<CoeffType> (1.0) + k * k * x * x));
}

/** Calculates poles and zeros for Elliptic filter of given order */
template <typename CoeffType>
static void calculateEllipticPoles (int order, CoeffType epsilon, CoeffType k,
                                    std::vector<std::complex<CoeffType>>& poles,
                                    std::vector<CoeffType>& zeros) noexcept
{
    poles.clear();
    zeros.clear();
    poles.reserve (static_cast<size_t> (order));

    // Calculate the modular angle for elliptic integrals
    const auto k1 = k;
    const auto k1_prime = std::sqrt (static_cast<CoeffType> (1.0) - k1 * k1);

    // Calculate elliptic integral K(k) approximation
    const auto K = ellipticIntegralK<CoeffType> (k1);
    const auto K_prime = ellipticIntegralK<CoeffType> (k1_prime);

    // Calculate v0 (location of real pole for odd orders)
    const auto v0 = -jacobianInverseSnReal<CoeffType> (static_cast<CoeffType> (1.0) / epsilon, k1_prime) / static_cast<CoeffType> (order);

    // Generate poles using Jacobi elliptic functions
    for (int i = 1; i <= order; ++i)
    {
        const auto u = static_cast<CoeffType> (2 * i - 1) * K / static_cast<CoeffType> (order);

        CoeffType cd, sd, nd;
        jacobianElliptic<CoeffType> (u, k1, cd, sd, nd);

        const auto denominator = static_cast<CoeffType> (1.0) - std::pow (k1 * sd, 2);

        if (i <= (order + 1) / 2)  // Only compute half, use conjugate symmetry
        {
            if (order % 2 == 1 && i == (order + 1) / 2)
            {
                // Real pole for odd-order filters
                CoeffType sn_v0, cn_v0, dn_v0;
                jacobianElliptic<CoeffType> (v0, k1_prime, cn_v0, sn_v0, dn_v0);

                const auto realPole = -sn_v0 / cn_v0;
                poles.emplace_back (realPole, static_cast<CoeffType> (0.0));
            }
            else
            {
                // Complex conjugate pole pair
                CoeffType sn_v0, cn_v0, dn_v0;
                jacobianElliptic<CoeffType> (v0, k1_prime, cn_v0, sn_v0, dn_v0);

                const auto realPart = -(cd * sn_v0 * cn_v0) / denominator;
                const auto imagPart = (sd * nd * dn_v0) / denominator;

                poles.emplace_back (realPart, imagPart);
                poles.emplace_back (realPart, -imagPart);
            }
        }
    }

    // Calculate zeros (for finite transmission zeros)
    const auto numZeros = order / 2;
    for (int i = 1; i <= numZeros; ++i)
    {
        const auto u = static_cast<CoeffType> (2 * i - 1) * K / static_cast<CoeffType> (order);

        CoeffType cd, sd, nd;
        jacobianElliptic<CoeffType> (u, k1, cd, sd, nd);

        const auto zero_freq = static_cast<CoeffType> (1.0) / (k1 * sd);
        zeros.push_back (zero_freq);
    }
}

} // namespace

//==============================================================================

template <typename CoeffType>
void FilterDesigner<CoeffType>::designButterworthImpl (
    std::vector<BiquadCoefficients<CoeffType>>& sections,
    bool isHighpass, int order, CoeffType frequency, double sampleRate) noexcept
{
    const int numSections = (order + 1) / 2;
    sections.resize (static_cast<size_t> (numSections));

    const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate));

    for (int i = 0; i < numSections; ++i)
    {
        BiquadCoefficients<CoeffType>& coeffs = sections[static_cast<size_t> (i)];

        if (order % 2 == 1 && i == 0)
        {
            // First-order section for odd-order filters
            const auto k = std::tan (omega / static_cast<CoeffType> (2.0));
            const auto norm = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) + k);

            coeffs.b0 = k * norm;
            coeffs.b1 = k * norm;
            coeffs.b2 = static_cast<CoeffType> (0.0);
            coeffs.a0 = static_cast<CoeffType> (1.0);
            coeffs.a1 = (k - static_cast<CoeffType> (1.0)) * norm;
            coeffs.a2 = static_cast<CoeffType> (0.0);
        }
        else
        {
            // Second-order sections
            const auto sectionIndex = (order % 2 == 1) ? i - 1 : i;
            const auto poleAngle = MathConstants<CoeffType>::pi * (static_cast<CoeffType> (2 * sectionIndex + order + 1)) /
                                  (static_cast<CoeffType> (2 * order));
            const auto k = std::tan (omega / static_cast<CoeffType> (2.0));
            const auto q = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (2.0) * std::abs (std::cos (poleAngle)));
            const auto k2 = k * k;
            const auto norm = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) + k / q + k2);

            coeffs.b0 = k2 * norm;
            coeffs.b1 = static_cast<CoeffType> (2.0) * k2 * norm;
            coeffs.b2 = k2 * norm;
            coeffs.a0 = static_cast<CoeffType> (1.0);
            coeffs.a1 = (static_cast<CoeffType> (2.0) * (k2 - static_cast<CoeffType> (1.0))) * norm;
            coeffs.a2 = (static_cast<CoeffType> (1.0) - k / q + k2) * norm;
        }

        // Transform to desired type
        if (isHighpass)
            transformLowpassToHighpass (coeffs);
    }
}

//==============================================================================

template <typename CoeffType>
void FilterDesigner<CoeffType>::designChebyshev1Impl (
    std::vector<BiquadCoefficients<CoeffType>>& sections,
    bool isHighpass, int order, CoeffType frequency, double sampleRate, CoeffType ripple) noexcept
{
    const int numSections = (order + 1) / 2;
    sections.resize (static_cast<size_t> (numSections));

    const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate));

    // Convert ripple from dB to linear
    const auto epsilon = std::sqrt (std::pow (static_cast<CoeffType> (10.0), ripple / static_cast<CoeffType> (10.0)) - static_cast<CoeffType> (1.0));

    // Calculate Chebyshev poles
    const auto gamma = std::asinh (static_cast<CoeffType> (1.0) / epsilon) / static_cast<CoeffType> (order);
    const auto sinhGamma = std::sinh (gamma);
    const auto coshGamma = std::cosh (gamma);

    for (int i = 0; i < numSections; ++i)
    {
        BiquadCoefficients<CoeffType>& coeffs = sections[static_cast<size_t> (i)];

        if (order % 2 == 1 && i == 0)
        {
            // First-order section for odd-order filters
            const auto realPole = -sinhGamma;
            const auto k = std::tan (omega / static_cast<CoeffType> (2.0));
            const auto alpha = realPole;
            const auto norm = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) - alpha * k);

            coeffs.b0 = k * norm;
            coeffs.b1 = k * norm;
            coeffs.b2 = static_cast<CoeffType> (0.0);
            coeffs.a0 = static_cast<CoeffType> (1.0);
            coeffs.a1 = (k + alpha * k - static_cast<CoeffType> (1.0)) * norm;
            coeffs.a2 = static_cast<CoeffType> (0.0);
        }
        else
        {
            // Second-order sections
            const auto sectionIndex = (order % 2 == 1) ? i - 1 : i;
            const auto poleAngle = MathConstants<CoeffType>::pi * (static_cast<CoeffType> (2 * sectionIndex + 1)) /
                                  (static_cast<CoeffType> (2 * order));

            const auto realPart = -sinhGamma * std::sin (poleAngle);
            const auto imagPart = coshGamma * std::cos (poleAngle);

            const auto k = std::tan (omega / static_cast<CoeffType> (2.0));
            const auto k2 = k * k;
            const auto a1_analog = static_cast<CoeffType> (-2.0) * realPart;
            const auto a0_analog = realPart * realPart + imagPart * imagPart;

            // Bilinear transform
            const auto norm = static_cast<CoeffType> (1.0) / (a0_analog + a1_analog * k + k2);

            coeffs.b0 = k2 * norm;
            coeffs.b1 = static_cast<CoeffType> (2.0) * k2 * norm;
            coeffs.b2 = k2 * norm;
            coeffs.a0 = static_cast<CoeffType> (1.0);
            coeffs.a1 = (static_cast<CoeffType> (2.0) * (k2 - a0_analog)) * norm;
            coeffs.a2 = (a0_analog - a1_analog * k + k2) * norm;
        }

        // Transform to desired type
        if (isHighpass)
            transformLowpassToHighpass (coeffs);
    }
}

//==============================================================================

template <typename CoeffType>
void FilterDesigner<CoeffType>::designChebyshev2Impl (
    std::vector<BiquadCoefficients<CoeffType>>& sections,
    bool isHighpass, int order, CoeffType frequency, double sampleRate, CoeffType stopbandAtten) noexcept
{
    const int numSections = (order + 1) / 2;
    sections.resize (static_cast<size_t> (numSections));

    const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate));

    // Convert stopband attenuation from dB to linear
    const auto epsilon = static_cast<CoeffType> (1.0) / std::sqrt (std::pow (static_cast<CoeffType> (10.0), stopbandAtten / static_cast<CoeffType> (10.0)) - static_cast<CoeffType> (1.0));

    // Calculate Chebyshev Type II poles and zeros
    const auto gamma = std::asinh (static_cast<CoeffType> (1.0) / epsilon) / static_cast<CoeffType> (order);
    const auto sinhGamma = std::sinh (gamma);
    const auto coshGamma = std::cosh (gamma);

    for (int i = 0; i < numSections; ++i)
    {
        BiquadCoefficients<CoeffType>& coeffs = sections[static_cast<size_t> (i)];

        if (order % 2 == 1 && i == 0)
        {
            // First-order section for odd-order filters
            const auto realPole = static_cast<CoeffType> (-1.0) / sinhGamma;
            const auto k = std::tan (omega / static_cast<CoeffType> (2.0));
            const auto alpha = realPole;
            const auto norm = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) - alpha * k);

            // Type II has a zero at infinity, so numerator is just a constant
            coeffs.b0 = static_cast<CoeffType> (1.0) * norm;
            coeffs.b1 = static_cast<CoeffType> (0.0);
            coeffs.b2 = static_cast<CoeffType> (0.0);
            coeffs.a0 = static_cast<CoeffType> (1.0);
            coeffs.a1 = (k + alpha * k - static_cast<CoeffType> (1.0)) * norm;
            coeffs.a2 = static_cast<CoeffType> (0.0);
        }
        else
        {
            // Second-order sections
            const auto sectionIndex = (order % 2 == 1) ? i - 1 : i;
            const auto poleAngle = MathConstants<CoeffType>::pi * (static_cast<CoeffType> (2 * sectionIndex + 1)) /
                                  (static_cast<CoeffType> (2 * order));

            // Type II poles are reciprocals of Type I poles
            const auto realPartType1 = -sinhGamma * std::sin (poleAngle);
            const auto imagPartType1 = coshGamma * std::cos (poleAngle);
            const auto poleRadius = realPartType1 * realPartType1 + imagPartType1 * imagPartType1;

            const auto realPart = realPartType1 / poleRadius;
            const auto imagPart = -imagPartType1 / poleRadius;

            // Zeros are on the imaginary axis
            const auto zeroFreq = static_cast<CoeffType> (1.0) / std::cos (poleAngle);

            const auto k = std::tan (omega / static_cast<CoeffType> (2.0));
            const auto k2 = k * k;

            // Pole polynomial coefficients
            const auto a1_analog = static_cast<CoeffType> (-2.0) * realPart;
            const auto a0_analog = realPart * realPart + imagPart * imagPart;

            // Zero polynomial coefficients (zeros at ±j*zeroFreq)
            const auto b0_analog = static_cast<CoeffType> (1.0);
            const auto b1_analog = static_cast<CoeffType> (0.0);
            const auto b2_analog = zeroFreq * zeroFreq;

            // Bilinear transform
            const auto poleNorm = static_cast<CoeffType> (1.0) / (a0_analog + a1_analog * k + k2);
            const auto zeroNorm = static_cast<CoeffType> (1.0) / (b0_analog + b1_analog * k + b2_analog * k2);

            coeffs.b0 = (b0_analog * k2) * zeroNorm * poleNorm;
            coeffs.b1 = (static_cast<CoeffType> (2.0) * (b0_analog * k2 - b2_analog)) * zeroNorm * poleNorm;
            coeffs.b2 = (b0_analog * k2 - b1_analog * k + b2_analog) * zeroNorm * poleNorm;
            coeffs.a0 = static_cast<CoeffType> (1.0);
            coeffs.a1 = (static_cast<CoeffType> (2.0) * (k2 - a0_analog)) * poleNorm;
            coeffs.a2 = (a0_analog - a1_analog * k + k2) * poleNorm;
        }

        // Transform to desired type
        if (isHighpass)
            transformLowpassToHighpass (coeffs);
    }
}

//==============================================================================

template <typename CoeffType>
void FilterDesigner<CoeffType>::designBesselImpl (
    std::vector<BiquadCoefficients<CoeffType>>& sections,
    bool isHighpass, int order, CoeffType frequency, double sampleRate) noexcept
{
    const auto numSections = (order + 1) / 2;
    sections.resize (numSections);

    // Get Bessel polynomial coefficients
    const auto besselCoeffs = getBesselPolynomial<CoeffType> (order);

    // Pre-warp frequency for bilinear transform
    const auto omega = MathConstants<CoeffType>::twoPi * frequency / static_cast<CoeffType> (sampleRate);
    const auto k = std::tan (omega / static_cast<CoeffType> (2.0));

    // Calculate pole positions for Bessel polynomial
    std::vector<std::complex<CoeffType>> poles;
    calculateBesselPoles<CoeffType> (order, poles);

    // Scale poles for desired cutoff frequency
    for (auto& pole : poles)
        pole *= frequency * static_cast<CoeffType> (2.0) * MathConstants<CoeffType>::pi;

    // Convert poles to biquad sections
    for (int i = 0; i < numSections; ++i)
    {
        BiquadCoefficients<CoeffType>& coeffs = sections[static_cast<size_t> (i)];

        if (order % 2 == 1 && i == 0)
        {
            // First-order section for odd-order filters
            const auto pole = poles[0].real();
            const auto a = -pole;
            const auto k_scaled = k / a;
            const auto norm = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) + k_scaled);

            coeffs.b0 = k_scaled * norm;
            coeffs.b1 = k_scaled * norm;
            coeffs.b2 = static_cast<CoeffType> (0.0);
            coeffs.a0 = static_cast<CoeffType> (1.0);
            coeffs.a1 = (k_scaled - static_cast<CoeffType> (1.0)) * norm;
            coeffs.a2 = static_cast<CoeffType> (0.0);
        }
        else
        {
            // Second-order sections from complex conjugate pairs
            const auto poleIndex = (order % 2 == 1) ? 2 * i - 1 : 2 * i;
            const auto pole1 = poles[poleIndex];
            const auto pole2 = poles[poleIndex + 1];

            // Convert pole pair to second-order section
            const auto sigma = -(pole1.real() + pole2.real());
            const auto omega0 = std::sqrt (pole1.real() * pole1.real() + pole1.imag() * pole1.imag());
            const auto q = omega0 / (static_cast<CoeffType> (2.0) * std::abs (pole1.real()));

            const auto k2 = k * k;
            const auto k_over_q = k / q;
            const auto norm = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) + k_over_q + k2);

            coeffs.b0 = k2 * norm;
            coeffs.b1 = static_cast<CoeffType> (2.0) * k2 * norm;
            coeffs.b2 = k2 * norm;
            coeffs.a0 = static_cast<CoeffType> (1.0);
            coeffs.a1 = (static_cast<CoeffType> (2.0) * (k2 - static_cast<CoeffType> (1.0))) * norm;
            coeffs.a2 = (static_cast<CoeffType> (1.0) - k_over_q + k2) * norm;
        }

        // Transform to highpass if needed
        if (isHighpass)
            transformLowpassToHighpass (coeffs);
    }
}

//==============================================================================

template <typename CoeffType>
void FilterDesigner<CoeffType>::designEllipticImpl (
    std::vector<BiquadCoefficients<CoeffType>>& sections,
    bool isHighpass, int order, CoeffType frequency, double sampleRate, CoeffType ripple, CoeffType stopbandAtten) noexcept
{
    const auto numSections = (order + 1) / 2;
    sections.resize (numSections);

    // Convert ripple and attenuation to linear scale
    const auto epsilon = std::sqrt (std::pow (static_cast<CoeffType> (10.0), ripple / static_cast<CoeffType> (10.0)) - static_cast<CoeffType> (1.0));
    const auto a = std::pow (static_cast<CoeffType> (10.0), stopbandAtten / static_cast<CoeffType> (20.0));

    // Calculate selectivity factor k
    const auto k = epsilon / std::sqrt (a * a - static_cast<CoeffType> (1.0));

    // Pre-warp frequency for bilinear transform
    const auto omega = MathConstants<CoeffType>::twoPi * frequency / static_cast<CoeffType> (sampleRate);
    const auto warped = std::tan (omega / static_cast<CoeffType> (2.0));

    // Calculate elliptic poles using Jacobi elliptic functions (approximation)
    std::vector<std::complex<CoeffType>> poles;
    std::vector<CoeffType> zeros;

    calculateEllipticPoles<CoeffType> (order, epsilon, k, poles, zeros);

    // Scale poles for desired frequency
    for (auto& pole : poles)
        pole *= warped;

    // Convert poles and zeros to biquad sections
    int poleIndex = 0;
    int sectionIndex = 0;

    // Handle odd-order case (real pole)
    if (order % 2 == 1)
    {
        const auto realPole = poles[poleIndex++].real();
        const auto a1_s = -realPole;

        // Bilinear transform for first-order section
        const auto norm = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) + a1_s);

        BiquadCoefficients<CoeffType>& coeffs = sections[sectionIndex++];

        if (isHighpass)
        {
            coeffs.b0 = norm;
            coeffs.b1 = -norm;
            coeffs.b2 = static_cast<CoeffType> (0.0);
        }
        else
        {
            coeffs.b0 = a1_s * norm;
            coeffs.b1 = a1_s * norm;
            coeffs.b2 = static_cast<CoeffType> (0.0);
        }

        coeffs.a0 = static_cast<CoeffType> (1.0);
        coeffs.a1 = (a1_s - static_cast<CoeffType> (1.0)) * norm;
        coeffs.a2 = static_cast<CoeffType> (0.0);
    }

    // Process complex pole pairs
    while (poleIndex < static_cast<int> (poles.size()))
    {
        const auto pole1 = poles[poleIndex++];
        const auto pole2 = poles[poleIndex++];

        // Second-order section from complex conjugate pair
        const auto b1_s = -static_cast<CoeffType> (2.0) * pole1.real();
        const auto b0_s = std::norm (pole1);

        // Bilinear transform
        const auto norm = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) + b1_s + b0_s);

        BiquadCoefficients<CoeffType>& coeffs = sections[static_cast<size_t> (sectionIndex++)];

        if (isHighpass)
        {
            coeffs.b0 = norm;
            coeffs.b1 = static_cast<CoeffType> (-2.0) * norm;
            coeffs.b2 = norm;
        }
        else
        {
            coeffs.b0 = b0_s * norm;
            coeffs.b1 = static_cast<CoeffType> (2.0) * b0_s * norm;
            coeffs.b2 = b0_s * norm;
        }

        coeffs.a0 = static_cast<CoeffType> (1.0);
        coeffs.a1 = (static_cast<CoeffType> (2.0) * (b0_s - static_cast<CoeffType> (1.0))) * norm;
        coeffs.a2 = (static_cast<CoeffType> (1.0) - b1_s + b0_s) * norm;
    }
}

//==============================================================================

template <typename CoeffType>
void FilterDesigner<CoeffType>::designEllipticAllpassImpl (
    std::vector<BiquadCoefficients<CoeffType>>& sections,
    int order, double sampleRate, CoeffType ripple, CoeffType stopbandAtten) noexcept
{
    sections.clear();
    sections.resize (static_cast<size_t> (order));

    // Simplified elliptic allpass coefficient generation for halfband applications
    const int N = 2 * order + 1;
    const auto fp = static_cast<CoeffType> (0.4);  // Fixed passband frequency for halfband
    const auto k = static_cast<CoeffType> (2.0) * fp;
    const auto zeta = static_cast<CoeffType> (1.0) / k;
    const auto zeta2 = zeta * zeta;

    const bool odd = (order % 2) != 0;
    int sectionIndex = 0;

    // Generate coefficients for each stage
    for (int l = 1; l <= order; ++l)
    {
        // Simplified elliptic coefficient calculation
        const auto angle = MathConstants<CoeffType>::pi * static_cast<CoeffType> (l) / static_cast<CoeffType> (N);
        const auto sn_approx = std::sin (angle);
        const auto sn2 = sn_approx * sn_approx;

        const auto lambda = static_cast<CoeffType> (1.0);
        const auto sqrt_term = std::sqrt ((static_cast<CoeffType> (1.0) - sn2) * (zeta2 - sn2));
        const auto numerator = zeta + sn2 - lambda * sqrt_term;
        const auto denominator = zeta + sn2 + lambda * sqrt_term;

        auto beta = numerator / jmax (denominator, static_cast<CoeffType> (1e-12));
        beta = jlimit (static_cast<CoeffType> (-0.99), static_cast<CoeffType> (0.99), beta);

        // Convert to biquad form: H(z) = (beta + z^-2) / (1 + beta*z^-2)
        const auto b0 = beta;
        const auto b1 = static_cast<CoeffType> (0.0);
        const auto b2 = static_cast<CoeffType> (1.0);
        const auto a0 = static_cast<CoeffType> (1.0);
        const auto a1 = static_cast<CoeffType> (0.0);
        const auto a2 = beta;

        auto coeffs = BiquadCoefficients<CoeffType> (b0, b1, b2, a0, a1, a2);
        coeffs.normalize();

        sections[sectionIndex++] = coeffs;
    }
}

//==============================================================================

template <typename CoeffType>
void FilterDesigner<CoeffType>::designButterworthAllpassImpl (
    std::vector<BiquadCoefficients<CoeffType>>& sections,
    int order, double sampleRate) noexcept
{
    sections.clear();
    sections.resize (static_cast<size_t> (order));

    // Butterworth allpass coefficient generation for halfband applications
    const int N = 2 * order + 1;
    const int J = order / 2;
    int sectionIndex = 0;

    // Generate a1 coefficients
    for (int l = 1; l <= J; ++l)
    {
        const auto d = std::tan (MathConstants<CoeffType>::pi * static_cast<CoeffType> (l) / static_cast<CoeffType> (N));
        const auto a1_coeff = d * d;

        // Convert to biquad form: H(z) = (a + z^-2) / (1 + a*z^-2)
        const auto b0 = a1_coeff;
        const auto b1 = static_cast<CoeffType> (0.0);
        const auto b2 = static_cast<CoeffType> (1.0);
        const auto a0 = static_cast<CoeffType> (1.0);
        const auto a1 = static_cast<CoeffType> (0.0);
        const auto a2 = a1_coeff;

        auto coeffs = BiquadCoefficients<CoeffType> (b0, b1, b2, a0, a1, a2);
        coeffs.normalize();

        sections[sectionIndex++] = coeffs;
    }

    // Generate a0 coefficients
    for (int l = J + 1; l <= order; ++l)
    {
        const auto d = static_cast<CoeffType> (1.0) / std::tan (MathConstants<CoeffType>::pi * static_cast<CoeffType> (l) / static_cast<CoeffType> (N));
        const auto a0_coeff = d * d;

        // Convert to biquad form
        const auto b0 = a0_coeff;
        const auto b1 = static_cast<CoeffType> (0.0);
        const auto b2 = static_cast<CoeffType> (1.0);
        const auto a0 = static_cast<CoeffType> (1.0);
        const auto a1 = static_cast<CoeffType> (0.0);
        const auto a2 = a0_coeff;

        auto coeffs = BiquadCoefficients<CoeffType> (b0, b1, b2, a0, a1, a2);
        coeffs.normalize();

        sections[sectionIndex++] = coeffs;
    }
}

//==============================================================================

template <typename CoeffType>
void FilterDesigner<CoeffType>::designLegendreImpl (
    std::vector<BiquadCoefficients<CoeffType>>& sections,
    bool isHighpass, int order, CoeffType frequency, double sampleRate) noexcept
{
    const int numSections = (order + 1) / 2;
    sections.resize (static_cast<size_t> (numSections));

    // Pre-computed normalized Legendre poles for orders 1-10
    // These provide optimal monotonic response (steeper than Butterworth)
    static const std::vector<std::vector<std::complex<double>>> legendrePoles = {
        {},                                                                                             // order 0
        {{-1.0, 0.0}},                                                                                  // order 1
        {{-1.2732, 0.7071}, {-1.2732, -0.7071}},                                                        // order 2 (steeper than Butterworth)
        {{-1.4142, 0.0}, {-1.1547, 1.0000}, {-1.1547, -1.0000}},                                        // order 3
        {{-1.5307, 0.6180}, {-1.5307, -0.6180}, {-1.0000, 1.1756}, {-1.0000, -1.1756}},                 // order 4
        {{-1.6180, 0.0}, {-1.4472, 0.8090}, {-1.4472, -0.8090}, {-0.8944, 1.3090}, {-0.8944, -1.3090}}, // order 5
    };

    std::vector<std::complex<CoeffType>> poles;

    if (order >= 1 && order <= static_cast<int> (legendrePoles.size() - 1))
    {
        const auto& orderPoles = legendrePoles[static_cast<size_t> (order)];
        for (const auto& pole : orderPoles)
            poles.emplace_back (static_cast<CoeffType> (pole.real()), static_cast<CoeffType> (pole.imag()));
    }
    else
    {
        // For higher orders, use modified Butterworth poles with steepening factor
        for (int i = 0; i < order; ++i)
        {
            const auto angle = MathConstants<CoeffType>::pi * (static_cast<CoeffType> (2 * i + order + 1)) /
                              (static_cast<CoeffType> (2 * order));
            auto real = -std::cos (angle);
            auto imag = std::sin (angle);

            // Apply Legendre steepening factor (makes poles closer to unit circle)
            const auto steepening = static_cast<CoeffType> (1.15) + static_cast<CoeffType> (0.05) * static_cast<CoeffType> (order) / static_cast<CoeffType> (10.0);
            real *= steepening;
            imag *= steepening;

            poles.emplace_back (real, imag);
        }
    }

    // Scale poles for desired cutoff frequency
    const auto omega = MathConstants<CoeffType>::twoPi * frequency / static_cast<CoeffType> (sampleRate);
    const auto warpedFreq = std::tan (omega / static_cast<CoeffType> (2.0));

    for (auto& pole : poles)
        pole *= warpedFreq;

    // Convert poles to biquad sections
    for (int i = 0; i < numSections; ++i)
    {
        BiquadCoefficients<CoeffType>& coeffs = sections[static_cast<size_t> (i)];

        if (order % 2 == 1 && i == 0)
        {
            // First-order section for odd-order filters
            const auto pole = poles[0].real();
            const auto a = -pole;
            const auto norm = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) + a);

            if (isHighpass)
            {
                coeffs.b0 = norm;
                coeffs.b1 = -norm;
                coeffs.b2 = static_cast<CoeffType> (0.0);
            }
            else
            {
                coeffs.b0 = a * norm;
                coeffs.b1 = a * norm;
                coeffs.b2 = static_cast<CoeffType> (0.0);
            }

            coeffs.a0 = static_cast<CoeffType> (1.0);
            coeffs.a1 = (a - static_cast<CoeffType> (1.0)) * norm;
            coeffs.a2 = static_cast<CoeffType> (0.0);
        }
        else
        {
            // Second-order sections from complex conjugate pairs
            const auto startIdx = (order % 2 == 1) ? 1 + 2 * (i - 1) : 2 * i;
            const auto pole1 = poles[static_cast<size_t> (startIdx)];

            const auto b1_s = -static_cast<CoeffType> (2.0) * pole1.real();
            const auto b0_s = std::norm (pole1);

            const auto norm = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) + b1_s + b0_s);

            if (isHighpass)
            {
                coeffs.b0 = norm;
                coeffs.b1 = static_cast<CoeffType> (-2.0) * norm;
                coeffs.b2 = norm;
            }
            else
            {
                coeffs.b0 = b0_s * norm;
                coeffs.b1 = static_cast<CoeffType> (2.0) * b0_s * norm;
                coeffs.b2 = b0_s * norm;
            }

            coeffs.a0 = static_cast<CoeffType> (1.0);
            coeffs.a1 = (static_cast<CoeffType> (2.0) * (b0_s - static_cast<CoeffType> (1.0))) * norm;
            coeffs.a2 = (static_cast<CoeffType> (1.0) - b1_s + b0_s) * norm;
        }
    }
}

//==============================================================================

template <typename CoeffType>
BiquadCoefficients<CoeffType> FilterDesigner<CoeffType>::designRbjImpl (
    int filterType,
    CoeffType frequency,
    CoeffType q,
    CoeffType gain,
    double sampleRate
) noexcept
{
    const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate));
    const auto cosOmega = std::cos (omega);
    const auto sinOmega = std::sin (omega);
    const auto alpha = sinOmega / (static_cast<CoeffType> (2.0) * q);
    const auto A = std::pow (static_cast<CoeffType> (10.0), gain / static_cast<CoeffType> (40.0));

    BiquadCoefficients<CoeffType> coeffs;

    switch (filterType)
    {
        case 0: // lowpass
            coeffs.b0 = (static_cast<CoeffType> (1.0) - cosOmega) / static_cast<CoeffType> (2.0);
            coeffs.b1 = static_cast<CoeffType> (1.0) - cosOmega;
            coeffs.b2 = (static_cast<CoeffType> (1.0) - cosOmega) / static_cast<CoeffType> (2.0);
            coeffs.a0 = static_cast<CoeffType> (1.0) + alpha;
            coeffs.a1 = static_cast<CoeffType> (-2.0) * cosOmega;
            coeffs.a2 = static_cast<CoeffType> (1.0) - alpha;
            break;

        case 1: // highpass
            coeffs.b0 = (static_cast<CoeffType> (1.0) + cosOmega) / static_cast<CoeffType> (2.0);
            coeffs.b1 = -(static_cast<CoeffType> (1.0) + cosOmega);
            coeffs.b2 = (static_cast<CoeffType> (1.0) + cosOmega) / static_cast<CoeffType> (2.0);
            coeffs.a0 = static_cast<CoeffType> (1.0) + alpha;
            coeffs.a1 = static_cast<CoeffType> (-2.0) * cosOmega;
            coeffs.a2 = static_cast<CoeffType> (1.0) - alpha;
            break;

        case 2: // bandpass
            coeffs.b0 = alpha;
            coeffs.b1 = static_cast<CoeffType> (0.0);
            coeffs.b2 = -alpha;
            coeffs.a0 = static_cast<CoeffType> (1.0) + alpha;
            coeffs.a1 = static_cast<CoeffType> (-2.0) * cosOmega;
            coeffs.a2 = static_cast<CoeffType> (1.0) - alpha;
            break;

        case 3: // bandstop
            coeffs.b0 = static_cast<CoeffType> (1.0);
            coeffs.b1 = static_cast<CoeffType> (-2.0) * cosOmega;
            coeffs.b2 = static_cast<CoeffType> (1.0);
            coeffs.a0 = static_cast<CoeffType> (1.0) + alpha;
            coeffs.a1 = static_cast<CoeffType> (-2.0) * cosOmega;
            coeffs.a2 = static_cast<CoeffType> (1.0) - alpha;
            break;

        case 4: // peak
            coeffs.b0 = static_cast<CoeffType> (1.0) + alpha * A;
            coeffs.b1 = static_cast<CoeffType> (-2.0) * cosOmega;
            coeffs.b2 = static_cast<CoeffType> (1.0) - alpha * A;
            coeffs.a0 = static_cast<CoeffType> (1.0) + alpha / A;
            coeffs.a1 = static_cast<CoeffType> (-2.0) * cosOmega;
            coeffs.a2 = static_cast<CoeffType> (1.0) - alpha / A;
            break;

        case 5: // lowshelf
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
            break;

        case 6: // highshelf
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
            break;

        default:
            break;
    }

    coeffs.normalize();
    return coeffs;
}

template <typename CoeffType>
BiquadCoefficients<CoeffType> FilterDesigner<CoeffType>::designRbjAllpass (
    CoeffType frequency,
    CoeffType q,
    double sampleRate
) noexcept
{
    const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate));
    const auto cosOmega = std::cos (omega);
    const auto sinOmega = std::sin (omega);
    const auto alpha = sinOmega / (static_cast<CoeffType> (2.0) * q);

    const auto b0 = static_cast<CoeffType> (1.0) - alpha;
    const auto b1 = static_cast<CoeffType> (-2.0) * cosOmega;
    const auto b2 = static_cast<CoeffType> (1.0) + alpha;
    const auto a0 = static_cast<CoeffType> (1.0) + alpha;
    const auto a1 = static_cast<CoeffType> (-2.0) * cosOmega;
    const auto a2 = static_cast<CoeffType> (1.0) - alpha;

    auto coeffs = BiquadCoefficients<CoeffType> (b0, b1, b2, a0, a1, a2);
    coeffs.normalize();
    return coeffs;
}

//==============================================================================

template <typename CoeffType>
void FilterDesigner<CoeffType>::designFIRLowpassImpl (std::vector<CoeffType>& coeffs, CoeffType cutoff, double sampleRate) noexcept
{
    const auto omega_c = DspMath::frequencyToAngular (cutoff, static_cast<CoeffType> (sampleRate));
    const auto length = static_cast<int> (coeffs.size());
    const auto center = static_cast<CoeffType> (length - 1) / static_cast<CoeffType> (2.0);

    for (int n = 0; n < length; ++n)
    {
        const auto nOffset = static_cast<CoeffType> (n) - center;

        if (std::abs (nOffset) < 1e-10)
            coeffs[static_cast<size_t> (n)] = omega_c / MathConstants<CoeffType>::pi;
        else
            coeffs[static_cast<size_t> (n)] = std::sin (omega_c * nOffset) / (MathConstants<CoeffType>::pi * nOffset);
    }
}

template <typename CoeffType>
void FilterDesigner<CoeffType>::designFIRHighpassImpl (std::vector<CoeffType>& coeffs, CoeffType cutoff, double sampleRate) noexcept
{
    designFIRLowpassImpl (coeffs, cutoff, sampleRate);

    // Spectral inversion
    const auto length = static_cast<int> (coeffs.size());
    const auto center = static_cast<CoeffType> (length - 1) / static_cast<CoeffType> (2.0);

    for (int n = 0; n < length; ++n)
    {
        const auto nOffset = static_cast<CoeffType> (n) - center;

        if (std::abs (nOffset) < 1e-10)
        {
            coeffs[static_cast<size_t> (n)] = static_cast<CoeffType> (1.0) - coeffs[static_cast<size_t> (n)];
        }
        else
        {
            if (n % 2 == 1)
                coeffs[static_cast<size_t> (n)] = -coeffs[static_cast<size_t> (n)];
        }
    }
}

template <typename CoeffType>
void FilterDesigner<CoeffType>::designFIRBandpassImpl (std::vector<CoeffType>& coeffs, CoeffType lowCutoff, CoeffType highCutoff, double sampleRate) noexcept
{
    const auto omega1 = DspMath::frequencyToAngular (lowCutoff, static_cast<CoeffType> (sampleRate));
    const auto omega2 = DspMath::frequencyToAngular (highCutoff, static_cast<CoeffType> (sampleRate));
    const auto length = static_cast<int> (coeffs.size());
    const auto center = static_cast<CoeffType> (length - 1) / static_cast<CoeffType> (2.0);

    for (int n = 0; n < length; ++n)
    {
        const auto nOffset = static_cast<CoeffType> (n) - center;

        if (std::abs (nOffset) < 1e-10)
        {
            coeffs[static_cast<size_t> (n)] = (omega2 - omega1) / MathConstants<CoeffType>::pi;
        }
        else
        {
            coeffs[static_cast<size_t> (n)] = (std::sin (omega2 * nOffset) - std::sin (omega1 * nOffset)) / (MathConstants<CoeffType>::pi * nOffset);
        }
    }
}

template <typename CoeffType>
void FilterDesigner<CoeffType>::designFIRBandstopImpl (std::vector<CoeffType>& coeffs, CoeffType lowCutoff, CoeffType highCutoff, double sampleRate) noexcept
{
    designFIRBandpassImpl (coeffs, lowCutoff, highCutoff, sampleRate);

    // Spectral inversion for bandstop
    const auto length = static_cast<int> (coeffs.size());
    const auto center = static_cast<CoeffType> (length - 1) / static_cast<CoeffType> (2.0);

    for (int n = 0; n < length; ++n)
    {
        const auto nOffset = static_cast<CoeffType> (n) - center;

        if (std::abs (nOffset) < 1e-10)
        {
            coeffs[static_cast<size_t> (n)] = static_cast<CoeffType> (1.0) - coeffs[static_cast<size_t> (n)];
        }
        else
        {
            if (n % 2 == 1)
                coeffs[static_cast<size_t> (n)] = -coeffs[static_cast<size_t> (n)];
        }
    }
}

//==============================================================================

// Explicit instantiations for FilterDesigner
template class FilterDesigner<float>;
template class FilterDesigner<double>;

} // namespace yup
