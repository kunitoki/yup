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
#include <bit>
#include <array>
#include <cstdint>

using namespace yup;

namespace
{
class WriterTestAudioFormatReader : public AudioFormatReader
{
public:
    WriterTestAudioFormatReader (int numChannelsToUse, int numSamples)
        : AudioFormatReader (nullptr, "MockAudioFormatReader")
        , buffer (numChannelsToUse, numSamples)
    {
        sampleRate = 44100.0;
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

class MockAudioFormatWriter : public AudioFormatWriter
{
public:
    MockAudioFormatWriter (int numChannelsToUse, int bitsPerSampleToUse, double sampleRateToUse = 44100.0)
        : AudioFormatWriter (nullptr, "MockAudioFormatWriter", sampleRateToUse, numChannelsToUse, bitsPerSampleToUse)
    {
    }

    bool write (const float* const* samplesToWrite, int numSamples) override
    {
        if (writtenSamples.size() != static_cast<size_t> (getNumChannels()))
            writtenSamples.resize (static_cast<size_t> (getNumChannels()));

        for (int channel = 0; channel < getNumChannels(); ++channel)
            writtenSamples[static_cast<size_t> (channel)].insert (writtenSamples[static_cast<size_t> (channel)].end(),
                                                                  samplesToWrite[channel],
                                                                  samplesToWrite[channel] + numSamples);

        ++writeCalls;
        lastNumSamples = numSamples;
        return true;
    }

    void reset()
    {
        writeCalls = 0;
        lastNumSamples = 0;
        writtenSamples.clear();
    }

    int writeCalls = 0;
    int lastNumSamples = 0;
    std::vector<std::vector<float>> writtenSamples;
};

static void fillWriterChannel (AudioBuffer<float>& buffer, int channel, std::initializer_list<float> values)
{
    auto* data = buffer.getWritePointer (channel);

    int index = 0;
    for (float value : values)
        data[index++] = value;
}

static void expectFloatArrayEquals (const std::vector<float>& actual, std::initializer_list<float> expected)
{
    ASSERT_EQ (expected.size(), actual.size());

    size_t index = 0;
    for (float value : expected)
    {
        EXPECT_FLOAT_EQ (value, actual[index]);
        ++index;
    }
}
} // namespace

TEST (AudioFormatWriterTests, WriteFromAudioSampleBufferCopiesAvailableChannels)
{
    MockAudioFormatWriter writer (3, 16);

    AudioBuffer<float> source (2, 4);
    fillWriterChannel (source, 0, { 0.1f, -0.2f, 0.3f, -0.4f });
    fillWriterChannel (source, 1, { 0.5f, 0.6f, -0.7f, 0.8f });

    EXPECT_TRUE (writer.writeFromAudioSampleBuffer (source, 1, 10));

    EXPECT_EQ (1, writer.writeCalls);
    EXPECT_EQ (3, writer.lastNumSamples);
    ASSERT_EQ (3u, writer.writtenSamples.size());

    expectFloatArrayEquals (writer.writtenSamples[0], { -0.2f, 0.3f, -0.4f });
    expectFloatArrayEquals (writer.writtenSamples[1], { 0.6f, -0.7f, 0.8f });
    expectFloatArrayEquals (writer.writtenSamples[2], { 0.0f, 0.0f, 0.0f });
}

TEST (AudioFormatWriterTests, WriteFromAudioReaderUsesReaderSamples)
{
    WriterTestAudioFormatReader reader (2, 5);
    fillWriterChannel (reader.buffer, 0, { 0.1f, -0.2f, 0.3f, -0.4f, 0.5f });
    fillWriterChannel (reader.buffer, 1, { -0.5f, 0.4f, -0.3f, 0.2f, -0.1f });

    MockAudioFormatWriter writer (2, 16);

    EXPECT_TRUE (writer.writeFromAudioReader (reader, 1, 3));

    EXPECT_EQ (1, writer.writeCalls);
    EXPECT_EQ (3, writer.lastNumSamples);
    ASSERT_EQ (2u, writer.writtenSamples.size());
    EXPECT_EQ (3u, writer.writtenSamples[0].size());
    EXPECT_EQ (3u, writer.writtenSamples[1].size());
}

TEST (AudioFormatWriterTests, WriteHelperEncodesIntegerAndFloatFormats)
{
    const std::array<float, 4> sourceValues { -1.0f, -0.5f, 0.0f, 1.0f };

    {
        std::array<std::uint8_t, sourceValues.size()> destination {};
        AudioFormatWriter::WriteHelper::writeInt8 (destination.data(), sourceValues.data(), static_cast<int> (sourceValues.size()));

        EXPECT_EQ (1, destination[0]);
        EXPECT_EQ (65, destination[1]);
        EXPECT_EQ (128, destination[2]);
        EXPECT_EQ (255, destination[3]);
    }

    {
        std::array<std::uint16_t, 2> destination {};
        const std::array<float, 2> values { 0.25f, 1.0f };

        AudioFormatWriter::WriteHelper::writeInt16 (destination.data(), values.data(), static_cast<int> (values.size()), true);

        EXPECT_EQ (ByteOrder::swapIfBigEndian (static_cast<std::uint16_t> (static_cast<int> (values[0] * 32767.0f))), destination[0]);
        EXPECT_EQ (ByteOrder::swapIfBigEndian (static_cast<std::uint16_t> (static_cast<int> (values[1] * 32767.0f))), destination[1]);
    }

    {
        std::array<std::uint8_t, 6> destination {};
        const std::array<float, 2> values { 0.25f, 1.0f };

        AudioFormatWriter::WriteHelper::writeInt24 (destination.data(), values.data(), static_cast<int> (values.size()), true);

        EXPECT_EQ (0xFF, destination[0]);
        EXPECT_EQ (0xFF, destination[1]);
        EXPECT_EQ (0x1F, destination[2]);
        EXPECT_EQ (0xFF, destination[3]);
        EXPECT_EQ (0xFF, destination[4]);
        EXPECT_EQ (0x7F, destination[5]);
    }

    {
        std::array<std::uint32_t, 1> destination {};
        const std::array<float, 1> values { 0.25f };

        AudioFormatWriter::WriteHelper::writeInt32 (destination.data(), values.data(), static_cast<int> (values.size()), true);

        EXPECT_EQ (ByteOrder::swapIfBigEndian (static_cast<std::uint32_t> (static_cast<int> (values[0] * 2147483647.0f))), destination[0]);
    }

    {
        std::array<float, 2> destination {};
        const std::array<float, 2> values { 0.125f, -0.75f };

        AudioFormatWriter::WriteHelper::writeFloat32 (destination.data(), values.data(), static_cast<int> (values.size()), true);

        EXPECT_EQ (std::bit_cast<std::uint32_t> (ByteOrder::swapIfBigEndian (values[0])), std::bit_cast<std::uint32_t> (destination[0]));
        EXPECT_EQ (std::bit_cast<std::uint32_t> (ByteOrder::swapIfBigEndian (values[1])), std::bit_cast<std::uint32_t> (destination[1]));
    }

    {
        std::array<double, 2> destination {};
        const std::array<double, 2> values { 0.125, -0.75 };

        AudioFormatWriter::WriteHelper::writeFloat64 (destination.data(), values.data(), static_cast<int> (values.size()), true);

        EXPECT_EQ (std::bit_cast<std::uint64_t> (ByteOrder::swapIfBigEndian (values[0])), std::bit_cast<std::uint64_t> (destination[0]));
        EXPECT_EQ (std::bit_cast<std::uint64_t> (ByteOrder::swapIfBigEndian (values[1])), std::bit_cast<std::uint64_t> (destination[1]));
    }
}