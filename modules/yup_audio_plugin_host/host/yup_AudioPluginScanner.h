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

/**
    Scans directories for plugins across all registered formats.

    Construct one AudioPluginScanner, register format backends via addFormat(),
    then call scan(), scanDefaults(), scanAsync(), or scanDefaultsAsync().
    Results are cached only when a database is set with setDatabase().

    Thread safety: formats may be scanned concurrently by the scanner. Do not
    modify the registered formats while a scan is running.
*/
class AudioPluginScanner
{
public:
    //==============================================================================

    /** Aggregated output of a scan operation. */
    struct ScanResult
    {
        /** All successfully parsed plugin descriptions, in discovery order. */
        std::vector<AudioPluginDescription> discovered;

        /** Absolute paths of files that could not be parsed. */
        std::vector<String> failedPaths;

        /** Absolute paths of files skipped because the database blacklisted them. */
        std::vector<String> blacklistedPaths;
    };

    //==============================================================================

    /** Configures scanning plugins in separate worker processes. */
    struct OutOfProcessOptions
    {
        /** Executable creating an AudioPluginScannerWorker at startup, usually the host itself. */
        File executable;

        /** Must match the identifier passed to AudioPluginScannerWorker::initialiseFromCommandLine(). */
        String commandLineUniqueID = AudioPluginScannerWorker::defaultCommandLineUniqueID;

        /** Maximum time allowed to open the plugins of one file, after which the worker is killed. */
        int timeoutMs = 60000;
    };

    //==============================================================================

    /** Callback invoked when an asynchronous scan finishes. */
    using ScanCallback = std::function<void (ScanResult result)>;

    //==============================================================================

    AudioPluginScanner();
    ~AudioPluginScanner();

    //==============================================================================

    /**
        Registers a format backend.

        If a format with the same AudioPluginFormatType is already registered it
        is replaced. Takes ownership.
    */
    void addFormat (std::unique_ptr<AudioPluginFormat> format);

    /** Returns the number of registered formats. */
    int getNumFormats() const noexcept;

    /** Returns the format at @p index, or nullptr if out of range. */
    AudioPluginFormat* getFormat (int index) const noexcept;

    /** Returns the format for the given type, or nullptr if not registered. */
    AudioPluginFormat* getFormatForType (AudioPluginFormatType type) const noexcept;

    //==============================================================================

    /**
        Sets the database caching scan results, or nullptr to scan everything every time.

        With a database, scans only open files that are new, changed on disk or scanned
        with an older scan version; unchanged files return their cached descriptions or
        failures, blacklisted files are skipped and files no longer on disk are pruned.
        Each scan is recorded with AudioPluginDatabase::beginScan(), so a plugin crashing
        the process is blacklisted when the database is opened again.

        Registry based formats (e.g. AUv2) are always scanned and never cached.
    */
    void setDatabase (std::shared_ptr<AudioPluginDatabase> newDatabase);

    /** Returns the database set with setDatabase(), if any. */
    std::shared_ptr<AudioPluginDatabase> getDatabase() const noexcept;

    //==============================================================================

    /**
        Opens plugins in worker processes instead of reading their metadata in-process.

        Each file is fully opened with AudioPluginFormat::scanFileFully() by a worker
        process launched from @p options, so plugins crashing or hanging while being
        opened only take down the worker. Crashed files and files exceeding the timeout
        are reported in ScanResult::failedPaths and blacklisted in the database, if any.
        When the worker cannot be launched, files are scanned in-process with
        AudioPluginFormat::scanFile().

        Pass std::nullopt to scan in-process again (the default).

        @see AudioPluginScannerWorker
    */
    void setOutOfProcessScanning (std::optional<OutOfProcessOptions> options);

    /** Returns the options set with setOutOfProcessScanning(), if any. */
    std::optional<OutOfProcessOptions> getOutOfProcessScanning() const;

    //==============================================================================

    /**
        Scans all files found under @p searchPath using all registered formats.

        Files and bundles are offered to registered formats whose extension
        matches the format. If a format returns a success, its results are
        appended; if it fails, the path is added to ScanResult::failedPaths.

        Formats that return an empty array from getFileExtensions() (e.g. AUv2) use
        registry-based discovery and are scanned independently of the search path.
    */
    ScanResult scan (const FileSearchPath& searchPath);

    /**
        Scans each registered format's default search paths.
    */
    ScanResult scanDefaults();

    /**
        Starts scanning @p searchPath on a background thread.

        The callback is invoked from a scanner worker thread. UI clients should
        dispatch back to the message thread before touching components.
    */
    void scanAsync (const FileSearchPath& searchPath, ScanCallback callback);

    /**
        Starts scanning each registered format's default search paths on a
        background thread.
    */
    void scanDefaultsAsync (ScanCallback callback);

    /**
        Attempts to stop queued or running asynchronous scans.
    */
    void cancelPendingScans();

private:
    std::vector<std::unique_ptr<AudioPluginFormat>> formats;
    std::shared_ptr<AudioPluginDatabase> database;
    std::optional<OutOfProcessOptions> outOfProcessOptions;
    CriticalSection inProcessScanLock;
    ThreadPool backgroundScanPool;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginScanner)
};

} // namespace yup
