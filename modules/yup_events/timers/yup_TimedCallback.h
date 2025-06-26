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
namespace yup
{

/** Utility class wrapping a single non-null callback called by a Timer.

    You can use the usual Timer functions to start and stop the TimedCallback. Deleting the
    TimedCallback will automatically stop the underlying Timer.

    With this class you can use the Timer facility without inheritance.

    @see Timer
    @tags{Events}
*/
class TimedCallback final : private Timer
{
public:
    /** Constructor.

        The passed in callback won't be set but must be then set before starting the timer.

        @see onTimer
    */
    TimedCallback() = default;

    /** Constructor.

        The passed in callback must be non-null.
    */
    explicit TimedCallback (std::function<void()> timerCallback)
        : onTimer (std::move (timerCallback))
    {
        jassert (onTimer != nullptr);
    }

    /** Constructor.

        The passed in callback can be null but must be then set before starting the timer.

        @see onTimer
    */
    explicit TimedCallback (std::nullptr_t) = delete;

    /** Destructor. */
    ~TimedCallback() noexcept override
    {
        stopTimer();
    }

    /** Timer callback. */
    std::function<void()> onTimer;

    using Timer::getTimerInterval;
    using Timer::isTimerRunning;
    using Timer::startTimer;
    using Timer::startTimerHz;
    using Timer::stopTimer;

private:
    void timerCallback() override
    {
        jassert (onTimer != nullptr); // Did you forgot to set a timer callback before starting it ?

        if (onTimer)
            onTimer();
    }
};

} // namespace yup
