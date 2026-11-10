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

    /** Closes the database and runs raw SQL on its file, the caller reopens it. */
    void modifyDatabase (const String& sql)
    {
        database.close();

        SqliteDatabase raw;
        ASSERT_TRUE (raw.open (databaseFile()).wasOk());
        ASSERT_TRUE (raw.executeQuery (sql).wasOk());
    }

    void insertInterruptedScan (int processId)
    {
        modifyDatabase ("INSERT INTO scan_state (path, format, started, pid) VALUES ('"
                        + plugin.getFullPathName().replace ("'", "''") + "', "
                        + String (static_cast<int> (AudioPluginFormatType::clap)) + ", 0, "
                        + String (processId) + ")");
    }

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
    EXPECT_EQ (database.getScanVersion(), 2);
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

TEST_F (AudioPluginDatabaseTests, ClosedDatabaseReturnsNothingAndRefusesWrites)
{
    EXPECT_TRUE (database.isOpen());

    database.close();

    EXPECT_FALSE (database.isOpen());
    EXPECT_TRUE (database.getPlugins().empty());
    EXPECT_TRUE (database.getFailures().empty());
    EXPECT_EQ (database.pruneMissingFiles(), 0);
    EXPECT_TRUE (database.beginScan (plugin, AudioPluginFormatType::clap).failed());
}

TEST_F (AudioPluginDatabaseTests, OlderSchemasAreDroppedOnOpen)
{
    ASSERT_TRUE (database.storeScanResult (plugin, AudioPluginFormatType::clap, { makeDescription (plugin) }).wasOk());

    modifyDatabase ("UPDATE meta SET value = '1' WHERE key = 'schema_version'");
    ASSERT_TRUE (database.open (databaseFile()).wasOk());

    EXPECT_TRUE (database.getPlugins().empty());
    EXPECT_EQ (database.getFileState (plugin, AudioPluginFormatType::clap), AudioPluginDatabase::FileState::needsScan);
}

TEST_F (AudioPluginDatabaseTests, ClearBlacklistLiftsEveryBlacklistedFileOnly)
{
    const auto hanging = directory.getChildFile ("Hanging.clap");
    const auto broken = directory.getChildFile ("Broken.clap");
    hanging.replaceWithText ("binary");
    broken.replaceWithText ("binary");

    ASSERT_TRUE (database.storeScanFailure (plugin, AudioPluginFormatType::clap, AudioPluginDatabase::FailureReason::crash, "crashed").wasOk());
    ASSERT_TRUE (database.storeScanFailure (hanging, AudioPluginFormatType::clap, AudioPluginDatabase::FailureReason::timeout, "hung").wasOk());
    ASSERT_TRUE (database.storeScanFailure (broken, AudioPluginFormatType::clap, AudioPluginDatabase::FailureReason::error, "broken").wasOk());

    ASSERT_TRUE (database.clearBlacklist().wasOk());

    EXPECT_EQ (database.getFileState (plugin, AudioPluginFormatType::clap), AudioPluginDatabase::FileState::needsScan);
    EXPECT_EQ (database.getFileState (hanging, AudioPluginFormatType::clap), AudioPluginDatabase::FileState::needsScan);
    EXPECT_EQ (database.getFileState (broken, AudioPluginFormatType::clap), AudioPluginDatabase::FileState::failed);
    EXPECT_EQ (database.getFailures().size(), 1u);
}

TEST_F (AudioPluginDatabaseTests, BundlesNeedScanWhenTheirBinariesChange)
{
    const auto bundle = directory.getChildFile ("Synth.vst3");
    const auto contents = bundle.getChildFile ("Contents");
    const auto binary = contents.getChildFile ("x86_64-linux").getChildFile ("Synth.so");
    const auto resource = contents.getChildFile ("Resources").getChildFile ("info.txt");

    ASSERT_TRUE (binary.getParentDirectory().createDirectory().wasOk());
    ASSERT_TRUE (resource.getParentDirectory().createDirectory().wasOk());
    ASSERT_TRUE (binary.replaceWithText ("binary"));
    ASSERT_TRUE (resource.replaceWithText ("info"));

    ASSERT_TRUE (database.storeScanResult (bundle, AudioPluginFormatType::vst3, { makeDescription (bundle) }).wasOk());
    EXPECT_EQ (database.getFileState (bundle, AudioPluginFormatType::vst3), AudioPluginDatabase::FileState::upToDate);

    ASSERT_TRUE (binary.replaceWithText ("a rebuilt and much larger binary"));
    EXPECT_EQ (database.getFileState (bundle, AudioPluginFormatType::vst3), AudioPluginDatabase::FileState::needsScan);
}

TEST_F (AudioPluginDatabaseTests, InterruptedScanOfADeadProcessIsBlacklistedOnReopen)
{
    insertInterruptedScan (2147483000);
    ASSERT_TRUE (database.open (databaseFile()).wasOk());

    EXPECT_EQ (database.getFileState (plugin, AudioPluginFormatType::clap), AudioPluginDatabase::FileState::blacklisted);

    const auto failures = database.getFailures();
    ASSERT_EQ (failures.size(), 1u);
    EXPECT_EQ (failures.front().reason, AudioPluginDatabase::FailureReason::crash);
}

#if YUP_MAC || YUP_LINUX
TEST_F (AudioPluginDatabaseTests, ScanOfAnotherRunningProcessIsNotInterrupted)
{
    insertInterruptedScan (1);
    ASSERT_TRUE (database.open (databaseFile()).wasOk());

    EXPECT_EQ (database.getFileState (plugin, AudioPluginFormatType::clap), AudioPluginDatabase::FileState::needsScan);
    EXPECT_TRUE (database.getFailures().empty());
}
#endif

TEST_F (AudioPluginDatabaseTests, RejectedStatementsFail)
{
    modifyDatabase ("CREATE TRIGGER reject_scans BEFORE INSERT ON scan_state BEGIN SELECT RAISE (ABORT, 'rejected'); END;");
    ASSERT_TRUE (database.open (databaseFile()).wasOk());

    EXPECT_TRUE (database.beginScan (plugin, AudioPluginFormatType::clap).failed());
}

TEST_F (AudioPluginDatabaseTests, FailedWritesRollBack)
{
    ASSERT_TRUE (database.storeScanResult (plugin, AudioPluginFormatType::clap, { makeDescription (plugin) }).wasOk());

    modifyDatabase ("CREATE TRIGGER reject_plugins BEFORE INSERT ON plugins BEGIN SELECT RAISE (ABORT, 'rejected'); END;"
                    "CREATE TRIGGER reject_file_inserts BEFORE INSERT ON files BEGIN SELECT RAISE (ABORT, 'rejected'); END;"
                    "CREATE TRIGGER reject_file_deletes BEFORE DELETE ON files BEGIN SELECT RAISE (ABORT, 'rejected'); END;");
    ASSERT_TRUE (database.open (databaseFile()).wasOk());

    auto other = makeDescription (plugin);
    other.identifier = "org.yup.other";

    EXPECT_TRUE (database.storeScanResult (plugin, AudioPluginFormatType::clap, { makeDescription (plugin), other }).failed());
    EXPECT_TRUE (database.storeScanFailure (plugin, AudioPluginFormatType::clap, AudioPluginDatabase::FailureReason::error, "broken").failed());
    EXPECT_TRUE (database.invalidate (plugin).failed());

    EXPECT_EQ (database.getFileState (plugin, AudioPluginFormatType::clap), AudioPluginDatabase::FileState::upToDate);
    ASSERT_EQ (database.getPlugins (plugin, AudioPluginFormatType::clap).size(), 1u);
}

TEST_F (AudioPluginDatabaseTests, OpenFailsWhenTheSchemaCannotBeWritten)
{
    modifyDatabase ("CREATE TRIGGER reject_meta BEFORE INSERT ON meta BEGIN SELECT RAISE (ABORT, 'rejected'); END;");

    EXPECT_TRUE (database.open (databaseFile()).failed());
}

TEST_F (AudioPluginDatabaseTests, CorruptPluginsTableFailsToOpenAndReturnsNoPlugins)
{
    modifyDatabase ("DROP TABLE plugins; CREATE VIEW plugins AS SELECT 1 AS path;");

    EXPECT_TRUE (database.open (databaseFile()).failed());
    EXPECT_TRUE (database.isOpen());
    EXPECT_TRUE (database.getPlugins().empty());
}

TEST_F (AudioPluginDatabaseTests, CorruptFilesTableReturnsNoFailures)
{
    modifyDatabase ("DROP TABLE files; CREATE VIEW files AS SELECT 1 AS path;");
    ASSERT_TRUE (database.open (databaseFile()).wasOk());

    EXPECT_TRUE (database.getFailures().empty());
    EXPECT_TRUE (database.storeScanFailure (plugin, AudioPluginFormatType::clap, AudioPluginDatabase::FailureReason::error, "broken").failed());
}
