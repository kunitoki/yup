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

const File getTestDataDirectory()
{
    return File (__FILE__)
        .getParentDirectory()
        .getParentDirectory()
        .getChildFile ("data")
        .getChildFile ("sounds");
}

} // namespace

class AudioFormatManagerTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        manager = std::make_unique<AudioFormatManager>();
    }

    std::unique_ptr<AudioFormatManager> manager;
};

TEST_F (AudioFormatManagerTests, ConstructorCreatesEmptyManager)
{
    EXPECT_NE (nullptr, manager);
}

#if ! YUP_EMSCRIPTEN
TEST_F (AudioFormatManagerTests, RegisterDefaultFormatsAddsWaveFormat)
{
    manager->registerDefaultFormats();

    File testDataDir = getTestDataDirectory();
    File waveFile = testDataDir.getChildFile ("M1F1-int16-AFsp.wav");

    if (waveFile.exists())
    {
        auto reader = manager->createReaderFor (waveFile);
        EXPECT_NE (nullptr, reader);
    }
}
#endif

TEST_F (AudioFormatManagerTests, CreateReaderForNonExistentFile)
{
    manager->registerDefaultFormats();

    File nonExistentFile ("/path/that/does/not/exist.wav");
    auto reader = manager->createReaderFor (nonExistentFile);
    EXPECT_EQ (nullptr, reader);
}

TEST_F (AudioFormatManagerTests, CreateReaderForUnsupportedFormat)
{
    manager->registerDefaultFormats();

    File testFile = File::createTempFile (".unsupported");
    testFile.replaceWithText ("not audio data");

    auto reader = manager->createReaderFor (testFile);
    EXPECT_EQ (nullptr, reader);

    testFile.deleteFile();
}

TEST_F (AudioFormatManagerTests, CreateWriterForValidWaveFile)
{
    manager->registerDefaultFormats();

    File tempFile = File::createTempFile (".wav");
    auto writer = manager->createWriterFor (tempFile, 44100, 2, 16);

    EXPECT_NE (nullptr, writer);

    tempFile.deleteFile();
}

TEST_F (AudioFormatManagerTests, CreateWriterForUnsupportedFormat)
{
    manager->registerDefaultFormats();

    File tempFile = File::createTempFile (".unsupported");
    auto writer = manager->createWriterFor (tempFile, 44100, 2, 16);

    EXPECT_EQ (nullptr, writer);

    tempFile.deleteFile();
}
