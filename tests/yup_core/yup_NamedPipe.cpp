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

#include <yup_core/yup_core.h>

using namespace yup;

namespace
{
struct NamedPipeThread : public Thread
{
    NamedPipeThread (const String& tName, const String& pName, bool shouldCreatePipe, WaitableEvent& completed)
        : Thread (tName)
        , pipeName (pName)
        , workCompleted (completed)
    {
        if (shouldCreatePipe)
            pipe.createNewPipe (pipeName);
        else
            pipe.openExisting (pipeName);
    }

    NamedPipe pipe;
    const String& pipeName;
    WaitableEvent& workCompleted;

    int result = -2;
};

struct SenderThread final : public NamedPipeThread
{
    SenderThread (const String& pName, bool shouldCreatePipe, WaitableEvent& completed, int sData)
        : NamedPipeThread ("NamePipeSender", pName, shouldCreatePipe, completed)
        , sendData (sData)
    {
    }

    ~SenderThread() override
    {
        stopThread (100);
    }

    void run() override
    {
        result = pipe.write (&sendData, sizeof (sendData), 2000);
        workCompleted.signal();
    }

    const int sendData;
};

struct ReceiverThread final : public NamedPipeThread
{
    ReceiverThread (const String& pName, bool shouldCreatePipe, WaitableEvent& completed)
        : NamedPipeThread ("NamePipeSender", pName, shouldCreatePipe, completed)
    {
    }

    ~ReceiverThread() override
    {
        stopThread (100);
    }

    void run() override
    {
        result = pipe.read (&recvData, sizeof (recvData), 2000);
        workCompleted.signal();
    }

    int recvData = -2;
};
} // namespace

class NamedPipeTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        pipeName = "TestPipe" + String ((intptr_t) Thread::getCurrentThreadId());
    }

    String pipeName;
};

TEST_F (NamedPipeTests, PreTestCleanup)
{
    NamedPipe pipe;
    EXPECT_TRUE (pipe.createNewPipe (pipeName, false));
}

TEST_F (NamedPipeTests, CreatePipe)
{
    NamedPipe pipe;
    EXPECT_FALSE (pipe.isOpen());

    EXPECT_TRUE (pipe.createNewPipe (pipeName, true));
    EXPECT_TRUE (pipe.isOpen());

    EXPECT_TRUE (pipe.createNewPipe (pipeName, false));
    EXPECT_TRUE (pipe.isOpen());

    NamedPipe otherPipe;
    EXPECT_FALSE (otherPipe.createNewPipe (pipeName, true));
    EXPECT_FALSE (otherPipe.isOpen());
}

TEST_F (NamedPipeTests, ExistingPipe)
{
    NamedPipe pipe;

    EXPECT_FALSE (pipe.openExisting (pipeName));
    EXPECT_FALSE (pipe.isOpen());

    EXPECT_TRUE (pipe.createNewPipe (pipeName, true));

    NamedPipe otherPipe;
    EXPECT_TRUE (otherPipe.openExisting (pipeName));
    EXPECT_TRUE (otherPipe.isOpen());
}

TEST_F (NamedPipeTests, ReceiveMessageCreatedPipe)
{
    NamedPipe pipe;
    EXPECT_TRUE (pipe.createNewPipe (pipeName, true));

    const int sendData = 4684682;
    WaitableEvent senderFinished;
    SenderThread sender (pipeName, false, senderFinished, sendData);

    sender.startThread();

    int recvData = -1;
    auto bytesRead = pipe.read (&recvData, sizeof (recvData), 2000);

    EXPECT_TRUE (senderFinished.wait (4000));

    EXPECT_EQ (bytesRead, (int) sizeof (recvData));
    EXPECT_EQ (sender.result, (int) sizeof (sendData));
    EXPECT_EQ (recvData, sendData);
}

TEST_F (NamedPipeTests, ReceiveMessageExistingPipe)
{
    const int sendData = 4684682;
    WaitableEvent senderFinished;
    SenderThread sender (pipeName, true, senderFinished, sendData);

    NamedPipe pipe;
    EXPECT_TRUE (pipe.openExisting (pipeName));

    sender.startThread();

    int recvData = -1;
    auto bytesRead = pipe.read (&recvData, sizeof (recvData), 2000);

    EXPECT_TRUE (senderFinished.wait (4000));

    EXPECT_EQ (bytesRead, (int) sizeof (recvData));
    EXPECT_EQ (sender.result, (int) sizeof (sendData));
    EXPECT_EQ (recvData, sendData);
}

TEST_F (NamedPipeTests, SendMessageCreatedPipe)
{
    NamedPipe pipe;
    EXPECT_TRUE (pipe.createNewPipe (pipeName, true));

    const int sendData = 4684682;
    WaitableEvent receiverFinished;
    ReceiverThread receiver (pipeName, false, receiverFinished);

    receiver.startThread();

    auto bytesWritten = pipe.write (&sendData, sizeof (sendData), 2000);

    EXPECT_TRUE (receiverFinished.wait (4000));

    EXPECT_EQ (bytesWritten, (int) sizeof (sendData));
    EXPECT_EQ (receiver.result, (int) sizeof (receiver.recvData));
    EXPECT_EQ (receiver.recvData, sendData);
}

TEST_F (NamedPipeTests, SendMessageExistingPipe)
{
    const int sendData = 4684682;
    WaitableEvent receiverFinished;
    ReceiverThread receiver (pipeName, true, receiverFinished);

    NamedPipe pipe;
    EXPECT_TRUE (pipe.openExisting (pipeName));

    receiver.startThread();

    auto bytesWritten = pipe.write (&sendData, sizeof (sendData), 2000);

    EXPECT_TRUE (receiverFinished.wait (4000));

    EXPECT_EQ (bytesWritten, (int) sizeof (sendData));
    EXPECT_EQ (receiver.result, (int) sizeof (receiver.recvData));
    EXPECT_EQ (receiver.recvData, sendData);
}