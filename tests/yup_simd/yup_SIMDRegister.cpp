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

using namespace yup;

// ==============================================================================
// Float4 tests
// ==============================================================================

TEST (SIMDRegisterTests, Float4DefaultConstructorIsZero)
{
    Float4 r;
    float stored[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    r.storeUnaligned (stored);

    for (int i = 0; i < 4; ++i)
        EXPECT_FLOAT_EQ (stored[i], 0.0f);
}

TEST (SIMDRegisterTests, Float4ZeroHelperIsAllZero)
{
    const auto r = Float4::zero();
    float stored[4] = {};
    r.storeUnaligned (stored);

    for (int i = 0; i < 4; ++i)
        EXPECT_FLOAT_EQ (stored[i], 0.0f);
}

TEST (SIMDRegisterTests, Float4BroadcastFillsAllLanes)
{
    const auto r = Float4::broadcast (3.14f);
    float stored[4] = {};
    r.storeUnaligned (stored);

    for (int i = 0; i < 4; ++i)
        EXPECT_FLOAT_EQ (stored[i], 3.14f);
}

TEST (SIMDRegisterTests, Float4ElementAccessOperator)
{
    const float values[4] = { 10.0f, 20.0f, 30.0f, 40.0f };
    const auto r = Float4::loadUnaligned (values);

    EXPECT_FLOAT_EQ (r[0], 10.0f);
    EXPECT_FLOAT_EQ (r[1], 20.0f);
    EXPECT_FLOAT_EQ (r[2], 30.0f);
    EXPECT_FLOAT_EQ (r[3], 40.0f);
}

TEST (SIMDRegisterTests, Float4ArithmeticAndHorizontalOps)
{
    const float aValues[4] = { 1.0f, -2.0f, 3.0f, -4.0f };
    const float bValues[4] = { 5.0f, 6.0f, -7.0f, -8.0f };

    const auto a = Float4::loadUnaligned (aValues);
    const auto b = Float4::loadUnaligned (bValues);
    const auto result = a + b * Float4::broadcast (2.0f);

    float stored[4] = {};
    result.storeUnaligned (stored);

    for (int i = 0; i < 4; ++i)
        EXPECT_FLOAT_EQ (stored[i], aValues[i] + bValues[i] * 2.0f);

    EXPECT_FLOAT_EQ (a.sum(), -2.0f);
    EXPECT_FLOAT_EQ (a.abs().hmax(), 4.0f);
}

TEST (SIMDRegisterTests, Float4Subtraction)
{
    const float aValues[4] = { 10.0f, 20.0f, 30.0f, 40.0f };
    const float bValues[4] = { 1.0f, 3.0f, 5.0f, 7.0f };

    const auto a = Float4::loadUnaligned (aValues);
    const auto b = Float4::loadUnaligned (bValues);
    const auto result = a - b;

    float stored[4] = {};
    result.storeUnaligned (stored);

    for (int i = 0; i < 4; ++i)
        EXPECT_FLOAT_EQ (stored[i], aValues[i] - bValues[i]);
}

TEST (SIMDRegisterTests, Float4Division)
{
    const float aValues[4] = { 4.0f, 9.0f, 16.0f, 25.0f };
    const float bValues[4] = { 2.0f, 3.0f, 4.0f, 5.0f };

    const auto a = Float4::loadUnaligned (aValues);
    const auto b = Float4::loadUnaligned (bValues);
    const auto result = a / b;

    float stored[4] = {};
    result.storeUnaligned (stored);

    for (int i = 0; i < 4; ++i)
        EXPECT_FLOAT_EQ (stored[i], aValues[i] / bValues[i]);
}

TEST (SIMDRegisterTests, Float4CompoundAddAssign)
{
    const float aValues[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    const float bValues[4] = { 10.0f, 20.0f, 30.0f, 40.0f };

    auto a = Float4::loadUnaligned (aValues);
    const auto b = Float4::loadUnaligned (bValues);
    a += b;

    float stored[4] = {};
    a.storeUnaligned (stored);

    for (int i = 0; i < 4; ++i)
        EXPECT_FLOAT_EQ (stored[i], aValues[i] + bValues[i]);
}

TEST (SIMDRegisterTests, Float4CompoundMulAssign)
{
    const float aValues[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    const float bValues[4] = { 2.0f, 3.0f, 4.0f, 5.0f };

    auto a = Float4::loadUnaligned (aValues);
    const auto b = Float4::loadUnaligned (bValues);
    a *= b;

    float stored[4] = {};
    a.storeUnaligned (stored);

    for (int i = 0; i < 4; ++i)
        EXPECT_FLOAT_EQ (stored[i], aValues[i] * bValues[i]);
}

TEST (SIMDRegisterTests, Float4ElementwiseMin)
{
    const float aValues[4] = { 1.0f, 5.0f, 2.0f, 4.0f };
    const float bValues[4] = { 3.0f, 2.0f, 4.0f, 1.0f };

    const auto a = Float4::loadUnaligned (aValues);
    const auto b = Float4::loadUnaligned (bValues);
    const auto result = a.min (b);

    float stored[4] = {};
    result.storeUnaligned (stored);

    for (int i = 0; i < 4; ++i)
        EXPECT_FLOAT_EQ (stored[i], std::min (aValues[i], bValues[i]));
}

TEST (SIMDRegisterTests, Float4ElementwiseMax)
{
    const float aValues[4] = { 1.0f, 5.0f, 2.0f, 4.0f };
    const float bValues[4] = { 3.0f, 2.0f, 4.0f, 1.0f };

    const auto a = Float4::loadUnaligned (aValues);
    const auto b = Float4::loadUnaligned (bValues);
    const auto result = a.max (b);

    float stored[4] = {};
    result.storeUnaligned (stored);

    for (int i = 0; i < 4; ++i)
        EXPECT_FLOAT_EQ (stored[i], std::max (aValues[i], bValues[i]));
}

TEST (SIMDRegisterTests, Float4AbsOnMixedValues)
{
    const float values[4] = { -1.0f, 2.0f, -3.0f, 4.0f };
    const auto r = Float4::loadUnaligned (values);
    const auto result = r.abs();

    float stored[4] = {};
    result.storeUnaligned (stored);

    EXPECT_FLOAT_EQ (stored[0], 1.0f);
    EXPECT_FLOAT_EQ (stored[1], 2.0f);
    EXPECT_FLOAT_EQ (stored[2], 3.0f);
    EXPECT_FLOAT_EQ (stored[3], 4.0f);
}

TEST (SIMDRegisterTests, Float4SumAllLanes)
{
    const float values[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    const auto r = Float4::loadUnaligned (values);
    EXPECT_FLOAT_EQ (r.sum(), 10.0f);
}

TEST (SIMDRegisterTests, Float4SumWithNegatives)
{
    const float values[4] = { 1.0f, -1.0f, 2.0f, -2.0f };
    const auto r = Float4::loadUnaligned (values);
    EXPECT_FLOAT_EQ (r.sum(), 0.0f);
}

TEST (SIMDRegisterTests, Float4HmaxFindsLargest)
{
    const float values[4] = { -3.0f, 7.0f, 1.0f, -10.0f };
    const auto r = Float4::loadUnaligned (values);
    EXPECT_FLOAT_EQ (r.hmax(), 7.0f);
}

TEST (SIMDRegisterTests, MulAddAndLoadStoreRoundTrip)
{
    alignas (16) const float base[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    alignas (16) const float mul[4] = { -1.0f, 0.5f, 2.0f, -0.25f };
    alignas (16) const float add[4] = { 10.0f, 20.0f, 30.0f, 40.0f };
    alignas (16) float stored[4] = {};

    const auto result = Float4::loadAligned (base).mulAdd (Float4::loadAligned (mul), Float4::loadAligned (add));
    result.storeAligned (stored);

    for (int i = 0; i < 4; ++i)
        EXPECT_FLOAT_EQ (stored[i], base[i] + mul[i] * add[i]);
}

TEST (SIMDRegisterTests, Float4LoadFromPointerConstructor)
{
    const float values[4] = { 5.0f, 6.0f, 7.0f, 8.0f };
    const Float4 r (values);

    float stored[4] = {};
    r.storeUnaligned (stored);

    for (int i = 0; i < 4; ++i)
        EXPECT_FLOAT_EQ (stored[i], values[i]);
}

TEST (SIMDRegisterTests, Float4ScalarConstructorBroadcasts)
{
    const Float4 r (42.0f);
    for (int i = 0; i < 4; ++i)
        EXPECT_FLOAT_EQ (r[i], 42.0f);
}

// ==============================================================================
// Float8 tests
// ==============================================================================

TEST (SIMDRegisterTests, Float8DefaultConstructorIsZero)
{
    Float8 r;
    for (int i = 0; i < 8; ++i)
        EXPECT_FLOAT_EQ (r[i], 0.0f);
}

TEST (SIMDRegisterTests, Float8BroadcastAndArithmetic)
{
    const Float8 a = Float8::broadcast (2.0f);
    const Float8 b = Float8::broadcast (3.0f);
    const auto result = a * b;

    for (int i = 0; i < 8; ++i)
        EXPECT_FLOAT_EQ (result[i], 6.0f);
}

TEST (SIMDRegisterTests, Float8LoadStoreRoundTrip)
{
    float values[8] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
    const auto r = Float8::loadUnaligned (values);

    float stored[8] = {};
    r.storeUnaligned (stored);

    for (int i = 0; i < 8; ++i)
        EXPECT_FLOAT_EQ (stored[i], values[i]);
}

TEST (SIMDRegisterTests, Float8Sum)
{
    float values[8] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
    const auto r = Float8::loadUnaligned (values);
    EXPECT_FLOAT_EQ (r.sum(), 36.0f);
}

TEST (SIMDRegisterTests, Float8Hmax)
{
    float values[8] = { 1.0f, -5.0f, 3.0f, 9.0f, 2.0f, -3.0f, 4.0f, 0.0f };
    const auto r = Float8::loadUnaligned (values);
    EXPECT_FLOAT_EQ (r.hmax(), 9.0f);
}

TEST (SIMDRegisterTests, Float8AbsNegatesAll)
{
    float values[8];
    for (int i = 0; i < 8; ++i)
        values[i] = (i % 2 == 0) ? -(float) (i + 1) : (float) (i + 1);

    const auto r = Float8::loadUnaligned (values).abs();
    for (int i = 0; i < 8; ++i)
        EXPECT_FLOAT_EQ (r[i], (float) (i + 1));
}

// ==============================================================================
// Double2 tests
// ==============================================================================

TEST (SIMDRegisterTests, Double2DefaultConstructorIsZero)
{
    Double2 r;
    for (int i = 0; i < 2; ++i)
        EXPECT_DOUBLE_EQ (r[i], 0.0);
}

TEST (SIMDRegisterTests, Double2ArithmeticOperations)
{
    const double aValues[2] = { 1.5, -2.5 };
    const double bValues[2] = { 3.0, 4.0 };

    const auto a = Double2::loadUnaligned (aValues);
    const auto b = Double2::loadUnaligned (bValues);

    const auto sum = a + b;
    const auto diff = a - b;
    const auto prod = a * b;
    const auto quot = a / b;

    EXPECT_DOUBLE_EQ (sum[0], aValues[0] + bValues[0]);
    EXPECT_DOUBLE_EQ (sum[1], aValues[1] + bValues[1]);
    EXPECT_DOUBLE_EQ (diff[0], aValues[0] - bValues[0]);
    EXPECT_DOUBLE_EQ (diff[1], aValues[1] - bValues[1]);
    EXPECT_DOUBLE_EQ (prod[0], aValues[0] * bValues[0]);
    EXPECT_DOUBLE_EQ (prod[1], aValues[1] * bValues[1]);
    EXPECT_DOUBLE_EQ (quot[0], aValues[0] / bValues[0]);
    EXPECT_DOUBLE_EQ (quot[1], aValues[1] / bValues[1]);
}

TEST (SIMDRegisterTests, Double2BroadcastAndSum)
{
    const auto r = Double2::broadcast (3.14);
    EXPECT_NEAR (r.sum(), 6.28, 1.0e-12);
}

TEST (SIMDRegisterTests, Double2MinMax)
{
    const double aValues[2] = { 1.0, 5.0 };
    const double bValues[2] = { 3.0, 2.0 };

    const auto a = Double2::loadUnaligned (aValues);
    const auto b = Double2::loadUnaligned (bValues);

    const auto minResult = a.min (b);
    const auto maxResult = a.max (b);

    EXPECT_DOUBLE_EQ (minResult[0], 1.0);
    EXPECT_DOUBLE_EQ (minResult[1], 2.0);
    EXPECT_DOUBLE_EQ (maxResult[0], 3.0);
    EXPECT_DOUBLE_EQ (maxResult[1], 5.0);
}

// ==============================================================================
// Double4 tests
// ==============================================================================

TEST (SIMDRegisterTests, Double4LoadStoreRoundTrip)
{
    const double values[4] = { 1.1, 2.2, 3.3, 4.4 };
    const auto r = Double4::loadUnaligned (values);

    double stored[4] = {};
    r.storeUnaligned (stored);

    for (int i = 0; i < 4; ++i)
        EXPECT_DOUBLE_EQ (stored[i], values[i]);
}

TEST (SIMDRegisterTests, Double4MulAdd)
{
    const double baseValues[4] = { 1.0, 2.0, 3.0, 4.0 };
    const double mulValues[4] = { 2.0, 3.0, 4.0, 5.0 };
    const double addValues[4] = { 10.0, 20.0, 30.0, 40.0 };

    const auto base = Double4::loadUnaligned (baseValues);
    const auto mul = Double4::loadUnaligned (mulValues);
    const auto add = Double4::loadUnaligned (addValues);
    const auto result = base.mulAdd (mul, add);

    for (int i = 0; i < 4; ++i)
        EXPECT_DOUBLE_EQ (result[i], baseValues[i] + mulValues[i] * addValues[i]);
}

TEST (SIMDRegisterTests, Double4Sum)
{
    const double values[4] = { 1.0, 2.0, 3.0, 4.0 };
    const auto r = Double4::loadUnaligned (values);
    EXPECT_DOUBLE_EQ (r.sum(), 10.0);
}

TEST (SIMDRegisterTests, Double4Hmax)
{
    const double values[4] = { -1.0, 3.5, 2.0, -4.0 };
    const auto r = Double4::loadUnaligned (values);
    EXPECT_DOUBLE_EQ (r.hmax(), 3.5);
}

// ==============================================================================
// General SIMDRegister<T,N> with non-power-of-two N
// ==============================================================================

TEST (SIMDRegisterTests, SIMDRegisterFloat5PartialBatch)
{
    using Float5 = SIMDRegister<float, 5>;
    const float values[5] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    const auto r = Float5::loadUnaligned (values);

    float stored[5] = {};
    r.storeUnaligned (stored);

    for (int i = 0; i < 5; ++i)
        EXPECT_FLOAT_EQ (stored[i], values[i]);
}

TEST (SIMDRegisterTests, SIMDRegisterFloat5Sum)
{
    using Float5 = SIMDRegister<float, 5>;
    const float values[5] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    const auto r = Float5::loadUnaligned (values);
    EXPECT_FLOAT_EQ (r.sum(), 15.0f);
}

TEST (SIMDRegisterTests, SIMDRegisterFloat1ElementAccess)
{
    using Float1 = SIMDRegister<float, 1>;
    const float value = 7.0f;
    const Float1 r (value);
    EXPECT_FLOAT_EQ (r[0], value);
    EXPECT_FLOAT_EQ (r.sum(), value);
    EXPECT_FLOAT_EQ (r.hmax(), value);
}
