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

#include <yup_audio_formats/yup_audio_formats.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>

using namespace yup;

namespace
{
class MockAudioFormatReader : public AudioFormatReader
{
public:
    MockAudioFormatReader (int numChannelsToUse, int numSamples)
        : AudioFormatReader (nullptr, "MockAudioFormatReader")
        , buffer (numChannelsToUse, numSamples)
    {
        sampleRate = 48000.0;
        bitsPerSample = 32;
        lengthInSamples = numSamples;
        numChannels = numChannelsToUse;
        usesFloatingPointData = true;
    }

    bool readSamples (float* const* destChannels,
                      int numDestChannels,
                      int startOffsetInDestBuffer,
                      int64 startSampleInFile,
                      int numSamples) override
    {
        if (startSampleInFile < 0 || startSampleInFile + numSamples > lengthInSamples)
            return false;

        const auto channelsToCopy = jmin (numDestChannels, numChannels);

        for (int ch = 0; ch < channelsToCopy; ++ch)
        {
            if (destChannels[ch] == nullptr)
                continue;

            const auto* source = buffer.getReadPointer (ch) + startSampleInFile;
            auto* destination = destChannels[ch] + startOffsetInDestBuffer;
            std::copy (source, source + numSamples, destination);
        }

        return true;
    }

    AudioBuffer<float> buffer;
};

static void fillChannel (AudioBuffer<float>& buffer, int channel, std::initializer_list<float> values)
{
    auto* data = buffer.getWritePointer (channel);

    int index = 0;
    for (float value : values)
        data[index++] = value;
}
} // namespace

TEST (AudioFormatReaderTests, ReadCopiesMonoSourceToAllDestinationChannels)
{
    MockAudioFormatReader reader (1, 4);
    fillChannel (reader.buffer, 0, { 0.1f, -0.25f, 0.75f, 0.0f });

    AudioBuffer<float> destination (3, 4);
    destination.clear();

    EXPECT_TRUE (reader.read (&destination, 0, 4, 0, true, false));

    for (int channel = 0; channel < destination.getNumChannels(); ++channel)
    {
        for (int sample = 0; sample < destination.getNumSamples(); ++sample)
            EXPECT_FLOAT_EQ (reader.buffer.getSample (0, sample), destination.getSample (channel, sample));
    }
}

TEST (AudioFormatReaderTests, ReadClearsDestinationWhenNoChannelsAreRequested)
{
    MockAudioFormatReader reader (2, 4);
    fillChannel (reader.buffer, 0, { 0.1f, -0.25f, 0.75f, 0.0f });
    fillChannel (reader.buffer, 1, { -0.8f, 0.5f, 0.2f, -0.1f });

    AudioBuffer<float> destination (2, 4);
    destination.fill (1.0f);

    EXPECT_TRUE (reader.read (&destination, 0, 4, 0, false, false));

    for (int channel = 0; channel < destination.getNumChannels(); ++channel)
    {
        for (int sample = 0; sample < destination.getNumSamples(); ++sample)
            EXPECT_FLOAT_EQ (0.0f, destination.getSample (channel, sample));
    }
}

TEST (AudioFormatReaderTests, ReadMaxLevelsReturnsChannelExtremes)
{
    MockAudioFormatReader reader (2, 4);
    fillChannel (reader.buffer, 0, { 0.1f, -0.4f, 0.25f, 0.05f });
    fillChannel (reader.buffer, 1, { -0.9f, 0.2f, 0.6f, -0.1f });

    Range<float> levels[2];
    reader.readMaxLevels (0, 4, levels, 2);

    EXPECT_NEAR (-0.4f, levels[0].getStart(), 1.0e-6f);
    EXPECT_NEAR (0.25f, levels[0].getEnd(), 1.0e-6f);
    EXPECT_NEAR (-0.9f, levels[1].getStart(), 1.0e-6f);
    EXPECT_NEAR (0.6f, levels[1].getEnd(), 1.0e-6f);
}

TEST (AudioFormatReaderTests, SearchForLevelFindsFirstMatchingRun)
{
    MockAudioFormatReader reader (2, 5);
    fillChannel (reader.buffer, 0, { 0.1f, 0.55f, -0.6f, 0.2f, 0.1f });
    fillChannel (reader.buffer, 1, { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f });

    EXPECT_EQ (1, reader.searchForLevel (0, 5, 0.5, 0.7, 2));
    EXPECT_EQ (-1, reader.searchForLevel (0, 5, 0.8, 0.9, 1));
}

TEST (AudioFormatReaderTests, GetChannelLayoutMatchesChannelCount)
{
    MockAudioFormatReader monoReader (1, 1);
    MockAudioFormatReader stereoReader (2, 1);
    MockAudioFormatReader surroundReader (4, 1);

    EXPECT_EQ (AudioChannelSet::mono(), monoReader.getChannelLayout());
    EXPECT_EQ (AudioChannelSet::stereo(), stereoReader.getChannelLayout());
    EXPECT_EQ (AudioChannelSet::discreteChannels (4), surroundReader.getChannelLayout());
}

TEST (AudioFormatReaderTests, MetadataAttributesAreAccessible)
{
    MockAudioFormatReader reader (2, 100);

    EXPECT_DOUBLE_EQ (48000.0, reader.sampleRate);
    EXPECT_EQ (32u, reader.bitsPerSample);
    EXPECT_EQ (100, (int) reader.lengthInSamples);
    EXPECT_EQ (2u, reader.numChannels);
    EXPECT_TRUE (reader.usesFloatingPointData);
}

TEST (AudioFormatReaderTests, ReadWithStartOffsetInDestinationBuffer)
{
    MockAudioFormatReader reader (1, 3);
    fillChannel (reader.buffer, 0, { 0.1f, 0.2f, 0.3f });

    AudioBuffer<float> destination (1, 6);
    destination.clear();

    // Read 3 samples starting at dest offset 2
    EXPECT_TRUE (reader.read (&destination, 2, 3, 0, true, false));

    EXPECT_FLOAT_EQ (destination.getSample (0, 0), 0.0f); // before offset untouched
    EXPECT_FLOAT_EQ (destination.getSample (0, 1), 0.0f);
    EXPECT_FLOAT_EQ (destination.getSample (0, 2), 0.1f);
    EXPECT_FLOAT_EQ (destination.getSample (0, 3), 0.2f);
    EXPECT_FLOAT_EQ (destination.getSample (0, 4), 0.3f);
    EXPECT_FLOAT_EQ (destination.getSample (0, 5), 0.0f);
}

TEST (AudioFormatReaderTests, ReadOutOfRangeReturnsFalse)
{
    MockAudioFormatReader reader (1, 4);
    fillChannel (reader.buffer, 0, { 0.1f, 0.2f, 0.3f, 0.4f });

    AudioBuffer<float> destination (1, 4);
    destination.clear();

    // Request samples beyond length
    EXPECT_FALSE (reader.read (&destination, 0, 4, 2, true, false));
}
