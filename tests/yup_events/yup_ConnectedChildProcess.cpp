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

#include <yup_events/yup_events.h>

#if ! YUP_WASM

using namespace yup;

class ConnectedChildProcessTests : public ::testing::Test
{
protected:
    static constexpr uint32 coordinatorWorkerMagic = 0x712baf04;
    static constexpr int connectionTimeoutMs = 5000;

    static MemoryBlock makeSpecialMessage (const char* type) { return MemoryBlock (type, 8); }

    static MemoryBlock makeTextMessage (const char* text) { return MemoryBlock (text, std::strlen (text)); }

    static bool isPing (const MemoryBlock& mb) { return mb.getSize() == 8 && mb.matches ("__ipc_p_", 8); }

    // Plays the coordinator role in-process, speaking the coordinator/worker wire protocol
    class FakeCoordinatorConnection : public InterprocessConnection
    {
    public:
        FakeCoordinatorConnection()
            : InterprocessConnection (false, coordinatorWorkerMagic)
        {
        }

        ~FakeCoordinatorConnection() override { disconnect (2000); }

        void connectionMade() override {}

        void connectionLost() override {}

        void messageReceived (const MemoryBlock& message) override
        {
            if (isPing (message))
            {
                pingEvent.signal();
                return;
            }

            const ScopedLock sl (lock);
            messages.add (message);
            messageEvent.signal();
        }

        WaitableEvent pingEvent { true }, messageEvent;
        CriticalSection lock;
        Array<MemoryBlock> messages;
    };

    class TestWorker : public ChildProcessWorker
    {
    public:
        void handleMessageFromCoordinator (const MemoryBlock& mb) override
        {
            ChildProcessWorker::handleMessageFromCoordinator (mb);

            const ScopedLock sl (lock);
            messages.add (mb);
            messageEvent.signal();
        }

        void handleConnectionMade() override
        {
            ChildProcessWorker::handleConnectionMade();

            madeCount.fetch_add (1);
            madeEvent.signal();
        }

        void handleConnectionLost() override
        {
            ChildProcessWorker::handleConnectionLost();

            lostCount.fetch_add (1);

            if (MessageManager::existsAndIsCurrentThread())
                lostOnMessageThreadCount.fetch_add (1);
        }

        std::atomic<int> madeCount { 0 }, lostCount { 0 }, lostOnMessageThreadCount { 0 };
        WaitableEvent madeEvent { true }, messageEvent;
        CriticalSection lock;
        Array<MemoryBlock> messages;
    };

    class TestCoordinator : public ChildProcessCoordinator
    {
    public:
        void handleConnectionLost() override
        {
            ChildProcessCoordinator::handleConnectionLost();

            lostCount.fetch_add (1);

            if (MessageManager::existsAndIsCurrentThread())
                lostOnMessageThreadCount.fetch_add (1);
        }

        std::atomic<int> lostCount { 0 }, lostOnMessageThreadCount { 0 };
    };

    void SetUp() override
    {
        pipeName = "YUPChildProc" + String::toHexString (Random::getSystemRandom().nextInt());
    }

    String commandLineFor (const String& name) const { return "  --yuptest:" + name + " --other-arg"; }

#if YUP_MODAL_LOOPS_PERMITTED
    template <typename Predicate>
    static bool dispatchUntil (Predicate&& done, int timeoutMs = 3000)
    {
        const auto endTime = Time::getMillisecondCounter() + (uint32) timeoutMs;

        while (! done() && Time::getMillisecondCounter() < endTime)
            MessageManager::getInstance()->runDispatchLoopUntil (10);

        return done();
    }
#endif

    String pipeName;
};

//==============================================================================
TEST_F (ConnectedChildProcessTests, DefaultCallbacksAndSendingWithoutConnectionDoNothing)
{
    const auto message = makeTextMessage ("data");

    ChildProcessWorker worker;
    worker.handleConnectionMade();
    worker.handleConnectionLost();
    worker.handleMessageFromCoordinator (message);
    EXPECT_FALSE (worker.sendMessageToCoordinator (message));

    ChildProcessCoordinator coordinator;
    coordinator.handleConnectionLost();
    coordinator.handleMessageFromWorker (message);
    EXPECT_FALSE (coordinator.sendMessageToWorker (message));
    EXPECT_FALSE (coordinator.isWorkerProcessRunning());

    coordinator.killWorkerProcess();
    coordinator.terminateWorkerProcess();
    EXPECT_FALSE (coordinator.isWorkerProcessRunning());
}

TEST_F (ConnectedChildProcessTests, InitialiseFromCommandLineRejectsMismatchedOrEmptyArguments)
{
    TestWorker worker;

    EXPECT_FALSE (worker.initialiseFromCommandLine ("--other:" + pipeName, "yuptest"));
    EXPECT_FALSE (worker.initialiseFromCommandLine ("--yuptest:", "yuptest"));
    EXPECT_FALSE (worker.initialiseFromCommandLine ("--yuptest: " + pipeName, "yuptest"));
    EXPECT_FALSE (worker.initialiseFromCommandLine ("", "yuptest"));

    EXPECT_EQ (0, worker.madeCount.load());
}

TEST_F (ConnectedChildProcessTests, InitialiseFromCommandLineFailsWhenPipeDoesNotExist)
{
    TestWorker worker;

    EXPECT_FALSE (worker.initialiseFromCommandLine (commandLineFor (pipeName), "yuptest", connectionTimeoutMs));
    EXPECT_FALSE (worker.sendMessageToCoordinator (makeTextMessage ("data")));
    EXPECT_EQ (0, worker.lostCount.load());
}

TEST_F (ConnectedChildProcessTests, WorkerExchangesMessagesWithCoordinator)
{
    FakeCoordinatorConnection coordinator;
    ASSERT_TRUE (coordinator.createPipe (pipeName, connectionTimeoutMs, true));

    TestWorker worker;
    ASSERT_TRUE (worker.initialiseFromCommandLine (commandLineFor (pipeName), "yuptest", connectionTimeoutMs));

    EXPECT_TRUE (coordinator.pingEvent.wait (3000));

    const auto userMessage = makeTextMessage ("hello worker");
    EXPECT_TRUE (coordinator.sendMessage (makeSpecialMessage ("__ipc_p_")));
    EXPECT_TRUE (coordinator.sendMessage (makeSpecialMessage ("__ipc_st")));
    EXPECT_TRUE (coordinator.sendMessage (userMessage));

    EXPECT_TRUE (worker.madeEvent.wait (3000));
    ASSERT_TRUE (worker.messageEvent.wait (3000));

    EXPECT_EQ (1, worker.madeCount.load());
    {
        const ScopedLock sl (worker.lock);
        ASSERT_EQ (1, worker.messages.size());
        EXPECT_EQ (userMessage, worker.messages[0]);
    }

    const auto reply = makeTextMessage ("hello coordinator");
    EXPECT_TRUE (worker.sendMessageToCoordinator (reply));
    ASSERT_TRUE (coordinator.messageEvent.wait (3000));

    const ScopedLock sl (coordinator.lock);
    ASSERT_EQ (1, coordinator.messages.size());
    EXPECT_EQ (reply, coordinator.messages[0]);
}

#if YUP_MODAL_LOOPS_PERMITTED
TEST_F (ConnectedChildProcessTests, WorkerReportsConnectionLostOnKillMessage)
{
    FakeCoordinatorConnection coordinator;
    ASSERT_TRUE (coordinator.createPipe (pipeName, connectionTimeoutMs, true));

    TestWorker worker;
    ASSERT_TRUE (worker.initialiseFromCommandLine (commandLineFor (pipeName), "yuptest", connectionTimeoutMs));
    EXPECT_TRUE (coordinator.pingEvent.wait (3000));

    EXPECT_TRUE (coordinator.sendMessage (makeSpecialMessage ("__ipc_k_")));

    EXPECT_TRUE (dispatchUntil ([&]
    {
        return worker.lostOnMessageThreadCount.load() > 0;
    }));

    const ScopedLock sl (worker.lock);
    EXPECT_TRUE (worker.messages.isEmpty());
}

TEST_F (ConnectedChildProcessTests, WorkerReportsConnectionLostWhenPingTimeoutExpires)
{
    FakeCoordinatorConnection coordinator;
    ASSERT_TRUE (coordinator.createPipe (pipeName, connectionTimeoutMs, true));

    // A timeout below one second leaves a single ping countdown, which expires on the first ping
    TestWorker worker;
    ASSERT_TRUE (worker.initialiseFromCommandLine (commandLineFor (pipeName), "yuptest", 500));

    EXPECT_TRUE (dispatchUntil ([&]
    {
        return worker.lostOnMessageThreadCount.load() > 0;
    }));
}

#if YUP_LINUX || YUP_MAC
TEST_F (ConnectedChildProcessTests, CoordinatorReportsConnectionLostWhenWorkerNeverResponds)
{
    // /bin/sh rejects the worker argument and exits without ever connecting
    const File shell ("/bin/sh");
    ASSERT_TRUE (shell.existsAsFile());

    TestCoordinator coordinator;
    ASSERT_TRUE (coordinator.launchWorkerProcess (shell, "yuptest", 300));

    EXPECT_TRUE (dispatchUntil ([&]
    {
        return coordinator.lostOnMessageThreadCount.load() > 0;
    }));

    coordinator.terminateWorkerProcess();
    EXPECT_FALSE (coordinator.isWorkerProcessRunning());
    EXPECT_FALSE (coordinator.sendMessageToWorker (makeTextMessage ("data")));
}
#endif
#endif

#endif
