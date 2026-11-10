/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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

#if ! (YUP_WASM || YUP_ANDROID)

using namespace yup;

class InterProcessSemaphoreTests : public ::testing::Test
{
protected:
    String name = SharedMemory::createUniqueName ("sem");
};

TEST_F (InterProcessSemaphoreTests, WaitTimesOutWhenNotSignalled)
{
    InterProcessSemaphore semaphore;
    ASSERT_TRUE (semaphore.create (name).wasOk());

    const auto start = Time::getMillisecondCounterHiRes();
    EXPECT_FALSE (semaphore.wait (30));
    EXPECT_GE (Time::getMillisecondCounterHiRes() - start, 25.0);

    EXPECT_FALSE (semaphore.wait (0));
}

TEST_F (InterProcessSemaphoreTests, SignalsAreCounted)
{
    InterProcessSemaphore semaphore;
    ASSERT_TRUE (semaphore.create (name).wasOk());

    semaphore.signal();
    semaphore.signal();

    EXPECT_TRUE (semaphore.wait (0));
    EXPECT_TRUE (semaphore.wait (0));
    EXPECT_FALSE (semaphore.wait (0));
}

TEST_F (InterProcessSemaphoreTests, SignalWakesWaiterOnOpenedSemaphore)
{
    InterProcessSemaphore created, opened;
    ASSERT_TRUE (created.create (name).wasOk());
    ASSERT_TRUE (opened.open (name).wasOk());

    std::atomic<bool> woken { false };
    std::thread waiter ([&]
    {
        woken = opened.wait (5000);
    });

    Thread::sleep (20);
    created.signal();
    waiter.join();

    EXPECT_TRUE (woken);
}

TEST_F (InterProcessSemaphoreTests, PingPongBetweenHandles)
{
    InterProcessSemaphore requestA, doneA, requestB, doneB;
    ASSERT_TRUE (requestA.create (name + "r").wasOk());
    ASSERT_TRUE (doneA.create (name + "d").wasOk());
    ASSERT_TRUE (requestB.open (name + "r").wasOk());
    ASSERT_TRUE (doneB.open (name + "d").wasOk());

    constexpr int numRounds = 1000;
    std::thread server ([&]
    {
        for (int i = 0; i < numRounds; ++i)
        {
            if (! requestB.wait (5000))
                return;

            doneB.signal();
        }
    });

    int completed = 0;
    for (int i = 0; i < numRounds; ++i)
    {
        requestA.signal();
        if (doneA.wait (5000))
            ++completed;
    }

    server.join();
    EXPECT_EQ (numRounds, completed);
}

TEST_F (InterProcessSemaphoreTests, OpenFailsForUnknownNameAndUnopenedIsInert)
{
    InterProcessSemaphore semaphore;

    EXPECT_TRUE (semaphore.open (name).failed());
    EXPECT_FALSE (semaphore.isOpen());

    semaphore.signal();
    EXPECT_FALSE (semaphore.wait (0));
}

#endif
