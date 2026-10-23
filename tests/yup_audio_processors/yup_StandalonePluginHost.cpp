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

#include <yup_audio_processors/yup_audio_processors.h>

using namespace yup;

//==============================================================================
class StandalonePluginHostTests : public ::testing::Test
{
protected:
    struct TestHost final : StandalonePluginHost
    {
        void showAudioSettings() override { ++settingsShown; }

        void setToolbarVisible (bool shouldBeVisible) override { toolbarVisible = shouldBeVisible; }

        int settingsShown = 0;
        bool toolbarVisible = true;
    };
};

TEST_F (StandalonePluginHostTests, InstanceIsNullByDefault)
{
    EXPECT_EQ (nullptr, StandalonePluginHost::getInstance());
}

TEST_F (StandalonePluginHostTests, InstanceIsTheLiveHost)
{
    TestHost host;

    EXPECT_EQ (&host, StandalonePluginHost::getInstance());

    StandalonePluginHost::getInstance()->showAudioSettings();
    StandalonePluginHost::getInstance()->setToolbarVisible (false);

    EXPECT_EQ (1, host.settingsShown);
    EXPECT_FALSE (host.toolbarVisible);
}

TEST_F (StandalonePluginHostTests, InstanceIsNullAfterDestruction)
{
    {
        TestHost host;
        EXPECT_NE (nullptr, StandalonePluginHost::getInstance());
    }

    EXPECT_EQ (nullptr, StandalonePluginHost::getInstance());
}

#if ! YUP_ASSERTIONS_ENABLED_OR_LOGGED
TEST_F (StandalonePluginHostTests, SecondInstanceDoesNotReplaceTheFirst)
{
    TestHost first;

    {
        TestHost second;
        EXPECT_EQ (&first, StandalonePluginHost::getInstance());
    }

    EXPECT_EQ (&first, StandalonePluginHost::getInstance());
}
#endif
