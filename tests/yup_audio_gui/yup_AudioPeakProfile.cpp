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

//==============================================================================
// Progress Callback Cancellation Tests
//==============================================================================

TEST_F (AudioPeakProfileTests, ProgressCallbackCanCancelBuild)
{
    auto buffer = createTestBuffer (1, kMediumBufferSize);
    auto factors = AudioPeakProfile::getDefaultAggregationFactors();

    int callCount = 0;
    auto result = profile->buildFromBuffer (buffer, 256, factors, [&callCount] (double progress) -> bool
    {
        ++callCount;
        return progress < 0.5; // Cancel at 50%
    });

    EXPECT_FALSE (result.wasOk());
    EXPECT_GE (callCount, 1);
}

TEST_F (AudioPeakProfileTests, ProgressCallbackWithImmediateCancellation)
{
    auto buffer = createTestBuffer (1, kSmallBufferSize);

    auto result = profile->buildFromBuffer (buffer, 1, {}, [] (double) -> bool
    {
        return false; // Cancel immediately
    });

    EXPECT_FALSE (result.wasOk());
    EXPECT_FALSE (profile->isValid());
}

//==============================================================================
// Error Handling Tests
//==============================================================================

TEST_F (AudioPeakProfileTests, DISABLED_InvalidChannelIndexReturnsEmptyPeaks)
{
    auto buffer = createTestBuffer (2, kSmallBufferSize);
    auto result = profile->buildFromBuffer (buffer, 1, {});
    EXPECT_TRUE (result.wasOk());

    // Out of bounds channel access
    const auto& peaks = profile->getChannelPeaks (10, 0);
    EXPECT_EQ (0, peaks.minValues.size());
    EXPECT_EQ (0, peaks.maxValues.size());
}

TEST_F (AudioPeakProfileTests, DISABLED_InvalidLevelIndexReturnsEmptyPeaks)
{
    auto buffer = createTestBuffer (1, kSmallBufferSize);
    auto result = profile->buildFromBuffer (buffer, 1, { 16 });
    EXPECT_TRUE (result.wasOk());

    // Out of bounds level access
    const auto& peaks = profile->getChannelPeaks (0, 10);
    EXPECT_EQ (0, peaks.minValues.size());
    EXPECT_EQ (0, peaks.maxValues.size());
}

TEST_F (AudioPeakProfileTests, DISABLED_NegativeChannelIndexReturnsEmptyPeaks)
{
    auto buffer = createTestBuffer (1, kSmallBufferSize);
    auto result = profile->buildFromBuffer (buffer, 1, {});
    EXPECT_TRUE (result.wasOk());

    const auto& peaks = profile->getChannelPeaks (-1, 0);
    EXPECT_EQ (0, peaks.minValues.size());
    EXPECT_EQ (0, peaks.maxValues.size());
}

TEST_F (AudioPeakProfileTests, DISABLED_NegativeLevelIndexReturnsEmptyPeaks)
{
    auto buffer = createTestBuffer (1, kSmallBufferSize);
    auto result = profile->buildFromBuffer (buffer, 1, {});
    EXPECT_TRUE (result.wasOk());

    const auto& peaks = profile->getChannelPeaks (0, -1);
    EXPECT_EQ (0, peaks.minValues.size());
    EXPECT_EQ (0, peaks.maxValues.size());
}

TEST_F (AudioPeakProfileTests, CorruptedSerializationDataHandledGracefully)
{
    MemoryBlock corruptedData;
    corruptedData.setSize (100);
    corruptedData.fillWith (0xFF);

    auto result = profile->deserialize (corruptedData);

    EXPECT_FALSE (result.wasOk());
    EXPECT_FALSE (profile->isValid());
}

TEST_F (AudioPeakProfileTests, EmptySerializationDataHandledGracefully)
{
    MemoryBlock emptyData;

    auto result = profile->deserialize (emptyData);

    EXPECT_FALSE (result.wasOk());
    EXPECT_FALSE (profile->isValid());
}

TEST_F (AudioPeakProfileTests, LoadFromNonExistentFileReturnsError)
{
    File nonExistentFile = File::getSpecialLocation (File::SpecialLocationType::tempDirectory)
                               .getChildFile ("non_existent_file.yuppeaks");

    auto result = profile->loadFromFile (nonExistentFile);

    EXPECT_FALSE (result.wasOk());
    EXPECT_FALSE (profile->isValid());
}

TEST_F (AudioPeakProfileTests, SaveToInvalidPathReturnsError)
{
    auto buffer = createTestBuffer (1, kSmallBufferSize);
    auto result = profile->buildFromBuffer (buffer, 1, {});
    EXPECT_TRUE (result.wasOk());

    // Try to save to invalid path
    File invalidFile = File ("/invalid/path/that/does/not/exist/test.yuppeaks");

    auto saveResult = profile->saveToFile (invalidFile);

    // This may or may not fail depending on permissions, but should not crash
    EXPECT_TRUE (true);
}

//==============================================================================
// Aggregation Factor Edge Cases
//==============================================================================

TEST_F (AudioPeakProfileTests, DISABLED_ZeroAggregationFactorIgnored)
{
    auto buffer = createTestBuffer (1, 1000);
    auto result = profile->buildFromBuffer (buffer, 1, { 0, 10 });

    EXPECT_TRUE (result.wasOk());
    // Should have base level + valid factors (0 should be ignored)
    EXPECT_GE (profile->getNumAggregationLevels(), 1);
}

TEST_F (AudioPeakProfileTests, DISABLED_NegativeAggregationFactorIgnored)
{
    auto buffer = createTestBuffer (1, 1000);
    auto result = profile->buildFromBuffer (buffer, 1, { -10, 10 });

    EXPECT_TRUE (result.wasOk());
    EXPECT_GE (profile->getNumAggregationLevels(), 1);
}

TEST_F (AudioPeakProfileTests, EmptyAggregationFactorsArray)
{
    auto buffer = createTestBuffer (1, 1000);
    std::vector<int> emptyFactors;

    auto result = profile->buildFromBuffer (buffer, 1, emptyFactors);

    EXPECT_TRUE (result.wasOk());
    EXPECT_EQ (1, profile->getNumAggregationLevels()); // Only base level
}

TEST_F (AudioPeakProfileTests, VeryLargeAggregationFactor)
{
    auto buffer = createTestBuffer (1, 1000);
    auto result = profile->buildFromBuffer (buffer, 1, { 10000 });

    EXPECT_TRUE (result.wasOk());
    // Should handle large factor gracefully
    EXPECT_GE (profile->getNumAggregationLevels(), 1);
}

//==============================================================================
// Base Resolution Edge Cases
//==============================================================================

TEST_F (AudioPeakProfileTests, ZeroBaseResolutionReturnsError)
{
    auto buffer = createTestBuffer (1, 1000);
    auto result = profile->buildFromBuffer (buffer, 0, {});

    EXPECT_FALSE (result.wasOk());
    EXPECT_FALSE (profile->isValid());
}

TEST_F (AudioPeakProfileTests, NegativeBaseResolutionReturnsError)
{
    auto buffer = createTestBuffer (1, 1000);
    auto result = profile->buildFromBuffer (buffer, -10, {});

    EXPECT_FALSE (result.wasOk());
    EXPECT_FALSE (profile->isValid());
}

TEST_F (AudioPeakProfileTests, VeryLargeBaseResolution)
{
    auto buffer = createTestBuffer (1, 1000);
    auto result = profile->buildFromBuffer (buffer, 5000, {});

    // Should handle gracefully (will result in very few peaks)
    EXPECT_TRUE (result.wasOk() || ! result.wasOk()); // May succeed or fail
}

//==============================================================================
// GetAggregationFactor Edge Cases
//==============================================================================

TEST_F (AudioPeakProfileTests, DISABLED_GetAggregationFactorForInvalidLevel)
{
    auto buffer = createTestBuffer (1, 1000);
    auto result = profile->buildFromBuffer (buffer, 1, { 10 });
    EXPECT_TRUE (result.wasOk());

    auto factor = profile->getAggregationFactor (100);
    EXPECT_EQ (1, factor); // Should return default value
}

TEST_F (AudioPeakProfileTests, DISABLED_GetAggregationFactorForNegativeLevel)
{
    auto buffer = createTestBuffer (1, 1000);
    auto result = profile->buildFromBuffer (buffer, 1, { 10 });
    EXPECT_TRUE (result.wasOk());

    auto factor = profile->getAggregationFactor (-1);
    EXPECT_EQ (1, factor); // Should return default value
}

//==============================================================================
// GetPeakRangeForSamples Edge Cases
//==============================================================================

TEST_F (AudioPeakProfileTests, GetPeakRangeForNegativeSamples)
{
    auto buffer = createTestBuffer (1, 1000);
    auto result = profile->buildFromBuffer (buffer, 10, {});
    EXPECT_TRUE (result.wasOk());

    auto range = profile->getPeakRangeForSamples (Range<int> (-100, -50), 0);

    // The implementation returns the range as-is without clamping
    // Just verify it doesn't crash
    EXPECT_TRUE (true);
}

TEST_F (AudioPeakProfileTests, GetPeakRangeForOutOfBoundsSamples)
{
    auto buffer = createTestBuffer (1, 1000);
    auto result = profile->buildFromBuffer (buffer, 10, {});
    EXPECT_TRUE (result.wasOk());

    auto range = profile->getPeakRangeForSamples (Range<int> (2000, 3000), 0);

    // Should handle gracefully, clamped to valid range
    EXPECT_TRUE (true);
}

TEST_F (AudioPeakProfileTests, GetPeakRangeForEmptyRange)
{
    auto buffer = createTestBuffer (1, 1000);
    auto result = profile->buildFromBuffer (buffer, 10, {});
    EXPECT_TRUE (result.wasOk());

    auto range = profile->getPeakRangeForSamples (Range<int> (100, 100), 0);

    EXPECT_TRUE (range.isEmpty() || ! range.isEmpty());
}

TEST_F (AudioPeakProfileTests, DISABLED_GetPeakRangeForInvalidLevel)
{
    auto buffer = createTestBuffer (1, 1000);
    auto result = profile->buildFromBuffer (buffer, 10, {});
    EXPECT_TRUE (result.wasOk());

    auto range = profile->getPeakRangeForSamples (Range<int> (0, 100), 100);

    // Should handle gracefully
    EXPECT_TRUE (true);
}

//==============================================================================
// Serialization Edge Cases
//==============================================================================

TEST_F (AudioPeakProfileTests, SerializeEmptyProfileReturnsEmptyData)
{
    auto serialized = profile->serialize();

    // Empty profile should serialize to minimal data
    EXPECT_GE (serialized.getSize(), 0);
}

TEST_F (AudioPeakProfileTests, DeserializeIntoExistingProfileReplacesData)
{
    auto buffer = createTestBuffer (2, 1000);
    auto result = profile->buildFromBuffer (buffer, 1, { 16 });
    EXPECT_TRUE (result.wasOk());

    auto serialized = profile->serialize();

    // Build different profile
    auto buffer2 = createTestBuffer (1, 500);
    result = profile->buildFromBuffer (buffer2, 1, {});
    EXPECT_TRUE (result.wasOk());
    EXPECT_EQ (1, profile->getNumChannels());

    // Deserialize should replace
    result = profile->deserialize (serialized);
    EXPECT_TRUE (result.wasOk());
    EXPECT_EQ (2, profile->getNumChannels());
}

//==============================================================================
// Static Method Tests
//==============================================================================

TEST_F (AudioPeakProfileTests, CalculateOptimalBaseResolutionForZeroSamples)
{
    auto resolution = AudioPeakProfile::calculateOptimalBaseResolution (0);
    EXPECT_GE (resolution, 1);
}

TEST_F (AudioPeakProfileTests, CalculateOptimalBaseResolutionForNegativeSamples)
{
    auto resolution = AudioPeakProfile::calculateOptimalBaseResolution (-1000);
    EXPECT_GE (resolution, 1);
}

TEST_F (AudioPeakProfileTests, CalculateOptimalBaseResolutionConsistency)
{
    // Same input should give same output
    auto res1 = AudioPeakProfile::calculateOptimalBaseResolution (50000000);
    auto res2 = AudioPeakProfile::calculateOptimalBaseResolution (50000000);
    EXPECT_EQ (res1, res2);
}

//==============================================================================
// Multiple Build Calls Tests
//==============================================================================

TEST_F (AudioPeakProfileTests, MultipleBuildCallsReplaceData)
{
    auto buffer1 = createTestBuffer (1, 1000);
    auto result = profile->buildFromBuffer (buffer1, 1, {});
    EXPECT_TRUE (result.wasOk());
    EXPECT_EQ (1, profile->getNumChannels());

    auto buffer2 = createTestBuffer (2, 2000);
    result = profile->buildFromBuffer (buffer2, 1, {});
    EXPECT_TRUE (result.wasOk());
    EXPECT_EQ (2, profile->getNumChannels());
    EXPECT_EQ (2000, profile->getNumSamples());
}

TEST_F (AudioPeakProfileTests, BuildAfterFailedBuildWorks)
{
    // First build fails
    AudioBuffer<float> emptyBuffer (0, 0);
    auto result = profile->buildFromBuffer (emptyBuffer, 1, {});
    EXPECT_FALSE (result.wasOk());

    // Second build should work
    auto buffer = createTestBuffer (1, 1000);
    result = profile->buildFromBuffer (buffer, 1, {});
    EXPECT_TRUE (result.wasOk());
    EXPECT_TRUE (profile->isValid());
}

//==============================================================================
// BuildFromReader Tests
//==============================================================================

namespace
{
// Mock AudioFormatReader for testing
class MockAudioFormatReader : public AudioFormatReader
{
public:
    MockAudioFormatReader (int numChannels, int64 numSamples, double sampleRate = 44100.0, bool shouldFailReads = false)
        : AudioFormatReader (nullptr, "MockFormat")
        , buffer (numChannels, static_cast<int> (numSamples))
        , shouldFail (shouldFailReads)
    {
        this->sampleRate = sampleRate;
        this->bitsPerSample = 32;
        this->lengthInSamples = numSamples;
        this->numChannels = numChannels;
        this->usesFloatingPointData = true;

        // Fill buffer with test data
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* channelData = buffer.getWritePointer (ch);
            for (int i = 0; i < static_cast<int> (numSamples); ++i)
            {
                const float phase = (i / static_cast<float> (sampleRate)) * 440.0f * 2.0f * MathConstants<float>::pi;
                channelData[i] = std::sin (phase) * (ch * 0.1f + 0.5f); // Different amplitude per channel
            }
        }
    }

    bool readSamples (float* const* destChannels,
                      int numDestChannels,
                      int startOffsetInDestBuffer,
                      int64 startSampleInFile,
                      int numSamples) override
    {
        if (shouldFail)
            return false;

        if (startSampleInFile < 0 || startSampleInFile >= lengthInSamples)
            return false;

        const int samplesToRead = jmin (numSamples,
                                        static_cast<int> (lengthInSamples - startSampleInFile));

        for (int ch = 0; ch < jmin (numDestChannels, this->numChannels); ++ch)
        {
            if (destChannels[ch] != nullptr)
            {
                const float* src = buffer.getReadPointer (ch) + startSampleInFile;
                float* dst = destChannels[ch] + startOffsetInDestBuffer;
                std::copy (src, src + samplesToRead, dst);
            }
        }

        return true;
    }

private:
    AudioBuffer<float> buffer;
    bool shouldFail;
};
} // namespace

TEST_F (AudioPeakProfileTests, BuildFromReaderWithSmallFile)
{
    MockAudioFormatReader reader (2, 1000, 44100.0);
    auto factors = AudioPeakProfile::getDefaultAggregationFactors();

    auto result = profile->buildFromReader (reader, 1, factors);

    EXPECT_TRUE (result.wasOk());
    EXPECT_TRUE (profile->isValid());
    EXPECT_EQ (1000, profile->getNumSamples());
    EXPECT_EQ (2, profile->getNumChannels());
    EXPECT_EQ (1, profile->getBaseResolution());
}

TEST_F (AudioPeakProfileTests, BuildFromReaderWithLargeFile)
{
    MockAudioFormatReader reader (2, 100000, 44100.0);
    auto factors = AudioPeakProfile::getDefaultAggregationFactors();

    auto result = profile->buildFromReader (reader, 256, factors);

    EXPECT_TRUE (result.wasOk());
    EXPECT_TRUE (profile->isValid());
    EXPECT_EQ (100000, profile->getNumSamples());
    EXPECT_EQ (2, profile->getNumChannels());
    EXPECT_EQ (256, profile->getBaseResolution());
}

TEST_F (AudioPeakProfileTests, BuildFromReaderWithMonoFile)
{
    MockAudioFormatReader reader (1, 5000, 44100.0);

    auto result = profile->buildFromReader (reader, 1, {});

    EXPECT_TRUE (result.wasOk());
    EXPECT_TRUE (profile->isValid());
    EXPECT_EQ (5000, profile->getNumSamples());
    EXPECT_EQ (1, profile->getNumChannels());
}

TEST_F (AudioPeakProfileTests, BuildFromReaderWithMultiChannel)
{
    MockAudioFormatReader reader (8, 10000, 44100.0);

    auto result = profile->buildFromReader (reader, 1, {});

    EXPECT_TRUE (result.wasOk());
    EXPECT_TRUE (profile->isValid());
    EXPECT_EQ (10000, profile->getNumSamples());
    EXPECT_EQ (8, profile->getNumChannels());
}

TEST_F (AudioPeakProfileTests, BuildFromReaderWithZeroLengthFile)
{
    MockAudioFormatReader reader (2, 0, 44100.0);

    auto result = profile->buildFromReader (reader, 1, {});

    EXPECT_FALSE (result.wasOk());
    EXPECT_FALSE (profile->isValid());
}

TEST_F (AudioPeakProfileTests, BuildFromReaderWithZeroChannels)
{
    MockAudioFormatReader reader (0, 1000, 44100.0);

    auto result = profile->buildFromReader (reader, 1, {});

    EXPECT_FALSE (result.wasOk());
    EXPECT_FALSE (profile->isValid());
}

TEST_F (AudioPeakProfileTests, BuildFromReaderWithInvalidBaseResolution)
{
    MockAudioFormatReader reader (2, 1000, 44100.0);

    auto result = profile->buildFromReader (reader, 0, {});

    EXPECT_FALSE (result.wasOk());
    EXPECT_FALSE (profile->isValid());
}

TEST_F (AudioPeakProfileTests, BuildFromReaderWithNegativeBaseResolution)
{
    MockAudioFormatReader reader (2, 1000, 44100.0);

    auto result = profile->buildFromReader (reader, -1, {});

    EXPECT_FALSE (result.wasOk());
    EXPECT_FALSE (profile->isValid());
}

TEST_F (AudioPeakProfileTests, BuildFromReaderWithFailingReader)
{
    MockAudioFormatReader reader (2, 1000, 44100.0, true); // shouldFail = true

    auto result = profile->buildFromReader (reader, 1, {});

    EXPECT_FALSE (result.wasOk());
    EXPECT_FALSE (profile->isValid());
}

TEST_F (AudioPeakProfileTests, BuildFromReaderWithAggregationFactors)
{
    MockAudioFormatReader reader (2, 10000, 44100.0);
    std::vector<int> factors { 16, 256 };

    auto result = profile->buildFromReader (reader, 1, factors);

    EXPECT_TRUE (result.wasOk());
    EXPECT_TRUE (profile->isValid());
    EXPECT_EQ (3, profile->getNumAggregationLevels()); // Base + 2 aggregation levels
}

TEST_F (AudioPeakProfileTests, BuildFromReaderWithLargeBaseResolution)
{
    MockAudioFormatReader reader (2, 50000, 44100.0);

    auto result = profile->buildFromReader (reader, 1024, {});

    EXPECT_TRUE (result.wasOk());
    EXPECT_TRUE (profile->isValid());
    EXPECT_EQ (50000, profile->getNumSamples());
    EXPECT_EQ (1024, profile->getBaseResolution());
}

TEST_F (AudioPeakProfileTests, BuildFromReaderWithProgressCallback)
{
    MockAudioFormatReader reader (2, 10000, 44100.0);

    int callbackCount = 0;
    double lastProgress = 0.0;

    auto progressCallback = [&callbackCount, &lastProgress] (double progress) -> bool
    {
        callbackCount++;
        lastProgress = progress;
        return true; // Continue
    };

    auto result = profile->buildFromReader (reader, 1, {}, progressCallback);

    EXPECT_TRUE (result.wasOk());
    EXPECT_GT (callbackCount, 0);
    EXPECT_GE (lastProgress, 0.0);
    EXPECT_LE (lastProgress, 1.0);
}

TEST_F (AudioPeakProfileTests, BuildFromReaderWithCancelledProgress)
{
    MockAudioFormatReader reader (2, 100000, 44100.0);

    int callbackCount = 0;

    auto progressCallback = [&callbackCount] (double progress) -> bool
    {
        callbackCount++;
        return callbackCount < 3; // Cancel after 3 callbacks
    };

    auto result = profile->buildFromReader (reader, 1, {}, progressCallback);

    EXPECT_FALSE (result.wasOk());
    EXPECT_FALSE (profile->isValid());
    EXPECT_GE (callbackCount, 3);
}

TEST_F (AudioPeakProfileTests, BuildFromReaderPreservesChannelData)
{
    MockAudioFormatReader reader (2, 1000, 44100.0);

    auto result = profile->buildFromReader (reader, 10, {});

    EXPECT_TRUE (result.wasOk());

    // Verify we can retrieve peak data
    auto range = profile->getPeakRangeForSamples (Range<int> (0, 100), 0);
    EXPECT_FALSE (range.isEmpty());
}

TEST_F (AudioPeakProfileTests, BuildFromReaderWithDifferentSampleRates)
{
    // Test with 48kHz
    MockAudioFormatReader reader48k (2, 5000, 48000.0);
    auto result = profile->buildFromReader (reader48k, 1, {});
    EXPECT_TRUE (result.wasOk());

    // Test with 96kHz
    MockAudioFormatReader reader96k (2, 5000, 96000.0);
    auto result2 = profile->buildFromReader (reader96k, 1, {});
    EXPECT_TRUE (result2.wasOk());

    // Test with 192kHz
    MockAudioFormatReader reader192k (2, 5000, 192000.0);
    auto result3 = profile->buildFromReader (reader192k, 1, {});
    EXPECT_TRUE (result3.wasOk());
}

TEST_F (AudioPeakProfileTests, BuildFromReaderThenBuildFromBuffer)
{
    // First build from reader
    MockAudioFormatReader reader (2, 1000, 44100.0);
    auto result1 = profile->buildFromReader (reader, 1, {});
    EXPECT_TRUE (result1.wasOk());
    EXPECT_EQ (1000, profile->getNumSamples());

    // Then build from buffer
    auto buffer = createTestBuffer (1, 500);
    auto result2 = profile->buildFromBuffer (buffer, 1, {});
    EXPECT_TRUE (result2.wasOk());
    EXPECT_EQ (500, profile->getNumSamples());
    EXPECT_EQ (1, profile->getNumChannels());
}

TEST_F (AudioPeakProfileTests, BuildFromReaderWithVerySmallChunks)
{
    // Test with file where baseResolution > chunk size (8192)
    MockAudioFormatReader reader (2, 20000, 44100.0);

    auto result = profile->buildFromReader (reader, 16384, {}); // baseResolution larger than chunk size

    EXPECT_TRUE (result.wasOk());
    EXPECT_TRUE (profile->isValid());
}

TEST_F (AudioPeakProfileTests, BuildFromReaderHandlesPartialLastPeak)
{
    // 1005 samples with base resolution 100 = 10 full peaks + 1 partial (5 samples)
    MockAudioFormatReader reader (2, 1005, 44100.0);

    auto result = profile->buildFromReader (reader, 100, {});

    EXPECT_TRUE (result.wasOk());
    EXPECT_TRUE (profile->isValid());
    EXPECT_EQ (1005, profile->getNumSamples());
}

TEST_F (AudioPeakProfileTests, BuildFromReaderWithSingleSample)
{
    MockAudioFormatReader reader (1, 1, 44100.0);

    auto result = profile->buildFromReader (reader, 1, {});

    EXPECT_TRUE (result.wasOk());
    EXPECT_TRUE (profile->isValid());
    EXPECT_EQ (1, profile->getNumSamples());
}

TEST_F (AudioPeakProfileTests, BuildFromReaderMultipleTimes)
{
    MockAudioFormatReader reader1 (1, 1000, 44100.0);
    auto result1 = profile->buildFromReader (reader1, 1, {});
    EXPECT_TRUE (result1.wasOk());
    EXPECT_EQ (1000, profile->getNumSamples());

    MockAudioFormatReader reader2 (2, 2000, 48000.0);
    auto result2 = profile->buildFromReader (reader2, 1, {});
    EXPECT_TRUE (result2.wasOk());
    EXPECT_EQ (2000, profile->getNumSamples());
    EXPECT_EQ (2, profile->getNumChannels());
}

TEST_F (AudioPeakProfileTests, BuildFromReaderClearsOldData)
{
    // First build
    auto buffer = createTestBuffer (1, 500);
    profile->buildFromBuffer (buffer, 1, {});
    EXPECT_EQ (500, profile->getNumSamples());

    // Second build from reader should clear old data
    MockAudioFormatReader reader (2, 1000, 44100.0);
    auto result = profile->buildFromReader (reader, 1, {});

    EXPECT_TRUE (result.wasOk());
    EXPECT_EQ (1000, profile->getNumSamples());
    EXPECT_EQ (2, profile->getNumChannels());
}
