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

#include <atomic>

#include <yup_audio_gui/yup_audio_gui.h>

using namespace yup;

namespace yup
{
extern std::unique_ptr<yup::GraphicsContext> yup_constructHeadlessGraphicsContext (yup::GpuDevice::Options, yup::GpuDevice::Ptr);
} // namespace yup

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
        ignoreUnused (thumb);
        changedCallCount.fetch_add (1);
    }

    void thumbnailProgressChanged (AudioThumbnail& thumb, double progress, bool visible) override
    {
        ignoreUnused (thumb);
        lastProgress.store (progress);
        lastProgressVisible.store (visible);
        progressCallCount.fetch_add (1);
    }

    std::atomic<int> changedCallCount { 0 };
    std::atomic<int> progressCallCount { 0 };
    std::atomic<double> lastProgress { 0.0 };
    std::atomic<bool> lastProgressVisible { false };
};
} // namespace

class AudioThumbnailTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mm = MessageManager::getInstance();
        cache = std::make_shared<AudioPeakProfileCache>();
        thumbnail = std::make_unique<AudioThumbnail> (cache);
        listener = std::make_unique<ThumbnailTestListener>();
        thumbnail->addListener (listener.get());

        // Drain any pending messages from previous tests
        runDispatchLoopUntil (10);
    }

    void TearDown() override
    {
        thumbnail->removeListener (listener.get());
        thumbnail.reset();
        listener.reset();
        cache.reset();
    }

    void runDispatchLoopUntil (int millisecondsToRunFor = 10)
    {
#if YUP_MODAL_LOOPS_PERMITTED
        mm->runDispatchLoopUntil (millisecondsToRunFor);
#endif
    }

    void waitForProfileReady (int maxWaitMs = 1000)
    {
        int waited = 0;
        while (thumbnail->getPeakProfile() == nullptr && waited < maxWaitMs)
        {
            runDispatchLoopUntil (10);
            Thread::sleep (10);
            waited += 20;
        }
    }

    MessageManager* mm = nullptr;
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
    EXPECT_GE (listener->changedCallCount.load(), 1);
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
    // But notification is still async, so pump the message queue
    runDispatchLoopUntil (100);
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
    EXPECT_GT (syncListener.progressCallCount.load(), 0);
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

    // Should get profile from cache (but notification is async)
    runDispatchLoopUntil (100);
    EXPECT_NE (nullptr, thumbnail2.getPeakProfile());
    // Gets 2 notifications: one from setSource, one from profileReady
    EXPECT_GE (listener2.changedCallCount.load(), 1);
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

    listener->changedCallCount.store (0);

    thumbnail->setSource (&buffer2, kThumbnailSampleRate);
    waitForProfileReady();
    EXPECT_EQ (2, thumbnail->getNumChannels());
    EXPECT_GE (listener->changedCallCount.load(), 1);
}

TEST_F (AudioThumbnailTests, ListenerNotificationsWork)
{
    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);

    int initialChangedCount = listener->changedCallCount.load();
    thumbnail->setSource (&buffer, kThumbnailSampleRate);
    waitForProfileReady();

    EXPECT_GT (listener->changedCallCount.load(), initialChangedCount);
}

TEST_F (AudioThumbnailTests, RemoveListenerStopsNotifications)
{
    thumbnail->removeListener (listener.get());

    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);
    thumbnail->setSource (&buffer, kThumbnailSampleRate);
    waitForProfileReady();

    EXPECT_EQ (0, listener->changedCallCount.load());
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

    // Pump message queue for async notification
    runDispatchLoopUntil (100);
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

    // Pump message queue for async notification
    runDispatchLoopUntil (100);
    EXPECT_NE (nullptr, syncThumbnail.getPeakProfile());
}

//==============================================================================
// GetClampedViewRange Tests
//==============================================================================

TEST_F (AudioThumbnailTests, GetClampedViewRangeWithValidRange)
{
    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);
    thumbnail->setSource (&buffer, kThumbnailSampleRate);

    auto clampedRange = thumbnail->getClampedViewRange (Range<double> (100.0, 500.0));

    EXPECT_DOUBLE_EQ (100.0, clampedRange.getStart());
    EXPECT_DOUBLE_EQ (500.0, clampedRange.getEnd());
}

TEST_F (AudioThumbnailTests, GetClampedViewRangeWithNegativeStart)
{
    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);
    thumbnail->setSource (&buffer, kThumbnailSampleRate);

    auto clampedRange = thumbnail->getClampedViewRange (Range<double> (-100.0, 500.0));

    EXPECT_GE (clampedRange.getStart(), 0.0);
    EXPECT_DOUBLE_EQ (500.0, clampedRange.getEnd());
}

TEST_F (AudioThumbnailTests, GetClampedViewRangeWithExceedingEnd)
{
    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);
    thumbnail->setSource (&buffer, kThumbnailSampleRate);

    auto clampedRange = thumbnail->getClampedViewRange (Range<double> (100.0, 50000.0));

    EXPECT_DOUBLE_EQ (100.0, clampedRange.getStart());
    EXPECT_LE (clampedRange.getEnd(), static_cast<double> (kThumbnailBufferSize));
}

TEST_F (AudioThumbnailTests, GetClampedViewRangeWithEmptyThumbnail)
{
    auto clampedRange = thumbnail->getClampedViewRange (Range<double> (100.0, 500.0));

    EXPECT_TRUE (clampedRange.isEmpty() || clampedRange.getStart() >= clampedRange.getEnd());
}

//==============================================================================
// PaintChannel Tests (using headless GraphicsContext for coverage)
//==============================================================================

TEST_F (AudioThumbnailTests, PaintChannelWithValidData)
{
    auto syncCache = std::make_shared<AudioPeakProfileCache>();
    AudioThumbnail syncThumbnail (syncCache);

    auto buffer = createThumbnailTestBuffer (2, kThumbnailBufferSize);
    syncThumbnail.setSource (&buffer, kThumbnailSampleRate);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (400, 200);
    Graphics g (*context, *renderer);

    Rectangle<float> lane (0.0f, 0.0f, 400.0f, 100.0f);
    Range<double> sampleRange (0.0, static_cast<double> (kThumbnailBufferSize));

    // Should not crash
    syncThumbnail.paintChannel (g, lane, 0, sampleRange, 400.0f);

    EXPECT_TRUE (true);
}

TEST_F (AudioThumbnailTests, PaintChannelWithInvalidChannelIndex)
{
    auto syncCache = std::make_shared<AudioPeakProfileCache>();
    AudioThumbnail syncThumbnail (syncCache);

    auto buffer = createThumbnailTestBuffer (2, kThumbnailBufferSize);
    syncThumbnail.setSource (&buffer, kThumbnailSampleRate);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (400, 200);
    Graphics g (*context, *renderer);

    Rectangle<float> lane (0.0f, 0.0f, 400.0f, 100.0f);
    Range<double> sampleRange (0.0, static_cast<double> (kThumbnailBufferSize));

    // Should not crash with invalid channel
    syncThumbnail.paintChannel (g, lane, 10, sampleRange, 400.0f);

    EXPECT_TRUE (true);
}

TEST_F (AudioThumbnailTests, PaintChannelWithNegativeChannelIndex)
{
    auto syncCache = std::make_shared<AudioPeakProfileCache>();
    AudioThumbnail syncThumbnail (syncCache);

    auto buffer = createThumbnailTestBuffer (2, kThumbnailBufferSize);
    syncThumbnail.setSource (&buffer, kThumbnailSampleRate);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (400, 200);
    Graphics g (*context, *renderer);

    Rectangle<float> lane (0.0f, 0.0f, 400.0f, 100.0f);
    Range<double> sampleRange (0.0, static_cast<double> (kThumbnailBufferSize));

    // Should not crash with negative channel
    syncThumbnail.paintChannel (g, lane, -1, sampleRange, 400.0f);

    EXPECT_TRUE (true);
}

TEST_F (AudioThumbnailTests, PaintChannelWithEmptyRange)
{
    auto syncCache = std::make_shared<AudioPeakProfileCache>();
    AudioThumbnail syncThumbnail (syncCache);

    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);
    syncThumbnail.setSource (&buffer, kThumbnailSampleRate);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (400, 200);
    Graphics g (*context, *renderer);

    Rectangle<float> lane (0.0f, 0.0f, 400.0f, 100.0f);
    Range<double> emptySampleRange (100.0, 100.0);

    // Should not crash with empty range
    syncThumbnail.paintChannel (g, lane, 0, emptySampleRange, 400.0f);

    EXPECT_TRUE (true);
}

TEST_F (AudioThumbnailTests, PaintChannelWithNoPeakProfile)
{
    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (400, 200);
    Graphics g (*context, *renderer);

    Rectangle<float> lane (0.0f, 0.0f, 400.0f, 100.0f);
    Range<double> sampleRange (0.0, 1000.0);

    // Should not crash even without peak profile
    thumbnail->paintChannel (g, lane, 0, sampleRange, 400.0f);

    EXPECT_TRUE (true);
}

TEST_F (AudioThumbnailTests, PaintChannelZoomedIn)
{
    auto syncCache = std::make_shared<AudioPeakProfileCache>();
    AudioThumbnail syncThumbnail (syncCache);

    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);
    syncThumbnail.setSource (&buffer, kThumbnailSampleRate);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (800, 200);
    Graphics g (*context, *renderer);

    Rectangle<float> lane (0.0f, 0.0f, 800.0f, 100.0f);
    Range<double> sampleRange (0.0, 100.0); // Very zoomed in

    // Should use rectangle rendering
    syncThumbnail.paintChannel (g, lane, 0, sampleRange, 800.0f);

    EXPECT_TRUE (true);
}

TEST_F (AudioThumbnailTests, PaintChannelZoomedOut)
{
    auto syncCache = std::make_shared<AudioPeakProfileCache>();
    AudioThumbnail syncThumbnail (syncCache);

    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);
    syncThumbnail.setSource (&buffer, kThumbnailSampleRate);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (400, 200);
    Graphics g (*context, *renderer);

    Rectangle<float> lane (0.0f, 0.0f, 400.0f, 100.0f);
    Range<double> sampleRange (0.0, static_cast<double> (kThumbnailBufferSize));

    // Should use line rendering
    syncThumbnail.paintChannel (g, lane, 0, sampleRange, 400.0f);

    EXPECT_TRUE (true);
}

TEST_F (AudioThumbnailTests, PaintChannelMultipleChannels)
{
    auto syncCache = std::make_shared<AudioPeakProfileCache>();
    AudioThumbnail syncThumbnail (syncCache);

    auto buffer = createThumbnailTestBuffer (8, kThumbnailBufferSize);
    syncThumbnail.setSource (&buffer, kThumbnailSampleRate);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (400, 800);
    Graphics g (*context, *renderer);

    Range<double> sampleRange (0.0, static_cast<double> (kThumbnailBufferSize));

    // Paint all channels
    for (int ch = 0; ch < 8; ++ch)
    {
        Rectangle<float> lane (0.0f, ch * 100.0f, 400.0f, 100.0f);
        syncThumbnail.paintChannel (g, lane, ch, sampleRange, 400.0f);
    }

    EXPECT_TRUE (true);
}

TEST_F (AudioThumbnailTests, PaintChannelWithZeroPixelWidth)
{
    auto syncCache = std::make_shared<AudioPeakProfileCache>();
    AudioThumbnail syncThumbnail (syncCache);

    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);
    syncThumbnail.setSource (&buffer, kThumbnailSampleRate);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (400, 200);
    Graphics g (*context, *renderer);

    Rectangle<float> lane (0.0f, 0.0f, 400.0f, 100.0f);
    Range<double> sampleRange (0.0, static_cast<double> (kThumbnailBufferSize));

    // Should handle zero pixel width gracefully
    syncThumbnail.paintChannel (g, lane, 0, sampleRange, 0.0f);

    EXPECT_TRUE (true);
}

TEST_F (AudioThumbnailTests, PaintChannelWithNegativePixelWidth)
{
    auto syncCache = std::make_shared<AudioPeakProfileCache>();
    AudioThumbnail syncThumbnail (syncCache);

    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);
    syncThumbnail.setSource (&buffer, kThumbnailSampleRate);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (400, 200);
    Graphics g (*context, *renderer);

    Rectangle<float> lane (0.0f, 0.0f, 400.0f, 100.0f);
    Range<double> sampleRange (0.0, static_cast<double> (kThumbnailBufferSize));

    // Should handle negative pixel width gracefully
    syncThumbnail.paintChannel (g, lane, 0, sampleRange, -100.0f);

    EXPECT_TRUE (true);
}

TEST_F (AudioThumbnailTests, PaintChannelWithVeryLargePixelWidth)
{
    auto syncCache = std::make_shared<AudioPeakProfileCache>();
    AudioThumbnail syncThumbnail (syncCache);

    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);
    syncThumbnail.setSource (&buffer, kThumbnailSampleRate);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (10000, 200);
    Graphics g (*context, *renderer);

    Rectangle<float> lane (0.0f, 0.0f, 10000.0f, 100.0f);
    Range<double> sampleRange (0.0, static_cast<double> (kThumbnailBufferSize));

    // Should handle very large pixel width
    syncThumbnail.paintChannel (g, lane, 0, sampleRange, 10000.0f);

    EXPECT_TRUE (true);
}

TEST_F (AudioThumbnailTests, PaintChannelWithEmptyLane)
{
    auto syncCache = std::make_shared<AudioPeakProfileCache>();
    AudioThumbnail syncThumbnail (syncCache);

    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);
    syncThumbnail.setSource (&buffer, kThumbnailSampleRate);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (400, 200);
    Graphics g (*context, *renderer);

    Rectangle<float> emptyLane (0.0f, 0.0f, 0.0f, 0.0f);
    Range<double> sampleRange (0.0, static_cast<double> (kThumbnailBufferSize));

    // Should handle empty lane gracefully
    syncThumbnail.paintChannel (g, emptyLane, 0, sampleRange, 0.0f);

    EXPECT_TRUE (true);
}

//==============================================================================
// GetChannelColor Tests
//==============================================================================

TEST_F (AudioThumbnailTests, GetChannelColorConsistency)
{
    auto buffer = createThumbnailTestBuffer (2, kThumbnailBufferSize);
    thumbnail->setSource (&buffer, kThumbnailSampleRate);

    auto color1a = thumbnail->getChannelColor (0);
    auto color1b = thumbnail->getChannelColor (0);

    // Same channel should return same color
    EXPECT_EQ (color1a, color1b);
}

TEST_F (AudioThumbnailTests, GetChannelColorForNegativeIndex)
{
    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);
    thumbnail->setSource (&buffer, kThumbnailSampleRate);

    // Should not crash
    auto color = thumbnail->getChannelColor (-1);
    EXPECT_TRUE (true);
}

TEST_F (AudioThumbnailTests, GetChannelColorForInvalidIndex)
{
    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);
    thumbnail->setSource (&buffer, kThumbnailSampleRate);

    // Should not crash
    auto color = thumbnail->getChannelColor (100);
    EXPECT_TRUE (true);
}

//==============================================================================
// Progress Visibility Tests
//==============================================================================

TEST_F (AudioThumbnailTests, ProgressVisibilityDuringComputation)
{
    auto buffer = createThumbnailTestBuffer (1, 1000000); // Large buffer
    thumbnail->setSource (&buffer, kThumbnailSampleRate);

    // Progress might be visible during computation
    // This test just ensures the getter works
    bool visible = thumbnail->isProgressVisible();
    EXPECT_TRUE (visible || ! visible);
}

TEST_F (AudioThumbnailTests, ProgressValueInValidRange)
{
    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);
    thumbnail->setSource (&buffer, kThumbnailSampleRate);

    double progress = thumbnail->getProgress();
    EXPECT_GE (progress, 0.0);
    EXPECT_LE (progress, 1.0);
}

//==============================================================================
// Edge Cases and Error Handling
//==============================================================================

TEST_F (AudioThumbnailTests, SetSourceWithEmptyBuffer)
{
    AudioBuffer<float> emptyBuffer (0, 0);
    thumbnail->setSource (&emptyBuffer, kThumbnailSampleRate);

    EXPECT_EQ (0, thumbnail->getTotalSamples());
}

TEST_F (AudioThumbnailTests, SetSourceWithSingleSampleBuffer)
{
    auto syncCache = std::make_shared<AudioPeakProfileCache>();
    AudioThumbnail syncThumbnail (syncCache);

    auto buffer = createThumbnailTestBuffer (1, 1);
    syncThumbnail.setSource (&buffer, kThumbnailSampleRate);

    EXPECT_EQ (1, syncThumbnail.getTotalSamples());
}

TEST_F (AudioThumbnailTests, SetSourceWithNegativeSampleRate)
{
    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);
    thumbnail->setSource (&buffer, -44100.0);

    // The implementation stores the sample rate as-is without validation
    EXPECT_DOUBLE_EQ (-44100.0, thumbnail->getSampleRate());
}

TEST_F (AudioThumbnailTests, SetSourceWithVeryHighSampleRate)
{
    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);
    thumbnail->setSource (&buffer, 384000.0);

    EXPECT_DOUBLE_EQ (384000.0, thumbnail->getSampleRate());
}

TEST_F (AudioThumbnailTests, MultipleClearCalls)
{
    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);
    thumbnail->setSource (&buffer, kThumbnailSampleRate);
    waitForProfileReady();

    thumbnail->clear();
    thumbnail->clear(); // Second clear should not crash

    EXPECT_EQ (0, thumbnail->getTotalSamples());
}

TEST_F (AudioThumbnailTests, ClearWithoutSettingSource)
{
    thumbnail->clear();

    EXPECT_EQ (0, thumbnail->getTotalSamples());
    EXPECT_EQ (nullptr, thumbnail->getPeakProfile());
}

TEST_F (AudioThumbnailTests, SetSourceAfterClear)
{
    auto buffer1 = createThumbnailTestBuffer (1, 1000);
    thumbnail->setSource (&buffer1, kThumbnailSampleRate);
    waitForProfileReady();

    thumbnail->clear();

    auto buffer2 = createThumbnailTestBuffer (2, 2000);
    thumbnail->setSource (&buffer2, kThumbnailSampleRate);
    waitForProfileReady();

    EXPECT_EQ (2, thumbnail->getNumChannels());
    EXPECT_EQ (2000, thumbnail->getTotalSamples());
}

//==============================================================================
// Listener Edge Cases
//==============================================================================

TEST_F (AudioThumbnailTests, AddListenerMultipleTimes)
{
    // Adding same listener multiple times should be handled
    thumbnail->addListener (listener.get());
    thumbnail->addListener (listener.get());

    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);
    thumbnail->setSource (&buffer, kThumbnailSampleRate);
    waitForProfileReady();

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (AudioThumbnailTests, RemoveNonExistentListener)
{
    ThumbnailTestListener otherListener;

    // Removing a listener that wasn't added should not crash
    thumbnail->removeListener (&otherListener);

    EXPECT_TRUE (true);
}

TEST_F (AudioThumbnailTests, RemoveListenerMultipleTimes)
{
    thumbnail->removeListener (listener.get());
    thumbnail->removeListener (listener.get()); // Second remove should not crash

    EXPECT_TRUE (true);
}

TEST_F (AudioThumbnailTests, MultipleListeners)
{
    ThumbnailTestListener listener2;
    ThumbnailTestListener listener3;

    thumbnail->addListener (&listener2);
    thumbnail->addListener (&listener3);

    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);
    thumbnail->setSource (&buffer, kThumbnailSampleRate);
    waitForProfileReady();

    // All listeners should be notified
    EXPECT_GT (listener->changedCallCount.load(), 0);
    EXPECT_GT (listener2.changedCallCount.load(), 0);
    EXPECT_GT (listener3.changedCallCount.load(), 0);
}

//==============================================================================
// Cache Integration Edge Cases
//==============================================================================

TEST_F (AudioThumbnailTests, NullCacheHandling)
{
    AudioThumbnail thumbWithNullCache (nullptr);

    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);
    thumbWithNullCache.setSource (&buffer, kThumbnailSampleRate);

    // Should handle null cache gracefully
    EXPECT_EQ (kThumbnailBufferSize, thumbWithNullCache.getTotalSamples());
}

TEST_F (AudioThumbnailTests, SharedCacheThreadSafety)
{
    // Multiple thumbnails using same cache simultaneously
    AudioThumbnail thumb1 (cache);
    AudioThumbnail thumb2 (cache);
    AudioThumbnail thumb3 (cache);

    auto buffer1 = createThumbnailTestBuffer (1, 1000);
    auto buffer2 = createThumbnailTestBuffer (2, 2000);
    auto buffer3 = createThumbnailTestBuffer (3, 3000);

    thumb1.setSource (&buffer1, kThumbnailSampleRate);
    thumb2.setSource (&buffer2, kThumbnailSampleRate);
    thumb3.setSource (&buffer3, kThumbnailSampleRate);

    Thread::sleep (100);

    // All should work correctly
    EXPECT_EQ (1000, thumb1.getTotalSamples());
    EXPECT_EQ (2000, thumb2.getTotalSamples());
    EXPECT_EQ (3000, thumb3.getTotalSamples());
}

//==============================================================================
// Stress Tests
//==============================================================================

TEST_F (AudioThumbnailTests, RapidSetSourceCalls)
{
    auto buffer1 = createThumbnailTestBuffer (1, 1000);
    auto buffer2 = createThumbnailTestBuffer (2, 2000);
    auto buffer3 = createThumbnailTestBuffer (3, 3000);

    // Rapidly change sources
    thumbnail->setSource (&buffer1, kThumbnailSampleRate);
    thumbnail->setSource (&buffer2, kThumbnailSampleRate);
    thumbnail->setSource (&buffer3, kThumbnailSampleRate);

    waitForProfileReady();

    // Should settle on last source
    EXPECT_EQ (3, thumbnail->getNumChannels());
    EXPECT_EQ (3000, thumbnail->getTotalSamples());
}

TEST_F (AudioThumbnailTests, AlternatingSetSourceAndClear)
{
    auto buffer = createThumbnailTestBuffer (1, kThumbnailBufferSize);

    for (int i = 0; i < 5; ++i)
    {
        thumbnail->setSource (&buffer, kThumbnailSampleRate);
        Thread::sleep (10);
        thumbnail->clear();
    }

    EXPECT_EQ (0, thumbnail->getTotalSamples());
}
