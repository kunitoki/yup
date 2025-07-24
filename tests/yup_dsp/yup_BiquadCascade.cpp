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
            testData[i] = static_cast<float> (i) / blockSize - 0.5f;
            doubleTestData[i] = static_cast<double> (i) / blockSize - 0.5;
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

TEST_F (BiquadCascadeFilterTests, SetNumSectionsChangesSize)
{
    EXPECT_EQ (2, cascadeFloat.getNumSections());

    cascadeFloat.setNumSections (5);
    EXPECT_EQ (5, cascadeFloat.getNumSections());

    cascadeFloat.setNumSections (1);
    EXPECT_EQ (1, cascadeFloat.getNumSections());
}

TEST_F (BiquadCascadeFilterTests, SetAndGetSectionCoefficients)
{
    // Create lowpass coefficients
    auto coeffs = FilterDesigner<double>::designRbjLowpass (1000.0, 0.707, sampleRate);

    cascadeDouble.setSectionCoefficients (0, coeffs);
    auto retrievedCoeffs = cascadeDouble.getSectionCoefficients (0);

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
    cascadeDouble.setSectionCoefficients (999, coeffs);

    // Should return empty coefficients for invalid index
    auto emptyCoeffs = cascadeDouble.getSectionCoefficients (999);
    EXPECT_EQ (1.0, emptyCoeffs.b0); // Default biquad passes through (b0=1)
    EXPECT_EQ (0.0, emptyCoeffs.b1);
    EXPECT_EQ (0.0, emptyCoeffs.b2);
    EXPECT_EQ (0.0, emptyCoeffs.a1);
    EXPECT_EQ (0.0, emptyCoeffs.a2);
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
    for (int i = 0; i < blockSize; ++i)
        noiseInput[i] = (static_cast<float> (rand()) / RAND_MAX) * 2.0f - 1.0f;

    cascadeFloat.processBlock (noiseInput.data(), outputData.data(), blockSize);

    // Output should remain finite
    for (int i = 0; i < blockSize; ++i)
    {
        EXPECT_TRUE (std::isfinite (outputData[i]));
        EXPECT_LT (std::abs (outputData[i]), 10.0f); // Reasonable bounds
    }
}
