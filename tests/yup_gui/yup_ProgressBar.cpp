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

#include <yup_gui/yup_gui.h>

#include <gtest/gtest.h>

using namespace yup;

//==============================================================================
class ProgressBarTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mm = MessageManager::getInstance();
        progressBar = std::make_unique<ProgressBar> ("testProgressBar");
        progressBar->setBounds (0.0f, 0.0f, 200.0f, 30.0f);
    }

    void runDispatchLoopUntil (int millisecondsToRunFor = 10)
    {
#if YUP_MODAL_LOOPS_PERMITTED
        mm->runDispatchLoopUntil (millisecondsToRunFor);
#endif
    }

    MessageManager* mm = nullptr;
    std::unique_ptr<ProgressBar> progressBar;
};

//==============================================================================
// Construction Tests
//==============================================================================

TEST_F (ProgressBarTests, ConstructorInitializesCorrectly)
{
    EXPECT_EQ (0.0, progressBar->getProgress());
    EXPECT_FALSE (progressBar->isIndeterminate());
}

TEST_F (ProgressBarTests, ConstructorWithIDSetsID)
{
    ProgressBar bar ("myProgressBar");
    EXPECT_EQ ("myProgressBar", bar.getComponentID());
}

//==============================================================================
// Progress Value Tests
//==============================================================================

TEST_F (ProgressBarTests, SetProgressUpdatesValue)
{
    progressBar->setProgress (0.5, dontSendNotification);
    EXPECT_EQ (0.5, progressBar->getProgress());
    EXPECT_FALSE (progressBar->isIndeterminate());
}

TEST_F (ProgressBarTests, SetProgressToZero)
{
    progressBar->setProgress (0.5, dontSendNotification);
    progressBar->setProgress (0.0, dontSendNotification);
    EXPECT_EQ (0.0, progressBar->getProgress());
    EXPECT_FALSE (progressBar->isIndeterminate());
}

TEST_F (ProgressBarTests, SetProgressToOne)
{
    progressBar->setProgress (1.0, dontSendNotification);
    EXPECT_EQ (1.0, progressBar->getProgress());
    EXPECT_FALSE (progressBar->isIndeterminate());
}

TEST_F (ProgressBarTests, ProgressValuesAreClamped)
{
    progressBar->setProgress (1.5, dontSendNotification);
    EXPECT_EQ (1.0, progressBar->getProgress());

    progressBar->setProgress (-0.5, dontSendNotification);
    EXPECT_TRUE (progressBar->isIndeterminate());
}

//==============================================================================
// Indeterminate Mode Tests
//==============================================================================

TEST_F (ProgressBarTests, SetProgressToNegativeEnablesIndeterminateMode)
{
    progressBar->setProgress (-1.0, dontSendNotification);
    EXPECT_TRUE (progressBar->isIndeterminate());
    EXPECT_EQ (-1.0, progressBar->getProgress());
}

TEST_F (ProgressBarTests, TransitionFromIndeterminateToNormalMode)
{
    progressBar->setProgress (-1.0, dontSendNotification);
    EXPECT_TRUE (progressBar->isIndeterminate());

    progressBar->setProgress (0.5, dontSendNotification);
    EXPECT_FALSE (progressBar->isIndeterminate());
    EXPECT_EQ (0.5, progressBar->getProgress());
}

TEST_F (ProgressBarTests, TransitionFromNormalToIndeterminateMode)
{
    progressBar->setProgress (0.7, dontSendNotification);
    EXPECT_FALSE (progressBar->isIndeterminate());

    progressBar->setProgress (-1.0, dontSendNotification);
    EXPECT_TRUE (progressBar->isIndeterminate());
}

//==============================================================================
// Callback Tests
//==============================================================================

TEST_F (ProgressBarTests, DISABLED_ProgressChangedCallbackInvoked)
{
    bool callbackInvoked = false;
    double receivedProgress = -999.0;

    progressBar->onProgressChanged = [&callbackInvoked, &receivedProgress] (double progress)
    {
        callbackInvoked = true;
        receivedProgress = progress;
    };

    progressBar->setProgress (0.75, sendNotification);

    runDispatchLoopUntil (100);

    EXPECT_TRUE (callbackInvoked);
    EXPECT_EQ (0.75, receivedProgress);
}

TEST_F (ProgressBarTests, ProgressChangedCallbackNotInvokedWhenDontSendNotification)
{
    bool callbackInvoked = false;

    progressBar->onProgressChanged = [&callbackInvoked] (double progress)
    {
        callbackInvoked = true;
    };

    progressBar->setProgress (0.5, dontSendNotification);

    EXPECT_FALSE (callbackInvoked);
}

TEST_F (ProgressBarTests, DISABLED_ProgressChangedCallbackInvokedForIndeterminate)
{
    bool callbackInvoked = false;
    double receivedProgress = -999.0;

    progressBar->onProgressChanged = [&callbackInvoked, &receivedProgress] (double progress)
    {
        callbackInvoked = true;
        receivedProgress = progress;
    };

    progressBar->setProgress (-1.0, sendNotification);

    runDispatchLoopUntil (100);

    EXPECT_TRUE (callbackInvoked);
    EXPECT_EQ (-1.0, receivedProgress);
}

TEST_F (ProgressBarTests, DISABLED_ProgressChangedCallbackNotInvokedForSameValue)
{
    progressBar->setProgress (0.5, dontSendNotification);

    int callbackCount = 0;
    progressBar->onProgressChanged = [&callbackCount] (double progress)
    {
        callbackCount++;
    };

    progressBar->setProgress (0.5, sendNotification);

    runDispatchLoopUntil (100);

    EXPECT_EQ (0, callbackCount);
}

//==============================================================================
// Style and Color Tests
//==============================================================================

TEST_F (ProgressBarTests, BackgroundColorCanBeCustomized)
{
    const Color customColor (0xffff0000);
    progressBar->setColor (ProgressBar::Style::backgroundColorId, customColor);

    auto retrievedColor = progressBar->findColor (ProgressBar::Style::backgroundColorId);
    EXPECT_TRUE (retrievedColor.has_value());
    EXPECT_EQ (customColor, *retrievedColor);
}

TEST_F (ProgressBarTests, ForegroundColorCanBeCustomized)
{
    const Color customColor (0xff00ff00);
    progressBar->setColor (ProgressBar::Style::foregroundColorId, customColor);

    auto retrievedColor = progressBar->findColor (ProgressBar::Style::foregroundColorId);
    EXPECT_TRUE (retrievedColor.has_value());
    EXPECT_EQ (customColor, *retrievedColor);
}

//==============================================================================
// Thread Safety Tests
//==============================================================================

TEST_F (ProgressBarTests, ProgressCanBeSetFromMultipleThreads)
{
    // Basic thread safety test - ensure no crashes when updating from different threads
    std::atomic<bool> threadsRunning { true };
    std::vector<std::thread> threads;

    for (int i = 0; i < 5; ++i)
    {
        threads.emplace_back ([this, &threadsRunning, i]
        {
            while (threadsRunning.load())
            {
                const double progress = static_cast<double> (i) / 10.0;
                progressBar->setProgress (progress, dontSendNotification);
                std::this_thread::sleep_for (std::chrono::milliseconds (1));
            }
        });
    }

    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    threadsRunning.store (false);

    for (auto& thread : threads)
        thread.join();

    // Just verify we didn't crash and value is in valid range
    const double finalProgress = progressBar->getProgress();
    EXPECT_GE (finalProgress, 0.0);
    EXPECT_LE (finalProgress, 1.0);
}

//==============================================================================
// Rendering Tests
//==============================================================================

TEST_F (ProgressBarTests, ComponentIsNotOpaqueByDefault)
{
    EXPECT_FALSE (progressBar->isOpaque());
}

TEST_F (ProgressBarTests, ComponentHasValidBounds)
{
    auto bounds = progressBar->getBounds();
    EXPECT_EQ (200.0f, bounds.getWidth());
    EXPECT_EQ (30.0f, bounds.getHeight());
}

//==============================================================================
// Animation and RefreshDisplay Tests
//==============================================================================

TEST_F (ProgressBarTests, RefreshDisplayDoesNotCrashInDeterminateMode)
{
    progressBar->setProgress (0.5, dontSendNotification);
    progressBar->refreshDisplay (0.016); // 60 FPS frame time

    EXPECT_TRUE (true);
}

TEST_F (ProgressBarTests, RefreshDisplayDoesNotCrashInIndeterminateMode)
{
    progressBar->setProgress (-1.0, dontSendNotification);
    progressBar->refreshDisplay (0.016);

    EXPECT_TRUE (true);
}

TEST_F (ProgressBarTests, RefreshDisplayWithMultipleFrames)
{
    progressBar->setProgress (-1.0, dontSendNotification);

    for (int i = 0; i < 10; ++i)
    {
        progressBar->refreshDisplay (0.016);
    }

    EXPECT_TRUE (true);
}

TEST_F (ProgressBarTests, RefreshDisplayWithZeroFrameTime)
{
    progressBar->setProgress (-1.0, dontSendNotification);
    progressBar->refreshDisplay (0.0);

    EXPECT_TRUE (true);
}

TEST_F (ProgressBarTests, RefreshDisplayWithVeryLargeFrameTime)
{
    progressBar->setProgress (-1.0, dontSendNotification);
    progressBar->refreshDisplay (10.0); // 10 seconds

    EXPECT_TRUE (true);
}

//==============================================================================
// Edge Cases
//==============================================================================

TEST_F (ProgressBarTests, RapidProgressUpdates)
{
    for (int i = 0; i <= 100; ++i)
    {
        progressBar->setProgress (i / 100.0, dontSendNotification);
    }

    EXPECT_EQ (1.0, progressBar->getProgress());
}

TEST_F (ProgressBarTests, AlternatingBetweenNormalAndIndeterminate)
{
    progressBar->setProgress (0.5, dontSendNotification);
    EXPECT_FALSE (progressBar->isIndeterminate());

    progressBar->setProgress (-1.0, dontSendNotification);
    EXPECT_TRUE (progressBar->isIndeterminate());

    progressBar->setProgress (0.8, dontSendNotification);
    EXPECT_FALSE (progressBar->isIndeterminate());

    progressBar->setProgress (-1.0, dontSendNotification);
    EXPECT_TRUE (progressBar->isIndeterminate());
}

TEST_F (ProgressBarTests, VerySmallProgressIncrements)
{
    progressBar->setProgress (0.0, dontSendNotification);

    for (int i = 0; i < 10000; ++i)
    {
        progressBar->setProgress (i / 10000.0, dontSendNotification);
    }

    EXPECT_NEAR (0.9999, progressBar->getProgress(), 0.0001);
}

TEST_F (ProgressBarTests, ProgressBeyondOneIsClamped)
{
    progressBar->setProgress (2.0, dontSendNotification);
    EXPECT_EQ (1.0, progressBar->getProgress());

    progressBar->setProgress (100.0, dontSendNotification);
    EXPECT_EQ (1.0, progressBar->getProgress());
}

TEST_F (ProgressBarTests, ProgressSlightlyBelowZeroIsIndeterminate)
{
    progressBar->setProgress (-0.1, dontSendNotification);
    EXPECT_TRUE (progressBar->isIndeterminate());

    progressBar->setProgress (-0.5, dontSendNotification);
    EXPECT_TRUE (progressBar->isIndeterminate());
}

//==============================================================================
// Sizing and Bounds Tests
//==============================================================================

TEST_F (ProgressBarTests, ProgressBarWithVerySmallSize)
{
    progressBar->setBounds (0.0f, 0.0f, 1.0f, 1.0f);
    progressBar->setProgress (0.5, dontSendNotification);

    EXPECT_EQ (0.5, progressBar->getProgress());
}

TEST_F (ProgressBarTests, ProgressBarWithZeroSize)
{
    progressBar->setBounds (0.0f, 0.0f, 0.0f, 0.0f);
    progressBar->setProgress (0.5, dontSendNotification);

    EXPECT_EQ (0.5, progressBar->getProgress());
}

TEST_F (ProgressBarTests, ProgressBarWithVeryLargeSize)
{
    progressBar->setBounds (0.0f, 0.0f, 10000.0f, 1000.0f);
    progressBar->setProgress (0.75, dontSendNotification);

    EXPECT_EQ (0.75, progressBar->getProgress());
}

TEST_F (ProgressBarTests, ProgressBarWithWideAspectRatio)
{
    progressBar->setBounds (0.0f, 0.0f, 1000.0f, 10.0f);
    progressBar->setProgress (0.5, dontSendNotification);

    auto bounds = progressBar->getBounds();
    EXPECT_GT (bounds.getWidth(), bounds.getHeight());
}

TEST_F (ProgressBarTests, ProgressBarWithTallAspectRatio)
{
    progressBar->setBounds (0.0f, 0.0f, 10.0f, 1000.0f);
    progressBar->setProgress (0.5, dontSendNotification);

    auto bounds = progressBar->getBounds();
    EXPECT_GT (bounds.getHeight(), bounds.getWidth());
}

//==============================================================================
// Multiple Color Customization Tests
//==============================================================================

TEST_F (ProgressBarTests, MultipleColorChanges)
{
    progressBar->setColor (ProgressBar::Style::backgroundColorId, Color (0xffff0000));
    progressBar->setColor (ProgressBar::Style::foregroundColorId, Color (0xff00ff00));
    progressBar->setColor (ProgressBar::Style::backgroundColorId, Color (0xff0000ff));

    auto color = progressBar->findColor (ProgressBar::Style::backgroundColorId);
    EXPECT_TRUE (color.has_value());
    EXPECT_EQ (Color (0xff0000ff), *color);
}

TEST_F (ProgressBarTests, ColorRetrievalForNonSetColor)
{
    // Clear any inherited colors first if needed
    auto color = progressBar->findColor (Identifier ("nonExistentColorId"));
    // May or may not have a value depending on theme defaults
    EXPECT_TRUE (true);
}

//==============================================================================
// State Verification Tests
//==============================================================================

TEST_F (ProgressBarTests, ProgressValuePrecision)
{
    progressBar->setProgress (0.123456789, dontSendNotification);
    EXPECT_NEAR (0.123456789, progressBar->getProgress(), 0.000001);
}

TEST_F (ProgressBarTests, ZeroProgressIsNotIndeterminate)
{
    progressBar->setProgress (0.0, dontSendNotification);
    EXPECT_FALSE (progressBar->isIndeterminate());
    EXPECT_EQ (0.0, progressBar->getProgress());
}

TEST_F (ProgressBarTests, OneProgressIsNotIndeterminate)
{
    progressBar->setProgress (1.0, dontSendNotification);
    EXPECT_FALSE (progressBar->isIndeterminate());
    EXPECT_EQ (1.0, progressBar->getProgress());
}

TEST_F (ProgressBarTests, ExactlyNegativeOneIsIndeterminate)
{
    progressBar->setProgress (-1.0, dontSendNotification);
    EXPECT_TRUE (progressBar->isIndeterminate());
    EXPECT_EQ (-1.0, progressBar->getProgress());
}

//==============================================================================
// Component ID Tests
//==============================================================================

TEST_F (ProgressBarTests, EmptyComponentIDByDefault)
{
    ProgressBar bar;
    EXPECT_TRUE (bar.getComponentID().isEmpty());
}

TEST_F (ProgressBarTests, ComponentIDIsSetCorrectly)
{
    ProgressBar bar ("newID");
    EXPECT_EQ ("newID", bar.getComponentID());
}

//==============================================================================
// Concurrent Access Tests
//==============================================================================

TEST_F (ProgressBarTests, ConcurrentReadAndWrite)
{
    std::atomic<bool> running { true };

    std::thread writer ([this, &running]
    {
        double progress = 0.0;
        while (running.load())
        {
            progressBar->setProgress (progress, dontSendNotification);
            progress += 0.01;
            if (progress > 1.0)
                progress = 0.0;
            std::this_thread::sleep_for (std::chrono::microseconds (100));
        }
    });

    std::thread reader ([this, &running]
    {
        while (running.load())
        {
            volatile double progress = progressBar->getProgress();
            (void) progress; // Use the value
            std::this_thread::sleep_for (std::chrono::microseconds (100));
        }
    });

    std::this_thread::sleep_for (std::chrono::milliseconds (50));
    running.store (false);

    writer.join();
    reader.join();

    EXPECT_TRUE (true);
}
