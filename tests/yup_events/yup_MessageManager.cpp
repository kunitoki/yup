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

#include <yup_events/yup_events.h>

using namespace yup;

class MessageManagerTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mm = MessageManager::getInstance();
        jassert (mm != nullptr);
    }

    void TearDown() override
    {
    }

    void runDispatchLoopUntil (int millisecondsToRunFor = 10)
    {
#if YUP_MODAL_LOOPS_PERMITTED
        mm->runDispatchLoopUntil (millisecondsToRunFor);
#endif
    }

    MessageManager* mm = nullptr;
};

#if 0

TEST_F (MessageManagerTests, Existence)
{
    EXPECT_NE (MessageManager::getInstanceWithoutCreating(), nullptr);
    EXPECT_NE (MessageManager::getInstance(), nullptr);

    EXPECT_FALSE (mm->hasStopMessageBeenSent());

    EXPECT_TRUE (mm->isThisTheMessageThread());
    EXPECT_TRUE (MessageManager::existsAndIsCurrentThread());
    EXPECT_TRUE (mm->currentThreadHasLockedMessageManager());
    EXPECT_TRUE (MessageManager::existsAndIsLockedByCurrentThread());

#if ! YUP_EMSCRIPTEN
    auto messageThreadId = mm->getCurrentMessageThread();

    auto t = std::thread ([messageThreadId]
    {
        auto mmt = MessageManager::getInstance();
        EXPECT_EQ (messageThreadId, mmt->getCurrentMessageThread());

        EXPECT_FALSE (mmt->isThisTheMessageThread());
        EXPECT_FALSE (MessageManager::existsAndIsCurrentThread());
        EXPECT_FALSE (mmt->currentThreadHasLockedMessageManager());
        EXPECT_FALSE (MessageManager::existsAndIsLockedByCurrentThread());
    });

    if (t.joinable())
        t.join();
#endif
}

#if YUP_MODAL_LOOPS_PERMITTED
TEST_F (MessageManagerTests, CallAsync)
{
    bool called = false;
    mm->callAsync ([&]
    {
        called = true;
    });

    runDispatchLoopUntil();

    EXPECT_TRUE (called);
}

#if ! YUP_EMSCRIPTEN
TEST_F (MessageManagerTests, DISABLED_CallFunctionOnMessageThread)
{
    int called = 0;

    auto t = std::thread ([&]
    {
        // clang-format off
        auto result = mm->callFunctionOnMessageThread (+[] (void* data) -> void*
        {
            *reinterpret_cast<int*> (data) = 42;
            return nullptr;
        }, (void*) &called);
        // clang-format on

        EXPECT_EQ (result, nullptr);
    });

    runDispatchLoopUntil();

    if (t.joinable())
        t.join();

    EXPECT_EQ (called, 42);
}
#endif

TEST_F (MessageManagerTests, BroadcastMessage)
{
    struct Listener : ActionListener
    {
        String valueCalled;

        void actionListenerCallback (const String& message) override
        {
            valueCalled = message;
        }
    } listener;

    mm->registerBroadcastListener (&listener);
    mm->deliverBroadcastMessage ("xyz");
    EXPECT_TRUE (listener.valueCalled.isEmpty());

    runDispatchLoopUntil();
    EXPECT_EQ (listener.valueCalled, "xyz");

    mm->deregisterBroadcastListener (&listener);
    mm->deliverBroadcastMessage ("123");

    runDispatchLoopUntil();
    EXPECT_EQ (listener.valueCalled, "xyz");
}

TEST_F (MessageManagerTests, PostMessage)
{
    String valueCalled;

    struct Message : MessageManager::MessageBase
    {
        String& data;

        Message (String& data)
            : data (data)
        {
        }

        void messageCallback() override
        {
            data = "xyz";
        }
    };

    (new Message (valueCalled))->post();

    runDispatchLoopUntil();

    EXPECT_EQ (valueCalled, "xyz");
}

#endif

#endif
