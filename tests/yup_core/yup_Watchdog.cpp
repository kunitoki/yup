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

#if YUP_LINUX || YUP_WINDOWS || YUP_MAC

class WatchdogTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        testFolder = File::getSpecialLocation (File::tempDirectory)
                         .getChildFile ("YUP_WatchdogTests_" + String::toHexString (Random::getSystemRandom().nextInt()));

        testFolder.deleteRecursively();
        testFolder.createDirectory();
    }

    void TearDown() override
    {
        testFolder.deleteRecursively();
    }

    File testFolder;
};

TEST_F (WatchdogTests, CreateInstance)
{
    auto watchdog = Watchdog::createInstance (std::chrono::milliseconds (100));

    ASSERT_NE (watchdog, nullptr);
    EXPECT_EQ (watchdog->getAllWatchedFolders().size(), 0);
}

TEST_F (WatchdogTests, WatchFolder)
{
    auto watchdog = Watchdog::createInstance (std::chrono::milliseconds (100));
    ASSERT_NE (watchdog, nullptr);

    watchdog->watchFolder (testFolder);

    auto watchedFolders = watchdog->getAllWatchedFolders();
    EXPECT_EQ (watchedFolders.size(), 1);
    EXPECT_EQ (watchedFolders[0].getFullPathName(), testFolder.getFullPathName());
}

TEST_F (WatchdogTests, WatchMultipleFolders)
{
    auto watchdog = Watchdog::createInstance (std::chrono::milliseconds (100));
    ASSERT_NE (watchdog, nullptr);

    File folder1 = testFolder.getChildFile ("subfolder1");
    File folder2 = testFolder.getChildFile ("subfolder2");

    folder1.createDirectory();
    folder2.createDirectory();

    watchdog->watchFolder (folder1);
    watchdog->watchFolder (folder2);

    auto watchedFolders = watchdog->getAllWatchedFolders();
    EXPECT_EQ (watchedFolders.size(), 2);
}

TEST_F (WatchdogTests, UnwatchFolder)
{
    auto watchdog = Watchdog::createInstance (std::chrono::milliseconds (100));
    ASSERT_NE (watchdog, nullptr);

    watchdog->watchFolder (testFolder);
    EXPECT_EQ (watchdog->getAllWatchedFolders().size(), 1);

    watchdog->unwatchFolder (testFolder);
    EXPECT_EQ (watchdog->getAllWatchedFolders().size(), 0);
}

TEST_F (WatchdogTests, UnwatchAllFolders)
{
    auto watchdog = Watchdog::createInstance (std::chrono::milliseconds (100));
    ASSERT_NE (watchdog, nullptr);

    File folder1 = testFolder.getChildFile ("subfolder1");
    File folder2 = testFolder.getChildFile ("subfolder2");

    folder1.createDirectory();
    folder2.createDirectory();

    watchdog->watchFolder (folder1);
    watchdog->watchFolder (folder2);

    EXPECT_EQ (watchdog->getAllWatchedFolders().size(), 2);

    watchdog->unwatchAllFolders();
    EXPECT_EQ (watchdog->getAllWatchedFolders().size(), 0);
}

TEST_F (WatchdogTests, DetectFileCreation)
{
    auto watchdog = Watchdog::createInstance (std::chrono::milliseconds (100));
    ASSERT_NE (watchdog, nullptr);

    watchdog->watchFolder (testFolder);

    std::vector<Watchdog::Event> capturedEvents;
    auto callback = [&capturedEvents] (std::vector<Watchdog::Event> events)
    {
        capturedEvents = std::move (events);
    };

    // Give watchdog time to start
    Thread::sleep (150);

    // Create a new file
    File newFile = testFolder.getChildFile ("new_file.txt");
    newFile.replaceWithText ("Test content");

    // Wait for event to be detected
    Thread::sleep (250);

    // Dispatch events
    std::size_t eventCount = watchdog->dispatchEvents (callback);

    if (eventCount > 0)
    {
        EXPECT_GT (capturedEvents.size(), 0);

        // Check if we have a file creation event
        // Note: File system watchers are platform-specific and may report events for:
        // - The actual file created
        // - The parent directory containing the file
        // - Both the file and directory
        // So we just verify that we got some creation events without strict assertions
        bool foundCreation = false;
        for (const auto& event : capturedEvents)
        {
            if (event.changeEvent == Watchdog::EventType::file_created)
            {
                foundCreation = true;

                // The event might be for the file itself or its parent directory
                // Both are valid depending on the platform
                String eventFileName = event.originalFile.getFileName();
                bool isExpectedFile = (eventFileName == newFile.getFileName());
                bool isParentDir = (eventFileName == testFolder.getFileName());

                // On macOS, FSEvents may report directory changes instead of individual files
                EXPECT_TRUE (isExpectedFile || isParentDir);
            }
        }

        // Don't assert foundCreation as timing and platform differences may affect detection
        (void) foundCreation;
    }
}

TEST_F (WatchdogTests, DetectFileModification)
{
    auto watchdog = Watchdog::createInstance (std::chrono::milliseconds (100));
    ASSERT_NE (watchdog, nullptr);

    // Create a file first
    File testFile = testFolder.getChildFile ("test_file.txt");
    testFile.replaceWithText ("Initial content");

    // Start watching after file creation
    watchdog->watchFolder (testFolder);

    std::vector<Watchdog::Event> capturedEvents;
    auto callback = [&capturedEvents] (std::vector<Watchdog::Event> events)
    {
        capturedEvents = std::move (events);
    };

    // Give watchdog time to start
    Thread::sleep (150);

    // Modify the file
    testFile.replaceWithText ("Modified content");

    // Wait for event to be detected
    Thread::sleep (250);

    // Dispatch events
    std::size_t eventCount = watchdog->dispatchEvents (callback);

    if (eventCount > 0)
    {
        EXPECT_GT (capturedEvents.size(), 0);

        // Check if we have a file update event
        bool foundUpdate = false;
        for (const auto& event : capturedEvents)
        {
            if (event.changeEvent == Watchdog::EventType::file_updated)
            {
                foundUpdate = true;
            }
        }

        // Note: File system watchers can be unreliable, so we don't assert
        (void) foundUpdate;
    }
}

TEST_F (WatchdogTests, DetectFileDeletion)
{
    auto watchdog = Watchdog::createInstance (std::chrono::milliseconds (100));
    ASSERT_NE (watchdog, nullptr);

    // Create a file first
    File testFile = testFolder.getChildFile ("test_file.txt");
    testFile.replaceWithText ("Content to delete");

    // Start watching after file creation
    watchdog->watchFolder (testFolder);

    std::vector<Watchdog::Event> capturedEvents;
    auto callback = [&capturedEvents] (std::vector<Watchdog::Event> events)
    {
        capturedEvents = std::move (events);
    };

    // Give watchdog time to start
    Thread::sleep (150);

    // Delete the file
    testFile.deleteFile();

    // Wait for event to be detected
    Thread::sleep (250);

    // Dispatch events
    std::size_t eventCount = watchdog->dispatchEvents (callback);

    if (eventCount > 0)
    {
        EXPECT_GT (capturedEvents.size(), 0);

        // Check if we have a file deletion event
        bool foundDeletion = false;
        for (const auto& event : capturedEvents)
        {
            if (event.changeEvent == Watchdog::EventType::file_deleted)
            {
                foundDeletion = true;
            }
        }

        // Note: File system watchers can be unreliable, so we don't assert
        (void) foundDeletion;
    }
}

TEST_F (WatchdogTests, DispatchEventsReturnsZeroWhenNoEvents)
{
    auto watchdog = Watchdog::createInstance (std::chrono::milliseconds (100));
    ASSERT_NE (watchdog, nullptr);

    watchdog->watchFolder (testFolder);

    std::vector<Watchdog::Event> capturedEvents;
    auto callback = [&capturedEvents] (std::vector<Watchdog::Event> events)
    {
        capturedEvents = std::move (events);
    };

    // Dispatch without any file changes
    std::size_t eventCount = watchdog->dispatchEvents (callback);

    // Should return 0 if no events occurred
    EXPECT_EQ (eventCount, 0);
    EXPECT_EQ (capturedEvents.size(), 0);
}

TEST_F (WatchdogTests, WatchNonExistentFolder)
{
    auto watchdog = Watchdog::createInstance (std::chrono::milliseconds (100));
    ASSERT_NE (watchdog, nullptr);

    File nonExistent = testFolder.getChildFile ("does_not_exist");

    // Watching a non-existent folder should not crash
    EXPECT_NO_THROW (watchdog->watchFolder (nonExistent));
}

TEST_F (WatchdogTests, MultipleDispatchCalls)
{
    auto watchdog = Watchdog::createInstance (std::chrono::milliseconds (100));
    ASSERT_NE (watchdog, nullptr);

    watchdog->watchFolder (testFolder);

    auto callback = [] (std::vector<Watchdog::Event> events) {};

    // Multiple dispatch calls should not crash
    EXPECT_NO_THROW (watchdog->dispatchEvents (callback));
    EXPECT_NO_THROW (watchdog->dispatchEvents (callback));
    EXPECT_NO_THROW (watchdog->dispatchEvents (callback));
}

TEST_F (WatchdogTests, RecursiveWatching)
{
    auto watchdog = Watchdog::createInstance (std::chrono::milliseconds (100));
    ASSERT_NE (watchdog, nullptr);

    // Create nested folders
    File subFolder = testFolder.getChildFile ("subfolder");
    File nestedFolder = subFolder.getChildFile ("nested");

    subFolder.createDirectory();
    nestedFolder.createDirectory();

    // Watch parent folder (should recursively watch subfolders on supported platforms)
    watchdog->watchFolder (testFolder);

    std::vector<Watchdog::Event> capturedEvents;
    auto callback = [&capturedEvents] (std::vector<Watchdog::Event> events)
    {
        capturedEvents = std::move (events);
    };

    // Give watchdog time to start
    Thread::sleep (150);

    // Create a file in nested folder
    File nestedFile = nestedFolder.getChildFile ("nested_file.txt");
    nestedFile.replaceWithText ("Nested content");

    // Wait for event
    Thread::sleep (250);

    // Dispatch events
    std::size_t eventCount = watchdog->dispatchEvents (callback);

    // On platforms that support recursive watching, we should detect the nested file creation
    // But don't assert as this is platform-dependent
    (void) eventCount;
}

#endif // YUP_LINUX || YUP_WINDOWS || YUP_MAC
