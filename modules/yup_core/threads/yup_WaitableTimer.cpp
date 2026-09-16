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

namespace yup
{

//==============================================================================

namespace {

#if YUP_APPLE || YUP_LINUX || YUP_ANDROID
static void spinWaitHint() noexcept
{
#if defined(__aarch64__) || defined(__arm__)
    __asm__ __volatile__ ("isb" ::: "memory");
#elif defined(__x86_64__) || defined(__i386__)
    __builtin_ia32_pause();
#else
    std::atomic_signal_fence (std::memory_order_acq_rel);
#endif
}
#endif

#if YUP_LINUX || YUP_ANDROID
/*  CLOCK_MONOTONIC in nanoseconds: the clock Time::getMillisecondCounterHiRes() reads, without
    its truncation to microseconds.
*/
static int64 monotonicNanos() noexcept
{
    timespec t;
    clock_gettime (CLOCK_MONOTONIC, &t);

    return (t.tv_sec * (int64) 1000000000) + t.tv_nsec;
}

static void reduceTimerSlackOnce() noexcept
{
    static thread_local bool alreadyReduced = false;

    if (alreadyReduced)
        return;

    alreadyReduced = true;
    prctl (PR_SET_TIMERSLACK, 1UL);
}
#endif

} // namespace

//==============================================================================

WaitableTimer::WaitableTimer()
{
#if YUP_WINDOWS
    handle = CreateWaitableTimerExW (nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    // CREATE_WAITABLE_TIMER_HIGH_RESOLUTION needs Windows 10 1803+, fall back to a plain waitable timer on older systems.
    if (handle == nullptr)
        handle = CreateWaitableTimerExW (nullptr, nullptr, 0, TIMER_ALL_ACCESS);
#endif
}

WaitableTimer::~WaitableTimer()
{
#if YUP_WINDOWS
    if (handle != nullptr)
        CloseHandle (handle);
#endif
}

void WaitableTimer::waitUntil (double milliseconds)
{
    const auto relativeMs = (milliseconds - 1.0) - Time::getMillisecondCounterHiRes();
    if (relativeMs <= 0.0)
        return;

#if YUP_WINDOWS
    LARGE_INTEGER dueTime;
    dueTime.QuadPart = -static_cast<LONGLONG> (relativeMs * 10000.0); // relative, in 100ns units

    if (handle != nullptr && SetWaitableTimer (handle, &dueTime, 0, nullptr, nullptr, FALSE) != 0)
    {
        WaitForSingleObject (handle, INFINITE);

        while (Time::getMillisecondCounterHiRes() < milliseconds)
            std::this_thread::yield();

        return;
    }

    waitUntilFallback (milliseconds);

#elif YUP_APPLE
    constexpr double sleepFraction = 0.75;
    constexpr double spinMilliseconds = 0.05;
    const auto ticksPerMillisecond = (double) Time::getHighResolutionTicksPerSecond() / 1000.0;

    for (;;)
    {
        const auto remainingMs = milliseconds - Time::getMillisecondCounterHiRes();
        if (remainingMs <= spinMilliseconds)
            break;

        mach_wait_until (mach_absolute_time() + (uint64) (remainingMs * sleepFraction * ticksPerMillisecond));
    }

    while (Time::getMillisecondCounterHiRes() < milliseconds)
        spinWaitHint();

#elif YUP_LINUX || YUP_ANDROID
    constexpr double sleepFraction = 0.75;
    constexpr double spinMilliseconds = 0.15;

    reduceTimerSlackOnce();

    for (;;)
    {
        const auto remainingMs = milliseconds - Time::getMillisecondCounterHiRes();
        if (remainingMs <= spinMilliseconds)
            break;

        const auto targetNs = monotonicNanos() + (int64) (remainingMs * sleepFraction * 1.0e6);

        timespec target;
        target.tv_sec = (time_t) (targetNs / 1000000000);
        target.tv_nsec = (long) (targetNs % 1000000000);

        while (clock_nanosleep (CLOCK_MONOTONIC, TIMER_ABSTIME, &target, nullptr) == EINTR)
            ;
    }

    while (Time::getMillisecondCounterHiRes() < milliseconds)
        spinWaitHint();

#else
    waitUntilFallback (milliseconds);

#endif
}

#if ! (YUP_APPLE || YUP_LINUX || YUP_ANDROID)
void WaitableTimer::waitUntilFallback (double milliseconds)
{
    if (const auto nowMs = Time::getMillisecondCounterHiRes(); milliseconds - nowMs > 4.0)
    {
        const auto target = std::chrono::steady_clock::now()
            + std::chrono::duration<double, std::milli> ((milliseconds - 4.0) - nowMs);

        std::unique_lock lock (mutex);
        cv.wait_until (lock, target, []{ return false; });
    }

    while (Time::getMillisecondCounterHiRes() < milliseconds - 2.0)
        std::this_thread::sleep_for (std::chrono::microseconds (20));

    while (Time::getMillisecondCounterHiRes() < milliseconds - 1.0)
        std::this_thread::sleep_for (std::chrono::microseconds (10));

    while (Time::getMillisecondCounterHiRes() < milliseconds - 0.5)
        std::this_thread::sleep_for (std::chrono::microseconds (5));

    while (Time::getMillisecondCounterHiRes() < milliseconds - 0.1)
        std::this_thread::sleep_for (std::chrono::microseconds (1));

    while (Time::getMillisecondCounterHiRes() < milliseconds)
        std::this_thread::yield();
}
#endif

} // namespace yup
