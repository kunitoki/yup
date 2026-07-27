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

#include <cstdint>
#include <functional>

namespace yup
{

//==============================================================================

template <class T>
class GenericInteger
{
public:
    using underlying_type = T;

    constexpr GenericInteger() = default;

    constexpr GenericInteger (T value) noexcept
        : value (value)
    {
    }

    constexpr GenericInteger (const GenericInteger& other) = default;
    constexpr GenericInteger (GenericInteger&& other) = default;
    constexpr GenericInteger& operator= (const GenericInteger& other) = default;
    constexpr GenericInteger& operator= (GenericInteger&& other) = default;

    constexpr operator T() const noexcept
    {
        return value;
    }

    constexpr T get() const noexcept
    {
        return value;
    }

private:
    T value {};
};

//==============================================================================

template <class T>
struct underlying_type
{
    using type = T;
};

template <class T>
struct underlying_type<GenericInteger<T>>
{
    using type = typename GenericInteger<T>::underlying_type;
};

template <class T>
using underlying_type_t = typename underlying_type<T>::type;

} // namespace yup
