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

#include <yup_audio_gui/yup_audio_gui.h>

using namespace yup;

namespace
{
constexpr int kCacheSampleRate = 44100;
constexpr int kCacheBufferSize = 10000;

AudioBuffer<float> createCacheTestBuffer (int numChannels, int numSamples)
{
    AudioBuffer<float> buffer (numChannels, numSamples);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* channelData = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
            channelData[i] = static_cast<float> (i) / numSamples;
    }

    return buffer;
}

class CacheTestListener : public AudioPeakProfileCache::Listener
{
public:
    void profileReady (const String& key, std::shared_ptr<AudioPeakProfile> prof) override
    {
        lastCacheKey = key;
        lastProfile = prof;
        readyCallCount++;
    }

    void profileProgress (const String& key, double prog) override
    {
        lastCacheKey = key;
        lastProgress = prog;
        progressCallCount++;
    }

    String lastCacheKey;
    std::shared_ptr<AudioPeakProfile> lastProfile;
    double lastProgress = 0.0;
    int readyCallCount = 0;
    int progressCallCount = 0;
};
} // namespace

class AudioPeakProfileCacheTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        cache = std::make_unique<AudioPeakProfileCache>();
        listener = std::make_unique<CacheTestListener>();
        cache->addListener (listener.get());

        tempDir = File::getSpecialLocation (File::SpecialLocationType::tempDirectory)
                      .getChildFile ("yup_cache_test");
        tempDir.createDirectory();
        cache->setCacheDirectory (tempDir);
    }

    void TearDown() override
    {
        cache->removeListener (listener.get());
        cache.reset();
        listener.reset();
        tempDir.deleteRecursively();
    }

    std::unique_ptr<AudioPeakProfileCache> cache;
    std::unique_ptr<CacheTestListener> listener;
    File tempDir;
};

TEST_F (AudioPeakProfileCacheTests, DefaultConstructor)
{
    AudioPeakProfileCache testCache;
    EXPECT_EQ (nullptr, testCache.getProfile ("nonexistent"));
}

TEST_F (AudioPeakProfileCacheTests, GenerateCacheKeyFromFile)
{
    File testFile = tempDir.getChildFile ("test_audio.wav");
    testFile.create();

    String key1 = AudioPeakProfileCache::generateCacheKey (testFile);
    EXPECT_FALSE (key1.isEmpty());

    String key2 = AudioPeakProfileCache::generateCacheKey (testFile);
    EXPECT_EQ (key1, key2); // Same file should produce same key

    // Modify file and verify key changes
    Thread::sleep (1100); // Ensure modification time changes
    testFile.appendText ("modified");
    String key3 = AudioPeakProfileCache::generateCacheKey (testFile);
    EXPECT_NE (key1, key3);

    testFile.deleteFile();
}

TEST_F (AudioPeakProfileCacheTests, GenerateCacheKeyFromBuffer)
{
    auto buffer1 = createCacheTestBuffer (2, kCacheBufferSize);
    auto buffer2 = createCacheTestBuffer (2, kCacheBufferSize);

    String key1 = AudioPeakProfileCache::generateCacheKey (buffer1, kTestSampleRate);
    String key2 = AudioPeakProfileCache::generateCacheKey (buffer2, kTestSampleRate);

    // Same content should produce same key
    EXPECT_EQ (key1, key2);

    // Different buffer should produce different key
    auto buffer3 = createCacheTestBuffer (1, kCacheBufferSize);
    String key3 = AudioPeakProfileCache::generateCacheKey (buffer3, kTestSampleRate);
    EXPECT_NE (key1, key3);
}

TEST_F (AudioPeakProfileCacheTests, RequestProfileSynchronously)
{
    String cacheKey = "test_sync_key";
    bool buildFunctionCalled = false;

    auto buildFunction = [&buildFunctionCalled]() -> std::shared_ptr<AudioPeakProfile>
    {
        buildFunctionCalled = true;
        auto profile = std::make_shared<AudioPeakProfile>();
        auto buffer = createCacheTestBuffer (1, 1000);
        profile->buildFromBuffer (buffer, 1, {});
        return profile;
    };

    cache->requestProfile (cacheKey, buildFunction, false);

    EXPECT_TRUE (buildFunctionCalled);
    EXPECT_EQ (1, listener->readyCallCount);
    EXPECT_EQ (cacheKey, listener->lastCacheKey);
    EXPECT_NE (nullptr, listener->lastProfile);
    EXPECT_TRUE (listener->lastProfile->isValid());
}

TEST_F (AudioPeakProfileCacheTests, RequestProfileAsynchronously)
{
    ThreadPool threadPool (2);
    cache->setThreadPool (&threadPool);

    String cacheKey = "test_async_key";

    auto buildFunction = []() -> std::shared_ptr<AudioPeakProfile>
    {
        auto profile = std::make_shared<AudioPeakProfile>();
        auto buffer = createCacheTestBuffer (1, 1000);
        profile->buildFromBuffer (buffer, 1, {});
        return profile;
    };

    cache->requestProfile (cacheKey, buildFunction, true);

    // Wait for async completion
    int attempts = 0;
    while (listener->readyCallCount == 0 && attempts++ < 100)
        Thread::sleep (10);

    EXPECT_EQ (1, listener->readyCallCount);
    EXPECT_EQ (cacheKey, listener->lastCacheKey);
    EXPECT_NE (nullptr, listener->lastProfile);
}

TEST_F (AudioPeakProfileCacheTests, MemoryCacheHit)
{
    String cacheKey = "test_cache_hit";
    int buildCallCount = 0;

    auto buildFunction = [&buildCallCount]() -> std::shared_ptr<AudioPeakProfile>
    {
        buildCallCount++;
        auto profile = std::make_shared<AudioPeakProfile>();
        auto buffer = createCacheTestBuffer (1, 1000);
        profile->buildFromBuffer (buffer, 1, {});
        return profile;
    };

    // First request builds profile
    cache->requestProfile (cacheKey, buildFunction, false);
    EXPECT_EQ (1, buildCallCount);
    EXPECT_EQ (1, listener->readyCallCount);

    // Second request hits memory cache
    listener->readyCallCount = 0;
    cache->requestProfile (cacheKey, buildFunction, false);
    EXPECT_EQ (1, buildCallCount);           // Build function not called again
    EXPECT_EQ (1, listener->readyCallCount); // But listener notified
}

TEST_F (AudioPeakProfileCacheTests, GetProfileReturnsNullForMissingKey)
{
    auto profile = cache->getProfile ("nonexistent_key");
    EXPECT_EQ (nullptr, profile);
}

TEST_F (AudioPeakProfileCacheTests, GetProfileReturnsExistingProfile)
{
    String cacheKey = "test_get_profile";

    auto buildFunction = []() -> std::shared_ptr<AudioPeakProfile>
    {
        auto profile = std::make_shared<AudioPeakProfile>();
        auto buffer = createCacheTestBuffer (1, 1000);
        profile->buildFromBuffer (buffer, 1, {});
        return profile;
    };

    cache->requestProfile (cacheKey, buildFunction, false);

    auto retrieved = cache->getProfile (cacheKey);
    EXPECT_NE (nullptr, retrieved);
    EXPECT_TRUE (retrieved->isValid());
}

TEST_F (AudioPeakProfileCacheTests, DiskCacheEnabled)
{
    cache->setDiskCacheEnabled (true);

    String cacheKey = "test_disk_cache";
    int buildCallCount = 0;

    auto buildFunction = [&buildCallCount]() -> std::shared_ptr<AudioPeakProfile>
    {
        buildCallCount++;
        auto profile = std::make_shared<AudioPeakProfile>();
        auto buffer = createCacheTestBuffer (1, 1000);
        profile->buildFromBuffer (buffer, 1, {});
        return profile;
    };

    // First request builds and saves to disk
    cache->requestProfile (cacheKey, buildFunction, false);
    EXPECT_EQ (1, buildCallCount);

    // Clear memory cache
    cache->clearMemoryCache();

    // Second request should load from disk
    listener->readyCallCount = 0;
    cache->requestProfile (cacheKey, buildFunction, false);
    EXPECT_EQ (1, buildCallCount);           // Build function not called again
    EXPECT_EQ (1, listener->readyCallCount); // Loaded from disk
}

TEST_F (AudioPeakProfileCacheTests, ClearMemoryCacheRemovesEntries)
{
    String cacheKey = "test_clear";

    auto buildFunction = []() -> std::shared_ptr<AudioPeakProfile>
    {
        auto profile = std::make_shared<AudioPeakProfile>();
        auto buffer = createCacheTestBuffer (1, 1000);
        profile->buildFromBuffer (buffer, 1, {});
        return profile;
    };

    cache->requestProfile (cacheKey, buildFunction, false);
    EXPECT_NE (nullptr, cache->getProfile (cacheKey));

    cache->clearMemoryCache();
    EXPECT_EQ (nullptr, cache->getProfile (cacheKey));
}

TEST_F (AudioPeakProfileCacheTests, ClearDiskCacheRemovesFiles)
{
    cache->setDiskCacheEnabled (true);

    String cacheKey = "test_clear_disk";

    auto buildFunction = []() -> std::shared_ptr<AudioPeakProfile>
    {
        auto profile = std::make_shared<AudioPeakProfile>();
        auto buffer = createCacheTestBuffer (1, 1000);
        profile->buildFromBuffer (buffer, 1, {});
        return profile;
    };

    cache->requestProfile (cacheKey, buildFunction, false);

    // Verify cache file exists
    auto cacheFiles = tempDir.findChildFiles (File::TypesOfFileToFind::findFiles, false, "*.yuppeaks");
    EXPECT_GT (cacheFiles.size(), 0);

    cache->clearDiskCache();

    cacheFiles = tempDir.findChildFiles (File::TypesOfFileToFind::findFiles, false, "*.yuppeaks");
    EXPECT_EQ (0, cacheFiles.size());
}

TEST_F (AudioPeakProfileCacheTests, GetDiskCacheSizeReturnsCorrectValue)
{
    cache->setDiskCacheEnabled (true);

    auto buildFunction = []() -> std::shared_ptr<AudioPeakProfile>
    {
        auto profile = std::make_shared<AudioPeakProfile>();
        auto buffer = createCacheTestBuffer (2, 10000);
        profile->buildFromBuffer (buffer, 1, { 16 });
        return profile;
    };

    auto initialSize = cache->getDiskCacheSize();
    EXPECT_EQ (0, initialSize);

    cache->requestProfile ("key1", buildFunction, false);
    cache->requestProfile ("key2", buildFunction, false);

    auto newSize = cache->getDiskCacheSize();
    EXPECT_GT (newSize, initialSize);
}

TEST_F (AudioPeakProfileCacheTests, ProgressCallbacksWork)
{
    String cacheKey = "test_progress";

    auto buildFunction = [this, &cacheKey]() -> std::shared_ptr<AudioPeakProfile>
    {
        auto profile = std::make_shared<AudioPeakProfile>();
        auto buffer = createCacheTestBuffer (1, 100000);

        auto progressCallback = [this, &cacheKey] (double progress) -> bool
        {
            // Simulate cache notifying listeners
            for (auto* l : Array<AudioPeakProfileCache::Listener*> { listener.get() })
                l->profileProgress (cacheKey, progress);
            return true;
        };

        profile->buildFromBuffer (buffer, 1, { 16, 256 }, progressCallback);
        return profile;
    };

    cache->requestProfile (cacheKey, buildFunction, false);

    EXPECT_GT (listener->progressCallCount, 0);
    EXPECT_GE (listener->lastProgress, 0.0);
}

TEST_F (AudioPeakProfileCacheTests, MultipleListenersNotified)
{
    CacheTestListener listener2;
    cache->addListener (&listener2);

    String cacheKey = "test_multiple_listeners";

    auto buildFunction = []() -> std::shared_ptr<AudioPeakProfile>
    {
        auto profile = std::make_shared<AudioPeakProfile>();
        auto buffer = createCacheTestBuffer (1, 1000);
        profile->buildFromBuffer (buffer, 1, {});
        return profile;
    };

    cache->requestProfile (cacheKey, buildFunction, false);

    EXPECT_EQ (1, listener->readyCallCount);
    EXPECT_EQ (1, listener2.readyCallCount);

    cache->removeListener (&listener2);
}

TEST_F (AudioPeakProfileCacheTests, RemoveListenerStopsNotifications)
{
    String cacheKey = "test_remove_listener";

    auto buildFunction = []() -> std::shared_ptr<AudioPeakProfile>
    {
        auto profile = std::make_shared<AudioPeakProfile>();
        auto buffer = createCacheTestBuffer (1, 1000);
        profile->buildFromBuffer (buffer, 1, {});
        return profile;
    };

    cache->requestProfile (cacheKey, buildFunction, false);
    EXPECT_EQ (1, listener->readyCallCount);

    cache->removeListener (listener.get());
    listener->readyCallCount = 0;

    cache->clearMemoryCache();
    cache->requestProfile (cacheKey, buildFunction, false);
    EXPECT_EQ (0, listener->readyCallCount);
}
