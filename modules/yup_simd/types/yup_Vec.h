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
struct alignas (8) Vec2f
{
    float x = 0.0f;
    float y = 0.0f;

    constexpr Vec2f() noexcept = default;

    constexpr Vec2f (float xIn, float yIn) noexcept
        : x (xIn)
        , y (yIn)
    {
    }

    constexpr Vec2f operator+ (Vec2f other) const noexcept
    {
        return { x + other.x, y + other.y };
    }

    constexpr Vec2f operator* (float scalar) const noexcept
    {
        return { x * scalar, y * scalar };
    }

    constexpr float dot (Vec2f other) const noexcept
    {
        return x * other.x + y * other.y;
    }

    float length() const noexcept
    {
        return std::sqrt (dot (*this));
    }

    Vec2f normalized() const noexcept
    {
        const auto len = length();
        return len > 0.0f ? *this * (1.0f / len) : Vec2f();
    }
};

//==============================================================================
struct alignas (16) Vec4f
{
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 0.0f;

    constexpr Vec4f() noexcept = default;

    constexpr Vec4f (float rIn, float gIn, float bIn, float aIn) noexcept
        : r (rIn)
        , g (gIn)
        , b (bIn)
        , a (aIn)
    {
    }

    Vec4f operator+ (Vec4f other) const noexcept
    {
        alignas (16) float result[4];
        (Float32x4::loadUnaligned (data()) + Float32x4::loadUnaligned (other.data())).storeUnaligned (result);
        return load (result);
    }

    Vec4f operator* (float scalar) const noexcept
    {
        alignas (16) float result[4];
        (Float32x4::loadUnaligned (data()) * Float32x4::broadcast (scalar)).storeUnaligned (result);
        return load (result);
    }

    Vec4f operator* (Vec4f other) const noexcept
    {
        alignas (16) float result[4];
        (Float32x4::loadUnaligned (data()) * Float32x4::loadUnaligned (other.data())).storeUnaligned (result);
        return load (result);
    }

    Vec4f lerp (Vec4f other, float t) const noexcept
    {
        return *this + (other + (*this * -1.0f)) * t;
    }

    constexpr Vec4f premultiplied() const noexcept
    {
        return { r * a, g * a, b * a, a };
    }

    static Vec4f load (const float* ptr) noexcept
    {
        return { ptr[0], ptr[1], ptr[2], ptr[3] };
    }

    void store (float* ptr) const noexcept
    {
        ptr[0] = r;
        ptr[1] = g;
        ptr[2] = b;
        ptr[3] = a;
    }

private:
    const float* data() const noexcept
    {
        return std::addressof (r);
    }
};

} // namespace yup
