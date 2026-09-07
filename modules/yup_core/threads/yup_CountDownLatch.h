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

#pragma once

#include <condition_variable>
#include <mutex>

namespace yup
{

//==============================================================================
/** A synchronization primitive that blocks until a counter reaches zero.

    Each unit of scheduled work calls countDown() when it finishes; the
    counter may also be raised again with addCount() before the pending work
    is dispatched, which is what lets a worker that discovers more work enqueue
    it without the waiter waking early. wait() returns once every counted unit
    has called countDown().

    @tags{Core}
*/
class YUP_API CountDownLatch
{
public:
    //==============================================================================
    /** Creates a latch with the given initial count. */
    explicit CountDownLatch (int initialCount = 0);

    /** Destructor. */
    ~CountDownLatch() = default;

    //==============================================================================
    /** Increases the count by `amount`. Used to book extra work before it is
        dispatched, so the count can only reach zero after that work has been
        accounted for. */
    void addCount (int amount = 1) noexcept;

    /** Decrements the count by one, waking every waiter when it reaches zero.
        Counting below zero is ignored. */
    void countDown() noexcept;

    /** Blocks until the count reaches zero. */
    void wait() const;

    /** Returns the current count. */
    int getCount() const noexcept;

private:
    mutable std::mutex mutex;
    mutable std::condition_variable cv;
    int count;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CountDownLatch)
};

} // namespace yup
