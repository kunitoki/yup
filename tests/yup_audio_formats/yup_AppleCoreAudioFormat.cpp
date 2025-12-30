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
static void fillTestTone (std::vector<float>& left, std::vector<float>& right, int numSamples, int sampleRate)
{
    left.resize ((size_t) numSamples);
    right.resize ((size_t) numSamples);

    constexpr double frequency = 440.0;
    constexpr double twoPi = 6.2831853071795864769;

    for (int i = 0; i < numSamples; ++i)
    {
        const double phase = twoPi * frequency * ((double) i / (double) sampleRate);
        const float sample = (float) std::sin (phase) * 0.25f;
        left[(size_t) i] = sample;
        right[(size_t) i] = sample;
    }
}
} // namespace

#if YUP_AUDIO_FORMAT_COREAUDIO
class AppleCoreAudioFormatTests : public ::testing::Test
{
protected:
    static constexpr int kTestSampleRate = 44100;
    static constexpr int kTestChannels = 2;
    static constexpr int kTestBitsPerSample = 32;
    static constexpr int kTestNumSamples = 2048;

    void SetUp() override
    {
        format = std::make_unique<AppleCoreAudioFormat>();
    }

    std::unique_ptr<AppleCoreAudioFormat> format;
};

TEST_F (AppleCoreAudioFormatTests, GetFormatNameReturnsCoreAudio)
{
    const String& name = format->getFormatName();
    EXPECT_FALSE (name.isEmpty());
    EXPECT_TRUE (name.containsIgnoreCase ("coreaudio") || name.containsIgnoreCase ("core audio"));
}

TEST_F (AppleCoreAudioFormatTests, GetFileExtensionsIncludesM4aOrAac)
{
    Array<String> extensions = format->getFileExtensions();
    EXPECT_FALSE (extensions.isEmpty());

    bool foundExtension = false;
    for (const auto& ext : extensions)
    {
        if (ext.equalsIgnoreCase (".m4a") || ext.equalsIgnoreCase ("m4a")
            || ext.equalsIgnoreCase (".aac") || ext.equalsIgnoreCase ("aac"))
        {
            foundExtension = true;
            break;
        }
    }
    EXPECT_TRUE (foundExtension);
}

TEST_F (AppleCoreAudioFormatTests, GetPossibleBitDepthsAndSampleRates)
{
    Array<int> bitDepths = format->getPossibleBitDepths();
    Array<int> sampleRates = format->getPossibleSampleRates();

    EXPECT_FALSE (bitDepths.isEmpty());
    EXPECT_FALSE (sampleRates.isEmpty());
    EXPECT_TRUE (bitDepths.contains (kTestBitsPerSample));
    EXPECT_TRUE (sampleRates.contains (kTestSampleRate));
}

TEST_F (AppleCoreAudioFormatTests, CanDoMonoAndStereo)
{
    EXPECT_TRUE (format->canDoMono());
    EXPECT_TRUE (format->canDoStereo());
}

TEST_F (AppleCoreAudioFormatTests, IsCompressed)
{
    EXPECT_TRUE (format->isCompressed());
}

TEST_F (AppleCoreAudioFormatTests, CreateReaderForNullStream)
{
    auto reader = format->createReaderFor (nullptr);
    EXPECT_EQ (nullptr, reader);
}

TEST_F (AppleCoreAudioFormatTests, CreateWriterForNullStream)
{
    auto writer = format->createWriterFor (nullptr, kTestSampleRate, kTestChannels, kTestBitsPerSample, {}, 0);
    EXPECT_EQ (nullptr, writer);
}

TEST_F (AppleCoreAudioFormatTests, WriteAndReadBackMemoryStream)
{
    MemoryBlock outputBlock;
    auto* outputStream = new MemoryOutputStream (outputBlock, false);
    auto writer = format->createWriterFor (outputStream,
                                           kTestSampleRate,
                                           kTestChannels,
                                           kTestBitsPerSample,
                                           {},
                                           0);
    ASSERT_NE (nullptr, writer);

    std::vector<float> left;
    std::vector<float> right;
    fillTestTone (left, right, kTestNumSamples, kTestSampleRate);

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

    const auto validation = validateAudioData (*reader);
    EXPECT_FALSE (validation.hasClippedSamples);
    EXPECT_FALSE (validation.hasExtremeValues);
}
#endif
