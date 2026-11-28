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

#include <chrono>
#include <thread>

using namespace yup;

//==============================================================================
class SpectrumAnalyzerStateTests : public ::testing::Test
{
protected:
    static constexpr float tolerance = 1e-6f;

    void SetUp() override
    {
        analyzer = std::make_unique<SpectrumAnalyzerState>();
    }

    // Pushes a ramp going up by one at every sample, starting from zero
    void pushRamp (int numSamples)
    {
        std::vector<float> ramp (static_cast<size_t> (numSamples));
        std::iota (ramp.begin(), ramp.end(), 0.0f);

        analyzer->pushSamples (ramp.data(), numSamples);
    }

    std::unique_ptr<SpectrumAnalyzerState> analyzer;
    std::vector<float> testBuffer;
};

//==============================================================================
TEST_F (SpectrumAnalyzerStateTests, DefaultConstructorInitializes)
{
    EXPECT_EQ (2048, analyzer->getFftSize());
    EXPECT_FALSE (analyzer->isFFTDataReady());
    EXPECT_EQ (0, analyzer->getNumAvailableSamples());
    EXPECT_GT (analyzer->getFreeSpace(), 0);
}

TEST_F (SpectrumAnalyzerStateTests, CustomSizeConstructorInitializes)
{
    SpectrumAnalyzerState customAnalyzer (1024);

    EXPECT_EQ (1024, customAnalyzer.getFftSize());
    EXPECT_FALSE (customAnalyzer.isFFTDataReady());
    EXPECT_EQ (0, customAnalyzer.getNumAvailableSamples());
    EXPECT_GT (customAnalyzer.getFreeSpace(), 0);
}

TEST_F (SpectrumAnalyzerStateTests, SetFftSizeUpdatesSize)
{
    analyzer->setFftSize (512);
    EXPECT_EQ (512, analyzer->getFftSize());

    analyzer->setFftSize (4096);
    EXPECT_EQ (4096, analyzer->getFftSize());
}

TEST_F (SpectrumAnalyzerStateTests, PushSingleSampleIncrementsCount)
{
    EXPECT_EQ (0, analyzer->getNumAvailableSamples());

    analyzer->pushSample (0.5f);
    EXPECT_EQ (1, analyzer->getNumAvailableSamples());

    analyzer->pushSample (-0.3f);
    EXPECT_EQ (2, analyzer->getNumAvailableSamples());
}

TEST_F (SpectrumAnalyzerStateTests, PushMultipleSamplesIncrementsCount)
{
    std::vector<float> samples = { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f };

    analyzer->pushSamples (samples.data(), static_cast<int> (samples.size()));
    EXPECT_EQ (5, analyzer->getNumAvailableSamples());
}

TEST_F (SpectrumAnalyzerStateTests, FFTDataReadyAfterEnoughSamples)
{
    const int fftSize = analyzer->getFftSize();
    EXPECT_FALSE (analyzer->isFFTDataReady());

    // Push more than fftSize samples to ensure buffer has enough for processing
    const int samplesToAdd = fftSize + 100;
    for (int i = 0; i < samplesToAdd; ++i)
        analyzer->pushSample (static_cast<float> (i) / fftSize);

    // Check if we have enough samples
    EXPECT_GE (analyzer->getNumAvailableSamples(), fftSize);
    EXPECT_TRUE (analyzer->isFFTDataReady());
}

TEST_F (SpectrumAnalyzerStateTests, GetFFTDataReturnsCorrectData)
{
    const int fftSize = analyzer->getFftSize();
    testBuffer.resize (fftSize);

    // Push known test pattern - need extra samples for buffer to be ready
    const int samplesToAdd = fftSize + 100;
    for (int i = 0; i < samplesToAdd; ++i)
        analyzer->pushSample (static_cast<float> (i) / fftSize);

    // Ensure we have enough samples and data is ready
    EXPECT_GE (analyzer->getNumAvailableSamples(), fftSize);
    EXPECT_TRUE (analyzer->isFFTDataReady());

    // Get FFT data
    bool success = analyzer->getFFTData (testBuffer.data());
    EXPECT_TRUE (success);

    // Verify that we got some meaningful data (the exact values depend on internal buffering)
    // Just check that the buffer is not all zeros
    bool hasNonZeroData = false;
    for (int i = 0; i < fftSize; ++i)
    {
        if (std::abs (testBuffer[i]) > tolerance)
        {
            hasNonZeroData = true;
            break;
        }
    }
    EXPECT_TRUE (hasNonZeroData);
}

TEST_F (SpectrumAnalyzerStateTests, GetFFTDataAdvancesReadPosition)
{
    const int fftSize = analyzer->getFftSize();
    testBuffer.resize (fftSize);

    // Fill buffer beyond FFT size
    for (int i = 0; i < fftSize + 100; ++i)
        analyzer->pushSample (static_cast<float> (i));

    int samplesBeforeRead = analyzer->getNumAvailableSamples();
    EXPECT_TRUE (analyzer->getFFTData (testBuffer.data()));

    // Should advance by hop size (with default 75% overlap, hop = 25% of FFT size)
    int expectedRemaining = samplesBeforeRead - analyzer->getHopSize();
    EXPECT_EQ (expectedRemaining, analyzer->getNumAvailableSamples());
}

TEST_F (SpectrumAnalyzerStateTests, ResetClearsBuffer)
{
    const int fftSize = analyzer->getFftSize();

    // Fill with enough samples to make data ready
    const int samplesToAdd = fftSize + 100;
    for (int i = 0; i < samplesToAdd; ++i)
        analyzer->pushSample (0.5f);

    // Verify we have samples and data is ready
    EXPECT_GE (analyzer->getNumAvailableSamples(), fftSize);
    EXPECT_TRUE (analyzer->isFFTDataReady());

    // Reset should clear everything
    analyzer->reset();

    // After reset, should have no samples and no data ready
    EXPECT_FALSE (analyzer->isFFTDataReady());
    EXPECT_EQ (0, analyzer->getNumAvailableSamples());
}

TEST_F (SpectrumAnalyzerStateTests, OverlapFactorAffectsHopSize)
{
    const int fftSize = analyzer->getFftSize();

    // Test 50% overlap
    analyzer->setOverlapFactor (0.5f);
    EXPECT_EQ (0.5f, analyzer->getOverlapFactor());
    EXPECT_EQ (fftSize / 2, analyzer->getHopSize());

    // Test 75% overlap (default)
    analyzer->setOverlapFactor (0.75f);
    EXPECT_EQ (0.75f, analyzer->getOverlapFactor());
    EXPECT_EQ (fftSize / 4, analyzer->getHopSize());

    // Test no overlap
    analyzer->setOverlapFactor (0.0f);
    EXPECT_EQ (0.0f, analyzer->getOverlapFactor());
    EXPECT_EQ (fftSize, analyzer->getHopSize());
}

TEST_F (SpectrumAnalyzerStateTests, HandleNullPointerInPushSamples)
{
    // Should not crash with null pointer - but may assert in debug builds
    // In debug builds, this will trigger an assertion, so we skip this test
    // In release builds, it should handle gracefully
#if YUP_ASSERTIONS_ENABLED_OR_LOGGED
    // In debug builds, we expect this to assert, so we skip the test
    GTEST_SKIP() << "Skipping null pointer test in debug build (triggers assertion)";
#else
    analyzer->pushSamples (nullptr, 10);
    EXPECT_EQ (0, analyzer->getNumAvailableSamples());
#endif
}

TEST_F (SpectrumAnalyzerStateTests, HandleZeroSamplesInPushSamples)
{
    std::vector<float> samples = { 0.1f, 0.2f, 0.3f };

    // Should not crash with zero samples
    analyzer->pushSamples (samples.data(), 0);
    EXPECT_EQ (0, analyzer->getNumAvailableSamples());
}

TEST_F (SpectrumAnalyzerStateTests, ThreadSafetyBasic)
{
    const int fftSize = analyzer->getFftSize();
    testBuffer.resize (fftSize);

    // Simulate basic audio thread / UI thread interaction
    // Audio thread pushes samples - need enough samples to be ready
    const int samplesToAdd = fftSize + 100;
    for (int i = 0; i < samplesToAdd; ++i)
        analyzer->pushSample (std::sin (2.0f * 3.14159f * i / fftSize));

    // UI thread checks and retrieves data
    EXPECT_TRUE (analyzer->isFFTDataReady());
    EXPECT_TRUE (analyzer->getFFTData (testBuffer.data()));

    // Verify we got some meaningful data
    bool hasNonZeroData = false;
    for (int i = 0; i < fftSize; ++i)
    {
        if (std::abs (testBuffer[i]) > tolerance)
        {
            hasNonZeroData = true;
            break;
        }
    }
    EXPECT_TRUE (hasNonZeroData);
}

TEST_F (SpectrumAnalyzerStateTests, LargeBufferHandling)
{
    const int fftSize = analyzer->getFftSize();
    const int largeBufferSize = fftSize * 3; // Larger than internal FIFO
    std::vector<float> largeSamples (largeBufferSize);

    // Fill with ramp
    for (int i = 0; i < largeBufferSize; ++i)
        largeSamples[i] = static_cast<float> (i) / largeBufferSize;

    // Push the large buffer
    analyzer->pushSamples (largeSamples.data(), largeBufferSize);

    // Check that we have samples (might not be ready immediately with large buffers)
    EXPECT_GT (analyzer->getNumAvailableSamples(), 0);

    // If not ready, push a few more samples to trigger readiness
    if (! analyzer->isFFTDataReady())
    {
        for (int i = 0; i < 100; ++i)
            analyzer->pushSample (0.5f);
    }

    // Should now be able to get FFT data
    testBuffer.resize (fftSize);
    if (analyzer->isFFTDataReady())
    {
        EXPECT_TRUE (analyzer->getFFTData (testBuffer.data()));
    }
    else
    {
        // If still not ready, just verify that samples were stored
        EXPECT_GT (analyzer->getNumAvailableSamples(), largeBufferSize / 2);
    }
}

TEST_F (SpectrumAnalyzerStateTests, MultipleFFTRetrievals)
{
    const int fftSize = analyzer->getFftSize();
    const int totalSamples = fftSize * 3;
    testBuffer.resize (fftSize);

    // Push enough samples for multiple FFT frames
    for (int i = 0; i < totalSamples; ++i)
        analyzer->pushSample (static_cast<float> (i));

    // Should be able to get multiple FFT frames
    EXPECT_TRUE (analyzer->isFFTDataReady());
    EXPECT_TRUE (analyzer->getFFTData (testBuffer.data()));

    // Due to overlap, should still have data ready
    if (analyzer->getOverlapFactor() > 0.0f)
    {
        EXPECT_TRUE (analyzer->isFFTDataReady());
        EXPECT_TRUE (analyzer->getFFTData (testBuffer.data()));
    }
}

//==============================================================================
TEST_F (SpectrumAnalyzerStateTests, SetFftSizeKeepsTheBufferedSamples)
{
    pushRamp (1024);
    EXPECT_FALSE (analyzer->isFFTDataReady());

    // Nothing is reallocated: the samples already pushed are read in frames of the new size
    analyzer->setFftSize (512);

    EXPECT_EQ (1024, analyzer->getNumAvailableSamples());
    ASSERT_TRUE (analyzer->isFFTDataReady());

    testBuffer.resize (512);
    ASSERT_TRUE (analyzer->getFFTData (Span<float> (testBuffer)));

    EXPECT_FLOAT_EQ (0.0f, testBuffer.front());
    EXPECT_FLOAT_EQ (511.0f, testBuffer.back());
    EXPECT_EQ (1024 - analyzer->getHopSize(), analyzer->getNumAvailableSamples());
}

TEST_F (SpectrumAnalyzerStateTests, TakesTheSmallestAndTheLargestFftSize)
{
    analyzer->setFftSize (SpectrumAnalyzerState::maxFftSize);
    EXPECT_EQ (65536, analyzer->getFftSize());

    pushRamp (65536);
    testBuffer.resize (65536);

    ASSERT_TRUE (analyzer->getFFTData (Span<float> (testBuffer)));
    EXPECT_FLOAT_EQ (0.0f, testBuffer.front());
    EXPECT_FLOAT_EQ (65535.0f, testBuffer.back());

    analyzer->setFftSize (SpectrumAnalyzerState::minFftSize);
    EXPECT_EQ (64, analyzer->getFftSize());
    EXPECT_TRUE (analyzer->isFFTDataReady());
}

TEST_F (SpectrumAnalyzerStateTests, AnInvalidFftSizeIsIgnored)
{
#if YUP_ASSERTIONS_ENABLED_OR_LOGGED
    // In debug builds, we expect this to assert, so we skip the test
    GTEST_SKIP() << "Skipping invalid FFT size test in debug build (triggers assertion)";
#else
    analyzer->setFftSize (1000);
    analyzer->setFftSize (0);
    analyzer->setFftSize (-1024);
    analyzer->setFftSize (SpectrumAnalyzerState::maxFftSize * 2);
    EXPECT_EQ (2048, analyzer->getFftSize());

    SpectrumAnalyzerState invalidAnalyzer (1000);
    EXPECT_EQ (2048, invalidAnalyzer.getFftSize());
#endif
}

TEST_F (SpectrumAnalyzerStateTests, FifoKeepsFourFramesOfTheCurrentFftSize)
{
    analyzer->setFftSize (64);
    pushRamp (1000);

    // The samples that do not fit are dropped, the ones already buffered are kept
    EXPECT_EQ (64 * 4 - 1, analyzer->getNumAvailableSamples());
    EXPECT_EQ (0, analyzer->getFreeSpace());

    testBuffer.resize (64);
    ASSERT_TRUE (analyzer->getFFTData (Span<float> (testBuffer)));
    EXPECT_FLOAT_EQ (0.0f, testBuffer.front());

    // A larger size makes room for more, next to what is buffered
    analyzer->setFftSize (4096);

    const int numBuffered = analyzer->getNumAvailableSamples();
    pushRamp (1000);

    EXPECT_EQ (numBuffered + 1000, analyzer->getNumAvailableSamples());
}

TEST_F (SpectrumAnalyzerStateTests, TheMaximumFftSizeIsTheLargestOneUnlessGiven)
{
    EXPECT_EQ (SpectrumAnalyzerState::maxFftSize, analyzer->getMaxFftSize());
    EXPECT_EQ (SpectrumAnalyzerState::maxFftSize, SpectrumAnalyzerState (1024).getMaxFftSize());

    SpectrumAnalyzerState small (512, 2048);

    EXPECT_EQ (512, small.getFftSize());
    EXPECT_EQ (2048, small.getMaxFftSize());

    // Every size up to the maximum is taken, the maximum included
    small.setFftSize (2048);
    EXPECT_EQ (2048, small.getFftSize());

    small.setFftSize (SpectrumAnalyzerState::minFftSize);
    EXPECT_EQ (64, small.getFftSize());
}

TEST_F (SpectrumAnalyzerStateTests, FifoOfAStateMadeForAMaximumKeepsFourFramesOfTheCurrentFftSize)
{
    analyzer = std::make_unique<SpectrumAnalyzerState> (64, 1024);
    pushRamp (1000);

    EXPECT_EQ (64 * 4 - 1, analyzer->getNumAvailableSamples());
    EXPECT_EQ (0, analyzer->getFreeSpace());

    // At the maximum the four frames are all the FIFO was allocated for
    analyzer->reset();
    analyzer->setFftSize (1024);
    pushRamp (5000);

    EXPECT_EQ (1024 * 4 - 1, analyzer->getNumAvailableSamples());
    EXPECT_EQ (0, analyzer->getFreeSpace());

    // A frame that goes past the end of the FIFO is read whole
    analyzer->reset();
    pushRamp (3500);
    analyzer->reset();
    pushRamp (2048);

    testBuffer.resize (1024);
    analyzer->setOverlapFactor (0.0f);

    for (int frame = 0; frame < 2; ++frame)
    {
        ASSERT_TRUE (analyzer->getFFTData (Span<float> (testBuffer)));
        EXPECT_FLOAT_EQ (static_cast<float> (frame * 1024), testBuffer.front());
        EXPECT_FLOAT_EQ (static_cast<float> (frame * 1024 + 1023), testBuffer.back());
    }
}

TEST_F (SpectrumAnalyzerStateTests, AnFftSizeAboveTheMaximumIsIgnored)
{
#if YUP_ASSERTIONS_ENABLED_OR_LOGGED
    // In debug builds, we expect this to assert, so we skip the test
    GTEST_SKIP() << "Skipping FFT size above the maximum test in debug build (triggers assertion)";
#else
    SpectrumAnalyzerState small (512, 1024);

    small.setFftSize (2048);
    small.setFftSize (SpectrumAnalyzerState::maxFftSize);
    EXPECT_EQ (512, small.getFftSize());

    // The default size is the maximum when 2048 is above it
    SpectrumAnalyzerState tooLarge (4096, 1024);
    EXPECT_EQ (1024, tooLarge.getFftSize());
    EXPECT_EQ (1024, tooLarge.getMaxFftSize());
#endif
}

TEST_F (SpectrumAnalyzerStateTests, AnInvalidMaximumFftSizeFallsBackToTheLargestOne)
{
#if YUP_ASSERTIONS_ENABLED_OR_LOGGED
    // In debug builds, we expect this to assert, so we skip the test
    GTEST_SKIP() << "Skipping invalid maximum FFT size test in debug build (triggers assertion)";
#else
    for (const int invalidMaximum : { 3000, 0, -1024, SpectrumAnalyzerState::minFftSize / 2, SpectrumAnalyzerState::maxFftSize * 2 })
    {
        SpectrumAnalyzerState invalidAnalyzer (1024, invalidMaximum);

        EXPECT_EQ (SpectrumAnalyzerState::maxFftSize, invalidAnalyzer.getMaxFftSize());
        EXPECT_EQ (1024, invalidAnalyzer.getFftSize());
    }
#endif
}

TEST_F (SpectrumAnalyzerStateTests, GetFFTDataIntoASpanNeedsABufferOfTheFftSize)
{
    pushRamp (2048);

    std::vector<float> tooSmall (1024);
    std::vector<float> tooLarge (4096);

    EXPECT_FALSE (analyzer->getFFTData (Span<float> (tooSmall)));
    EXPECT_FALSE (analyzer->getFFTData (Span<float> (tooLarge)));
    EXPECT_EQ (2048, analyzer->getNumAvailableSamples());

    testBuffer.resize (2048);
    EXPECT_TRUE (analyzer->getFFTData (Span<float> (testBuffer)));
    EXPECT_FLOAT_EQ (2047.0f, testBuffer.back());
}

TEST_F (SpectrumAnalyzerStateTests, SkipSamplesDropsTheOldestSamples)
{
    pushRamp (2048 + 100);

    analyzer->skipSamples (100);
    EXPECT_EQ (2048, analyzer->getNumAvailableSamples());

    testBuffer.resize (2048);
    ASSERT_TRUE (analyzer->getFFTData (Span<float> (testBuffer)));
    EXPECT_FLOAT_EQ (100.0f, testBuffer.front());

    // Nothing for a negative count, and no more than what is buffered
    const int numBuffered = analyzer->getNumAvailableSamples();

    analyzer->skipSamples (-10);
    EXPECT_EQ (numBuffered, analyzer->getNumAvailableSamples());

    analyzer->skipSamples (numBuffered + 1000);
    EXPECT_EQ (0, analyzer->getNumAvailableSamples());
    EXPECT_FALSE (analyzer->isFFTDataReady());
}

TEST_F (SpectrumAnalyzerStateTests, ResizingWhileTheAudioThreadPushesKeepsTheFramesInOrder)
{
    std::atomic<bool> running { true };

    // The audio thread pushes a ramp, so the samples of every frame read have to be rising
    std::thread audioThread ([&]
    {
        std::vector<float> block (64);
        float nextValue = 0.0f;

        while (running)
        {
            for (auto& sample : block)
                sample = nextValue++;

            analyzer->pushSamples (block.data(), static_cast<int> (block.size()));
            std::this_thread::sleep_for (std::chrono::microseconds (50));
        }
    });

    const int fftSizes[] = { 64, 1024, 256, 4096 };
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds (5);

    int numFramesRead = 0;
    int numFramesOutOfOrder = 0;

    for (int i = 0; numFramesRead < 100 && std::chrono::steady_clock::now() < deadline; ++i)
    {
        const int fftSize = fftSizes[i % 4];

        analyzer->setFftSize (fftSize);
        analyzer->setOverlapFactor (i % 2 == 0 ? 0.0f : 0.75f);
        testBuffer.resize (static_cast<size_t> (fftSize));

        while (analyzer->getFFTData (Span<float> (testBuffer)))
        {
            ++numFramesRead;

            if (std::adjacent_find (testBuffer.begin(), testBuffer.end(), std::greater_equal<float>()) != testBuffer.end())
                ++numFramesOutOfOrder;
        }

        std::this_thread::yield();
    }

    running = false;
    audioThread.join();

    EXPECT_GE (numFramesRead, 100);
    EXPECT_EQ (0, numFramesOutOfOrder);
}
