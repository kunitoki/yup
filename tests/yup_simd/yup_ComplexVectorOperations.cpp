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
#include <random>
#include <vector>

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

    template <typename Type>
    static std::vector<Type> randomVector (int size, unsigned int seed)
    {
        std::mt19937 generator (seed);
        std::uniform_real_distribution<Type> distribution (Type (-1), Type (1));

        std::vector<Type> result (static_cast<size_t> (size));
        for (auto& value : result)
            value = distribution (generator);

        return result;
    }

    template <typename Type>
    static void expectVectorsNear (const std::vector<Type>& actual, const std::vector<Type>& expected, Type maxError)
    {
        ASSERT_EQ (actual.size(), expected.size());

        for (size_t i = 0; i < actual.size(); ++i)
            EXPECT_NEAR (actual[i], expected[i], maxError) << "at index " << i;
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

// ==============================================================================
// multiplyConjugate tests
// ==============================================================================

TEST_F (ComplexVectorOperationsTests, MultiplyConjugateMatchesScalarReference)
{
    constexpr int complexPairs = 11;
    const auto a = randomVector<float> (complexPairs * 2, 1);
    auto b = randomVector<float> (complexPairs * 2, 2);

    std::vector<float> actual (complexPairs * 2);
    ComplexVectorOperations::multiplyConjugate (actual.data(), a.data(), b.data(), complexPairs);

    for (int i = 0; i < complexPairs; ++i)
        b[i * 2 + 1] = -b[i * 2 + 1];

    std::vector<float> expected (complexPairs * 2);
    multiplyReference (expected.data(), a.data(), b.data(), complexPairs);

    expectVectorsNear (actual, expected, 1.0e-5f);
}

// ==============================================================================
// Packed real layout tests
// ==============================================================================

TEST_F (ComplexVectorOperationsTests, PackedRealMultipliesDcAndNyquistAsReals)
{
    // Pair 0: DC 2 and 5, Nyquist 3 and 7. Pair 1: (1 + 2i) and (3 - 1i)
    const std::vector<float> a { 2.0f, 3.0f, 1.0f, 2.0f };
    const std::vector<float> b { 5.0f, 7.0f, 3.0f, -1.0f };
    std::vector<float> dest (4);

    ComplexVectorOperations::multiply (dest.data(), a.data(), b.data(), 2, ComplexLayout::packedReal);
    expectVectorsNear (dest, { 10.0f, 21.0f, 5.0f, 5.0f }, 1.0e-5f);

    ComplexVectorOperations::multiplyConjugate (dest.data(), a.data(), b.data(), 2, ComplexLayout::packedReal);
    expectVectorsNear (dest, { 10.0f, 21.0f, 1.0f, 7.0f }, 1.0e-5f);

    dest = { 1.0f, 1.0f, 1.0f, 1.0f };
    ComplexVectorOperations::multiplyAccumulate (a.data(), b.data(), dest.data(), 2, ComplexLayout::packedReal);
    expectVectorsNear (dest, { 11.0f, 22.0f, 6.0f, 6.0f }, 1.0e-5f);
}

TEST_F (ComplexVectorOperationsTests, PackedRealMatchesInterleavedAfterPairZero)
{
    constexpr int complexPairs = 19;
    const auto a = randomVector<float> (complexPairs * 2, 3);
    const auto b = randomVector<float> (complexPairs * 2, 4);

    std::vector<float> packed (complexPairs * 2);
    std::vector<float> interleaved (complexPairs * 2);

    ComplexVectorOperations::multiply (packed.data(), a.data(), b.data(), complexPairs, ComplexLayout::packedReal);
    ComplexVectorOperations::multiply (interleaved.data(), a.data(), b.data(), complexPairs);

    for (int i = 2; i < complexPairs * 2; ++i)
        EXPECT_FLOAT_EQ (packed[i], interleaved[i]) << "at index " << i;
}

TEST_F (ComplexVectorOperationsTests, PackedRealMagnitudePowerAndPhaseReportDc)
{
    // DC = -3, Nyquist = 4 (not reported), pair 1 = 3 + 4i, pair 2 = 0 - 2i
    const std::vector<float> spectrum { -3.0f, 4.0f, 3.0f, 4.0f, 0.0f, -2.0f };
    std::vector<float> dest (3);

    ComplexVectorOperations::magnitude (dest.data(), spectrum.data(), 3, ComplexLayout::packedReal);
    expectVectorsNear (dest, { 3.0f, 5.0f, 2.0f }, 1.0e-5f);

    ComplexVectorOperations::powerSpectrum (dest.data(), spectrum.data(), 3, ComplexLayout::packedReal);
    expectVectorsNear (dest, { 9.0f, 25.0f, 4.0f }, 1.0e-5f);

    const float pi = MathConstants<float>::pi;
    ComplexVectorOperations::phase (dest.data(), spectrum.data(), 3, ComplexLayout::packedReal);
    expectVectorsNear (dest, { pi, std::atan2 (4.0f, 3.0f), -pi / 2.0f }, 1.0e-5f);

    const std::vector<float> positiveDc { 2.0f, -5.0f };
    ComplexVectorOperations::phase (dest.data(), positiveDc.data(), 1, ComplexLayout::packedReal);
    EXPECT_NEAR (dest[0], 0.0f, 1.0e-5f);
}

TEST_F (ComplexVectorOperationsTests, PhaseOfZeroPairsFollowsAtan2)
{
    // signed zeros in every combination, repeated to fill whole batches and a padded tail
    const float zeros[4][2] = { { 0.0f, 0.0f }, { -0.0f, 0.0f }, { 0.0f, -0.0f }, { -0.0f, -0.0f } };

    for (auto layout : { ComplexLayout::interleaved, ComplexLayout::packedReal })
    {
        const int complexPairs = 19;
        std::vector<float> floats (complexPairs * 2);
        std::vector<double> doubles (complexPairs * 2);

        for (int i = 0; i < complexPairs; ++i)
        {
            floats[i * 2] = zeros[i % 4][0];
            floats[i * 2 + 1] = zeros[i % 4][1];
            doubles[i * 2] = zeros[i % 4][0];
            doubles[i * 2 + 1] = zeros[i % 4][1];
        }

        std::vector<float> floatPhases (complexPairs);
        std::vector<double> doublePhases (complexPairs);
        ComplexVectorOperations::phase (floatPhases.data(), floats.data(), complexPairs, layout);
        ComplexVectorOperations::phase (doublePhases.data(), doubles.data(), complexPairs, layout);

        const int first = layout == ComplexLayout::packedReal ? 1 : 0;
        // vForce may round pi to the neighbouring value: compare with a tolerance and check the sign
        for (int i = first; i < complexPairs; ++i)
        {
            const float floatExpected = std::atan2 (floats[i * 2 + 1], floats[i * 2]);
            const double doubleExpected = std::atan2 (doubles[i * 2 + 1], doubles[i * 2]);

            EXPECT_NEAR (floatPhases[i], floatExpected, 1.0e-6f) << "at pair " << i;
            EXPECT_EQ (std::signbit (floatPhases[i]), std::signbit (floatExpected)) << "at pair " << i;
            EXPECT_NEAR (doublePhases[i], doubleExpected, 1.0e-12) << "at pair " << i;
            EXPECT_EQ (std::signbit (doublePhases[i]), std::signbit (doubleExpected)) << "at pair " << i;
        }
    }
}

TEST_F (ComplexVectorOperationsTests, MultiplicationsWorkInPlace)
{
    constexpr int complexPairs = 21;
    const auto a = randomVector<float> (complexPairs * 2, 5);
    const auto b = randomVector<float> (complexPairs * 2, 6);

    for (auto layout : { ComplexLayout::interleaved, ComplexLayout::packedReal })
    {
        std::vector<float> expected (complexPairs * 2);
        ComplexVectorOperations::multiply (expected.data(), a.data(), b.data(), complexPairs, layout);

        auto inPlaceA = a;
        ComplexVectorOperations::multiply (inPlaceA.data(), inPlaceA.data(), b.data(), complexPairs, layout);
        expectVectorsNear (inPlaceA, expected, 0.0f);

        auto inPlaceB = b;
        ComplexVectorOperations::multiply (inPlaceB.data(), a.data(), inPlaceB.data(), complexPairs, layout);
        expectVectorsNear (inPlaceB, expected, 0.0f);

        ComplexVectorOperations::multiplyConjugate (expected.data(), a.data(), b.data(), complexPairs, layout);

        inPlaceA = a;
        ComplexVectorOperations::multiplyConjugate (inPlaceA.data(), inPlaceA.data(), b.data(), complexPairs, layout);
        expectVectorsNear (inPlaceA, expected, 0.0f);

        inPlaceB = b;
        ComplexVectorOperations::multiplyConjugate (inPlaceB.data(), a.data(), inPlaceB.data(), complexPairs, layout);
        expectVectorsNear (inPlaceB, expected, 0.0f);
    }
}

TEST_F (ComplexVectorOperationsTests, OperationsOnlyTouchTheRequestedPairs)
{
    constexpr int size = 32;
    constexpr float sentinel = 12345.0f;
    const auto a = randomVector<float> (size, 7);
    const auto b = randomVector<float> (size, 8);

    for (auto layout : { ComplexLayout::interleaved, ComplexLayout::packedReal })
    {
        for (int complexPairs : { 0, 1, 3, 9 })
        {
            std::vector<float> dest (size, sentinel);
            ComplexVectorOperations::multiply (dest.data(), a.data(), b.data(), complexPairs, layout);

            for (int i = complexPairs * 2; i < size; ++i)
                EXPECT_EQ (dest[i], sentinel) << "multiply of " << complexPairs << " pairs wrote index " << i;

            dest.assign (size, sentinel);
            ComplexVectorOperations::multiplyConjugate (dest.data(), a.data(), b.data(), complexPairs, layout);

            for (int i = complexPairs * 2; i < size; ++i)
                EXPECT_EQ (dest[i], sentinel) << "multiplyConjugate of " << complexPairs << " pairs wrote index " << i;

            dest.assign (size, sentinel);
            ComplexVectorOperations::multiplyAccumulate (a.data(), b.data(), dest.data(), complexPairs, layout);

            for (int i = complexPairs * 2; i < size; ++i)
                EXPECT_EQ (dest[i], sentinel) << "multiplyAccumulate of " << complexPairs << " pairs wrote index " << i;

            dest.assign (size, sentinel);
            ComplexVectorOperations::phase (dest.data(), a.data(), complexPairs, layout);

            for (int i = complexPairs; i < size; ++i)
                EXPECT_EQ (dest[i], sentinel) << "phase of " << complexPairs << " pairs wrote index " << i;
        }
    }
}

// ==============================================================================
// magnitude and phase tests
// ==============================================================================

TEST_F (ComplexVectorOperationsTests, MagnitudeAndPhaseOfKnownPairs)
{
    const float pi = MathConstants<float>::pi;

    // 3 + 4i, 0 + 1i, -1 + 0i, 1 - 1i
    const std::vector<float> data { 3.0f, 4.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f, -1.0f };
    std::vector<float> dest (4);

    ComplexVectorOperations::magnitude (dest.data(), data.data(), 4);
    expectVectorsNear (dest, { 5.0f, 1.0f, 1.0f, std::sqrt (2.0f) }, 1.0e-5f);

    ComplexVectorOperations::phase (dest.data(), data.data(), 4);
    expectVectorsNear (dest, { std::atan2 (4.0f, 3.0f), pi / 2.0f, pi, -pi / 4.0f }, 1.0e-5f);
}

TEST_F (ComplexVectorOperationsTests, RealResultsWorkInPlace)
{
    constexpr int complexPairs = 37;
    const auto data = randomVector<float> (complexPairs * 2, 9);

    for (auto layout : { ComplexLayout::interleaved, ComplexLayout::packedReal })
    {
        std::vector<float> expected (complexPairs);
        auto inPlace = data;

        ComplexVectorOperations::magnitude (expected.data(), data.data(), complexPairs, layout);
        ComplexVectorOperations::magnitude (inPlace.data(), inPlace.data(), complexPairs, layout);
        inPlace.resize (complexPairs);
        expectVectorsNear (inPlace, expected, 0.0f);

        inPlace = data;
        ComplexVectorOperations::powerSpectrum (expected.data(), data.data(), complexPairs, layout);
        ComplexVectorOperations::powerSpectrum (inPlace.data(), inPlace.data(), complexPairs, layout);
        inPlace.resize (complexPairs);
        expectVectorsNear (inPlace, expected, 0.0f);

        inPlace = data;
        ComplexVectorOperations::phase (expected.data(), data.data(), complexPairs, layout);
        ComplexVectorOperations::phase (inPlace.data(), inPlace.data(), complexPairs, layout);
        inPlace.resize (complexPairs);
        expectVectorsNear (inPlace, expected, 0.0f);
    }
}

TEST_F (ComplexVectorOperationsTests, TailPairsGetTheSameResultAsTheRest)
{
    // The same pair repeated: no matter where it falls (full batch or tail), the result is identical
    constexpr int complexPairs = 23;
    std::vector<float> data;
    for (int i = 0; i < complexPairs; ++i)
        data.insert (data.end(), { 0.3f, -0.7f });

    std::vector<float> phases (complexPairs);
    std::vector<float> magnitudes (complexPairs);
    ComplexVectorOperations::phase (phases.data(), data.data(), complexPairs);
    ComplexVectorOperations::magnitude (magnitudes.data(), data.data(), complexPairs);

    for (int i = 1; i < complexPairs; ++i)
    {
        EXPECT_EQ (phases[i], phases[0]) << "at pair " << i;
        EXPECT_EQ (magnitudes[i], magnitudes[0]) << "at pair " << i;
    }
}

TEST_F (ComplexVectorOperationsTests, TailPairsGetTheSameProductAsTheRest)
{
    constexpr int complexPairs = 23;
    std::vector<float> a, b;
    for (int i = 0; i < complexPairs; ++i)
    {
        a.insert (a.end(), { 0.3f, -0.7f });
        b.insert (b.end(), { -1.1f, 0.9f });
    }

    for (auto layout : { ComplexLayout::interleaved, ComplexLayout::packedReal })
    {
        std::vector<float> product (complexPairs * 2);
        std::vector<float> conjugateProduct (complexPairs * 2);
        std::vector<float> accumulated (complexPairs * 2, 0.5f);

        ComplexVectorOperations::multiply (product.data(), a.data(), b.data(), complexPairs, layout);
        ComplexVectorOperations::multiplyConjugate (conjugateProduct.data(), a.data(), b.data(), complexPairs, layout);
        ComplexVectorOperations::multiplyAccumulate (a.data(), b.data(), accumulated.data(), complexPairs, layout);

        for (int i = 4; i < complexPairs * 2; ++i)
        {
            EXPECT_EQ (product[i], product[2 + i % 2]) << "at index " << i;
            EXPECT_EQ (conjugateProduct[i], conjugateProduct[2 + i % 2]) << "at index " << i;
            EXPECT_EQ (accumulated[i], accumulated[2 + i % 2]) << "at index " << i;
        }
    }
}

TEST_F (ComplexVectorOperationsTests, PhaseOfLargeBufferMatchesAtan2)
{
    for (int complexPairs : { 1024, 1023 })
    {
        const auto floats = randomVector<float> (complexPairs * 2, 10);
        std::vector<float> floatExpected (complexPairs);
        std::vector<float> floatPhases (complexPairs);

        for (int i = 0; i < complexPairs; ++i)
            floatExpected[i] = std::atan2 (floats[i * 2 + 1], floats[i * 2]);

        ComplexVectorOperations::phase (floatPhases.data(), floats.data(), complexPairs);
        expectVectorsNear (floatPhases, floatExpected, 1.0e-5f);

        const auto doubles = randomVector<double> (complexPairs * 2, 11);
        std::vector<double> doubleExpected (complexPairs);
        std::vector<double> doublePhases (complexPairs);

        for (int i = 0; i < complexPairs; ++i)
            doubleExpected[i] = std::atan2 (doubles[i * 2 + 1], doubles[i * 2]);

        ComplexVectorOperations::phase (doublePhases.data(), doubles.data(), complexPairs);
        expectVectorsNear (doublePhases, doubleExpected, 1.0e-12);
    }
}

// ==============================================================================
// deinterleave and interleave tests
// ==============================================================================

TEST_F (ComplexVectorOperationsTests, DeinterleaveSplitsBothLayouts)
{
    const std::vector<float> data { 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f };

    std::vector<float> real (4);
    std::vector<float> imag (4);
    ComplexVectorOperations::deinterleave (real.data(), imag.data(), data.data(), 4);
    expectVectorsNear (real, { 10.0f, 12.0f, 14.0f, 16.0f }, 0.0f);
    expectVectorsNear (imag, { 11.0f, 13.0f, 15.0f, 17.0f }, 0.0f);

    // Packed: DC to Nyquist in real, the imaginary parts of bins 1 to 3 in imag
    real.assign (5, 0.0f);
    imag.assign (3, 0.0f);
    ComplexVectorOperations::deinterleave (real.data(), imag.data(), data.data(), 4, ComplexLayout::packedReal);
    expectVectorsNear (real, { 10.0f, 12.0f, 14.0f, 16.0f, 11.0f }, 0.0f);
    expectVectorsNear (imag, { 13.0f, 15.0f, 17.0f }, 0.0f);
}

TEST_F (ComplexVectorOperationsTests, InterleaveInvertsDeinterleave)
{
    constexpr int complexPairs = 29;

    for (auto layout : { ComplexLayout::interleaved, ComplexLayout::packedReal })
    {
        const auto floats = randomVector<float> (complexPairs * 2, 12);
        std::vector<float> floatReal (complexPairs + 1);
        std::vector<float> floatImag (complexPairs);
        std::vector<float> floatResult (complexPairs * 2);

        ComplexVectorOperations::deinterleave (floatReal.data(), floatImag.data(), floats.data(), complexPairs, layout);
        ComplexVectorOperations::interleave (floatResult.data(), floatReal.data(), floatImag.data(), complexPairs, layout);
        expectVectorsNear (floatResult, floats, 0.0f);

        const auto doubles = randomVector<double> (complexPairs * 2, 13);
        std::vector<double> doubleReal (complexPairs + 1);
        std::vector<double> doubleImag (complexPairs);
        std::vector<double> doubleResult (complexPairs * 2);

        ComplexVectorOperations::deinterleave (doubleReal.data(), doubleImag.data(), doubles.data(), complexPairs, layout);
        ComplexVectorOperations::interleave (doubleResult.data(), doubleReal.data(), doubleImag.data(), complexPairs, layout);
        expectVectorsNear (doubleResult, doubles, 0.0);
    }
}

// ==============================================================================
// double precision tests
// ==============================================================================

TEST_F (ComplexVectorOperationsTests, DoubleOperationsMatchScalarReference)
{
    constexpr int complexPairs = 13;
    const auto a = randomVector<double> (complexPairs * 2, 14);
    const auto b = randomVector<double> (complexPairs * 2, 15);
    const auto initial = randomVector<double> (complexPairs * 2, 16);

    std::vector<double> product (complexPairs * 2);
    std::vector<double> conjugateProduct (complexPairs * 2);
    std::vector<double> power (complexPairs);
    std::vector<double> magnitude (complexPairs);

    for (int i = 0; i < complexPairs; ++i)
    {
        const double ar = a[i * 2], ai = a[i * 2 + 1];
        const double br = b[i * 2], bi = b[i * 2 + 1];

        product[i * 2] = ar * br - ai * bi;
        product[i * 2 + 1] = ai * br + ar * bi;
        conjugateProduct[i * 2] = ar * br + ai * bi;
        conjugateProduct[i * 2 + 1] = ai * br - ar * bi;
        power[i] = ar * ar + ai * ai;
        magnitude[i] = std::sqrt (power[i]);
    }

    std::vector<double> actual (complexPairs * 2);

    ComplexVectorOperations::multiply (actual.data(), a.data(), b.data(), complexPairs);
    expectVectorsNear (actual, product, 1.0e-12);

    ComplexVectorOperations::multiplyConjugate (actual.data(), a.data(), b.data(), complexPairs);
    expectVectorsNear (actual, conjugateProduct, 1.0e-12);

    actual = initial;
    ComplexVectorOperations::multiplyAccumulate (a.data(), b.data(), actual.data(), complexPairs);
    for (int i = 0; i < complexPairs * 2; ++i)
        EXPECT_NEAR (actual[i], initial[i] + product[i], 1.0e-12) << "at index " << i;

    std::vector<double> reals (complexPairs);

    ComplexVectorOperations::powerSpectrum (reals.data(), a.data(), complexPairs);
    expectVectorsNear (reals, power, 1.0e-12);

    ComplexVectorOperations::magnitude (reals.data(), a.data(), complexPairs);
    expectVectorsNear (reals, magnitude, 1.0e-12);
}
