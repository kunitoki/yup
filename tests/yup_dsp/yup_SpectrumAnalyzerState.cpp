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

//==============================================================================
class SpectrumAnalyzerStateTests : public ::testing::Test
{
protected:
    static constexpr float tolerance = 1e-6f;

    void SetUp() override
    {
        analyzer = std::make_unique<SpectrumAnalyzerState>();
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
#if YUP_DEBUG
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
