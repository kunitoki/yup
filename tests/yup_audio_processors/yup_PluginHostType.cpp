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

TEST (PluginHostTypeTests, DefaultIsUnknown)
{
    PluginHostType hostType;
    EXPECT_TRUE (hostType.isUnknown());
    EXPECT_EQ (String ("Unknown"), String (hostType.getHostName()));
}

TEST (PluginHostTypeTests, DetectsFromHostReportedName)
{
    EXPECT_TRUE (PluginHostType::detect ("Bitwig Studio", {}).isBitwigStudio());
    EXPECT_TRUE (PluginHostType::detect ("REAPER", {}).isReaper());
    EXPECT_TRUE (PluginHostType::detect ("Cubase 13", {}).isCubaseOrNuendo());
    EXPECT_TRUE (PluginHostType::detect ("Nuendo", {}).isCubaseOrNuendo());
    EXPECT_TRUE (PluginHostType::detect ("renoise", {}).isRenoise());
    EXPECT_TRUE (PluginHostType::detect ("Studio One", {}).isStudioOne());
}

TEST (PluginHostTypeTests, DetectsFromExecutablePathWhenNameIsUnknown)
{
    EXPECT_TRUE (PluginHostType::detect ("", "/Applications/Ableton Live 12 Suite.app/Contents/MacOS/Live").isAbletonLive());
    EXPECT_TRUE (PluginHostType::detect ("Some Host", "/Applications/Logic Pro X.app/Contents/MacOS/Logic Pro X").isAppleLogic());
    EXPECT_TRUE (PluginHostType::detect ("", "C:\\Program Files\\Image-Line\\FL Studio 21\\FL64.exe").isFLStudio());
}

TEST (PluginHostTypeTests, HostNameTakesPrecedenceOverExecutable)
{
    EXPECT_TRUE (PluginHostType::detect ("REAPER", "/usr/bin/bitwig-studio").isReaper());
}

TEST (PluginHostTypeTests, ValidatorsAreReported)
{
    EXPECT_TRUE (PluginHostType::detect ("", "/usr/bin/auvaltool").isValidator());
    EXPECT_TRUE (PluginHostType::detect ("pluginval", {}).isValidator());
    EXPECT_FALSE (PluginHostType::detect ("REAPER", {}).isValidator());
}

TEST (PluginHostTypeTests, UnmatchedInputsStayUnknown)
{
    EXPECT_TRUE (PluginHostType::detect ("", "").isUnknown());
    EXPECT_TRUE (PluginHostType::detect ("My Custom Host", "/opt/custom/host").isUnknown());
}

TEST (PluginHostTypeTests, IgnoresUnrelatedFoldersInExecutablePath)
{
    EXPECT_TRUE (PluginHostType::detect ("", "/home/carla/.local/opt/SomeDAW/SomeDAW").isUnknown());
    EXPECT_TRUE (PluginHostType::detect ("", "/Users/reason/Applications/Some Host.app/Contents/MacOS/Some Host").isUnknown());
    EXPECT_TRUE (PluginHostType::detect ("", "C:\\Users\\reason\\Some Host\\host.exe").isUnknown());
}

TEST (PluginHostTypeTests, KnownHostsReportDisplayNames)
{
    EXPECT_EQ (String ("REAPER"), String (PluginHostType (PluginHostType::Host::reaper).getHostName()));
    EXPECT_EQ (String ("Cubase"), String (PluginHostType (PluginHostType::Host::steinbergCubase).getHostName()));
    EXPECT_EQ (String ("YUP Plugin Host"), String (PluginHostType (PluginHostType::Host::yupPluginHost).getHostName()));
}

TEST (PluginHostTypeTests, DetectsFromParentFolderAndAppBundle)
{
    EXPECT_TRUE (PluginHostType::detect ("", "/opt/Renoise/launcher").isRenoise());
    EXPECT_TRUE (PluginHostType::detect ("", "/Applications/GarageBand.app/Contents/MacOS/Helper").isAppleGarageBand());
    EXPECT_TRUE (PluginHostType::detect ("", "/usr/local/bin/Live").isAbletonLive());
    EXPECT_TRUE (PluginHostType::detect ("", "C:\\Program Files\\Renoise\\launcher.exe").isRenoise());
}
