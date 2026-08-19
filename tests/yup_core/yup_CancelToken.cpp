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

#include <algorithm>
#include <atomic>
#include <thread>
#include <vector>

using namespace yup;

TEST (CancelTokenTests, DefaultConstructedTokenIsNeverCancelledAndNotCancellable)
{
    CancelToken token;

    EXPECT_FALSE (token.wasCancelled());
    EXPECT_FALSE (token.isCancellable());
    EXPECT_EQ (CancelToken::none(), token);
}

TEST (CancelTokenTests, NoneTokenIsNeverCancelledAndNotCancellable)
{
    auto token = CancelToken::none();

    EXPECT_FALSE (token.isCancellable());
    EXPECT_FALSE (token.wasCancelled());
    EXPECT_FALSE (token.waitForCancellation (0));
}

TEST (CancelTokenTests, AllNoneTokensAreEqual)
{
    EXPECT_EQ (CancelToken::none(), CancelToken::none());
    EXPECT_EQ (CancelToken::none(), CancelToken());
}

TEST (CancelTokenTests, EqualityComparesSharedState)
{
    CancelToken defaultTokenA;
    CancelToken defaultTokenB;

    // default-constructed tokens are all the same never-cancellable token
    EXPECT_EQ (defaultTokenA, defaultTokenB);

    CancelTokenSource sourceA;
    CancelTokenSource sourceB;

    auto tokenA = sourceA.getToken();
    auto tokenB = sourceB.getToken();

    EXPECT_NE (tokenA, tokenB);

    auto tokenACopy = tokenA;

    EXPECT_EQ (tokenA, tokenACopy);
    EXPECT_TRUE (tokenA == tokenACopy);
    EXPECT_TRUE (tokenA != tokenB);
}

TEST (CancelTokenTests, TokenFromSourceIsCancellableAndNotCancelled)
{
    CancelTokenSource source;
    auto token = source.getToken();

    EXPECT_TRUE (token.isCancellable());
    EXPECT_FALSE (token.wasCancelled());
}

TEST (CancelTokenTests, MovedFromTokenIsNotCancellable)
{
    CancelTokenSource source;
    auto token = source.getToken();

    CancelToken destination (std::move (token));

    EXPECT_FALSE (token.isCancellable());
    EXPECT_TRUE (destination.isCancellable());

    source.cancel();

    EXPECT_TRUE (destination.wasCancelled());
}

TEST (CancelTokenTests, CopiesShareCancellationState)
{
    CancelTokenSource source;
    auto token = source.getToken();
    auto copy = token;

    source.cancel();

    EXPECT_TRUE (token.wasCancelled());
    EXPECT_TRUE (copy.wasCancelled());
}

TEST (CancelTokenTests, AssignmentSharesCancellationState)
{
    CancelTokenSource source;
    auto token = source.getToken();

    CancelToken other;
    other = token;

    source.cancel();

    EXPECT_TRUE (other.wasCancelled());
}

TEST (CancelTokenTests, WaitForCancellationTimesOut)
{
    CancelTokenSource source;
    auto token = source.getToken();

    EXPECT_FALSE (token.waitForCancellation (50));
    EXPECT_FALSE (token.wasCancelled());
}

TEST (CancelTokenTests, WaitForCancellationReturnsImmediatelyWhenAlreadyCancelled)
{
    CancelTokenSource source;
    auto token = source.getToken();

    source.cancel();

    EXPECT_TRUE (token.waitForCancellation (0));
}

TEST (CancelTokenTests, WaitForCancellationUnblocksWhenCancelledFromAnotherThread)
{
    CancelTokenSource source;
    auto token = source.getToken();

    std::thread canceller ([&]
    {
        Thread::sleep (50);
        source.cancel();
    });

    EXPECT_TRUE (token.waitForCancellation (2000));

    canceller.join();

    EXPECT_TRUE (token.wasCancelled());
}

TEST (CancelTokenTests, WaitForCancellationWakesMultipleWaiters)
{
    CancelTokenSource source;
    auto token = source.getToken();

    std::atomic<int> woken { 0 };
    std::vector<std::thread> waiters;

    for (int i = 0; i < 4; ++i)
    {
        waiters.emplace_back ([&]
        {
            if (token.waitForCancellation (2000))
                ++woken;
        });
    }

    Thread::sleep (50);
    source.cancel();

    for (auto& waiter : waiters)
        waiter.join();

    EXPECT_EQ (4, woken.load());
}

TEST (CancelTokenTests, CallbackIsInvokedOnCancel)
{
    CancelTokenSource source;
    auto token = source.getToken();

    std::atomic<int> callCount { 0 };

    auto registration = token.registerCallback ([&]
    {
        ++callCount;
    });

    EXPECT_EQ (0, callCount.load());

    source.cancel();

    EXPECT_EQ (1, callCount.load());
}

TEST (CancelTokenTests, CallbackRegisteredAfterCancellationRunsImmediately)
{
    CancelTokenSource source;
    auto token = source.getToken();

    std::atomic<int> callCount { 0 };

    source.cancel();

    auto registration = token.registerCallback ([&]
    {
        ++callCount;
    });

    EXPECT_EQ (1, callCount.load());
    EXPECT_FALSE (registration.isValid()); // already ran; nothing left to unregister
}

TEST (CancelTokenTests, CallbacksRunInRegistrationOrder)
{
    CancelTokenSource source;
    auto token = source.getToken();

    std::vector<int> order;
    std::vector<CancelToken::Registration> registrations;

    for (int i = 0; i < 5; ++i)
        registrations.push_back (token.registerCallback ([&, i]
        {
            order.push_back (i);
        }));

    source.cancel();

    EXPECT_EQ (std::vector<int> ({ 0, 1, 2, 3, 4 }), order);
}

TEST (CancelTokenTests, UnregisterPreventsCallbackInvocation)
{
    CancelTokenSource source;
    auto token = source.getToken();

    std::atomic<int> callCount { 0 };

    auto registration = token.registerCallback ([&]
    {
        ++callCount;
    });
    registration.unregister();

    EXPECT_FALSE (registration.isValid());

    source.cancel();

    EXPECT_EQ (0, callCount.load());
}

TEST (CancelTokenTests, RegistrationDestructorUnregisters)
{
    CancelTokenSource source;
    auto token = source.getToken();

    std::atomic<int> callCount { 0 };

    {
        auto registration = token.registerCallback ([&]
        {
            ++callCount;
        });
        EXPECT_TRUE (registration.isValid());
    }

    source.cancel();

    EXPECT_EQ (0, callCount.load());
}

TEST (CancelTokenTests, RegistrationIsMoveable)
{
    CancelTokenSource source;
    auto token = source.getToken();

    std::atomic<int> callCount { 0 };

    auto registration = token.registerCallback ([&]
    {
        ++callCount;
    });
    auto moved = std::move (registration);

    EXPECT_FALSE (registration.isValid());
    EXPECT_TRUE (moved.isValid());

    source.cancel();

    EXPECT_EQ (1, callCount.load());
}

TEST (CancelTokenTests, RegisteringOnNoneTokenIsNoOp)
{
    auto token = CancelToken::none();
    std::atomic<int> callCount { 0 };

    auto registration = token.registerCallback ([&]
    {
        ++callCount;
    });

    EXPECT_FALSE (registration.isValid());
    EXPECT_EQ (0, callCount.load());

    EXPECT_FALSE (token.wasCancelled());
}

TEST (CancelTokenTests, RegisteringOnDefaultTokenIsNoOp)
{
    CancelToken token;
    std::atomic<int> callCount { 0 };

    auto registration = token.registerCallback ([&]
    {
        ++callCount;
    });

    EXPECT_FALSE (registration.isValid());
    EXPECT_EQ (0, callCount.load());
}

TEST (CancelTokenTests, ConcurrentRegisterUnregisterAndCancelIsDataRaceFree)
{
    CancelTokenSource source;
    auto token = source.getToken();

    constexpr int numThreads = 8;
    constexpr int callbacksPerThread = 200;

    std::atomic<int> mainCallbackCount { 0 };
    auto mainRegistration = token.registerCallback ([&]
    {
        ++mainCallbackCount;
    });

    // Per-callback invocation counters, indexed as [thread * callbacksPerThread + index]
    std::vector<std::atomic<int>> invocationCounts (static_cast<size_t> (numThreads * callbacksPerThread));

    std::vector<std::vector<CancelToken::Registration>> keptRegistrations (static_cast<size_t> (numThreads));
    std::vector<std::atomic<bool>> finished (static_cast<size_t> (numThreads));
    std::vector<std::thread> threads;

    for (int t = 0; t < numThreads; ++t)
    {
        threads.emplace_back ([&, t]
        {
            for (int i = 0; i < callbacksPerThread; ++i)
            {
                auto registration = token.registerCallback ([&, t, i]
                {
                    ++invocationCounts[static_cast<size_t> (t * callbacksPerThread + i)];
                });

                if (i % 3 == 0)
                    registration.unregister(); // these must never fire
                else
                    keptRegistrations[static_cast<size_t> (t)].push_back (std::move (registration));
            }

            finished[static_cast<size_t> (t)] = true;
        });
    }

    // Wait for every thread to finish registering before cancelling, so the
    // expected per-callback counts are deterministic
    const auto startTime = Time::getMillisecondCounter();

    while (std::any_of (finished.begin(), finished.end(), [] (const auto& f)
    {
        return ! f.load();
    }))
    {
        if (Time::getMillisecondCounter() - startTime > 5000)
            FAIL() << "Timed out waiting for registration threads";

        Thread::sleep (5);
    }

    source.cancel();

    for (auto& thread : threads)
        thread.join();

    EXPECT_EQ (1, mainCallbackCount.load());

    int totalInvocationCount = 0;

    for (int t = 0; t < numThreads; ++t)
    {
        for (int i = 0; i < callbacksPerThread; ++i)
        {
            const auto count = invocationCounts[static_cast<size_t> (t * callbacksPerThread + i)].load();

            if (i % 3 == 0)
                EXPECT_EQ (0, count); // unregistered before cancel -> never invoked
            else
                EXPECT_EQ (1, count); // registered before cancel -> invoked exactly once

            totalInvocationCount += count;
        }
    }

    const int unregisteredPerThread = (callbacksPerThread + 2) / 3; // number of i in [0, N) with i % 3 == 0
    const int keptPerThread = callbacksPerThread - unregisteredPerThread;

    EXPECT_EQ (numThreads * keptPerThread, totalInvocationCount);
}

TEST (CancelTokenTests, ConcurrentCancellationFromMultipleThreadsInvokesCallbacksExactlyOnce)
{
    CancelTokenSource source;
    auto token = source.getToken();

    constexpr int numCallbacks = 100;
    constexpr int numCancellers = 8;

    std::vector<std::atomic<int>> callCounts (static_cast<size_t> (numCallbacks));
    std::vector<CancelToken::Registration> registrations;

    for (int i = 0; i < numCallbacks; ++i)
        registrations.push_back (token.registerCallback ([&, i]
        {
            ++callCounts[static_cast<size_t> (i)];
        }));

    std::atomic<bool> start { false };
    std::atomic<int> sawCancelled { 0 };
    std::vector<std::thread> cancellers;

    for (int t = 0; t < numCancellers; ++t)
    {
        cancellers.emplace_back ([&]
        {
            while (! start.load())
                Thread::yield();

            source.cancel(); // only the first of these may invoke the callbacks

            if (token.wasCancelled())
                ++sawCancelled;
        });
    }

    start = true;

    for (auto& canceller : cancellers)
        canceller.join();

    // every thread that requested cancellation observes the cancelled state
    EXPECT_EQ (numCancellers, sawCancelled.load());

    // exactly once, despite 8 concurrent cancellation requests
    for (auto& count : callCounts)
        EXPECT_EQ (1, count.load());
}

TEST (CancelTokenTests, CancellationRacingWithUnregistrationInvokesCallbacksAtMostOnce)
{
    CancelTokenSource source;
    auto token = source.getToken();

    constexpr int numThreads = 8;
    constexpr int callbacksPerThread = 100;
    constexpr int unregisterEvery = 3; // 1/3 of the registrations are unregistered while cancelling

    std::vector<std::atomic<int>> callCounts (static_cast<size_t> (numThreads * callbacksPerThread));

    std::vector<std::vector<CancelToken::Registration>> registrations (static_cast<size_t> (numThreads));
    std::vector<std::atomic<bool>> ready (static_cast<size_t> (numThreads));
    std::atomic<bool> go { false };
    std::vector<std::thread> threads;

    for (int t = 0; t < numThreads; ++t)
    {
        threads.emplace_back ([&, t]
        {
            // phase 1: register all callbacks
            for (int i = 0; i < callbacksPerThread; ++i)
                registrations[static_cast<size_t> (t)].push_back (token.registerCallback ([&, t, i]
                {
                    ++callCounts[static_cast<size_t> (t * callbacksPerThread + i)];
                }));

            ready[static_cast<size_t> (t)] = true;

            // phase 2: unregister 1/3 of them, racing with the in-flight cancellation
            while (! go.load())
                Thread::yield();

            for (int i = 0; i < callbacksPerThread; ++i)
                if (i % unregisterEvery == 0)
                    registrations[static_cast<size_t> (t)][static_cast<size_t> (i)].unregister();
        });
    }

    // Wait for every callback to be registered before starting the race
    const auto startTime = Time::getMillisecondCounter();

    while (std::any_of (ready.begin(), ready.end(), [] (const auto& r)
    {
        return ! r.load();
    }))
    {
        if (Time::getMillisecondCounter() - startTime > 5000)
            FAIL() << "Timed out waiting for registration threads";

        Thread::sleep (5);
    }

    std::thread canceller ([&]
    {
        while (! go.load())
            Thread::yield();

        source.cancel();
    });

    go = true;

    canceller.join();

    for (auto& thread : threads)
        thread.join();

    // A callback may be captured by the in-flight cancellation even after
    // unregister() removed it, but it can never run more than once
    for (auto& count : callCounts)
        EXPECT_LE (count.load(), 1);
}

TEST (CancelTokenTests, ConcurrentCancellationWakesAllWaitingThreads)
{
    CancelTokenSource source;
    auto token = source.getToken();

    constexpr int numWaiters = 8;
    constexpr int numCancellers = 4;

    std::atomic<int> woken { 0 };
    std::vector<std::thread> waiters;

    for (int i = 0; i < numWaiters; ++i)
    {
        waiters.emplace_back ([&]
        {
            if (token.waitForCancellation (2000))
                ++woken;
        });
    }

    Thread::sleep (50); // let the waiters block on the event before cancelling

    std::atomic<bool> start { false };
    std::vector<std::thread> cancellers;

    for (int t = 0; t < numCancellers; ++t)
    {
        cancellers.emplace_back ([&]
        {
            while (! start.load())
                Thread::yield();

            source.cancel();
        });
    }

    start = true;

    for (auto& canceller : cancellers)
        canceller.join();

    for (auto& waiter : waiters)
        waiter.join();

    // manual-reset semantics: all blocked waiters are woken, and any waiter
    // that arrives after the cancellation still returns true immediately
    EXPECT_EQ (numWaiters, woken.load());
    EXPECT_TRUE (token.wasCancelled());
}
