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

#include <yup_dsp/yup_dsp.h>

#include <gtest/gtest.h>

using namespace yup;

namespace
{
constexpr double tolerance = 1e-4;
constexpr float toleranceF = 1e-4f;
constexpr double sampleRate = 44100.0;
constexpr int blockSize = 256;
} // namespace

//==============================================================================
class BiquadCascadeFilterTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        cascadeFloat.prepare (sampleRate, blockSize);
        cascadeDouble.prepare (sampleRate, blockSize);

        // Initialize test vectors
        testData.resize (blockSize);
        outputData.resize (blockSize);
        doubleTestData.resize (blockSize);
        doubleOutputData.resize (blockSize);

        for (int i = 0; i < blockSize; ++i)
        {
            testData[i] = 0.1f * std::sin (2.0f * MathConstants<float>::pi * 1000.0f * i / static_cast<float> (sampleRate));
            doubleTestData[i] = static_cast<double> (testData[i]);
        }
    }

    BiquadCascade<float> cascadeFloat { 2 };
    BiquadCascade<double> cascadeDouble { 2 };

    std::vector<float> testData;
    std::vector<float> outputData;
    std::vector<double> doubleTestData;
    std::vector<double> doubleOutputData;
};

//==============================================================================
TEST_F (BiquadCascadeFilterTests, DefaultConstructorInitializes)
{
    BiquadCascade<float> defaultCascade;
    EXPECT_EQ (1, defaultCascade.getNumSections());
}

TEST_F (BiquadCascadeFilterTests, ConstructorWithSectionsInitializes)
{
    BiquadCascade<float> cascade (4);
    EXPECT_EQ (4, cascade.getNumSections());
}

TEST_F (BiquadCascadeFilterTests, SectionManagement)
{
    cascadeFloat.setNumSections (3);
    EXPECT_EQ (cascadeFloat.getNumSections(), 3u);

    // Set coefficients for each section
    auto coeffs1 = FilterDesigner<double>::designRbjLowpass (500.0, 0.707, sampleRate);
    auto coeffs2 = FilterDesigner<double>::designRbjBandpass (1000.0, 2.0, sampleRate);
    auto coeffs3 = FilterDesigner<double>::designRbjHighpass (2000.0, 0.707, sampleRate);

    cascadeFloat.setSectionCoefficients (0, coeffs1);
    cascadeFloat.setSectionCoefficients (1, coeffs2);
    cascadeFloat.setSectionCoefficients (2, coeffs3);

    // Verify coefficients were set correctly
    auto retrievedCoeffs1 = cascadeFloat.getSectionCoefficients (0);
    EXPECT_FLOAT_EQ (retrievedCoeffs1.b0, coeffs1.b0);
    EXPECT_FLOAT_EQ (retrievedCoeffs1.a1, coeffs1.a1);
}

TEST_F (BiquadCascadeFilterTests, SetAndGetSectionCoefficients)
{
    // Create lowpass coefficients
    auto coeffs = FilterDesigner<double>::designRbjLowpass (1000.0, 0.707, sampleRate);

    cascadeFloat.setSectionCoefficients (0, coeffs);
    auto retrievedCoeffs = cascadeFloat.getSectionCoefficients (0);

    EXPECT_NEAR (coeffs.b0, retrievedCoeffs.b0, tolerance);
    EXPECT_NEAR (coeffs.b1, retrievedCoeffs.b1, tolerance);
    EXPECT_NEAR (coeffs.b2, retrievedCoeffs.b2, tolerance);
    EXPECT_NEAR (coeffs.a1, retrievedCoeffs.a1, tolerance);
    EXPECT_NEAR (coeffs.a2, retrievedCoeffs.a2, tolerance);
}

TEST_F (BiquadCascadeFilterTests, InvalidSectionIndexHandling)
{
    auto coeffs = FilterDesigner<double>::designRbjLowpass (1000.0, 0.707, sampleRate);

    // Should not crash with invalid index
    cascadeFloat.setSectionCoefficients (999, coeffs);

    // Should return empty coefficients for invalid index
    auto emptyCoeffs = cascadeFloat.getSectionCoefficients (999);
    EXPECT_EQ (1.0, emptyCoeffs.b0); // Default biquad passes through (b0=1)
    EXPECT_EQ (0.0, emptyCoeffs.b1);
    EXPECT_EQ (0.0, emptyCoeffs.b2);
    EXPECT_EQ (0.0, emptyCoeffs.a1);
    EXPECT_EQ (0.0, emptyCoeffs.a2);
}

TEST_F (BiquadCascadeFilterTests, InvalidSectionAccess)
{
    cascadeFloat.setNumSections (2);

    // Trying to access section 5 when only 2 sections exist should not crash
    auto coeffs = cascadeFloat.getSectionCoefficients (5);
    // Should return default/empty coefficients
    EXPECT_TRUE (std::isfinite (coeffs.b0));
}

TEST_F (BiquadCascadeFilterTests, DynamicSectionResize)
{
    // Start with 1 section
    cascadeFloat.setNumSections (1);
    EXPECT_EQ (cascadeFloat.getNumSections(), 1u);

    // Expand to 4 sections
    cascadeFloat.setNumSections (4);
    EXPECT_EQ (cascadeFloat.getNumSections(), 4u);

    // Shrink to 2 sections
    cascadeFloat.setNumSections (2);
    EXPECT_EQ (cascadeFloat.getNumSections(), 2u);

    // Should still process correctly after resize
    cascadeFloat.processBlock (testData.data(), outputData.data(), blockSize);

    for (int i = 0; i < blockSize; ++i)
    {
        EXPECT_TRUE (std::isfinite (outputData[i]));
    }
}

TEST_F (BiquadCascadeFilterTests, ProcessesFloatSamples)
{
    // Set up lowpass filter on first section
    auto coeffs = FilterDesigner<double>::designRbjLowpass (1000.0, 0.707, sampleRate);
    cascadeFloat.setSectionCoefficients (0, coeffs);

    cascadeFloat.processBlock (testData.data(), outputData.data(), blockSize);

    // Output should be different from input (filtered)
    bool outputDiffers = false;
    for (int i = 0; i < blockSize; ++i)
    {
        if (std::abs (outputData[i] - testData[i]) > toleranceF)
        {
            outputDiffers = true;
            break;
        }
    }
    EXPECT_TRUE (outputDiffers);

    // Output should not contain NaN or inf
    for (int i = 0; i < blockSize; ++i)
    {
        EXPECT_TRUE (std::isfinite (outputData[i]));
    }
}

TEST_F (BiquadCascadeFilterTests, ProcessesDoubleSamples)
{
    // Set up lowpass filter on first section
    auto coeffs = FilterDesigner<double>::designRbjLowpass (1000.0, 0.707, sampleRate);
    cascadeDouble.setSectionCoefficients (0, coeffs);

    cascadeDouble.processBlock (doubleTestData.data(), doubleOutputData.data(), blockSize);

    // Output should be different from input (filtered)
    bool outputDiffers = false;
    for (int i = 0; i < blockSize; ++i)
    {
        if (std::abs (doubleOutputData[i] - doubleTestData[i]) > tolerance)
        {
            outputDiffers = true;
            break;
        }
    }
    EXPECT_TRUE (outputDiffers);

    // Output should not contain NaN or inf
    for (int i = 0; i < blockSize; ++i)
    {
        EXPECT_TRUE (std::isfinite (doubleOutputData[i]));
    }
}

TEST_F (BiquadCascadeFilterTests, MultipleSectionsCascadeCorrectly)
{
    // Set up two identical lowpass sections
    auto coeffs = FilterDesigner<double>::designRbjLowpass (1000.0, 0.707, sampleRate);

    cascadeDouble.setSectionCoefficients (0, coeffs);
    cascadeDouble.setSectionCoefficients (1, coeffs);

    // Process with cascade
    cascadeDouble.processBlock (doubleTestData.data(), doubleOutputData.data(), blockSize);

    // Create single section for comparison
    BiquadCascade<double> singleSection (1);
    singleSection.prepare (sampleRate, blockSize);
    singleSection.setSectionCoefficients (0, coeffs);

    std::vector<double> singleOutput (blockSize);
    singleSection.processBlock (doubleTestData.data(), singleOutput.data(), blockSize);

    // The two-section cascade should have more attenuation than single section
    double cascadeEnergy = 0.0;
    double singleEnergy = 0.0;

    for (int i = 0; i < blockSize; ++i)
    {
        cascadeEnergy += doubleOutputData[i] * doubleOutputData[i];
        singleEnergy += singleOutput[i] * singleOutput[i];
    }

    // Cascade should have less energy (more filtering)
    EXPECT_LT (cascadeEnergy, singleEnergy);
}

TEST_F (BiquadCascadeFilterTests, ProcessingThroughCascade)
{
    cascadeFloat.setNumSections (3);

    // Set up a multi-stage filter
    auto lowpass = FilterDesigner<double>::designRbjLowpass (2000.0, 0.707, sampleRate);
    auto peak = FilterDesigner<double>::designRbjPeak (1000.0, 2.0, 6.0, sampleRate);
    auto highpass = FilterDesigner<double>::designRbjHighpass (500.0, 0.707, sampleRate);

    cascadeFloat.setSectionCoefficients (0, lowpass);
    cascadeFloat.setSectionCoefficients (1, peak);
    cascadeFloat.setSectionCoefficients (2, highpass);

    cascadeFloat.processBlock (testData.data(), outputData.data(), blockSize);

    for (int i = 0; i < blockSize; ++i)
    {
        EXPECT_TRUE (std::isfinite (outputData[i]));
    }
}

TEST_F (BiquadCascadeFilterTests, EmptyCascade)
{
    cascadeFloat.setNumSections (0);
    EXPECT_EQ (cascadeFloat.getNumSections(), 0u);

    // Processing through empty cascade should pass signal through unchanged
    cascadeFloat.processBlock (testData.data(), outputData.data(), blockSize);

    for (int i = 0; i < blockSize; ++i)
    {
        EXPECT_FLOAT_EQ (outputData[i], testData[i]);
    }
}

TEST_F (BiquadCascadeFilterTests, InPlaceProcessing)
{
    auto coeffs = FilterDesigner<double>::designRbjLowpass (1000.0, 0.707, sampleRate);
    cascadeFloat.setSectionCoefficients (0, coeffs);

    // Make a copy for comparison
    std::vector<float> originalData = testData;

    // Process in-place
    cascadeFloat.processBlock (testData.data(), testData.data(), blockSize);

    // Output should be different from original
    bool outputDiffers = false;
    for (int i = 0; i < blockSize; ++i)
    {
        if (std::abs (testData[i] - originalData[i]) > toleranceF)
        {
            outputDiffers = true;
            break;
        }
    }
    EXPECT_TRUE (outputDiffers);
}

TEST_F (BiquadCascadeFilterTests, ResetClearsState)
{
    auto coeffs = FilterDesigner<double>::designRbjLowpass (1000.0, 0.707, sampleRate);
    cascadeFloat.setSectionCoefficients (0, coeffs);

    // Process some data to build up state
    cascadeFloat.processBlock (testData.data(), outputData.data(), blockSize);

    // Reset and process impulse
    cascadeFloat.reset();

    std::vector<float> impulse (blockSize, 0.0f);
    impulse[0] = 1.0f;

    cascadeFloat.processBlock (impulse.data(), outputData.data(), blockSize);

    // First output should be b0 coefficient (impulse response)
    EXPECT_NEAR (coeffs.b0, outputData[0], toleranceF);
}

TEST_F (BiquadCascadeFilterTests, CascadeStateReset)
{
    cascadeFloat.setNumSections (2);

    auto coeffs = FilterDesigner<double>::designRbjLowpass (1000.0, 0.707, sampleRate);
    cascadeFloat.setSectionCoefficients (0, coeffs);
    cascadeFloat.setSectionCoefficients (1, coeffs);

    // Build up internal state
    for (int i = 0; i < 50; ++i)
        cascadeFloat.processSample (1.0f);

    auto outputBeforeReset = cascadeFloat.processSample (0.0f);

    cascadeFloat.reset();
    auto outputAfterReset = cascadeFloat.processSample (0.0f);

    // After reset, the output should be closer to zero
    EXPECT_LT (std::abs (outputAfterReset), std::abs (outputBeforeReset));
}

TEST_F (BiquadCascadeFilterTests, CascadeFrequencyResponse)
{
    cascadeFloat.setNumSections (2);

    // Two identical lowpass filters in cascade
    auto coeffs = FilterDesigner<double>::designRbjLowpass (1000.0, 0.707, sampleRate);
    cascadeFloat.setSectionCoefficients (0, coeffs);
    cascadeFloat.setSectionCoefficients (1, coeffs);

    // Overall response should be the product of individual responses
    auto singleResponse = std::abs (BiquadFloat (BiquadFloat::Topology::directFormII).getComplexResponse (1000.0));
    BiquadFloat singleFilter;
    singleFilter.setCoefficients (coeffs);
    singleResponse = std::abs (singleFilter.getComplexResponse (1000.0));

    auto cascadeResponse = std::abs (cascadeFloat.getComplexResponse (1000.0));
    auto expectedResponse = singleResponse * singleResponse;

    EXPECT_NEAR (cascadeResponse, expectedResponse, 0.1f);
}

TEST_F (BiquadCascadeFilterTests, ImpulseResponseCharacteristics)
{
    // Set up lowpass filter
    auto coeffs = FilterDesigner<double>::designRbjLowpass (1000.0, 0.707, sampleRate);
    cascadeFloat.setSectionCoefficients (0, coeffs);

    // Create impulse
    std::vector<float> impulse (blockSize, 0.0f);
    impulse[0] = 1.0f;

    cascadeFloat.reset();
    cascadeFloat.processBlock (impulse.data(), outputData.data(), blockSize);

    // Impulse response should start with b0 and decay
    EXPECT_NEAR (coeffs.b0, outputData[0], toleranceF);

    // Response should be finite
    for (int i = 0; i < blockSize; ++i)
    {
        EXPECT_TRUE (std::isfinite (outputData[i]));
    }
}

TEST_F (BiquadCascadeFilterTests, StabilityCheck)
{
    // Create a high-Q filter that could become unstable
    auto coeffs = FilterDesigner<double>::designRbjLowpass (5000.0, 50.0, sampleRate);
    cascadeFloat.setSectionCoefficients (0, coeffs);

    // Process white noise-like signal
    std::vector<float> noiseInput (blockSize);
    WhiteNoise noise;
    for (int i = 0; i < blockSize; ++i)
        noiseInput[i] = noise.getNextSample();

    cascadeFloat.processBlock (noiseInput.data(), outputData.data(), blockSize);

    // Output should remain finite
    for (int i = 0; i < blockSize; ++i)
    {
        EXPECT_TRUE (std::isfinite (outputData[i]));
        EXPECT_LT (std::abs (outputData[i]), 10.0f); // Reasonable bounds
    }
}

TEST_F (BiquadCascadeFilterTests, CascadeVsManualChaining)
{
    // Compare cascade processing with manual chaining of individual biquads
    auto coeffs1 = FilterDesigner<double>::designRbjLowpass (1000.0, 0.707, sampleRate);
    auto coeffs2 = FilterDesigner<double>::designRbjHighpass (500.0, 0.707, sampleRate);

    // Set up cascade
    cascadeFloat.setNumSections (2);
    cascadeFloat.setSectionCoefficients (0, coeffs1);
    cascadeFloat.setSectionCoefficients (1, coeffs2);

    // Set up manual chain
    BiquadFloat filter1, filter2;
    filter1.prepare (sampleRate, blockSize);
    filter2.prepare (sampleRate, blockSize);
    filter1.setCoefficients (coeffs1);
    filter2.setCoefficients (coeffs2);

    std::vector<float> cascadeOutput (blockSize);
    std::vector<float> manualOutput (blockSize);
    std::vector<float> tempOutput (blockSize);

    // Process through cascade
    cascadeFloat.processBlock (testData.data(), cascadeOutput.data(), blockSize);

    // Process through manual chain
    filter1.processBlock (testData.data(), tempOutput.data(), blockSize);
    filter2.processBlock (tempOutput.data(), manualOutput.data(), blockSize);

    // Results should be identical
    for (int i = 0; i < blockSize; ++i)
    {
        EXPECT_NEAR (cascadeOutput[i], manualOutput[i], toleranceF);
    }
}

TEST_F (BiquadCascadeFilterTests, LargeCascade)
{
    // Test with many sections
    const int numSections = 10;
    cascadeFloat.setNumSections (numSections);
    EXPECT_EQ (cascadeFloat.getNumSections(), static_cast<size_t> (numSections));

    // Set mild filtering on each section
    auto coeffs = FilterDesigner<double>::designRbjLowpass (5000.0, 0.707, sampleRate);
    for (int i = 0; i < numSections; ++i)
    {
        cascadeFloat.setSectionCoefficients (static_cast<size_t> (i), coeffs);
    }

    cascadeFloat.processBlock (testData.data(), outputData.data(), blockSize);

    for (int i = 0; i < blockSize; ++i)
    {
        EXPECT_TRUE (std::isfinite (outputData[i]));
    }
}
