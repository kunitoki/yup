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
#if YUP_WINDOWS
    const auto relativeMs = (milliseconds - 1.0) - Time::getMillisecondCounterHiRes();
    if (relativeMs <= 0.0)
        return;

    LARGE_INTEGER dueTime;
    dueTime.QuadPart = -static_cast<LONGLONG> (relativeMs * 10000.0); // relative, in 100ns units

    if (handle != nullptr && SetWaitableTimer (handle, &dueTime, 0, nullptr, nullptr, FALSE) != 0)
    {
        WaitForSingleObject (handle, INFINITE);

        while (Time::getMillisecondCounterHiRes() < milliseconds)
            std::this_thread::yield();

        return;
    }
#endif

    waitUntilFallback (milliseconds);
}

void WaitableTimer::waitUntilFallback (double milliseconds)
{
    if (const auto nowMs = Time::getMillisecondCounterHiRes(); milliseconds - nowMs > 4.0)
    {
        const auto target = std::chrono::steady_clock::now() + std::chrono::duration<double, std::milli> ((milliseconds - 4.0) - nowMs);

        std::unique_lock lock (mutex);
        cv.wait_until (lock, target);
    }

    while (Time::getMillisecondCounterHiRes() < milliseconds - 4.0)
        std::this_thread::sleep_for (std::chrono::microseconds (25));

    while (Time::getMillisecondCounterHiRes() < milliseconds - 2.0)
        std::this_thread::sleep_for (std::chrono::microseconds (10));

    while (Time::getMillisecondCounterHiRes() < milliseconds)
        std::this_thread::sleep_for (std::chrono::microseconds (1));
}

} // namespace yup
