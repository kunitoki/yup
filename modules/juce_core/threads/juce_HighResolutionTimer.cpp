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

namespace juce
{

//==============================================================================
class HighResolutionTimer::Impl : private PlatformTimerListener
{
public:
    explicit Impl (HighResolutionTimer& o)
        : owner { o }
    {
    }

    void startTimer (int newIntervalMs)
    {
        shouldCancelCallbacks.store (true);

        const auto shouldWaitForPendingCallbacks = [&]
        {
            const std::scoped_lock lock { timerMutex };

            if (timer.getIntervalMs() > 0)
                timer.cancelTimer();

            jassert (timer.getIntervalMs() == 0);

            if (newIntervalMs > 0)
                timer.startTimer (jmax (0, newIntervalMs));

            return callbackThreadId != std::this_thread::get_id()
                && timer.getIntervalMs() <= 0;
        }();

        if (shouldWaitForPendingCallbacks)
            std::scoped_lock lock { callbackMutex };
    }

    int getIntervalMs() const
    {
        const std::scoped_lock lock { timerMutex };
        return timer.getIntervalMs();
    }

    bool isTimerRunning() const
    {
        return getIntervalMs() > 0;
    }

private:
    void onTimerExpired() final
    {
        callbackThreadId.store (std::this_thread::get_id());

        {
            std::scoped_lock lock { callbackMutex };

            if (isTimerRunning())
            {
                try
                {
                    owner.hiResTimerCallback();
                }
                catch (...)
                {
                    // Exceptions thrown in a timer callback won't be
                    // propagated to the main thread, it's best to find
                    // a way to avoid them if possible
                    jassertfalse;
                }
            }
        }

        callbackThreadId.store ({});
    }

    HighResolutionTimer& owner;
    mutable std::mutex timerMutex;
    std::mutex callbackMutex;
    std::atomic<std::thread::id> callbackThreadId {};
    std::atomic<bool> shouldCancelCallbacks { false };
    PlatformTimer timer { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Impl)
    JUCE_DECLARE_NON_MOVEABLE (Impl)
};

//==============================================================================
HighResolutionTimer::HighResolutionTimer()
    : impl (std::make_unique<Impl> (*this))
{
}

HighResolutionTimer::~HighResolutionTimer()
{
    // You *must* call stopTimer from the derived class destructor to
    // avoid data races on the timer's vtable
    jassert (! isTimerRunning());
    stopTimer();
}

void HighResolutionTimer::startTimer (int newIntervalMs)
{
    impl->startTimer (newIntervalMs);
}

void HighResolutionTimer::stopTimer()
{
    impl->startTimer (0);
}

int HighResolutionTimer::getTimerInterval() const noexcept
{
    return impl->getIntervalMs();
}

bool HighResolutionTimer::isTimerRunning() const noexcept
{
    return impl->isTimerRunning();
}

} // namespace juce
