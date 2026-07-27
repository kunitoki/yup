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

#include "yup_core/yup_core.h"
#include "yup_dsp/yup_dsp.h"

#include <gtest/gtest.h>

template <typename FloatType>
class SoftClipperTests : public ::testing::Test
{
public:
    using Clipper = yup::SoftClipper<FloatType>;

    void testDefaultConstruction()
    {
        Clipper clipper;
        EXPECT_NEAR (clipper.getMaxAmplitude(), FloatType (1), FloatType (1e-5));
        EXPECT_NEAR (clipper.getAmount(), FloatType (0.85), FloatType (1e-5));
    }

    void testParameterizedConstruction()
    {
        Clipper clipper (FloatType (2), FloatType (0.5));
        EXPECT_NEAR (clipper.getMaxAmplitude(), FloatType (2), FloatType (1e-5));
        EXPECT_NEAR (clipper.getAmount(), FloatType (0.5), FloatType (1e-5));
    }

    void testPassThrough()
    {
        Clipper clipper;
        FloatType threshold = clipper.getMaxAmplitude() * clipper.getAmount();

        // Test values below threshold should pass through unchanged
        FloatType testValue = threshold * FloatType (0.5);
        EXPECT_NEAR (clipper.processSample (testValue), testValue, FloatType (1e-5));
        EXPECT_NEAR (clipper.processSample (-testValue), -testValue, FloatType (1e-5));
    }

    void testPositiveClipping()
    {
        Clipper clipper;
        FloatType maxAmp = clipper.getMaxAmplitude();

        // Test clipping for values above threshold
        FloatType input = maxAmp * FloatType (0.95);
        FloatType output = clipper.processSample (input);

        // Output should be less than input but greater than threshold
        EXPECT_LT (output, input);
        EXPECT_GT (output, clipper.getMaxAmplitude() * clipper.getAmount());

        // Test extreme value
        input = maxAmp * FloatType (2);
        output = clipper.processSample (input);
        EXPECT_LT (output, maxAmp);
        EXPECT_GT (output, FloatType (0));
    }

    void testNegativeClipping()
    {
        Clipper clipper;
        FloatType maxAmp = clipper.getMaxAmplitude();

        // Test clipping for values below negative threshold
        FloatType input = -maxAmp * FloatType (0.95);
        FloatType output = clipper.processSample (input);

        // Output should be greater than input but less than negative threshold
        EXPECT_GT (output, input);
        EXPECT_LT (output, -clipper.getMaxAmplitude() * clipper.getAmount());

        // Test extreme value
        input = -maxAmp * FloatType (2);
        output = clipper.processSample (input);
        EXPECT_GT (output, -maxAmp);
        EXPECT_LT (output, FloatType (0));
    }

    void testSetParameters()
    {
        Clipper clipper;

        clipper.setMaxAmplitude (FloatType (2));
        EXPECT_NEAR (clipper.getMaxAmplitude(), FloatType (2), FloatType (1e-5));

        clipper.setAmount (FloatType (0.7));
        EXPECT_NEAR (clipper.getAmount(), FloatType (0.7), FloatType (1e-5));

        clipper.setParameters (FloatType (3), FloatType (0.9));
        EXPECT_NEAR (clipper.getMaxAmplitude(), FloatType (3), FloatType (1e-5));
        EXPECT_NEAR (clipper.getAmount(), FloatType (0.9), FloatType (1e-5));
    }

    void testBlockProcessing()
    {
        Clipper clipper;
        const int numSamples = 10;
        FloatType input[numSamples];
        FloatType output[numSamples];

        // Fill with test values
        for (int i = 0; i < numSamples; ++i)
        {
            input[i] = FloatType (i - 5) * FloatType (0.3);
        }

        // Process block
        clipper.processBlock (input, output, numSamples);

        // Verify each sample
        for (int i = 0; i < numSamples; ++i)
        {
            FloatType expected = clipper.processSample (input[i]);
            EXPECT_NEAR (output[i], expected, FloatType (1e-5));
        }
    }

    void testInPlaceProcessing()
    {
        Clipper clipper;
        const int numSamples = 10;
        FloatType data[numSamples];
        FloatType backup[numSamples];

        // Fill with test values and backup
        for (int i = 0; i < numSamples; ++i)
        {
            data[i] = FloatType (i - 5) * FloatType (0.3);
            backup[i] = data[i];
        }

        // Process in-place
        clipper.processBlock (data, data, numSamples);

        // Verify each sample
        for (int i = 0; i < numSamples; ++i)
        {
            FloatType expected = clipper.processSample (backup[i]);
            EXPECT_NEAR (data[i], expected, FloatType (1e-5));
        }
    }

    void testExtremeCases()
    {
        Clipper clipper;

        // Test very small values (should pass through)
        FloatType tiny = std::numeric_limits<FloatType>::epsilon();
        EXPECT_NEAR (clipper.processSample (tiny), tiny, FloatType (1e-5));
        EXPECT_NEAR (clipper.processSample (-tiny), -tiny, FloatType (1e-5));

        // Test zero
        EXPECT_NEAR (clipper.processSample (FloatType (0)), FloatType (0), FloatType (1e-5));

        // Test very large values
        FloatType huge = std::numeric_limits<FloatType>::max() / FloatType (2);
        FloatType clipped = clipper.processSample (huge);
        EXPECT_LE (clipped, clipper.getMaxAmplitude());
        EXPECT_GT (clipped, FloatType (0));
    }

    void testAmountParameter()
    {
        FloatType maxAmp = FloatType (1);

        // Test with amount = 0 (clipping starts immediately)
        Clipper clipper1 (maxAmp, FloatType (0));
        FloatType output1 = clipper1.processSample (FloatType (0.1));
        EXPECT_LT (output1, FloatType (0.1));

        // Test with amount = 1 (no clipping until maxAmplitude)
        Clipper clipper2 (maxAmp, FloatType (1));
        FloatType output2 = clipper2.processSample (FloatType (0.99));
        EXPECT_NEAR (output2, FloatType (0.99), FloatType (1e-5));

        // Test with amount = 0.5
        Clipper clipper3 (maxAmp, FloatType (0.5));
        FloatType threshold3 = maxAmp * FloatType (0.5);
        FloatType belowThreshold = threshold3 * FloatType (0.9);
        FloatType aboveThreshold = threshold3 * FloatType (1.1);

        EXPECT_NEAR (clipper3.processSample (belowThreshold), belowThreshold, FloatType (1e-5));
        EXPECT_LT (clipper3.processSample (aboveThreshold), aboveThreshold);
    }

    void testMaxAmplitudeScaling()
    {
        // Test with different max amplitudes
        for (FloatType maxAmp : { FloatType (0.5), FloatType (1), FloatType (2), FloatType (10) })
        {
            Clipper clipper (maxAmp, FloatType (0.8));

            // Value at 90% of max should be clipped
            FloatType input = maxAmp * FloatType (0.9);
            FloatType output = clipper.processSample (input);

            // Output should be less than input but not exceed maxAmp
            EXPECT_LT (output, input);
            EXPECT_LT (output, maxAmp);

            // Very large input should approach but not exceed maxAmp
            FloatType hugeInput = maxAmp * FloatType (100);
            FloatType hugeOutput = clipper.processSample (hugeInput);
            EXPECT_LT (hugeOutput, maxAmp);
            EXPECT_GT (hugeOutput, maxAmp * FloatType (0.8));
        }
    }
};

// Define typed tests for float and double
using TestTypes = ::testing::Types<float, double>;
TYPED_TEST_SUITE (SoftClipperTests, TestTypes);

TYPED_TEST (SoftClipperTests, DefaultConstruction)
{
    this->testDefaultConstruction();
}

TYPED_TEST (SoftClipperTests, ParameterizedConstruction)
{
    this->testParameterizedConstruction();
}

TYPED_TEST (SoftClipperTests, PassThrough)
{
    this->testPassThrough();
}

TYPED_TEST (SoftClipperTests, PositiveClipping)
{
    this->testPositiveClipping();
}

TYPED_TEST (SoftClipperTests, NegativeClipping)
{
    this->testNegativeClipping();
}

TYPED_TEST (SoftClipperTests, SetParameters)
{
    this->testSetParameters();
}

TYPED_TEST (SoftClipperTests, BlockProcessing)
{
    this->testBlockProcessing();
}

TYPED_TEST (SoftClipperTests, InPlaceProcessing)
{
    this->testInPlaceProcessing();
}

TYPED_TEST (SoftClipperTests, ExtremeCases)
{
    this->testExtremeCases();
}

TYPED_TEST (SoftClipperTests, AmountParameter)
{
    this->testAmountParameter();
}

TYPED_TEST (SoftClipperTests, MaxAmplitudeScaling)
{
    this->testMaxAmplitudeScaling();
}
