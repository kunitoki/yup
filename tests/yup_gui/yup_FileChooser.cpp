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

#include <yup_gui/yup_gui.h>

using namespace yup;

namespace
{
struct CallbackTracker
{
    bool called = false;
    bool success = false;
    Array<File> results;

    void reset()
    {
        called = false;
        success = false;
        results.clear();
    }

    FileChooser::CompletionCallback makeCallback()
    {
        return [this] (bool callbackSuccess, const Array<File>& callbackResults)
        {
            called = true;
            success = callbackSuccess;
            results = callbackResults;
        };
    }
};

File makeTemporaryFile()
{
    auto file = File::getSpecialLocation (File::tempDirectory)
                    .getNonexistentChildFile ("yup_file_chooser_test", ".txt");

    EXPECT_TRUE (file.create().wasOk());
    return file;
}
} // namespace

TEST (FileChooserTests, CreateUsesHomeDirectoryWhenInitialFileIsDefault)
{
    auto chooser = FileChooser::create ("Test Dialog");
    EXPECT_NE (nullptr, chooser.get());
}

TEST (FileChooserTests, CreateAcceptsInitialDirectoryAndFilters)
{
    auto chooser = FileChooser::create ("Test Dialog",
                                        File::getSpecialLocation (File::userHomeDirectory),
                                        "*.txt;*.doc");

    EXPECT_NE (nullptr, chooser.get());
}

TEST (FileChooserTests, CreateAcceptsInitialFile)
{
    const auto file = makeTemporaryFile();
    const ScopeGuard deleteFile { [&file]
    {
        file.deleteFile();
    } };

    auto chooser = FileChooser::create ("Test Dialog", file, "*.txt");
    EXPECT_NE (nullptr, chooser.get());
}

TEST (FileChooserTests, CreateAcceptsPackageDirectoryOption)
{
    auto chooser = FileChooser::create ("Test Dialog",
                                        File::getSpecialLocation (File::userHomeDirectory),
                                        "*",
                                        true,
                                        true);

    EXPECT_NE (nullptr, chooser.get());
}

TEST (FileChooserTests, CompletionCallbackReceivesResults)
{
    CallbackTracker tracker;
    Array<File> results;
    results.add (File::getSpecialLocation (File::userHomeDirectory));

    auto callback = tracker.makeCallback();
    callback (true, results);

    EXPECT_TRUE (tracker.called);
    EXPECT_TRUE (tracker.success);
    ASSERT_EQ (1, tracker.results.size());
    EXPECT_EQ (results[0], tracker.results[0]);
}
