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
AudioPeakProfileCache::PendingJob::PendingJob (AudioPeakProfileCache& owner,
                                               const String& key,
                                               std::function<std::shared_ptr<AudioPeakProfile>()> buildFn,
                                               int id)
    : ThreadPoolJob ("PeakProfileGeneration")
    , cache (owner)
    , cacheKey (key)
    , buildFunction (std::move (buildFn))
    , jobId (id)
{
}

ThreadPoolJob::JobStatus AudioPeakProfileCache::PendingJob::runJob()
{
    // Check if this job has been cancelled
    {
        const ScopedLock lock (cache.jobsLock);
        auto it = cache.pendingJobs.find (cacheKey);
        if (it == cache.pendingJobs.end() || it->second != jobId)
            return jobHasFinished; // Job was cancelled or superseded
    }

    // Build the profile
    auto profile = buildFunction();

    if (shouldExit())
        return jobHasFinished;

    if (profile && profile->isValid())
    {
        // Save to disk cache if enabled
        if (cache.diskCacheEnabled)
            cache.saveToDisk (cacheKey, *profile);

        // Add to memory cache
        {
            const ScopedLock lock (cache.cacheLock);
            cache.memoryCache[cacheKey] = { profile, cacheKey, Time::currentTimeMillis() };
        }

        // Remove from pending jobs
        {
            const ScopedLock lock (cache.jobsLock);
            cache.pendingJobs.erase (cacheKey);
        }

        // Notify listeners
        cache.notifyProfileReady (cacheKey, profile);
    }
    else
    {
        // Remove from pending jobs
        const ScopedLock lock (cache.jobsLock);
        cache.pendingJobs.erase (cacheKey);
    }

    return jobHasFinished;
}

//==============================================================================
AudioPeakProfileCache::AudioPeakProfileCache()
{
    // Set default cache directory to temp folder
    cacheDirectory = File::getSpecialLocation (File::tempDirectory).getChildFile ("YUP_PeakCache");
}

AudioPeakProfileCache::~AudioPeakProfileCache()
{
    // Cancel all pending jobs
    if (threadPool != nullptr)
    {
        const ScopedLock lock (jobsLock);
        for (auto& [key, jobId] : pendingJobs)
        {
            ignoreUnused (jobId);
            // Jobs will check pendingJobs map and exit gracefully
        }
        pendingJobs.clear();
    }
}

//==============================================================================
void AudioPeakProfileCache::setDiskCacheEnabled (bool enabled) noexcept
{
    diskCacheEnabled = enabled;

    if (enabled && ! cacheDirectory.exists())
        cacheDirectory.createDirectory();
}

void AudioPeakProfileCache::setCacheDirectory (const File& directory)
{
    cacheDirectory = directory;

    if (diskCacheEnabled && ! cacheDirectory.exists())
        cacheDirectory.createDirectory();
}

void AudioPeakProfileCache::setThreadPool (ThreadPool* pool) noexcept
{
    threadPool = pool;
}

//==============================================================================
std::shared_ptr<AudioPeakProfile> AudioPeakProfileCache::getProfile (const String& cacheKey)
{
    const ScopedLock lock (cacheLock);

    auto it = memoryCache.find (cacheKey);
    if (it != memoryCache.end())
    {
        it->second.lastAccessTime = Time::currentTimeMillis();
        return it->second.profile;
    }

    return nullptr;
}

void AudioPeakProfileCache::requestProfile (const String& cacheKey,
                                            std::function<std::shared_ptr<AudioPeakProfile>()> buildFunction,
                                            bool useBackground)
{
    // Check memory cache
    {
        const ScopedLock lock (cacheLock);
        auto it = memoryCache.find (cacheKey);
        if (it != memoryCache.end())
        {
            it->second.lastAccessTime = Time::currentTimeMillis();
            notifyProfileReady (cacheKey, it->second.profile);
            return;
        }
    }

    // Check if already pending
    {
        const ScopedLock lock (jobsLock);
        if (pendingJobs.find (cacheKey) != pendingJobs.end())
            return; // Already being computed
    }

    // Check disk cache
    if (diskCacheEnabled)
    {
        auto profile = loadFromDisk (cacheKey);
        if (profile)
        {
            const ScopedLock lock (cacheLock);
            memoryCache[cacheKey] = { profile, cacheKey, Time::currentTimeMillis() };
            notifyProfileReady (cacheKey, profile);
            return;
        }
    }

    // Build new profile
    const int newJobId = ++jobCounter;

    {
        const ScopedLock lock (jobsLock);
        pendingJobs[cacheKey] = newJobId;
    }

    if (useBackground && threadPool != nullptr)
    {
        // Compute in background
        auto* job = new PendingJob (*this, cacheKey, buildFunction, newJobId);
        threadPool->addJob (job, true);
    }
    else
    {
        // Compute synchronously
        auto profile = buildFunction();

        if (profile && profile->isValid())
        {
            if (diskCacheEnabled)
                saveToDisk (cacheKey, *profile);

            {
                const ScopedLock lock (cacheLock);
                memoryCache[cacheKey] = { profile, cacheKey, Time::currentTimeMillis() };
            }

            notifyProfileReady (cacheKey, profile);
        }

        {
            const ScopedLock lock (jobsLock);
            pendingJobs.erase (cacheKey);
        }
    }
}

void AudioPeakProfileCache::cancelPendingRequests (const String& cacheKey)
{
    const ScopedLock lock (jobsLock);
    pendingJobs.erase (cacheKey);
}

void AudioPeakProfileCache::clearMemoryCache()
{
    const ScopedLock lock (cacheLock);
    memoryCache.clear();
}

//==============================================================================
void AudioPeakProfileCache::addListener (Listener* listener)
{
    listeners.add (listener);
}

void AudioPeakProfileCache::removeListener (Listener* listener)
{
    listeners.remove (listener);
}

//==============================================================================
String AudioPeakProfileCache::generateCacheKey (const File& audioFile)
{
    if (! audioFile.existsAsFile())
        return {};

    const int64 hash = audioFile.hashCode64();
    const int64 modTime = audioFile.getLastModificationTime().toMilliseconds();

    return String::toHexString (hash) + "_" + String::toHexString (modTime);
}

String AudioPeakProfileCache::generateCacheKey (const AudioBuffer<float>& buffer, double sampleRate)
{
    int64 hash = buffer.getNumSamples();
    hash = hash * 31 + buffer.getNumChannels();
    hash = hash * 31 + static_cast<int64> (sampleRate * 1000.0);

    // Sample buffer content at intervals for unique identification
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        const float* data = buffer.getReadPointer (channel);
        const int numSamples = buffer.getNumSamples();

        for (int i = 0; i < jmin (1000, numSamples); i += 100)
        {
            hash = hash * 31 + static_cast<int64> (data[i] * 1000000.0f);
        }
    }

    return String::toHexString (hash);
}

int64 AudioPeakProfileCache::getDiskCacheSize() const
{
    if (! diskCacheEnabled || ! cacheDirectory.exists())
        return 0;

    int64 totalSize = 0;
    for (const auto& entry : RangedDirectoryIterator (cacheDirectory, false, "*.yuppeaks"))
    {
        totalSize += entry.getFile().getSize();
    }

    return totalSize;
}

void AudioPeakProfileCache::clearDiskCache()
{
    if (! cacheDirectory.exists())
        return;

    for (const auto& entry : RangedDirectoryIterator (cacheDirectory, false, "*.yuppeaks"))
    {
        entry.getFile().deleteFile();
    }
}

//==============================================================================
File AudioPeakProfileCache::getCacheFilePath (const String& cacheKey) const
{
    return cacheDirectory.getChildFile (cacheKey + ".yuppeaks");
}

std::shared_ptr<AudioPeakProfile> AudioPeakProfileCache::loadFromDisk (const String& cacheKey)
{
    const File cacheFile = getCacheFilePath (cacheKey);

    if (! cacheFile.existsAsFile())
        return nullptr;

    auto profile = std::make_shared<AudioPeakProfile>();
    const auto result = profile->loadFromFile (cacheFile);

    if (result.wasOk() && profile->isValid())
        return profile;

    return nullptr;
}

void AudioPeakProfileCache::saveToDisk (const String& cacheKey, const AudioPeakProfile& profile)
{
    if (! diskCacheEnabled)
        return;

    if (! cacheDirectory.exists())
        cacheDirectory.createDirectory();

    const File cacheFile = getCacheFilePath (cacheKey);
    profile.saveToFile (cacheFile);
}

void AudioPeakProfileCache::notifyProfileReady (const String& cacheKey, std::shared_ptr<AudioPeakProfile> profile)
{
    auto notify = [this, cacheKey, profile]
    {
        listeners.call ([&] (Listener& l)
        {
            l.profileReady (cacheKey, profile);
        });
    };

#if YUP_MODAL_LOOPS_PERMITTED
    if (auto* mm = MessageManager::getInstanceWithoutCreating())
    {
        if (mm->isThisTheMessageThread())
        {
            notify();
            return;
        }
    }

    if (MessageManager::callAsync (notify))
        return;
#endif

    notify();
}

void AudioPeakProfileCache::notifyProfileProgress (const String& cacheKey, double progress)
{
    auto notify = [this, cacheKey, progress]
    {
        listeners.call ([&] (Listener& l)
        {
            l.profileProgress (cacheKey, progress);
        });
    };

#if YUP_MODAL_LOOPS_PERMITTED
    if (auto* mm = MessageManager::getInstanceWithoutCreating())
    {
        if (mm->isThisTheMessageThread())
        {
            notify();
            return;
        }
    }

    if (MessageManager::callAsync (notify))
        return;
#endif

    notify();
}

} // namespace yup
