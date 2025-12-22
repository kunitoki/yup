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
