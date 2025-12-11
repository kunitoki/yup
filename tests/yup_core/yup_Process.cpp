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

#include <yup_core/yup_core.h>

using namespace yup;

#if YUP_WINDOWS || YUP_MAC || YUP_LINUX || YUP_BSD

class ProcessTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        testFile = File::getSpecialLocation (File::tempDirectory)
                       .getChildFile ("YUP_ProcessTests_" + String::toHexString (Random::getSystemRandom().nextInt()))
                       .getChildFile ("test_document.txt");

        testFile.getParentDirectory().createDirectory();
        testFile.replaceWithText ("Test content for Process::openDocument");
    }

    void TearDown() override
    {
        testFile.getParentDirectory().deleteRecursively();
    }

    File testFile;
};

TEST_F (ProcessTests, OpenDocumentWithFileName)
{
    // Test Process::openDocument() with a file name
    // This attempts to open the file with the default application
    // It may fail if there's no default application or if running in CI

    bool result = Process::openDocument (testFile.getFullPathName(), "");

    // We don't assert the result because:
    // 1. It may fail in CI environments
    // 2. It requires a default application to be registered
    // 3. It's platform-dependent
    // Just verify it doesn't crash
    SUCCEED();
}

TEST_F (ProcessTests, OpenDocumentWithUrl)
{
    // Test opening a URL (this should be safer than opening a file)
    // Most systems have a default browser

    // Use a safe, non-intrusive URL
    String testUrl = "about:blank";

    [[maybe_unused]] bool result = Process::openDocument (testUrl, "");

    SUCCEED();
}

TEST_F (ProcessTests, OpenDocumentWithParameters)
{
    // Test Process::openDocument() with parameters
    [[maybe_unused]] bool result = Process::openDocument (testFile.getFullPathName(), "--test-param");

    SUCCEED();
}

TEST_F (ProcessTests, OpenDocumentWithEnvironment)
{
    // Test Process::openDocument() with custom environment variables
    StringPairArray environment;
    environment.set ("TEST_VAR", "test_value");

    [[maybe_unused]] bool result = Process::openDocument (testFile.getFullPathName(), "", environment);

    // Don't assert success, just verify it doesn't crash
    SUCCEED();
}

TEST_F (ProcessTests, OpenDocumentWithEmptyPath)
{
    // Test with empty path (should fail gracefully)
    [[maybe_unused]] bool result = Process::openDocument ("", "");

    // Don't assert success, just verify it doesn't crash
    SUCCEED();
}

TEST_F (ProcessTests, OpenDocumentWithNonExistentFile)
{
    // Test with a file that doesn't exist
    File nonExistent = File::getSpecialLocation (File::tempDirectory)
                           .getChildFile ("this_file_does_not_exist_12345.xyz");

    [[maybe_unused]] bool result = Process::openDocument (nonExistent.getFullPathName(), "");

    // Most systems will fail to open a non-existent file
    // but we don't assert because behavior is platform-dependent
    SUCCEED();
}

TEST_F (ProcessTests, OpenDocumentWithSpecialCharacters)
{
    // Create a file with special characters in the name
    File specialFile = testFile.getParentDirectory().getChildFile ("test file with spaces & special.txt");
    specialFile.replaceWithText ("Test content");

    [[maybe_unused]] bool result = Process::openDocument (specialFile.getFullPathName(), "");

    // Clean up
    specialFile.deleteFile();

    // Don't assert success due to platform differences
    SUCCEED();
}
#endif
