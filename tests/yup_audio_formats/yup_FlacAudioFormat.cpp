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

#include <yup_audio_formats/yup_audio_formats.h>

#include "yup_AudioFormatTools.h"

#include <cmath>
#include <gtest/gtest.h>

using namespace yup;

namespace
{
constexpr int kTestSampleRate = 44100;
constexpr int kTestChannels = 2;
constexpr int kTestBitsPerSample = 16;
constexpr int kTestNumSamples = 512;

const std::vector<String> getAllFlacTestFiles()
{
    return {
        "M1F1-int16.flac",
        "M1F1-int24.flac",
        "M1F1-int32.flac"
        "M1F1-uint8.flac"
    };
}

static void fillTestTone (std::vector<float>& left, std::vector<float>& right)
{
    left.resize ((size_t) kTestNumSamples);
    right.resize ((size_t) kTestNumSamples);

    constexpr double frequency = 440.0;
    constexpr double twoPi = 6.2831853071795864769;

    for (int i = 0; i < kTestNumSamples; ++i)
    {
        const double phase = twoPi * frequency * ((double) i / (double) kTestSampleRate);
        const float sample = (float) std::sin (phase) * 0.5f;
        left[(size_t) i] = sample;
        right[(size_t) i] = sample;
    }
}
} // namespace

class FlacAudioFormatTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        format = std::make_unique<FlacAudioFormat>();
    }

    std::unique_ptr<FlacAudioFormat> format;
};

TEST_F (FlacAudioFormatTests, GetFormatNameReturnsFlac)
{
    const String& name = format->getFormatName();
    EXPECT_FALSE (name.isEmpty());
    EXPECT_TRUE (name.containsIgnoreCase ("flac"));
}

TEST_F (FlacAudioFormatTests, GetFileExtensionsIncludesFlac)
{
    Array<String> extensions = format->getFileExtensions();
    EXPECT_FALSE (extensions.isEmpty());

    bool foundFlac = false;
    for (const auto& ext : extensions)
    {
        if (ext.equalsIgnoreCase (".flac") || ext.equalsIgnoreCase ("flac"))
        {
            foundFlac = true;
            break;
        }
    }
    EXPECT_TRUE (foundFlac);
}

TEST_F (FlacAudioFormatTests, GetPossibleBitDepthsAndSampleRates)
{
    Array<int> bitDepths = format->getPossibleBitDepths();
    Array<int> sampleRates = format->getPossibleSampleRates();

    EXPECT_FALSE (bitDepths.isEmpty());
    EXPECT_FALSE (sampleRates.isEmpty());
    EXPECT_TRUE (bitDepths.contains (kTestBitsPerSample));
    EXPECT_TRUE (sampleRates.contains (kTestSampleRate));
}

TEST_F (FlacAudioFormatTests, CanDoMonoAndStereo)
{
    EXPECT_TRUE (format->canDoMono());
    EXPECT_TRUE (format->canDoStereo());
}

TEST_F (FlacAudioFormatTests, IsCompressed)
{
    EXPECT_TRUE (format->isCompressed());
}

TEST_F (FlacAudioFormatTests, CreateReaderForNullStream)
{
    auto reader = format->createReaderFor (nullptr);
    EXPECT_EQ (nullptr, reader);
}

TEST_F (FlacAudioFormatTests, CreateWriterForNullStream)
{
    auto writer = format->createWriterFor (nullptr, kTestSampleRate, kTestChannels, kTestBitsPerSample, {}, 5);
    EXPECT_EQ (nullptr, writer);
}

#if ! YUP_EMSCRIPTEN
class FlacAudioFormatFileTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        format = std::make_unique<FlacAudioFormat>();
        testDataDir = File (__FILE__)
                          .getParentDirectory()
                          .getParentDirectory()
                          .getChildFile ("data")
                          .getChildFile ("sounds");
    }

    std::unique_ptr<FlacAudioFormat> format;
    File testDataDir;
};

TEST_F (FlacAudioFormatFileTests, TestAllFlacFilesCanBeOpened)
{
    auto flacFiles = getAllFlacTestFiles();

    for (const auto& filename : flacFiles)
    {
        File flacFile = testDataDir.getChildFile (filename);

        if (! flacFile.exists())
        {
            FAIL() << "Test file does not exist: " << filename.toRawUTF8();
            continue;
        }

        std::unique_ptr<FileInputStream> inputStream = std::make_unique<FileInputStream> (flacFile);
        if (! inputStream->openedOk())
        {
            FAIL() << "Could not open file stream for: " << filename.toRawUTF8();
            continue;
        }

        auto reader = format->createReaderFor (inputStream.get());
        if (reader == nullptr)
        {
            inputStream.release();
            FAIL() << "Could not create reader for: " << filename.toRawUTF8();
            continue;
        }

        EXPECT_GT (reader->sampleRate, 0.0) << "Unexpected sample rate for: " << filename.toRawUTF8();
        EXPECT_GT (reader->numChannels, 0) << "Invalid channel count for: " << filename.toRawUTF8();
        EXPECT_GE (reader->lengthInSamples, 0) << "Invalid length for: " << filename.toRawUTF8();
        EXPECT_GT (reader->bitsPerSample, 0) << "Invalid bit depth for: " << filename.toRawUTF8();

        if (reader->lengthInSamples > 0)
        {
            const int samplesToRead = static_cast<int> (std::min (reader->lengthInSamples, static_cast<int64> (1024)));
            AudioBuffer<float> buffer (static_cast<int> (reader->numChannels), samplesToRead);

            bool readSuccess = reader->read (&buffer, 0, samplesToRead, 0, true, true);
            EXPECT_TRUE (readSuccess) << "Failed to read samples from: " << filename.toRawUTF8();
        }

        inputStream.release();
    }
}
#endif

TEST_F (FlacAudioFormatTests, WriteAndReadBackMemoryStream)
{
    MemoryBlock outputBlock;
    auto* outputStream = new MemoryOutputStream (outputBlock, false);
    auto writer = format->createWriterFor (outputStream,
                                           kTestSampleRate,
                                           kTestChannels,
                                           kTestBitsPerSample,
                                           {},
                                           5);
    ASSERT_NE (nullptr, writer);

    std::vector<float> left;
    std::vector<float> right;
    fillTestTone (left, right);

    const float* channels[] = { left.data(), right.data() };
    EXPECT_TRUE (writer->write (channels, kTestNumSamples));
    EXPECT_TRUE (writer->flush());
    writer.reset();

    MemoryInputStream* inputStream = new MemoryInputStream (outputBlock, true);

    auto reader = format->createReaderFor (inputStream);
    ASSERT_NE (nullptr, reader);

    EXPECT_EQ ((double) kTestSampleRate, reader->sampleRate);
    EXPECT_EQ (kTestChannels, reader->numChannels);
    EXPECT_GT (reader->lengthInSamples, 0);

    const int samplesToRead = (int) jmin<int64> (reader->lengthInSamples, kTestNumSamples);
    AudioBuffer<float> buffer (reader->numChannels, samplesToRead);
    EXPECT_TRUE (reader->read (&buffer, 0, samplesToRead, 0, true, true));

    const float tolerance = 1.0f / 32768.0f + 1.0e-6f;
    for (int ch = 0; ch < reader->numChannels; ++ch)
    {
        const float* source = ch == 0 ? left.data() : right.data();
        const float* decoded = buffer.getReadPointer (ch);

        for (int i = 0; i < samplesToRead; ++i)
            EXPECT_NEAR (source[i], decoded[i], tolerance);
    }
}
