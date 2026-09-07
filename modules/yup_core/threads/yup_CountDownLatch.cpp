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

#include "yup_CountDownLatch.h"

namespace yup
{

CountDownLatch::CountDownLatch (int initialCount)
    : count (jmax (0, initialCount))
{
}

void CountDownLatch::addCount (int amount) noexcept
{
    if (amount <= 0)
        return;

    std::lock_guard lock (mutex);
    count += amount;
}

void CountDownLatch::countDown() noexcept
{
    std::lock_guard lock (mutex);

    if (count > 0)
        --count;

    if (count == 0)
        cv.notify_all();
}

void CountDownLatch::wait() const
{
    std::unique_lock lock (mutex);
    cv.wait (lock, [&]
    {
        return count == 0;
    });
}

int CountDownLatch::getCount() const noexcept
{
    std::lock_guard lock (mutex);
    return count;
}

} // namespace yup
