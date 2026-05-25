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
class TimeStretchProcessorTests : public ::testing::Test
{
protected:
    static constexpr double sampleRate = 48000.0;
    static constexpr int maximumBlockSize = 512;
    static constexpr int numChannels = 2;

    void SetUp() override
    {
        spec.inputSampleRate = sampleRate;
        spec.outputSampleRate = sampleRate;
        spec.maximumBlockSize = maximumBlockSize;
        spec.numChannels = numChannels;

        // Initialize test buffers
        inputBuffer.setSize (numChannels, maximumBlockSize);
        outputBuffer.setSize (numChannels, maximumBlockSize * 4);

        // Fill input with a test signal (sine wave + noise)
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* channelData = inputBuffer.getWritePointer (ch);
            for (int i = 0; i < maximumBlockSize; ++i)
            {
                const double phase = 2.0 * MathConstants<double>::pi * 440.0 * i / sampleRate;
                channelData[i] = static_cast<float> (0.5 * std::sin (phase));
            }
        }
    }

    TimeStretchProcessor::ProcessSpec spec;
    AudioBuffer<float> inputBuffer;
    AudioBuffer<float> outputBuffer;
    int64 inputPosition = 0;
};

//==============================================================================
TEST_F (TimeStretchProcessorTests, DefaultConstruction)
{
    TimeStretchProcessor processor;

    EXPECT_FALSE (processor.isPrepared());
    EXPECT_DOUBLE_EQ (processor.getTimeRatio(), 1.0);
    EXPECT_DOUBLE_EQ (processor.getPitchRatio(), 1.0);
}

TEST_F (TimeStretchProcessorTests, ParametersConstruction)
{
    TimeStretchProcessor::Parameters params;
    params.timeRatio = 1.5;
    params.pitchRatio = 2.0;

    EXPECT_DOUBLE_EQ (params.timeRatio, 1.5);
    EXPECT_DOUBLE_EQ (params.pitchRatio, 2.0);
}

TEST_F (TimeStretchProcessorTests, MoveConstructor)
{
    TimeStretchProcessor processor1;
    auto result = processor1.prepare (spec);
    ASSERT_TRUE (result.wasOk());

    TimeStretchProcessor processor2 (std::move (processor1));
    EXPECT_TRUE (processor2.isPrepared());
}

TEST_F (TimeStretchProcessorTests, MoveAssignment)
{
    TimeStretchProcessor processor1;
    auto result = processor1.prepare (spec);
    ASSERT_TRUE (result.wasOk());

    TimeStretchProcessor processor2;
    processor2 = std::move (processor1);
    EXPECT_TRUE (processor2.isPrepared());
}

//==============================================================================
TEST_F (TimeStretchProcessorTests, PrepareWithValidSpec)
{
    TimeStretchProcessor processor;
    auto result = processor.prepare (spec);

    ASSERT_TRUE (result.wasOk());
    EXPECT_TRUE (processor.isPrepared());
}

TEST_F (TimeStretchProcessorTests, PrepareWithAutomaticBackend)
{
    TimeStretchProcessor processor;
    auto result = processor.prepare (spec, TimeStretchProcessor::Backend::automatic);

    ASSERT_TRUE (result.wasOk());
    EXPECT_TRUE (processor.isPrepared());
}

TEST_F (TimeStretchProcessorTests, PrepareWithTimeDomainBackend)
{
    TimeStretchProcessor processor;
    auto result = processor.prepare (spec, TimeStretchProcessor::Backend::timeDomain);

    ASSERT_TRUE (result.wasOk());
    EXPECT_TRUE (processor.isPrepared());
    EXPECT_EQ (processor.getBackend(), TimeStretchProcessor::Backend::timeDomain);
    EXPECT_EQ (processor.getBackendName(), "Time Domain");
}

#if YUP_ENABLE_BUNGEE
TEST_F (TimeStretchProcessorTests, PrepareWithBungeeBackend)
{
    TimeStretchProcessor processor;
    auto result = processor.prepare (spec, TimeStretchProcessor::Backend::bungee);

    ASSERT_TRUE (result.wasOk());
    EXPECT_TRUE (processor.isPrepared());
    EXPECT_EQ (processor.getBackend(), TimeStretchProcessor::Backend::bungee);
    EXPECT_EQ (processor.getBackendName(), "Bungee");
}
#endif

TEST_F (TimeStretchProcessorTests, PrepareWithInvalidSampleRate)
{
    TimeStretchProcessor processor;
    spec.inputSampleRate = 0.0;

    auto result = processor.prepare (spec);

    EXPECT_TRUE (result.failed());
    EXPECT_FALSE (processor.isPrepared());
}

TEST_F (TimeStretchProcessorTests, PrepareWithInvalidBlockSize)
{
    TimeStretchProcessor processor;
    spec.maximumBlockSize = 0;

    auto result = processor.prepare (spec);

    EXPECT_TRUE (result.failed());
    EXPECT_FALSE (processor.isPrepared());
}

TEST_F (TimeStretchProcessorTests, PrepareWithInvalidChannelCount)
{
    TimeStretchProcessor processor;
    spec.numChannels = 0;

    auto result = processor.prepare (spec);

    EXPECT_TRUE (result.failed());
    EXPECT_FALSE (processor.isPrepared());
}

TEST_F (TimeStretchProcessorTests, PrepareWithZeroOutputSampleRate)
{
    TimeStretchProcessor processor;
    spec.outputSampleRate = 0.0;

    auto result = processor.prepare (spec);

    ASSERT_TRUE (result.wasOk());
    EXPECT_TRUE (processor.isPrepared());
}

TEST_F (TimeStretchProcessorTests, PrepareWithDifferentOutputSampleRate)
{
    TimeStretchProcessor processor;
    spec.outputSampleRate = 96000.0;

    auto result = processor.prepare (spec);

    ASSERT_TRUE (result.wasOk());
    EXPECT_TRUE (processor.isPrepared());
}

//==============================================================================
TEST_F (TimeStretchProcessorTests, BackendAvailability)
{
    EXPECT_TRUE (TimeStretchProcessor::isBackendAvailable (TimeStretchProcessor::Backend::automatic));
    EXPECT_TRUE (TimeStretchProcessor::isBackendAvailable (TimeStretchProcessor::Backend::timeDomain));
#if YUP_ENABLE_BUNGEE
    EXPECT_TRUE (TimeStretchProcessor::isBackendAvailable (TimeStretchProcessor::Backend::bungee));
#endif
}

TEST_F (TimeStretchProcessorTests, GetAvailableBackends)
{
    auto backends = TimeStretchProcessor::getAvailableBackends();

    EXPECT_FALSE (backends.empty());
    EXPECT_TRUE (std::find (backends.begin(), backends.end(), TimeStretchProcessor::Backend::timeDomain) != backends.end());
#if YUP_ENABLE_BUNGEE
    EXPECT_TRUE (std::find (backends.begin(), backends.end(), TimeStretchProcessor::Backend::bungee) != backends.end());
#endif
}

#if YUP_ENABLE_BUNGEE
TEST_F (TimeStretchProcessorTests, SetBackendBeforePrepare)
{
    TimeStretchProcessor processor;
    auto result = processor.setBackend (TimeStretchProcessor::Backend::bungee);

    ASSERT_TRUE (result.wasOk());
    EXPECT_EQ (processor.getBackend(), TimeStretchProcessor::Backend::bungee);
}
#endif

#if YUP_ENABLE_BUNGEE
TEST_F (TimeStretchProcessorTests, SetBackendAfterPrepare)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec).wasOk());

    auto result = processor.setBackend (TimeStretchProcessor::Backend::bungee);

    ASSERT_TRUE (result.wasOk());
    EXPECT_EQ (processor.getBackend(), TimeStretchProcessor::Backend::bungee);
}
#endif

TEST_F (TimeStretchProcessorTests, SetTimeDomainBackendAfterPrepare)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec).wasOk());

    auto result = processor.setBackend (TimeStretchProcessor::Backend::timeDomain);

    ASSERT_TRUE (result.wasOk());
    EXPECT_EQ (processor.getBackend(), TimeStretchProcessor::Backend::timeDomain);
    EXPECT_EQ (processor.getBackendName(), "Time Domain");
}

#if YUP_ENABLE_BUNGEE
TEST_F (TimeStretchProcessorTests, SetSameBackend)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec, TimeStretchProcessor::Backend::bungee).wasOk());

    auto result = processor.setBackend (TimeStretchProcessor::Backend::bungee);

    ASSERT_TRUE (result.wasOk());
}
#endif

//==============================================================================
TEST_F (TimeStretchProcessorTests, SetTimeRatio)
{
    TimeStretchProcessor processor;
    processor.setTimeRatio (1.5);

    EXPECT_DOUBLE_EQ (processor.getTimeRatio(), 1.5);
}

TEST_F (TimeStretchProcessorTests, SetTimeRatioSpeedUp)
{
    TimeStretchProcessor processor;
    processor.setTimeRatio (0.5);

    EXPECT_DOUBLE_EQ (processor.getTimeRatio(), 0.5);
}

TEST_F (TimeStretchProcessorTests, SetTimeRatioSlowDown)
{
    TimeStretchProcessor processor;
    processor.setTimeRatio (2.0);

    EXPECT_DOUBLE_EQ (processor.getTimeRatio(), 2.0);
}

TEST_F (TimeStretchProcessorTests, DISABLED_SetInvalidTimeRatio)
{
    TimeStretchProcessor processor;
    processor.setTimeRatio (-1.0);

    // Should clamp to 1.0
    EXPECT_DOUBLE_EQ (processor.getTimeRatio(), 1.0);
}

TEST_F (TimeStretchProcessorTests, SetPitchRatio)
{
    TimeStretchProcessor processor;
    processor.setPitchRatio (1.5);

    EXPECT_DOUBLE_EQ (processor.getPitchRatio(), 1.5);
}

TEST_F (TimeStretchProcessorTests, SetPitchRatioHigher)
{
    TimeStretchProcessor processor;
    processor.setPitchRatio (2.0);

    EXPECT_DOUBLE_EQ (processor.getPitchRatio(), 2.0);
}

TEST_F (TimeStretchProcessorTests, SetPitchRatioLower)
{
    TimeStretchProcessor processor;
    processor.setPitchRatio (0.5);

    EXPECT_DOUBLE_EQ (processor.getPitchRatio(), 0.5);
}

TEST_F (TimeStretchProcessorTests, DISABLED_SetInvalidPitchRatio)
{
    TimeStretchProcessor processor;
    processor.setPitchRatio (-1.0);

    // Should clamp to 1.0
    EXPECT_DOUBLE_EQ (processor.getPitchRatio(), 1.0);
}

TEST_F (TimeStretchProcessorTests, SetParametersStruct)
{
    TimeStretchProcessor processor;
    TimeStretchProcessor::Parameters params;
    params.timeRatio = 1.25;
    params.pitchRatio = 0.8;

    processor.setParameters (params);

    EXPECT_DOUBLE_EQ (processor.getTimeRatio(), 1.25);
    EXPECT_DOUBLE_EQ (processor.getPitchRatio(), 0.8);
}

TEST_F (TimeStretchProcessorTests, GetParameters)
{
    TimeStretchProcessor processor;
    processor.setTimeRatio (1.5);
    processor.setPitchRatio (2.0);

    auto params = processor.getParameters();

    EXPECT_DOUBLE_EQ (params.timeRatio, 1.5);
    EXPECT_DOUBLE_EQ (params.pitchRatio, 2.0);
}

//==============================================================================
TEST_F (TimeStretchProcessorTests, GetExpectedOutputFrameCount)
{
    TimeStretchProcessor processor;
    processor.setTimeRatio (2.0);

    auto expected = processor.getExpectedOutputFrameCount (100);

    EXPECT_EQ (expected, 200);
}

TEST_F (TimeStretchProcessorTests, GetExpectedOutputFrameCountSpeedUp)
{
    TimeStretchProcessor processor;
    processor.setTimeRatio (0.5);

    auto expected = processor.getExpectedOutputFrameCount (100);

    EXPECT_EQ (expected, 50);
}

TEST_F (TimeStretchProcessorTests, GetExpectedOutputFrameCountZeroInput)
{
    TimeStretchProcessor processor;

    auto expected = processor.getExpectedOutputFrameCount (0);

    EXPECT_EQ (expected, 0);
}

TEST_F (TimeStretchProcessorTests, GetExpectedOutputFrameCountNegativeInput)
{
    TimeStretchProcessor processor;

    auto expected = processor.getExpectedOutputFrameCount (-10);

    EXPECT_EQ (expected, 0);
}

//==============================================================================
TEST_F (TimeStretchProcessorTests, SetInputPosition)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec).wasOk());

    processor.setInputPosition (1000);

    // No direct way to verify, but should not crash
    SUCCEED();
}

TEST_F (TimeStretchProcessorTests, GetMaxInputFrameCount)
{
    TimeStretchProcessor processor;

    // Before prepare, should return 0
    EXPECT_EQ (processor.getMaxInputFrameCount(), 0);

    ASSERT_TRUE (processor.prepare (spec).wasOk());

    // After prepare, should return a positive value
    EXPECT_GT (processor.getMaxInputFrameCount(), 0);
}

TEST_F (TimeStretchProcessorTests, TimeDomainMaxInputFrameCountCoversPitchChanges)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec, TimeStretchProcessor::Backend::timeDomain).wasOk());

    const int preparedMaxInputFrames = processor.getMaxInputFrameCount();
    EXPECT_GT (preparedMaxInputFrames, 0);

    processor.setTimeRatio (0.5);
    processor.setPitchRatio (0.5);

    EXPECT_LE (processor.getMaxInputFrameCount(), preparedMaxInputFrames);
}

TEST_F (TimeStretchProcessorTests, GetLatencyInFrames)
{
    TimeStretchProcessor processor;

    // Before prepare, should return 0
    EXPECT_DOUBLE_EQ (processor.getLatencyInFrames(), 0.0);

    ASSERT_TRUE (processor.prepare (spec).wasOk());

    // After prepare, latency should be non-negative
    EXPECT_GE (processor.getLatencyInFrames(), 0.0);
}

//==============================================================================
TEST_F (TimeStretchProcessorTests, SetInputProvider)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec).wasOk());

    bool providerCalled = false;
    auto provider = [&providerCalled, numChannels = this->numChannels, sampleRate = this->sampleRate] (int64 beginFrame, int numFrames, float* const* destChannels, int channelStride, int& muteHead, int& muteTail)
    {
        providerCalled = true;
        muteHead = 0;
        muteTail = 0;

        // Fill with test data
        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int i = 0; i < numFrames; ++i)
            {
                const double phase = 2.0 * MathConstants<double>::pi * 440.0
                                   * (beginFrame + i) / sampleRate;
                destChannels[ch][i] = static_cast<float> (0.5 * std::sin (phase));
            }
        }
    };

    processor.setInputProvider (provider);

    // Process to trigger the provider
    processor.setTimeRatio (1.5);
    auto result = processor.process (inputBuffer.getArrayOfReadPointers(),
                                     inputBuffer.getNumSamples(),
                                     outputBuffer.getArrayOfWritePointers(),
                                     processor.getExpectedOutputFrameCount (inputBuffer.getNumSamples()));

    ASSERT_TRUE (result.wasOk());
    EXPECT_TRUE (providerCalled);
}

//==============================================================================
TEST_F (TimeStretchProcessorTests, ProcessWithoutPrepare)
{
    TimeStretchProcessor processor;

    auto result = processor.process (inputBuffer.getArrayOfReadPointers(),
                                     inputBuffer.getNumSamples(),
                                     outputBuffer.getArrayOfWritePointers(),
                                     maximumBlockSize);

    EXPECT_TRUE (result.failed());
}

TEST_F (TimeStretchProcessorTests, ProcessWithValidInput)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec).wasOk());

    // Set input provider
    auto provider = [this] (int64 beginFrame, int numFrames, float* const* destChannels, int channelStride, int& muteHead, int& muteTail)
    {
        (void) channelStride;

        const int64 totalLength = maximumBlockSize;
        const int64 clampedBegin = jlimit<int64> (0, totalLength, beginFrame);
        const int64 clampedEnd = jlimit<int64> (0, totalLength, beginFrame + numFrames);

        if (clampedBegin >= clampedEnd)
        {
            muteHead = numFrames;
            muteTail = 0;
            for (int ch = 0; ch < numChannels; ++ch)
                std::fill (destChannels[ch], destChannels[ch] + numFrames, 0.0f);
            return;
        }

        muteHead = static_cast<int> (clampedBegin - beginFrame);
        muteTail = static_cast<int> ((beginFrame + numFrames) - clampedEnd);
        const int validFrames = static_cast<int> (clampedEnd - clampedBegin);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            if (muteHead > 0)
                std::fill (destChannels[ch], destChannels[ch] + muteHead, 0.0f);

            const float* srcData = inputBuffer.getReadPointer (ch);
            for (int i = 0; i < validFrames; ++i)
                destChannels[ch][muteHead + i] = srcData[static_cast<int> (clampedBegin + i)];

            if (muteTail > 0)
                std::fill (destChannels[ch] + muteHead + validFrames,
                           destChannels[ch] + numFrames,
                           0.0f);
        }
    };
    processor.setInputProvider (provider);

    processor.setTimeRatio (1.5);
    const int outputFrames = processor.getExpectedOutputFrameCount (maximumBlockSize);

    auto result = processor.process (inputBuffer.getArrayOfReadPointers(),
                                     inputBuffer.getNumSamples(),
                                     outputBuffer.getArrayOfWritePointers(),
                                     outputFrames);

    ASSERT_TRUE (result.wasOk());
    EXPECT_GT (result.getValue(), 0);
    EXPECT_LE (result.getValue(), outputFrames + 1);

    // Verify output is not silent
    bool hasNonZeroSamples = false;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto* channelData = outputBuffer.getReadPointer (ch);
        for (int i = 0; i < result.getValue(); ++i)
        {
            if (std::abs (channelData[i]) > 0.0001f)
            {
                hasNonZeroSamples = true;
                break;
            }
        }
    }
    EXPECT_TRUE (hasNonZeroSamples);
}

TEST_F (TimeStretchProcessorTests, TimeDomainBackendProcessesInput)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec, TimeStretchProcessor::Backend::timeDomain).wasOk());

    processor.setTimeRatio (1.5);
    const int outputFrames = processor.getExpectedOutputFrameCount (inputBuffer.getNumSamples());

    auto result = processor.process (inputBuffer.getArrayOfReadPointers(),
                                     inputBuffer.getNumSamples(),
                                     outputBuffer.getArrayOfWritePointers(),
                                     outputFrames);

    ASSERT_TRUE (result.wasOk());
    EXPECT_EQ (result.getValue(), outputFrames);

    bool hasNonZeroSamples = false;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto* channelData = outputBuffer.getReadPointer (ch);
        for (int i = 0; i < result.getValue(); ++i)
        {
            if (std::abs (channelData[i]) > 0.0001f)
            {
                hasNonZeroSamples = true;
                break;
            }
        }
    }

    EXPECT_TRUE (hasNonZeroSamples);
}

TEST_F (TimeStretchProcessorTests, TimeDomainBackendSupportsPitchShift)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec, TimeStretchProcessor::Backend::timeDomain).wasOk());

    processor.setPitchRatio (2.0);
    const int outputFrames = processor.getExpectedOutputFrameCount (inputBuffer.getNumSamples());

    auto result = processor.process (inputBuffer.getArrayOfReadPointers(),
                                     inputBuffer.getNumSamples(),
                                     outputBuffer.getArrayOfWritePointers(),
                                     outputFrames);

    ASSERT_TRUE (result.wasOk());
    EXPECT_EQ (result.getValue(), outputFrames);

    bool hasNonZeroSamples = false;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto* channelData = outputBuffer.getReadPointer (ch);
        for (int i = 0; i < result.getValue(); ++i)
        {
            if (std::abs (channelData[i]) > 0.0001f)
            {
                hasNonZeroSamples = true;
                break;
            }
        }
    }

    EXPECT_TRUE (hasNonZeroSamples);
}

TEST_F (TimeStretchProcessorTests, TimeDomainProviderMuteRegionsAreApplied)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec, TimeStretchProcessor::Backend::timeDomain).wasOk());

    int providerCallCount = 0;
    processor.setInputProvider ([&providerCallCount, numChannels = this->numChannels] (int64 beginFrame,
                                                                                       int numFrames,
                                                                                       float* const* destChannels,
                                                                                       int channelStride,
                                                                                       int& muteHead,
                                                                                       int& muteTail)
    {
        (void) beginFrame;
        (void) channelStride;

        ++providerCallCount;
        muteHead = 3;
        muteTail = jmax (0, numFrames - 16);

        for (int ch = 0; ch < numChannels; ++ch)
            std::fill (destChannels[ch], destChannels[ch] + numFrames, static_cast<float> (ch + 1));
    });

    AudioBuffer<float> providerOutput (numChannels, 32);
    auto result = processor.process (nullptr,
                                     0,
                                     providerOutput.getArrayOfWritePointers(),
                                     providerOutput.getNumSamples());

    ASSERT_TRUE (result.wasOk());
    EXPECT_EQ (result.getValue(), providerOutput.getNumSamples());
    EXPECT_GT (providerCallCount, 0);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto expectedValue = static_cast<float> (ch + 1);
        const auto* samples = providerOutput.getReadPointer (ch);

        for (int i = 0; i < 3; ++i)
            EXPECT_FLOAT_EQ (samples[i], 0.0f);

        for (int i = 3; i < 16; ++i)
            EXPECT_FLOAT_EQ (samples[i], expectedValue);

        for (int i = 16; i < providerOutput.getNumSamples(); ++i)
            EXPECT_FLOAT_EQ (samples[i], 0.0f);
    }
}

TEST_F (TimeStretchProcessorTests, TimeDomainUnityTempoCopiesProviderInput)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec, TimeStretchProcessor::Backend::timeDomain).wasOk());

    int lastRequestedFrameCount = 0;
    processor.setInputProvider ([&lastRequestedFrameCount, numChannels = this->numChannels] (int64 beginFrame,
                                                                                             int numFrames,
                                                                                             float* const* destChannels,
                                                                                             int channelStride,
                                                                                             int& muteHead,
                                                                                             int& muteTail)
    {
        (void) channelStride;

        lastRequestedFrameCount = numFrames;
        muteHead = 0;
        muteTail = 0;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int i = 0; i < numFrames; ++i)
                destChannels[ch][i] = static_cast<float> ((beginFrame + i) * (ch + 1));
        }
    });

    AudioBuffer<float> providerOutput (numChannels, 64);
    auto result = processor.process (nullptr,
                                     0,
                                     providerOutput.getArrayOfWritePointers(),
                                     providerOutput.getNumSamples());

    ASSERT_TRUE (result.wasOk());
    EXPECT_EQ (result.getValue(), providerOutput.getNumSamples());
    EXPECT_GE (lastRequestedFrameCount, providerOutput.getNumSamples());

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto* samples = providerOutput.getReadPointer (ch);
        for (int i = 0; i < providerOutput.getNumSamples(); ++i)
            EXPECT_FLOAT_EQ (samples[i], static_cast<float> (i * (ch + 1)));
    }
}

TEST_F (TimeStretchProcessorTests, TimeDomainStretchUsesOverlapSearchWithProviderInput)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec, TimeStretchProcessor::Backend::timeDomain).wasOk());

    int providerCallCount = 0;
    int64 lastBeginFrame = 0;
    processor.setInputProvider ([&providerCallCount, &lastBeginFrame, numChannels = this->numChannels, sampleRate = this->sampleRate] (int64 beginFrame,
                                                                                                                                       int numFrames,
                                                                                                                                       float* const* destChannels,
                                                                                                                                       int channelStride,
                                                                                                                                       int& muteHead,
                                                                                                                                       int& muteTail)
    {
        (void) channelStride;

        ++providerCallCount;
        lastBeginFrame = beginFrame;
        muteHead = 0;
        muteTail = 0;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int i = 0; i < numFrames; ++i)
            {
                const double phase = 2.0 * MathConstants<double>::pi * 220.0
                                   * static_cast<double> (beginFrame + i) / sampleRate;
                destChannels[ch][i] = static_cast<float> (0.5 * std::sin (phase));
            }
        }
    });

    processor.setTimeRatio (1.5);

    AudioBuffer<float> stretchedOutput (numChannels, 4096);
    auto result = processor.process (nullptr,
                                     0,
                                     stretchedOutput.getArrayOfWritePointers(),
                                     stretchedOutput.getNumSamples());

    ASSERT_TRUE (result.wasOk());
    EXPECT_EQ (result.getValue(), stretchedOutput.getNumSamples());
    EXPECT_GT (providerCallCount, 1);
    EXPECT_GT (lastBeginFrame, 0);

    bool hasNonZeroSamples = false;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto* samples = stretchedOutput.getReadPointer (ch);
        for (int i = 0; i < result.getValue(); ++i)
        {
            if (std::abs (samples[i]) > 0.0001f)
            {
                hasNonZeroSamples = true;
                break;
            }
        }
    }

    EXPECT_TRUE (hasNonZeroSamples);
}

TEST_F (TimeStretchProcessorTests, ProcessWithNegativeInputFrameCount)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec).wasOk());

    auto result = processor.process (inputBuffer.getArrayOfReadPointers(),
                                     -1,
                                     outputBuffer.getArrayOfWritePointers(),
                                     maximumBlockSize);

    EXPECT_TRUE (result.failed());
}

TEST_F (TimeStretchProcessorTests, ProcessWithNegativeOutputFrameCount)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec).wasOk());

    auto result = processor.process (inputBuffer.getArrayOfReadPointers(),
                                     inputBuffer.getNumSamples(),
                                     outputBuffer.getArrayOfWritePointers(),
                                     -1);

    EXPECT_TRUE (result.failed());
}

TEST_F (TimeStretchProcessorTests, ProcessWithZeroOutputFrameCount)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec).wasOk());

    auto result = processor.process (inputBuffer.getArrayOfReadPointers(),
                                     inputBuffer.getNumSamples(),
                                     outputBuffer.getArrayOfWritePointers(),
                                     0);

    ASSERT_TRUE (result.wasOk());
    EXPECT_EQ (result.getValue(), 0);
}

TEST_F (TimeStretchProcessorTests, ProcessWithNullInputChannels)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec).wasOk());

    auto result = processor.process (nullptr,
                                     inputBuffer.getNumSamples(),
                                     outputBuffer.getArrayOfWritePointers(),
                                     maximumBlockSize);

    EXPECT_TRUE (result.failed());
}

TEST_F (TimeStretchProcessorTests, ProcessWithNullOutputChannels)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec).wasOk());

    auto result = processor.process (inputBuffer.getArrayOfReadPointers(),
                                     inputBuffer.getNumSamples(),
                                     nullptr,
                                     maximumBlockSize);

    EXPECT_TRUE (result.failed());
}

TEST_F (TimeStretchProcessorTests, ProcessAudioBufferVariant)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec).wasOk());

    // Set input provider
    auto provider = [this] (int64 beginFrame, int numFrames, float* const* destChannels, int channelStride, int& muteHead, int& muteTail)
    {
        (void) channelStride;

        const int64 totalLength = maximumBlockSize;
        const int64 clampedBegin = jlimit<int64> (0, totalLength, beginFrame);
        const int64 clampedEnd = jlimit<int64> (0, totalLength, beginFrame + numFrames);

        if (clampedBegin >= clampedEnd)
        {
            muteHead = numFrames;
            muteTail = 0;
            for (int ch = 0; ch < numChannels; ++ch)
                std::fill (destChannels[ch], destChannels[ch] + numFrames, 0.0f);
            return;
        }

        muteHead = static_cast<int> (clampedBegin - beginFrame);
        muteTail = static_cast<int> ((beginFrame + numFrames) - clampedEnd);
        const int validFrames = static_cast<int> (clampedEnd - clampedBegin);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            if (muteHead > 0)
                std::fill (destChannels[ch], destChannels[ch] + muteHead, 0.0f);

            const float* srcData = inputBuffer.getReadPointer (ch);
            for (int i = 0; i < validFrames; ++i)
                destChannels[ch][muteHead + i] = srcData[static_cast<int> (clampedBegin + i)];

            if (muteTail > 0)
                std::fill (destChannels[ch] + muteHead + validFrames,
                           destChannels[ch] + numFrames,
                           0.0f);
        }
    };
    processor.setInputProvider (provider);

    processor.setTimeRatio (1.5);
    const int outputFrames = processor.getExpectedOutputFrameCount (inputBuffer.getNumSamples());

    auto result = processor.process (inputBuffer, outputBuffer, outputFrames);

    ASSERT_TRUE (result.wasOk());
    EXPECT_GT (result.getValue(), 0);
}

TEST_F (TimeStretchProcessorTests, ProcessAudioBufferChannelMismatch)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec).wasOk());

    AudioBuffer<float> wrongChannelBuffer (1, maximumBlockSize);

    auto result = processor.process (wrongChannelBuffer, outputBuffer, maximumBlockSize);

    EXPECT_TRUE (result.failed());
}

TEST_F (TimeStretchProcessorTests, ProcessAudioBufferOutputTooSmall)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec).wasOk());

    auto result = processor.process (inputBuffer, outputBuffer, outputBuffer.getNumSamples() + 1);

    EXPECT_TRUE (result.failed());
}

//==============================================================================
TEST_F (TimeStretchProcessorTests, ProcessUsingTimeRatio)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec).wasOk());

    // Set input provider
    auto provider = [this] (int64 beginFrame, int numFrames, float* const* destChannels, int channelStride, int& muteHead, int& muteTail)
    {
        (void) channelStride;

        const int64 totalLength = maximumBlockSize;
        const int64 clampedBegin = jlimit<int64> (0, totalLength, beginFrame);
        const int64 clampedEnd = jlimit<int64> (0, totalLength, beginFrame + numFrames);

        if (clampedBegin >= clampedEnd)
        {
            muteHead = numFrames;
            muteTail = 0;
            for (int ch = 0; ch < numChannels; ++ch)
                std::fill (destChannels[ch], destChannels[ch] + numFrames, 0.0f);
            return;
        }

        muteHead = static_cast<int> (clampedBegin - beginFrame);
        muteTail = static_cast<int> ((beginFrame + numFrames) - clampedEnd);
        const int validFrames = static_cast<int> (clampedEnd - clampedBegin);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            if (muteHead > 0)
                std::fill (destChannels[ch], destChannels[ch] + muteHead, 0.0f);

            const float* srcData = inputBuffer.getReadPointer (ch);
            for (int i = 0; i < validFrames; ++i)
                destChannels[ch][muteHead + i] = srcData[static_cast<int> (clampedBegin + i)];

            if (muteTail > 0)
                std::fill (destChannels[ch] + muteHead + validFrames,
                           destChannels[ch] + numFrames,
                           0.0f);
        }
    };
    processor.setInputProvider (provider);

    processor.setTimeRatio (1.5);

    auto result = processor.processUsingTimeRatio (inputBuffer.getArrayOfReadPointers(),
                                                   inputBuffer.getNumSamples(),
                                                   outputBuffer.getArrayOfWritePointers(),
                                                   outputBuffer.getNumSamples());

    ASSERT_TRUE (result.wasOk());
    EXPECT_GT (result.getValue(), 0);
}

TEST_F (TimeStretchProcessorTests, ProcessUsingTimeRatioOutputTooSmall)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec).wasOk());

    processor.setTimeRatio (10.0);

    auto result = processor.processUsingTimeRatio (inputBuffer.getArrayOfReadPointers(),
                                                   inputBuffer.getNumSamples(),
                                                   outputBuffer.getArrayOfWritePointers(),
                                                   10);

    EXPECT_TRUE (result.failed());
}

TEST_F (TimeStretchProcessorTests, ProcessUsingTimeRatioAudioBuffer)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec).wasOk());

    // Set input provider
    auto provider = [this] (int64 beginFrame, int numFrames, float* const* destChannels, int channelStride, int& muteHead, int& muteTail)
    {
        (void) channelStride;

        const int64 totalLength = maximumBlockSize;
        const int64 clampedBegin = jlimit<int64> (0, totalLength, beginFrame);
        const int64 clampedEnd = jlimit<int64> (0, totalLength, beginFrame + numFrames);

        if (clampedBegin >= clampedEnd)
        {
            muteHead = numFrames;
            muteTail = 0;
            for (int ch = 0; ch < numChannels; ++ch)
                std::fill (destChannels[ch], destChannels[ch] + numFrames, 0.0f);
            return;
        }

        muteHead = static_cast<int> (clampedBegin - beginFrame);
        muteTail = static_cast<int> ((beginFrame + numFrames) - clampedEnd);
        const int validFrames = static_cast<int> (clampedEnd - clampedBegin);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            if (muteHead > 0)
                std::fill (destChannels[ch], destChannels[ch] + muteHead, 0.0f);

            const float* srcData = inputBuffer.getReadPointer (ch);
            for (int i = 0; i < validFrames; ++i)
                destChannels[ch][muteHead + i] = srcData[static_cast<int> (clampedBegin + i)];

            if (muteTail > 0)
                std::fill (destChannels[ch] + muteHead + validFrames,
                           destChannels[ch] + numFrames,
                           0.0f);
        }
    };
    processor.setInputProvider (provider);

    processor.setTimeRatio (1.5);

    auto result = processor.processUsingTimeRatio (inputBuffer, outputBuffer);

    ASSERT_TRUE (result.wasOk());
    EXPECT_GT (result.getValue(), 0);
}

TEST_F (TimeStretchProcessorTests, ProcessUsingTimeRatioAudioBufferTooSmall)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec).wasOk());

    processor.setTimeRatio (10.0);

    AudioBuffer<float> smallOutput (numChannels, 10);
    auto result = processor.processUsingTimeRatio (inputBuffer, smallOutput);

    EXPECT_TRUE (result.failed());
}

//==============================================================================
TEST_F (TimeStretchProcessorTests, ResetState)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec).wasOk());

    // Set input provider
    auto provider = [this] (int64 beginFrame, int numFrames, float* const* destChannels, int channelStride, int& muteHead, int& muteTail)
    {
        (void) channelStride;

        const int64 totalLength = maximumBlockSize;
        const int64 clampedBegin = jlimit<int64> (0, totalLength, beginFrame);
        const int64 clampedEnd = jlimit<int64> (0, totalLength, beginFrame + numFrames);

        if (clampedBegin >= clampedEnd)
        {
            muteHead = numFrames;
            muteTail = 0;
            for (int ch = 0; ch < numChannels; ++ch)
                std::fill (destChannels[ch], destChannels[ch] + numFrames, 0.0f);
            return;
        }

        muteHead = static_cast<int> (clampedBegin - beginFrame);
        muteTail = static_cast<int> ((beginFrame + numFrames) - clampedEnd);
        const int validFrames = static_cast<int> (clampedEnd - clampedBegin);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            if (muteHead > 0)
                std::fill (destChannels[ch], destChannels[ch] + muteHead, 0.0f);

            const float* srcData = inputBuffer.getReadPointer (ch);
            for (int i = 0; i < validFrames; ++i)
                destChannels[ch][muteHead + i] = srcData[static_cast<int> (clampedBegin + i)];

            if (muteTail > 0)
                std::fill (destChannels[ch] + muteHead + validFrames,
                           destChannels[ch] + numFrames,
                           0.0f);
        }
    };
    processor.setInputProvider (provider);

    // Process some audio
    processor.process (inputBuffer.getArrayOfReadPointers(),
                       inputBuffer.getNumSamples(),
                       outputBuffer.getArrayOfWritePointers(),
                       processor.getExpectedOutputFrameCount (inputBuffer.getNumSamples()));

    // Reset should not crash
    processor.reset();

    // Should still be able to process after reset
    auto result = processor.process (inputBuffer.getArrayOfReadPointers(),
                                     inputBuffer.getNumSamples(),
                                     outputBuffer.getArrayOfWritePointers(),
                                     processor.getExpectedOutputFrameCount (inputBuffer.getNumSamples()));

    ASSERT_TRUE (result.wasOk());
}

TEST_F (TimeStretchProcessorTests, ResetBeforePrepare)
{
    TimeStretchProcessor processor;

    // Should not crash
    processor.reset();

    SUCCEED();
}

//==============================================================================
TEST_F (TimeStretchProcessorTests, MultipleProcessCalls)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec).wasOk());

    // Set input provider
    auto provider = [this] (int64 beginFrame, int numFrames, float* const* destChannels, int channelStride, int& muteHead, int& muteTail)
    {
        (void) channelStride;

        const int64 totalLength = maximumBlockSize;
        const int64 clampedBegin = jlimit<int64> (0, totalLength, beginFrame);
        const int64 clampedEnd = jlimit<int64> (0, totalLength, beginFrame + numFrames);

        if (clampedBegin >= clampedEnd)
        {
            muteHead = numFrames;
            muteTail = 0;
            for (int ch = 0; ch < numChannels; ++ch)
                std::fill (destChannels[ch], destChannels[ch] + numFrames, 0.0f);
            return;
        }

        muteHead = static_cast<int> (clampedBegin - beginFrame);
        muteTail = static_cast<int> ((beginFrame + numFrames) - clampedEnd);
        const int validFrames = static_cast<int> (clampedEnd - clampedBegin);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            if (muteHead > 0)
                std::fill (destChannels[ch], destChannels[ch] + muteHead, 0.0f);

            const float* srcData = inputBuffer.getReadPointer (ch);
            for (int i = 0; i < validFrames; ++i)
                destChannels[ch][muteHead + i] = srcData[static_cast<int> (clampedBegin + i)];

            if (muteTail > 0)
                std::fill (destChannels[ch] + muteHead + validFrames,
                           destChannels[ch] + numFrames,
                           0.0f);
        }
    };
    processor.setInputProvider (provider);

    processor.setTimeRatio (1.5);

    // Process multiple blocks
    for (int i = 0; i < 5; ++i)
    {
        auto result = processor.process (inputBuffer.getArrayOfReadPointers(),
                                         inputBuffer.getNumSamples(),
                                         outputBuffer.getArrayOfWritePointers(),
                                         processor.getExpectedOutputFrameCount (inputBuffer.getNumSamples()));

        ASSERT_TRUE (result.wasOk());
        EXPECT_GT (result.getValue(), 0);
    }
}

TEST_F (TimeStretchProcessorTests, ChangeParametersDuringProcessing)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec).wasOk());

    // Set input provider
    auto provider = [this] (int64 beginFrame, int numFrames, float* const* destChannels, int channelStride, int& muteHead, int& muteTail)
    {
        (void) channelStride;

        const int64 totalLength = maximumBlockSize;
        const int64 clampedBegin = jlimit<int64> (0, totalLength, beginFrame);
        const int64 clampedEnd = jlimit<int64> (0, totalLength, beginFrame + numFrames);

        if (clampedBegin >= clampedEnd)
        {
            muteHead = numFrames;
            muteTail = 0;
            for (int ch = 0; ch < numChannels; ++ch)
                std::fill (destChannels[ch], destChannels[ch] + numFrames, 0.0f);
            return;
        }

        muteHead = static_cast<int> (clampedBegin - beginFrame);
        muteTail = static_cast<int> ((beginFrame + numFrames) - clampedEnd);
        const int validFrames = static_cast<int> (clampedEnd - clampedBegin);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            if (muteHead > 0)
                std::fill (destChannels[ch], destChannels[ch] + muteHead, 0.0f);

            const float* srcData = inputBuffer.getReadPointer (ch);
            for (int i = 0; i < validFrames; ++i)
                destChannels[ch][muteHead + i] = srcData[static_cast<int> (clampedBegin + i)];

            if (muteTail > 0)
                std::fill (destChannels[ch] + muteHead + validFrames,
                           destChannels[ch] + numFrames,
                           0.0f);
        }
    };
    processor.setInputProvider (provider);

    processor.setTimeRatio (1.5);
    auto result1 = processor.process (inputBuffer.getArrayOfReadPointers(),
                                      inputBuffer.getNumSamples(),
                                      outputBuffer.getArrayOfWritePointers(),
                                      processor.getExpectedOutputFrameCount (inputBuffer.getNumSamples()));
    EXPECT_TRUE (result1.wasOk());

    processor.setTimeRatio (2.0);
    processor.setPitchRatio (0.5);
    auto result2 = processor.process (inputBuffer.getArrayOfReadPointers(),
                                      inputBuffer.getNumSamples(),
                                      outputBuffer.getArrayOfWritePointers(),
                                      processor.getExpectedOutputFrameCount (inputBuffer.getNumSamples()));
    EXPECT_TRUE (result2.wasOk());
}

//==============================================================================
TEST_F (TimeStretchProcessorTests, DifferentTimeRatios)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec).wasOk());

    // Set input provider
    auto provider = [this] (int64 beginFrame, int numFrames, float* const* destChannels, int channelStride, int& muteHead, int& muteTail)
    {
        (void) channelStride;

        const int64 totalLength = maximumBlockSize;
        const int64 clampedBegin = jlimit<int64> (0, totalLength, beginFrame);
        const int64 clampedEnd = jlimit<int64> (0, totalLength, beginFrame + numFrames);

        if (clampedBegin >= clampedEnd)
        {
            muteHead = numFrames;
            muteTail = 0;
            for (int ch = 0; ch < numChannels; ++ch)
                std::fill (destChannels[ch], destChannels[ch] + numFrames, 0.0f);
            return;
        }

        muteHead = static_cast<int> (clampedBegin - beginFrame);
        muteTail = static_cast<int> ((beginFrame + numFrames) - clampedEnd);
        const int validFrames = static_cast<int> (clampedEnd - clampedBegin);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            if (muteHead > 0)
                std::fill (destChannels[ch], destChannels[ch] + muteHead, 0.0f);

            const float* srcData = inputBuffer.getReadPointer (ch);
            for (int i = 0; i < validFrames; ++i)
                destChannels[ch][muteHead + i] = srcData[static_cast<int> (clampedBegin + i)];

            if (muteTail > 0)
                std::fill (destChannels[ch] + muteHead + validFrames,
                           destChannels[ch] + numFrames,
                           0.0f);
        }
    };
    processor.setInputProvider (provider);

    const double ratios[] = { 0.5, 0.75, 1.0, 1.5, 2.0, 3.0 };

    for (auto ratio : ratios)
    {
        processor.setTimeRatio (ratio);
        processor.setInputPosition (0);

        auto result = processor.process (inputBuffer.getArrayOfReadPointers(),
                                         inputBuffer.getNumSamples(),
                                         outputBuffer.getArrayOfWritePointers(),
                                         processor.getExpectedOutputFrameCount (inputBuffer.getNumSamples()));

        EXPECT_TRUE (result.wasOk()) << "Failed with time ratio: " << ratio;
    }
}

TEST_F (TimeStretchProcessorTests, DifferentPitchRatios)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec).wasOk());

    // Set input provider
    auto provider = [this] (int64 beginFrame, int numFrames, float* const* destChannels, int channelStride, int& muteHead, int& muteTail)
    {
        (void) channelStride;

        const int64 totalLength = maximumBlockSize;
        const int64 clampedBegin = jlimit<int64> (0, totalLength, beginFrame);
        const int64 clampedEnd = jlimit<int64> (0, totalLength, beginFrame + numFrames);

        if (clampedBegin >= clampedEnd)
        {
            muteHead = numFrames;
            muteTail = 0;
            for (int ch = 0; ch < numChannels; ++ch)
                std::fill (destChannels[ch], destChannels[ch] + numFrames, 0.0f);
            return;
        }

        muteHead = static_cast<int> (clampedBegin - beginFrame);
        muteTail = static_cast<int> ((beginFrame + numFrames) - clampedEnd);
        const int validFrames = static_cast<int> (clampedEnd - clampedBegin);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            if (muteHead > 0)
                std::fill (destChannels[ch], destChannels[ch] + muteHead, 0.0f);

            const float* srcData = inputBuffer.getReadPointer (ch);
            for (int i = 0; i < validFrames; ++i)
                destChannels[ch][muteHead + i] = srcData[static_cast<int> (clampedBegin + i)];

            if (muteTail > 0)
                std::fill (destChannels[ch] + muteHead + validFrames,
                           destChannels[ch] + numFrames,
                           0.0f);
        }
    };
    processor.setInputProvider (provider);

    const double ratios[] = { 0.5, 0.75, 1.0, 1.5, 2.0 };

    for (auto ratio : ratios)
    {
        processor.setPitchRatio (ratio);
        processor.setInputPosition (0);

        auto result = processor.process (inputBuffer.getArrayOfReadPointers(),
                                         inputBuffer.getNumSamples(),
                                         outputBuffer.getArrayOfWritePointers(),
                                         processor.getExpectedOutputFrameCount (inputBuffer.getNumSamples()));

        EXPECT_TRUE (result.wasOk()) << "Failed with pitch ratio: " << ratio;
    }
}

TEST_F (TimeStretchProcessorTests, IndependentTimeAndPitchShift)
{
    TimeStretchProcessor processor;
    ASSERT_TRUE (processor.prepare (spec).wasOk());

    // Set input provider
    auto provider = [this] (int64 beginFrame, int numFrames, float* const* destChannels, int channelStride, int& muteHead, int& muteTail)
    {
        (void) channelStride;

        const int64 totalLength = maximumBlockSize;
        const int64 clampedBegin = jlimit<int64> (0, totalLength, beginFrame);
        const int64 clampedEnd = jlimit<int64> (0, totalLength, beginFrame + numFrames);

        if (clampedBegin >= clampedEnd)
        {
            muteHead = numFrames;
            muteTail = 0;
            for (int ch = 0; ch < numChannels; ++ch)
                std::fill (destChannels[ch], destChannels[ch] + numFrames, 0.0f);
            return;
        }

        muteHead = static_cast<int> (clampedBegin - beginFrame);
        muteTail = static_cast<int> ((beginFrame + numFrames) - clampedEnd);
        const int validFrames = static_cast<int> (clampedEnd - clampedBegin);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            if (muteHead > 0)
                std::fill (destChannels[ch], destChannels[ch] + muteHead, 0.0f);

            const float* srcData = inputBuffer.getReadPointer (ch);
            for (int i = 0; i < validFrames; ++i)
                destChannels[ch][muteHead + i] = srcData[static_cast<int> (clampedBegin + i)];

            if (muteTail > 0)
                std::fill (destChannels[ch] + muteHead + validFrames,
                           destChannels[ch] + numFrames,
                           0.0f);
        }
    };
    processor.setInputProvider (provider);

    // Slow down without pitch change
    processor.setTimeRatio (2.0);
    processor.setPitchRatio (1.0);

    auto result1 = processor.process (inputBuffer.getArrayOfReadPointers(),
                                      inputBuffer.getNumSamples(),
                                      outputBuffer.getArrayOfWritePointers(),
                                      processor.getExpectedOutputFrameCount (inputBuffer.getNumSamples()));
    EXPECT_TRUE (result1.wasOk());

    processor.reset();

    // Speed up with higher pitch
    processor.setTimeRatio (0.5);
    processor.setPitchRatio (2.0);

    auto result2 = processor.process (inputBuffer.getArrayOfReadPointers(),
                                      inputBuffer.getNumSamples(),
                                      outputBuffer.getArrayOfWritePointers(),
                                      processor.getExpectedOutputFrameCount (inputBuffer.getNumSamples()));
    EXPECT_TRUE (result2.wasOk());
}
