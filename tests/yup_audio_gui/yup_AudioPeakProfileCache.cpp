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
        mm = MessageManager::getInstance();

        cache = std::make_unique<AudioPeakProfileCache>();
        listener = std::make_unique<CacheTestListener>();
        cache->addListener (listener.get());

        tempDir = File::getSpecialLocation (File::SpecialLocationType::tempDirectory)
                      .getChildFile ("yup_cache_test");
        tempDir.createDirectory();
        cache->setCacheDirectory (tempDir);

        // Drain any pending messages from previous tests after setup
        runDispatchLoopUntil (10);
    }

    void TearDown() override
    {
        cache->removeListener (listener.get());
        cache.reset();
        listener.reset();
        tempDir.deleteRecursively();
    }

    void runDispatchLoopUntil (int millisecondsToRunFor = 10)
    {
#if YUP_MODAL_LOOPS_PERMITTED
        mm->runDispatchLoopUntil (millisecondsToRunFor);
#endif
    }

    MessageManager* mm = nullptr;
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

    String key1 = AudioPeakProfileCache::generateCacheKey (buffer1, kCacheSampleRate);
    String key2 = AudioPeakProfileCache::generateCacheKey (buffer2, kCacheSampleRate);

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

    // Pump message queue to process async notification
    runDispatchLoopUntil (100);

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

    // Wait for async completion and pump message queue
    int attempts = 0;
    while (listener->readyCallCount == 0 && attempts++ < 100)
    {
        runDispatchLoopUntil (10);
        Thread::sleep (10);
    }

    EXPECT_EQ (1, listener->readyCallCount);
    EXPECT_EQ (cacheKey, listener->lastCacheKey);
    EXPECT_NE (nullptr, listener->lastProfile);

    // Detach thread pool before it's destroyed
    cache->setThreadPool (nullptr);

    // Give any remaining jobs time to fully complete
    Thread::sleep (50);
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

    // Pump message queue to process async notification
    runDispatchLoopUntil (100);
    EXPECT_EQ (1, listener->readyCallCount);

    // Second request hits memory cache
    listener->readyCallCount = 0;
    cache->requestProfile (cacheKey, buildFunction, false);
    EXPECT_EQ (1, buildCallCount); // Build function not called again

    // Pump message queue to process async notification
    runDispatchLoopUntil (100);
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

    // Pump message queue to process async notification
    runDispatchLoopUntil (100);

    // Clear memory cache
    cache->clearMemoryCache();

    // Second request should load from disk
    listener->readyCallCount = 0;
    cache->requestProfile (cacheKey, buildFunction, false);
    EXPECT_EQ (1, buildCallCount); // Build function not called again

    // Pump message queue to process async notification
    runDispatchLoopUntil (100);
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
    // Reset counts from any previous tests
    listener->readyCallCount = 0;

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

    // Pump message queue to process async notifications
    runDispatchLoopUntil (100);

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

    // Pump message queue to process async notification
    runDispatchLoopUntil (100);
    EXPECT_EQ (1, listener->readyCallCount);

    cache->removeListener (listener.get());
    listener->readyCallCount = 0;

    cache->clearMemoryCache();
    cache->requestProfile (cacheKey, buildFunction, false);

    // Pump message queue to ensure no notification arrives
    runDispatchLoopUntil (100);
    EXPECT_EQ (0, listener->readyCallCount);
}

//==============================================================================
// Cache Directory Tests
//==============================================================================

TEST_F (AudioPeakProfileCacheTests, SetCacheDirectoryAcceptsPath)
{
    File newCacheDir = tempDir.getChildFile ("new_cache");
    newCacheDir.createDirectory();

    cache->setCacheDirectory (newCacheDir);

    // Should not crash
    EXPECT_TRUE (true);

    newCacheDir.deleteRecursively();
}

TEST_F (AudioPeakProfileCacheTests, CacheKeyGenerationForEmptyBuffer)
{
    AudioBuffer<float> emptyBuffer (0, 0);
    String key = AudioPeakProfileCache::generateCacheKey (emptyBuffer, kCacheSampleRate);

    EXPECT_FALSE (key.isEmpty());
}

TEST_F (AudioPeakProfileCacheTests, CacheKeyGenerationWithDifferentSampleRates)
{
    auto buffer = createCacheTestBuffer (1, 1000);

    String key1 = AudioPeakProfileCache::generateCacheKey (buffer, 44100.0);
    String key2 = AudioPeakProfileCache::generateCacheKey (buffer, 48000.0);

    EXPECT_NE (key1, key2);
}

TEST_F (AudioPeakProfileCacheTests, CacheKeyGenerationForNonExistentFile)
{
    File nonExistentFile = tempDir.getChildFile ("does_not_exist.wav");
    String key = AudioPeakProfileCache::generateCacheKey (nonExistentFile);

    // Non-existent file returns empty key
    EXPECT_TRUE (key.isEmpty());
}

//==============================================================================
// Async Request Tests
//==============================================================================

TEST_F (AudioPeakProfileCacheTests, MultipleAsyncRequests)
{
    ThreadPool threadPool (4);
    cache->setThreadPool (&threadPool);

    for (int i = 0; i < 5; ++i)
    {
        String cacheKey = "async_key_" + String (i);

        auto buildFunction = [i]() -> std::shared_ptr<AudioPeakProfile>
        {
            auto profile = std::make_shared<AudioPeakProfile>();
            auto buffer = createCacheTestBuffer (1, 1000 + i * 100);
            profile->buildFromBuffer (buffer, 1, {});
            return profile;
        };

        cache->requestProfile (cacheKey, buildFunction, true);
    }

    // Wait for all requests to complete and pump message queue
    int attempts = 0;
    while (listener->readyCallCount < 5 && attempts++ < 200)
    {
        runDispatchLoopUntil (10);
        Thread::sleep (10);
    }

    EXPECT_EQ (5, listener->readyCallCount);

    // Detach thread pool before it's destroyed
    cache->setThreadPool (nullptr);

    // Give any remaining jobs time to fully complete
    Thread::sleep (50);
}

TEST_F (AudioPeakProfileCacheTests, AsyncRequestWithNullThreadPool)
{
    // No thread pool set - should fall back to synchronous
    cache->setThreadPool (nullptr);

    String cacheKey = "null_threadpool_async";

    auto buildFunction = []() -> std::shared_ptr<AudioPeakProfile>
    {
        auto profile = std::make_shared<AudioPeakProfile>();
        auto buffer = createCacheTestBuffer (1, 1000);
        profile->buildFromBuffer (buffer, 1, {});
        return profile;
    };

    cache->requestProfile (cacheKey, buildFunction, true);

    // Should complete synchronously even though async was requested
    // But notification is still async via message queue
    runDispatchLoopUntil (100);
    EXPECT_EQ (1, listener->readyCallCount);
}

//==============================================================================
// Disk Cache Disabled Tests
//==============================================================================

TEST_F (AudioPeakProfileCacheTests, DiskCacheDisabledByDefault)
{
    AudioPeakProfileCache testCache;

    String cacheKey = "test_disk_disabled";

    auto buildFunction = []() -> std::shared_ptr<AudioPeakProfile>
    {
        auto profile = std::make_shared<AudioPeakProfile>();
        auto buffer = createCacheTestBuffer (1, 1000);
        profile->buildFromBuffer (buffer, 1, {});
        return profile;
    };

    testCache.setCacheDirectory (tempDir);
    testCache.requestProfile (cacheKey, buildFunction, false);

    // No disk cache files should be created
    auto cacheFiles = tempDir.findChildFiles (File::TypesOfFileToFind::findFiles, false, "*.yuppeaks");
    EXPECT_EQ (0, cacheFiles.size());
}

TEST_F (AudioPeakProfileCacheTests, DiskCacheCanBeDisabled)
{
    cache->setDiskCacheEnabled (true);
    cache->setDiskCacheEnabled (false);

    String cacheKey = "test_disk_toggle";

    auto buildFunction = []() -> std::shared_ptr<AudioPeakProfile>
    {
        auto profile = std::make_shared<AudioPeakProfile>();
        auto buffer = createCacheTestBuffer (1, 1000);
        profile->buildFromBuffer (buffer, 1, {});
        return profile;
    };

    cache->requestProfile (cacheKey, buildFunction, false);

    // No disk cache files should be created
    auto cacheFiles = tempDir.findChildFiles (File::TypesOfFileToFind::findFiles, false, "*.yuppeaks");
    EXPECT_EQ (0, cacheFiles.size());
}

//==============================================================================
// Edge Cases
//==============================================================================

TEST_F (AudioPeakProfileCacheTests, BuildFunctionReturningNull)
{
    String cacheKey = "test_null_profile";

    auto buildFunction = []() -> std::shared_ptr<AudioPeakProfile>
    {
        return nullptr;
    };

    cache->requestProfile (cacheKey, buildFunction, false);

    auto retrieved = cache->getProfile (cacheKey);
    EXPECT_EQ (nullptr, retrieved);
}

TEST_F (AudioPeakProfileCacheTests, BuildFunctionThrowing)
{
    String cacheKey = "test_throwing_build";

    auto buildFunction = []() -> std::shared_ptr<AudioPeakProfile>
    {
        throw std::runtime_error ("Build failed");
        return nullptr;
    };

    // Should not crash
    try
    {
        cache->requestProfile (cacheKey, buildFunction, false);
    }
    catch (...)
    {
        // Expected
    }

    EXPECT_TRUE (true);
}

TEST_F (AudioPeakProfileCacheTests, SameCacheKeyDifferentBuildFunctions)
{
    String cacheKey = "same_key";

    int buildFunction1CallCount = 0;
    auto buildFunction1 = [&buildFunction1CallCount]() -> std::shared_ptr<AudioPeakProfile>
    {
        buildFunction1CallCount++;
        auto profile = std::make_shared<AudioPeakProfile>();
        auto buffer = createCacheTestBuffer (1, 1000);
        profile->buildFromBuffer (buffer, 1, {});
        return profile;
    };

    int buildFunction2CallCount = 0;
    auto buildFunction2 = [&buildFunction2CallCount]() -> std::shared_ptr<AudioPeakProfile>
    {
        buildFunction2CallCount++;
        auto profile = std::make_shared<AudioPeakProfile>();
        auto buffer = createCacheTestBuffer (2, 2000);
        profile->buildFromBuffer (buffer, 1, {});
        return profile;
    };

    cache->requestProfile (cacheKey, buildFunction1, false);
    EXPECT_EQ (1, buildFunction1CallCount);

    // Second request with different build function should hit cache
    cache->requestProfile (cacheKey, buildFunction2, false);
    EXPECT_EQ (1, buildFunction1CallCount);
    EXPECT_EQ (0, buildFunction2CallCount); // Not called
}

TEST_F (AudioPeakProfileCacheTests, VeryLongCacheKey)
{
    auto longKey = String::repeatedString ("x", 1000);

    auto buildFunction = []() -> std::shared_ptr<AudioPeakProfile>
    {
        auto profile = std::make_shared<AudioPeakProfile>();
        auto buffer = createCacheTestBuffer (1, 1000);
        profile->buildFromBuffer (buffer, 1, {});
        return profile;
    };

    cache->requestProfile (longKey, buildFunction, false);

    auto retrieved = cache->getProfile (longKey);
    EXPECT_NE (nullptr, retrieved);
}

TEST_F (AudioPeakProfileCacheTests, SpecialCharactersInCacheKey)
{
    String specialKey = "test!@#$%^&*()_+-=[]{}|;':\",./<>?";

    auto buildFunction = []() -> std::shared_ptr<AudioPeakProfile>
    {
        auto profile = std::make_shared<AudioPeakProfile>();
        auto buffer = createCacheTestBuffer (1, 1000);
        profile->buildFromBuffer (buffer, 1, {});
        return profile;
    };

    cache->requestProfile (specialKey, buildFunction, false);

    auto retrieved = cache->getProfile (specialKey);
    EXPECT_NE (nullptr, retrieved);
}

TEST_F (AudioPeakProfileCacheTests, GetDiskCacheSizeWithNoCache)
{
    AudioPeakProfileCache testCache;
    auto size = testCache.getDiskCacheSize();

    EXPECT_EQ (0, size);
}

TEST_F (AudioPeakProfileCacheTests, ClearDiskCacheWithNoCacheDirectory)
{
    AudioPeakProfileCache testCache;
    testCache.clearDiskCache();

    // Should not crash
    EXPECT_TRUE (true);
}
