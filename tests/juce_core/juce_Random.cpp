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

TEST (RandomTests, RandomNumbers)
{
    Random& r = Random::getSystemRandom();

    for (int i = 2000; --i >= 0;)
    {
        EXPECT_TRUE (r.nextDouble() >= 0.0 && r.nextDouble() < 1.0);
        EXPECT_TRUE (r.nextFloat() >= 0.0f && r.nextFloat() < 1.0f);
        EXPECT_TRUE (r.nextInt (5) >= 0 && r.nextInt (5) < 5);
        EXPECT_TRUE (r.nextInt (1) == 0);

        int n = r.nextInt (50) + 1;
        EXPECT_TRUE (r.nextInt (n) >= 0 && r.nextInt (n) < n);

        n = r.nextInt (0x7ffffffe) + 1;
        EXPECT_TRUE (r.nextInt (n) >= 0 && r.nextInt (n) < n);
    }
}

TEST (RandomTests, Concurrent)
{
    class FastWaitableEvent
    {
    public:
        void notify() { notified = true; }

        void wait() const
        {
            while (! notified)
            {
            }
        }

    private:
        std::atomic<bool> notified = false;
    };

    class InvokerThread final : private Thread
    {
    public:
        InvokerThread (std::function<void()> fn, FastWaitableEvent& notificationEvent, int numInvocationsToTrigger)
            : Thread ("InvokerThread")
            , invokable (fn)
            , notified (&notificationEvent)
            , numInvocations (numInvocationsToTrigger)
        {
            startThread();
        }

        ~InvokerThread()
        {
            stopThread (-1);
        }

        void waitUntilReady() const
        {
            ready.wait();
        }

    private:
        void run() final
        {
            ready.notify();
            notified->wait();

            for (int i = numInvocations; --i >= 0;)
                invokable();
        }

        std::function<void()> invokable;
        FastWaitableEvent* notified;
        FastWaitableEvent ready;
        int numInvocations;
    };

    constexpr int numberOfInvocationsPerThread = 10000;
    constexpr int numberOfThreads = 100;

    FastWaitableEvent start;

    std::vector<std::unique_ptr<InvokerThread>> threads;
    threads.reserve ((size_t) numberOfThreads);

    auto threadCallback = []
    {
        Random::getSystemRandom().nextInt();
    };

    for (int i = numberOfThreads; --i >= 0;)
    {
        threads.push_back (std::make_unique<InvokerThread> (threadCallback,
                                                            start,
                                                            numberOfInvocationsPerThread));
    }

    for (auto& thread : threads)
        thread->waitUntilReady();

    Thread::sleep (1);
    start.notify();
}
