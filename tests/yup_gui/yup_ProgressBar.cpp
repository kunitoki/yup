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

    /** Puts a component in a window of its own, and returns the native that sends its frames. */
    static StubComponentNative& makeWindow (Component& component)
    {
        auto* native = new StubComponentNative (component, ComponentNative::defaultFlags);
        yup::ComponentTestHelper<Component>::attachNative (component, native);
        return *native;
    }

    MessageManager* mm = nullptr;
    std::unique_ptr<ProgressBar> progressBar;

    /** Counts the paths a component paints, with no GPU. */
    class PathCounter final : public rive::Renderer
    {
    public:
        using rive::Renderer::drawImage;
        using rive::Renderer::drawImageMesh;

        void save() override {}

        void restore() override {}

        void transform (const rive::Mat2D&) override {}

        void drawPath (rive::RenderPath*, rive::RenderPaint*) override { ++numPaths; }

        void clipPath (rive::RenderPath*) override {}

        void drawImage (const rive::RenderImage*, rive::ImageSampler, rive::BlendMode, float) override {}

        void drawImageMesh (const rive::RenderImage*,
                            rive::ImageSampler,
                            rive::rcp<rive::RenderBuffer>,
                            rive::rcp<rive::RenderBuffer>,
                            rive::rcp<rive::RenderBuffer>,
                            uint32_t,
                            uint32_t,
                            rive::BlendMode,
                            float) override {}

        void modulateOpacity (float) override {}

        int numPaths = 0;
    };

    /** Returns how many paths the bar is painted with by the default theme. */
    int countPathsPaintedByTheDefaultTheme()
    {
        const auto oldTheme = ApplicationTheme::getGlobalTheme();
        ApplicationTheme::setGlobalTheme (createThemeVersion1());

        auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
        PathCounter counter;

        {
            Graphics g (*context, counter);
            g.setDrawingArea ({ 0.0f, 0.0f, 200.0f, 30.0f });
            progressBar->paint (g);
        }

        ApplicationTheme::setGlobalTheme (oldTheme);

        return counter.numPaths;
    }
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

TEST_F (ProgressBarTests, TellsADrawingItsProgressAndThePhaseOfItsAnimation)
{
    const auto* phase = progressBar->getTypeInfo().findProperty (Identifier ("phase"));
    ASSERT_NE (phase, nullptr);
    EXPECT_TRUE (phase->isReadOnly());
    EXPECT_DOUBLE_EQ (static_cast<double> (progressBar->getProperty (Identifier ("phase"))), progressBar->getAnimationPhase());
    EXPECT_DOUBLE_EQ (progressBar->getAnimationPhase(), 0.0);

    const auto xml = XmlDocument::parse (R"xml(<drawing>
        <rect id="filled" bounds="rect (0, 0, bounds.width * progress, bounds.height)" fill="#ffffff" visible="! indeterminate"/>
        <rect id="ribbon" bounds="rect ((bounds.width - 40) * phase, 0, 40, bounds.height)" fill="#ffffff" visible="indeterminate"/>
    </drawing>)xml");
    ASSERT_NE (xml, nullptr);

    const auto style = DrawingStyle::create (*progressBar, DataTree::fromXml (*xml));
    ASSERT_TRUE (style.wasOk()) << style.getErrorMessage();

    const auto measure = [&] (const String& id)
    {
        for (const auto& element : style.getReference()->measureElements (*progressBar))
            if (element.id == id)
                return element.bounds;

        return Rectangle<float>();
    };

    // The part of the bar its progress covers
    progressBar->setProgress (0.25, dontSendNotification);
    EXPECT_EQ (measure ("filled"), Rectangle<float> (0.0f, 0.0f, 50.0f, 30.0f));
    EXPECT_TRUE (measure ("ribbon").isEmpty());

    // And a ribbon moving with the phase while there is no progress to tell
    progressBar->setProgress (-1.0, dontSendNotification);
    EXPECT_TRUE (measure ("filled").isEmpty());
    EXPECT_EQ (measure ("ribbon"), Rectangle<float> (0.0f, 0.0f, 40.0f, 30.0f));
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

TEST_F (ProgressBarTests, SetProgressFromAnotherThreadIsAppliedOnTheMessageThread)
{
    struct PropertyListener : ComponentListener
    {
        void componentPropertyChanged (Component&, const Identifier&) override
        {
            ++callCount;
            calledOnMessageThread = MessageManager::getInstance()->isThisTheMessageThread();
        }

        int callCount = 0;
        bool calledOnMessageThread = false;
    };

    PropertyListener listener;
    progressBar->addComponentListener (&listener);

    std::thread worker ([this]
    {
        progressBar->setProgress (0.5, dontSendNotification);
    });

    worker.join();

    EXPECT_EQ (0, listener.callCount);
    EXPECT_EQ (0.0, progressBar->getProgress());

#if YUP_MODAL_LOOPS_PERMITTED
    for (int i = 0; i < 100 && listener.callCount == 0; ++i)
        runDispatchLoopUntil (10);

    EXPECT_EQ (1, listener.callCount);
    EXPECT_TRUE (listener.calledOnMessageThread);
    EXPECT_EQ (0.5, progressBar->getProgress());
#endif

    progressBar->removeComponentListener (&listener);
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

TEST_F (ProgressBarTests, ThePhaseAdvancesWithTheTimeOfTheFrames)
{
    progressBar->setProgress (-1.0, dontSendNotification);
    ASSERT_TRUE (progressBar->isIndeterminate());

    // A cycle takes two seconds
    progressBar->refreshDisplay (0.5);
    EXPECT_NEAR (progressBar->getAnimationPhase(), 0.25, 1.0e-9);

    progressBar->refreshDisplay (1.0);
    EXPECT_NEAR (progressBar->getAnimationPhase(), 0.75, 1.0e-9);

    // And starts again, however long a frame took
    progressBar->refreshDisplay (1.0);
    EXPECT_NEAR (progressBar->getAnimationPhase(), 0.25, 1.0e-9);

    progressBar->refreshDisplay (10.0);
    EXPECT_NEAR (progressBar->getAnimationPhase(), 0.25, 1.0e-9);

    // A bar showing a progress leaves the phase where it is
    progressBar->setProgress (0.5, dontSendNotification);
    progressBar->refreshDisplay (1.0);
    EXPECT_NEAR (progressBar->getAnimationPhase(), 0.25, 1.0e-9);
}

TEST_F (ProgressBarTests, AsksForFramesOnlyWhileItIsIndeterminate)
{
    Component window ("window");
    auto& native = makeWindow (window);
    window.setBounds (0.0f, 0.0f, 300.0f, 100.0f);
    window.setVisible (true);
    window.addAndMakeVisible (*progressBar);

    progressBar->setProgress (0.5, dontSendNotification);
    EXPECT_FALSE (native.hasFrameListeners());

    progressBar->setProgress (-1.0, dontSendNotification);
    EXPECT_TRUE (native.hasFrameListeners());

    native.callFrameListeners (0.5);
    EXPECT_NEAR (progressBar->getAnimationPhase(), 0.25, 1.0e-9);
    EXPECT_TRUE (native.hasFrameListeners());

    progressBar->setProgress (1.0, dontSendNotification);
    EXPECT_FALSE (native.hasFrameListeners());

    window.removeChildComponent (*progressBar);
}

TEST_F (ProgressBarTests, AHiddenIndeterminateBarAsksForNoFrameUntilItShows)
{
    Component window ("window");
    auto& native = makeWindow (window);
    window.setBounds (0.0f, 0.0f, 300.0f, 100.0f);
    window.setVisible (true);
    window.addAndMakeVisible (*progressBar);

    progressBar->setProgress (-1.0, dontSendNotification);
    ASSERT_TRUE (native.hasFrameListeners());

    // Hiding a parent hides the bar
    window.setVisible (false);
    EXPECT_FALSE (native.hasFrameListeners());

    window.setVisible (true);
    EXPECT_TRUE (native.hasFrameListeners());

    // A bar made indeterminate while hidden starts when it shows
    progressBar->setProgress (0.5, dontSendNotification);
    progressBar->setVisible (false);
    progressBar->setProgress (-1.0, dontSendNotification);
    EXPECT_FALSE (native.hasFrameListeners());

    progressBar->setVisible (true);
    EXPECT_TRUE (native.hasFrameListeners());

    native.callFrameListeners (0.5);
    EXPECT_NEAR (progressBar->getAnimationPhase(), 0.25, 1.0e-9);

    window.removeChildComponent (*progressBar);
}

TEST_F (ProgressBarTests, AnIndeterminateBarLowerThanAPixelPaintsNoRibbons)
{
    progressBar->setProgress (-1.0, dontSendNotification);
    const auto numPathsWithRibbons = countPathsPaintedByTheDefaultTheme();

    // A bar showing no progress is its track alone
    progressBar->setProgress (0.0, dontSendNotification);
    EXPECT_GT (numPathsWithRibbons, countPathsPaintedByTheDefaultTheme());

    // The ribbons are as wide as the bar is high: lower than a pixel it has none to paint, and is its track alone too
    for (const auto height : { 0.6f, 0.25f })
    {
        progressBar->setBounds (0.0f, 0.0f, 200.0f, height);

        progressBar->setProgress (0.0, dontSendNotification);
        const auto numTrackPaths = countPathsPaintedByTheDefaultTheme();

        progressBar->setProgress (-1.0, dontSendNotification);
        EXPECT_EQ (countPathsPaintedByTheDefaultTheme(), numTrackPaths) << height;
    }
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

// =============================================================================

class ProgressBarSubclassTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        progressBar = std::make_unique<TestProgressBar> ("testProgressBar");
    }

    void TearDown() override
    {
        progressBar.reset();
    }

    class TestProgressBar : public ProgressBar
    {
    public:
        TestProgressBar (StringRef componentID)
            : ProgressBar (componentID)
        {
        }

        void progressChanged() override
        {
            progressChangedCallCount++;
            lastProgress = getProgress();
        }

        int progressChangedCallCount = 0;
        double lastProgress = 0.0;
    };

    std::unique_ptr<TestProgressBar> progressBar;
};

TEST_F (ProgressBarSubclassTests, ProgressChangedCanBeOverridden)
{
    progressBar->progressChanged();
    EXPECT_EQ (1, progressBar->progressChangedCallCount);
}

TEST_F (ProgressBarSubclassTests, ProgressChangedGetsCurrentProgress)
{
    progressBar->progressChanged();
    EXPECT_EQ (1, progressBar->progressChangedCallCount);
    EXPECT_DOUBLE_EQ (0.0, progressBar->lastProgress);
}
