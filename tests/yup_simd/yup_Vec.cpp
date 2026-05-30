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

#include <gtest/gtest.h>

#include <yup_simd/yup_simd.h>

#include <cmath>

using namespace yup;

// ==============================================================================
// Vec2f tests
// ==============================================================================

TEST (Vec2fTests, DefaultConstructorZeroInitialises)
{
    const Vec2f v;
    EXPECT_FLOAT_EQ (v.x, 0.0f);
    EXPECT_FLOAT_EQ (v.y, 0.0f);
}

TEST (Vec2fTests, ParameterisedConstructor)
{
    const Vec2f v (3.0f, -7.5f);
    EXPECT_FLOAT_EQ (v.x, 3.0f);
    EXPECT_FLOAT_EQ (v.y, -7.5f);
}

TEST (Vec2fTests, Addition)
{
    const Vec2f a (1.0f, 2.0f);
    const Vec2f b (3.0f, 4.0f);
    const Vec2f r = a + b;
    EXPECT_FLOAT_EQ (r.x, 4.0f);
    EXPECT_FLOAT_EQ (r.y, 6.0f);
}

TEST (Vec2fTests, AdditionWithNegatives)
{
    const Vec2f a (5.0f, -3.0f);
    const Vec2f b (-2.0f, 7.0f);
    const Vec2f r = a + b;
    EXPECT_FLOAT_EQ (r.x, 3.0f);
    EXPECT_FLOAT_EQ (r.y, 4.0f);
}

TEST (Vec2fTests, AdditionIdentity)
{
    const Vec2f a (10.0f, -5.0f);
    const Vec2f zero;
    const Vec2f r = a + zero;
    EXPECT_FLOAT_EQ (r.x, a.x);
    EXPECT_FLOAT_EQ (r.y, a.y);
}

TEST (Vec2fTests, ScalarMultiplication)
{
    const Vec2f v (2.0f, -4.0f);
    const Vec2f r = v * 3.0f;
    EXPECT_FLOAT_EQ (r.x, 6.0f);
    EXPECT_FLOAT_EQ (r.y, -12.0f);
}

TEST (Vec2fTests, ScalarMultiplicationByZero)
{
    const Vec2f v (99.0f, -77.0f);
    const Vec2f r = v * 0.0f;
    EXPECT_FLOAT_EQ (r.x, 0.0f);
    EXPECT_FLOAT_EQ (r.y, 0.0f);
}

TEST (Vec2fTests, ScalarMultiplicationByOne)
{
    const Vec2f v (3.5f, -1.5f);
    const Vec2f r = v * 1.0f;
    EXPECT_FLOAT_EQ (r.x, v.x);
    EXPECT_FLOAT_EQ (r.y, v.y);
}

TEST (Vec2fTests, DotProduct)
{
    const Vec2f a (1.0f, 0.0f);
    const Vec2f b (0.0f, 1.0f);
    EXPECT_FLOAT_EQ (a.dot (b), 0.0f); // orthogonal
}

TEST (Vec2fTests, DotProductCollinear)
{
    const Vec2f a (1.0f, 0.0f);
    EXPECT_FLOAT_EQ (a.dot (a), 1.0f);
}

TEST (Vec2fTests, DotProductGeneral)
{
    // (1,2)·(3,4) = 1*3 + 2*4 = 11
    const Vec2f a (1.0f, 2.0f);
    const Vec2f b (3.0f, 4.0f);
    EXPECT_FLOAT_EQ (a.dot (b), 11.0f);
}

TEST (Vec2fTests, LengthOf345Triangle)
{
    const Vec2f v (3.0f, 4.0f);
    EXPECT_NEAR (v.length(), 5.0f, 1.0e-5f);
}

TEST (Vec2fTests, LengthOfZeroVector)
{
    const Vec2f v;
    EXPECT_NEAR (v.length(), 0.0f, 1.0e-7f);
}

TEST (Vec2fTests, LengthOfUnitVector)
{
    const Vec2f v (1.0f, 0.0f);
    EXPECT_NEAR (v.length(), 1.0f, 1.0e-7f);
}

TEST (Vec2fTests, NormalizedHasUnitLength)
{
    const Vec2f v (3.0f, 4.0f);
    const Vec2f n = v.normalized();
    EXPECT_NEAR (n.length(), 1.0f, 1.0e-6f);
}

TEST (Vec2fTests, NormalizedDirection)
{
    const Vec2f v (3.0f, 4.0f);
    const Vec2f n = v.normalized();
    EXPECT_NEAR (n.x, 0.6f, 1.0e-6f);
    EXPECT_NEAR (n.y, 0.8f, 1.0e-6f);
}

TEST (Vec2fTests, NormalizedZeroVectorReturnsZero)
{
    const Vec2f v;
    const Vec2f n = v.normalized();
    EXPECT_FLOAT_EQ (n.x, 0.0f);
    EXPECT_FLOAT_EQ (n.y, 0.0f);
}

// ==============================================================================
// Vec4f tests
// ==============================================================================

TEST (Vec4fTests, DefaultConstructorZeroInitialises)
{
    const Vec4f v;
    EXPECT_FLOAT_EQ (v.r, 0.0f);
    EXPECT_FLOAT_EQ (v.g, 0.0f);
    EXPECT_FLOAT_EQ (v.b, 0.0f);
    EXPECT_FLOAT_EQ (v.a, 0.0f);
}

TEST (Vec4fTests, ParameterisedConstructor)
{
    const Vec4f v (0.1f, 0.2f, 0.3f, 1.0f);
    EXPECT_FLOAT_EQ (v.r, 0.1f);
    EXPECT_FLOAT_EQ (v.g, 0.2f);
    EXPECT_FLOAT_EQ (v.b, 0.3f);
    EXPECT_FLOAT_EQ (v.a, 1.0f);
}

TEST (Vec4fTests, Addition)
{
    const Vec4f a (0.1f, 0.2f, 0.3f, 0.4f);
    const Vec4f b (0.4f, 0.3f, 0.2f, 0.1f);
    const Vec4f r = a + b;
    EXPECT_NEAR (r.r, 0.5f, 1.0e-6f);
    EXPECT_NEAR (r.g, 0.5f, 1.0e-6f);
    EXPECT_NEAR (r.b, 0.5f, 1.0e-6f);
    EXPECT_NEAR (r.a, 0.5f, 1.0e-6f);
}

TEST (Vec4fTests, AdditionIdentity)
{
    const Vec4f a (1.0f, 2.0f, 3.0f, 4.0f);
    const Vec4f zero;
    const Vec4f r = a + zero;
    EXPECT_NEAR (r.r, a.r, 1.0e-6f);
    EXPECT_NEAR (r.g, a.g, 1.0e-6f);
    EXPECT_NEAR (r.b, a.b, 1.0e-6f);
    EXPECT_NEAR (r.a, a.a, 1.0e-6f);
}

TEST (Vec4fTests, ScalarMultiplication)
{
    const Vec4f v (1.0f, 2.0f, 3.0f, 4.0f);
    const Vec4f r = v * 2.0f;
    EXPECT_NEAR (r.r, 2.0f, 1.0e-6f);
    EXPECT_NEAR (r.g, 4.0f, 1.0e-6f);
    EXPECT_NEAR (r.b, 6.0f, 1.0e-6f);
    EXPECT_NEAR (r.a, 8.0f, 1.0e-6f);
}

TEST (Vec4fTests, ScalarMultiplicationByZero)
{
    const Vec4f v (9.0f, 8.0f, 7.0f, 6.0f);
    const Vec4f r = v * 0.0f;
    EXPECT_NEAR (r.r, 0.0f, 1.0e-6f);
    EXPECT_NEAR (r.g, 0.0f, 1.0e-6f);
    EXPECT_NEAR (r.b, 0.0f, 1.0e-6f);
    EXPECT_NEAR (r.a, 0.0f, 1.0e-6f);
}

TEST (Vec4fTests, ScalarMultiplicationByOne)
{
    const Vec4f v (5.0f, 6.0f, 7.0f, 8.0f);
    const Vec4f r = v * 1.0f;
    EXPECT_NEAR (r.r, v.r, 1.0e-6f);
    EXPECT_NEAR (r.g, v.g, 1.0e-6f);
    EXPECT_NEAR (r.b, v.b, 1.0e-6f);
    EXPECT_NEAR (r.a, v.a, 1.0e-6f);
}

TEST (Vec4fTests, ElementWiseMultiplication)
{
    const Vec4f a (2.0f, 3.0f, 4.0f, 5.0f);
    const Vec4f b (1.5f, 2.0f, 0.5f, 0.25f);
    const Vec4f r = a * b;
    EXPECT_NEAR (r.r, 3.0f, 1.0e-6f);
    EXPECT_NEAR (r.g, 6.0f, 1.0e-6f);
    EXPECT_NEAR (r.b, 2.0f, 1.0e-6f);
    EXPECT_NEAR (r.a, 1.25f, 1.0e-6f);
}

TEST (Vec4fTests, ElementWiseMultiplicationByZeroVec)
{
    const Vec4f a (3.0f, 4.0f, 5.0f, 6.0f);
    const Vec4f zero;
    const Vec4f r = a * zero;
    EXPECT_NEAR (r.r, 0.0f, 1.0e-6f);
    EXPECT_NEAR (r.g, 0.0f, 1.0e-6f);
    EXPECT_NEAR (r.b, 0.0f, 1.0e-6f);
    EXPECT_NEAR (r.a, 0.0f, 1.0e-6f);
}

TEST (Vec4fTests, LerpAtZeroReturnsThis)
{
    const Vec4f a (0.0f, 0.0f, 0.0f, 1.0f);
    const Vec4f b (1.0f, 1.0f, 1.0f, 1.0f);
    const Vec4f r = a.lerp (b, 0.0f);
    EXPECT_NEAR (r.r, a.r, 1.0e-5f);
    EXPECT_NEAR (r.g, a.g, 1.0e-5f);
    EXPECT_NEAR (r.b, a.b, 1.0e-5f);
    EXPECT_NEAR (r.a, a.a, 1.0e-5f);
}

TEST (Vec4fTests, LerpAtOneReturnsOther)
{
    const Vec4f a (0.0f, 0.0f, 0.0f, 1.0f);
    const Vec4f b (1.0f, 0.5f, 0.25f, 0.75f);
    const Vec4f r = a.lerp (b, 1.0f);
    EXPECT_NEAR (r.r, b.r, 1.0e-5f);
    EXPECT_NEAR (r.g, b.g, 1.0e-5f);
    EXPECT_NEAR (r.b, b.b, 1.0e-5f);
    EXPECT_NEAR (r.a, b.a, 1.0e-5f);
}

TEST (Vec4fTests, LerpAtHalfIsMidpoint)
{
    const Vec4f a (0.0f, 0.0f, 0.0f, 0.0f);
    const Vec4f b (2.0f, 4.0f, 6.0f, 8.0f);
    const Vec4f r = a.lerp (b, 0.5f);
    EXPECT_NEAR (r.r, 1.0f, 1.0e-5f);
    EXPECT_NEAR (r.g, 2.0f, 1.0e-5f);
    EXPECT_NEAR (r.b, 3.0f, 1.0e-5f);
    EXPECT_NEAR (r.a, 4.0f, 1.0e-5f);
}

TEST (Vec4fTests, LerpQuarterPoint)
{
    const Vec4f a (0.0f, 0.0f, 0.0f, 0.0f);
    const Vec4f b (4.0f, 8.0f, 12.0f, 16.0f);
    const Vec4f r = a.lerp (b, 0.25f);
    EXPECT_NEAR (r.r, 1.0f, 1.0e-5f);
    EXPECT_NEAR (r.g, 2.0f, 1.0e-5f);
    EXPECT_NEAR (r.b, 3.0f, 1.0e-5f);
    EXPECT_NEAR (r.a, 4.0f, 1.0e-5f);
}

TEST (Vec4fTests, PremultipliedFullAlpha)
{
    // alpha=1: RGB unchanged
    const Vec4f v (0.5f, 0.25f, 0.75f, 1.0f);
    const Vec4f p = v.premultiplied();
    EXPECT_NEAR (p.r, 0.5f, 1.0e-6f);
    EXPECT_NEAR (p.g, 0.25f, 1.0e-6f);
    EXPECT_NEAR (p.b, 0.75f, 1.0e-6f);
    EXPECT_NEAR (p.a, 1.0f, 1.0e-6f);
}

TEST (Vec4fTests, PremultipliedZeroAlpha)
{
    // alpha=0: all channels become 0
    const Vec4f v (1.0f, 1.0f, 1.0f, 0.0f);
    const Vec4f p = v.premultiplied();
    EXPECT_NEAR (p.r, 0.0f, 1.0e-6f);
    EXPECT_NEAR (p.g, 0.0f, 1.0e-6f);
    EXPECT_NEAR (p.b, 0.0f, 1.0e-6f);
    EXPECT_NEAR (p.a, 0.0f, 1.0e-6f);
}

TEST (Vec4fTests, PremultipliedHalfAlpha)
{
    // alpha=0.5: each channel halved
    const Vec4f v (0.8f, 0.6f, 0.4f, 0.5f);
    const Vec4f p = v.premultiplied();
    EXPECT_NEAR (p.r, 0.4f, 1.0e-6f);
    EXPECT_NEAR (p.g, 0.3f, 1.0e-6f);
    EXPECT_NEAR (p.b, 0.2f, 1.0e-6f);
    EXPECT_NEAR (p.a, 0.5f, 1.0e-6f);
}

TEST (Vec4fTests, LoadAndStore)
{
    const float src[] = { 1.0f, 2.0f, 3.0f, 4.0f };
    const Vec4f v = Vec4f::load (src);
    EXPECT_FLOAT_EQ (v.r, 1.0f);
    EXPECT_FLOAT_EQ (v.g, 2.0f);
    EXPECT_FLOAT_EQ (v.b, 3.0f);
    EXPECT_FLOAT_EQ (v.a, 4.0f);

    float dst[4] = {};
    v.store (dst);
    EXPECT_FLOAT_EQ (dst[0], 1.0f);
    EXPECT_FLOAT_EQ (dst[1], 2.0f);
    EXPECT_FLOAT_EQ (dst[2], 3.0f);
    EXPECT_FLOAT_EQ (dst[3], 4.0f);
}

TEST (Vec4fTests, LoadRoundTrip)
{
    const float original[] = { 7.5f, -3.25f, 0.0f, 1.0f };
    float result[4] = {};
    Vec4f::load (original).store (result);
    for (int i = 0; i < 4; ++i)
        EXPECT_FLOAT_EQ (result[i], original[i]);
}
