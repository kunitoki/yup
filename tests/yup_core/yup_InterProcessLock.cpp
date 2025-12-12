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

TEST (InterProcessLockTests, BasicLockUnlock)
{
    InterProcessLock lock ("YUP_TEST_LOCK");

    // Should be able to enter lock
    EXPECT_TRUE (lock.enter (1000));

    // Should be able to exit lock
    EXPECT_NO_THROW (lock.exit());
}

TEST (InterProcessLockTests, ReentrantLocking)
{
    InterProcessLock lock ("YUP_TEST_REENTRANT_LOCK");

    // Should be able to enter multiple times from same process
    EXPECT_TRUE (lock.enter (1000));
    EXPECT_TRUE (lock.enter (1000));
    EXPECT_TRUE (lock.enter (1000));

    // Should exit same number of times
    EXPECT_NO_THROW (lock.exit());
    EXPECT_NO_THROW (lock.exit());
    EXPECT_NO_THROW (lock.exit());
}

TEST (InterProcessLockTests, ImmediateTimeout)
{
    InterProcessLock lock1 ("YUP_TEST_TIMEOUT_LOCK");

    // First lock should succeed immediately
    EXPECT_TRUE (lock1.enter (0));

    // Cleanup
    lock1.exit();
}

TEST (InterProcessLockTests, WithTimeout)
{
    InterProcessLock lock ("YUP_TEST_TIMED_LOCK");

    // Should succeed with timeout
    EXPECT_TRUE (lock.enter (500));

    // Test entering again (should work - reentrant)
    EXPECT_TRUE (lock.enter (500));

    // Cleanup
    lock.exit();
    lock.exit();
}

TEST (InterProcessLockTests, DifferentLockNames)
{
    InterProcessLock lock1 ("YUP_TEST_LOCK_A");
    InterProcessLock lock2 ("YUP_TEST_LOCK_B");

    // Different locks should not interfere
    EXPECT_TRUE (lock1.enter (100));
    EXPECT_TRUE (lock2.enter (100));

    lock1.exit();
    lock2.exit();
}

TEST (InterProcessLockTests, LockScope)
{
    InterProcessLock lock ("YUP_TEST_SCOPED_LOCK");

    {
        InterProcessLock::ScopedLockType scopedLock (lock);
        // Lock should be held here
        EXPECT_TRUE (true); // Just verify no crash
    }
    // Lock should be released here

    // Should be able to acquire again
    EXPECT_TRUE (lock.enter (100));
    lock.exit();
}
