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
*/

namespace yup
{

//==============================================================================
/**
    Detects if a specified timeout duration has been reached.

    The `TimeoutDetector` class allows you to determine whether a certain amount
    of time has elapsed since its creation. It uses high-resolution timing to ensure
    precise timeout detection.

    @see RelativeTime, Time

    @tags{Core}
*/
class YUP_API TimeoutDetector
{
public:
    //==============================================================================
    /**
        Creates a TimeoutDetector object with a specified timeout duration.

        @param timeoutSeconds The duration in seconds after which the detector will consider the timeout to have been reached.
    */
    TimeoutDetector (double timeoutSeconds)
        : initialTimeTicks (Time::getHighResolutionTicks())
        , timeoutTicks (initialTimeTicks + Time::secondsToHighResolutionTicks (timeoutSeconds))
    {
    }

    //==============================================================================
    /**
        Checks whether the timeout duration has been reached.

        @return `true` if the current time has reached or exceeded the timeout threshold, otherwise `false`.
    */
    forcedinline bool hasTimedOut() const
    {
        return Time::getHighResolutionTicks() >= timeoutTicks;
    }

private:
    //==============================================================================
    int64 initialTimeTicks = 0;
    int64 timeoutTicks = 0;
};

} // namespace yup
