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

#include <thread>
#include <atomic>

using namespace yup;

//==============================================================================
class KMeterStateTests : public ::testing::Test
{
protected:
    static constexpr float tolerance = 0.5f; // 0.5 dB tolerance
    static constexpr double sampleRate = 48000.0;

    void SetUp() override
    {
        meter = std::make_unique<KMeterState> (sampleRate, 2);
    }

    std::unique_ptr<KMeterState> meter;
};

//==============================================================================
TEST_F (KMeterStateTests, DefaultConstructorInitializes)
{
    KMeterState m;
    EXPECT_GT (m.getSampleRate(), 0.0);
    EXPECT_GT (m.getNumChannels(), 0);
}

TEST_F (KMeterStateTests, CustomConstructorInitializes)
{
    KMeterState m (44100.0, 4);
    EXPECT_DOUBLE_EQ (44100.0, m.getSampleRate());
    EXPECT_EQ (4, m.getNumChannels());
}

TEST_F (KMeterStateTests, PrepareSetsSampleRate)
{
    meter->prepare (96000.0, 2);
    EXPECT_DOUBLE_EQ (96000.0, meter->getSampleRate());
}

TEST_F (KMeterStateTests, PrepareSetChannelCount)
{
    meter->prepare (48000.0, 4);
    EXPECT_EQ (4, meter->getNumChannels());
}

//==============================================================================
TEST_F (KMeterStateTests, InitialLevelsAreSilent)
{
    EXPECT_LT (meter->getPeakLevel(), -90.0f);
    EXPECT_LT (meter->getAverageLevel(), -90.0f);
    EXPECT_LT (meter->getPeakHoldLevel(), -90.0f);
    EXPECT_EQ (0, meter->getOverCount());
    EXPECT_FALSE (meter->isClipping());
}

TEST_F (KMeterStateTests, ResetClearsLevels)
{
    // Push some audio
    std::vector<float> samples (512, 0.5f);
    const float* channels[2] = { samples.data(), samples.data() };
    meter->pushSamples (channels, 2, 512);
    meter->processPendingAudio();

    // Should have levels
    EXPECT_GT (meter->getPeakLevel(), -90.0f);

    // Reset
    meter->reset();
    EXPECT_LT (meter->getPeakLevel(), -90.0f);
    EXPECT_LT (meter->getAverageLevel(), -90.0f);
}

//==============================================================================
TEST_F (KMeterStateTests, PushSamplesHandlesSilence)
{
    std::vector<float> silence (512, 0.0f);
    const float* channels[2] = { silence.data(), silence.data() };

    // Reset to ensure clean state
    meter->reset();

    meter->pushSamples (channels, 2, 512);
    meter->processPendingAudio();

    // Peak should be very low for silence
    EXPECT_LT (meter->getPeakLevel(), -90.0f);
    // Average with K-20 offset (+20dB): -100dBFS + 20 = -80dB
    EXPECT_LT (meter->getAverageLevel(), -70.0f);
}

TEST_F (KMeterStateTests, PushMonoSamplesWorks)
{
    std::vector<float> samples (512, 0.5f);
    meter->pushMonoSamples (samples.data(), 512);
    meter->processPendingAudio();

    EXPECT_GT (meter->getPeakLevel(), -90.0f);
}

TEST_F (KMeterStateTests, PushSamplesDetectsPeaks)
{
    std::vector<float> samples (512, 0.5f);
    const float* channels[2] = { samples.data(), samples.data() };

    meter->pushSamples (channels, 2, 512);
    meter->processPendingAudio();

    // 0.5 amplitude = -6.02 dBFS, K-20 calibrated = +13.98 dB
    float expectedPeak = Decibels::gainToDecibels (0.5f) + 20.0f;
    EXPECT_NEAR (expectedPeak, meter->getPeakLevel(), tolerance);
}

TEST_F (KMeterStateTests, PushSamplesCalculatesRMS)
{
    // Generate sine wave
    constexpr int numSamples = 4800; // 100ms
    std::vector<float> samples (numSamples);

    for (int i = 0; i < numSamples; ++i)
        samples[i] = 0.5f * std::sin (2.0f * MathConstants<float>::pi * 1000.0f * i / sampleRate);

    const float* channels[2] = { samples.data(), samples.data() };

    // Push multiple times to fill integration window
    for (int pass = 0; pass < 10; ++pass)
    {
        meter->pushSamples (channels, 2, numSamples);

        // Process all pending audio (processPendingAudio processes max 512 samples at a time)
        while (meter->getNumSamplesInFifo() > 0)
            meter->processPendingAudio();
    }

    // RMS of 0.5 amplitude sine = 0.5/sqrt(2) ≈ 0.35355 ≈ -9.03 dBFS
    // Apply RMS peak-to-average correction (+3.0103 dB), then K-20 offset (+20 dB)
    const float rmsAmplitude = 0.5f / std::sqrt (2.0f);
    float expectedAverage = Decibels::gainToDecibels (rmsAmplitude) + 3.0103f + 20.0f;
    EXPECT_NEAR (expectedAverage, meter->getAverageLevel(), 1.0f); // 1 dB tolerance
}

//==============================================================================
TEST_F (KMeterStateTests, GetPeakLevelReturnsMaxAcrossChannels)
{
    // Channel 0: 0.3, Channel 1: 0.5
    std::vector<float> ch0 (512, 0.3f);
    std::vector<float> ch1 (512, 0.5f);
    const float* channels[2] = { ch0.data(), ch1.data() };

    meter->pushSamples (channels, 2, 512);
    meter->processPendingAudio();

    // Should return the max (channel 1 = 0.5 = -6.02 dBFS, K-20 calibrated)
    float expectedPeak = Decibels::gainToDecibels (0.5f) + 20.0f;
    EXPECT_NEAR (expectedPeak, meter->getPeakLevel(), tolerance);
}

TEST_F (KMeterStateTests, GetPeakLevelPerChannel)
{
    std::vector<float> ch0 (512, 0.3f);
    std::vector<float> ch1 (512, 0.5f);
    const float* channels[2] = { ch0.data(), ch1.data() };

    meter->pushSamples (channels, 2, 512);
    meter->processPendingAudio();

    float peak0 = meter->getPeakLevel (0);
    float peak1 = meter->getPeakLevel (1);

    EXPECT_NEAR (Decibels::gainToDecibels (0.3f) + 20.0f, peak0, tolerance);
    EXPECT_NEAR (Decibels::gainToDecibels (0.5f) + 20.0f, peak1, tolerance);
    EXPECT_GT (peak1, peak0);
}

//==============================================================================
TEST_F (KMeterStateTests, ScaleOffsetForK20)
{
    EXPECT_FLOAT_EQ (20.0f, KMeterState::scaleOffsetForScale (KMeterState::Scale::k20));
}

TEST_F (KMeterStateTests, ScaleOffsetForK14)
{
    EXPECT_FLOAT_EQ (14.0f, KMeterState::scaleOffsetForScale (KMeterState::Scale::k14));
}

TEST_F (KMeterStateTests, ScaleOffsetForK12)
{
    EXPECT_FLOAT_EQ (12.0f, KMeterState::scaleOffsetForScale (KMeterState::Scale::k12));
}

TEST_F (KMeterStateTests, RangeMinForK20)
{
    EXPECT_FLOAT_EQ (-70.0f, KMeterState::rangeMinForScale (KMeterState::Scale::k20));
}

TEST_F (KMeterStateTests, RangeMaxForK20)
{
    EXPECT_FLOAT_EQ (20.0f, KMeterState::rangeMaxForScale (KMeterState::Scale::k20));
}

TEST_F (KMeterStateTests, RangeMinForK14)
{
    EXPECT_FLOAT_EQ (-64.0f, KMeterState::rangeMinForScale (KMeterState::Scale::k14));
}

TEST_F (KMeterStateTests, RangeMaxForK14)
{
    EXPECT_FLOAT_EQ (26.0f, KMeterState::rangeMaxForScale (KMeterState::Scale::k14));
}

//==============================================================================
TEST_F (KMeterStateTests, SetScaleChangesCalibration)
{
    std::vector<float> samples (512, 0.5f);
    const float* channels[2] = { samples.data(), samples.data() };

    // K-20 scale
    meter->setScale (KMeterState::Scale::k20);
    meter->pushSamples (channels, 2, 512);
    meter->processPendingAudio();
    float avgK20 = meter->getAverageLevel();

    meter->reset();

    // K-14 scale
    meter->setScale (KMeterState::Scale::k14);
    meter->pushSamples (channels, 2, 512);
    meter->processPendingAudio();
    float avgK14 = meter->getAverageLevel();

    // K-20 should read 6 dB higher than K-14 (more headroom = higher meter reading for same signal)
    EXPECT_NEAR (6.0f, avgK20 - avgK14, 1.0f);
}

TEST_F (KMeterStateTests, SetMeteringStandardChangesMode)
{
    meter->setMeteringStandard (KMeterState::MeteringStandard::rmsFlat);
    EXPECT_EQ (KMeterState::MeteringStandard::rmsFlat, meter->getMeteringStandard());

    meter->setMeteringStandard (KMeterState::MeteringStandard::ituBS1770_4);
    EXPECT_EQ (KMeterState::MeteringStandard::ituBS1770_4, meter->getMeteringStandard());
}

//==============================================================================
TEST_F (KMeterStateTests, SetIntegrationTimeAffectsSmoothing)
{
    // Short integration
    meter->setIntegrationTime (0.1);

    std::vector<float> samples (4800, 0.5f); // 100ms
    const float* channels[2] = { samples.data(), samples.data() };

    meter->pushSamples (channels, 2, 4800);
    meter->processPendingAudio();

    float avgShort = meter->getAverageLevel();

    // Reset and use long integration
    meter->reset();
    meter->setIntegrationTime (1.0);

    meter->pushSamples (channels, 2, 4800);
    meter->processPendingAudio();

    float avgLong = meter->getAverageLevel();

    // Short integration should respond faster (likely higher for step input)
    EXPECT_NE (avgShort, avgLong);
}

TEST_F (KMeterStateTests, SetPeakFallTimeAffectsDecay)
{
    std::vector<float> loud (512, 1.0f);
    std::vector<float> quiet (512, 0.0f);
    const float* loudChannels[2] = { loud.data(), loud.data() };
    const float* quietChannels[2] = { quiet.data(), quiet.data() };

    // Fast fall
    meter->setPeakFallTime (0.1);
    meter->pushSamples (loudChannels, 2, 512);
    meter->processPendingAudio();
    float peakBefore = meter->getPeakLevel();

    // Push silence
    meter->pushSamples (quietChannels, 2, 512);
    meter->processPendingAudio();
    float peakAfterFast = meter->getPeakLevel();

    // Reset and use slow fall
    meter->reset();
    meter->setPeakFallTime (5.0);
    meter->pushSamples (loudChannels, 2, 512);
    meter->processPendingAudio();

    meter->pushSamples (quietChannels, 2, 512);
    meter->processPendingAudio();
    float peakAfterSlow = meter->getPeakLevel();

    // Slow fall should decay less
    EXPECT_GT (peakAfterSlow, peakAfterFast);
}

TEST_F (KMeterStateTests, SetAverageFallTimeAffectsDecay)
{
    meter->setIntegrationTime (0.01);

    std::vector<float> loud (4800, 0.5f);
    std::vector<float> quiet (4800, 0.0f);
    const float* loudChannels[2] = { loud.data(), loud.data() };
    const float* quietChannels[2] = { quiet.data(), quiet.data() };

    // Fast fall
    meter->setAverageFallTime (0.1);
    meter->pushSamples (loudChannels, 2, 4800);
    while (meter->getNumSamplesInFifo() > 0)
        meter->processPendingAudio();

    meter->pushSamples (quietChannels, 2, 4800);
    while (meter->getNumSamplesInFifo() > 0)
        meter->processPendingAudio();

    const float avgAfterFast = meter->getAverageLevel();

    // Reset and use slow fall
    meter->reset();
    meter->setIntegrationTime (0.01);
    meter->setAverageFallTime (2.0);

    meter->pushSamples (loudChannels, 2, 4800);
    while (meter->getNumSamplesInFifo() > 0)
        meter->processPendingAudio();

    meter->pushSamples (quietChannels, 2, 4800);
    while (meter->getNumSamplesInFifo() > 0)
        meter->processPendingAudio();

    const float avgAfterSlow = meter->getAverageLevel();

    // Fast fall should decay more
    EXPECT_LT (avgAfterFast, avgAfterSlow);
}

TEST_F (KMeterStateTests, SetPeakHoldTimeControlsHold)
{
    // Set hold time
    meter->setPeakHoldTime (1.0); // 1 second

    std::vector<float> samples (512, 0.5f);
    const float* channels[2] = { samples.data(), samples.data() };

    meter->pushSamples (channels, 2, 512);
    meter->processPendingAudio();

    float holdBefore = meter->getPeakHoldLevel();
    EXPECT_GT (holdBefore, -90.0f);

    // Push silence
    std::vector<float> silence (512, 0.0f);
    const float* silentChannels[2] = { silence.data(), silence.data() };
    meter->pushSamples (silentChannels, 2, 512);
    meter->processPendingAudio();

    // Hold should still be there (not enough time passed)
    float holdAfter = meter->getPeakHoldLevel();
    EXPECT_NEAR (holdBefore, holdAfter, tolerance);
}

TEST_F (KMeterStateTests, SetOverThresholdControlsClipping)
{
    meter->setOverThreshold (0.8f); // Lower threshold

    std::vector<float> samples (512, 0.9f); // Above threshold
    const float* channels[2] = { samples.data(), samples.data() };

    meter->pushSamples (channels, 2, 512);
    meter->processPendingAudio();

    EXPECT_TRUE (meter->isClipping());
    EXPECT_GT (meter->getOverCount(), 0);
}

TEST_F (KMeterStateTests, SetOverCounterModeUpdatesState)
{
    meter->setOverCounterMode (KMeterState::OverCounterMode::total);
    EXPECT_EQ (KMeterState::OverCounterMode::total, meter->getOverCounterMode());

    meter->setOverCounterMode (KMeterState::OverCounterMode::contiguous);
    EXPECT_EQ (KMeterState::OverCounterMode::contiguous, meter->getOverCounterMode());
}

//==============================================================================
TEST_F (KMeterStateTests, OverCountTracksContiguousSamples)
{
    meter->setOverThreshold (0.5f);
    meter->setOverCounterMode (KMeterState::OverCounterMode::contiguous);

    // 100 samples above threshold
    std::vector<float> loud (100, 0.6f);
    const float* loudChannels[2] = { loud.data(), loud.data() };

    meter->pushSamples (loudChannels, 2, 100);
    meter->processPendingAudio();

    int overCount = meter->getOverCount();
    EXPECT_EQ (100, overCount);
}

TEST_F (KMeterStateTests, OverCountAccumulatesAcrossBlocks)
{
    meter->setOverThreshold (0.5f);
    meter->setOverCounterMode (KMeterState::OverCounterMode::total);

    // Push loud samples
    std::vector<float> loud (100, 0.6f);
    const float* loudChannels[2] = { loud.data(), loud.data() };
    meter->pushSamples (loudChannels, 2, 100);
    while (meter->getNumSamplesInFifo() > 0)
        meter->processPendingAudio();

    const int overCountBefore = meter->getOverCount();
    EXPECT_EQ (200, overCountBefore);

    // Push quiet samples
    std::vector<float> quiet (100, 0.1f);
    const float* quietChannels[2] = { quiet.data(), quiet.data() };
    meter->pushSamples (quietChannels, 2, 100);
    while (meter->getNumSamplesInFifo() > 0)
        meter->processPendingAudio();

    // Should not decrease without reset
    EXPECT_EQ (overCountBefore, meter->getOverCount());
}

TEST_F (KMeterStateTests, IsClippingDetectsThreshold)
{
    meter->setOverThreshold (0.999f);

    // Just below threshold
    std::vector<float> samples1 (512, 0.99f);
    const float* channels1[2] = { samples1.data(), samples1.data() };
    meter->pushSamples (channels1, 2, 512);
    meter->processPendingAudio();

    EXPECT_FALSE (meter->isClipping());

    // At threshold
    std::vector<float> samples2 (512, 1.0f);
    const float* channels2[2] = { samples2.data(), samples2.data() };
    meter->pushSamples (channels2, 2, 512);
    meter->processPendingAudio();

    EXPECT_TRUE (meter->isClipping());
}

//==============================================================================
TEST_F (KMeterStateTests, ThreadSafeConcurrentPushAndRead)
{
    std::atomic<bool> running { true };
    std::atomic<int> pushCount { 0 };
    std::atomic<int> readCount { 0 };

    // Producer thread (audio thread)
    std::thread audioThread ([&]()
    {
        std::vector<float> samples (512, 0.5f);
        const float* channels[2] = { samples.data(), samples.data() };

        while (running)
        {
            meter->pushSamples (channels, 2, 512);
            meter->processPendingAudio();
            ++pushCount;
            std::this_thread::sleep_for (std::chrono::microseconds (100));
        }
    });

    // Consumer thread (UI thread)
    std::thread uiThread ([&]()
    {
        while (running)
        {
            volatile float peak = meter->getPeakLevel();
            volatile float avg = meter->getAverageLevel();
            volatile int overCount = meter->getOverCount();
            ignoreUnused (peak, avg, overCount);
            ++readCount;
            std::this_thread::sleep_for (std::chrono::microseconds (200));
        }
    });

    // Run for 100ms
    std::this_thread::sleep_for (std::chrono::milliseconds (100));
    running = false;

    audioThread.join();
    uiThread.join();

    // Should have completed some operations without crashing
    EXPECT_GT (pushCount.load(), 0);
    EXPECT_GT (readCount.load(), 0);
}

//==============================================================================
TEST_F (KMeterStateTests, EBU_R128_LoudnessInitializedToSilence)
{
    EXPECT_LT (meter->getIntegratedLoudness(), -60.0f);
    EXPECT_LT (meter->getShortTermLoudness(), -60.0f);
    EXPECT_LT (meter->getMomentaryLoudness(), -60.0f);
    EXPECT_FLOAT_EQ (0.0f, meter->getLoudnessRange());
}

//==============================================================================
TEST_F (KMeterStateTests, HandlesLargeBuffers)
{
    std::vector<float> samples (48000, 0.5f); // 1 second
    const float* channels[2] = { samples.data(), samples.data() };

    meter->pushSamples (channels, 2, 48000);
    meter->processPendingAudio();

    // Should handle without crashing
    EXPECT_GT (meter->getPeakLevel(), -90.0f);
}

TEST_F (KMeterStateTests, HandlesEmptyBuffers)
{
    const float* channels[2] = { nullptr, nullptr };
    meter->pushSamples (channels, 2, 0);
    meter->processPendingAudio();

    // Should not crash
    SUCCEED();
}

TEST_F (KMeterStateTests, HandlesMultipleChannels)
{
    KMeterState multiMeter (sampleRate, 8);

    std::vector<float> samples (512, 0.5f);
    const float* channels[8] = {
        samples.data(), samples.data(), samples.data(), samples.data(), samples.data(), samples.data(), samples.data(), samples.data()
    };

    multiMeter.pushSamples (channels, 8, 512);
    multiMeter.processPendingAudio();

    EXPECT_GT (multiMeter.getPeakLevel(), -90.0f);
}

//==============================================================================
// Phase 2: Peak Hold Auto-Release Tests
//==============================================================================
TEST_F (KMeterStateTests, PeakHoldAutoReleaseAfterTime)
{
    meter->setPeakHoldTime (0.2); // 200ms auto-release

    // Push loud sample
    std::vector<float> loud (512, 0.8f);
    const float* loudChannels[2] = { loud.data(), loud.data() };
    meter->pushSamples (loudChannels, 2, 512);

    while (meter->getNumSamplesInFifo() > 0)
        meter->processPendingAudio();

    float peakBefore = meter->getPeakHoldLevel();
    EXPECT_GT (peakBefore, -10.0f);

    // Push silence for 300ms (more than hold time)
    std::vector<float> silence (14400, 0.0f); // 300ms at 48kHz
    const float* silentChannels[2] = { silence.data(), silence.data() };
    meter->pushSamples (silentChannels, 2, 14400);

    while (meter->getNumSamplesInFifo() > 0)
        meter->processPendingAudio();

    // Peak hold should have released and started falling
    float peakAfter = meter->getPeakHoldLevel();
    EXPECT_LT (peakAfter, peakBefore);
    EXPECT_LT (peakAfter, peakBefore - 0.3f);
}

TEST_F (KMeterStateTests, PeakHoldInfiniteMode)
{
    meter->setPeakHoldTime (-1.0); // Infinite hold

    // Push loud sample
    std::vector<float> loud (512, 0.8f);
    const float* loudChannels[2] = { loud.data(), loud.data() };
    meter->pushSamples (loudChannels, 2, 512);

    while (meter->getNumSamplesInFifo() > 0)
        meter->processPendingAudio();

    float peakBefore = meter->getPeakHoldLevel();
    EXPECT_GT (peakBefore, -10.0f);

    // Push silence for a long time
    std::vector<float> silence (48000, 0.0f); // 1 second at 48kHz
    const float* silentChannels[2] = { silence.data(), silence.data() };
    meter->pushSamples (silentChannels, 2, 48000);

    while (meter->getNumSamplesInFifo() > 0)
        meter->processPendingAudio();

    // Peak hold should NOT have released (infinite mode)
    float peakAfter = meter->getPeakHoldLevel();
    EXPECT_NEAR (peakBefore, peakAfter, 0.1f);
}

TEST_F (KMeterStateTests, PeakHoldUpdatesOnNewPeak)
{
    meter->setPeakHoldTime (10.0); // Long hold time

    // Push medium level
    std::vector<float> medium (512, 0.5f);
    const float* mediumChannels[2] = { medium.data(), medium.data() };
    meter->pushSamples (mediumChannels, 2, 512);

    while (meter->getNumSamplesInFifo() > 0)
        meter->processPendingAudio();

    float holdBefore = meter->getPeakHoldLevel();

    // Push louder sample
    std::vector<float> loud (512, 0.9f);
    const float* loudChannels[2] = { loud.data(), loud.data() };
    meter->pushSamples (loudChannels, 2, 512);

    while (meter->getNumSamplesInFifo() > 0)
        meter->processPendingAudio();

    // Peak hold should update to new higher peak
    float holdAfter = meter->getPeakHoldLevel();
    EXPECT_GT (holdAfter, holdBefore);
    EXPECT_NEAR (Decibels::gainToDecibels (0.9f) + 20.0f, holdAfter, tolerance);
}

//==============================================================================
// Phase 2: OVER Counter Tests
//==============================================================================
TEST_F (KMeterStateTests, OverCounterWithMixedSamples)
{
    meter->setOverThreshold (0.5f);
    meter->setOverCounterMode (KMeterState::OverCounterMode::total);

    // Create pattern: [over, over, under, over, over, over]
    std::vector<float> mixed = { 0.6f, 0.7f, 0.3f, 0.8f, 0.9f, 0.6f };
    const float* mixedChannels[2] = { mixed.data(), mixed.data() };

    meter->pushSamples (mixedChannels, 2, 6);
    while (meter->getNumSamplesInFifo() > 0)
        meter->processPendingAudio();

    // Counter should show 10 total overflows (2 channels)
    int overCount = meter->getOverCount();
    EXPECT_EQ (10, overCount);
}

TEST_F (KMeterStateTests, OverCounterResetsImmediately)
{
    meter->setOverThreshold (0.5f);
    meter->setOverCounterMode (KMeterState::OverCounterMode::contiguous);

    // Pattern: [over, over, over, under]
    std::vector<float> samples = { 0.8f, 0.9f, 0.7f, 0.3f };
    const float* channels[2] = { samples.data(), samples.data() };

    meter->pushSamples (channels, 2, 4);
    while (meter->getNumSamplesInFifo() > 0)
        meter->processPendingAudio();

    // Counter should be 0 (reset by last sample)
    int overCount = meter->getOverCount();
    EXPECT_EQ (0, overCount);
}

TEST_F (KMeterStateTests, OverCounterTracksMaxContiguous)
{
    meter->setOverThreshold (0.5f);
    meter->setOverCounterMode (KMeterState::OverCounterMode::contiguous);

    // Long sequence of samples over threshold
    std::vector<float> loud (250, 0.8f);
    const float* loudChannels[2] = { loud.data(), loud.data() };

    meter->pushSamples (loudChannels, 2, 250);
    meter->processPendingAudio();

    // Counter should show 250 overflows
    int overCount = meter->getOverCount();
    EXPECT_EQ (250, overCount);
}

TEST_F (KMeterStateTests, OverCounterIgnoresSamplesJustBelowThreshold)
{
    meter->setOverThreshold (0.999f);
    meter->setOverCounterMode (KMeterState::OverCounterMode::contiguous);

    // Samples just below threshold
    std::vector<float> samples (100, 0.998f);
    const float* channels[2] = { samples.data(), samples.data() };

    meter->pushSamples (channels, 2, 100);
    meter->processPendingAudio();

    // Counter should be 0 (all samples below threshold)
    int overCount = meter->getOverCount();
    EXPECT_EQ (0, overCount);
    EXPECT_FALSE (meter->isClipping());
}

TEST_F (KMeterStateTests, OverCounterDetectsNegativePeaks)
{
    meter->setOverThreshold (0.5f);
    meter->setOverCounterMode (KMeterState::OverCounterMode::contiguous);

    // Negative peaks should also count (using absolute value)
    std::vector<float> negative = { -0.6f, -0.7f, -0.8f };
    const float* negativeChannels[2] = { negative.data(), negative.data() };

    meter->pushSamples (negativeChannels, 2, 3);
    meter->processPendingAudio();

    // Counter should count negative peaks
    int overCount = meter->getOverCount();
    EXPECT_EQ (3, overCount);
    EXPECT_TRUE (meter->isClipping());
}
