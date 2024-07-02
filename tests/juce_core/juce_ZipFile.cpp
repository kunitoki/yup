/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include <gtest/gtest.h>

#include <juce_core/juce_core.h>

using namespace juce;

class ZipFileTests : public ::testing::Test
{
protected:
    File getNonExistingZipFile() const
    {
        return File::getCurrentWorkingDirectory().getChildFile ("test.zip");
    }

    MemoryBlock createZipMemoryBlock (const StringArray& entryNames) const
    {
        ZipFile::Builder builder;
        HashMap<String, MemoryBlock> blocks;

        for (auto& entryName : entryNames)
        {
            auto& block = blocks.getReference (entryName);

            MemoryOutputStream mo (block, false);
            mo << entryName;
            mo.flush();

            builder.addEntry (new MemoryInputStream (block, false), 9, entryName, Time::getCurrentTime());
        }

        MemoryBlock data;
        MemoryOutputStream mo (data, false);
        builder.writeToStream (mo, nullptr);

        return data;
    }
};

TEST_F (ZipFileTests, BasicZipFileFunctionality)
{
    StringArray entryNames { "first", "second", "third" };
    auto data = createZipMemoryBlock (entryNames);
    MemoryInputStream mi (data, false);
    ZipFile zip (mi);

    EXPECT_EQ (zip.getNumEntries(), entryNames.size());

    for (const auto& entryName : entryNames)
    {
        const ZipFile::ZipEntry* entry = zip.getEntry (entryName);
        ASSERT_NE (entry, nullptr);

        std::unique_ptr<InputStream> input (zip.createStreamForEntry (*entry));
        ASSERT_NE (input, nullptr);
        EXPECT_EQ (input->readEntireStreamAsString(), entryName);
    }
}

TEST_F (ZipFileTests, ZipFileSlipTest)
{
    const std::map<String, bool> testCases = {
        { "a", true },
#if JUCE_WINDOWS
        { "C:/b", false },
#else
        { "/b", false },
#endif
        { "c/d", true },
        { "../e/f", false },
        { "../../g/h", false },
        { "i/../j", true },
        { "k/l/../", true },
        { "m/n/../../", false },
        { "o/p/../../../", false }
    };

    StringArray entryNames;

    for (const auto& testCase : testCases)
        entryNames.add (testCase.first);

    TemporaryFile tmpDir;
    tmpDir.getFile().createDirectory();
    auto data = createZipMemoryBlock (entryNames);
    MemoryInputStream mi (data, false);
    ZipFile zip (mi);

    for (int i = 0; i < zip.getNumEntries(); ++i)
    {
        const auto result = zip.uncompressEntry (i, tmpDir.getFile());
        const auto caseIt = testCases.find (zip.getEntry (i)->filename);

        if (caseIt != testCases.end())
        {
            EXPECT_EQ (result.wasOk(), caseIt->second)
                << zip.getEntry (i)->filename << " was unexpectedly " << (result.wasOk() ? "OK" : "not OK");
        }
        else
        {
            ADD_FAILURE() << "Test case not found for " << zip.getEntry (i)->filename;
        }
    }
}

TEST_F (ZipFileTests, CreateFromFile)
{
    File zipFile = getNonExistingZipFile();
    ZipFile zip (zipFile);

    EXPECT_EQ (zip.getNumEntries(), 0); // Assumes the test.zip is empty or non-existent
}

TEST_F (ZipFileTests, CreateFromInputStream)
{
    /*
    File zipFile = getNonExistingZipFile();
    FileInputStream fileStream(zipFile);
    ZipFile zip(&fileStream, false);

    EXPECT_EQ(zip.getNumEntries(), 0); // Assumes the test.zip is empty or non-existent
    */
}

TEST_F (ZipFileTests, CreateFromInputStreamNoOwnership)
{
    /*
    File zipFile = getNonExistingZipFile();
    FileInputStream fileStream(zipFile);
    ZipFile zip(fileStream);

    EXPECT_EQ(zip.getNumEntries(), 0); // Assumes the test.zip is empty or non-existent
    */
}

TEST_F (ZipFileTests, CreateFromInputSource)
{
    class TestInputSource : public InputSource
    {
    public:
        InputStream* createInputStream() override { return nullptr; }

        InputStream* createInputStreamFor (const String&) override { return nullptr; }

        int64 hashCode() const override { return 0; }
    };

    auto* inputSource = new TestInputSource;
    ZipFile zip (inputSource);

    EXPECT_EQ (zip.getNumEntries(), 0); // Assumes the TestInputSource returns null streams
}

TEST_F (ZipFileTests, GetNumEntries)
{
    File zipFile = getNonExistingZipFile();
    ZipFile zip (zipFile);

    EXPECT_EQ (zip.getNumEntries(), 0); // Assumes the test.zip is empty or non-existent
}

TEST_F (ZipFileTests, GetEntryByIndex)
{
    File zipFile = getNonExistingZipFile();
    ZipFile zip (zipFile);

    EXPECT_EQ (zip.getEntry (0), nullptr); // Assumes the test.zip is empty or non-existent
}

TEST_F (ZipFileTests, GetEntryByName)
{
    File zipFile = getNonExistingZipFile();
    ZipFile zip (zipFile);

    EXPECT_EQ (zip.getEntry ("nonexistent.txt"), nullptr); // Assumes the test.zip does not contain this file
}

TEST_F (ZipFileTests, GetIndexOfFileName)
{
    File zipFile = getNonExistingZipFile();
    ZipFile zip (zipFile);

    EXPECT_EQ (zip.getIndexOfFileName ("nonexistent.txt"), -1); // Assumes the test.zip does not contain this file
}

TEST_F (ZipFileTests, SortEntriesByFilename)
{
    File zipFile = getNonExistingZipFile();
    ZipFile zip (zipFile);

    zip.sortEntriesByFilename();
    // No direct way to verify sorting, but we can call it to ensure it doesn't crash
}

TEST_F (ZipFileTests, CreateStreamForEntryByIndex)
{
    File zipFile = getNonExistingZipFile();
    ZipFile zip (zipFile);

    EXPECT_EQ (zip.createStreamForEntry (0), nullptr); // Assumes the test.zip is empty or non-existent
}

TEST_F (ZipFileTests, CreateStreamForEntryByName)
{
    File zipFile = getNonExistingZipFile();
    ZipFile zip (zipFile);

    const ZipFile::ZipEntry* entry = zip.getEntry ("nonexistent.txt");
    EXPECT_EQ (zip.createStreamForEntry (*entry), nullptr); // Assumes the test.zip does not contain this file
}

TEST_F (ZipFileTests, UncompressTo)
{
    /*
    File zipFile = getNonExistingZipFile();
    ZipFile zip(zipFile);
    File targetDirectory(File::getCurrentWorkingDirectory().getChildFile("unzip_test"));

    Result result = zip.uncompressTo(targetDirectory, true);
    EXPECT_FALSE(result.wasOk()); // Assumes the test.zip is empty or non-existent
    */
}

TEST_F (ZipFileTests, UncompressEntry)
{
    /*
    File zipFile = getNonExistingZipFile();
    ZipFile zip(zipFile);
    File targetDirectory(File::getCurrentWorkingDirectory().getChildFile("unzip_test"));

    Result result = zip.uncompressEntry(0, targetDirectory, true);
    EXPECT_FALSE(result.wasOk()); // Assumes the test.zip is empty or non-existent
    */
}

TEST_F (ZipFileTests, BuilderAddFile)
{
    ZipFile::Builder builder;
    File fileToAdd (File::getCurrentWorkingDirectory().getChildFile ("test.txt"));

    builder.addFile (fileToAdd, 9, "test.txt");
}

TEST_F (ZipFileTests, BuilderAddEntry)
{
    ZipFile::Builder builder;
    auto* memoryStream = new MemoryInputStream ("dummy data", 10, false);

    builder.addEntry (memoryStream, 9, "dummy.txt", Time::getCurrentTime());
}

TEST_F (ZipFileTests, BuilderWriteToStream)
{
    ZipFile::Builder builder;
    MemoryOutputStream outputStream;

    bool result = builder.writeToStream (outputStream, nullptr);
    EXPECT_TRUE (result); // Expecting true even if the builder is empty
}
