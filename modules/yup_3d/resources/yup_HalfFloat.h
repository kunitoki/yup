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

} // namespace yup
