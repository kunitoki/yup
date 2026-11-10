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

//==============================================================================
/**
    A persistent SQLite cache of plugin scan results.

    The database remembers, for every scanned plugin file and format, the file
    size and modification time seen at scan time, the descriptions found and any
    failure. AudioPluginScanner consults it to scan only new or changed files.

    Crash protection works like a dead-man's pedal: beginScan() records the file
    being scanned and storeScanResult() / storeScanFailure() clear it. If the
    process dies in between, the next open() finds the leftover record and
    blacklists the file, so a plugin crashing the scanner is not loaded again
    until it changes on disk or clearBlacklist() is called.

    All methods are thread-safe.

    @see AudioPluginScanner::setDatabase
*/
class AudioPluginDatabase
{
public:
    //==============================================================================
    /** Why scanning a file failed. */
    enum class FailureReason
    {
        /** The format reported an error, e.g. the file is not a valid plugin. */
        error,

        /** The plugin crashed the scanning process. */
        crash,

        /** The plugin did not finish scanning in time. */
        timeout
    };

    /** The cached state of a plugin file. */
    enum class FileState
    {
        /** Never scanned, changed on disk, or scanned by an older scan version. */
        needsScan,

        /** Scanned successfully, the cached descriptions are valid. */
        upToDate,

        /** The last scan failed with an error, the file is unchanged since. */
        failed,

        /** The file crashed or hung the scanner and is skipped until it changes or is cleared. */
        blacklisted
    };

    /** A failed file, as returned by getFailures(). */
    struct Failure
    {
        File file;
        AudioPluginFormatType formatType = AudioPluginFormatType::unknown;
        FailureReason reason = FailureReason::error;
        String message;
        int count = 0;
        Time lastFailureTime;
        bool blacklisted = false;
    };

    //==============================================================================
    AudioPluginDatabase();
    ~AudioPluginDatabase();

    //==============================================================================
    /** Opens or creates the database file, then blacklists files left over by an interrupted scan. */
    Result open (const File& databaseFile);

    /** Opens a temporary database that lives in memory only. */
    Result openInMemory();

    /** Closes the database. */
    void close();

    /** Returns true when the database is open. */
    bool isOpen() const;

    //==============================================================================
    /** Sets the scan version, bump it to rescan every non-blacklisted file (default 1). */
    void setScanVersion (int newScanVersion);

    /** Returns the scan version written with new results. */
    int getScanVersion() const;

    //==============================================================================
    /** Returns the cached state of @p file for @p formatType. */
    FileState getFileState (const File& file, AudioPluginFormatType formatType) const;

    /** Marks @p file as being scanned, call before scanning it. */
    Result beginScan (const File& file, AudioPluginFormatType formatType);

    /** Stores the descriptions found in @p file, replacing previous ones, and ends its scan. */
    Result storeScanResult (const File& file, AudioPluginFormatType formatType, const std::vector<AudioPluginDescription>& descriptions);

    /** Records a failed scan of @p file and ends it. Crashes and timeouts blacklist the file. */
    Result storeScanFailure (const File& file, AudioPluginFormatType formatType, FailureReason reason, const String& message);

    //==============================================================================
    /** Returns every cached plugin description. */
    std::vector<AudioPluginDescription> getPlugins() const;

    /** Returns the cached plugin descriptions found in @p file. */
    std::vector<AudioPluginDescription> getPlugins (const File& file, AudioPluginFormatType formatType) const;

    /** Returns every file whose last scan failed, blacklisted or not. */
    std::vector<Failure> getFailures() const;

    //==============================================================================
    /** Forgets @p file for every format, so it is scanned again. */
    Result invalidate (const File& file);

    /** Removes the blacklist mark of @p file so it is scanned again. */
    Result clearBlacklist (const File& file);

    /** Removes every blacklist mark. */
    Result clearBlacklist();

    /** Forgets every file that no longer exists, returns how many were removed. */
    int pruneMissingFiles();

    //==============================================================================
    /**
        Sets whether the plugin described by @p description should run sandboxed, in a
        separate process through RemoteAudioPluginInstance.

        Preferences are keyed by format and identifier and are kept when files are rescanned,
        invalidated or pruned. Pass std::nullopt to fall back to getDefaultRunSandboxed().
    */
    Result setRunSandboxed (const AudioPluginDescription& description, std::optional<bool> shouldRunSandboxed);

    /** Returns the sandboxing preference of a plugin, or std::nullopt when it uses the default. */
    std::optional<bool> getRunSandboxed (const AudioPluginDescription& description) const;

    /** Sets whether plugins without their own preference run sandboxed (default false). */
    Result setDefaultRunSandboxed (bool shouldRunSandboxed);

    /** Returns whether plugins without their own preference run sandboxed. */
    bool getDefaultRunSandboxed() const;

    /** Returns the preference of @p description when set, the default otherwise. */
    bool shouldRunSandboxed (const AudioPluginDescription& description) const;

private:
    Result initialise();
    Result recoverInterruptedScans();

    mutable CriticalSection lock;
    mutable SqliteDatabase database;
    int scanVersion = 1;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginDatabase)
};

} // namespace yup
