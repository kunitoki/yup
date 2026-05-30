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

// ==============================================================================
// multiply tests
// ==============================================================================

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

TEST_F (ComplexVectorOperationsTests, MultiplySinglePair)
{
    // (1 + 2i) * (3 + 4i) = (1*3 - 2*4) + (1*4 + 2*3)i = -5 + 10i
    const float a[] = { 1.0f, 2.0f };
    const float b[] = { 3.0f, 4.0f };
    float result[2] = {};

    ComplexVectorOperations::multiply (result, a, b, 1);

    EXPECT_NEAR (result[0], -5.0f, 1.0e-5f);
    EXPECT_NEAR (result[1], 10.0f, 1.0e-5f);
}

TEST_F (ComplexVectorOperationsTests, MultiplyByOne)
{
    // Multiplying by (1 + 0i) should leave values unchanged
    const float a[] = { 3.0f, 4.0f, -1.0f, 2.0f };
    const float one[] = { 1.0f, 0.0f, 1.0f, 0.0f };
    float result[4] = {};

    ComplexVectorOperations::multiply (result, a, one, 2);

    EXPECT_NEAR (result[0], a[0], 1.0e-5f);
    EXPECT_NEAR (result[1], a[1], 1.0e-5f);
    EXPECT_NEAR (result[2], a[2], 1.0e-5f);
    EXPECT_NEAR (result[3], a[3], 1.0e-5f);
}

TEST_F (ComplexVectorOperationsTests, MultiplyByConjugateGivesModulusSq)
{
    // (a + bi) * (a - bi) = a^2 + b^2 (real, zero imaginary)
    const float a[] = { 3.0f, 4.0f };
    const float conj[] = { 3.0f, -4.0f };
    float result[2] = {};

    ComplexVectorOperations::multiply (result, a, conj, 1);

    EXPECT_NEAR (result[0], 25.0f, 1.0e-5f); // 3^2 + 4^2 = 25
    EXPECT_NEAR (result[1], 0.0f, 1.0e-5f);
}

TEST_F (ComplexVectorOperationsTests, MultiplyLargeBufferMatchesReference)
{
    constexpr int complexPairs = 20;
    float a[complexPairs * 2], b[complexPairs * 2];
    float expected[complexPairs * 2], actual[complexPairs * 2];

    for (int i = 0; i < complexPairs * 2; ++i)
    {
        a[i] = (float) (i - complexPairs) * 0.1f;
        b[i] = (float) (complexPairs - i) * 0.2f;
    }

    multiplyReference (expected, a, b, complexPairs);
    ComplexVectorOperations::multiply (actual, a, b, complexPairs);

    for (int i = 0; i < complexPairs * 2; ++i)
        EXPECT_NEAR (actual[i], expected[i], 1.0e-4f);
}

// ==============================================================================
// multiplyAccumulate tests
// ==============================================================================

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

TEST_F (ComplexVectorOperationsTests, MultiplyAccumulateSinglePair)
{
    // Start with (10 + 5i), add (1+2i)*(3+4i) = -5+10i
    // Result: (10-5) + (5+10)i = 5 + 15i
    const float a[] = { 1.0f, 2.0f };
    const float b[] = { 3.0f, 4.0f };
    float y[] = { 10.0f, 5.0f };

    ComplexVectorOperations::multiplyAccumulate (a, b, y, 1);

    EXPECT_NEAR (y[0], 5.0f, 1.0e-5f);
    EXPECT_NEAR (y[1], 15.0f, 1.0e-5f);
}

TEST_F (ComplexVectorOperationsTests, MultiplyAccumulateWithZeroInputLeavesDestUnchanged)
{
    // a = (0 + 0i), any b: product is (0 + 0i) -> dest unchanged
    const float a[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    const float b[] = { 3.0f, 4.0f, 1.0f, 2.0f };
    float y[] = { 7.0f, 8.0f, 9.0f, 10.0f };

    ComplexVectorOperations::multiplyAccumulate (a, b, y, 2);

    EXPECT_NEAR (y[0], 7.0f, 1.0e-5f);
    EXPECT_NEAR (y[1], 8.0f, 1.0e-5f);
    EXPECT_NEAR (y[2], 9.0f, 1.0e-5f);
    EXPECT_NEAR (y[3], 10.0f, 1.0e-5f);
}

TEST_F (ComplexVectorOperationsTests, MultiplyAccumulateLargeBuffer)
{
    constexpr int complexPairs = 16;
    float a[complexPairs * 2], b[complexPairs * 2];
    float initY[complexPairs * 2];
    float expected[complexPairs * 2], actual[complexPairs * 2];

    for (int i = 0; i < complexPairs * 2; ++i)
    {
        a[i] = (float) i * 0.1f;
        b[i] = (float) (complexPairs * 2 - i) * 0.1f;
        initY[i] = (float) i * 0.5f;
        expected[i] = initY[i];
        actual[i] = initY[i];
    }

    float product[complexPairs * 2] = {};
    multiplyReference (product, a, b, complexPairs);
    for (int i = 0; i < complexPairs * 2; ++i)
        expected[i] += product[i];

    ComplexVectorOperations::multiplyAccumulate (a, b, actual, complexPairs);

    for (int i = 0; i < complexPairs * 2; ++i)
        EXPECT_NEAR (actual[i], expected[i], 1.0e-4f);
}

// ==============================================================================
// powerSpectrum tests
// ==============================================================================

TEST_F (ComplexVectorOperationsTests, PowerSpectrumMatchesScalarReference)
{
    constexpr int complexPairs = 6;
    const float data[complexPairs * 2] = { 3.0f, 4.0f, 1.0f, -2.0f, -5.0f, 12.0f, 0.5f, 0.25f, -0.75f, 1.25f, 2.0f, -3.0f };
    float actual[complexPairs] = {};

    ComplexVectorOperations::powerSpectrum (actual, data, complexPairs);

    for (int i = 0; i < complexPairs; ++i)
        EXPECT_NEAR (actual[i], data[i * 2] * data[i * 2] + data[i * 2 + 1] * data[i * 2 + 1], 1.0e-5f);
}

TEST_F (ComplexVectorOperationsTests, PowerSpectrumSinglePair)
{
    // (3, 4): magnitude^2 = 9 + 16 = 25
    const float data[] = { 3.0f, 4.0f };
    float result = 0.0f;

    ComplexVectorOperations::powerSpectrum (&result, data, 1);

    EXPECT_NEAR (result, 25.0f, 1.0e-5f);
}

TEST_F (ComplexVectorOperationsTests, PowerSpectrumZeroPairs)
{
    const float data[] = { 3.0f, 4.0f };
    float result = 99.0f;

    ComplexVectorOperations::powerSpectrum (&result, data, 0);

    // Should not have written anything
    EXPECT_FLOAT_EQ (result, 99.0f);
}

TEST_F (ComplexVectorOperationsTests, PowerSpectrumUnitVector)
{
    // (1 + 0i): power = 1
    const float data[] = { 1.0f, 0.0f };
    float result = 0.0f;
    ComplexVectorOperations::powerSpectrum (&result, data, 1);
    EXPECT_NEAR (result, 1.0f, 1.0e-5f);
}

TEST_F (ComplexVectorOperationsTests, PowerSpectrumZeroVector)
{
    // (0 + 0i): power = 0
    const float data[] = { 0.0f, 0.0f };
    float result = 99.0f;
    ComplexVectorOperations::powerSpectrum (&result, data, 1);
    EXPECT_NEAR (result, 0.0f, 1.0e-5f);
}

TEST_F (ComplexVectorOperationsTests, PowerSpectrumLargeBuffer)
{
    constexpr int complexPairs = 20;
    float data[complexPairs * 2];
    float actual[complexPairs] = {};

    for (int i = 0; i < complexPairs * 2; ++i)
        data[i] = (float) (i + 1) * 0.5f;

    ComplexVectorOperations::powerSpectrum (actual, data, complexPairs);

    for (int i = 0; i < complexPairs; ++i)
        EXPECT_NEAR (actual[i], data[i * 2] * data[i * 2] + data[i * 2 + 1] * data[i * 2 + 1], 1.0e-4f);
}

TEST_F (ComplexVectorOperationsTests, PowerSpectrumPythagoreanTriple)
{
    // (3, 4): 3^2+4^2=25, (5, 12): 5^2+12^2=169, (8, 15): 8^2+15^2=289
    const float data[] = { 3.0f, 4.0f, 5.0f, 12.0f, 8.0f, 15.0f };
    float actual[3] = {};

    ComplexVectorOperations::powerSpectrum (actual, data, 3);

    EXPECT_NEAR (actual[0], 25.0f, 1.0e-4f);
    EXPECT_NEAR (actual[1], 169.0f, 1.0e-4f);
    EXPECT_NEAR (actual[2], 289.0f, 1.0e-4f);
}
