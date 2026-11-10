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

namespace yup
{

namespace
{

constexpr int pluginDatabaseSchemaVersion = 1;

enum class PluginFileStatus
{
    ok = 0,
    failed = 1,
    blacklisted = 2
};

struct PluginFileStamp
{
    int64 size = 0;
    int64 modificationTime = 0;
};

void addToPluginFileStamp (PluginFileStamp& stamp, const File& file)
{
    if (! file.isDirectory())
        stamp.size += file.getSize();

    stamp.modificationTime = jmax (stamp.modificationTime, file.getLastModificationTime().toMilliseconds());
}

PluginFileStamp getPluginFileStamp (const File& file)
{
    PluginFileStamp stamp;
    addToPluginFileStamp (stamp, file);

    // Installers replace the binaries inside a bundle without necessarily touching the bundle directory
    const auto contents = file.getChildFile ("Contents");
    if (! file.isDirectory() || ! contents.isDirectory())
        return stamp;

    for (const auto& child : contents.findChildFiles (File::findFilesAndDirectories, false))
    {
        addToPluginFileStamp (stamp, child);

        if (child.isDirectory() && child.getFileName() != "Resources")
            for (const auto& binary : child.findChildFiles (File::findFiles, false))
                addToPluginFileStamp (stamp, binary);
    }

    return stamp;
}

Result runStatement (ResultValue<SqliteDatabase::Statement> statement)
{
    if (statement.failed())
        return Result::fail (statement.getErrorMessage());

    const auto status = statement.getReference().step();
    if (status != SQLITE_DONE && status != SQLITE_ROW)
        return Result::fail ("SQLite error " + String (status));

    return Result::ok();
}

} // namespace

//==============================================================================

AudioPluginDatabase::AudioPluginDatabase() = default;

AudioPluginDatabase::~AudioPluginDatabase()
{
    close();
}

Result AudioPluginDatabase::open (const File& databaseFile)
{
    const ScopedLock sl (lock);

    if (auto result = database.open (databaseFile); result.failed())
        return result;

    return initialise();
}

Result AudioPluginDatabase::openInMemory()
{
    const ScopedLock sl (lock);

    if (auto result = database.openInMemory(); result.failed())
        return result;

    return initialise();
}

void AudioPluginDatabase::close()
{
    const ScopedLock sl (lock);
    database.close();
}

bool AudioPluginDatabase::isOpen() const
{
    const ScopedLock sl (lock);
    return database.isOpen();
}

Result AudioPluginDatabase::initialise()
{
    if (auto result = database.executeQuery ("PRAGMA journal_mode = WAL; PRAGMA synchronous = NORMAL;"); result.failed())
        return result;

    std::optional<int> storedVersion;
    if (auto result = database.executeQuery ("CREATE TABLE IF NOT EXISTS meta (key TEXT PRIMARY KEY, value TEXT NOT NULL)"); result.failed())
        return result;

    if (auto statement = database.prepareStatement ("SELECT value FROM meta WHERE key = 'schema_version'"); statement.step() == SQLITE_ROW)
        storedVersion = statement.columnText (0).getIntValue();

    if (storedVersion.has_value() && *storedVersion > pluginDatabaseSchemaVersion)
        return Result::fail ("Plugin database was written by a newer version (schema " + String (*storedVersion) + ")");

    // The database is a cache: older schemas are dropped and rebuilt by the next scan
    if (storedVersion.has_value() && *storedVersion < pluginDatabaseSchemaVersion)
    {
        if (auto result = database.executeQuery ("DROP TABLE IF EXISTS files; DROP TABLE IF EXISTS plugins; DROP TABLE IF EXISTS scan_state;"); result.failed())
            return result;
    }

    if (auto result = database.executeQuery (
            "CREATE TABLE IF NOT EXISTS files ("
            " path TEXT NOT NULL, format INTEGER NOT NULL, size INTEGER NOT NULL, mtime INTEGER NOT NULL,"
            " scan_version INTEGER NOT NULL, status INTEGER NOT NULL, reason INTEGER NOT NULL DEFAULT 0,"
            " message TEXT NOT NULL DEFAULT '', failure_count INTEGER NOT NULL DEFAULT 0,"
            " last_failure INTEGER NOT NULL DEFAULT 0, PRIMARY KEY (path, format));"
            "CREATE TABLE IF NOT EXISTS plugins ("
            " path TEXT NOT NULL, format INTEGER NOT NULL, identifier TEXT NOT NULL, bundle_path TEXT NOT NULL,"
            " name TEXT NOT NULL, vendor TEXT NOT NULL, version TEXT NOT NULL, category TEXT NOT NULL,"
            " is_v3 INTEGER NOT NULL, is_instrument INTEGER NOT NULL, is_effect INTEGER NOT NULL,"
            " num_inputs INTEGER NOT NULL, num_outputs INTEGER NOT NULL,"
            " num_midi_inputs INTEGER NOT NULL, num_midi_outputs INTEGER NOT NULL);"
            "CREATE INDEX IF NOT EXISTS plugins_by_file ON plugins (path, format);"
            "CREATE TABLE IF NOT EXISTS scan_state ("
            " path TEXT NOT NULL, format INTEGER NOT NULL, started INTEGER NOT NULL, PRIMARY KEY (path, format));"
            "CREATE TABLE IF NOT EXISTS plugin_settings ("
            " format INTEGER NOT NULL, identifier TEXT NOT NULL, sandboxed INTEGER NOT NULL, PRIMARY KEY (format, identifier));");
        result.failed())
    {
        return result;
    }

    if (auto result = runStatement (database.prepareStatement ("INSERT OR REPLACE INTO meta (key, value) VALUES ('schema_version', ?)",
                                                               String (pluginDatabaseSchemaVersion)));
        result.failed())
    {
        return result;
    }

    return recoverInterruptedScans();
}

Result AudioPluginDatabase::recoverInterruptedScans()
{
    std::vector<std::pair<File, AudioPluginFormatType>> interrupted;

    for (auto statement = database.prepareStatement ("SELECT path, format FROM scan_state"); statement.step() == SQLITE_ROW;)
        interrupted.emplace_back (statement.columnFile (0), static_cast<AudioPluginFormatType> (statement.columnInt (1)));

    for (const auto& [file, formatType] : interrupted)
    {
        if (auto result = storeScanFailure (file, formatType, FailureReason::crash, "The scanner was interrupted while scanning this file"); result.failed())
            return result;
    }

    return Result::ok();
}

//==============================================================================

void AudioPluginDatabase::setScanVersion (int newScanVersion)
{
    const ScopedLock sl (lock);
    scanVersion = newScanVersion;
}

int AudioPluginDatabase::getScanVersion() const
{
    const ScopedLock sl (lock);
    return scanVersion;
}

//==============================================================================

AudioPluginDatabase::FileState AudioPluginDatabase::getFileState (const File& file, AudioPluginFormatType formatType) const
{
    const ScopedLock sl (lock);

    auto statement = database.prepareStatement ("SELECT size, mtime, scan_version, status FROM files WHERE path = ? AND format = ?",
                                                file.getFullPathName(),
                                                static_cast<int> (formatType));

    if (statement.failed() || statement.getReference().step() != SQLITE_ROW)
        return FileState::needsScan;

    const auto& row = statement.getReference();
    const auto stamp = getPluginFileStamp (file);

    if (row.columnInt64 (0) != stamp.size || row.columnInt64 (1) != stamp.modificationTime)
        return FileState::needsScan;

    const auto status = static_cast<PluginFileStatus> (row.columnInt (3));
    if (status == PluginFileStatus::blacklisted)
        return FileState::blacklisted;

    if (row.columnInt (2) != scanVersion)
        return FileState::needsScan;

    return status == PluginFileStatus::failed ? FileState::failed : FileState::upToDate;
}

Result AudioPluginDatabase::beginScan (const File& file, AudioPluginFormatType formatType)
{
    const ScopedLock sl (lock);

    return runStatement (database.prepareStatement ("INSERT OR REPLACE INTO scan_state (path, format, started) VALUES (?, ?, ?)",
                                                    file.getFullPathName(),
                                                    static_cast<int> (formatType),
                                                    Time::currentTimeMillis()));
}

Result AudioPluginDatabase::storeScanResult (const File& file, AudioPluginFormatType formatType, const std::vector<AudioPluginDescription>& descriptions)
{
    const ScopedLock sl (lock);

    const auto path = file.getFullPathName();
    const auto format = static_cast<int> (formatType);
    const auto stamp = getPluginFileStamp (file);

    auto transaction = database.beginTransaction();

    auto result = runStatement (database.prepareStatement ("DELETE FROM plugins WHERE path = ? AND format = ?", path, format));

    for (const auto& description : descriptions)
    {
        if (result.failed())
            break;

        result = runStatement (database.prepareStatement ("INSERT INTO plugins VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
                                                          path,
                                                          format,
                                                          description.identifier,
                                                          description.fileOrBundlePath,
                                                          description.name,
                                                          description.vendor,
                                                          description.version,
                                                          description.category,
                                                          description.isV3,
                                                          description.isInstrument,
                                                          description.isEffect,
                                                          description.numInputChannels,
                                                          description.numOutputChannels,
                                                          description.numMidiInputPorts,
                                                          description.numMidiOutputPorts));
    }

    if (result.wasOk())
    {
        result = runStatement (database.prepareStatement ("INSERT OR REPLACE INTO files (path, format, size, mtime, scan_version, status) VALUES (?, ?, ?, ?, ?, ?)",
                                                          path,
                                                          format,
                                                          stamp.size,
                                                          stamp.modificationTime,
                                                          scanVersion,
                                                          static_cast<int> (PluginFileStatus::ok)));
    }

    if (result.wasOk())
        result = runStatement (database.prepareStatement ("DELETE FROM scan_state WHERE path = ? AND format = ?", path, format));

    if (result.failed())
        transaction.rollback();

    return result;
}

Result AudioPluginDatabase::storeScanFailure (const File& file, AudioPluginFormatType formatType, FailureReason reason, const String& message)
{
    const ScopedLock sl (lock);

    const auto path = file.getFullPathName();
    const auto format = static_cast<int> (formatType);
    const auto stamp = getPluginFileStamp (file);
    const auto status = reason == FailureReason::error ? PluginFileStatus::failed : PluginFileStatus::blacklisted;

    auto transaction = database.beginTransaction();

    auto result = runStatement (database.prepareStatement ("DELETE FROM plugins WHERE path = ? AND format = ?", path, format));

    if (result.wasOk())
    {
        result = runStatement (database.prepareStatement (
            "INSERT INTO files (path, format, size, mtime, scan_version, status, reason, message, failure_count, last_failure)"
            " VALUES (?, ?, ?, ?, ?, ?, ?, ?, 1, ?)"
            " ON CONFLICT (path, format) DO UPDATE SET size = excluded.size, mtime = excluded.mtime,"
            " scan_version = excluded.scan_version, status = excluded.status, reason = excluded.reason,"
            " message = excluded.message, failure_count = files.failure_count + 1, last_failure = excluded.last_failure",
            path,
            format,
            stamp.size,
            stamp.modificationTime,
            scanVersion,
            static_cast<int> (status),
            static_cast<int> (reason),
            message,
            Time::currentTimeMillis()));
    }

    if (result.wasOk())
        result = runStatement (database.prepareStatement ("DELETE FROM scan_state WHERE path = ? AND format = ?", path, format));

    if (result.failed())
        transaction.rollback();

    return result;
}

//==============================================================================

std::vector<AudioPluginDescription> AudioPluginDatabase::getPlugins() const
{
    const ScopedLock sl (lock);
    return getPlugins ({}, AudioPluginFormatType::unknown);
}

std::vector<AudioPluginDescription> AudioPluginDatabase::getPlugins (const File& file, AudioPluginFormatType formatType) const
{
    const ScopedLock sl (lock);

    std::vector<AudioPluginDescription> descriptions;

    if (! database.isOpen())
        return descriptions;

    const bool allFiles = file == File();
    auto statement = database.prepareStatement (
        "SELECT format, identifier, bundle_path, name, vendor, version, category, is_v3, is_instrument, is_effect,"
        " num_inputs, num_outputs, num_midi_inputs, num_midi_outputs FROM plugins"
        " WHERE ? OR (path = ? AND format = ?) ORDER BY rowid",
        allFiles,
        file.getFullPathName(),
        static_cast<int> (formatType));

    if (statement.failed())
        return descriptions;

    for (auto& row = statement.getReference(); row.step() == SQLITE_ROW;)
    {
        AudioPluginDescription description;
        description.formatType = static_cast<AudioPluginFormatType> (row.columnInt (0));
        description.identifier = row.columnText (1);
        description.fileOrBundlePath = row.columnText (2);
        description.name = row.columnText (3);
        description.vendor = row.columnText (4);
        description.version = row.columnText (5);
        description.category = row.columnText (6);
        description.isV3 = row.columnBool (7);
        description.isInstrument = row.columnBool (8);
        description.isEffect = row.columnBool (9);
        description.numInputChannels = row.columnInt (10);
        description.numOutputChannels = row.columnInt (11);
        description.numMidiInputPorts = row.columnInt (12);
        description.numMidiOutputPorts = row.columnInt (13);
        descriptions.push_back (std::move (description));
    }

    return descriptions;
}

std::vector<AudioPluginDatabase::Failure> AudioPluginDatabase::getFailures() const
{
    const ScopedLock sl (lock);

    std::vector<Failure> failures;

    if (! database.isOpen())
        return failures;

    auto statement = database.prepareStatement ("SELECT path, format, reason, message, failure_count, last_failure, status FROM files WHERE status != ? ORDER BY path",
                                                static_cast<int> (PluginFileStatus::ok));

    if (statement.failed())
        return failures;

    for (auto& row = statement.getReference(); row.step() == SQLITE_ROW;)
    {
        Failure failure;
        failure.file = row.columnFile (0);
        failure.formatType = static_cast<AudioPluginFormatType> (row.columnInt (1));
        failure.reason = static_cast<FailureReason> (row.columnInt (2));
        failure.message = row.columnText (3);
        failure.count = row.columnInt (4);
        failure.lastFailureTime = Time (row.columnInt64 (5));
        failure.blacklisted = static_cast<PluginFileStatus> (row.columnInt (6)) == PluginFileStatus::blacklisted;
        failures.push_back (std::move (failure));
    }

    return failures;
}

//==============================================================================

Result AudioPluginDatabase::invalidate (const File& file)
{
    const ScopedLock sl (lock);

    const auto path = file.getFullPathName();
    auto transaction = database.beginTransaction();

    for (const auto* sql : { "DELETE FROM plugins WHERE path = ?", "DELETE FROM files WHERE path = ?", "DELETE FROM scan_state WHERE path = ?" })
    {
        if (auto result = runStatement (database.prepareStatement (sql, path)); result.failed())
        {
            transaction.rollback();
            return result;
        }
    }

    return Result::ok();
}

Result AudioPluginDatabase::clearBlacklist (const File& file)
{
    const ScopedLock sl (lock);

    return runStatement (database.prepareStatement ("DELETE FROM files WHERE path = ? AND status = ?",
                                                    file.getFullPathName(),
                                                    static_cast<int> (PluginFileStatus::blacklisted)));
}

Result AudioPluginDatabase::clearBlacklist()
{
    const ScopedLock sl (lock);

    return runStatement (database.prepareStatement ("DELETE FROM files WHERE status = ?",
                                                    static_cast<int> (PluginFileStatus::blacklisted)));
}

int AudioPluginDatabase::pruneMissingFiles()
{
    const ScopedLock sl (lock);

    if (! database.isOpen())
        return 0;

    std::vector<File> missing;

    for (auto statement = database.prepareStatement ("SELECT DISTINCT path FROM files UNION SELECT DISTINCT path FROM plugins"); statement.step() == SQLITE_ROW;)
    {
        if (const auto file = statement.columnFile (0); ! file.exists())
            missing.push_back (file);
    }

    int numRemoved = 0;

    for (const auto& file : missing)
        numRemoved += invalidate (file).wasOk() ? 1 : 0;

    return numRemoved;
}

//==============================================================================

Result AudioPluginDatabase::setRunSandboxed (const AudioPluginDescription& description, std::optional<bool> shouldRunSandboxed)
{
    const ScopedLock sl (lock);

    const auto format = static_cast<int> (description.formatType);

    if (! shouldRunSandboxed.has_value())
        return runStatement (database.prepareStatement ("DELETE FROM plugin_settings WHERE format = ? AND identifier = ?", format, description.identifier));

    return runStatement (database.prepareStatement ("INSERT OR REPLACE INTO plugin_settings (format, identifier, sandboxed) VALUES (?, ?, ?)",
                                                    format,
                                                    description.identifier,
                                                    *shouldRunSandboxed));
}

std::optional<bool> AudioPluginDatabase::getRunSandboxed (const AudioPluginDescription& description) const
{
    const ScopedLock sl (lock);

    auto statement = database.prepareStatement ("SELECT sandboxed FROM plugin_settings WHERE format = ? AND identifier = ?",
                                                static_cast<int> (description.formatType),
                                                description.identifier);

    if (statement.failed() || statement.getReference().step() != SQLITE_ROW)
        return std::nullopt;

    return statement.getReference().columnBool (0);
}

Result AudioPluginDatabase::setDefaultRunSandboxed (bool shouldRunSandboxed)
{
    const ScopedLock sl (lock);

    return runStatement (database.prepareStatement ("INSERT OR REPLACE INTO meta (key, value) VALUES ('default_sandboxed', ?)",
                                                    String (shouldRunSandboxed ? 1 : 0)));
}

bool AudioPluginDatabase::getDefaultRunSandboxed() const
{
    const ScopedLock sl (lock);

    auto statement = database.prepareStatement ("SELECT value FROM meta WHERE key = 'default_sandboxed'");

    return statement.step() == SQLITE_ROW && statement.columnText (0).getIntValue() != 0;
}

bool AudioPluginDatabase::shouldRunSandboxed (const AudioPluginDescription& description) const
{
    const ScopedLock sl (lock);
    return getRunSandboxed (description).value_or (getDefaultRunSandboxed());
}

} // namespace yup
