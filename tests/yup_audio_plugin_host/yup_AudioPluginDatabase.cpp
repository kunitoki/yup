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

#include <yup_audio_plugin_host/yup_audio_plugin_host.h>

using namespace yup;

class AudioPluginDatabaseTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        directory = File::createTempFile ("yup_plugin_database");
        directory.createDirectory();

        plugin = directory.getChildFile ("Synth.clap");
        plugin.replaceWithText ("binary");

        ASSERT_TRUE (database.open (databaseFile()).wasOk());
    }

    void TearDown() override
    {
        database.close();
        directory.deleteRecursively();
    }

    File databaseFile() const { return directory.getChildFile ("plugins.db"); }

    static AudioPluginDescription makeDescription (const File& file)
    {
        AudioPluginDescription description;
        description.formatType = AudioPluginFormatType::clap;
        description.name = "Synth";
        description.vendor = "YUP";
        description.version = "1.0.0";
        description.category = "Instrument";
        description.identifier = "org.yup.synth";
        description.fileOrBundlePath = file.getFullPathName();
        description.isInstrument = true;
        description.numOutputChannels = 2;
        description.numMidiInputPorts = 1;
        return description;
    }

    File directory;
    File plugin;
    AudioPluginDatabase database;
};

TEST_F (AudioPluginDatabaseTests, StoredResultsAreUpToDateAndRoundTrip)
{
    EXPECT_EQ (database.getFileState (plugin, AudioPluginFormatType::clap), AudioPluginDatabase::FileState::needsScan);

    ASSERT_TRUE (database.storeScanResult (plugin, AudioPluginFormatType::clap, { makeDescription (plugin) }).wasOk());

    EXPECT_EQ (database.getFileState (plugin, AudioPluginFormatType::clap), AudioPluginDatabase::FileState::upToDate);
    EXPECT_EQ (database.getFileState (plugin, AudioPluginFormatType::vst3), AudioPluginDatabase::FileState::needsScan);

    const auto plugins = database.getPlugins (plugin, AudioPluginFormatType::clap);
    ASSERT_EQ (plugins.size(), 1u);
    EXPECT_EQ (plugins.front(), makeDescription (plugin));
    EXPECT_EQ (database.getPlugins().size(), 1u);
}

TEST_F (AudioPluginDatabaseTests, ChangedFilesAndScanVersionsNeedScan)
{
    ASSERT_TRUE (database.storeScanResult (plugin, AudioPluginFormatType::clap, { makeDescription (plugin) }).wasOk());

    database.setScanVersion (2);
    EXPECT_EQ (database.getFileState (plugin, AudioPluginFormatType::clap), AudioPluginDatabase::FileState::needsScan);

    ASSERT_TRUE (database.storeScanResult (plugin, AudioPluginFormatType::clap, { makeDescription (plugin) }).wasOk());
    plugin.replaceWithText ("updated binary");
    EXPECT_EQ (database.getFileState (plugin, AudioPluginFormatType::clap), AudioPluginDatabase::FileState::needsScan);
}

TEST_F (AudioPluginDatabaseTests, ErrorsAreCachedButNotBlacklisted)
{
    ASSERT_TRUE (database.storeScanResult (plugin, AudioPluginFormatType::clap, { makeDescription (plugin) }).wasOk());
    ASSERT_TRUE (database.storeScanFailure (plugin, AudioPluginFormatType::clap, AudioPluginDatabase::FailureReason::error, "broken").wasOk());
    ASSERT_TRUE (database.storeScanFailure (plugin, AudioPluginFormatType::clap, AudioPluginDatabase::FailureReason::error, "still broken").wasOk());

    EXPECT_EQ (database.getFileState (plugin, AudioPluginFormatType::clap), AudioPluginDatabase::FileState::failed);
    EXPECT_TRUE (database.getPlugins().empty());

    const auto failures = database.getFailures();
    ASSERT_EQ (failures.size(), 1u);
    EXPECT_EQ (failures.front().file, plugin);
    EXPECT_EQ (failures.front().message, "still broken");
    EXPECT_EQ (failures.front().count, 2);
    EXPECT_FALSE (failures.front().blacklisted);
}

TEST_F (AudioPluginDatabaseTests, InterruptedScanIsBlacklistedOnReopen)
{
    ASSERT_TRUE (database.beginScan (plugin, AudioPluginFormatType::clap).wasOk());
    database.close();

    ASSERT_TRUE (database.open (databaseFile()).wasOk());
    EXPECT_EQ (database.getFileState (plugin, AudioPluginFormatType::clap), AudioPluginDatabase::FileState::blacklisted);

    const auto failures = database.getFailures();
    ASSERT_EQ (failures.size(), 1u);
    EXPECT_EQ (failures.front().reason, AudioPluginDatabase::FailureReason::crash);
    EXPECT_TRUE (failures.front().blacklisted);

    ASSERT_TRUE (database.clearBlacklist (plugin).wasOk());
    EXPECT_EQ (database.getFileState (plugin, AudioPluginFormatType::clap), AudioPluginDatabase::FileState::needsScan);
}

TEST_F (AudioPluginDatabaseTests, BlacklistIsLiftedWhenTheFileChanges)
{
    ASSERT_TRUE (database.storeScanFailure (plugin, AudioPluginFormatType::clap, AudioPluginDatabase::FailureReason::timeout, "hung").wasOk());
    EXPECT_EQ (database.getFileState (plugin, AudioPluginFormatType::clap), AudioPluginDatabase::FileState::blacklisted);

    plugin.replaceWithText ("fixed binary");
    EXPECT_EQ (database.getFileState (plugin, AudioPluginFormatType::clap), AudioPluginDatabase::FileState::needsScan);
}

TEST_F (AudioPluginDatabaseTests, PruneForgetsMissingFiles)
{
    const auto other = directory.getChildFile ("Other.clap");
    other.replaceWithText ("binary");

    ASSERT_TRUE (database.storeScanResult (plugin, AudioPluginFormatType::clap, { makeDescription (plugin) }).wasOk());
    ASSERT_TRUE (database.storeScanResult (other, AudioPluginFormatType::clap, { makeDescription (other) }).wasOk());

    other.deleteFile();

    EXPECT_EQ (database.pruneMissingFiles(), 1);
    ASSERT_EQ (database.getPlugins().size(), 1u);
    EXPECT_EQ (database.getPlugins().front().fileOrBundlePath, plugin.getFullPathName());
}

TEST_F (AudioPluginDatabaseTests, RefusesDatabasesFromNewerVersions)
{
    database.close();

    {
        SqliteDatabase raw;
        ASSERT_TRUE (raw.open (databaseFile()).wasOk());
        ASSERT_TRUE (raw.executeQuery ("UPDATE meta SET value = '999' WHERE key = 'schema_version'").wasOk());
    }

    EXPECT_TRUE (database.open (databaseFile()).failed());
}

TEST_F (AudioPluginDatabaseTests, SandboxPreferencesFallBackToDefaultAndSurviveRescans)
{
    const auto description = makeDescription (plugin);
    auto other = description;
    other.identifier = "org.yup.other";

    EXPECT_FALSE (database.getDefaultRunSandboxed());
    EXPECT_FALSE (database.shouldRunSandboxed (description));

    ASSERT_TRUE (database.setRunSandboxed (description, true).wasOk());
    EXPECT_TRUE (database.shouldRunSandboxed (description));
    EXPECT_FALSE (database.shouldRunSandboxed (other));

    ASSERT_TRUE (database.setDefaultRunSandboxed (true).wasOk());
    EXPECT_TRUE (database.shouldRunSandboxed (other));

    ASSERT_TRUE (database.setRunSandboxed (other, false).wasOk());
    EXPECT_FALSE (database.shouldRunSandboxed (other));

    ASSERT_TRUE (database.setRunSandboxed (other, std::nullopt).wasOk());
    EXPECT_TRUE (database.shouldRunSandboxed (other));

    ASSERT_TRUE (database.invalidate (plugin).wasOk());
    database.close();
    ASSERT_TRUE (database.open (databaseFile()).wasOk());

    EXPECT_TRUE (database.getDefaultRunSandboxed());
    EXPECT_EQ (database.getRunSandboxed (description), std::optional<bool> (true));
    EXPECT_EQ (database.getRunSandboxed (other), std::nullopt);
}
