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
constexpr int iterationsPerThread = 500;
constexpr auto shortDelay = std::chrono::microseconds (10);
constexpr auto mediumDelay = std::chrono::milliseconds (2);
} // namespace

//==============================================================================
class RecursiveSpinLockTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        counter = 0;
        recursionDepth = 0;
    }

    RecursiveSpinLock recursiveSpinLock;
    std::atomic<int> counter { 0 };
    std::atomic<int> recursionDepth { 0 };
};

//==============================================================================
TEST_F (RecursiveSpinLockTests, BasicLockUnlock)
{
    // Test basic lock/unlock functionality
    recursiveSpinLock.enter();
    EXPECT_TRUE (true); // If we get here, enter() worked
    recursiveSpinLock.exit();
    EXPECT_TRUE (true); // If we get here, exit() worked
}

TEST_F (RecursiveSpinLockTests, TryEnterSuccess)
{
    // Test tryEnter when lock is available
    EXPECT_TRUE (recursiveSpinLock.tryEnter());
    recursiveSpinLock.exit();
}

TEST_F (RecursiveSpinLockTests, RecursiveLockingDeep)
{
    // Test deep recursive locking
    const int depth = 100;

    // Acquire lock many times
    for (int i = 0; i < depth; ++i)
    {
        recursiveSpinLock.enter();
    }

    // Same thread should still be able to acquire more
    EXPECT_TRUE (recursiveSpinLock.tryEnter());
    recursiveSpinLock.exit(); // Clean up the extra tryEnter

    // Release all but one
    for (int i = 0; i < depth - 1; ++i)
    {
        recursiveSpinLock.exit();
        EXPECT_TRUE (recursiveSpinLock.tryEnter()); // Same thread can still acquire
        recursiveSpinLock.exit();                   // Clean up the tryEnter
    }

    // Release the last one
    recursiveSpinLock.exit();
    EXPECT_TRUE (recursiveSpinLock.tryEnter()); // Now should be unlocked
    recursiveSpinLock.exit();
}

TEST_F (RecursiveSpinLockTests, TryEnterRecursiveMany)
{
    // Test many recursive tryEnter calls
    const int depth = 50;

    for (int i = 0; i < depth; ++i)
    {
        EXPECT_TRUE (recursiveSpinLock.tryEnter());
    }

    // All should succeed for same thread
    for (int i = 0; i < depth; ++i)
    {
        recursiveSpinLock.exit();
    }

    EXPECT_TRUE (recursiveSpinLock.tryEnter());
    recursiveSpinLock.exit();
}

TEST_F (RecursiveSpinLockTests, ScopedLockNested)
{
    // Test deeply nested scoped locks
    std::function<void (int)> nestedLocking;
    nestedLocking = [&] (int depth) -> void
    {
        RecursiveSpinLock::ScopedLockType lock (recursiveSpinLock);
        ++counter;

        if (depth > 0)
        {
            // Recursive call with another scoped lock
            RecursiveSpinLock::ScopedLockType innerLock (recursiveSpinLock);
            ++counter;

            if (depth > 1)
                nestedLocking (depth - 2); // Recurse
        }
    };

    nestedLocking (10);

    // Should be unlocked after all scoped locks destroyed
    EXPECT_TRUE (recursiveSpinLock.tryEnter());
    recursiveSpinLock.exit();

    EXPECT_GT (counter.load(), 0);
}

TEST_F (RecursiveSpinLockTests, RecursiveFunctionSimulation)
{
    // Simulate a recursive algorithm that needs locking
    std::function<int (int)> fibonacci = [&] (int n) -> int
    {
        RecursiveSpinLock::ScopedLockType lock (recursiveSpinLock);
        ++counter; // Count function calls

        if (n <= 1)
            return n;

        return fibonacci (n - 1) + fibonacci (n - 2);
    };

    int result = fibonacci (5);
    EXPECT_EQ (result, 5);         // fibonacci(5) = 5
    EXPECT_GT (counter.load(), 5); // Should have been called multiple times
}

TEST_F (RecursiveSpinLockTests, MixedLockingPatterns)
{
    // Test mixing different locking methods
    recursiveSpinLock.enter();
    {
        RecursiveSpinLock::ScopedLockType scopedLock (recursiveSpinLock);
        EXPECT_TRUE (recursiveSpinLock.tryEnter());
        {
            RecursiveSpinLock::ScopedTryLockType tryLock (recursiveSpinLock);
            EXPECT_TRUE (tryLock.isLocked());
            recursiveSpinLock.enter(); // Mix in another enter
            recursiveSpinLock.exit();
        }
        recursiveSpinLock.exit(); // Match the tryEnter
    }
    recursiveSpinLock.exit(); // Match the initial enter

    EXPECT_TRUE (recursiveSpinLock.tryEnter());
    recursiveSpinLock.exit();
}

TEST_F (RecursiveSpinLockTests, MultiThreadedRecursive)
{
    // Test multiple threads each doing recursive locking
    std::vector<std::thread> threads;
    std::atomic<int> totalRecursions { 0 };

    for (int i = 0; i < numThreads; ++i)
    {
        threads.emplace_back ([this, &totalRecursions]()
        {
            for (int j = 0; j < iterationsPerThread; ++j)
            {
                // Each thread does recursive locking
                RecursiveSpinLock::ScopedLockType lock1 (recursiveSpinLock);
                ++counter;
                {
                    RecursiveSpinLock::ScopedLockType lock2 (recursiveSpinLock);
                    ++counter;
                    {
                        RecursiveSpinLock::ScopedLockType lock3 (recursiveSpinLock);
                        ++counter;
                        ++totalRecursions;
                    }
                }
            }
        });
    }

    for (auto& thread : threads)
        thread.join();

    EXPECT_EQ (totalRecursions.load(), numThreads * iterationsPerThread);
    EXPECT_EQ (counter.load(), numThreads * iterationsPerThread * 3);
}

TEST_F (RecursiveSpinLockTests, ThreadContention)
{
    // Test that different threads still block each other
    std::atomic<bool> thread1HasLock { false };
    std::atomic<bool> thread2Blocked { false };
    std::atomic<bool> shouldExit { false };
    std::atomic<int> successfulAcquisitions { 0 };

    std::thread thread1 ([&]()
    {
        recursiveSpinLock.enter();
        recursiveSpinLock.enter(); // Double lock from same thread
        thread1HasLock = true;

        // Hold lock for a bit
        std::this_thread::sleep_for (mediumDelay);

        recursiveSpinLock.exit();
        recursiveSpinLock.exit();
        thread1HasLock = false;
    });

    std::thread thread2 ([&]()
    {
        // Wait for thread1 to acquire lock
        while (! thread1HasLock.load())
            std::this_thread::sleep_for (shortDelay);

        // Try to acquire - should fail since different thread
        if (! recursiveSpinLock.tryEnter())
        {
            thread2Blocked = true;
        }
        else
        {
            // This shouldn't happen, but clean up if it does
            recursiveSpinLock.exit();
        }

        // Wait for thread1 to release, then acquire
        while (thread1HasLock.load())
            std::this_thread::sleep_for (shortDelay);

        // Now should be able to acquire
        if (recursiveSpinLock.tryEnter())
        {
            ++successfulAcquisitions;
            recursiveSpinLock.exit();
        }
    });

    thread1.join();
    thread2.join();

    EXPECT_TRUE (thread2Blocked.load());          // Thread2 should have been blocked initially
    EXPECT_EQ (successfulAcquisitions.load(), 1); // Thread2 should acquire after thread1 releases
}

TEST_F (RecursiveSpinLockTests, ScopedTryLockRecursive)
{
    // Test scoped try-lock with recursion
    {
        RecursiveSpinLock::ScopedTryLockType tryLock1 (recursiveSpinLock);
        EXPECT_TRUE (tryLock1.isLocked());

        {
            RecursiveSpinLock::ScopedTryLockType tryLock2 (recursiveSpinLock);
            EXPECT_TRUE (tryLock2.isLocked()); // Should succeed for same thread

            {
                RecursiveSpinLock::ScopedTryLockType tryLock3 (recursiveSpinLock);
                EXPECT_TRUE (tryLock3.isLocked()); // Should succeed for same thread
            }
        }
    }

    // All locks should be released
    EXPECT_TRUE (recursiveSpinLock.tryEnter());
    recursiveSpinLock.exit();
}

TEST_F (RecursiveSpinLockTests, ExceptionSafetyDeep)
{
    // Test exception safety with deep nesting
    bool exceptionCaught = false;

    try
    {
        RecursiveSpinLock::ScopedLockType lock1 (recursiveSpinLock);
        recursiveSpinLock.enter(); // Manual lock
        {
            RecursiveSpinLock::ScopedLockType lock2 (recursiveSpinLock);
            {
                RecursiveSpinLock::ScopedLockType lock3 (recursiveSpinLock);
                // For recursive locks, same thread can always acquire
                EXPECT_TRUE (recursiveSpinLock.tryEnter()); // Same thread should succeed
                recursiveSpinLock.exit();                   // Clean up the tryEnter
                throw std::runtime_error ("Deep exception");
            }
        }
        recursiveSpinLock.exit(); // This should not be reached
    }
    catch (const std::exception&)
    {
        exceptionCaught = true;
        // Manual lock still needs to be released
        recursiveSpinLock.exit();
    }

    EXPECT_TRUE (exceptionCaught);
    EXPECT_TRUE (recursiveSpinLock.tryEnter()); // Should be fully unlocked
    recursiveSpinLock.exit();
}

TEST_F (RecursiveSpinLockTests, StressTestRecursion)
{
    // Stress test with high recursion depth
#if YUP_WASM
    const int maxDepth = 20;
#else
    const int maxDepth = 1000;
#endif
    std::atomic<int> maxReached { 0 };

    std::function<void (int)> deepRecursion = [&] (int depth)
    {
        if (depth >= maxDepth)
        {
            maxReached = std::max (maxReached.load(), depth);
            return;
        }

        RecursiveSpinLock::ScopedLockType lock (recursiveSpinLock);
        ++counter;
        deepRecursion (depth + 1);
    };

    deepRecursion (0);

    EXPECT_EQ (maxReached.load(), maxDepth);
    EXPECT_EQ (counter.load(), maxDepth);

    // Should be unlocked
    EXPECT_TRUE (recursiveSpinLock.tryEnter());
    recursiveSpinLock.exit();
}

TEST_F (RecursiveSpinLockTests, PerformanceComparison)
{
    // Basic performance test for recursive operations
    const int iterations = 1000;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i)
    {
        RecursiveSpinLock::ScopedLockType lock1 (recursiveSpinLock);
        {
            RecursiveSpinLock::ScopedLockType lock2 (recursiveSpinLock);
            {
                RecursiveSpinLock::ScopedLockType lock3 (recursiveSpinLock);
                ++counter;
            }
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds> (end - start);

    EXPECT_EQ (counter.load(), iterations);
    EXPECT_LT (duration.count(), 1000); // Should complete reasonably quickly
}
