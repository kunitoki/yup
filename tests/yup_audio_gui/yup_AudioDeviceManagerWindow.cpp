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

#include <yup_audio_gui/yup_audio_gui.h>

using namespace yup;

//==============================================================================
class AudioDeviceManagerWindowTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        manager = std::make_unique<AudioDeviceManager>();
    }

    std::unique_ptr<AudioDeviceManager> manager;
};

TEST_F (AudioDeviceManagerWindowTests, ConstructsWithoutCrash)
{
    AudioDeviceManagerWindow window (*manager);
    EXPECT_GT (window.getNumChildComponents(), 0);
}

TEST_F (AudioDeviceManagerWindowTests, ResizedDoesNotCrash)
{
    AudioDeviceManagerWindow window (*manager);
    window.setSize (500, 700);

    EXPECT_NO_THROW (window.resized());
}

TEST_F (AudioDeviceManagerWindowTests, UserTriedToCloseWindowHidesWindow)
{
    AudioDeviceManagerWindow window (*manager);

    // Closing should hide rather than destroy
    EXPECT_NO_THROW (window.userTriedToCloseWindow());
}
