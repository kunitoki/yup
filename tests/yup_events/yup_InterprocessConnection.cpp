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

namespace
{

class TestConnection : public InterprocessConnection
{
public:
    TestConnection()
        : InterprocessConnection (false) // callbacks on connection thread, no message loop needed
    {
    }

    ~TestConnection() override
    {
        disconnect (2000);
    }

    void connectionMade() override
    {
        madeCount.fetch_add (1);
        madeEvent.signal();
    }

    void connectionLost() override
    {
        lostCount.fetch_add (1);
        lostEvent.signal();
    }

    void messageReceived (const MemoryBlock& message) override
    {
        const ScopedLock sl (messagesLock);
        receivedMessages.add (message);
        messageEvent.signal();
    }

    bool waitForConnection (int timeoutMs = 5000) { return madeEvent.wait (timeoutMs); }

    bool waitForMessage (int timeoutMs = 5000) { return messageEvent.wait (timeoutMs); }

    bool waitForDisconnect (int timeoutMs = 5000) { return lostEvent.wait (timeoutMs); }

    std::atomic<int> madeCount { 0 };
    std::atomic<int> lostCount { 0 };
    WaitableEvent madeEvent, lostEvent, messageEvent;

    CriticalSection messagesLock;
    Array<MemoryBlock> receivedMessages;
};

class TestConnectionServer : public InterprocessConnectionServer
{
public:
    ~TestConnectionServer() override
    {
        stop();
        const ScopedLock sl (connectionsLock);
        for (auto* conn : serverConnections)
        {
            conn->disconnect (2000);
            delete conn;
        }
        serverConnections.clearQuick();
    }

    InterprocessConnection* createConnectionObject() override
    {
        auto* conn = new TestConnection();
        {
            const ScopedLock sl (connectionsLock);
            serverConnections.add (conn);
        }
        newConnectionEvent.signal();
        return conn;
    }

    bool waitForServerConnection (int timeoutMs = 5000)
    {
        return newConnectionEvent.wait (timeoutMs);
    }

    TestConnection* getLastConnection()
    {
        const ScopedLock sl (connectionsLock);
        return serverConnections.isEmpty() ? nullptr : serverConnections.getLast();
    }

    WaitableEvent newConnectionEvent;
    CriticalSection connectionsLock;
    Array<TestConnection*> serverConnections;
};

} // namespace

// =============================================================================
// InterprocessConnection via named pipe
// =============================================================================

class InterprocessConnectionPipeTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        pipeName = "YUP_IPC_Test_" + String::toHexString (Random::getSystemRandom().nextInt());
    }

    String pipeName;
};

TEST_F (InterprocessConnectionPipeTests, CreateAndConnectPipe)
{
    TestConnection creator;
    ASSERT_TRUE (creator.createPipe (pipeName, 2000, true));

    TestConnection connector;
    ASSERT_TRUE (connector.connectToPipe (pipeName, 2000));

    EXPECT_TRUE (creator.waitForConnection());
    EXPECT_TRUE (connector.waitForConnection());

    EXPECT_TRUE (creator.isConnected());
    EXPECT_TRUE (connector.isConnected());

    EXPECT_EQ (creator.madeCount.load(), 1);
    EXPECT_EQ (connector.madeCount.load(), 1);
}

TEST_F (InterprocessConnectionPipeTests, SendMessageFromConnectorToCreator)
{
    TestConnection creator;
    ASSERT_TRUE (creator.createPipe (pipeName, 2000, true));

    TestConnection connector;
    ASSERT_TRUE (connector.connectToPipe (pipeName, 2000));

    ASSERT_TRUE (creator.waitForConnection());
    ASSERT_TRUE (connector.waitForConnection());

    const char* text = "hello pipe";
    MemoryBlock message (text, strlen (text));
    EXPECT_TRUE (connector.sendMessage (message));

    EXPECT_TRUE (creator.waitForMessage());

    const ScopedLock sl (creator.messagesLock);
    ASSERT_EQ (creator.receivedMessages.size(), 1);
    EXPECT_EQ (creator.receivedMessages[0], message);
}

TEST_F (InterprocessConnectionPipeTests, SendMessageFromCreatorToConnector)
{
    TestConnection creator;
    ASSERT_TRUE (creator.createPipe (pipeName, 2000, true));

    TestConnection connector;
    ASSERT_TRUE (connector.connectToPipe (pipeName, 2000));

    ASSERT_TRUE (creator.waitForConnection());
    ASSERT_TRUE (connector.waitForConnection());

    const char* text = "reply pipe";
    MemoryBlock message (text, strlen (text));
    EXPECT_TRUE (creator.sendMessage (message));

    EXPECT_TRUE (connector.waitForMessage());

    const ScopedLock sl (connector.messagesLock);
    ASSERT_EQ (connector.receivedMessages.size(), 1);
    EXPECT_EQ (connector.receivedMessages[0], message);
}

TEST_F (InterprocessConnectionPipeTests, DisconnectingConnectorNotifiesCreator)
{
    TestConnection creator;
    ASSERT_TRUE (creator.createPipe (pipeName, 2000, true));

    TestConnection connector;
    ASSERT_TRUE (connector.connectToPipe (pipeName, 2000));

    ASSERT_TRUE (creator.waitForConnection());
    ASSERT_TRUE (connector.waitForConnection());

    connector.disconnect (2000);

    EXPECT_TRUE (creator.waitForDisconnect());
    EXPECT_EQ (creator.lostCount.load(), 1);
}

TEST_F (InterprocessConnectionPipeTests, SendMessageIsNotConnectedWhenNoPipe)
{
    TestConnection connection;
    EXPECT_FALSE (connection.isConnected());

    MemoryBlock message ("data", 4);
    EXPECT_FALSE (connection.sendMessage (message));
}

TEST_F (InterprocessConnectionPipeTests, CreatePipeWithMustNotExistFailsOnDuplicate)
{
    TestConnection first;
    ASSERT_TRUE (first.createPipe (pipeName, 2000, true));

    TestConnection second;
    EXPECT_FALSE (second.createPipe (pipeName, 2000, true));
}

// =============================================================================
// InterprocessConnection via socket server
// =============================================================================

class InterprocessConnectionSocketTests : public ::testing::Test
{
protected:
};

TEST_F (InterprocessConnectionSocketTests, ServerListensOnValidPort)
{
    TestConnectionServer server;
    ASSERT_TRUE (server.beginWaitingForSocket (0));
    EXPECT_NE (server.getBoundPort(), -1);
    EXPECT_NE (server.getBoundPort(), 0);
}

TEST_F (InterprocessConnectionSocketTests, ClientConnectsToServer)
{
    TestConnectionServer server;
    ASSERT_TRUE (server.beginWaitingForSocket (0));
    const int port = server.getBoundPort();

    TestConnection client;
    ASSERT_TRUE (client.connectToSocket ("127.0.0.1", port, 3000));

    EXPECT_TRUE (client.waitForConnection());
    EXPECT_TRUE (server.waitForServerConnection());

    EXPECT_TRUE (client.isConnected());

    auto* serverConn = server.getLastConnection();
    ASSERT_NE (serverConn, nullptr);
    EXPECT_TRUE (serverConn->waitForConnection());
    EXPECT_TRUE (serverConn->isConnected());
}

TEST_F (InterprocessConnectionSocketTests, SendMessageClientToServerConnection)
{
    TestConnectionServer server;
    ASSERT_TRUE (server.beginWaitingForSocket (0));
    const int port = server.getBoundPort();

    TestConnection client;
    ASSERT_TRUE (client.connectToSocket ("127.0.0.1", port, 3000));

    ASSERT_TRUE (client.waitForConnection());
    ASSERT_TRUE (server.waitForServerConnection());

    auto* serverConn = server.getLastConnection();
    ASSERT_NE (serverConn, nullptr);
    ASSERT_TRUE (serverConn->waitForConnection());

    const char* text = "hello socket";
    MemoryBlock message (text, strlen (text));
    EXPECT_TRUE (client.sendMessage (message));

    EXPECT_TRUE (serverConn->waitForMessage());

    const ScopedLock sl (serverConn->messagesLock);
    ASSERT_EQ (serverConn->receivedMessages.size(), 1);
    EXPECT_EQ (serverConn->receivedMessages[0], message);
}

TEST_F (InterprocessConnectionSocketTests, SendMessageServerConnectionToClient)
{
    TestConnectionServer server;
    ASSERT_TRUE (server.beginWaitingForSocket (0));
    const int port = server.getBoundPort();

    TestConnection client;
    ASSERT_TRUE (client.connectToSocket ("127.0.0.1", port, 3000));

    ASSERT_TRUE (client.waitForConnection());
    ASSERT_TRUE (server.waitForServerConnection());

    auto* serverConn = server.getLastConnection();
    ASSERT_NE (serverConn, nullptr);
    ASSERT_TRUE (serverConn->waitForConnection());

    const char* text = "reply socket";
    MemoryBlock message (text, strlen (text));
    EXPECT_TRUE (serverConn->sendMessage (message));

    EXPECT_TRUE (client.waitForMessage());

    const ScopedLock sl (client.messagesLock);
    ASSERT_EQ (client.receivedMessages.size(), 1);
    EXPECT_EQ (client.receivedMessages[0], message);
}

TEST_F (InterprocessConnectionSocketTests, DisconnectingClientNotifiesServer)
{
    TestConnectionServer server;
    ASSERT_TRUE (server.beginWaitingForSocket (0));
    const int port = server.getBoundPort();

    TestConnection client;
    ASSERT_TRUE (client.connectToSocket ("127.0.0.1", port, 3000));

    ASSERT_TRUE (client.waitForConnection());
    ASSERT_TRUE (server.waitForServerConnection());

    auto* serverConn = server.getLastConnection();
    ASSERT_NE (serverConn, nullptr);
    ASSERT_TRUE (serverConn->waitForConnection());

    client.disconnect (2000);

    EXPECT_TRUE (serverConn->waitForDisconnect());
    EXPECT_EQ (serverConn->lostCount.load(), 1);
}

TEST_F (InterprocessConnectionSocketTests, ConnectToInvalidPortFails)
{
    TestConnection client;
    // Port 1 is typically reserved and not listening
    EXPECT_FALSE (client.connectToSocket ("127.0.0.1", 1, 500));
    EXPECT_FALSE (client.isConnected());
}

TEST_F (InterprocessConnectionSocketTests, GetConnectedHostName)
{
    TestConnectionServer server;
    ASSERT_TRUE (server.beginWaitingForSocket (0));
    const int port = server.getBoundPort();

    TestConnection client;
    ASSERT_TRUE (client.connectToSocket ("127.0.0.1", port, 3000));
    ASSERT_TRUE (client.waitForConnection());

    EXPECT_FALSE (client.getConnectedHostName().isEmpty());
}

#endif
