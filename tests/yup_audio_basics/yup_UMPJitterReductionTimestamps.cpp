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

#include <yup_audio_basics/yup_audio_basics.h>

#include <gtest/gtest.h>

using namespace yup;
using namespace yup::ump;

TEST (JitterReductionTimestampTests, TimestampValue)
{
    JitterTimestamp t0;
    EXPECT_EQ (0u, t0.value);

    JitterTimestamp t1 { 55u };
    EXPECT_EQ (55u, t1.value);

    JitterTimestamp t2 { 0xffff };
    EXPECT_EQ (0xffffu, t2.value);
}

TEST (JitterReductionTimestampTests, TimestampEquality)
{
    JitterTimestamp t1 { 55u };
    JitterTimestamp t2 { 17283u };
    JitterTimestamp t3 { 0xffff };
    JitterTimestamp t4 { 55u };

    EXPECT_FALSE (t1 == t2);
    EXPECT_TRUE (t1 != t2);
    EXPECT_FALSE (t2 == t3);
    EXPECT_TRUE (t2 != t3);
    EXPECT_TRUE (t1 == t4);
    EXPECT_FALSE (t1 != t4);
}

TEST (JitterReductionTimestampTests, TimestampDifference)
{
    JitterTimestamp t1 { 55u };
    JitterTimestamp t2 { 17283u };
    JitterTimestamp t3 { 0xffff };
    JitterTimestamp t4 { 55u };

    EXPECT_EQ ((t2 - t1).count(), 17228);
    EXPECT_EQ ((t3 - t2).count(), 48252);
    EXPECT_EQ ((t4 - t3).count(), 56);
    EXPECT_EQ ((t1 - t4).count(), 0);
}

TEST (JitterReductionTimestampTests, JrClockMessage)
{
    JitterClockMessage clockMsg;
    EXPECT_EQ (clockMsg.getType(), PacketType::utility);
    EXPECT_EQ (clockMsg.getStatus(), uint8_t (UtilityStatus::jitterClock));
    EXPECT_EQ (clockMsg.getByte3(), 0u);
    EXPECT_EQ (clockMsg.getByte4(), 0u);

    JitterTimestamp timestamp { 0xf3f4 };
    clockMsg.setTimestamp (timestamp);
    EXPECT_EQ (clockMsg.getTimestamp(), timestamp);
    EXPECT_EQ (clockMsg.getByte3(), 0xf3u);
    EXPECT_EQ (clockMsg.getByte4(), 0xf4u);
}

TEST (JitterReductionTimestampTests, JitterTimestampMessage)
{
    JitterTimestampMessage tsMsg;
    EXPECT_EQ (tsMsg.getType(), PacketType::utility);
    EXPECT_EQ (tsMsg.getStatus(), uint8_t (UtilityStatus::jitterTimestamp));
    EXPECT_EQ (tsMsg.getByte3(), 0u);
    EXPECT_EQ (tsMsg.getByte4(), 0u);

    JitterTimestamp timestamp { 0x7b7c };
    tsMsg.setTimestamp (timestamp);
    EXPECT_EQ (tsMsg.getTimestamp(), timestamp);
    EXPECT_EQ (tsMsg.getByte3(), 0x7bu);
    EXPECT_EQ (tsMsg.getByte4(), 0x7cu);
}

TEST (JitterReductionTimestampTests, JitterClockFollowerScheduling)
{
    JitterClockFollower follower;
    follower.reset();

    const auto now = JitterClockFollower::SystemClock::now();
    const auto offset = std::chrono::milliseconds (5);
    follower.setSecurityOffset (offset);

    follower.processClock (now, JitterTimestamp { 0 });
    const auto scheduled = follower.scheduleMessage (now, JitterTimestamp { 0 });

    EXPECT_GE (scheduled, now + offset);
    EXPECT_EQ (follower.getSecurityOffset(), offset);
}

TEST (JitterReductionTimestampTests, JitterClockNow)
{
    auto t1 = JitterClock::now();
    std::this_thread::sleep_for (std::chrono::microseconds (100));
    auto t2 = JitterClock::now();

    EXPECT_NE (t1.value, t2.value);
}

TEST (JitterReductionTimestampTests, JitterTimestampWraparound)
{
    JitterTimestamp t1 { 0xfffe };
    JitterTimestamp t2 { 0x0002 };

    auto diff = t2 - t1;
    EXPECT_EQ (diff.count(), 4);
}

TEST (JitterReductionTimestampTests, JitterClockFollowerInitialState)
{
    JitterClockFollower follower;
    follower.reset();

    EXPECT_EQ (follower.getSecurityOffset().count(), 2);
}

TEST (JitterReductionTimestampTests, JitterClockFollowerFirstClock)
{
    JitterClockFollower follower;
    follower.reset();

    const auto now = JitterClockFollower::SystemClock::now();
    follower.processClock (now, JitterTimestamp { 100 });

    const auto scheduled = follower.scheduleMessage (now, JitterTimestamp { 100 });
    EXPECT_GE (scheduled, now);
}

TEST (JitterReductionTimestampTests, JitterClockFollowerSubsequentClocks)
{
    JitterClockFollower follower;
    follower.reset();

    auto now = JitterClockFollower::SystemClock::now();
    follower.processClock (now, JitterTimestamp { 1000 });

    now += std::chrono::milliseconds (10);
    follower.processClock (now, JitterTimestamp { 1100 });

    const auto scheduled = follower.scheduleMessage (now, JitterTimestamp { 1100 });
    EXPECT_GE (scheduled, now);
}

TEST (JitterReductionTimestampTests, JitterClockFollowerEarlyClock)
{
    JitterClockFollower follower;
    follower.reset();

    auto now = JitterClockFollower::SystemClock::now();
    follower.processClock (now, JitterTimestamp { 1000 });

    auto early = now + std::chrono::milliseconds (5);
    follower.processClock (early, JitterTimestamp { 1100 });

    auto laterStill = now + std::chrono::milliseconds (20);
    const auto scheduled = follower.scheduleMessage (laterStill, JitterTimestamp { 1200 });
    EXPECT_GE (scheduled, now);
}

TEST (JitterReductionTimestampTests, JitterClockFollowerLateClock)
{
    JitterClockFollower follower;
    follower.reset();

    auto now = JitterClockFollower::SystemClock::now();
    follower.processClock (now, JitterTimestamp { 1000 });

    auto late = now + std::chrono::milliseconds (20);
    follower.processClock (late, JitterTimestamp { 1100 });

    const auto scheduled = follower.scheduleMessage (late, JitterTimestamp { 1100 });
    EXPECT_GE (scheduled, now);
}

TEST (JitterReductionTimestampTests, JitterClockFollowerScheduleWithoutClock)
{
    JitterClockFollower follower;
    follower.reset();

    const auto now = JitterClockFollower::SystemClock::now();
    const auto scheduled = follower.scheduleMessage (now, JitterTimestamp { 100 });

    EXPECT_GE (scheduled, now);
}

TEST (JitterReductionTimestampTests, JitterClockFollowerSecurityOffsetAdjustment)
{
    JitterClockFollower follower;
    follower.reset();

    auto now = JitterClockFollower::SystemClock::now();
    follower.processClock (now, JitterTimestamp { 1000 });

    auto late = now + std::chrono::milliseconds (50);
    follower.processClock (late, JitterTimestamp { 1100 });

    EXPECT_GT (follower.getSecurityOffset().count(), 2);
}

TEST (JitterReductionTimestampTests, JitterClockFollowerMultipleSchedules)
{
    JitterClockFollower follower;
    follower.reset();

    auto now = JitterClockFollower::SystemClock::now();
    follower.processClock (now, JitterTimestamp { 1000 });

    auto scheduled1 = follower.scheduleMessage (now, JitterTimestamp { 1000 });
    auto scheduled2 = follower.scheduleMessage (now, JitterTimestamp { 1010 });
    auto scheduled3 = follower.scheduleMessage (now, JitterTimestamp { 1020 });

    EXPECT_LE (scheduled1, scheduled2);
    EXPECT_LE (scheduled2, scheduled3);
}

TEST (JitterReductionTimestampTests, JitterClockMessageConstruction)
{
    JitterClockMessage msg;
    EXPECT_EQ (msg.getType(), PacketType::utility);
    EXPECT_EQ (msg.getStatus(), uint8_t (UtilityStatus::jitterClock));
}

TEST (JitterReductionTimestampTests, JitterTimestampMessageConstruction)
{
    JitterTimestampMessage msg;
    EXPECT_EQ (msg.getType(), PacketType::utility);
    EXPECT_EQ (msg.getStatus(), uint8_t (UtilityStatus::jitterTimestamp));
}

TEST (JitterReductionTimestampTests, JitterClockMessageSetGetTimestamp)
{
    JitterClockMessage msg;

    for (uint16_t i = 0; i < 1000; i += 100)
    {
        JitterTimestamp ts { i };
        msg.setTimestamp (ts);
        EXPECT_EQ (msg.getTimestamp(), ts);
    }
}

TEST (JitterReductionTimestampTests, JitterTimestampMessageSetGetTimestamp)
{
    JitterTimestampMessage msg;

    for (uint16_t i = 0; i < 1000; i += 100)
    {
        JitterTimestamp ts { i };
        msg.setTimestamp (ts);
        EXPECT_EQ (msg.getTimestamp(), ts);
    }
}

TEST (JitterReductionTimestampTests, JitterTimestampMaxValue)
{
    JitterTimestamp t { 0xffff };
    EXPECT_EQ (t.value, 0xffffu);

    JitterTimestamp t2 { 0 };
    auto diff = t2 - t;
    EXPECT_EQ (diff.count(), 1);
}

TEST (JitterReductionTimestampTests, JitterClockFollowerResetClearsState)
{
    JitterClockFollower follower;

    auto now = JitterClockFollower::SystemClock::now();
    follower.processClock (now, JitterTimestamp { 1000 });
    follower.setSecurityOffset (std::chrono::milliseconds (100));

    follower.reset();

    EXPECT_EQ (follower.getSecurityOffset().count(), 2);
}
