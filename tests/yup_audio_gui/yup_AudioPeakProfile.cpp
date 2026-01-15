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

#include <gtest/gtest.h>

#include <yup_audio_gui/yup_audio_gui.h>

using namespace yup;

namespace
{
constexpr int kTestSampleRate = 44100;
constexpr int kSmallBufferSize = 1000;
constexpr int kMediumBufferSize = 100000;
constexpr int kLargeBufferSize = 50000000;

AudioBuffer<float> createTestBuffer (int numChannels, int numSamples, float frequency = 440.0f)
{
    AudioBuffer<float> buffer (numChannels, numSamples);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* channelData = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
        {
            const float phase = (i / static_cast<float> (kTestSampleRate)) * frequency * 2.0f * MathConstants<float>::pi;
            channelData[i] = std::sin (phase);
        }
    }

    return buffer;
}
} // namespace

class AudioPeakProfileTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        profile = std::make_unique<AudioPeakProfile>();
    }

    void TearDown() override
    {
        profile.reset();
    }

    std::unique_ptr<AudioPeakProfile> profile;
};

TEST_F (AudioPeakProfileTests, DefaultConstructorCreatesInvalidProfile)
{
    EXPECT_FALSE (profile->isValid());
    EXPECT_EQ (0, profile->getNumSamples());
    EXPECT_EQ (0, profile->getNumChannels());
    EXPECT_EQ (0, profile->getNumAggregationLevels());
}

TEST_F (AudioPeakProfileTests, AdaptiveBaseResolutionSelectsCorrectly)
{
    EXPECT_EQ (1, AudioPeakProfile::calculateOptimalBaseResolution (1000));
    EXPECT_EQ (1, AudioPeakProfile::calculateOptimalBaseResolution (5000000));
    EXPECT_EQ (256, AudioPeakProfile::calculateOptimalBaseResolution (50000000));
    EXPECT_EQ (512, AudioPeakProfile::calculateOptimalBaseResolution (500000000));
    EXPECT_EQ (1024, AudioPeakProfile::calculateOptimalBaseResolution (2000000000));
}

TEST_F (AudioPeakProfileTests, DefaultAggregationFactorsAreCorrect)
{
    auto factors = AudioPeakProfile::getDefaultAggregationFactors();
    ASSERT_EQ (3, factors.size());
    EXPECT_EQ (16, factors[0]);
    EXPECT_EQ (256, factors[1]);
    EXPECT_EQ (4096, factors[2]);
}

TEST_F (AudioPeakProfileTests, BuildFromBufferWithSmallBuffer)
{
    auto buffer = createTestBuffer (2, kSmallBufferSize);
    auto factors = AudioPeakProfile::getDefaultAggregationFactors();

    auto result = profile->buildFromBuffer (buffer, 1, factors);

    EXPECT_TRUE (result.wasOk());
    EXPECT_TRUE (profile->isValid());
    EXPECT_EQ (kSmallBufferSize, profile->getNumSamples());
    EXPECT_EQ (2, profile->getNumChannels());
    EXPECT_EQ (1, profile->getBaseResolution());
    EXPECT_EQ (4, profile->getNumAggregationLevels()); // Base + 3 aggregated
}

TEST_F (AudioPeakProfileTests, BuildFromBufferWithProgressCallback)
{
    auto buffer = createTestBuffer (1, kMediumBufferSize);
    auto factors = AudioPeakProfile::getDefaultAggregationFactors();

    double lastProgress = 0.0;
    int callCount = 0;

    auto result = profile->buildFromBuffer (buffer, 256, factors, [&lastProgress, &callCount] (double progress) -> bool
    {
        lastProgress = progress;
        ++callCount;
        return true; // Continue
    });

    EXPECT_TRUE (result.wasOk());
    EXPECT_GE (callCount, 1);
    EXPECT_GE (lastProgress, 0.9); // Should reach near 100%
}

TEST_F (AudioPeakProfileTests, PeakValuesAreAccurate)
{
    // Create buffer with known values
    AudioBuffer<float> buffer (1, 10);
    float* data = buffer.getWritePointer (0);
    for (int i = 0; i < 10; ++i)
        data[i] = static_cast<float> (i - 5); // -5, -4, -3, -2, -1, 0, 1, 2, 3, 4

    auto result = profile->buildFromBuffer (buffer, 1, {});
    EXPECT_TRUE (result.wasOk());

    const auto& peaks = profile->getChannelPeaks (0, 0);
    EXPECT_EQ (10, peaks.minValues.size());
    EXPECT_EQ (10, peaks.maxValues.size());

    // Verify each peak matches input
    for (int i = 0; i < 10; ++i)
    {
        EXPECT_FLOAT_EQ (data[i], peaks.minValues[i]);
        EXPECT_FLOAT_EQ (data[i], peaks.maxValues[i]);
    }
}

TEST_F (AudioPeakProfileTests, AggregationLevelsAreCorrect)
{
    auto buffer = createTestBuffer (1, 1000);
    auto result = profile->buildFromBuffer (buffer, 1, { 10, 100 });

    EXPECT_TRUE (result.wasOk());
    EXPECT_EQ (3, profile->getNumAggregationLevels());

    EXPECT_EQ (1, profile->getAggregationFactor (0));
    EXPECT_EQ (10, profile->getAggregationFactor (1));
    EXPECT_EQ (100, profile->getAggregationFactor (2));

    const auto& baseLevel = profile->getChannelPeaks (0, 0);
    const auto& level1 = profile->getChannelPeaks (0, 1);
    const auto& level2 = profile->getChannelPeaks (0, 2);

    EXPECT_EQ (1000, baseLevel.minValues.size());
    EXPECT_EQ (100, level1.minValues.size());
    EXPECT_EQ (10, level2.minValues.size());
}

TEST_F (AudioPeakProfileTests, AggregatedPeaksPreserveExtrema)
{
    // Create buffer with known extrema
    AudioBuffer<float> buffer (1, 100);
    float* data = buffer.getWritePointer (0);
    for (int i = 0; i < 100; ++i)
        data[i] = std::sin (i * 0.1f);

    auto result = profile->buildFromBuffer (buffer, 1, { 10 });
    EXPECT_TRUE (result.wasOk());

    const auto& baseLevel = profile->getChannelPeaks (0, 0);
    const auto& aggregated = profile->getChannelPeaks (0, 1);

    // Verify aggregated level preserves overall min/max
    float baseMin = *std::min_element (baseLevel.minValues.begin(), baseLevel.minValues.end());
    float baseMax = *std::max_element (baseLevel.maxValues.begin(), baseLevel.maxValues.end());

    float aggMin = *std::min_element (aggregated.minValues.begin(), aggregated.minValues.end());
    float aggMax = *std::max_element (aggregated.maxValues.begin(), aggregated.maxValues.end());

    EXPECT_FLOAT_EQ (baseMin, aggMin);
    EXPECT_FLOAT_EQ (baseMax, aggMax);
}

TEST_F (AudioPeakProfileTests, GetPeakRangeForSamplesCalculatesCorrectly)
{
    auto buffer = createTestBuffer (1, 1000);
    auto result = profile->buildFromBuffer (buffer, 10, {});
    EXPECT_TRUE (result.wasOk());

    // With base resolution of 10, we have 100 peaks
    auto range = profile->getPeakRangeForSamples (Range<int> (0, 100), 0);
    EXPECT_EQ (0, range.getStart());
    EXPECT_EQ (10, range.getEnd());

    range = profile->getPeakRangeForSamples (Range<int> (100, 200), 0);
    EXPECT_EQ (10, range.getStart());
    EXPECT_EQ (20, range.getEnd());
}

TEST_F (AudioPeakProfileTests, SerializationRoundTrip)
{
    auto buffer = createTestBuffer (2, 1000);
    auto result = profile->buildFromBuffer (buffer, 1, { 16 });
    EXPECT_TRUE (result.wasOk());

    // Serialize
    auto serialized = profile->serialize();
    EXPECT_GT (serialized.getSize(), 0);

    // Deserialize into new profile
    AudioPeakProfile newProfile;
    auto deserializeResult = newProfile.deserialize (serialized);

    EXPECT_TRUE (deserializeResult.wasOk());
    EXPECT_EQ (profile->getNumSamples(), newProfile.getNumSamples());
    EXPECT_EQ (profile->getNumChannels(), newProfile.getNumChannels());
    EXPECT_EQ (profile->getBaseResolution(), newProfile.getBaseResolution());
    EXPECT_EQ (profile->getNumAggregationLevels(), newProfile.getNumAggregationLevels());

    // Verify peak data matches
    const auto& originalPeaks = profile->getChannelPeaks (0, 0);
    const auto& deserializedPeaks = newProfile.getChannelPeaks (0, 0);

    EXPECT_EQ (originalPeaks.minValues.size(), deserializedPeaks.minValues.size());
    EXPECT_EQ (originalPeaks.maxValues.size(), deserializedPeaks.maxValues.size());

    for (size_t i = 0; i < originalPeaks.minValues.size(); ++i)
    {
        EXPECT_FLOAT_EQ (originalPeaks.minValues[i], deserializedPeaks.minValues[i]);
        EXPECT_FLOAT_EQ (originalPeaks.maxValues[i], deserializedPeaks.maxValues[i]);
    }
}

TEST_F (AudioPeakProfileTests, SaveAndLoadFromFile)
{
    auto buffer = createTestBuffer (2, 1000);
    auto result = profile->buildFromBuffer (buffer, 1, { 16, 256 });
    EXPECT_TRUE (result.wasOk());

    // Save to temporary file
    File tempFile = File::getSpecialLocation (File::SpecialLocationType::tempDirectory)
                        .getChildFile ("test_profile.yuppeaks");
    tempFile.deleteFile();

    auto saveResult = profile->saveToFile (tempFile);
    EXPECT_TRUE (saveResult.wasOk());
    EXPECT_TRUE (tempFile.existsAsFile());

    // Load into new profile
    AudioPeakProfile loadedProfile;
    auto loadResult = loadedProfile.loadFromFile (tempFile);

    EXPECT_TRUE (loadResult.wasOk());
    EXPECT_EQ (profile->getNumSamples(), loadedProfile.getNumSamples());
    EXPECT_EQ (profile->getNumChannels(), loadedProfile.getNumChannels());

    // Cleanup
    tempFile.deleteFile();
}

TEST_F (AudioPeakProfileTests, EmptyBufferHandling)
{
    AudioBuffer<float> emptyBuffer (0, 0);
    auto result = profile->buildFromBuffer (emptyBuffer, 1, {});

    EXPECT_FALSE (result.wasOk());
    EXPECT_FALSE (profile->isValid());
}

TEST_F (AudioPeakProfileTests, MultiChannelHandling)
{
    auto buffer = createTestBuffer (8, kSmallBufferSize);
    auto result = profile->buildFromBuffer (buffer, 1, { 16 });

    EXPECT_TRUE (result.wasOk());
    EXPECT_EQ (8, profile->getNumChannels());

    // Verify each channel has peaks
    for (int ch = 0; ch < 8; ++ch)
    {
        const auto& peaks = profile->getChannelPeaks (ch, 0);
        EXPECT_EQ (kSmallBufferSize, peaks.minValues.size());
        EXPECT_EQ (kSmallBufferSize, peaks.maxValues.size());
    }
}
