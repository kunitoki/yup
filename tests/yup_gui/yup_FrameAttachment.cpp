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

#include <atomic>
#include <memory>
#include <optional>

using namespace yup;

// ==============================================================================
// FrameAttachment - following a component in and out of windows. A stub native
// stands in for each window, so the only frames are the ones a test sends.
// ==============================================================================

class FrameAttachmentTests : public ::testing::Test
{
protected:
    /** Puts a component in a window of its own, and returns the native that sends its frames. */
    static StubComponentNative& makeWindow (Component& component)
    {
        auto* native = new StubComponentNative (component, ComponentNative::defaultFlags);
        yup::ComponentTestHelper<Component>::attachNative (component, native);
        return *native;
    }

    FrameAttachment::Callback countFrames()
    {
        return [this] (double elapsedSeconds)
        {
            ++frameCount;
            lastElapsedSeconds = elapsedSeconds;
        };
    }

    Component window { "window" };
    Component child { "child" };
    int frameCount = 0;
    double lastElapsedSeconds = 0.0;
};

TEST_F (FrameAttachmentTests, CallsBackForEveryFrameOfTheWindow)
{
    auto& native = makeWindow (window);
    window.addChildComponent (child);

    FrameAttachment attachment (child, countFrames());

    native.callFrameListeners (0.25);
    native.callFrameListeners (0.5);

    EXPECT_EQ (2, frameCount);
    EXPECT_DOUBLE_EQ (0.5, lastElapsedSeconds);
}

TEST_F (FrameAttachmentTests, StartsWhenAnAncestorEntersAWindow)
{
    auto& native = makeWindow (window);
    Component parent ("parent");
    parent.addChildComponent (child);

    FrameAttachment attachment (child, countFrames());
    EXPECT_FALSE (native.hasFrameListeners());

    window.addChildComponent (parent);

    native.callFrameListeners (0.25);

    EXPECT_EQ (1, frameCount);
}

TEST_F (FrameAttachmentTests, StopsWhenTheComponentLeavesTheWindow)
{
    auto& native = makeWindow (window);
    window.addChildComponent (child);

    FrameAttachment attachment (child, countFrames());

    window.removeChildComponent (child);

    native.callFrameListeners (0.25);

    EXPECT_EQ (0, frameCount);
    EXPECT_FALSE (native.hasFrameListeners());
}

TEST_F (FrameAttachmentTests, FollowsTheComponentToAnotherWindow)
{
    Component otherWindow ("otherWindow");
    auto& native = makeWindow (window);
    auto& otherNative = makeWindow (otherWindow);
    window.addChildComponent (child);

    FrameAttachment attachment (child, countFrames());

    otherWindow.addChildComponent (child);

    native.callFrameListeners (0.25);
    EXPECT_EQ (0, frameCount);

    otherNative.callFrameListeners (0.25);
    EXPECT_EQ (1, frameCount);
}

TEST_F (FrameAttachmentTests, StopsWhenDestroyed)
{
    auto& native = makeWindow (window);
    window.addChildComponent (child);

    {
        FrameAttachment attachment (child, countFrames());
        EXPECT_TRUE (native.hasFrameListeners());
    }

    native.callFrameListeners (0.25);

    EXPECT_EQ (0, frameCount);
    EXPECT_FALSE (native.hasFrameListeners());
}

TEST_F (FrameAttachmentTests, StopsWhenTheComponentIsDeleted)
{
    auto& native = makeWindow (window);
    auto shortLived = std::make_unique<Component> ("shortLived");
    window.addChildComponent (*shortLived);

    FrameAttachment attachment (*shortLived, countFrames());

    shortLived.reset();

    native.callFrameListeners (0.25);

    EXPECT_EQ (0, frameCount);
    EXPECT_FALSE (native.hasFrameListeners());
}

TEST_F (FrameAttachmentTests, OutlivesTheWindowItWasRegisteredWith)
{
    auto shortLivedWindow = std::make_unique<Component> ("shortLivedWindow");
    makeWindow (*shortLivedWindow);
    shortLivedWindow->addChildComponent (child);

    FrameAttachment attachment (child, countFrames());

    // A deleted parent does not detach its children: the native goes away still holding the attachment
    shortLivedWindow.reset();

    auto& native = makeWindow (window);
    window.addChildComponent (child);

    native.callFrameListeners (0.25);

    EXPECT_EQ (1, frameCount);
}

TEST_F (FrameAttachmentTests, CallbackCanDestroyItsOwnAttachment)
{
    auto& native = makeWindow (window);
    window.addChildComponent (child);

    std::optional<FrameAttachment> oneShot;
    oneShot.emplace (child, [&] (double)
    {
        ++frameCount;
        oneShot.reset();
    });

    int laterFrameCount = 0;
    FrameAttachment later (child, [&] (double)
    {
        ++laterFrameCount;
    });

    native.callFrameListeners (0.25);
    native.callFrameListeners (0.25);

    EXPECT_EQ (1, frameCount);
    EXPECT_EQ (2, laterFrameCount);
}

// ==============================================================================
// FrameAttachment - in a real window, where the frames come from the render
// thread while the test runs the message thread.
// ==============================================================================

class FrameAttachmentWindowTests : public ::testing::Test
{
protected:
    /** Work that only one thread at a time may be doing. */
    struct ExclusiveWork
    {
        void run()
        {
            if (workers.fetch_add (1) != 0)
                overlapped = true;

            Thread::sleep (1);

            workers.fetch_sub (1);
        }

        std::atomic<int> workers { 0 };
        std::atomic<bool> overlapped { false };
    };

    class PaintedWindow final : public Component
    {
    public:
        void paint (Graphics& g) override
        {
            g.setFillColor (Colors::black);
            g.fillAll();

            if (onPaint != nullptr)
                onPaint();
        }

        std::function<void()> onPaint;
    };

    void SetUp() override
    {
#if YUP_MODAL_LOOPS_PERMITTED
        window.setBounds (0.0f, 0.0f, 200.0f, 200.0f);
        window.setVisible (true);
        window.addToDesktop (ComponentNative::Options {}
                                 .withDecoration (false)
                                 .withResizableWindow (false)
                                 .withFocusable (false)
                                 .withAllowedHighDensityDisplay (false)
                                 .withTemporaryWindow (true));

        auto* native = window.getNativeComponent();

        if (native == nullptr || ! native->isVisible() || native->getGraphicsContext() == nullptr)
            GTEST_SKIP() << "no usable native window in this environment";
#else
        GTEST_SKIP() << "the frames need a message loop the test can run";
#endif
    }

    void TearDown() override
    {
        window.removeFromDesktop();
    }

    static void runDispatchLoopFor (int milliseconds)
    {
#if YUP_MODAL_LOOPS_PERMITTED
        MessageManager::getInstance()->runDispatchLoopUntil (milliseconds);
#else
        ignoreUnused (milliseconds);
#endif
    }

    /** Runs the message loop until the condition holds, giving up after two seconds. */
    template <class Condition>
    static bool runDispatchLoopUntil (Condition&& condition)
    {
        const auto startMs = Time::getMillisecondCounterHiRes();

        while (! condition() && Time::getMillisecondCounterHiRes() - startMs < 2000.0)
            runDispatchLoopFor (10);

        return condition();
    }

    ExclusiveWork work;
    PaintedWindow window;
};

TEST_F (FrameAttachmentWindowTests, CallsBackWithTheMessageManagerLocked)
{
    std::atomic<int> frames { 0 };
    std::atomic<bool> messageManagerLocked { false };

    FrameAttachment attachment (window, [&] (double)
    {
        messageManagerLocked = MessageManager::getInstance()->currentThreadHasLockedMessageManager();
        ++frames;
    });

    ASSERT_TRUE (runDispatchLoopUntil ([&]
    {
        return frames > 0;
    }));

    EXPECT_TRUE (messageManagerLocked.load());
}

TEST_F (FrameAttachmentWindowTests, FramesNeverRunAtTheSameTimeAsTheMessageThread)
{
    struct MessageThreadWork final : Timer
    {
        explicit MessageThreadWork (ExclusiveWork& workToRun)
            : work (workToRun)
        {
        }

        void timerCallback() override
        {
            work.run();
            ++runs;
        }

        ExclusiveWork& work;
        int runs = 0;
    };

    std::atomic<int> frames { 0 };

    window.onPaint = [this]
    {
        work.run();
    };

    FrameAttachment attachment (window, [&] (double)
    {
        work.run();
        window.repaint();
        ++frames;
    });

    MessageThreadWork messageThreadWork (work);
    messageThreadWork.startTimer (1);

    const bool enoughFrames = runDispatchLoopUntil ([&]
    {
        return frames >= 10;
    });

    messageThreadWork.stopTimer();

    ASSERT_TRUE (enoughFrames);
    EXPECT_GT (messageThreadWork.runs, 0);
    EXPECT_FALSE (work.overlapped.load());
}

TEST_F (FrameAttachmentWindowTests, FirstFrameAfterAnIdlePeriodReportsOneFrame)
{
    // Nothing repaints and nothing listens, so the window has no frames to run
    runDispatchLoopFor (500);

    std::atomic<int> frames { 0 };
    std::atomic<double> firstElapsedSeconds { 0.0 };

    FrameAttachment attachment (window, [&] (double elapsedSeconds)
    {
        if (frames++ == 0)
            firstElapsedSeconds = elapsedSeconds;
    });

    ASSERT_TRUE (runDispatchLoopUntil ([&]
    {
        return frames > 0;
    }));

    EXPECT_LT (firstElapsedSeconds.load(), 0.25);
}
