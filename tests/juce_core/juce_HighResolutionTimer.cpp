/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include <gtest/gtest.h>

#include <juce_core/juce_core.h>

using namespace juce;

namespace {

constexpr int maximumTimeoutMs { 30'000 };

class TestTimer final : public HighResolutionTimer
{
public:
    explicit TestTimer (std::function<void()> fn)
        : callback (std::move (fn))
    {
    }

    ~TestTimer() override { stopTimer(); }

    void hiResTimerCallback() override { callback(); }

private:
    std::function<void()> callback;
};

} // namespace

TEST (HighResolutionTimerTests, StartStopTimer)
{
    WaitableEvent timerFiredOnce;
    WaitableEvent timerFiredTwice;

    TestTimer timer { [&, callbackCount = 0]() mutable
                    {
                        switch (++callbackCount)
                        {
                            case 1:
                                timerFiredOnce.signal();
                                return;
                            case 2:
                                timerFiredTwice.signal();
                                return;
                            default:
                                return;
                        }
                    } };

    EXPECT_TRUE (! timer.isTimerRunning());
    EXPECT_TRUE (timer.getTimerInterval() == 0);

    timer.startTimer (1);
    EXPECT_TRUE (timer.isTimerRunning());
    EXPECT_TRUE (timer.getTimerInterval() == 1);
    EXPECT_TRUE (timerFiredOnce.wait (maximumTimeoutMs));
    EXPECT_TRUE (timerFiredTwice.wait (maximumTimeoutMs));

    timer.stopTimer();
    EXPECT_TRUE (! timer.isTimerRunning());
    EXPECT_TRUE (timer.getTimerInterval() == 0);
}

TEST (HighResolutionTimerTests, StartStopTimerWithInterval)
{
    WaitableEvent stoppedTimer;

    auto timerCallback = [&] (TestTimer& timer)
    {
        EXPECT_TRUE (timer.isTimerRunning());
        timer.stopTimer();
        EXPECT_TRUE (! timer.isTimerRunning());
        stoppedTimer.signal();
    };

    TestTimer timer { [&]
                    {
                        timerCallback (timer);
                    } };
    timer.startTimer (1);
    EXPECT_TRUE (stoppedTimer.wait (maximumTimeoutMs));
}

TEST (HighResolutionTimerTests, RestartTimerFromTimerCallback)
{
    WaitableEvent restartTimer;
    WaitableEvent timerRestarted;
    WaitableEvent timerFiredAfterRestart;

    TestTimer timer { [&, callbackCount = 0]() mutable
                    {
                        switch (++callbackCount)
                        {
                            case 1:
                                EXPECT_TRUE (restartTimer.wait (maximumTimeoutMs));
                                EXPECT_TRUE (timer.getTimerInterval() == 1);

                                timer.startTimer (2);
                                EXPECT_TRUE (timer.getTimerInterval() == 2);
                                timerRestarted.signal();
                                return;

                            case 2:
                                EXPECT_TRUE (timer.getTimerInterval() == 2);
                                timerFiredAfterRestart.signal();
                                return;

                            default:
                                return;
                        }
                    } };

    timer.startTimer (1);
    EXPECT_TRUE (timer.getTimerInterval() == 1);

    restartTimer.signal();
    EXPECT_TRUE (timerRestarted.wait (maximumTimeoutMs));
    EXPECT_TRUE (timer.getTimerInterval() == 2);
    EXPECT_TRUE (timerFiredAfterRestart.wait (maximumTimeoutMs));

    timer.stopTimer();

    EXPECT_TRUE (! timer.isTimerRunning());
}

TEST (HighResolutionTimerTests, StopTimerFromTimerCallback)
{
    WaitableEvent timerCallbackStarted;
    WaitableEvent stoppingTimer;
    std::atomic<bool> timerCallbackFinished { false };

    TestTimer timer { [&, callbackCount = 0]() mutable
                    {
                        switch (++callbackCount)
                        {
                            case 1:
                                timerCallbackStarted.signal();
                                EXPECT_TRUE (stoppingTimer.wait (maximumTimeoutMs));
                                Thread::sleep (10);
                                timerCallbackFinished = true;
                                return;

                            default:
                                return;
                        }
                    } };

    timer.startTimer (1);
    EXPECT_TRUE (timerCallbackStarted.wait (maximumTimeoutMs));

    stoppingTimer.signal();
    timer.stopTimer();
    EXPECT_TRUE (timerCallbackFinished);
}

TEST (HighResolutionTimerTests, StopTimerFromTimerCallbackFirst)
{
    WaitableEvent stoppedFromInsideTimerCallback;
    WaitableEvent stoppingFromOutsideTimerCallback;
    std::atomic<bool> timerCallbackFinished { false };

    TestTimer timer { [&]()
                    {
                        timer.stopTimer();
                        stoppedFromInsideTimerCallback.signal();
                        EXPECT_TRUE (stoppingFromOutsideTimerCallback.wait (maximumTimeoutMs));
                        Thread::sleep (10);
                        timerCallbackFinished = true;
                    } };

    timer.startTimer (1);
    EXPECT_TRUE (stoppedFromInsideTimerCallback.wait (maximumTimeoutMs));

    stoppingFromOutsideTimerCallback.signal();
    timer.stopTimer();
    EXPECT_TRUE (timerCallbackFinished);
}

TEST (HighResolutionTimerTests, AdjustTimerIntervalFromOutsideTimerCallback)
{
    WaitableEvent timerCallbackStarted;
    WaitableEvent timerRestarted;
    WaitableEvent timerFiredAfterRestart;
    std::atomic<int> lastCallbackCount { 0 };

    TestTimer timer { [&, callbackCount = 0]() mutable
                    {
                        switch (++callbackCount)
                        {
                            case 1:
                                EXPECT_TRUE (timer.getTimerInterval() == 1);
                                timerCallbackStarted.signal();
                                Thread::sleep (10);
                                lastCallbackCount = 1;
                                return;

                            case 2:
                                EXPECT_TRUE (timerRestarted.wait (maximumTimeoutMs));
                                EXPECT_TRUE (timer.getTimerInterval() == 2);
                                lastCallbackCount = 2;
                                timerFiredAfterRestart.signal();
                                return;

                            default:
                                return;
                        }
                    } };

    timer.startTimer (1);
    EXPECT_TRUE (timerCallbackStarted.wait (maximumTimeoutMs));

    timer.startTimer (2);
    timerRestarted.signal();

    EXPECT_TRUE (timerFiredAfterRestart.wait (maximumTimeoutMs));
    EXPECT_TRUE (lastCallbackCount == 2);

    timer.stopTimer();
    EXPECT_TRUE (lastCallbackCount == 2);
}

TEST (HighResolutionTimerTests, TimerCanBeRestartedExternallyAfterBeingStoppedInternally)
{
    WaitableEvent timerStopped;
    WaitableEvent timerFiredAfterRestart;

    TestTimer timer { [&, callbackCount = 0]() mutable
                    {
                        switch (++callbackCount)
                        {
                            case 1:
                                timer.stopTimer();
                                timerStopped.signal();
                                return;

                            case 2:
                                timerFiredAfterRestart.signal();
                                return;

                            default:
                                return;
                        }
                    } };

    EXPECT_TRUE (! timer.isTimerRunning());
    timer.startTimer (1);
    EXPECT_TRUE (timer.isTimerRunning());

    EXPECT_TRUE (timerStopped.wait (maximumTimeoutMs));
    EXPECT_TRUE (! timer.isTimerRunning());

    timer.startTimer (1);
    EXPECT_TRUE (timer.isTimerRunning());
    EXPECT_TRUE (timerFiredAfterRestart.wait (maximumTimeoutMs));
}

TEST (HighResolutionTimerTests, CallsToStartTimerAndGetTimerIntervalSucceedWhileACallbackIsBlocked)
{
    WaitableEvent timerBlocked;
    WaitableEvent unblockTimer;

    TestTimer timer { [&]
                    {
                        timerBlocked.signal();
                        unblockTimer.wait();
                        timer.stopTimer();
                    } };

    timer.startTimer (1);
    timerBlocked.wait();

    EXPECT_TRUE (timer.getTimerInterval() == 1);
    timer.startTimer (2);
    EXPECT_TRUE (timer.getTimerInterval() == 2);

    unblockTimer.signal();
    timer.stopTimer();
}

TEST (HighResolutionTimerTests, StressTest)
{
    constexpr auto maxNumTimers { 100 };

    std::vector<std::unique_ptr<TestTimer>> timers;
    timers.reserve (maxNumTimers);

    for (int i = 0; i < maxNumTimers; ++i)
    {
        auto timer = std::make_unique<TestTimer> ([] {});
        timer->startTimer (1);

        if (! timer->isTimerRunning())
            break;

        timers.push_back (std::move (timer));
    }

    EXPECT_TRUE (timers.size() >= 16);
}
