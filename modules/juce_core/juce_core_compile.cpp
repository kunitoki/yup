/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

#include <cstdint>
#include <bit>

namespace juce
{

namespace
{
consteval uint64_t constexprHash (uint64_t in) noexcept
{
    constexpr uint64_t r[] {
        0xdf15236c16d16793ull, 0x3a697614e0fe08e4ull, 0xa3a53275ccc10ff9ull, 0xb92fae55ecf491deull, 0x36e867730ed24a6aull, 0xd7153d8084adf386ull, 0x17110e766d411a6aull, 0xcbd41fed4b1d6b30ull
    };

    uint64_t out { in ^ r[in & 0x7] };
    out ^= std::rotl (in, 32) ^ r[(in >> 8) & 0x7];
    out ^= std::rotl (in, 16) ^ r[(in >> 16) & 0x7];
    out ^= std::rotl (in, 8) ^ r[(in >> 24) & 0x7];
    out ^= std::rotl (in, 4) ^ r[(in >> 32) & 0x7];
    out ^= std::rotl (in, 2) ^ r[(in >> 40) & 0x7];
    out ^= std::rotl (in, 1) ^ r[(in >> 48) & 0x7];
    return out;
}

template <size_t N>
consteval uint64_t constexprHash (const char (&str)[N]) noexcept
{
    uint64_t h {};

    for (uint64_t i = 0; i < N; ++i)
        h ^= uint64_t (str[i]) << (i % 8 * 8);

    return constexprHash (h);
}

template <size_t N>
consteval uint64_t constexprRandomImplementation (const char (&file)[N], uint64_t line, uint64_t column = 0x8dc97987) noexcept
{
    return constexprHash (
        constexprHash (__DATE__) ^ constexprHash (__TIME__) ^ constexprHash (file) ^ constexprHash (line) ^ constexprHash (column));
}

#define JUCE_CONSTEXPR_RANDOM [] {                             \
    return constexprRandomImplementation (__FILE__, __LINE__); \
}()
} // namespace

const char* const juce_compilationDate = __DATE__;
const char* const juce_compilationTime = __TIME__;
const uint64_t juce_compilationUniqueId = JUCE_CONSTEXPR_RANDOM;

} // namespace juce
