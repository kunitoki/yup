// Copyright (C) 2020-2026 Parabola Research Limited
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Assert.h"

#include <limits>
#include <type_traits>

namespace Bungee {

template <class T>
constexpr int bit_width(T x) noexcept
{
    static_assert(std::is_integral_v<T>, "T must be an integral type");
    using U = std::make_unsigned_t<T>;

    U u = static_cast<U>(x);

    if (u == 0)
        return 0;

    int width = 0;
    while (u != 0)
    {
        u >>= 1;
        ++width;
    }

    return width;
}

template <class T>
constexpr int countr_zero(T x) noexcept
{
    static_assert(std::is_integral_v<T>, "T must be an integral type");
    using U = std::make_unsigned_t<T>;
    U u = static_cast<U>(x);

    if (u == 0)
        return std::numeric_limits<U>::digits; // number of bits in type

    int count = 0;
    while ((u & 1) == 0)
    {
        u >>= 1;
        ++count;
    }

    return count;
}

template <bool floor = false>
static inline int log2(unsigned x)
{
	BUNGEE_ASSERT1(x > 0);
	BUNGEE_ASSERT1(floor || !(x & (x << 1)));

	int y;
	if constexpr (floor)
		y = bit_width(x) - 1;
	else
		y = countr_zero(x);

	BUNGEE_ASSERT1(floor ? (1 << y <= x && x < 2 << y) : (x == 1 << y));
	return y;
}

template <int x>
constexpr int log2(std::integral_constant<int, x>)
{
	static_assert(x > 0);
	static_assert(!(x & (x - 1)));

	if constexpr (x == 1)
		return 0;
	else
		return 1 + log2(std::integral_constant<int, x / 2>{});
}

} // namespace Bungee
