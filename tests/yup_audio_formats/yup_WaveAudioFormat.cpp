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

#include <gtest/gtest.h>

using namespace yup;

namespace
{
const std::vector<String> getAllWaveTestFiles()
{
    return {
        "M1F1-Alaw-AFsp.wav",
        "M1F1-AlawWE-AFsp.wav",
        "M1F1-float32-AFsp.wav",
        "M1F1-float32WE-AFsp.wav",
        "M1F1-float64-AFsp.wav",
        "M1F1-float64WE-AFsp.wav",
        "M1F1-int16-AFsp.wav",
        "M1F1-int16WE-AFsp.wav",
        "M1F1-int24-AFsp.wav",
        "M1F1-int24WE-AFsp.wav",
        "M1F1-int32-AFsp.wav",
        "M1F1-int32WE-AFsp.wav",
        "M1F1-mulaw-AFsp.wav",
        "M1F1-mulawWE-AFsp.wav",
        "M1F1-uint8-AFsp.wav",
        "M1F1-uint8WE-AFsp.wav",
        "addf8-Alaw-GW.wav",
        "addf8-mulaw-GW.wav"
    };
}

const std::vector<String> getFailingWaveTestFiles()
{
    return {
        "addf8-GSM-GW.wav"
    };
}

struct AudioValidationResult
{
    bool hasClippedSamples = false;
    bool hasExtremeValues = false;
    float maxAbsValue = 0.0f;
    float minValue = 0.0f;
    float maxValue = 0.0f;
    int clippedSampleCount = 0;
    int extremeValueCount = 0;
};

AudioValidationResult validateAudioData (AudioFormatReader& reader)
{
    AudioValidationResult result;

    if (reader.lengthInSamples <= 0)
        return result;

    // Read the entire file in chunks to validate all data
    const int bufferSize = 4096;
    AudioBuffer<float> buffer (static_cast<int> (reader.numChannels), bufferSize);

    int64 samplesRemaining = reader.lengthInSamples;
    int64 currentPos = 0;

    while (samplesRemaining > 0)
    {
        const int samplesToRead = static_cast<int> (std::min ((int64) bufferSize, samplesRemaining));

        if (! reader.read (&buffer, 0, samplesToRead, currentPos, true, true))
            break;

        // Check all channels and samples for extreme values
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const float* channelData = buffer.getReadPointer (ch);

            for (int sample = 0; sample < samplesToRead; ++sample)
            {
                const float value = channelData[sample];
                const float absValue = std::abs (value);

                // Update min/max tracking
                result.minValue = std::min (result.minValue, value);
                result.maxValue = std::max (result.maxValue, value);
                result.maxAbsValue = std::max (result.maxAbsValue, absValue);

                // Check for clipped samples - use a more realistic approach
                // Only flag samples that are obviously clipped or corrupted
                const float clipThreshold = 1.0001f; // Only flag if clearly exceeding normal range

                if (absValue > clipThreshold)
                {
                    result.hasClippedSamples = true;
                    result.clippedSampleCount++;
                }

                // Check for extreme values (beyond normal range, could indicate corruption)
                const float extremeThreshold = 10.0f; // Way beyond normal audio range
                if (absValue > extremeThreshold)
                {
                    result.hasExtremeValues = true;
                    result.extremeValueCount++;
                }
            }
        }

        currentPos += samplesToRead;
        samplesRemaining -= samplesToRead;
    }

    return result;
}

} // namespace

class WaveAudioFormatTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        format = std::make_unique<WaveAudioFormat>();
    }

    std::unique_ptr<WaveAudioFormat> format;
};

TEST_F (WaveAudioFormatTests, GetFormatNameReturnsWave)
{
    const String& name = format->getFormatName();
    EXPECT_FALSE (name.isEmpty());
    EXPECT_TRUE (name.containsIgnoreCase ("wav") || name.containsIgnoreCase ("wave"));
}

TEST_F (WaveAudioFormatTests, GetFileExtensionsIncludesWav)
{
    Array<String> extensions = format->getFileExtensions();
    EXPECT_FALSE (extensions.isEmpty());

    bool foundWav = false;
    for (const auto& ext : extensions)
    {
        if (ext.equalsIgnoreCase (".wav") || ext.equalsIgnoreCase ("wav"))
        {
            foundWav = true;
            break;
        }
    }
    EXPECT_TRUE (foundWav);
}

TEST_F (WaveAudioFormatTests, GetPossibleBitDepthsIsNotEmpty)
{
    Array<int> bitDepths = format->getPossibleBitDepths();
    EXPECT_FALSE (bitDepths.isEmpty());

    for (int depth : bitDepths)
    {
        EXPECT_GT (depth, 0);
        EXPECT_LE (depth, 64);
    }
}

TEST_F (WaveAudioFormatTests, GetPossibleSampleRatesIsNotEmpty)
{
    Array<int> sampleRates = format->getPossibleSampleRates();
    EXPECT_FALSE (sampleRates.isEmpty());

    for (int rate : sampleRates)
    {
        EXPECT_GT (rate, 0);
    }
}

TEST_F (WaveAudioFormatTests, CanDoMonoAndStereo)
{
    EXPECT_TRUE (format->canDoMono());
    EXPECT_TRUE (format->canDoStereo());
}

TEST_F (WaveAudioFormatTests, IsNotCompressed)
{
    EXPECT_FALSE (format->isCompressed());
}

TEST_F (WaveAudioFormatTests, CreateReaderForNullStream)
{
    auto reader = format->createReaderFor (nullptr);
    EXPECT_EQ (nullptr, reader);
}

TEST_F (WaveAudioFormatTests, CreateWriterForNullStream)
{
    auto writer = format->createWriterFor (nullptr, 44100, 2, 16, {}, 0);
    EXPECT_EQ (nullptr, writer);
}

#if ! YUP_EMSCRIPTEN
class WaveAudioFormatFileTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        format = std::make_unique<WaveAudioFormat>();
        testDataDir = File (__FILE__)
                          .getParentDirectory()
                          .getParentDirectory()
                          .getChildFile ("data")
                          .getChildFile ("sounds");
    }

    std::unique_ptr<WaveAudioFormat> format;
    File testDataDir;
};

TEST_F (WaveAudioFormatFileTests, TestAllWaveFilesCanBeOpened)
{
    auto waveFiles = getAllWaveTestFiles();

    for (const auto& filename : waveFiles)
    {
        File waveFile = testDataDir.getChildFile (filename);

        if (! waveFile.exists())
        {
            FAIL() << "Test file does not exist: " << filename.toRawUTF8();
            continue;
        }

        std::unique_ptr<FileInputStream> inputStream = std::make_unique<FileInputStream> (waveFile);
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

        EXPECT_GT (reader->sampleRate, 0) << "Invalid sample rate for: " << filename.toRawUTF8();
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

TEST_F (WaveAudioFormatFileTests, TestWaveFilesHaveValidData)
{
    auto waveFiles = getAllWaveTestFiles();

    for (const auto& filename : waveFiles)
    {
        File waveFile = testDataDir.getChildFile (filename);

        if (! waveFile.exists())
        {
            FAIL() << "Test file does not exist: " << filename.toRawUTF8();
            continue;
        }

        std::unique_ptr<FileInputStream> inputStream = std::make_unique<FileInputStream> (waveFile);
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

        // Validate the audio data
        auto validationResult = validateAudioData (*reader);

        // Check for obviously corrupted samples (values clearly beyond normal range)
        EXPECT_FALSE (validationResult.hasClippedSamples)
            << "File " << filename.toRawUTF8() << " contains "
            << validationResult.clippedSampleCount << " samples clearly exceeding ±1.0 (peak: "
            << validationResult.maxAbsValue << ")";

        // Check for extreme values (corruption/broken data)
        EXPECT_FALSE (validationResult.hasExtremeValues)
            << "File " << filename.toRawUTF8() << " contains "
            << validationResult.extremeValueCount << " extreme values (peak: "
            << validationResult.maxAbsValue << ")";

        // Validate reasonable audio range (allow some headroom for different formats)
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

TEST_F (WaveAudioFormatFileTests, TestFailingWaveFilesCantBeOpened)
{
    auto waveFiles = getFailingWaveTestFiles();

    for (const auto& filename : waveFiles)
    {
        File waveFile = testDataDir.getChildFile (filename);

        if (! waveFile.exists())
        {
            FAIL() << "Test file does not exist: " << filename.toRawUTF8();
            continue;
        }

        std::unique_ptr<FileInputStream> inputStream = std::make_unique<FileInputStream> (waveFile);
        if (! inputStream->openedOk())
        {
            FAIL() << "Could not open file stream for: " << filename.toRawUTF8();
            continue;
        }

        auto reader = format->createReaderFor (inputStream.get());
        EXPECT_TRUE (reader == nullptr);

        inputStream.release();
    }
}

TEST_F (WaveAudioFormatFileTests, TestSpecificWaveFileProperties)
{
    File int16File = testDataDir.getChildFile ("M1F1-int16-AFsp.wav");
    ASSERT_TRUE (int16File.exists());

    std::unique_ptr<FileInputStream> inputStream = std::make_unique<FileInputStream> (int16File);
    ASSERT_TRUE (inputStream->openedOk());

    auto reader = format->createReaderFor (inputStream.get());
    if (reader != nullptr)
    {
        EXPECT_EQ (16, reader->bitsPerSample);
        EXPECT_FALSE (reader->usesFloatingPointData);

        inputStream.release();
    }
    else
    {
        inputStream.release();
        FAIL() << "Could not create reader for M1F1-int16-AFsp.wav";
    }
}

TEST_F (WaveAudioFormatFileTests, TestFloatWaveFileProperties)
{
    File float32File = testDataDir.getChildFile ("M1F1-float32-AFsp.wav");
    ASSERT_TRUE (float32File.exists());

    std::unique_ptr<FileInputStream> inputStream = std::make_unique<FileInputStream> (float32File);
    ASSERT_TRUE (inputStream->openedOk());

    auto reader = format->createReaderFor (inputStream.get());
    if (reader != nullptr)
    {
        EXPECT_EQ (32, reader->bitsPerSample);
        EXPECT_TRUE (reader->usesFloatingPointData);

        inputStream.release();
    }
    else
    {
        inputStream.release();
        std::cout << "Warning: Could not create reader for float32 file" << std::endl;
    }
}

TEST_F (WaveAudioFormatFileTests, TestWriteAndReadRoundTrip)
{
    File tempFile = File::createTempFile (".wav");
    auto deleteTempFileAtExit = ScopeGuard { [&]
    {
        tempFile.deleteFile();
    } };

    const double sampleRate = 44100.0;
    const int numChannels = 2;
    const int bitsPerSample = 16;
    const int numSamples = 1000;

    {
        std::unique_ptr<FileOutputStream> outputStream = std::make_unique<FileOutputStream> (tempFile);
        auto writer = format->createWriterFor (outputStream.get(), sampleRate, numChannels, bitsPerSample, {}, 0);

        if (writer != nullptr)
        {
            AudioBuffer<float> buffer (numChannels, numSamples);

            for (int channel = 0; channel < numChannels; ++channel)
            {
                auto* channelData = buffer.getWritePointer (channel);
                for (int sample = 0; sample < numSamples; ++sample)
                    channelData[sample] = static_cast<float> (std::sin (2.0 * 3.14159 * 440.0 * sample / sampleRate));
            }

            const float* const* bufferData = buffer.getArrayOfReadPointers();
            bool writeSuccess = writer->write (bufferData, numSamples);
            EXPECT_TRUE (writeSuccess);

            outputStream.release();
        }
        else
        {
            outputStream.release();
            FAIL() << "Could not create writer for temporary file";
        }
    }

    {
        std::unique_ptr<FileInputStream> inputStream = std::make_unique<FileInputStream> (tempFile);
        auto reader = format->createReaderFor (inputStream.get());

        if (reader != nullptr)
        {
            EXPECT_DOUBLE_EQ (sampleRate, reader->sampleRate);
            EXPECT_EQ (numChannels, reader->numChannels);
            EXPECT_EQ (bitsPerSample, reader->bitsPerSample);
            EXPECT_GE (reader->lengthInSamples, numSamples);

            AudioBuffer<float> readBuffer (numChannels, numSamples);
            bool readSuccess = reader->read (&readBuffer, 0, numSamples, 0, true, true);
            EXPECT_TRUE (readSuccess);

            inputStream.release();
        }
        else
        {
            inputStream.release();
            FAIL() << "Could not create reader for temporary file";
        }
    }
}
#endif
