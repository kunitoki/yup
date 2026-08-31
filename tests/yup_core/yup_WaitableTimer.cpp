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

#include <gtest/gtest.h>

#include <yup_core/yup_core.h>

using namespace yup;

class WaitableTimerTests : public ::testing::Test
{
protected:
    double now() const
    {
        return Time::getMillisecondCounterHiRes();
    }
};

TEST_F (WaitableTimerTests, WaitUntilReturnsImmediatelyForPastDeadline)
{
    WaitableTimer timer;

    const auto before = now();
    timer.waitUntil (before - 100.0);
    const auto after = now();

    EXPECT_LT (after - before, 100.0);
}

TEST_F (WaitableTimerTests, WaitUntilReturnsImmediatelyForCurrentDeadline)
{
    WaitableTimer timer;

    const auto before = now();
    timer.waitUntil (before);
    const auto after = now();

    EXPECT_LT (after - before, 100.0);
}

TEST_F (WaitableTimerTests, WaitUntilBlocksUntilFutureDeadline)
{
    WaitableTimer timer;

    const auto start = now();
    const auto target = start + 25.0;

    timer.waitUntil (target);

    const auto after = now();
    EXPECT_GE (after, target - 2.0);
    EXPECT_LT (after - start, 2'000.0);
}

TEST_F (WaitableTimerTests, WaitUntilLongDeadlineDoesNotReturnEarly)
{
    WaitableTimer timer;

    const auto start = now();
    const auto target = start + 40.0;

    timer.waitUntil (target);

    const auto after = now();
    EXPECT_GE (after, target - 2.0);
    EXPECT_LT (after - start, 2'000.0);
}
