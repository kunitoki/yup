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

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace yup;

namespace
{
constexpr int numThreads = 4;
constexpr int iterationsPerThread = 1000;
constexpr auto shortDelay = std::chrono::microseconds (10);
constexpr auto mediumDelay = std::chrono::milliseconds (1);
} // namespace

//==============================================================================
class SpinLockTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        counter = 0;
    }

    SpinLock spinLock;
    std::atomic<int> counter { 0 };
};

//==============================================================================
TEST_F (SpinLockTests, BasicLockUnlock)
{
    // Test basic lock/unlock functionality
    spinLock.enter();
    EXPECT_TRUE (true); // If we get here, enter() worked
    spinLock.exit();
    EXPECT_TRUE (true); // If we get here, exit() worked
}

TEST_F (SpinLockTests, TryEnterSuccess)
{
    // Test tryEnter when lock is available
    EXPECT_TRUE (spinLock.tryEnter());
    spinLock.exit();
}

TEST_F (SpinLockTests, TryEnterFailure)
{
    // Test tryEnter when lock is already held
    spinLock.enter();
    EXPECT_FALSE (spinLock.tryEnter()); // Should fail as lock is held
    spinLock.exit();
}

TEST_F (SpinLockTests, ScopedLockBasic)
{
    // Test basic scoped lock functionality
    {
        SpinLock::ScopedLockType lock (spinLock);
        EXPECT_FALSE (spinLock.tryEnter()); // Should be locked
    }
    // Lock should be released now
    EXPECT_TRUE (spinLock.tryEnter());
    spinLock.exit();
}

TEST_F (SpinLockTests, ScopedUnlock)
{
    spinLock.enter();
    {
        SpinLock::ScopedUnlockType unlock (spinLock);
        EXPECT_TRUE (spinLock.tryEnter()); // Should be available during unlock
        spinLock.exit();
    }
    // Lock should be re-acquired
    EXPECT_FALSE (spinLock.tryEnter());
    spinLock.exit();
}

TEST_F (SpinLockTests, ScopedTryLockSuccess)
{
    // Test scoped try-lock when lock is available
    {
        SpinLock::ScopedTryLockType tryLock (spinLock);
        EXPECT_TRUE (tryLock.isLocked());
        EXPECT_FALSE (spinLock.tryEnter()); // Should be locked
    }
    // Lock should be released
    EXPECT_TRUE (spinLock.tryEnter());
    spinLock.exit();
}

TEST_F (SpinLockTests, MultiThreadedCounter)
{
    // Test thread safety with multiple threads incrementing a counter
    std::vector<std::thread> threads;

    for (int i = 0; i < numThreads; ++i)
    {
        threads.emplace_back ([this]()
        {
            for (int j = 0; j < iterationsPerThread; ++j)
            {
                SpinLock::ScopedLockType lock (spinLock);
                ++counter;
            }
        });
    }

    for (auto& thread : threads)
        thread.join();

    EXPECT_EQ (counter.load(), numThreads * iterationsPerThread);
}

TEST_F (SpinLockTests, MultiThreadedTryEnter)
{
    // Test that tryEnter works correctly under contention
    std::atomic<int> successCount { 0 };
    std::atomic<int> failureCount { 0 };
    std::vector<std::thread> threads;

    for (int i = 0; i < numThreads; ++i)
    {
        threads.emplace_back ([this, &successCount, &failureCount]()
        {
            for (int j = 0; j < iterationsPerThread; ++j)
            {
                if (spinLock.tryEnter())
                {
                    ++successCount;
                    // Do some brief work
                    std::this_thread::sleep_for (shortDelay);
                    spinLock.exit();
                }
                else
                {
                    ++failureCount;
                }
            }
        });
    }

    for (auto& thread : threads)
        thread.join();

    EXPECT_GT (successCount.load(), 0); // Some attempts should succeed
    EXPECT_GT (failureCount.load(), 0); // Some attempts should fail due to contention
    EXPECT_EQ (successCount.load() + failureCount.load(), numThreads * iterationsPerThread);
}

TEST_F (SpinLockTests, Performance)
{
    // Basic performance test - ensure locking doesn't take too long
    const int iterations = 10000;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i)
    {
        SpinLock::ScopedLockType lock (spinLock);
        ++counter; // Do minimal work
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds> (end - start);

    EXPECT_EQ (counter.load(), iterations);
    EXPECT_LT (duration.count(), 1000); // Should complete in under 1 second
}

TEST_F (SpinLockTests, ExceptionSafety)
{
    // Test that scoped lock releases even when exception is thrown
    bool exceptionThrown = false;

    try
    {
        SpinLock::ScopedLockType lock (spinLock);
        EXPECT_FALSE (spinLock.tryEnter()); // Should be locked
        throw std::runtime_error ("Test exception");
    }
    catch (const std::exception&)
    {
        exceptionThrown = true;
    }

    EXPECT_TRUE (exceptionThrown);
    EXPECT_TRUE (spinLock.tryEnter()); // Lock should be released
    spinLock.exit();
}
