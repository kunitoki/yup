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

bool canScanFileWithFormat (AudioPluginFormatType type, const File& file)
{
    switch (type)
    {
        case AudioPluginFormatType::vst3:
            return file.hasFileExtension (".vst3");

        case AudioPluginFormatType::clap:
            return file.hasFileExtension (".clap");

        case AudioPluginFormatType::audioUnit:
            return false;

        default:
            break;
    }

    return false;
}

struct FileScanTask
{
    File file;
    AudioPluginFormat* format = nullptr;
    std::vector<AudioPluginDescription> discovered;
    String failedPath;
};

void runFileScanTask (FileScanTask& task)
{
    jassert (task.format != nullptr);

    auto scanResult = task.format->scanFile (task.file);

    if (scanResult.wasOk())
    {
        task.discovered = std::move (scanResult).getValue();
        return;
    }

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

AudioPluginScanner::ScanResult AudioPluginScanner::scan (const FileSearchPath& searchPath)
{
    ScanResult result;

    // AUv2 does not use file system scanning; delegate directly if registered.
    if (auto* auFormat = getFormatForType (AudioPluginFormatType::audioUnit))
    {
        // AUv2 scanFile() with an invalid File triggers registry enumeration
        auto auResult = auFormat->scanFile (File {});
        if (auResult.wasOk())
        {
            auto descriptions = auResult.getValue();
            result.discovered.insert (result.discovered.end(),
                                      descriptions.begin(),
                                      descriptions.end());
        }
    }

    std::vector<FileScanTask> tasks;
    const int numPaths = searchPath.getNumPaths();

    for (int p = 0; p < numPaths; ++p)
    {
        Array<File> files;
        searchPath[p].findChildFiles (files, File::findFilesAndDirectories, true, "*", File::FollowSymlinks::no);

        for (const auto& file : files)
        {
            for (const auto& format : formats)
            {
                if (! canScanFileWithFormat (format->getFormatType(), file))
                    continue;

                FileScanTask task;
                task.file = file;
                task.format = format.get();
                tasks.push_back (std::move (task));
            }
        }
    }

    if (tasks.empty())
        return result;

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
                             .withNumberOfThreads (jmax (1, jmin (SystemStats::getNumCpus(), static_cast<int> (tasks.size())))));

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
