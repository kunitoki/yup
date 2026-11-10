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

struct ScanOutcome
{
    std::vector<AudioPluginDescription> descriptions;
    std::optional<AudioPluginDatabase::FailureReason> failure;
    String message;

    static ScanOutcome fail (AudioPluginDatabase::FailureReason reason, const String& message)
    {
        ScanOutcome outcome;
        outcome.failure = reason;
        outcome.message = message;
        return outcome;
    }
};

//==============================================================================

class ScannerProcess final : private ChildProcessCoordinator
{
public:
    explicit ScannerProcess (const AudioPluginScanner::OutOfProcessOptions& processOptions)
        : options (processOptions)
    {
    }

    ~ScannerProcess() override
    {
        terminateWorkerProcess();
    }

    /** Returns std::nullopt when the worker cannot be launched. */
    std::optional<ScanOutcome> scan (const File& file, AudioPluginFormatType formatType)
    {
        using FailureReason = AudioPluginDatabase::FailureReason;

        if (! isRunning || connectionLost || ! isWorkerProcessRunning())
        {
            if (! options.executable.existsAsFile())
                return std::nullopt;

            connectionLost = false;
            isRunning = launchWorkerProcess (options.executable, options.commandLineUniqueID, 0, 0);

            if (! isRunning)
                return std::nullopt;
        }

        {
            const ScopedLock sl (lock);
            reply.reset();
        }

        replyReceived.reset();

        if (! sendMessageToWorker (AudioPluginScanProtocol::writeRequest ({ formatType, file.getFullPathName() })))
            return stop (FailureReason::crash, "Lost the connection to the scanner worker");

        const auto deadline = Time::getMillisecondCounterHiRes() + options.timeoutMs;

        while (! replyReceived.wait (50))
        {
            if (! isWorkerProcessRunning())
                return stop (FailureReason::crash, "The plugin crashed the scanner worker");

            if (Time::getMillisecondCounterHiRes() >= deadline)
                return stop (FailureReason::timeout, "Timed out opening the plugin after " + String (options.timeoutMs) + " ms");
        }

        std::optional<MemoryBlock> received;

        {
            const ScopedLock sl (lock);
            received = std::move (reply);
        }

        if (! received.has_value())
            return stop (FailureReason::crash, "The plugin crashed the scanner worker");

        auto decoded = AudioPluginScanProtocol::readReply (*received);
        if (! decoded.has_value())
            return stop (FailureReason::error, "Malformed reply from the scanner worker");

        if (! decoded->ok)
            return ScanOutcome::fail (FailureReason::error, decoded->errorMessage);

        ScanOutcome outcome;
        outcome.descriptions = std::move (decoded->descriptions);
        return outcome;
    }

private:
    ScanOutcome stop (AudioPluginDatabase::FailureReason reason, const String& message)
    {
        terminateWorkerProcess();
        isRunning = false;
        return ScanOutcome::fail (reason, message);
    }

    void handleMessageFromWorker (const MemoryBlock& message) override
    {
        {
            const ScopedLock sl (lock);
            reply = message;
        }

        replyReceived.signal();
    }

    void handleConnectionLost() override
    {
        connectionLost = true;
        replyReceived.signal();
    }

    const AudioPluginScanner::OutOfProcessOptions options;
    bool isRunning = false;
    std::atomic<bool> connectionLost { false };
    CriticalSection lock;
    std::optional<MemoryBlock> reply;
    WaitableEvent replyReceived;
};

//==============================================================================

/** Hands out one worker process per scanning thread, killed when the pool is destroyed. */
class ScannerProcessPool
{
public:
    explicit ScannerProcessPool (const AudioPluginScanner::OutOfProcessOptions& poolOptions)
        : options (poolOptions)
    {
    }

    std::optional<ScanOutcome> scan (const File& file, AudioPluginFormatType formatType)
    {
        std::unique_ptr<ScannerProcess> process;

        {
            const ScopedLock sl (lock);

            if (! idle.empty())
            {
                process = std::move (idle.back());
                idle.pop_back();
            }
        }

        if (process == nullptr)
            process = std::make_unique<ScannerProcess> (options);

        auto outcome = process->scan (file, formatType);

        const ScopedLock sl (lock);
        idle.push_back (std::move (process));
        return outcome;
    }

private:
    const AudioPluginScanner::OutOfProcessOptions options;
    CriticalSection lock;
    std::vector<std::unique_ptr<ScannerProcess>> idle;
};

//==============================================================================

struct FileScanTask
{
    File file;
    AudioPluginFormat* format = nullptr;
    AudioPluginDatabase* database = nullptr;
    ScannerProcessPool* processes = nullptr;
    std::vector<AudioPluginDescription> discovered;
    String failedPath;
};

ScanOutcome scanInProcess (AudioPluginFormat& format, const File& file)
{
    auto result = format.scanFile (file);
    if (result.failed())
        return ScanOutcome::fail (AudioPluginDatabase::FailureReason::error, result.getErrorMessage());

    ScanOutcome outcome;
    outcome.descriptions = std::move (result).getValue();
    return outcome;
}

void runFileScanTask (FileScanTask& task)
{
    jassert (task.format != nullptr);

    const auto formatType = task.format->getFormatType();

    if (task.database != nullptr)
        task.database->beginScan (task.file, formatType);

    std::optional<ScanOutcome> outcome;

    if (task.processes != nullptr)
        outcome = task.processes->scan (task.file, formatType);

    if (! outcome.has_value())
        outcome = scanInProcess (*task.format, task.file);

    if (! outcome->failure.has_value())
    {
        task.discovered = std::move (outcome->descriptions);

        if (task.database != nullptr)
            task.database->storeScanResult (task.file, formatType, task.discovered);

        return;
    }

    if (task.database != nullptr)
        task.database->storeScanFailure (task.file, formatType, *outcome->failure, outcome->message);

    task.failedPath = task.file.getFullPathName();
}

} // namespace

AudioPluginScanner::AudioPluginScanner()
    : backgroundScanPool (ThreadPool::Options {}
                              .withThreadName ("YUP Plugin Scanner")
                              .withNumberOfThreads (2))
{
}

AudioPluginScanner::~AudioPluginScanner()
{
    cancelPendingScans();
}

void AudioPluginScanner::addFormat (std::unique_ptr<AudioPluginFormat> format)
{
    jassert (format != nullptr);

    const auto type = format->getFormatType();

    for (auto& existing : formats)
    {
        if (existing->getFormatType() == type)
        {
            existing = std::move (format);
            return;
        }
    }

    formats.push_back (std::move (format));
}

int AudioPluginScanner::getNumFormats() const noexcept
{
    return static_cast<int> (formats.size());
}

AudioPluginFormat* AudioPluginScanner::getFormat (int index) const noexcept
{
    if (index < 0 || index >= static_cast<int> (formats.size()))
        return nullptr;

    return formats[static_cast<std::size_t> (index)].get();
}

AudioPluginFormat* AudioPluginScanner::getFormatForType (AudioPluginFormatType type) const noexcept
{
    for (const auto& f : formats)
    {
        if (f->getFormatType() == type)
            return f.get();
    }

    return nullptr;
}

void AudioPluginScanner::setDatabase (std::shared_ptr<AudioPluginDatabase> newDatabase)
{
    database = std::move (newDatabase);
}

std::shared_ptr<AudioPluginDatabase> AudioPluginScanner::getDatabase() const noexcept
{
    return database;
}

void AudioPluginScanner::setOutOfProcessScanning (std::optional<OutOfProcessOptions> options)
{
    outOfProcessOptions = std::move (options);
}

std::optional<AudioPluginScanner::OutOfProcessOptions> AudioPluginScanner::getOutOfProcessScanning() const
{
    return outOfProcessOptions;
}

AudioPluginScanner::ScanResult AudioPluginScanner::scan (const FileSearchPath& searchPath)
{
    ScanResult result;
    const auto scanDatabase = database;

    for (const auto& format : formats)
    {
        if (format->getFileExtensions().isEmpty())
        {
            auto scanResult = format->scanFile (File {});
            if (scanResult.wasOk())
            {
                auto descriptions = scanResult.getValue();
                result.discovered.insert (result.discovered.end(),
                                          descriptions.begin(),
                                          descriptions.end());
            }
        }
    }

    std::vector<FileScanTask> tasks;
    const int numPaths = searchPath.getNumPaths();

    for (const auto& format : formats)
    {
        String wildcardPattern;
        for (const auto& ext : format->getFileExtensions())
        {
            if (wildcardPattern.isNotEmpty())
                wildcardPattern += ";";

            wildcardPattern += "*" + ext;
        }

        if (wildcardPattern.isEmpty())
            continue;

        for (int p = 0; p < numPaths; ++p)
        {
            Array<File> files;
            searchPath[p].findChildFiles (files, File::findFilesAndDirectories, true, wildcardPattern, File::FollowSymlinks::noCycles);

            for (const auto& file : files)
            {
                if (scanDatabase != nullptr)
                {
                    switch (scanDatabase->getFileState (file, format->getFormatType()))
                    {
                        case AudioPluginDatabase::FileState::upToDate:
                        {
                            const auto cached = scanDatabase->getPlugins (file, format->getFormatType());
                            result.discovered.insert (result.discovered.end(), cached.begin(), cached.end());
                            continue;
                        }

                        case AudioPluginDatabase::FileState::failed:
                            result.failedPaths.push_back (file.getFullPathName());
                            continue;

                        case AudioPluginDatabase::FileState::blacklisted:
                            result.blacklistedPaths.push_back (file.getFullPathName());
                            continue;

                        case AudioPluginDatabase::FileState::needsScan:
                            break;
                    }
                }

                FileScanTask task;
                task.file = file;
                task.format = format.get();
                task.database = scanDatabase.get();
                tasks.push_back (std::move (task));
            }
        }
    }

    if (scanDatabase != nullptr)
        scanDatabase->pruneMissingFiles();

    if (tasks.empty())
        return result;

    std::unique_ptr<ScannerProcessPool> processes;
    if (outOfProcessOptions.has_value())
    {
        processes = std::make_unique<ScannerProcessPool> (*outOfProcessOptions);

        for (auto& task : tasks)
            task.processes = processes.get();
    }

    // Each out-of-process scanning thread owns a worker process, keep their number reasonable
    const auto maxThreads = processes != nullptr ? jmin (4, SystemStats::getNumCpus()) : SystemStats::getNumCpus();

    if (tasks.size() == 1)
    {
        runFileScanTask (tasks.front());
    }
    else
    {
        WaitableEvent finished;
        std::atomic<int> remainingTasks { static_cast<int> (tasks.size()) };

        ThreadPool pool (ThreadPool::Options {}
                             .withThreadName ("YUP Plugin File Scanner")
                             .withNumberOfThreads (jmax (1, jmin (maxThreads, static_cast<int> (tasks.size())))));

        for (std::size_t i = 0; i < tasks.size(); ++i)
        {
            pool.addJob ([&tasks, &remainingTasks, &finished, i]
            {
                runFileScanTask (tasks[i]);

                if (--remainingTasks == 0)
                    finished.signal();
            });
        }

        finished.wait();
    }

    for (auto& task : tasks)
    {
        result.discovered.insert (result.discovered.end(),
                                  task.discovered.begin(),
                                  task.discovered.end());

        if (task.failedPath.isNotEmpty())
            result.failedPaths.push_back (task.failedPath);
    }

    return result;
}

AudioPluginScanner::ScanResult AudioPluginScanner::scanDefaults()
{
    FileSearchPath combined;

    for (const auto& format : formats)
    {
        const auto paths = format->getDefaultSearchPaths();
        const int n = paths.getNumPaths();

        for (int i = 0; i < n; ++i)
            combined.addIfNotAlreadyThere (paths[i]);
    }

    return scan (combined);
}

void AudioPluginScanner::scanAsync (const FileSearchPath& searchPath, ScanCallback callback)
{
    backgroundScanPool.addJob ([this, searchPath, callback = std::move (callback)]() mutable
    {
        auto result = scan (searchPath);

        if (callback)
            callback (std::move (result));
    });
}

void AudioPluginScanner::scanDefaultsAsync (ScanCallback callback)
{
    backgroundScanPool.addJob ([this, callback = std::move (callback)]() mutable
    {
        auto result = scanDefaults();

        if (callback)
            callback (std::move (result));
    });
}

void AudioPluginScanner::cancelPendingScans()
{
    backgroundScanPool.removeAllJobs (true, -1);
}

} // namespace yup
