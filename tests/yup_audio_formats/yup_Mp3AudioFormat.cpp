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
const std::vector<String> getAllMp3TestFiles()
{
    return {
        "M1F1-int16.mp3",
        "M1F1-int24.mp3",
        "M1F1-uint8.mp3"
    };
}
} // namespace

class Mp3AudioFormatTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        format = std::make_unique<Mp3AudioFormat>();
    }

    std::unique_ptr<Mp3AudioFormat> format;
};

TEST_F (Mp3AudioFormatTests, GetFormatNameReturnsMp3)
{
    const String& name = format->getFormatName();
    EXPECT_FALSE (name.isEmpty());
    EXPECT_TRUE (name.containsIgnoreCase ("mp3"));
}

TEST_F (Mp3AudioFormatTests, GetFileExtensionsIncludesMp3)
{
    {
        Array<String> extensions = format->getFileExtensions (AudioFormat::forReading);
        EXPECT_FALSE (extensions.isEmpty());

        bool foundMp3 = false;
        for (const auto& ext : extensions)
        {
            if (ext.equalsIgnoreCase (".mp3") || ext.equalsIgnoreCase ("mp3"))
            {
                foundMp3 = true;
                break;
            }
        }
        EXPECT_TRUE (foundMp3);
    }

    {
        Array<String> extensions = format->getFileExtensions (AudioFormat::forWriting);
        EXPECT_FALSE (extensions.isEmpty());

        bool foundMp3 = false;
        for (const auto& ext : extensions)
        {
            if (ext.equalsIgnoreCase (".mp3") || ext.equalsIgnoreCase ("mp3"))
            {
                foundMp3 = true;
                break;
            }
        }
        EXPECT_TRUE (foundMp3);
    }
}

TEST_F (Mp3AudioFormatTests, GetPossibleBitDepthsAndSampleRates)
{
    Array<int> bitDepths = format->getPossibleBitDepths();
    Array<int> sampleRates = format->getPossibleSampleRates();

    EXPECT_FALSE (bitDepths.isEmpty());
    EXPECT_FALSE (sampleRates.isEmpty());
    EXPECT_TRUE (bitDepths.contains (16));
    EXPECT_TRUE (sampleRates.contains (44100));
    EXPECT_TRUE (sampleRates.contains (48000));
}

TEST_F (Mp3AudioFormatTests, CanDoMonoAndStereo)
{
    EXPECT_TRUE (format->canDoMono());
    EXPECT_TRUE (format->canDoStereo());
}

TEST_F (Mp3AudioFormatTests, IsCompressed)
{
    EXPECT_TRUE (format->isCompressed());
}

TEST_F (Mp3AudioFormatTests, CreateReaderForNullStream)
{
    auto reader = format->createReaderFor (nullptr);
    EXPECT_EQ (nullptr, reader);
}

TEST_F (Mp3AudioFormatTests, CreateWriterForNullStream)
{
    auto writer = format->createWriterFor (nullptr, 44100, 2, 16, {}, 0);
    EXPECT_EQ (nullptr, writer);
}

#if ! YUP_EMSCRIPTEN
class Mp3AudioFormatFileTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        format = std::make_unique<Mp3AudioFormat>();
        testDataDir = File (__FILE__)
                          .getParentDirectory()
                          .getParentDirectory()
                          .getChildFile ("data")
                          .getChildFile ("sounds");
    }

    std::unique_ptr<Mp3AudioFormat> format;
    File testDataDir;
};

TEST_F (Mp3AudioFormatFileTests, TestAllMp3FilesCanBeOpened)
{
    auto mp3Files = getAllMp3TestFiles();

    for (const auto& filename : mp3Files)
    {
        File mp3File = testDataDir.getChildFile (filename);
        if (! mp3File.exists())
        {
            FAIL() << "Test file does not exist: " << filename.toRawUTF8();
            continue;
        }

        std::unique_ptr<FileInputStream> inputStream = std::make_unique<FileInputStream> (mp3File);
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

        EXPECT_GT (reader->sampleRate, 0.0) << "Invalid sample rate for: " << filename.toRawUTF8();
        EXPECT_GT (reader->numChannels, 0) << "Invalid channel count for: " << filename.toRawUTF8();
        EXPECT_GE (reader->lengthInSamples, 0) << "Invalid length for: " << filename.toRawUTF8();
        EXPECT_EQ (16, reader->bitsPerSample) << "Unexpected bit depth for: " << filename.toRawUTF8();

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

TEST_F (Mp3AudioFormatFileTests, TestMp3FilesHaveValidData)
{
    auto mp3Files = getAllMp3TestFiles();

    for (const auto& filename : mp3Files)
    {
        File mp3File = testDataDir.getChildFile (filename);

        if (! mp3File.exists())
        {
            FAIL() << "Test file does not exist: " << filename.toRawUTF8();
            continue;
        }

        std::unique_ptr<FileInputStream> inputStream = std::make_unique<FileInputStream> (mp3File);
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

        auto validationResult = validateAudioData (*reader);

        EXPECT_FALSE (validationResult.hasClippedSamples)
            << "File " << filename.toRawUTF8() << " contains "
            << validationResult.clippedSampleCount << " samples exceeding ±1.0 (peak: "
            << validationResult.maxAbsValue << ")";

        EXPECT_FALSE (validationResult.hasExtremeValues)
            << "File " << filename.toRawUTF8() << " contains "
            << validationResult.extremeValueCount << " extreme values (peak: "
            << validationResult.maxAbsValue << ")";

        EXPECT_LE (validationResult.maxAbsValue, 1.5f)
            << "File " << filename.toRawUTF8() << " has maximum absolute value of "
            << validationResult.maxAbsValue << " which seems unusually high";

        EXPECT_GE (validationResult.minValue, -1.5f)
            << "File " << filename.toRawUTF8() << " has minimum value of "
            << validationResult.minValue << " which seems unusually low";

        EXPECT_LE (validationResult.maxValue, 1.5f)
            << "File " << filename.toRawUTF8() << " has maximum value of "
            << validationResult.maxValue << " which seems unusually high";

        inputStream.release();
    }
}

TEST_F (Mp3AudioFormatFileTests, TestSeeking)
{
    auto mp3Files = getAllMp3TestFiles();

    for (const auto& filename : mp3Files)
    {
        File mp3File = testDataDir.getChildFile (filename);
        if (! mp3File.exists())
            continue; // Skip missing files

        std::unique_ptr<FileInputStream> inputStream = std::make_unique<FileInputStream> (mp3File);
        if (! inputStream->openedOk())
            continue; // Skip files that can't be opened

        auto reader = format->createReaderFor (inputStream.get());
        if (reader == nullptr)
        {
            inputStream.release();
            continue; // Skip files that can't be read
        }

        if (reader->lengthInSamples < 2000)
        {
            inputStream.release();
            continue; // Skip very short files
        }

        const int64 safeEnd = reader->lengthInSamples - 1000;
        const int64 testPositions[] = { 0, reader->lengthInSamples / 4, reader->lengthInSamples / 2, safeEnd };

        for (auto pos : testPositions)
        {
            if (pos < 0 || pos >= reader->lengthInSamples)
                continue;

            if (pos < 0 || pos >= reader->lengthInSamples)
                continue;

            const int samplesToRead = (int) jmin<int64> (100, reader->lengthInSamples - pos);
            if (samplesToRead <= 0)
                continue;

            AudioBuffer<float> buffer (static_cast<int> (reader->numChannels), samplesToRead);
            bool readSuccess = reader->read (&buffer, 0, samplesToRead, pos, true, true);
            EXPECT_TRUE (readSuccess) << "Failed to read at position " << pos << " in file: " << filename.toRawUTF8();
        }

        inputStream.release();
    }
}

TEST_F (Mp3AudioFormatFileTests, TestMetadataExtraction)
{
    auto mp3Files = getAllMp3TestFiles();

    for (const auto& filename : mp3Files)
    {
        File mp3File = testDataDir.getChildFile (filename);
        if (! mp3File.exists())
            continue; // Skip missing files

        std::unique_ptr<FileInputStream> inputStream = std::make_unique<FileInputStream> (mp3File);
        if (! inputStream->openedOk())
            continue; // Skip files that can't be opened

        auto reader = format->createReaderFor (inputStream.get());
        if (reader == nullptr)
        {
            inputStream.release();
            continue; // Skip files that can't be read
        }

        // MP3 files may or may not have metadata, so we just check that the metadataValues is accessible
        EXPECT_NO_THROW (reader->metadataValues.size());

        inputStream.release();
    }
}
#endif
