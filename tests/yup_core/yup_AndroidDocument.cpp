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

class AndroidDocumentTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        tempFolder = File::getSpecialLocation (File::tempDirectory)
                         .getChildFile ("YUP_AndroidDocumentTests_" + String::toHexString (Random::getSystemRandom().nextInt()));

        childFolder = tempFolder.getChildFile ("child");
        ASSERT_TRUE (childFolder.createDirectory().wasOk());
    }

    void TearDown() override
    {
        tempFolder.deleteRecursively();
    }

    File tempFolder;
    File childFolder;
};

TEST_F (AndroidDocumentTests, CreatesChildDirectoryWithValidName)
{
    auto parent = AndroidDocument::fromFile (childFolder);

    EXPECT_TRUE (parent.createChildDirectory ("valid").hasValue());
    EXPECT_TRUE (childFolder.getChildFile ("valid").isDirectory());
}

TEST_F (AndroidDocumentTests, RejectsChildNamesEscapingTheParent)
{
    auto parent = AndroidDocument::fromFile (childFolder);

    EXPECT_FALSE (parent.createChildDirectory ("").hasValue());
    EXPECT_FALSE (parent.createChildDirectory (".").hasValue());
    EXPECT_FALSE (parent.createChildDirectory ("..").hasValue());
    EXPECT_FALSE (parent.createChildDirectory ("../escape").hasValue());
    EXPECT_FALSE (parent.createChildDirectory ("nested/child").hasValue());
    EXPECT_FALSE (parent.createChildDirectory (tempFolder.getChildFile ("absolute").getFullPathName()).hasValue());

    EXPECT_FALSE (tempFolder.getChildFile ("escape").exists());
    EXPECT_FALSE (tempFolder.getChildFile ("absolute").exists());
}

TEST_F (AndroidDocumentTests, RenamesWithValidName)
{
    auto file = childFolder.getChildFile ("file.txt");
    ASSERT_TRUE (file.create().wasOk());

    auto document = AndroidDocument::fromFile (file);

    EXPECT_TRUE (document.renameTo ("renamed.txt"));
    EXPECT_TRUE (childFolder.getChildFile ("renamed.txt").existsAsFile());
}

TEST_F (AndroidDocumentTests, RejectsRenamesEscapingTheParent)
{
    auto file = childFolder.getChildFile ("file.txt");
    ASSERT_TRUE (file.create().wasOk());

    auto document = AndroidDocument::fromFile (file);

    EXPECT_FALSE (document.renameTo ("../renamed.txt"));
    EXPECT_FALSE (document.renameTo (".."));
    EXPECT_FALSE (document.renameTo (tempFolder.getChildFile ("absolute.txt").getFullPathName()));

    EXPECT_TRUE (file.existsAsFile());
    EXPECT_FALSE (tempFolder.getChildFile ("renamed.txt").exists());
    EXPECT_FALSE (tempFolder.getChildFile ("absolute.txt").exists());
}
