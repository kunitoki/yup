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

#include <yup_core/yup_core.h>

using namespace yup;

TEST (FileSearchPathTests, CopyConstructor)
{
#if YUP_WINDOWS
    const String prefix = "C:";
#else
    const String prefix = "";
#endif

    FileSearchPath fsp1 { prefix + "/a/b;" + prefix + "/c/d" };
    FileSearchPath fsp2 (fsp1);

    EXPECT_EQ (fsp1.toString(), fsp2.toString());
    EXPECT_EQ (fsp2.getNumPaths(), 2);
}

TEST (FileSearchPathTests, CopyAssignment)
{
#if YUP_WINDOWS
    const String prefix = "C:";
#else
    const String prefix = "";
#endif

    FileSearchPath fsp1 { prefix + "/a/b;" + prefix + "/c/d" };
    FileSearchPath fsp2;
    fsp2 = fsp1;

    EXPECT_EQ (fsp1.toString(), fsp2.toString());
    EXPECT_EQ (fsp2.getNumPaths(), 2);
}

TEST (FileSearchPathTests, StringAssignment)
{
#if YUP_WINDOWS
    const String prefix = "C:";
#else
    const String prefix = "";
#endif

    FileSearchPath fsp;
    fsp = prefix + "/a/b;" + prefix + "/c/d";

    EXPECT_EQ (fsp.getNumPaths(), 2);
    EXPECT_EQ (fsp[0].getFullPathName(), prefix + "/a/b");
    EXPECT_EQ (fsp[1].getFullPathName(), prefix + "/c/d");
}

TEST (FileSearchPathTests, GetNumPaths)
{
#if YUP_WINDOWS
    const String prefix = "C:";
#else
    const String prefix = "";
#endif

    FileSearchPath empty;
    EXPECT_EQ (empty.getNumPaths(), 0);

    FileSearchPath fsp { prefix + "/a/b;" + prefix + "/c/d;" + prefix + "/e/f" };
    EXPECT_EQ (fsp.getNumPaths(), 3);
}

TEST (FileSearchPathTests, IndexOperator)
{
#if YUP_WINDOWS
    const String prefix = "C:";
#else
    const String prefix = "";
#endif

    FileSearchPath fsp { prefix + "/a/b;" + prefix + "/c/d" };
    EXPECT_EQ (fsp[0].getFullPathName(), prefix + "/a/b");
    EXPECT_EQ (fsp[1].getFullPathName(), prefix + "/c/d");
}

TEST (FileSearchPathTests, GetRawString)
{
#if YUP_WINDOWS
    const String prefix = "C:";
#else
    const String prefix = "";
#endif

    FileSearchPath fsp { prefix + "/a/b;" + prefix + "/c/d" };
    EXPECT_EQ (fsp.getRawString (0), prefix + "/a/b");
    EXPECT_EQ (fsp.getRawString (1), prefix + "/c/d");
}

TEST (FileSearchPathTests, Add)
{
#if YUP_WINDOWS
    const String prefix = "C:";
#else
    const String prefix = "";
#endif

    FileSearchPath fsp;
    fsp.add (File (prefix + "/a/b"));
    fsp.add (File (prefix + "/c/d"));

    EXPECT_EQ (fsp.getNumPaths(), 2);
    EXPECT_EQ (fsp[0].getFullPathName(), prefix + "/a/b");
    EXPECT_EQ (fsp[1].getFullPathName(), prefix + "/c/d");
}

TEST (FileSearchPathTests, AddWithIndex)
{
#if YUP_WINDOWS
    const String prefix = "C:";
#else
    const String prefix = "";
#endif

    FileSearchPath fsp { prefix + "/a/b;" + prefix + "/c/d" };
    fsp.add (File (prefix + "/e/f"), 1);

    EXPECT_EQ (fsp.getNumPaths(), 3);
    EXPECT_EQ (fsp[0].getFullPathName(), prefix + "/a/b");
    EXPECT_EQ (fsp[1].getFullPathName(), prefix + "/e/f");
    EXPECT_EQ (fsp[2].getFullPathName(), prefix + "/c/d");
}

TEST (FileSearchPathTests, AddIfNotAlreadyThere)
{
#if YUP_WINDOWS
    const String prefix = "C:";
#else
    const String prefix = "";
#endif

    FileSearchPath fsp { prefix + "/a/b;" + prefix + "/c/d" };

    EXPECT_TRUE (fsp.addIfNotAlreadyThere (File (prefix + "/e/f")));
    EXPECT_EQ (fsp.getNumPaths(), 3);

    EXPECT_FALSE (fsp.addIfNotAlreadyThere (File (prefix + "/a/b")));
    EXPECT_EQ (fsp.getNumPaths(), 3);
}

TEST (FileSearchPathTests, Remove)
{
#if YUP_WINDOWS
    const String prefix = "C:";
#else
    const String prefix = "";
#endif

    FileSearchPath fsp { prefix + "/a/b;" + prefix + "/c/d;" + prefix + "/e/f" };
    fsp.remove (1);

    EXPECT_EQ (fsp.getNumPaths(), 2);
    EXPECT_EQ (fsp[0].getFullPathName(), prefix + "/a/b");
    EXPECT_EQ (fsp[1].getFullPathName(), prefix + "/e/f");
}

TEST (FileSearchPathTests, AddPath)
{
#if YUP_WINDOWS
    const String prefix = "C:";
#else
    const String prefix = "";
#endif

    FileSearchPath fsp1 { prefix + "/a/b;" + prefix + "/c/d" };
    FileSearchPath fsp2 { prefix + "/e/f;" + prefix + "/g/h" };

    fsp1.addPath (fsp2);

    EXPECT_EQ (fsp1.getNumPaths(), 4);
    EXPECT_EQ (fsp1[2].getFullPathName(), prefix + "/e/f");
    EXPECT_EQ (fsp1[3].getFullPathName(), prefix + "/g/h");
}

TEST (FileSearchPathTests, AddPathSkipsDuplicates)
{
#if YUP_WINDOWS
    const String prefix = "C:";
#else
    const String prefix = "";
#endif

    FileSearchPath fsp1 { prefix + "/a/b;" + prefix + "/c/d" };
    FileSearchPath fsp2 { prefix + "/c/d;" + prefix + "/e/f" };

    fsp1.addPath (fsp2);

    EXPECT_EQ (fsp1.getNumPaths(), 3);
    EXPECT_EQ (fsp1[0].getFullPathName(), prefix + "/a/b");
    EXPECT_EQ (fsp1[1].getFullPathName(), prefix + "/c/d");
    EXPECT_EQ (fsp1[2].getFullPathName(), prefix + "/e/f");
}

TEST (FileSearchPathTests, RemoveNonExistentPaths)
{
#if YUP_WINDOWS
    const String prefix = "C:";
#else
    const String prefix = "";
#endif

    auto tempDir = File::getSpecialLocation (File::SpecialLocationType::tempDirectory);
    auto existingPath = tempDir.getChildFile ("test_existing_path");
    existingPath.createDirectory();

    FileSearchPath fsp;
    fsp.add (existingPath);
    fsp.add (File (prefix + "/nonexistent/path/12345"));

    EXPECT_EQ (fsp.getNumPaths(), 2);

    fsp.removeNonExistentPaths();

    EXPECT_EQ (fsp.getNumPaths(), 1);
    EXPECT_EQ (fsp[0].getFullPathName(), existingPath.getFullPathName());

    existingPath.deleteFile();
}

TEST (FileSearchPathTests, FindChildFilesArray)
{
    auto tempDir = File::getSpecialLocation (File::SpecialLocationType::tempDirectory);
    auto testDir = tempDir.getChildFile ("test_find_child_files");
    testDir.createDirectory();

    auto subDir = testDir.getChildFile ("subdir");
    subDir.createDirectory();

    auto file1 = testDir.getChildFile ("test1.txt");
    auto file2 = testDir.getChildFile ("test2.txt");
    auto file3 = subDir.getChildFile ("test3.txt");

    file1.create();
    file2.create();
    file3.create();

    FileSearchPath fsp;
    fsp.add (testDir);

    auto files = fsp.findChildFiles (File::findFiles, false, "*.txt");
    EXPECT_EQ (files.size(), 2);

    auto filesRecursive = fsp.findChildFiles (File::findFiles, true, "*.txt");
    EXPECT_EQ (filesRecursive.size(), 3);

    file1.deleteFile();
    file2.deleteFile();
    file3.deleteFile();
    subDir.deleteFile();
    testDir.deleteFile();
}

TEST (FileSearchPathTests, FindChildFilesWithResults)
{
    auto tempDir = File::getSpecialLocation (File::SpecialLocationType::tempDirectory);
    auto testDir = tempDir.getChildFile ("test_find_child_files2");
    testDir.createDirectory();

    auto file1 = testDir.getChildFile ("test1.txt");
    auto file2 = testDir.getChildFile ("test2.txt");

    file1.create();
    file2.create();

    FileSearchPath fsp;
    fsp.add (testDir);

    Array<File> results;
    int count = fsp.findChildFiles (results, File::findFiles, false, "*.txt");

    EXPECT_EQ (count, 2);
    EXPECT_EQ (results.size(), 2);

    file1.deleteFile();
    file2.deleteFile();
    testDir.deleteFile();
}

TEST (FileSearchPathTests, IsFileInPathNonRecursive)
{
    auto tempDir = File::getSpecialLocation (File::SpecialLocationType::tempDirectory);
    auto testDir = tempDir.getChildFile ("test_is_file_in_path");
    testDir.createDirectory();

    auto subDir = testDir.getChildFile ("subdir");
    subDir.createDirectory();

    auto file1 = testDir.getChildFile ("test1.txt");
    auto file2 = subDir.getChildFile ("test2.txt");

    file1.create();
    file2.create();

    FileSearchPath fsp;
    fsp.add (testDir);

    EXPECT_TRUE (fsp.isFileInPath (file1, false));
    EXPECT_FALSE (fsp.isFileInPath (file2, false));

    file1.deleteFile();
    file2.deleteFile();
    subDir.deleteFile();
    testDir.deleteFile();
}

TEST (FileSearchPathTests, IsFileInPathRecursive)
{
    auto tempDir = File::getSpecialLocation (File::SpecialLocationType::tempDirectory);
    auto testDir = tempDir.getChildFile ("test_is_file_in_path_rec");
    testDir.createDirectory();

    auto subDir = testDir.getChildFile ("subdir");
    subDir.createDirectory();

    auto file1 = testDir.getChildFile ("test1.txt");
    auto file2 = subDir.getChildFile ("test2.txt");

    file1.create();
    file2.create();

    FileSearchPath fsp;
    fsp.add (testDir);

    EXPECT_TRUE (fsp.isFileInPath (file1, true));
    EXPECT_TRUE (fsp.isFileInPath (file2, true));

    file1.deleteFile();
    file2.deleteFile();
    subDir.deleteFile();
    testDir.deleteFile();
}

TEST (FileSearchPathTests, RemoveRedundantPaths)
{
#if YUP_WINDOWS
    const String prefix = "C:";
#else
    const String prefix = "";
#endif

    {
        FileSearchPath fsp { prefix + "/a/b/c/d;" + prefix + "/a/b/c/e;" + prefix + "/a/b/c" };
        fsp.removeRedundantPaths();
        EXPECT_EQ (fsp.toString(), prefix + "/a/b/c");
    }

    {
        FileSearchPath fsp { prefix + "/a/b/c;" + prefix + "/a/b/c/d;" + prefix + "/a/b/c/e" };
        fsp.removeRedundantPaths();
        EXPECT_EQ (fsp.toString(), prefix + "/a/b/c");
    }

    {
        FileSearchPath fsp { prefix + "/a/b/c/d;" + prefix + "/a/b/c;" + prefix + "/a/b/c/e" };
        fsp.removeRedundantPaths();
        EXPECT_EQ (fsp.toString(), prefix + "/a/b/c");
    }

    {
        FileSearchPath fsp { "%FOO%;" + prefix + "/a/b/c;%FOO%;" + prefix + "/a/b/c/d" };
        fsp.removeRedundantPaths();
        EXPECT_EQ (fsp.toString(), "%FOO%;" + prefix + "/a/b/c");
    }
}
