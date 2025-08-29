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

#include <gtest/gtest.h>

#include <yup_events/yup_events.h>

using namespace yup;

class TimerTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mm = MessageManager::getInstance();
        jassert (mm != nullptr);
    }

    void TearDown() override
    {
    }

    void runDispatchLoopUntil (int millisecondsToRunFor = 10)
    {
#if YUP_MODAL_LOOPS_PERMITTED
        mm->runDispatchLoopUntil (millisecondsToRunFor);
#endif
    }

    MessageManager* mm = nullptr;
};

TEST_F (TimerTests, DISABLED_SimpleTimerSingleCall)
{
    struct TestTimer : Timer
    {
        int calledCount = 0;

        void timerCallback() override
        {
            calledCount = 1;

            stopTimer();
        }
    } testTimer;

    testTimer.startTimer (1);

    EXPECT_EQ (testTimer.calledCount, 0);
    runDispatchLoopUntil (200);
    EXPECT_EQ (testTimer.calledCount, 1);
}
