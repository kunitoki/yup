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
const std::vector<String> getAllOpusTestFiles()
{
    return {
        "M1F1-float32-vbr.opus",
        "M1F1-float32.opus",
        "M1F1-int16-vbr.opus",
        "M1F1-int16.opus",
        "M1F1-int24-vbr.opus",
        "M1F1-int24.opus",
        "M1F1-uint8-vbr.opus",
        "M1F1-uint8.opus"
    };
}
} // namespace

class OpusAudioFormatTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        format = std::make_unique<OpusAudioFormat>();
    }

    std::unique_ptr<OpusAudioFormat> format;
};

TEST_F (OpusAudioFormatTests, GetFormatNameReturnsOpus)
{
    const String& name = format->getFormatName();
    EXPECT_FALSE (name.isEmpty());
    EXPECT_TRUE (name.containsIgnoreCase ("opus"));
}

TEST_F (OpusAudioFormatTests, GetFileExtensionsIncludesOpus)
{
    Array<String> extensions = format->getFileExtensions();
    EXPECT_FALSE (extensions.isEmpty());

    bool foundOpus = false;
    for (const auto& ext : extensions)
    {
        if (ext.equalsIgnoreCase (".opus") || ext.equalsIgnoreCase ("opus"))
        {
            foundOpus = true;
            break;
        }
    }
    EXPECT_TRUE (foundOpus);
}

TEST_F (OpusAudioFormatTests, GetPossibleBitDepthsAndSampleRates)
{
    Array<int> bitDepths = format->getPossibleBitDepths();
    Array<int> sampleRates = format->getPossibleSampleRates();

    EXPECT_FALSE (bitDepths.isEmpty());
    EXPECT_FALSE (sampleRates.isEmpty());
    EXPECT_TRUE (bitDepths.contains (32));
    EXPECT_TRUE (sampleRates.contains (48000));
}

TEST_F (OpusAudioFormatTests, CanDoMonoAndStereo)
{
    EXPECT_TRUE (format->canDoMono());
    EXPECT_TRUE (format->canDoStereo());
}

TEST_F (OpusAudioFormatTests, IsCompressed)
{
    EXPECT_TRUE (format->isCompressed());
}

TEST_F (OpusAudioFormatTests, CreateReaderForNullStream)
{
    auto reader = format->createReaderFor (nullptr);
    EXPECT_EQ (nullptr, reader);
}

TEST_F (OpusAudioFormatTests, CreateWriterForNullStream)
{
    auto writer = format->createWriterFor (nullptr, 48000, 2, 32, {}, 0);
    EXPECT_EQ (nullptr, writer);
}

#if ! YUP_EMSCRIPTEN
class OpusAudioFormatFileTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        format = std::make_unique<OpusAudioFormat>();
        testDataDir = File (__FILE__)
                          .getParentDirectory()
                          .getParentDirectory()
                          .getChildFile ("data")
                          .getChildFile ("sounds");
    }

    std::unique_ptr<OpusAudioFormat> format;
    File testDataDir;
};

TEST_F (OpusAudioFormatFileTests, TestAllOpusFilesCanBeOpened)
{
    auto opusFiles = getAllOpusTestFiles();

    for (const auto& filename : opusFiles)
    {
        File opusFile = testDataDir.getChildFile (filename);

        if (! opusFile.exists())
        {
            FAIL() << "Test file does not exist: " << filename.toRawUTF8();
            continue;
        }

        std::unique_ptr<FileInputStream> inputStream = std::make_unique<FileInputStream> (opusFile);
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

        EXPECT_EQ (48000.0, reader->sampleRate) << "Unexpected sample rate for: " << filename.toRawUTF8();
        EXPECT_GT (reader->numChannels, 0) << "Invalid channel count for: " << filename.toRawUTF8();
        EXPECT_GE (reader->lengthInSamples, 0) << "Invalid length for: " << filename.toRawUTF8();
        EXPECT_EQ (32, reader->bitsPerSample) << "Unexpected bit depth for: " << filename.toRawUTF8();
        EXPECT_TRUE (reader->usesFloatingPointData) << "Expected float data for: " << filename.toRawUTF8();

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

TEST_F (OpusAudioFormatFileTests, TestOpusFilesHaveValidData)
{
    auto opusFiles = getAllOpusTestFiles();

    for (const auto& filename : opusFiles)
    {
        File opusFile = testDataDir.getChildFile (filename);

        if (! opusFile.exists())
        {
            FAIL() << "Test file does not exist: " << filename.toRawUTF8();
            continue;
        }

        std::unique_ptr<FileInputStream> inputStream = std::make_unique<FileInputStream> (opusFile);
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

TEST_F (OpusAudioFormatFileTests, TestWriteAndReadRoundTrip)
{
    File tempFile = File::createTempFile (".opus");
    auto deleteTempFileAtExit = ScopeGuard { [&]
    {
        tempFile.deleteFile();
    } };

    const int numSamples = 960 * 3;
    AudioBuffer<float> buffer (1, numSamples);
    auto* channelData = buffer.getWritePointer (0);
    for (int sample = 0; sample < numSamples; ++sample)
        channelData[sample] = std::sin (2.0 * 3.14159 * 440.0 * sample / 48000.0);

    {
        auto outputStream = std::make_unique<FileOutputStream> (tempFile);
        auto writer = format->createWriterFor (outputStream.release(), 48000, 1, 32, {}, 0);
        ASSERT_NE (nullptr, writer);

        const float* const* bufferData = buffer.getArrayOfReadPointers();
        EXPECT_TRUE (writer->write (bufferData, numSamples));
        writer->flush();
    }

    auto inputStream = std::make_unique<FileInputStream> (tempFile);
    auto reader = format->createReaderFor (inputStream.release());
    ASSERT_NE (nullptr, reader);

    EXPECT_DOUBLE_EQ (48000.0, reader->sampleRate);
    EXPECT_EQ (1, reader->numChannels);
    EXPECT_TRUE (reader->usesFloatingPointData);
    EXPECT_GT (reader->lengthInSamples, 0);
    EXPECT_LE (reader->lengthInSamples, numSamples);
    EXPECT_GE (reader->lengthInSamples, numSamples - 1000);

    const int readSamples = static_cast<int> (reader->lengthInSamples);
    AudioBuffer<float> readBuffer (1, readSamples);
    EXPECT_TRUE (reader->read (&readBuffer, 0, readSamples, 0, true, true));

    const int startOffset = jmax (0, numSamples - readSamples);
    const int compareSamples = jmin (readSamples, numSamples - startOffset);
    ASSERT_GT (compareSamples, 0);

    const float* original = buffer.getReadPointer (0) + startOffset;
    const float* decoded = readBuffer.getReadPointer (0);

    double originalSumSq = 0.0;
    double decodedSumSq = 0.0;
    double originalPeak = 0.0;
    double decodedPeak = 0.0;
    for (int i = 0; i < compareSamples; ++i)
    {
        const double ov = original[i];
        const double dv = decoded[i];
        originalSumSq += ov * ov;
        decodedSumSq += dv * dv;
        originalPeak = std::max (originalPeak, std::abs (ov));
        decodedPeak = std::max (decodedPeak, std::abs (dv));

        const double oa = std::abs (ov);
        const double da = std::abs (dv);
    }

    const double originalRms = std::sqrt (originalSumSq / compareSamples);
    const double decodedRms = std::sqrt (decodedSumSq / compareSamples);
    const double rmsRatio = (originalRms > 0.0) ? (decodedRms / originalRms) : 0.0;
    const double peakRatio = (originalPeak > 0.0) ? (decodedPeak / originalPeak) : 0.0;

    HeapBlock<float> absOriginal (compareSamples);
    HeapBlock<float> absDecoded (compareSamples);
    FloatVectorOperations::abs (absOriginal.get(), original, compareSamples);
    FloatVectorOperations::abs (absDecoded.get(), decoded, compareSamples);
    const double simdCorrelation = FloatVectorOperations::computeCorrelation (absOriginal.get(), absDecoded.get(), compareSamples);

    EXPECT_GT (decodedRms, 0.0);
    EXPECT_GT (decodedPeak, 0.0);
    EXPECT_GT (simdCorrelation, 0.7);
    EXPECT_GT (rmsRatio, 0.4);
    EXPECT_LT (rmsRatio, 1.6);
    EXPECT_GT (peakRatio, 0.4);
    EXPECT_LT (peakRatio, 1.6);
}
#endif
