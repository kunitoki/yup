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

#include <yup_dsp/yup_dsp.h>

#include <gtest/gtest.h>

using namespace yup;

//==============================================================================
class LevelProcessorTests : public ::testing::Test
{
protected:
    static constexpr float tolerance = 1e-3f; // 0.1% tolerance
    static constexpr double sampleRate = 48000.0;

    void SetUp() override
    {
        processor = std::make_unique<LevelProcessor>();
        processor->setSampleRate (sampleRate);
    }

    std::unique_ptr<LevelProcessor> processor;
};

//==============================================================================
TEST_F (LevelProcessorTests, DefaultConstructorInitializes)
{
    LevelProcessor p;

    EXPECT_GT (p.getSampleRate(), 0.0);
    EXPECT_GT (p.getIntegrationTime(), 0.0);
    EXPECT_GT (p.getFallTime(), 0.0);
}

TEST_F (LevelProcessorTests, SetSampleRateUpdatesRate)
{
    processor->setSampleRate (44100.0);
    EXPECT_DOUBLE_EQ (44100.0, processor->getSampleRate());

    processor->setSampleRate (96000.0);
    EXPECT_DOUBLE_EQ (96000.0, processor->getSampleRate());
}

TEST_F (LevelProcessorTests, SetIntegrationTimeUpdatesTime)
{
    processor->setIntegrationTime (0.3);
    EXPECT_DOUBLE_EQ (0.3, processor->getIntegrationTime());

    processor->setIntegrationTime (1.0);
    EXPECT_DOUBLE_EQ (1.0, processor->getIntegrationTime());
}

TEST_F (LevelProcessorTests, SetFallTimeUpdatesTime)
{
    processor->setFallTime (2.0);
    EXPECT_DOUBLE_EQ (2.0, processor->getFallTime());

    processor->setFallTime (5.0);
    EXPECT_DOUBLE_EQ (5.0, processor->getFallTime());
}

//==============================================================================
TEST_F (LevelProcessorTests, ProcessPeakFindsSilence)
{
    std::vector<float> samples (512, 0.0f);
    float peak = 1.0f;

    processor->processPeak (samples.data(), 512, peak);
    EXPECT_FLOAT_EQ (0.0f, peak);
}

TEST_F (LevelProcessorTests, ProcessPeakFindsMaximumAbsoluteValue)
{
    std::vector<float> samples = { 0.1f, -0.5f, 0.3f, -0.8f, 0.2f };
    float peak = 0.0f;

    processor->processPeak (samples.data(), static_cast<int> (samples.size()), peak);
    EXPECT_FLOAT_EQ (0.8f, peak);
}

TEST_F (LevelProcessorTests, ProcessPeakHandlesPositivePeak)
{
    std::vector<float> samples = { 0.1f, 0.2f, 0.9f, 0.3f, 0.1f };
    float peak = 0.0f;

    processor->processPeak (samples.data(), static_cast<int> (samples.size()), peak);
    EXPECT_FLOAT_EQ (0.9f, peak);
}

TEST_F (LevelProcessorTests, ProcessPeakHandlesNegativePeak)
{
    std::vector<float> samples = { 0.1f, 0.2f, -0.95f, 0.3f, 0.1f };
    float peak = 0.0f;

    processor->processPeak (samples.data(), static_cast<int> (samples.size()), peak);
    EXPECT_FLOAT_EQ (0.95f, peak);
}

//==============================================================================
TEST_F (LevelProcessorTests, ProcessRMSCalculatesSilence)
{
    std::vector<float> samples (512, 0.0f);
    float rms = 1.0f;

    processor->processRMS (samples.data(), 512, rms);
    EXPECT_NEAR (0.0f, rms, tolerance);
}

TEST_F (LevelProcessorTests, ProcessRMSCalculatesSineWave)
{
    // Generate 1kHz sine wave at 0.5 amplitude
    constexpr int numSamples = 4800; // 100ms at 48kHz
    std::vector<float> samples (numSamples);

    constexpr float amplitude = 0.5f;
    constexpr float frequency = 1000.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        const float phase = 2.0f * MathConstants<float>::pi * frequency * i / sampleRate;
        samples[i] = amplitude * std::sin (phase);
    }

    // RMS of sine wave = amplitude / sqrt(2) = 0.5 / 1.41421 ≈ 0.35355
    const float expectedRMS = amplitude / std::sqrt (2.0f);

    // Process multiple times to fill the integration window
    float rms = 0.0f;
    for (int pass = 0; pass < 10; ++pass)
        processor->processRMS (samples.data(), numSamples, rms);

    EXPECT_NEAR (expectedRMS, rms, 0.01f); // 1% tolerance
}

TEST_F (LevelProcessorTests, ProcessRMSHandlesSquareWave)
{
    // Square wave at 0.5 amplitude has RMS = amplitude
    constexpr int numSamples = 4800;
    std::vector<float> samples (numSamples);

    constexpr float amplitude = 0.5f;
    for (int i = 0; i < numSamples; ++i)
        samples[i] = (i % 100 < 50) ? amplitude : -amplitude;

    // Process multiple times to fill integration window
    float rms = 0.0f;
    for (int pass = 0; pass < 10; ++pass)
        processor->processRMS (samples.data(), numSamples, rms);

    EXPECT_NEAR (amplitude, rms, 0.01f);
}

//==============================================================================
TEST_F (LevelProcessorTests, ProcessPeakWithFallDecays)
{
    float peak = 1.0f;
    float output = peak;

    // Apply fall over 1 second with 3s fall time
    processor->processPeakWithFall (0.0f, 1.0, output);

    // After 1 second with 3s linear 26 dB fall: -8.667 dB -> ~0.3687
    EXPECT_LT (output, peak);
    EXPECT_GT (output, 0.34f);
    EXPECT_LT (output, 0.40f);
}

TEST_F (LevelProcessorTests, ProcessPeakWithFallNeverFallsBelowCurrent)
{
    float output = 0.5f;

    // Try to fall to 0.0, but current is 0.8
    processor->processPeakWithFall (0.8f, 0.1, output);

    // Output should be at least current peak
    EXPECT_GE (output, 0.8f);
}

TEST_F (LevelProcessorTests, ProcessPeakWithFallInstantlyRisesOnNewPeak)
{
    float output = 0.3f;

    processor->processPeakWithFall (0.9f, 0.001, output);

    EXPECT_FLOAT_EQ (0.9f, output);
}

//==============================================================================
TEST_F (LevelProcessorTests, CalculateBallisticsConvergesToTarget)
{
    float current = 0.0f;
    float target = 1.0f;
    double timeConstant = 1.0;

    // Apply ballistics over multiple steps
    for (int i = 0; i < 100; ++i)
        current = LevelProcessor::calculateBallistics (current, target, timeConstant, 0.1);

    // Should converge close to target
    EXPECT_NEAR (target, current, 0.01f);
}

TEST_F (LevelProcessorTests, CalculateBallisticsExponentialCurve)
{
    float current = 0.0f;
    float target = 1.0f;
    double timeConstant = 1.0;

    // After 1 time constant, should reach ~63% of target
    current = LevelProcessor::calculateBallistics (current, target, timeConstant, timeConstant);
    EXPECT_NEAR (0.632f, current, 0.01f);

    // After 2 time constants total, should reach ~86% of target
    current = LevelProcessor::calculateBallistics (current, target, timeConstant, timeConstant);
    EXPECT_NEAR (0.865f, current, 0.01f);
}

TEST_F (LevelProcessorTests, CalculateBallisticsInstantWithZeroTimeConstant)
{
    float current = 0.0f;
    float target = 1.0f;

    float result = LevelProcessor::calculateBallistics (current, target, 0.0, 0.1);
    EXPECT_FLOAT_EQ (target, result);
}

TEST_F (LevelProcessorTests, CalculateBallisticsNoChangeWithZeroTimeDelta)
{
    float current = 0.5f;
    float target = 1.0f;

    float result = LevelProcessor::calculateBallistics (current, target, 1.0, 0.0);
    EXPECT_FLOAT_EQ (target, result);
}

//==============================================================================
TEST_F (LevelProcessorTests, ResetClearsState)
{
    // Fill RMS buffer with data
    std::vector<float> samples (512, 0.5f);
    float rms = 0.0f;

    for (int i = 0; i < 10; ++i)
        processor->processRMS (samples.data(), 512, rms);

    EXPECT_GT (rms, 0.0f);

    // Reset should clear state
    processor->reset();

    // Next measurement should start from zero
    std::vector<float> silence (512, 0.0f);
    processor->processRMS (silence.data(), 512, rms);
    EXPECT_NEAR (0.0f, rms, tolerance);
}

//==============================================================================
TEST_F (LevelProcessorTests, IntegrationTimeAffectsRMSSmoothing)
{
    // Short integration time
    LevelProcessor shortProcessor;
    shortProcessor.setSampleRate (sampleRate);
    shortProcessor.setIntegrationTime (0.1); // 100ms

    // Long integration time
    LevelProcessor longProcessor;
    longProcessor.setSampleRate (sampleRate);
    longProcessor.setIntegrationTime (1.0); // 1000ms

    // Generate step change
    std::vector<float> samples (4800, 1.0f); // 100ms of full scale

    float rmsShort = 0.0f;
    float rmsLong = 0.0f;

    shortProcessor.processRMS (samples.data(), 4800, rmsShort);
    longProcessor.processRMS (samples.data(), 4800, rmsLong);

    // Short integration should respond faster (higher value)
    EXPECT_GT (rmsShort, rmsLong);
}

TEST_F (LevelProcessorTests, FallTimeAffectsPeakDecay)
{
    // Fast fall
    LevelProcessor fastProcessor;
    fastProcessor.setSampleRate (sampleRate);
    fastProcessor.setFallTime (0.5); // 500ms

    // Slow fall
    LevelProcessor slowProcessor;
    slowProcessor.setSampleRate (sampleRate);
    slowProcessor.setFallTime (5.0); // 5000ms

    float peakFast = 1.0f;
    float peakSlow = 1.0f;

    // Apply same fall time
    fastProcessor.processPeakWithFall (0.0f, 0.5, peakFast);
    slowProcessor.processPeakWithFall (0.0f, 0.5, peakSlow);

    // Fast should decay more
    EXPECT_LT (peakFast, peakSlow);
}
