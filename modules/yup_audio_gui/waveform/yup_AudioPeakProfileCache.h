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
    Manages caching of AudioPeakProfile objects in memory and on disk.

    This class coordinates peak profile computation, storage, and retrieval. It provides:
    - In-memory caching of recently used profiles
    - Optional disk cache persistence
    - Background computation via ThreadPool
    - Progress callbacks during profile generation

    Peak profiles are identified by cache keys, which are generated from the audio source
    (file path + modification time, or buffer content hash).

    @see AudioPeakProfile, AudioThumbnail
*/
class YUP_API AudioPeakProfileCache
{
public:
    //==============================================================================
    /** Receives notifications about profile generation progress and completion. */
    class Listener
    {
    public:
        virtual ~Listener() = default;

        /** Called when a profile has been successfully generated and cached.

            @param cacheKey  The cache key identifying the profile
            @param profile   The completed profile
        */
        virtual void profileReady (const String& cacheKey, std::shared_ptr<AudioPeakProfile> profile) = 0;

        /** Called periodically during profile generation to report progress.

            @param cacheKey  The cache key identifying the profile
            @param progress  Progress value from 0.0 to 1.0
        */
        virtual void profileProgress (const String& cacheKey, double progress) = 0;
    };

    //==============================================================================
    /** Creates an empty cache. */
    AudioPeakProfileCache();

    /** Destructor. Cancels any pending background jobs. */
    ~AudioPeakProfileCache();

    //==============================================================================
    /** Enables or disables disk caching of profiles.

        When enabled, profiles are saved to disk after generation and loaded from
        disk when available, avoiding recomputation.

        @param enabled  True to enable disk caching
    */
    void setDiskCacheEnabled (bool enabled) noexcept;

    /** Returns true if disk caching is enabled. */
    bool isDiskCacheEnabled() const noexcept { return diskCacheEnabled; }

    /** Sets the directory where disk cache files are stored.

        @param directory  The cache directory path
    */
    void setCacheDirectory (const File& directory);

    /** Returns the current cache directory. */
    File getCacheDirectory() const noexcept { return cacheDirectory; }

    /** Sets the thread pool to use for background profile computation.

        If null, profiles will be computed synchronously on the calling thread.

        @param pool  The thread pool to use, or nullptr for synchronous computation
    */
    void setThreadPool (ThreadPool* pool) noexcept;

    //==============================================================================
    /** Returns a cached profile if available, or nullptr if not found.

        This checks the in-memory cache only. Use requestProfile() to trigger
        computation if the profile is not found.

        @param cacheKey  The cache key identifying the profile
        @returns         The cached profile, or nullptr if not found
    */
    std::shared_ptr<AudioPeakProfile> getProfile (const String& cacheKey);

    /** Requests a profile, computing it if not already cached.

        This method checks the in-memory cache, then the disk cache (if enabled),
        and finally calls the buildFunction to generate a new profile if needed.

        @param cacheKey       The cache key identifying the profile
        @param buildFunction  Function that builds the profile if needed
        @param useBackground  True to compute in background thread (if ThreadPool is set)
    */
    void requestProfile (const String& cacheKey,
                         std::function<std::shared_ptr<AudioPeakProfile>()> buildFunction,
                         bool useBackground = true);

    /** Cancels any pending profile requests for the given cache key.

        @param cacheKey  The cache key identifying the profile to cancel
    */
    void cancelPendingRequests (const String& cacheKey);

    /** Clears all profiles from the in-memory cache. */
    void clearMemoryCache();

    //==============================================================================
    /** Registers a listener to receive cache notifications. */
    void addListener (Listener* listener);

    /** Unregisters a listener. */
    void removeListener (Listener* listener);

    //==============================================================================
    /** Generates a cache key from an audio file.

        The key is based on the file path hash and modification time, ensuring
        that the cache is invalidated when the file changes.

        @param audioFile  The audio file
        @returns          A unique cache key string
    */
    static String generateCacheKey (const File& audioFile);

    /** Generates a cache key from an audio buffer and sample rate.

        The key is based on a hash of the buffer metadata and sampled content.

        @param buffer      The audio buffer
        @param sampleRate  The sample rate
        @returns           A unique cache key string
    */
    static String generateCacheKey (const AudioBuffer<float>& buffer, double sampleRate);

    /** Returns the total size of all disk cache files in bytes.

        @returns  The disk cache size, or 0 if disk cache is disabled
    */
    int64 getDiskCacheSize() const;

    /** Deletes all files in the disk cache directory. */
    void clearDiskCache();

private:
    //==============================================================================
    struct CacheEntry
    {
        std::shared_ptr<AudioPeakProfile> profile;
        String cacheKey;
        int64 lastAccessTime = 0;
    };

    struct PendingJob : public ThreadPoolJob
    {
        PendingJob (AudioPeakProfileCache& owner,
                    const String& key,
                    std::function<std::shared_ptr<AudioPeakProfile>()> buildFn,
                    int jobId);

        JobStatus runJob() override;

        AudioPeakProfileCache& cache;
        String cacheKey;
        std::function<std::shared_ptr<AudioPeakProfile>()> buildFunction;
        int jobId;
    };

    File getCacheFilePath (const String& cacheKey) const;
    std::shared_ptr<AudioPeakProfile> loadFromDisk (const String& cacheKey);
    void saveToDisk (const String& cacheKey, const AudioPeakProfile& profile);
    void notifyProfileReady (const String& cacheKey, std::shared_ptr<AudioPeakProfile> profile);
    void notifyProfileProgress (const String& cacheKey, double progress);

    bool diskCacheEnabled = false;
    File cacheDirectory;
    ThreadPool* threadPool = nullptr;

    std::map<String, CacheEntry> memoryCache;
    CriticalSection cacheLock;

    ListenerList<Listener> listeners;
    std::atomic<int> jobCounter { 0 };
    std::map<String, int> pendingJobs; // cacheKey -> jobId
    CriticalSection jobsLock;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPeakProfileCache)
};

} // namespace yup
