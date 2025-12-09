/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

#include <yup_audio_basics/yup_audio_basics.h>

using namespace yup;

//==============================================================================
class AudioProcessLoadMeasurerTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        measurer = std::make_unique<AudioProcessLoadMeasurer>();
    }

    void TearDown() override
    {
        measurer.reset();
    }

    std::unique_ptr<AudioProcessLoadMeasurer> measurer;
};

//==============================================================================
TEST_F (AudioProcessLoadMeasurerTests, Constructor)
{
    EXPECT_NO_THROW (AudioProcessLoadMeasurer());
}

TEST_F (AudioProcessLoadMeasurerTests, Destructor)
{
    auto* temp = new AudioProcessLoadMeasurer();
    EXPECT_NO_THROW (delete temp);
}

//==============================================================================
TEST_F (AudioProcessLoadMeasurerTests, InitialState)
{
    EXPECT_DOUBLE_EQ (measurer->getLoadAsProportion(), 0.0);
    EXPECT_DOUBLE_EQ (measurer->getLoadAsPercentage(), 0.0);
    EXPECT_EQ (measurer->getXRunCount(), 0);
}

//==============================================================================
TEST_F (AudioProcessLoadMeasurerTests, ResetWithoutParameters)
{
    measurer->reset (44100.0, 512);
    measurer->registerBlockRenderTime (5.0);

    measurer->reset();

    EXPECT_DOUBLE_EQ (measurer->getLoadAsProportion(), 0.0);
    EXPECT_EQ (measurer->getXRunCount(), 0);
}

TEST_F (AudioProcessLoadMeasurerTests, ResetWithParameters)
{
    measurer->reset (44100.0, 512);
    measurer->registerBlockRenderTime (5.0);

    measurer->reset (48000.0, 1024);

    EXPECT_DOUBLE_EQ (measurer->getLoadAsProportion(), 0.0);
    EXPECT_EQ (measurer->getXRunCount(), 0);
}

TEST_F (AudioProcessLoadMeasurerTests, ResetWithZeroParameters)
{
    // Should handle zero parameters (line 59)
    EXPECT_NO_THROW (measurer->reset (0.0, 0));
}

//==============================================================================
TEST_F (AudioProcessLoadMeasurerTests, RegisterBlockRenderTime)
{
    measurer->reset (44100.0, 512);

    // Block time should be: 512 / 44100 = 0.0116 seconds = 11.6 ms
    // Register a time less than the available time
    measurer->registerBlockRenderTime (5.0);

    auto load = measurer->getLoadAsProportion();
    EXPECT_GT (load, 0.0);
    EXPECT_LT (load, 1.0);
}

TEST_F (AudioProcessLoadMeasurerTests, RegisterBlockRenderTimeExceedsAvailable)
{
    measurer->reset (44100.0, 512);

    // Register a time that exceeds available time (should increment xruns, line 89-90)
    measurer->registerBlockRenderTime (20.0);

    EXPECT_GT (measurer->getXRunCount(), 0);
}

TEST_F (AudioProcessLoadMeasurerTests, RegisterBlockRenderTimeMultiple)
{
    measurer->reset (44100.0, 512);

    // Register multiple times to test filtering (line 86-87)
    for (int i = 0; i < 10; ++i)
    {
        measurer->registerBlockRenderTime (5.0);
    }

    auto load = measurer->getLoadAsProportion();
    EXPECT_GT (load, 0.0);
    EXPECT_LT (load, 1.0);
}

TEST_F (AudioProcessLoadMeasurerTests, RegisterBlockRenderTimeWithoutReset)
{
    // Should handle registerBlockRenderTime without reset (msPerSample == 0, line 80-81)
    EXPECT_NO_THROW (measurer->registerBlockRenderTime (5.0));

    // Load should remain 0
    EXPECT_DOUBLE_EQ (measurer->getLoadAsProportion(), 0.0);
}

//==============================================================================
TEST_F (AudioProcessLoadMeasurerTests, RegisterRenderTime)
{
    measurer->reset (44100.0, 512);

    // Register with specific number of samples
    measurer->registerRenderTime (2.0, 256);

    auto load = measurer->getLoadAsProportion();
    EXPECT_GT (load, 0.0);
    EXPECT_LT (load, 1.0);
}

TEST_F (AudioProcessLoadMeasurerTests, RegisterRenderTimeExceedsAvailable)
{
    measurer->reset (44100.0, 512);

    // Time per sample: 1000 / 44100 = 0.0227 ms
    // For 256 samples: 256 * 0.0227 = 5.8 ms
    // Register 10ms which exceeds available time
    measurer->registerRenderTime (10.0, 256);

    EXPECT_GT (measurer->getXRunCount(), 0);
}

TEST_F (AudioProcessLoadMeasurerTests, RegisterRenderTimeWithoutReset)
{
    // Should handle registerRenderTime without reset
    EXPECT_NO_THROW (measurer->registerRenderTime (5.0, 512));

    // Load should remain 0
    EXPECT_DOUBLE_EQ (measurer->getLoadAsProportion(), 0.0);
}

//==============================================================================
TEST_F (AudioProcessLoadMeasurerTests, GetLoadAsProportion)
{
    measurer->reset (44100.0, 512);
    measurer->registerBlockRenderTime (5.0);

    auto proportion = measurer->getLoadAsProportion();

    // Should be clamped between 0 and 1 (line 93)
    EXPECT_GE (proportion, 0.0);
    EXPECT_LE (proportion, 1.0);
}

TEST_F (AudioProcessLoadMeasurerTests, GetLoadAsPercentage)
{
    measurer->reset (44100.0, 512);
    measurer->registerBlockRenderTime (5.0);

    auto percentage = measurer->getLoadAsPercentage();

    // Should be proportion * 100 (line 95)
    EXPECT_DOUBLE_EQ (percentage, measurer->getLoadAsProportion() * 100.0);
    EXPECT_GE (percentage, 0.0);
    EXPECT_LE (percentage, 100.0);
}

TEST_F (AudioProcessLoadMeasurerTests, GetLoadProportionClampingHigh)
{
    measurer->reset (44100.0, 512);

    // Register many high times to push proportion above 1.0
    for (int i = 0; i < 50; ++i)
    {
        measurer->registerBlockRenderTime (20.0);
    }

    // Should be clamped to 1.0
    EXPECT_DOUBLE_EQ (measurer->getLoadAsProportion(), 1.0);
}

//==============================================================================
TEST_F (AudioProcessLoadMeasurerTests, GetXRunCount)
{
    measurer->reset (44100.0, 512);

    EXPECT_EQ (measurer->getXRunCount(), 0);

    // Cause an xrun
    measurer->registerBlockRenderTime (20.0);

    EXPECT_EQ (measurer->getXRunCount(), 1);

    // Cause more xruns
    measurer->registerBlockRenderTime (20.0);
    measurer->registerBlockRenderTime (20.0);

    EXPECT_EQ (measurer->getXRunCount(), 3);
}

//==============================================================================
TEST_F (AudioProcessLoadMeasurerTests, ScopedTimerConstructor)
{
    measurer->reset (44100.0, 512);

    EXPECT_NO_THROW (AudioProcessLoadMeasurer::ScopedTimer (*measurer));
}

TEST_F (AudioProcessLoadMeasurerTests, ScopedTimerWithSamples)
{
    measurer->reset (44100.0, 512);

    EXPECT_NO_THROW (AudioProcessLoadMeasurer::ScopedTimer (*measurer, 256));
}

TEST_F (AudioProcessLoadMeasurerTests, ScopedTimerMeasures)
{
    measurer->reset (44100.0, 512);

    {
        AudioProcessLoadMeasurer::ScopedTimer timer (*measurer);
        Thread::sleep (5);
    }

    // Should have registered some load
    EXPECT_GT (measurer->getLoadAsProportion(), 0.0);
}

TEST_F (AudioProcessLoadMeasurerTests, ScopedTimerUsesDefaultSamples)
{
    measurer->reset (44100.0, 512);

    {
        // Uses default samples from measurer (line 100)
        AudioProcessLoadMeasurer::ScopedTimer timer (*measurer);
        Thread::sleep (2);
    }

    EXPECT_GT (measurer->getLoadAsProportion(), 0.0);
}

TEST_F (AudioProcessLoadMeasurerTests, ScopedTimerWithCustomSamples)
{
    measurer->reset (44100.0, 512);

    {
        AudioProcessLoadMeasurer::ScopedTimer timer (*measurer, 256);
        Thread::sleep (2);
    }

    EXPECT_GT (measurer->getLoadAsProportion(), 0.0);
}

TEST_F (AudioProcessLoadMeasurerTests, ScopedTimerDestructorRegisters)
{
    measurer->reset (44100.0, 512);

    EXPECT_DOUBLE_EQ (measurer->getLoadAsProportion(), 0.0);

    {
        AudioProcessLoadMeasurer::ScopedTimer timer (*measurer);
        Thread::sleep (3);
        // Destructor should call registerRenderTime (line 116)
    }

    // After destructor, load should be > 0
    EXPECT_GT (measurer->getLoadAsProportion(), 0.0);
}

//==============================================================================
TEST_F (AudioProcessLoadMeasurerTests, FilteringBehavior)
{
    measurer->reset (44100.0, 512);

    // Register low load
    for (int i = 0; i < 10; ++i)
    {
        measurer->registerBlockRenderTime (2.0);
    }

    auto lowLoad = measurer->getLoadAsProportion();

    // Register high load
    for (int i = 0; i < 10; ++i)
    {
        measurer->registerBlockRenderTime (8.0);
    }

    auto highLoad = measurer->getLoadAsProportion();

    // High load should be greater due to filtering (line 85-87)
    EXPECT_GT (highLoad, lowLoad);
}

TEST_F (AudioProcessLoadMeasurerTests, RealisticScenario)
{
    measurer->reset (48000.0, 480);

    // Simulate 100 audio blocks
    Random random;
    for (int i = 0; i < 100; ++i)
    {
        // Random processing time between 1-8 ms
        double processingTime = 1.0 + random.nextDouble() * 7.0;
        measurer->registerBlockRenderTime (processingTime);
    }

    auto load = measurer->getLoadAsProportion();
    EXPECT_GE (load, 0.0);
    EXPECT_LE (load, 1.0);

    auto percentage = measurer->getLoadAsPercentage();
    EXPECT_GE (percentage, 0.0);
    EXPECT_LE (percentage, 100.0);
}

TEST_F (AudioProcessLoadMeasurerTests, ThreadSafety)
{
    measurer->reset (44100.0, 512);

    // Test that concurrent access doesn't crash (uses SpinLock::ScopedTryLockType)
    std::atomic<bool> done { false };

    auto writerThread = [this, &done]()
    {
        while (! done)
        {
            measurer->registerBlockRenderTime (5.0);
            Thread::sleep (1);
        }
    };

    auto readerThread = [this, &done]()
    {
        while (! done)
        {
            volatile auto load = measurer->getLoadAsProportion();
            volatile auto xruns = measurer->getXRunCount();
            (void) load;
            (void) xruns;
            Thread::sleep (1);
        }
    };

    std::thread t1 (writerThread);
    std::thread t2 (readerThread);

    Thread::sleep (50);
    done = true;

    t1.join();
    t2.join();

    // Should not crash
    EXPECT_GE (measurer->getLoadAsProportion(), 0.0);
}
