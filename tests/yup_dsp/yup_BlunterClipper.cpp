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
class BlunterClipperTests : public ::testing::Test
{
public:
    using Clipper = yup::BlunterClipper<FloatType>;

    static FloatType blunterRef (FloatType x)
    {
        if (x >= FloatType (1))
            return FloatType (1);
        if (x <= FloatType (-1))
            return FloatType (-1);
        return x * (FloatType (2) - std::abs (x));
    }

    void testDefaultConstruction()
    {
        Clipper clipper;
        EXPECT_NEAR (clipper.getInputGain(), FloatType (1), FloatType (1e-7));
        EXPECT_NEAR (clipper.getOutputGain(), FloatType (1), FloatType (1e-7));
    }

    void testParameterizedConstruction()
    {
        Clipper clipper (FloatType (0.6), FloatType (0.5));
        EXPECT_NEAR (clipper.getInputGain(), FloatType (0.6), FloatType (1e-7));
        EXPECT_NEAR (clipper.getOutputGain(), FloatType (0.5), FloatType (1e-7));
    }

    void testSetParameters()
    {
        Clipper clipper;

        clipper.setInputGain (FloatType (2));
        EXPECT_NEAR (clipper.getInputGain(), FloatType (2), FloatType (1e-7));

        clipper.setOutputGain (FloatType (0.25));
        EXPECT_NEAR (clipper.getOutputGain(), FloatType (0.25), FloatType (1e-7));

        clipper.setParameters (FloatType (1.5), FloatType (0.75));
        EXPECT_NEAR (clipper.getInputGain(), FloatType (1.5), FloatType (1e-7));
        EXPECT_NEAR (clipper.getOutputGain(), FloatType (0.75), FloatType (1e-7));
    }

    void testSanitizesGainParameters()
    {
        Clipper clipper (FloatType (-1), FloatType (-1));

        EXPECT_GT (clipper.getInputGain(), FloatType (0));
        EXPECT_NEAR (clipper.getOutputGain(), FloatType (0), FloatType (1e-7));

        clipper.setInputGain (FloatType (0));
        EXPECT_GT (clipper.getInputGain(), FloatType (0));

        clipper.setOutputGain (FloatType (-0.25));
        EXPECT_NEAR (clipper.getOutputGain(), FloatType (0), FloatType (1e-7));

        clipper.setParameters (FloatType (-2), FloatType (-0.5));
        EXPECT_GT (clipper.getInputGain(), FloatType (0));
        EXPECT_NEAR (clipper.getOutputGain(), FloatType (0), FloatType (1e-7));
    }

    void testCanonicalFormulaUnityGains()
    {
        Clipper clipper;

        for (FloatType x : { FloatType (-1), FloatType (-0.5), FloatType (0), FloatType (0.25), FloatType (0.5), FloatType (1) })
        {
            EXPECT_NEAR (clipper.processSample (x), blunterRef (x), FloatType (1e-6));
        }
    }

    void testBoundedOutput()
    {
        Clipper clipper;

        for (FloatType x : { FloatType (-10), FloatType (-1.001), FloatType (1.001), FloatType (10) })
        {
            FloatType y = clipper.processSample (x);
            EXPECT_LE (y, FloatType (1));
            EXPECT_GE (y, FloatType (-1));
        }
    }

    void testMonotonicity()
    {
        Clipper clipper;

        FloatType prev = clipper.processSample (FloatType (-3));
        for (int i = -29; i <= 30; ++i)
        {
            FloatType x = FloatType (i) * FloatType (0.1);
            FloatType curr = clipper.processSample (x);
            EXPECT_GE (curr, prev - FloatType (1e-7));
            prev = curr;
        }
    }

    void testSymmetry()
    {
        Clipper clipper;

        for (FloatType x : { FloatType (0.1), FloatType (0.5), FloatType (0.9), FloatType (1.0), FloatType (1.5), FloatType (5.0) })
        {
            EXPECT_NEAR (clipper.processSample (-x), -clipper.processSample (x), FloatType (1e-6));
        }
    }

    void testZeroInput()
    {
        Clipper clipper;
        EXPECT_NEAR (clipper.processSample (FloatType (0)), FloatType (0), FloatType (1e-7));
    }

    void testSlopeAtOriginIsTwo()
    {
        Clipper clipper;
        const FloatType eps = FloatType (1e-5);
        const FloatType slope = clipper.processSample (eps) / eps;
        EXPECT_NEAR (slope, FloatType (2), FloatType (1e-4));
    }

    void testInputGainScalesDistortion()
    {
        Clipper low (FloatType (0.5), FloatType (1));
        Clipper high (FloatType (2.0), FloatType (1));

        const FloatType x = FloatType (0.6);

        FloatType outLow = low.processSample (x);
        FloatType outHigh = high.processSample (x);

        EXPECT_GT (outHigh, outLow);
        EXPECT_NEAR (outHigh, FloatType (1), FloatType (1e-6));
    }

    void testOutputGainScalesResult()
    {
        Clipper half (FloatType (1), FloatType (0.5));
        Clipper full (FloatType (1), FloatType (1));

        const FloatType x = FloatType (0.5);
        EXPECT_NEAR (half.processSample (x), full.processSample (x) * FloatType (0.5), FloatType (1e-6));
    }

    void testUnityGainAtOriginWithHalfOutputGain()
    {
        Clipper clipper (FloatType (1), FloatType (0.5));
        const FloatType eps = FloatType (1e-5);
        const FloatType slope = clipper.processSample (eps) / eps;
        EXPECT_NEAR (slope, FloatType (1), FloatType (1e-4));
    }

    void testBlockProcessing()
    {
        Clipper clipper;
        const int numSamples = 16;
        FloatType input[numSamples];
        FloatType output[numSamples];

        for (int i = 0; i < numSamples; ++i)
            input[i] = FloatType (i - 8) * FloatType (0.2);

        clipper.processBlock (input, output, numSamples);

        for (int i = 0; i < numSamples; ++i)
            EXPECT_NEAR (output[i], clipper.processSample (input[i]), FloatType (1e-6));
    }

    void testInPlaceProcessing()
    {
        Clipper clipper;
        const int numSamples = 16;
        FloatType data[numSamples];
        FloatType reference[numSamples];

        for (int i = 0; i < numSamples; ++i)
        {
            data[i] = FloatType (i - 8) * FloatType (0.2);
            reference[i] = data[i];
        }

        clipper.processInPlace (data, numSamples);

        for (int i = 0; i < numSamples; ++i)
            EXPECT_NEAR (data[i], clipper.processSample (reference[i]), FloatType (1e-6));
    }

    void testResetAndPrepareAreNoOps()
    {
        Clipper clipper (FloatType (0.6), FloatType (0.5));
        clipper.reset();
        clipper.prepare (44100.0, 512);

        EXPECT_NEAR (clipper.getInputGain(), FloatType (0.6), FloatType (1e-7));
        EXPECT_NEAR (clipper.getOutputGain(), FloatType (0.5), FloatType (1e-7));
    }

    void testSecondDerivativeIsConstantInKnee()
    {
        Clipper clipper;
        const FloatType h = FloatType (1e-2);

        for (FloatType x : { FloatType (-0.8), FloatType (-0.5), FloatType (-0.2), FloatType (0.2), FloatType (0.5), FloatType (0.8) })
        {
            const FloatType f_plus = clipper.processSample (x + h);
            const FloatType f_zero = clipper.processSample (x);
            const FloatType f_minus = clipper.processSample (x - h);
            const FloatType d2 = (f_plus - FloatType (2) * f_zero + f_minus) / (h * h);
            EXPECT_NEAR (std::abs (d2), FloatType (2), FloatType (0.01));
        }
    }
};

using TestTypes = ::testing::Types<float, double>;
TYPED_TEST_SUITE (BlunterClipperTests, TestTypes);

TYPED_TEST (BlunterClipperTests, DefaultConstruction) { this->testDefaultConstruction(); }

TYPED_TEST (BlunterClipperTests, ParameterizedConstruction) { this->testParameterizedConstruction(); }

TYPED_TEST (BlunterClipperTests, SetParameters) { this->testSetParameters(); }

TYPED_TEST (BlunterClipperTests, SanitizesGainParameters) { this->testSanitizesGainParameters(); }

TYPED_TEST (BlunterClipperTests, CanonicalFormulaUnityGains) { this->testCanonicalFormulaUnityGains(); }

TYPED_TEST (BlunterClipperTests, BoundedOutput) { this->testBoundedOutput(); }

TYPED_TEST (BlunterClipperTests, Monotonicity) { this->testMonotonicity(); }

TYPED_TEST (BlunterClipperTests, Symmetry) { this->testSymmetry(); }

TYPED_TEST (BlunterClipperTests, ZeroInput) { this->testZeroInput(); }

TYPED_TEST (BlunterClipperTests, SlopeAtOriginIsTwo) { this->testSlopeAtOriginIsTwo(); }

TYPED_TEST (BlunterClipperTests, InputGainScalesDistortion) { this->testInputGainScalesDistortion(); }

TYPED_TEST (BlunterClipperTests, OutputGainScalesResult) { this->testOutputGainScalesResult(); }

TYPED_TEST (BlunterClipperTests, UnityGainAtOriginWithHalfOutputGain) { this->testUnityGainAtOriginWithHalfOutputGain(); }

TYPED_TEST (BlunterClipperTests, BlockProcessing) { this->testBlockProcessing(); }

TYPED_TEST (BlunterClipperTests, InPlaceProcessing) { this->testInPlaceProcessing(); }

TYPED_TEST (BlunterClipperTests, ResetAndPrepareAreNoOps) { this->testResetAndPrepareAreNoOps(); }

TYPED_TEST (BlunterClipperTests, SecondDerivativeIsConstantInKnee) { this->testSecondDerivativeIsConstantInKnee(); }
