/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

namespace yup::ump
{

JitterTimestamp JitterClock::now()
{
    using SystemClock = std::chrono::high_resolution_clock;
    static const auto zero = SystemClock::now();

    const auto jrTimestampNow = std::chrono::duration_cast<JitterTicks> (SystemClock::now() - zero);
    return JitterTimestamp { uint16_t (jrTimestampNow.count() & 0xffff) };
}

void JitterClockFollower::reset()
{
    clockTimestamp = {};
    clockTime = zero;
    messageTime = zero;
    jitter = std::chrono::milliseconds (0);
    securityOffset = std::chrono::milliseconds (2);
}

void JitterClockFollower::setSecurityOffset (Duration offset)
{
    jassert (offset >= Duration { 0 });
    securityOffset = offset;
}

void JitterClockFollower::processClock (TimePoint received, JitterTimestamp timestamp)
{
    if (clockTime == zero)
    {
        clockTime = received;
        clockTimestamp = timestamp;
        messageTime = received;
        return;
    }

    const auto diffTimestamp = timestamp - clockTimestamp;
    clockTimestamp = timestamp;

    const auto diffDuration = std::chrono::duration_cast<SystemClock::duration> (diffTimestamp);
    const auto expectedReceiveTime = clockTime + diffDuration;
    const auto jitterValue = received - expectedReceiveTime;

    if (received < expectedReceiveTime)
    {
        clockTime = received;

        if (received > messageTime)
            messageTime = received;
    }
    else
    {
        clockTime = expectedReceiveTime;
        messageTime = received;
    }

    updateStats (jitterValue);
}

JitterClockFollower::TimePoint JitterClockFollower::scheduleMessage (TimePoint received, JitterTimestamp timestamp)
{
    if (clockTime != zero)
    {
        const auto diffTimestamp = timestamp - clockTimestamp;
        const auto diffDuration = std::chrono::duration_cast<SystemClock::duration> (diffTimestamp);
        const auto nextMessageTime = clockTime + diffDuration;

        if (nextMessageTime > messageTime)
            messageTime = nextMessageTime;
    }
    else
    {
        messageTime = received;
    }

    return messageTime + securityOffset;
}

void JitterClockFollower::updateStats (Duration jitterValue)
{
    if (jitterValue < Duration { 0 })
    {
        jitter -= jitterValue;
        recalcSecurityOffset();
    }
    else if (jitterValue > jitter)
    {
        jitter = jitterValue;
        recalcSecurityOffset();
    }
}

void JitterClockFollower::recalcSecurityOffset()
{
    const auto value = jitter * 12 / 10;
    if (value > securityOffset)
        securityOffset = value;
}

} // namespace yup::ump
