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

#include <cmath>
#include <complex>
#include <vector>

namespace yup
{

//==============================================================================

/** Mathematical constants and utility functions for DSP operations. */
namespace DspMath
{

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

/** Complex number type alias using std::complex */
template <typename FloatType>
using Complex = std::complex<FloatType>;

/** Creates a complex number from magnitude and phase */
template <typename FloatType>
constexpr Complex<FloatType> polar (FloatType magnitude, FloatType phase) noexcept
{
    return std::polar (magnitude, phase);
}

} // namespace DspMath
} // namespace yup
