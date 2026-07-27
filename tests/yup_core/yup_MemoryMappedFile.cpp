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

#if ! YUP_WASM

class MemoryMappedFileTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        tempFile = File::getSpecialLocation (File::tempDirectory)
                       .getChildFile ("YUP_MemoryMappedFileTest_" + String::toHexString (Random::getSystemRandom().nextInt()));

        // Create a test file with known content
        testData = "This is test data for memory mapped file testing.";
        tempFile.replaceWithText (testData);
    }

    void TearDown() override
    {
        tempFile.deleteFile();
    }

    File tempFile;
    String testData;
};

TEST_F (MemoryMappedFileTests, ReadOnlyMapping)
{
    MemoryMappedFile mmf (tempFile, MemoryMappedFile::readOnly);

    EXPECT_NE (mmf.getData(), nullptr);
    EXPECT_GT (mmf.getSize(), 0);

    // Verify content
    String content (static_cast<const char*> (mmf.getData()), static_cast<size_t> (mmf.getSize()));
    EXPECT_EQ (content, testData);
}

TEST_F (MemoryMappedFileTests, ReadWriteMapping)
{
    MemoryMappedFile mmf (tempFile, MemoryMappedFile::readWrite);

    EXPECT_NE (mmf.getData(), nullptr);
    EXPECT_GT (mmf.getSize(), 0);

    // Modify content (if system allows)
    if (mmf.getData() != nullptr)
    {
        char* data = static_cast<char*> (mmf.getData());
        if (data[0] != '\0')
        {
            char original = data[0];
            data[0] = 'X';
            EXPECT_EQ (data[0], 'X');
            data[0] = original; // Restore
        }
    }
}

TEST_F (MemoryMappedFileTests, RangeMapping)
{
    // Map only part of the file
    Range<int64> range (5, 15);
    MemoryMappedFile mmf (tempFile, range, MemoryMappedFile::readOnly);

    EXPECT_NE (mmf.getData(), nullptr);

    // The actual range may be adjusted for page alignment
    // so use getRange() to get the actual range
    auto actualRange = mmf.getRange();
    EXPECT_GE (actualRange.getEnd(), range.getEnd());
    EXPECT_LE (actualRange.getStart(), range.getStart());

    // Size should be at least the requested range length
    EXPECT_GE (mmf.getSize(), range.getLength());
}

TEST_F (MemoryMappedFileTests, NonExistentFile)
{
    File nonExistent = tempFile.getSiblingFile ("nonexistent_file.dat");
    MemoryMappedFile mmf (nonExistent, MemoryMappedFile::readOnly);

    // Should handle gracefully
    EXPECT_TRUE (mmf.getData() == nullptr || mmf.getSize() == 0);
}

TEST_F (MemoryMappedFileTests, ExclusiveMapping)
{
    MemoryMappedFile mmf (tempFile, MemoryMappedFile::readOnly, true);

    EXPECT_NE (mmf.getData(), nullptr);
    EXPECT_GT (mmf.getSize(), 0);
}

TEST_F (MemoryMappedFileTests, LargeRangeStart)
{
    // Test with range start > 0 to hit the page alignment code
    int64 pageSize = 4096;             // Common page size
    int64 offset = pageSize * 2 + 100; // Start at offset that needs alignment

    // Create larger file
    String largeData;
    for (int i = 0; i < 10000; ++i)
        largeData += "Test data line " + String (i) + "\n";

    tempFile.replaceWithText (largeData);

    Range<int64> range (offset, offset + 1000);
    MemoryMappedFile mmf (tempFile, range, MemoryMappedFile::readOnly);

    // Should handle page alignment
    EXPECT_TRUE (mmf.getData() != nullptr || mmf.getSize() == 0);
}

#endif // ! YUP_WASM
