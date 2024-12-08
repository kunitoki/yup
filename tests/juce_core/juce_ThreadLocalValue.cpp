/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include <gtest/gtest.h>

#include <juce_core/juce_core.h>

using namespace juce;

class ThreadStorage final : public Thread
{
public:
    ThreadStorage()
        : Thread ("ThreadLocalValue Thread")
    {
    }

    void run() override
    {
        sharedThreadLocal.get()->get() = 2;
        auxThreadResult = sharedThreadLocal.get()->get();
    }

    Atomic<int> auxThreadResult;
    Atomic<ThreadLocalValue<int>*> sharedThreadLocal;
};

TEST (ThreadLocalValueTests, ValuesAreThreadLocal)
{
    ThreadLocalValue<int> threadLocal;

    ThreadStorage threadStorage;
    threadStorage.sharedThreadLocal = &threadLocal;
    threadStorage.sharedThreadLocal.get()->get() = 1;
    threadStorage.startThread();
    threadStorage.signalThreadShouldExit();
    threadStorage.waitForThreadToExit (-1);

    Atomic<int> mainThreadResult = threadStorage.sharedThreadLocal.get()->get();

    EXPECT_EQ (mainThreadResult.get(), 1);
    EXPECT_EQ (threadStorage.auxThreadResult.get(), 2);
}

TEST (ThreadLocalValueTests, ValuesArePerInstance)
{
    ThreadLocalValue<int> a, b;

    a.get() = 1;
    b.get() = 2;

    EXPECT_EQ (a.get(), 1);
    EXPECT_EQ (b.get(), 2);
}
