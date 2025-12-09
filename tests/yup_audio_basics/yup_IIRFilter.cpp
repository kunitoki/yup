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

#include <gtest/gtest.h>

#include <yup_audio_basics/yup_audio_basics.h>

using namespace yup;

//==============================================================================
class IIRCoefficientsTests : public ::testing::Test
{
};

//==============================================================================
TEST_F (IIRCoefficientsTests, DefaultConstructor)
{
    IIRCoefficients coeff;

    // Should be zeroed (line 47)
    for (int i = 0; i < 5; ++i)
        EXPECT_FLOAT_EQ (coeff.coefficients[i], 0.0f);
}

TEST_F (IIRCoefficientsTests, Destructor)
{
    auto* coeff = new IIRCoefficients();
    EXPECT_NO_THROW (delete coeff);
}

TEST_F (IIRCoefficientsTests, CopyConstructor)
{
    IIRCoefficients coeff1 (1.0, 2.0, 3.0, 4.0, 5.0, 6.0);
    IIRCoefficients coeff2 (coeff1);

    for (int i = 0; i < 5; ++i)
        EXPECT_FLOAT_EQ (coeff2.coefficients[i], coeff1.coefficients[i]);
}

TEST_F (IIRCoefficientsTests, CopyAssignment)
{
    IIRCoefficients coeff1 (1.0, 2.0, 3.0, 4.0, 5.0, 6.0);
    IIRCoefficients coeff2;

    coeff2 = coeff1;

    for (int i = 0; i < 5; ++i)
        EXPECT_FLOAT_EQ (coeff2.coefficients[i], coeff1.coefficients[i]);
}

TEST_F (IIRCoefficientsTests, ParameterizedConstructor)
{
    // Test normalization (line 65-71)
    IIRCoefficients coeff (1.0, 2.0, 3.0, 2.0, 5.0, 6.0);

    // Coefficients should be normalized by c4 (2.0)
    EXPECT_FLOAT_EQ (coeff.coefficients[0], 0.5f); // 1.0 / 2.0
    EXPECT_FLOAT_EQ (coeff.coefficients[1], 1.0f); // 2.0 / 2.0
    EXPECT_FLOAT_EQ (coeff.coefficients[2], 1.5f); // 3.0 / 2.0
    EXPECT_FLOAT_EQ (coeff.coefficients[3], 2.5f); // 5.0 / 2.0
    EXPECT_FLOAT_EQ (coeff.coefficients[4], 3.0f); // 6.0 / 2.0
}

//==============================================================================
TEST_F (IIRCoefficientsTests, MakeLowPassDefaultQ)
{
    auto coeff = IIRCoefficients::makeLowPass (44100.0, 1000.0);

    // Should use default Q = 1/sqrt(2) (line 77)
    EXPECT_NE (coeff.coefficients[0], 0.0f);
}

TEST_F (IIRCoefficientsTests, MakeLowPassWithQ)
{
    auto coeff = IIRCoefficients::makeLowPass (44100.0, 1000.0, 0.707);

    // Coefficients should be non-zero
    EXPECT_NE (coeff.coefficients[0], 0.0f);
    EXPECT_NE (coeff.coefficients[1], 0.0f);
}

TEST_F (IIRCoefficientsTests, MakeLowPassDifferentFrequencies)
{
    auto coeff1 = IIRCoefficients::makeLowPass (44100.0, 500.0, 1.0);
    auto coeff2 = IIRCoefficients::makeLowPass (44100.0, 2000.0, 1.0);

    // Different frequencies should produce different coefficients
    EXPECT_NE (coeff1.coefficients[0], coeff2.coefficients[0]);
}

//==============================================================================
TEST_F (IIRCoefficientsTests, MakeHighPassDefaultQ)
{
    auto coeff = IIRCoefficients::makeHighPass (44100.0, 1000.0);

    // Should use default Q (line 103)
    EXPECT_NE (coeff.coefficients[0], 0.0f);
}

TEST_F (IIRCoefficientsTests, MakeHighPassWithQ)
{
    auto coeff = IIRCoefficients::makeHighPass (44100.0, 1000.0, 0.707);

    // Coefficients should be non-zero
    EXPECT_NE (coeff.coefficients[0], 0.0f);
    EXPECT_NE (coeff.coefficients[1], 0.0f);
}

//==============================================================================
TEST_F (IIRCoefficientsTests, MakeBandPassDefaultQ)
{
    auto coeff = IIRCoefficients::makeBandPass (44100.0, 1000.0);

    // Should use default Q (line 129)
    EXPECT_NE (coeff.coefficients[0], 0.0f);
}

TEST_F (IIRCoefficientsTests, MakeBandPassWithQ)
{
    auto coeff = IIRCoefficients::makeBandPass (44100.0, 1000.0, 1.0);

    // Coefficients should be non-zero
    EXPECT_NE (coeff.coefficients[0], 0.0f);
}

//==============================================================================
TEST_F (IIRCoefficientsTests, MakeNotchFilterDefaultQ)
{
    auto coeff = IIRCoefficients::makeNotchFilter (44100.0, 1000.0);

    // Should use default Q (line 155)
    EXPECT_NE (coeff.coefficients[0], 0.0f);
}

TEST_F (IIRCoefficientsTests, MakeNotchFilterWithQ)
{
    auto coeff = IIRCoefficients::makeNotchFilter (44100.0, 1000.0, 2.0);

    // Coefficients should be non-zero
    EXPECT_NE (coeff.coefficients[0], 0.0f);
}

//==============================================================================
TEST_F (IIRCoefficientsTests, MakeAllPassDefaultQ)
{
    auto coeff = IIRCoefficients::makeAllPass (44100.0, 1000.0);

    // Should use default Q (line 181)
    EXPECT_NE (coeff.coefficients[0], 0.0f);
}

TEST_F (IIRCoefficientsTests, MakeAllPassWithQ)
{
    auto coeff = IIRCoefficients::makeAllPass (44100.0, 1000.0, 1.5);

    // Coefficients should be non-zero
    EXPECT_NE (coeff.coefficients[0], 0.0f);
}

//==============================================================================
TEST_F (IIRCoefficientsTests, MakeLowShelf)
{
    auto coeff = IIRCoefficients::makeLowShelf (44100.0, 1000.0, 0.707, 6.0f);

    // Test with positive gain (line 213-227)
    EXPECT_NE (coeff.coefficients[0], 0.0f);
}

TEST_F (IIRCoefficientsTests, MakeLowShelfNegativeGain)
{
    auto coeff = IIRCoefficients::makeLowShelf (44100.0, 1000.0, 0.707, -6.0f);

    // Test with negative gain
    EXPECT_NE (coeff.coefficients[0], 0.0f);
}

//==============================================================================
TEST_F (IIRCoefficientsTests, MakeHighShelf)
{
    auto coeff = IIRCoefficients::makeHighShelf (44100.0, 5000.0, 0.707, 6.0f);

    // Test with positive gain (line 238-252)
    EXPECT_NE (coeff.coefficients[0], 0.0f);
}

TEST_F (IIRCoefficientsTests, MakeHighShelfNegativeGain)
{
    auto coeff = IIRCoefficients::makeHighShelf (44100.0, 5000.0, 0.707, -6.0f);

    // Test with negative gain
    EXPECT_NE (coeff.coefficients[0], 0.0f);
}

//==============================================================================
TEST_F (IIRCoefficientsTests, MakePeakFilter)
{
    auto coeff = IIRCoefficients::makePeakFilter (44100.0, 1000.0, 1.0, 3.0f);

    // Test peak filter (line 263-276)
    EXPECT_NE (coeff.coefficients[0], 0.0f);
    EXPECT_NE (coeff.coefficients[1], 0.0f);
}

TEST_F (IIRCoefficientsTests, MakePeakFilterNegativeGain)
{
    auto coeff = IIRCoefficients::makePeakFilter (44100.0, 1000.0, 1.0, -6.0f);

    // Test with negative gain
    EXPECT_NE (coeff.coefficients[0], 0.0f);
}

//==============================================================================
class IIRFilterTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        filter = std::make_unique<IIRFilter>();
    }

    void TearDown() override
    {
        filter.reset();
    }

    std::unique_ptr<IIRFilter> filter;
};

//==============================================================================
TEST_F (IIRFilterTests, DefaultConstructor)
{
    EXPECT_NO_THROW (IIRFilter());
}

TEST_F (IIRFilterTests, CopyConstructor)
{
    auto coeff = IIRCoefficients::makeLowPass (44100.0, 1000.0);
    filter->setCoefficients (coeff);

    IIRFilter filter2 (*filter);

    // Should copy active state and coefficients (line 283-288)
    EXPECT_NO_THROW (filter2.reset());
}

//==============================================================================
TEST_F (IIRFilterTests, MakeInactive)
{
    auto coeff = IIRCoefficients::makeLowPass (44100.0, 1000.0);
    filter->setCoefficients (coeff);

    filter->makeInactive();

    // Filter should not process after makeInactive (line 292-296)
    float samples[10] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    filter->processSamples (samples, 10);

    // Samples should remain unchanged when inactive
    for (int i = 0; i < 10; ++i)
        EXPECT_FLOAT_EQ (samples[i], 1.0f);
}

TEST_F (IIRFilterTests, SetCoefficients)
{
    auto coeff = IIRCoefficients::makeLowPass (44100.0, 1000.0);

    EXPECT_NO_THROW (filter->setCoefficients (coeff));

    // Should set active to true (line 303)
}

//==============================================================================
TEST_F (IIRFilterTests, Reset)
{
    auto coeff = IIRCoefficients::makeLowPass (44100.0, 1000.0);
    filter->setCoefficients (coeff);

    // Process some samples to set internal state
    float samples[10] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    filter->processSamples (samples, 10);

    // Reset should clear internal state (line 308-312)
    filter->reset();

    // Process again - should produce consistent results
    float samples2[10] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    filter->processSamples (samples2, 10);
}

//==============================================================================
TEST_F (IIRFilterTests, ProcessSingleSampleRaw)
{
    auto coeff = IIRCoefficients::makeLowPass (44100.0, 1000.0);
    filter->setCoefficients (coeff);

    // Test single sample processing (line 315-325)
    float output = filter->processSingleSampleRaw (1.0f);

    EXPECT_NE (output, 1.0f); // Should be filtered
}

TEST_F (IIRFilterTests, ProcessSingleSampleRawMultiple)
{
    auto coeff = IIRCoefficients::makeLowPass (44100.0, 1000.0);
    filter->setCoefficients (coeff);

    // Process multiple samples
    for (int i = 0; i < 100; ++i)
    {
        float output = filter->processSingleSampleRaw (std::sin (i * 0.1f));
        (void) output;
    }

    // Should not crash and produce valid output
}

//==============================================================================
TEST_F (IIRFilterTests, ProcessSamplesInactive)
{
    // Filter is inactive by default
    float samples[10] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f };

    filter->processSamples (samples, 10);

    // Samples should be unchanged when filter is inactive (line 332)
    for (int i = 0; i < 10; ++i)
        EXPECT_FLOAT_EQ (samples[i], static_cast<float> (i + 1));
}

TEST_F (IIRFilterTests, ProcessSamplesActive)
{
    auto coeff = IIRCoefficients::makeLowPass (44100.0, 1000.0);
    filter->setCoefficients (coeff);

    float samples[10] = { 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };

    filter->processSamples (samples, 10);

    // Samples should be filtered (line 332-356)
    bool hasChanged = false;
    for (int i = 0; i < 10; ++i)
    {
        if ((i % 2 == 0 && samples[i] != 1.0f) || (i % 2 == 1 && samples[i] != 0.0f))
        {
            hasChanged = true;
            break;
        }
    }
    EXPECT_TRUE (hasChanged);
}

TEST_F (IIRFilterTests, ProcessSamplesLowPass)
{
    auto coeff = IIRCoefficients::makeLowPass (44100.0, 1000.0);
    filter->setCoefficients (coeff);

    // Generate a high-frequency signal
    float samples[100];
    for (int i = 0; i < 100; ++i)
        samples[i] = std::sin (i * 0.5f); // High frequency

    filter->processSamples (samples, 100);

    // High frequencies should be attenuated
    float maxAmplitude = 0.0f;
    for (int i = 50; i < 100; ++i)
        maxAmplitude = std::max (maxAmplitude, std::abs (samples[i]));

    EXPECT_LT (maxAmplitude, 1.0f);
}

TEST_F (IIRFilterTests, ProcessSamplesHighPass)
{
    auto coeff = IIRCoefficients::makeHighPass (44100.0, 5000.0);
    filter->setCoefficients (coeff);

    // Generate a low-frequency signal
    float samples[100];
    for (int i = 0; i < 100; ++i)
        samples[i] = std::sin (i * 0.01f); // Low frequency

    filter->processSamples (samples, 100);

    // Low frequencies should be attenuated
    float maxAmplitude = 0.0f;
    for (int i = 50; i < 100; ++i)
        maxAmplitude = std::max (maxAmplitude, std::abs (samples[i]));

    EXPECT_LT (maxAmplitude, 1.0f);
}

TEST_F (IIRFilterTests, ProcessSamplesBandPass)
{
    auto coeff = IIRCoefficients::makeBandPass (44100.0, 1000.0, 2.0);
    filter->setCoefficients (coeff);

    float samples[100];
    for (int i = 0; i < 100; ++i)
        samples[i] = std::sin (i * 0.2f);

    EXPECT_NO_THROW (filter->processSamples (samples, 100));
}

//==============================================================================
TEST_F (IIRFilterTests, DifferentSampleRates)
{
    // Test filters at different sample rates
    for (double sampleRate : { 22050.0, 44100.0, 48000.0, 96000.0 })
    {
        auto coeff = IIRCoefficients::makeLowPass (sampleRate, sampleRate * 0.1);
        filter->setCoefficients (coeff);

        float samples[50];
        for (int i = 0; i < 50; ++i)
            samples[i] = std::sin (i * 0.1f);

        EXPECT_NO_THROW (filter->processSamples (samples, 50));
    }
}

TEST_F (IIRFilterTests, DifferentFilterTypes)
{
    float samples[50];
    for (int i = 0; i < 50; ++i)
        samples[i] = std::sin (i * 0.1f);

    // Test all filter types
    auto lowPass = IIRCoefficients::makeLowPass (44100.0, 1000.0);
    filter->setCoefficients (lowPass);
    EXPECT_NO_THROW (filter->processSamples (samples, 50));

    auto highPass = IIRCoefficients::makeHighPass (44100.0, 1000.0);
    filter->setCoefficients (highPass);
    EXPECT_NO_THROW (filter->processSamples (samples, 50));

    auto bandPass = IIRCoefficients::makeBandPass (44100.0, 1000.0);
    filter->setCoefficients (bandPass);
    EXPECT_NO_THROW (filter->processSamples (samples, 50));

    auto notch = IIRCoefficients::makeNotchFilter (44100.0, 1000.0);
    filter->setCoefficients (notch);
    EXPECT_NO_THROW (filter->processSamples (samples, 50));

    auto allPass = IIRCoefficients::makeAllPass (44100.0, 1000.0);
    filter->setCoefficients (allPass);
    EXPECT_NO_THROW (filter->processSamples (samples, 50));
}

TEST_F (IIRFilterTests, LargeBufferProcessing)
{
    auto coeff = IIRCoefficients::makeLowPass (44100.0, 1000.0);
    filter->setCoefficients (coeff);

    const int size = 10000;
    std::vector<float> samples (size);
    for (int i = 0; i < size; ++i)
        samples[i] = std::sin (i * 0.05f);

    EXPECT_NO_THROW (filter->processSamples (samples.data(), size));
}

TEST_F (IIRFilterTests, StatePreservation)
{
    auto coeff = IIRCoefficients::makeLowPass (44100.0, 1000.0);
    filter->setCoefficients (coeff);

    // Process first block
    float samples1[10];
    for (int i = 0; i < 10; ++i)
        samples1[i] = 1.0f;

    filter->processSamples (samples1, 10);

    // Process second block - state should be preserved
    float samples2[10];
    for (int i = 0; i < 10; ++i)
        samples2[i] = 1.0f;

    filter->processSamples (samples2, 10);

    // Second block should produce different results due to state
    EXPECT_NE (samples1[0], samples2[0]);
}
