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

namespace yup
{

//==============================================================================
namespace
{

// The density exponent stops growing here, so the fog far below its base stays finite
constexpr float maxFogDensityExponent = 40.0f;

float getFogDensityExponent (const WorldFog& fog, float height) noexcept
{
    return -fog.heightFalloff * (height - fog.baseHeight);
}

} // namespace

//==============================================================================

float WorldFog::getDensity (float height) const noexcept
{
    if (! enabled || density <= 0.0f)
        return 0.0f;

    return density * std::exp (jmin (maxFogDensityExponent, getFogDensityExponent (*this, height)));
}

float WorldFog::getOpticalDepth (const Vector3<float>& from, const Vector3<float>& to) const noexcept
{
    if (! enabled || density <= 0.0f)
        return 0.0f;

    const auto length = (to - from).length();
    const auto fromExponent = getFogDensityExponent (*this, from.getY());
    const auto toExponent = getFogDensityExponent (*this, to.getY());
    const auto thinExponent = jmin (fromExponent, toExponent);
    const auto denseExponent = jmax (fromExponent, toExponent);

    if (thinExponent >= maxFogDensityExponent)
        return density * std::exp (maxFogDensityExponent) * length;

    // The exponent is linear along the segment: the fraction past the clamp has a uniform density
    const auto clampedFraction = denseExponent > maxFogDensityExponent
                                   ? (denseExponent - maxFogDensityExponent) / (denseExponent - thinExponent)
                                   : 0.0f;

    // Integrated from the dense end, so the exponential decays: (1 - e^-drop) / drop, 1 - drop / 2 near zero slope
    const auto clampedDenseExponent = jmin (denseExponent, maxFogDensityExponent);
    const auto drop = clampedDenseExponent - thinExponent;
    const auto decay = drop < 1.0e-4f ? 1.0f - 0.5f * drop : -std::expm1 (-drop) / drop;

    return density * std::exp (clampedDenseExponent) * length * ((1.0f - clampedFraction) * decay + clampedFraction);
}

} // namespace yup
