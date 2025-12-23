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

#ifndef DOXYGEN

namespace yup::ump
{

constexpr uint16_t jrClockFrequency = 31250;

using JitterTicks = std::chrono::duration<long long, std::ratio<1, jrClockFrequency>>;

struct JitterTimestamp
{
    uint16_t value {};

    constexpr JitterTimestamp() = default;

    constexpr explicit JitterTimestamp (uint16_t v)
        : value (v)
    {
    }

    constexpr bool operator== (JitterTimestamp other) const { return value == other.value; }

    constexpr bool operator!= (JitterTimestamp other) const { return value != other.value; }

    JitterTicks operator- (JitterTimestamp other) const
    {
        int diff = int (value) - int (other.value);
        if (diff < 0)
            diff += 0x10000;
        return JitterTicks { static_cast<uint16_t> (diff) };
    }
};

class JitterClock
{
public:
    using Duration = JitterTicks;
    using Rep = Duration::rep;
    using Period = Duration::period;
    using TimePoint = uint16_t;

    static JitterTimestamp now();
};

struct JitterMessage : UniversalPacket
{
    JitterTimestamp getTimestamp() const
    {
        return JitterTimestamp { uint16_t ((getByte (2) << 8) | getByte (3)) };
    }

    void setTimestamp (JitterTimestamp timestamp)
    {
        setByte (2, uint8_t (timestamp.value >> 8));
        setByte (3, uint8_t (timestamp.value & 0xff));
    }

protected:
    explicit JitterMessage (UtilityStatus status)
    {
        setType (PacketType::utility);
        setByte (1, uint8_t (status));
    }

    JitterMessage (UtilityStatus status, JitterTimestamp timestamp)
        : JitterMessage (status)
    {
        setTimestamp (timestamp);
    }
};

struct JitterClockMessage : JitterMessage
{
    JitterClockMessage()
        : JitterMessage (UtilityStatus::jitterClock, {})
    {
    }

    explicit JitterClockMessage (JitterTimestamp timestamp)
        : JitterMessage (UtilityStatus::jitterClock, timestamp)
    {
    }
};

struct JitterTimestampMessage : JitterMessage
{
    JitterTimestampMessage()
        : JitterMessage (UtilityStatus::jitterTimestamp, {})
    {
    }

    explicit JitterTimestampMessage (JitterTimestamp timestamp)
        : JitterMessage (UtilityStatus::jitterTimestamp, timestamp)
    {
    }
};

class JitterClockFollower
{
public:
    JitterClockFollower() = default;

    void reset();

    using SystemClock = std::chrono::high_resolution_clock;
    using TimePoint = SystemClock::time_point;
    using Duration = SystemClock::duration;

    void processClock (TimePoint received, JitterTimestamp timestamp);
    TimePoint scheduleMessage (TimePoint received, JitterTimestamp timestamp);

    Duration getSecurityOffset() const { return securityOffset; }

    Duration getJitter() const { return jitter; }

    void setSecurityOffset (Duration offset);

protected:
    void updateStats (Duration jitterValue);
    void recalcSecurityOffset();

private:
    const SystemClock::time_point zero = SystemClock::now();

    JitterTimestamp clockTimestamp { 0 };
    TimePoint clockTime { zero };
    TimePoint messageTime { zero };

    Duration jitter { 0 };
    Duration securityOffset { std::chrono::milliseconds (2) };
};

} // namespace yup::ump

#endif
