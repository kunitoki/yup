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

#include <yup_core/yup_core.h>

using namespace yup;

TEST (ThreadTests, Sleep)
{
    // Test basic sleep
    auto startTime = Time::getMillisecondCounter();
    Thread::sleep (100);
    auto elapsed = Time::getMillisecondCounter() - startTime;

    // Should sleep at least 100ms (with some tolerance for scheduling)
    EXPECT_GE (elapsed, 95);
    EXPECT_LT (elapsed, 200);

    // Test zero sleep
    Thread::sleep (0);

    // Test very short sleep
    Thread::sleep (1);
}

TEST (ThreadTests, ThreadCreationAndExecution)
{
    class TestThread : public Thread
    {
    public:
        TestThread()
            : Thread ("TestThread")
            , executed (false)
        {
        }

        void run() override
        {
            executed = true;
            Thread::sleep (50);
        }

        bool executed;
    };

    TestThread thread;
    EXPECT_FALSE (thread.executed);

    thread.startThread();
    thread.waitForThreadToExit (1000);

    EXPECT_TRUE (thread.executed);
}

TEST (ThreadTests, GetCurrentThreadId)
{
    auto threadId1 = Thread::getCurrentThreadId();
    auto threadId2 = Thread::getCurrentThreadId();

    // Same thread should have same ID
    EXPECT_EQ (threadId1, threadId2);
}

TEST (ThreadTests, Yield)
{
    // Just test that yield doesn't crash
    EXPECT_NO_THROW (Thread::yield());
}

#if YUP_LINUX || YUP_BSD
TEST (ThreadTests, SetCurrentThreadAffinityMask)
{
    // Test setting thread affinity (may not work on all systems)
    // Just ensure it doesn't crash
    EXPECT_NO_THROW (Thread::setCurrentThreadAffinityMask (1));
    EXPECT_NO_THROW (Thread::setCurrentThreadAffinityMask (0xFFFFFFFF));
}
#endif

TEST (ThreadTests, SetCurrentThreadName)
{
    // Test setting thread name (should not crash)
    EXPECT_NO_THROW (Thread::setCurrentThreadName ("TestThread"));
    EXPECT_NO_THROW (Thread::setCurrentThreadName ("LongerTestThreadName"));
    EXPECT_NO_THROW (Thread::setCurrentThreadName (""));
}
