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

#include "yup_core/yup_core.h"
#include "yup_dsp/yup_dsp.h"

#include <gtest/gtest.h>

template <typename FloatType>
class FractionallyAddressedDelayTests : public ::testing::Test
{
public:
    using FAD = yup::FractionallyAddressedDelay<FloatType>;
};

using TestTypes = ::testing::Types<float, double>;
TYPED_TEST_SUITE (FractionallyAddressedDelayTests, TestTypes);

// --- Construction and parameter accessors ---

TYPED_TEST (FractionallyAddressedDelayTests, DefaultConstruction)
{
    using FAD = yup::FractionallyAddressedDelay<TypeParam>;
    FAD fad;
    EXPECT_EQ (fad.getBufferSize(), 0);
    EXPECT_NEAR (fad.getDelaySamples(), TypeParam (0), TypeParam (1e-6));
}

TYPED_TEST (FractionallyAddressedDelayTests, SetMaxDelaySamplesAllocatesPowerOfTwo)
{
    using FAD = yup::FractionallyAddressedDelay<TypeParam>;
    FAD fad;
    fad.setMaxDelaySamples (100);
    EXPECT_GE (fad.getBufferSize(), 100);
    const int sz = fad.getBufferSize();
    EXPECT_EQ (sz & (sz - 1), 0);
}

TYPED_TEST (FractionallyAddressedDelayTests, SetDelaySamplesRoundtrip)
{
    using FAD = yup::FractionallyAddressedDelay<TypeParam>;
    FAD fad;
    fad.setMaxDelaySamples (128);
    fad.setDelaySamples (TypeParam (64));
    EXPECT_NEAR (fad.getDelaySamples(), TypeParam (64), TypeParam (0.5));
}

TYPED_TEST (FractionallyAddressedDelayTests, SetDelaySamplesClampsToMinimum)
{
    using FAD = yup::FractionallyAddressedDelay<TypeParam>;
    FAD fad;
    fad.setMaxDelaySamples (128);
    fad.setDelaySamples (TypeParam (0));
    EXPECT_GT (fad.getDelaySamples(), TypeParam (0));
}

// --- Silence on empty buffer ---

TYPED_TEST (FractionallyAddressedDelayTests, OutputsSilenceBeforeAnyInput)
{
    using FAD = yup::FractionallyAddressedDelay<TypeParam>;
    FAD fad;
    fad.setMaxDelaySamples (64);
    fad.setDelaySamples (TypeParam (16));
    for (int i = 0; i < 10; ++i)
        EXPECT_NEAR (fad.processSample (TypeParam (0)), TypeParam (0), TypeParam (1e-6));
}

// --- Integer delay accuracy ---

TYPED_TEST (FractionallyAddressedDelayTests, IntegerDelayProducesImpulseAtCorrectOffset)
{
    using FAD = yup::FractionallyAddressedDelay<TypeParam>;
    FAD fad;
    constexpr int bufSize = 16;
    fad.setMaxDelaySamples (bufSize);
    fad.setDelaySamples (TypeParam (bufSize));

    std::vector<TypeParam> input (bufSize * 2, TypeParam (0));
    input[0] = TypeParam (1);

    std::vector<TypeParam> output (bufSize * 2);
    for (int i = 0; i < bufSize * 2; ++i)
        output[i] = fad.processSample (input[i]);

    EXPECT_NEAR (output[bufSize], TypeParam (1), TypeParam (0.05));

    for (int i = 0; i < bufSize; ++i)
        EXPECT_NEAR (output[i], TypeParam (0), TypeParam (0.05));
}

// --- Half-speed delay (increment = 0.5) ---

TYPED_TEST (FractionallyAddressedDelayTests, HalfSpeedDelayOutputsSlowedSignal)
{
    using FAD = yup::FractionallyAddressedDelay<TypeParam>;
    FAD fad;
    constexpr int bufSize = 32;
    fad.setMaxDelaySamples (bufSize);
    fad.setDelaySamples (TypeParam (bufSize * 2));

    const int totalSamples = bufSize * 4;
    std::vector<TypeParam> output (totalSamples);
    for (int i = 0; i < totalSamples; ++i)
        output[i] = fad.processSample (TypeParam (1));

    for (int i = 0; i < totalSamples; ++i)
    {
        EXPECT_TRUE (std::isfinite (output[i]));
        EXPECT_LE (std::abs (output[i]), TypeParam (1) + TypeParam (1e-4));
    }
}

// --- DC pass-through (steady state) ---

TYPED_TEST (FractionallyAddressedDelayTests, DCInputReachesSteadyState)
{
    using FAD = yup::FractionallyAddressedDelay<TypeParam>;
    FAD fad;
    constexpr int bufSize = 64;
    fad.setMaxDelaySamples (bufSize);
    fad.setDelaySamples (TypeParam (bufSize / 2));

    for (int i = 0; i < bufSize * 3; ++i)
        fad.processSample (TypeParam (1));

    for (int i = 0; i < 16; ++i)
        EXPECT_NEAR (fad.processSample (TypeParam (1)), TypeParam (1), TypeParam (0.02));
}

// --- Reset clears state ---

TYPED_TEST (FractionallyAddressedDelayTests, ResetClearsBufferAndPhase)
{
    using FAD = yup::FractionallyAddressedDelay<TypeParam>;
    FAD fad;
    fad.setMaxDelaySamples (64);
    fad.setDelaySamples (TypeParam (32));

    for (int i = 0; i < 64; ++i)
        fad.processSample (TypeParam (1));

    fad.reset();

    for (int i = 0; i < 10; ++i)
        EXPECT_NEAR (fad.processSample (TypeParam (0)), TypeParam (0), TypeParam (1e-6));
}

// --- prepare() is a no-op but must be callable ---

TYPED_TEST (FractionallyAddressedDelayTests, PrepareDoesNotThrow)
{
    using FAD = yup::FractionallyAddressedDelay<TypeParam>;
    FAD fad;
    fad.setMaxDelaySamples (64);
    fad.setDelaySamples (TypeParam (32));
    EXPECT_NO_THROW (fad.prepare (44100.0, 512));
}

// --- processBlock matches processSample ---

TYPED_TEST (FractionallyAddressedDelayTests, ProcessBlockMatchesProcessSample)
{
    using FAD = yup::FractionallyAddressedDelay<TypeParam>;
    constexpr int N = 128;
    std::vector<TypeParam> input (N);
    for (int i = 0; i < N; ++i)
        input[i] = static_cast<TypeParam> (i % 8) / TypeParam (8);

    FAD fadA, fadB;
    fadA.setMaxDelaySamples (64);
    fadA.setDelaySamples (TypeParam (32));
    fadB.setMaxDelaySamples (64);
    fadB.setDelaySamples (TypeParam (32));

    std::vector<TypeParam> outA (N), outB (N);

    for (int i = 0; i < N; ++i)
        outA[i] = fadA.processSample (input[i]);

    fadB.processBlock (input.data(), outB.data(), N);

    for (int i = 0; i < N; ++i)
        EXPECT_NEAR (outA[i], outB[i], TypeParam (1e-6));
}

// --- processInPlace matches processBlock ---

TYPED_TEST (FractionallyAddressedDelayTests, ProcessInPlaceMatchesProcessBlock)
{
    using FAD = yup::FractionallyAddressedDelay<TypeParam>;
    constexpr int N = 64;
    std::vector<TypeParam> input (N);
    for (int i = 0; i < N; ++i)
        input[i] = static_cast<TypeParam> (i % 4) / TypeParam (4);

    FAD fadA, fadB;
    fadA.setMaxDelaySamples (64);
    fadA.setDelaySamples (TypeParam (32));
    fadB.setMaxDelaySamples (64);
    fadB.setDelaySamples (TypeParam (32));

    std::vector<TypeParam> outBlock (N);
    fadA.processBlock (input.data(), outBlock.data(), N);

    std::vector<TypeParam> inPlace = input;
    fadB.processInPlace (inPlace.data(), N);

    for (int i = 0; i < N; ++i)
        EXPECT_NEAR (outBlock[i], inPlace[i], TypeParam (1e-6));
}

// --- Output is always finite ---

TYPED_TEST (FractionallyAddressedDelayTests, OutputIsAlwaysFiniteForBoundedInput)
{
    using FAD = yup::FractionallyAddressedDelay<TypeParam>;
    FAD fad;
    fad.setMaxDelaySamples (128);
    fad.setDelaySamples (TypeParam (75.7));

    for (int i = 0; i < 512; ++i)
    {
        const TypeParam x = (i % 2 == 0) ? TypeParam (0.9) : TypeParam (-0.9);
        EXPECT_TRUE (std::isfinite (fad.processSample (x)));
    }
}

// --- Modulation: changing delay mid-stream stays finite ---

TYPED_TEST (FractionallyAddressedDelayTests, DelayModulationStaysFinite)
{
    using FAD = yup::FractionallyAddressedDelay<TypeParam>;
    FAD fad;
    fad.setMaxDelaySamples (256);
    fad.setDelaySamples (TypeParam (128));

    bool anyFiniteViolation = false;
    for (int i = 0; i < 1024; ++i)
    {
        const TypeParam delayMod = TypeParam (128) + TypeParam (64) * std::sin (TypeParam (i) * TypeParam (0.01));
        fad.setDelaySamples (delayMod);
        const TypeParam out = fad.processSample (TypeParam (0.5));
        if (! std::isfinite (out))
            anyFiniteViolation = true;
    }
    EXPECT_FALSE (anyFiniteViolation);
}
