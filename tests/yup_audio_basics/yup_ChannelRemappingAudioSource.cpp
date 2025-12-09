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

#include <gtest/gtest.h>

#include <yup_audio_basics/yup_audio_basics.h>

using namespace yup;

//==============================================================================
namespace
{
class MockAudioSource : public AudioSource
{
public:
    MockAudioSource() = default;
    ~MockAudioSource() override = default;

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override
    {
        prepareToPlayCalled = true;
        lastSamplesPerBlock = samplesPerBlockExpected;
        lastSampleRate = sampleRate;
    }

    void releaseResources() override
    {
        releaseResourcesCalled = true;
    }

    void getNextAudioBlock (const AudioSourceChannelInfo& info) override
    {
        getNextAudioBlockCalled = true;

        // Fill each channel with different values for testing remapping
        for (int ch = 0; ch < info.buffer->getNumChannels(); ++ch)
        {
            const float value = static_cast<float> (ch) * 0.1f + 0.1f;
            for (int i = 0; i < info.numSamples; ++i)
            {
                info.buffer->setSample (ch, info.startSample + i, value);
            }
        }
    }

    bool prepareToPlayCalled = false;
    bool releaseResourcesCalled = false;
    bool getNextAudioBlockCalled = false;
    int lastSamplesPerBlock = 0;
    double lastSampleRate = 0.0;
};
} // namespace

//==============================================================================
class ChannelRemappingAudioSourceTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mockSource = new MockAudioSource();
        remapper = std::make_unique<ChannelRemappingAudioSource> (mockSource, true);
    }

    void TearDown() override
    {
        remapper.reset();
    }

    MockAudioSource* mockSource; // Owned by remapper
    std::unique_ptr<ChannelRemappingAudioSource> remapper;
};

//==============================================================================
TEST_F (ChannelRemappingAudioSourceTests, Constructor)
{
    auto* source = new MockAudioSource();
    EXPECT_NO_THROW (ChannelRemappingAudioSource (source, true));
}

TEST_F (ChannelRemappingAudioSourceTests, Destructor)
{
    auto* source = new MockAudioSource();
    auto* temp = new ChannelRemappingAudioSource (source, true);
    EXPECT_NO_THROW (delete temp);
}

//==============================================================================
TEST_F (ChannelRemappingAudioSourceTests, SetNumberOfChannelsToProduce)
{
    EXPECT_NO_THROW (remapper->setNumberOfChannelsToProduce (4));
    EXPECT_NO_THROW (remapper->setNumberOfChannelsToProduce (8));
    EXPECT_NO_THROW (remapper->setNumberOfChannelsToProduce (1));
}

//==============================================================================
TEST_F (ChannelRemappingAudioSourceTests, ClearAllMappings)
{
    remapper->setInputChannelMapping (0, 1);
    remapper->setInputChannelMapping (1, 0);
    remapper->setOutputChannelMapping (0, 1);

    EXPECT_NO_THROW (remapper->clearAllMappings());

    // After clearing, mappings should return -1
    EXPECT_EQ (remapper->getRemappedInputChannel (0), -1);
    EXPECT_EQ (remapper->getRemappedOutputChannel (0), -1);
}

//==============================================================================
TEST_F (ChannelRemappingAudioSourceTests, SetInputChannelMapping)
{
    remapper->setInputChannelMapping (0, 1);
    remapper->setInputChannelMapping (1, 0);

    EXPECT_EQ (remapper->getRemappedInputChannel (0), 1);
    EXPECT_EQ (remapper->getRemappedInputChannel (1), 0);
}

TEST_F (ChannelRemappingAudioSourceTests, SetInputChannelMappingWithGap)
{
    // Setting index 3 should fill gaps with -1 (line 73-74)
    remapper->setInputChannelMapping (3, 2);

    EXPECT_EQ (remapper->getRemappedInputChannel (0), -1);
    EXPECT_EQ (remapper->getRemappedInputChannel (1), -1);
    EXPECT_EQ (remapper->getRemappedInputChannel (2), -1);
    EXPECT_EQ (remapper->getRemappedInputChannel (3), 2);
}

//==============================================================================
TEST_F (ChannelRemappingAudioSourceTests, SetOutputChannelMapping)
{
    remapper->setOutputChannelMapping (0, 1);
    remapper->setOutputChannelMapping (1, 0);

    EXPECT_EQ (remapper->getRemappedOutputChannel (0), 1);
    EXPECT_EQ (remapper->getRemappedOutputChannel (1), 0);
}

TEST_F (ChannelRemappingAudioSourceTests, SetOutputChannelMappingWithGap)
{
    // Setting index 3 should fill gaps with -1 (line 83-84)
    remapper->setOutputChannelMapping (3, 2);

    EXPECT_EQ (remapper->getRemappedOutputChannel (0), -1);
    EXPECT_EQ (remapper->getRemappedOutputChannel (1), -1);
    EXPECT_EQ (remapper->getRemappedOutputChannel (2), -1);
    EXPECT_EQ (remapper->getRemappedOutputChannel (3), 2);
}

//==============================================================================
TEST_F (ChannelRemappingAudioSourceTests, GetRemappedInputChannelInvalid)
{
    // Negative index should return -1 (line 93)
    EXPECT_EQ (remapper->getRemappedInputChannel (-1), -1);

    // Out of bounds should return -1 (line 93)
    EXPECT_EQ (remapper->getRemappedInputChannel (100), -1);
}

TEST_F (ChannelRemappingAudioSourceTests, GetRemappedOutputChannelInvalid)
{
    // Negative index should return -1 (line 103)
    EXPECT_EQ (remapper->getRemappedOutputChannel (-1), -1);

    // Out of bounds should return -1 (line 103)
    EXPECT_EQ (remapper->getRemappedOutputChannel (100), -1);
}

//==============================================================================
TEST_F (ChannelRemappingAudioSourceTests, PrepareToPlay)
{
    remapper->prepareToPlay (512, 44100.0);

    // Should call prepareToPlay on source (line 112)
    EXPECT_TRUE (mockSource->prepareToPlayCalled);
    EXPECT_EQ (mockSource->lastSamplesPerBlock, 512);
    EXPECT_DOUBLE_EQ (mockSource->lastSampleRate, 44100.0);
}

TEST_F (ChannelRemappingAudioSourceTests, ReleaseResources)
{
    remapper->prepareToPlay (512, 44100.0);
    remapper->releaseResources();

    // Should call releaseResources on source (line 117)
    EXPECT_TRUE (mockSource->releaseResourcesCalled);
}

//==============================================================================
TEST_F (ChannelRemappingAudioSourceTests, GetNextAudioBlockNoMapping)
{
    remapper->setNumberOfChannelsToProduce (2);
    remapper->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (2, 512);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    remapper->getNextAudioBlock (info);

    // Should call getNextAudioBlock on source (line 144)
    EXPECT_TRUE (mockSource->getNextAudioBlockCalled);
}

TEST_F (ChannelRemappingAudioSourceTests, GetNextAudioBlockSwapChannels)
{
    remapper->setNumberOfChannelsToProduce (2);

    // Swap input channels 0 and 1
    remapper->setInputChannelMapping (0, 1);
    remapper->setInputChannelMapping (1, 0);

    // Swap output channels back
    remapper->setOutputChannelMapping (0, 1);
    remapper->setOutputChannelMapping (1, 0);

    remapper->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (2, 512);

    // Fill with different values per channel
    for (int i = 0; i < 512; ++i)
    {
        buffer.setSample (0, i, 1.0f);
        buffer.setSample (1, i, 2.0f);
    }

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    remapper->getNextAudioBlock (info);

    // Channels should be swapped twice (input then output), back to original
    EXPECT_TRUE (mockSource->getNextAudioBlockCalled);
}

TEST_F (ChannelRemappingAudioSourceTests, GetNextAudioBlockClearUnmappedInput)
{
    remapper->setNumberOfChannelsToProduce (2);

    // Don't map input channel 1, it should be cleared (line 138)
    remapper->setInputChannelMapping (0, 0);

    remapper->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (2, 512);
    buffer.clear();

    // Fill with values
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < 512; ++i)
        {
            buffer.setSample (ch, i, 1.0f);
        }
    }

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    remapper->getNextAudioBlock (info);

    EXPECT_TRUE (mockSource->getNextAudioBlockCalled);
}

TEST_F (ChannelRemappingAudioSourceTests, GetNextAudioBlockInvalidInputMapping)
{
    remapper->setNumberOfChannelsToProduce (2);

    // Map to invalid channel (line 132)
    remapper->setInputChannelMapping (0, 10);

    remapper->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (2, 512);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    // Should not crash (line 137-139)
    EXPECT_NO_THROW (remapper->getNextAudioBlock (info));
}

TEST_F (ChannelRemappingAudioSourceTests, GetNextAudioBlockInvalidOutputMapping)
{
    remapper->setNumberOfChannelsToProduce (2);

    // Map to invalid output channel (line 152)
    remapper->setOutputChannelMapping (0, 10);

    remapper->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (2, 512);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    // Should not crash (line 152-155)
    EXPECT_NO_THROW (remapper->getNextAudioBlock (info));
}

TEST_F (ChannelRemappingAudioSourceTests, GetNextAudioBlockClearsBuffer)
{
    remapper->setNumberOfChannelsToProduce (2);
    remapper->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (2, 512);

    // Fill with non-zero values
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < 512; ++i)
        {
            buffer.setSample (ch, i, 5.0f);
        }
    }

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    remapper->getNextAudioBlock (info);

    // Buffer should be cleared before writing output (line 146)
    // The output will be from mock source
    EXPECT_TRUE (mockSource->getNextAudioBlockCalled);
}

TEST_F (ChannelRemappingAudioSourceTests, GetNextAudioBlockWithStartSample)
{
    remapper->setNumberOfChannelsToProduce (2);
    remapper->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (2, 512);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 100;
    info.numSamples = 256;

    remapper->getNextAudioBlock (info);

    EXPECT_TRUE (mockSource->getNextAudioBlockCalled);

    // Samples before startSample should remain zero
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < 100; ++i)
        {
            EXPECT_FLOAT_EQ (buffer.getSample (ch, i), 0.0f);
        }
    }
}

//==============================================================================
TEST_F (ChannelRemappingAudioSourceTests, CreateXmlEmpty)
{
    auto xml = remapper->createXml();

    ASSERT_NE (xml, nullptr);
    EXPECT_TRUE (xml->hasTagName ("MAPPINGS"));
    EXPECT_EQ (xml->getStringAttribute ("inputs"), "");
    EXPECT_EQ (xml->getStringAttribute ("outputs"), "");
}

TEST_F (ChannelRemappingAudioSourceTests, CreateXmlWithMappings)
{
    remapper->setInputChannelMapping (0, 1);
    remapper->setInputChannelMapping (1, 0);
    remapper->setInputChannelMapping (2, 2);

    remapper->setOutputChannelMapping (0, 1);
    remapper->setOutputChannelMapping (1, 0);

    auto xml = remapper->createXml();

    ASSERT_NE (xml, nullptr);
    EXPECT_TRUE (xml->hasTagName ("MAPPINGS"));

    // Check inputs attribute (line 167-168)
    String inputs = xml->getStringAttribute ("inputs");
    EXPECT_FALSE (inputs.isEmpty());
    EXPECT_TRUE (inputs.contains ("1"));
    EXPECT_TRUE (inputs.contains ("0"));
    EXPECT_TRUE (inputs.contains ("2"));

    // Check outputs attribute (line 170-171)
    String outputs = xml->getStringAttribute ("outputs");
    EXPECT_FALSE (outputs.isEmpty());
    EXPECT_TRUE (outputs.contains ("1"));
    EXPECT_TRUE (outputs.contains ("0"));
}

TEST_F (ChannelRemappingAudioSourceTests, CreateXmlTrimmed)
{
    remapper->setInputChannelMapping (0, 1);
    remapper->setOutputChannelMapping (0, 2);

    auto xml = remapper->createXml();

    // Attributes should be trimmed (line 173-174)
    String inputs = xml->getStringAttribute ("inputs");
    String outputs = xml->getStringAttribute ("outputs");

    EXPECT_FALSE (inputs.startsWithChar (' '));
    EXPECT_FALSE (inputs.endsWithChar (' '));
    EXPECT_FALSE (outputs.startsWithChar (' '));
    EXPECT_FALSE (outputs.endsWithChar (' '));
}

//==============================================================================
TEST_F (ChannelRemappingAudioSourceTests, RestoreFromXmlInvalidTag)
{
    XmlElement xml ("INVALID");

    // Should not restore from invalid tag (line 181)
    EXPECT_NO_THROW (remapper->restoreFromXml (xml));
}

TEST_F (ChannelRemappingAudioSourceTests, RestoreFromXmlEmpty)
{
    XmlElement xml ("MAPPINGS");

    remapper->restoreFromXml (xml);

    // Should clear mappings (line 185)
    EXPECT_EQ (remapper->getRemappedInputChannel (0), -1);
    EXPECT_EQ (remapper->getRemappedOutputChannel (0), -1);
}

TEST_F (ChannelRemappingAudioSourceTests, RestoreFromXmlWithMappings)
{
    XmlElement xml ("MAPPINGS");
    xml.setAttribute ("inputs", "1 0 2");
    xml.setAttribute ("outputs", "1 0");

    remapper->restoreFromXml (xml);

    // Check restored input mappings (line 191-192)
    EXPECT_EQ (remapper->getRemappedInputChannel (0), 1);
    EXPECT_EQ (remapper->getRemappedInputChannel (1), 0);
    EXPECT_EQ (remapper->getRemappedInputChannel (2), 2);

    // Check restored output mappings (line 194-195)
    EXPECT_EQ (remapper->getRemappedOutputChannel (0), 1);
    EXPECT_EQ (remapper->getRemappedOutputChannel (1), 0);
}

TEST_F (ChannelRemappingAudioSourceTests, RestoreFromXmlClearsPrevious)
{
    remapper->setInputChannelMapping (0, 5);
    remapper->setOutputChannelMapping (0, 5);

    XmlElement xml ("MAPPINGS");
    xml.setAttribute ("inputs", "1");
    xml.setAttribute ("outputs", "2");

    remapper->restoreFromXml (xml);

    // Previous mappings should be cleared (line 185)
    EXPECT_EQ (remapper->getRemappedInputChannel (0), 1);
    EXPECT_EQ (remapper->getRemappedOutputChannel (0), 2);
}

TEST_F (ChannelRemappingAudioSourceTests, XmlRoundtrip)
{
    remapper->setInputChannelMapping (0, 2);
    remapper->setInputChannelMapping (1, 1);
    remapper->setInputChannelMapping (2, 0);

    remapper->setOutputChannelMapping (0, 1);
    remapper->setOutputChannelMapping (1, 2);
    remapper->setOutputChannelMapping (2, 0);

    // Create XML
    auto xml = remapper->createXml();
    ASSERT_NE (xml, nullptr);

    // Create new remapper and restore
    auto* newMockSource = new MockAudioSource();
    auto newRemapper = std::make_unique<ChannelRemappingAudioSource> (newMockSource, true);

    newRemapper->restoreFromXml (*xml);

    // Check all mappings were restored correctly
    EXPECT_EQ (newRemapper->getRemappedInputChannel (0), 2);
    EXPECT_EQ (newRemapper->getRemappedInputChannel (1), 1);
    EXPECT_EQ (newRemapper->getRemappedInputChannel (2), 0);

    EXPECT_EQ (newRemapper->getRemappedOutputChannel (0), 1);
    EXPECT_EQ (newRemapper->getRemappedOutputChannel (1), 2);
    EXPECT_EQ (newRemapper->getRemappedOutputChannel (2), 0);
}
