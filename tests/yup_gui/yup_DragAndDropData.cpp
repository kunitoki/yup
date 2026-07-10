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

#include <yup_gui/yup_gui.h>

#include <gtest/gtest.h>

using namespace yup;

TEST (DragAndDropDataTests, DefaultIsEmpty)
{
    DragAndDropData data;

    EXPECT_TRUE (data.isEmpty());
    EXPECT_FALSE (data.hasFiles());
    EXPECT_FALSE (data.hasText());
    EXPECT_FALSE (data.hasUris());
    EXPECT_TRUE (data.getFiles().isEmpty());
    EXPECT_TRUE (data.getText().isEmpty());
    EXPECT_TRUE (data.getUris().isEmpty());
}

TEST (DragAndDropDataTests, WithFilesSetsFiles)
{
    Array<File> files;
    files.add (File ("/tmp/one.txt"));
    files.add (File ("/tmp/two.txt"));

    auto data = DragAndDropData().withFiles (files);

    EXPECT_TRUE (data.hasFiles());
    EXPECT_FALSE (data.isEmpty());
    EXPECT_EQ (data.getFiles().size(), 2);
    EXPECT_EQ (data.getFiles()[0], File ("/tmp/one.txt"));
}

TEST (DragAndDropDataTests, WithTextSetsText)
{
    auto data = DragAndDropData().withText ("hello");

    EXPECT_TRUE (data.hasText());
    EXPECT_FALSE (data.isEmpty());
    EXPECT_EQ (data.getText(), String ("hello"));
}

TEST (DragAndDropDataTests, WithUrisSetsUris)
{
    StringArray uris;
    uris.add ("https://example.com");

    auto data = DragAndDropData().withUris (uris);

    EXPECT_TRUE (data.hasUris());
    EXPECT_FALSE (data.isEmpty());
    EXPECT_EQ (data.getUris().size(), 1);
    EXPECT_EQ (data.getUris()[0], String ("https://example.com"));
}

TEST (DragAndDropDataTests, BuildersAreImmutable)
{
    DragAndDropData original;
    auto withText = original.withText ("hello");

    EXPECT_TRUE (original.isEmpty());
    EXPECT_TRUE (withText.hasText());
}

TEST (DragAndDropDataTests, BuildersChainAndPreservePreviousValues)
{
    Array<File> files;
    files.add (File ("/tmp/one.txt"));

    auto data = DragAndDropData()
                    .withFiles (files)
                    .withText ("hello");

    EXPECT_TRUE (data.hasFiles());
    EXPECT_TRUE (data.hasText());
    EXPECT_EQ (data.getFiles().size(), 1);
    EXPECT_EQ (data.getText(), String ("hello"));
}

TEST (DragAndDropDataTests, EmptyTextDoesNotCountAsText)
{
    auto data = DragAndDropData().withText ("");

    EXPECT_FALSE (data.hasText());
    EXPECT_TRUE (data.isEmpty());
}
