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

#include <atomic>
#include <thread>
#include <vector>

using namespace yup;

namespace
{

void runWorkers (CountDownLatch& latch, int numWorkers, std::atomic<int>& done)
{
    std::vector<std::thread> workers;

    for (int i = 0; i < numWorkers; ++i)
        workers.emplace_back ([&]
        {
            for (volatile int spin = 0; spin < 1000 * (i + 1); ++spin)
            {
            }
            latch.countDown();
            ++done;
        });

    for (auto& worker : workers)
        worker.join();
}

} // namespace

TEST (CountDownLatchTests, WaitsUntilTheCountReachesZero)
{
    constexpr int numWorkers = 8;

    CountDownLatch latch (numWorkers);
    std::atomic<int> done { 0 };

    std::thread waiter ([&]
    {
        latch.wait();
    });

    runWorkers (latch, numWorkers, done);

    waiter.join();

    EXPECT_EQ (numWorkers, done.load());
    EXPECT_EQ (0, latch.getCount());
}

TEST (CountDownLatchTests, CountsDownBelowZeroWithoutWakingAnyone)
{
    CountDownLatch latch (0);

    latch.countDown();
    latch.countDown();

    EXPECT_EQ (0, latch.getCount());
}

TEST (CountDownLatchTests, AddCountBeforeDispatchingWorkKeepsTheWaiterBlocked)
{
    // The pattern the parallel import parser uses: a worker raises the count
    // before its work is dispatched. The waiter starts blocked (count 1), so
    // the only way it can wake before the worker finishes is if addCount()
    // failed to keep the count above zero.
    CountDownLatch latch (1);

    std::atomic<bool> workDone { false };

    std::thread worker ([&]
    {
        latch.addCount (1);

        for (volatile int spin = 0; spin < 100000; ++spin)
        {
        }

        workDone.store (true);
        latch.countDown(); // 2 -> 1: the waiter must still be blocked
        latch.countDown(); // 1 -> 0: only now may it wake
    });

    std::thread waiter ([&]
    {
        latch.wait();
        EXPECT_TRUE (workDone.load());
    });

    worker.join();
    waiter.join();

    EXPECT_EQ (0, latch.getCount());
}

TEST (CountDownLatchTests, WaitsFromMultipleThreads)
{
    constexpr int numWorkers = 8;

    CountDownLatch latch (numWorkers);
    std::atomic<int> waiting { 0 };
    std::atomic<int> released { 0 };

    std::vector<std::thread> waiters;

    for (int i = 0; i < 4; ++i)
        waiters.emplace_back ([&]
        {
            ++waiting;
            latch.wait();
            ++released;
        });

    while (waiting.load() < 4)
        std::this_thread::yield();

    std::atomic<int> done { 0 };
    runWorkers (latch, numWorkers, done);

    for (auto& waiter : waiters)
        waiter.join();

    EXPECT_EQ (4, released.load());
    EXPECT_EQ (0, latch.getCount());
}
