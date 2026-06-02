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

#include <yup_core/yup_core.h>

using namespace yup;

namespace
{

class RecordingLogger : public Logger
{
public:
    void logMessage (const String& message) override
    {
        const ScopedLock sl (lock);
        messages.add (message);
    }

    StringArray getMessages()
    {
        const ScopedLock sl (lock);
        return messages;
    }

private:
    CriticalSection lock;
    StringArray messages;
};

} // namespace

class LoggerTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        Logger::setCurrentLogger (nullptr);
    }

    void TearDown() override
    {
        Logger::setCurrentLogger (nullptr);
    }
};

TEST_F (LoggerTests, DefaultStateHasNoLogger)
{
    EXPECT_EQ (Logger::getCurrentLogger(), nullptr);
}

TEST_F (LoggerTests, SetAndGetCurrentLogger)
{
    RecordingLogger logger;
    Logger::setCurrentLogger (&logger);
    EXPECT_EQ (Logger::getCurrentLogger(), &logger);
    Logger::setCurrentLogger (nullptr);
}

TEST_F (LoggerTests, WriteToLogDispatchesToCurrentLogger)
{
    RecordingLogger logger;
    Logger::setCurrentLogger (&logger);

    Logger::writeToLog ("hello");
    Logger::writeToLog ("world");

    auto messages = logger.getMessages();
    EXPECT_EQ (messages.size(), 2);
    EXPECT_EQ (messages[0], "hello");
    EXPECT_EQ (messages[1], "world");
    Logger::setCurrentLogger (nullptr);
}

TEST_F (LoggerTests, WriteToLogWithNoLoggerDoesNotCrash)
{
    EXPECT_EQ (Logger::getCurrentLogger(), nullptr);
    Logger::writeToLog ("no logger set");
}

TEST_F (LoggerTests, SetCurrentLoggerToNullRemovesLogger)
{
    RecordingLogger logger;
    Logger::setCurrentLogger (&logger);
    EXPECT_NE (Logger::getCurrentLogger(), nullptr);

    Logger::setCurrentLogger (nullptr);
    EXPECT_EQ (Logger::getCurrentLogger(), nullptr);
}

TEST_F (LoggerTests, WriteAfterRemovingLoggerDoesNotDispatch)
{
    RecordingLogger logger;
    Logger::setCurrentLogger (&logger);
    Logger::writeToLog ("first");

    Logger::setCurrentLogger (nullptr);
    Logger::writeToLog ("second");

    EXPECT_EQ (logger.getMessages().size(), 1);
    EXPECT_EQ (logger.getMessages()[0], "first");
}

TEST_F (LoggerTests, OutputDebugStringDoesNotCrash)
{
    Logger::outputDebugString ("debug output");
}

// =============================================================================

class FileLoggerTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        Logger::setCurrentLogger (nullptr);
        tempFile = File::getSpecialLocation (File::tempDirectory)
                       .getChildFile ("YUP_FileLoggerTests_" + String::toHexString (Random::getSystemRandom().nextInt()) + ".log");
        tempFile.deleteFile();
    }

    void TearDown() override
    {
        Logger::setCurrentLogger (nullptr);
        tempFile.deleteFile();
    }

    File tempFile;
};

TEST_F (FileLoggerTests, ConstructionCreatesFile)
{
    FileLogger logger (tempFile, "welcome", -1);
    EXPECT_TRUE (tempFile.exists());
    EXPECT_EQ (logger.getLogFile(), tempFile);
}

TEST_F (FileLoggerTests, GetLogFileReturnsCorrectFile)
{
    FileLogger logger (tempFile, "test", -1);
    EXPECT_EQ (logger.getLogFile().getFullPathName(), tempFile.getFullPathName());
}

TEST_F (FileLoggerTests, WelcomeMessageIsWritten)
{
    {
        FileLogger logger (tempFile, "my welcome message", -1);
    }

    auto content = tempFile.loadFileAsString();
    EXPECT_TRUE (content.contains ("my welcome message"));
}

TEST_F (FileLoggerTests, LogMessageWritesToFile)
{
    {
        FileLogger logger (tempFile, "start", -1);
        Logger::setCurrentLogger (&logger);
        Logger::writeToLog ("test message");
        Logger::setCurrentLogger (nullptr);
    }

    EXPECT_TRUE (tempFile.loadFileAsString().contains ("test message"));
}

TEST_F (FileLoggerTests, MultipleMessagesAllWritten)
{
    {
        FileLogger logger (tempFile, "start", -1);
        logger.logMessage ("line one");
        logger.logMessage ("line two");
        logger.logMessage ("line three");
    }

    auto content = tempFile.loadFileAsString();
    EXPECT_TRUE (content.contains ("line one"));
    EXPECT_TRUE (content.contains ("line two"));
    EXPECT_TRUE (content.contains ("line three"));
}

TEST_F (FileLoggerTests, TrimFileSizeTruncatesLargeFile)
{
    {
        FileLogger logger (tempFile, "start", -1);
        for (int i = 0; i < 200; ++i)
            logger.logMessage ("line " + String (i) + " some extra padding content to make lines longer");
    }

    auto originalSize = tempFile.getSize();
    ASSERT_GT (originalSize, 0);

    auto trimTarget = originalSize / 2;
    FileLogger::trimFileSize (tempFile, trimTarget);

    EXPECT_LE (tempFile.getSize(), trimTarget + 512);
    EXPECT_GT (tempFile.getSize(), 0);
}

TEST_F (FileLoggerTests, TrimFileSizeWithZeroDeletesFile)
{
    {
        FileLogger logger (tempFile, "start", -1);
        logger.logMessage ("some content");
    }

    ASSERT_TRUE (tempFile.exists());
    FileLogger::trimFileSize (tempFile, 0);
    EXPECT_FALSE (tempFile.exists());
}

TEST_F (FileLoggerTests, TrimFileSizeDoesNothingWhenFileIsSmallerThanLimit)
{
    {
        FileLogger logger (tempFile, "start", -1);
        logger.logMessage ("short");
    }

    auto originalSize = tempFile.getSize();
    FileLogger::trimFileSize (tempFile, originalSize * 2);
    EXPECT_EQ (tempFile.getSize(), originalSize);
}

TEST_F (FileLoggerTests, GetSystemLogFileFolderReturnsNonEmptyPath)
{
    auto folder = FileLogger::getSystemLogFileFolder();
    EXPECT_FALSE (folder.getFullPathName().isEmpty());
}

TEST_F (FileLoggerTests, MaxInitialFileSizeTruncatesExistingLargeFile)
{
    {
        FileLogger firstLogger (tempFile, "first session", -1);
        for (int i = 0; i < 200; ++i)
            firstLogger.logMessage ("padding line " + String (i) + " with some extra content to grow the file");
    }

    auto sizeBeforeSecondOpen = tempFile.getSize();
    ASSERT_GT (sizeBeforeSecondOpen, 1024);

    {
        FileLogger secondLogger (tempFile, "second session", 512);
    }

    EXPECT_LE (tempFile.getSize(), sizeBeforeSecondOpen);
}
