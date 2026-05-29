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
