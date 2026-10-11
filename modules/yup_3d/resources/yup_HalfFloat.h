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
/** Converts a float to the bits of the nearest IEEE 754 half float, for rgba16float uploads.

    Values below the smallest normal half (6.1e-5) flush to a signed zero. Values beyond the
    largest finite half, infinities and NaN included, clamp to a signed 65504, so an upload never
    holds a non-finite texel.

    @internal
*/
inline uint16 floatToHalf (float value) noexcept
{
    uint32 bits = 0;
    std::memcpy (&bits, &value, sizeof (bits));

    const auto sign = static_cast<uint16> ((bits >> 16) & 0x8000u);
    const auto exponent = static_cast<int> ((bits >> 23) & 0xffu) - 127 + 15;
    const auto mantissa = bits & 0x7fffffu;

    if (exponent <= 0)
        return sign;

    if (exponent >= 31)
        return static_cast<uint16> (sign | 0x7bffu);

    // Round to nearest
    auto half = static_cast<uint32> (sign) | (static_cast<uint32> (exponent) << 10) | (mantissa >> 13);
    if ((mantissa & 0x1000u) != 0)
        ++half;

    return static_cast<uint16> (jmin (half, static_cast<uint32> (sign | 0x7bffu)));
}

/** Converts the bits of an IEEE 754 half float back to a float, exactly, subnormals included.

    @internal
*/
inline float halfToFloat (uint16 half) noexcept
{
    const auto sign = (half & 0x8000u) != 0 ? -1.0f : 1.0f;
    const auto exponent = static_cast<int> ((half >> 10) & 0x1fu);
    const auto mantissa = static_cast<float> (half & 0x3ffu);

    if (exponent == 0)
        return sign * std::ldexp (mantissa, -24);

    if (exponent == 31)
        return mantissa == 0.0f ? sign * std::numeric_limits<float>::infinity() : std::numeric_limits<float>::quiet_NaN();

    return sign * std::ldexp (1024.0f + mantissa, exponent - 25);
}

//==============================================================================
/** The largest value an RGBM texel holds, YUP_3D_RGBM_RANGE in shaders/lib/color.glsl. */
constexpr float rgbmRange = 8.0f;

/** Encodes linear RGB as the rgba8 texel yupEncodeRgbm() in shaders/lib/color.glsl writes, for
    uploads below the float floor: rgb divided by a shared multiplier stored in alpha, up to
    rgbmRange. The shaders decode it with yupDecodeRgbm().

    @param rgb   Three linear floats.
    @param rgbm  Four bytes receiving the texel.

    @internal
*/
inline void encodeRgbm (const float* rgb, uint8* rgbm) noexcept
{
    auto m = jlimit (0.0f, 1.0f, jmax (rgb[0], rgb[1], jmax (rgb[2], 1.0e-6f * rgbmRange)) / rgbmRange);
    m = std::ceil (m * 255.0f) / 255.0f;

    for (int c = 0; c < 3; ++c)
        rgbm[c] = static_cast<uint8> (std::round (jlimit (0.0f, 1.0f, rgb[c] / rgbmRange / m) * 255.0f));

    rgbm[3] = static_cast<uint8> (std::round (m * 255.0f));
}

} // namespace yup
