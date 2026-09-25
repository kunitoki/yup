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

#if YUP_ENABLE_ALLOCATION_HOOKS

namespace yup::test
{

/** Counts ordinary new/delete and HeapBlock growth on the calling thread.
    Construct/register outside measurement. This does not intercept arbitrary
    malloc calls or over-aligned C++ allocations. */
class YdspAllocationCounter : private AllocationHooks::Listener
{
public:
    YdspAllocationCounter()
    {
        AllocationHooks::getForCurrentThread().addListener (this);
    }

    ~YdspAllocationCounter() override
    {
        active = false;
        AllocationHooks::getForCurrentThread().removeListener (this);
    }

    void start() noexcept
    {
        count = 0;
        active = true;
    }

    size_t stop() noexcept
    {
        active = false;
        return count;
    }

private:
    void newOrDeleteCalled() noexcept override
    {
        if (active)
            ++count;
    }

    size_t count = 0;
    bool active = false;
};

} // namespace yup::test

#endif
