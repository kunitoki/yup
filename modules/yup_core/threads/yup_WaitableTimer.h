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

#pragma once

namespace yup
{

//==============================================================================
/**
    A timer that blocks a thread until an exact deadline without polling.

    waitUntil() suspends the calling thread until the given absolute time,
    measured in milliseconds on the same clock as
    Time::getMillisecondCounterHiRes(). Unlike a plain sleep, the wait is
    accurate to well under a millisecond on every platform:

    - On Windows it uses a high-resolution waitable timer
      (CreateWaitableTimerExW, with CREATE_WAITABLE_TIMER_HIGH_RESOLUTION when
      the OS supports it), which fires on a precise deadline and consumes no
      CPU while blocked. Plain sleeps are only as accurate as the 1 ms timer
      tick requested via timeBeginPeriod(), so a sleep targeting a deadline
      can overshoot it by up to a full tick.
    - On other platforms it waits on a condition variable with a
      steady-clock deadline, which the OS wakes with microsecond precision.

    Use it for frame pacing or any loop that must meet a deadline rather than
    merely sleep for a while.

    @see Time::getMillisecondCounterHiRes
*/
class YUP_API WaitableTimer
{
public:
    WaitableTimer();
    ~WaitableTimer();

    /**
        Waits until the given absolute time in milliseconds, on the same clock
        as Time::getMillisecondCounterHiRes().

        Returns immediately if the deadline has already passed.
    */
    void waitUntil (double milliseconds);

private:
#if YUP_WINDOWS
    void* handle = nullptr;
#endif

    void waitUntilFallback (double milliseconds);

    std::mutex mutex;
    std::condition_variable cv;
    std::chrono::steady_clock::time_point epoch;
    double epochCounterMs = 0.0;

    YUP_DECLARE_NON_COPYABLE (WaitableTimer)
};

} // namespace yup
