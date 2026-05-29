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

class ComplexVectorOperationsTests : public ::testing::Test
{
protected:
    static void multiplyReference (float* dest, const float* a, const float* b, int complexPairs)
    {
        for (int i = 0; i < complexPairs; ++i)
        {
            const int realIndex = i * 2;
            const int imagIndex = realIndex + 1;

            dest[realIndex] = a[realIndex] * b[realIndex] - a[imagIndex] * b[imagIndex];
            dest[imagIndex] = a[realIndex] * b[imagIndex] + a[imagIndex] * b[realIndex];
        }
    }
};

TEST_F (ComplexVectorOperationsTests, MultiplyMatchesScalarReference)
{
    constexpr int complexPairs = 7;
    const float a[complexPairs * 2] = { 1.0f, 2.0f, -3.0f, 4.0f, 5.0f, -6.0f, -7.0f, -8.0f, 0.5f, -0.25f, 2.5f, 3.5f, -4.0f, 1.0f };
    const float b[complexPairs * 2] = { -2.0f, 0.5f, 1.0f, -1.5f, -0.5f, -2.0f, 3.0f, 2.0f, -4.0f, 0.25f, 1.5f, -2.5f, 0.75f, 2.25f };

    float expected[complexPairs * 2] = {};
    float actual[complexPairs * 2] = {};

    multiplyReference (expected, a, b, complexPairs);
    ComplexVectorOperations::multiply (actual, a, b, complexPairs);

    for (int i = 0; i < complexPairs * 2; ++i)
        EXPECT_NEAR (actual[i], expected[i], 1.0e-5f);
}

TEST_F (ComplexVectorOperationsTests, MultiplyAccumulateMatchesScalarReference)
{
    constexpr int complexPairs = 5;
    const float a[complexPairs * 2] = { 1.0f, -1.0f, 2.0f, 3.0f, -4.0f, 0.5f, 0.25f, -0.75f, 3.0f, 2.0f };
    const float b[complexPairs * 2] = { 0.5f, 2.0f, -1.0f, 0.25f, 3.0f, -2.0f, 1.5f, 4.0f, -0.5f, 1.0f };
    float expected[complexPairs * 2] = { 0.25f, -0.5f, 0.75f, 1.0f, -1.25f, 1.5f, 1.75f, -2.0f, 2.25f, 2.5f };
    float actual[complexPairs * 2];

    for (int i = 0; i < complexPairs * 2; ++i)
        actual[i] = expected[i];

    float product[complexPairs * 2] = {};
    multiplyReference (product, a, b, complexPairs);

    for (int i = 0; i < complexPairs * 2; ++i)
        expected[i] += product[i];

    ComplexVectorOperations::multiplyAccumulate (a, b, actual, complexPairs);

    for (int i = 0; i < complexPairs * 2; ++i)
        EXPECT_NEAR (actual[i], expected[i], 1.0e-5f);
}

TEST_F (ComplexVectorOperationsTests, PowerSpectrumMatchesScalarReference)
{
    constexpr int complexPairs = 6;
    const float data[complexPairs * 2] = { 3.0f, 4.0f, 1.0f, -2.0f, -5.0f, 12.0f, 0.5f, 0.25f, -0.75f, 1.25f, 2.0f, -3.0f };
    float actual[complexPairs] = {};

    ComplexVectorOperations::powerSpectrum (actual, data, complexPairs);

    for (int i = 0; i < complexPairs; ++i)
        EXPECT_NEAR (actual[i], data[i * 2] * data[i * 2] + data[i * 2 + 1] * data[i * 2 + 1], 1.0e-5f);
}
