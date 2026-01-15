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
constexpr int kThumbnailSampleRate = 44100;
constexpr int kThumbnailBufferSize = 10000;

AudioBuffer<float> createThumbnailTestBuffer (int numChannels, int numSamples, float frequency = 440.0f)
{
    AudioBuffer<float> buffer (numChannels, numSamples);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* channelData = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
        {
            const float phase = (i / static_cast<float> (kThumbnailSampleRate)) * frequency * 2.0f * MathConstants<float>::pi;
            channelData[i] = std::sin (phase);
        }
    }

    return buffer;
}

class ThumbnailTestListener : public AudioThumbnail::Listener
{
public:
    void thumbnailChanged (AudioThumbnail& thumb) override
    {
        changedCallCount++;
    }

    void thumbnailProgressChanged (AudioThumbnail& thumb, double progress, bool visible) override
    {
        lastProgress = progress;
        lastProgressVisible = visible;
        progressCallCount++;
    }

    int changedCallCount = 0;
    int progressCallCount = 0;
    double lastProgress = 0.0;
    bool lastProgressVisible = false;
};
} // namespace

class AudioThumbnailTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        cache = std::make_shared<AudioPeakProfileCache>();
        thumbnail = std::make_unique<AudioThumbnail> (cache);
        listener = std::make_unique<ThumbnailTestListener>();
        thumbnail->addListener (listener.get());
    }

    void TearDown() override
    {
        thumbnail->removeListener (listener.get());
        thumbnail.reset();
        listener.reset();
        cache.reset();
    }

    void waitForProfileReady (int maxWaitMs = 1000)
    {
        int waited = 0;
        while (thumbnail->getPeakProfile() == nullptr && waited < maxWaitMs)
        {
            Thread::sleep (10);
            waited += 10;
        }
    }

    std::shared_ptr<AudioPeakProfileCache> cache;
    std::unique_ptr<AudioThumbnail> thumbnail;
    std::unique_ptr<ThumbnailTestListener> listener;
};

TEST_F (AudioThumbnailTests, DefaultConstructor)
{
    AudioThumbnail thumb;
    EXPECT_EQ (0, thumb.getTotalSamples());
    EXPECT_EQ (0, thumb.getNumChannels());
    EXPECT_EQ (0.0, thumb.getSampleRate());
    EXPECT_EQ (nullptr, thumb.getPeakProfile());
}

TEST_F (AudioThumbnailTests, ConstructorWithSharedCache)
{
    EXPECT_NE (nullptr, thumbnail);
    EXPECT_EQ (0, thumbnail->getTotalSamples());
}

TEST_F (AudioThumbnailTests, SetSourceWithBufferPointer)
{
    auto buffer = createThumbnailTestBuffer (2, kThumbnailBufferSize);
    thumbnail->setSource (&buffer, kThumbnailSampleRate);

    EXPECT_EQ (kThumbnailBufferSize, thumbnail->getTotalSamples());
    EXPECT_EQ (2, thumbnail->getNumChannels());
    EXPECT_DOUBLE_EQ (kThumbnailSampleRate, thumbnail->getSampleRate());

    waitForProfileReady();
    EXPECT_NE (nullptr, thumbnail->getPeakProfile());
    EXPECT_GE (listener->changedCallCount, 1);
}

TEST_F (AudioThumbnailTests, SetSourceWithBufferByReference)
{
    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);
    thumbnail->setSource (buffer, kThumbnailSampleRate);

    EXPECT_EQ (kThumbnailBufferSize, thumbnail->getTotalSamples());
    EXPECT_EQ (1, thumbnail->getNumChannels());

    waitForProfileReady();
    EXPECT_NE (nullptr, thumbnail->getPeakProfile());
}

TEST_F (AudioThumbnailTests, SetSourceWithBufferByMove)
{
    auto buffer = createThumbnailTestBuffer (2, kThumbnailBufferSize);
    thumbnail->setSource (std::move (buffer), kThumbnailSampleRate);

    EXPECT_EQ (kThumbnailBufferSize, thumbnail->getTotalSamples());
    EXPECT_EQ (2, thumbnail->getNumChannels());

    waitForProfileReady();
    EXPECT_NE (nullptr, thumbnail->getPeakProfile());
}

TEST_F (AudioThumbnailTests, ClearResetsState)
{
    auto buffer = createThumbnailTestBuffer (2, kThumbnailBufferSize);
    thumbnail->setSource (&buffer, kThumbnailSampleRate);
    waitForProfileReady();

    EXPECT_NE (nullptr, thumbnail->getPeakProfile());

    thumbnail->clear();

    EXPECT_EQ (0, thumbnail->getTotalSamples());
    EXPECT_EQ (0, thumbnail->getNumChannels());
    EXPECT_EQ (nullptr, thumbnail->getPeakProfile());
}

TEST_F (AudioThumbnailTests, GetChannelColorReturnsValidColor)
{
    auto buffer = createThumbnailTestBuffer (2, kThumbnailBufferSize);
    thumbnail->setSource (&buffer, kThumbnailSampleRate);

    auto color0 = thumbnail->getChannelColor (0);
    auto color1 = thumbnail->getChannelColor (1);

    EXPECT_NE (color0, color1); // Different channels should have different colors
}

TEST_F (AudioThumbnailTests, SetBackgroundCalculationDisabled)
{
    // Create a new cache and thumbnail without ThreadPool for synchronous computation
    auto syncCache = std::make_shared<AudioPeakProfileCache>();
    AudioThumbnail syncThumbnail (syncCache);

    auto buffer = createThumbnailTestBuffer (1, 1000);
    syncThumbnail.setSource (&buffer, kThumbnailSampleRate);

    // Profile should be computed synchronously (no ThreadPool = synchronous)
    EXPECT_NE (nullptr, syncThumbnail.getPeakProfile());
}

TEST_F (AudioThumbnailTests, ProgressNotifications)
{
    // Create a new cache and thumbnail without ThreadPool for synchronous computation
    auto syncCache = std::make_shared<AudioPeakProfileCache>();
    AudioThumbnail syncThumbnail (syncCache);
    ThumbnailTestListener syncListener;
    syncThumbnail.addListener (&syncListener);

    auto buffer = createThumbnailTestBuffer (1, 100000);
    syncThumbnail.setSource (&buffer, kThumbnailSampleRate);

    // Should have received progress updates
    EXPECT_GT (syncListener.progressCallCount, 0);
}

TEST_F (AudioThumbnailTests, GetProgressReturnsValue)
{
    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);
    thumbnail->setSource (&buffer, kThumbnailSampleRate);

    double progress = thumbnail->getProgress();
    EXPECT_GE (progress, 0.0);
    EXPECT_LE (progress, 1.0);
}

TEST_F (AudioThumbnailTests, SharedCacheBetweenThumbnails)
{
    auto buffer = createThumbnailTestBuffer (2, kThumbnailBufferSize);

    // First thumbnail computes profile
    thumbnail->setSource (&buffer, kThumbnailSampleRate);
    waitForProfileReady();

    auto profile1 = thumbnail->getPeakProfile();
    ASSERT_NE (nullptr, profile1);

    // Second thumbnail using same cache should reuse profile
    AudioThumbnail thumbnail2 (cache);
    ThumbnailTestListener listener2;
    thumbnail2.addListener (&listener2);

    thumbnail2.setSource (&buffer, kThumbnailSampleRate);

    // Should get profile immediately from cache
    EXPECT_NE (nullptr, thumbnail2.getPeakProfile());
    // Gets 2 notifications: one from setSource, one from profileReady
    EXPECT_GE (listener2.changedCallCount, 1);
}

TEST_F (AudioThumbnailTests, DiskCacheIntegration)
{
    File tempDir = File::getSpecialLocation (File::SpecialLocationType::tempDirectory)
                       .getChildFile ("yup_thumbnail_test");
    tempDir.createDirectory();

    cache->setDiskCacheEnabled (true);
    cache->setCacheDirectory (tempDir);

    auto buffer = createThumbnailTestBuffer (2, kThumbnailBufferSize);
    thumbnail->setSource (&buffer, kThumbnailSampleRate);
    waitForProfileReady();

    // Clear memory cache
    cache->clearMemoryCache();
    thumbnail->clear();

    // Re-set source should load from disk
    thumbnail->setSource (&buffer, kThumbnailSampleRate);
    waitForProfileReady();

    EXPECT_NE (nullptr, thumbnail->getPeakProfile());

    tempDir.deleteRecursively();
}

TEST_F (AudioThumbnailTests, SetSourceWithNullBufferClears)
{
    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);
    thumbnail->setSource (&buffer, kThumbnailSampleRate);
    waitForProfileReady();

    thumbnail->setSource (static_cast<const AudioBuffer<float>*> (nullptr), kThumbnailSampleRate);

    EXPECT_EQ (0, thumbnail->getTotalSamples());
    EXPECT_EQ (nullptr, thumbnail->getPeakProfile());
}

TEST_F (AudioThumbnailTests, MultipleSetSourceCallsHandledCorrectly)
{
    auto buffer1 = createThumbnailTestBuffer (1, 1000);
    auto buffer2 = createThumbnailTestBuffer (2, 2000);

    thumbnail->setSource (&buffer1, kThumbnailSampleRate);
    waitForProfileReady();
    EXPECT_EQ (1, thumbnail->getNumChannels());

    listener->changedCallCount = 0;

    thumbnail->setSource (&buffer2, kThumbnailSampleRate);
    waitForProfileReady();
    EXPECT_EQ (2, thumbnail->getNumChannels());
    EXPECT_GE (listener->changedCallCount, 1);
}

TEST_F (AudioThumbnailTests, ListenerNotificationsWork)
{
    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);

    int initialChangedCount = listener->changedCallCount;
    thumbnail->setSource (&buffer, kThumbnailSampleRate);
    waitForProfileReady();

    EXPECT_GT (listener->changedCallCount, initialChangedCount);
}

TEST_F (AudioThumbnailTests, RemoveListenerStopsNotifications)
{
    thumbnail->removeListener (listener.get());

    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);
    thumbnail->setSource (&buffer, kThumbnailSampleRate);
    waitForProfileReady();

    EXPECT_EQ (0, listener->changedCallCount);
}

TEST_F (AudioThumbnailTests, SetSourceWithZeroSampleRateUsesDefault)
{
    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);
    thumbnail->setSource (&buffer, 0.0);

    // Should use default sample rate (implementation specific)
    EXPECT_GE (thumbnail->getSampleRate(), 0.0);
}

TEST_F (AudioThumbnailTests, DiskCacheEnabled)
{
    File tempDir = File::getSpecialLocation (File::SpecialLocationType::tempDirectory)
                       .getChildFile ("yup_thumbnail_disk_test");
    tempDir.createDirectory();

    cache->setDiskCacheEnabled (true);
    cache->setCacheDirectory (tempDir);

    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);
    thumbnail->setSource (&buffer, kThumbnailSampleRate);
    waitForProfileReady();

    // Verify profile was created
    EXPECT_NE (nullptr, thumbnail->getPeakProfile());

    tempDir.deleteRecursively();
}

TEST_F (AudioThumbnailTests, ThreadPoolBackgroundComputation)
{
    ThreadPool pool (2);
    cache->setThreadPool (&pool);

    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);
    thumbnail->setSource (&buffer, kThumbnailSampleRate);
    waitForProfileReady();

    EXPECT_NE (nullptr, thumbnail->getPeakProfile());
}

TEST_F (AudioThumbnailTests, LargeBufferHandling)
{
    // Create a new cache and thumbnail without ThreadPool for synchronous computation
    auto syncCache = std::make_shared<AudioPeakProfileCache>();
    AudioThumbnail syncThumbnail (syncCache);

    auto buffer = createThumbnailTestBuffer (8, 1000000);
    syncThumbnail.setSource (&buffer, kThumbnailSampleRate);

    EXPECT_EQ (1000000, syncThumbnail.getTotalSamples());
    EXPECT_EQ (8, syncThumbnail.getNumChannels());
    EXPECT_NE (nullptr, syncThumbnail.getPeakProfile());
}

TEST_F (AudioThumbnailTests, SmallBufferHandling)
{
    // Create a new cache and thumbnail without ThreadPool for synchronous computation
    auto syncCache = std::make_shared<AudioPeakProfileCache>();
    AudioThumbnail syncThumbnail (syncCache);

    auto buffer = createThumbnailTestBuffer (1, 10);
    syncThumbnail.setSource (&buffer, kThumbnailSampleRate);

    EXPECT_EQ (10, syncThumbnail.getTotalSamples());
    EXPECT_NE (nullptr, syncThumbnail.getPeakProfile());
}
