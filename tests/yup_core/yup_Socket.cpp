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

// =============================================================================
// SocketOptions Tests
// =============================================================================

TEST (SocketOptionsTests, DefaultConstructionHasNoBufferSizes)
{
    SocketOptions opts;
    EXPECT_FALSE (opts.getReceiveBufferSize().has_value());
    EXPECT_FALSE (opts.getSendBufferSize().has_value());
}

TEST (SocketOptionsTests, WithReceiveBufferSize)
{
    auto opts = SocketOptions {}.withReceiveBufferSize (131072);
    EXPECT_TRUE (opts.getReceiveBufferSize().has_value());
    EXPECT_EQ (*opts.getReceiveBufferSize(), 131072);
    EXPECT_FALSE (opts.getSendBufferSize().has_value());
}

TEST (SocketOptionsTests, WithSendBufferSize)
{
    auto opts = SocketOptions {}.withSendBufferSize (65536);
    EXPECT_FALSE (opts.getReceiveBufferSize().has_value());
    EXPECT_TRUE (opts.getSendBufferSize().has_value());
    EXPECT_EQ (*opts.getSendBufferSize(), 65536);
}

TEST (SocketOptionsTests, WithBothBufferSizesChained)
{
    auto opts = SocketOptions {}
                    .withReceiveBufferSize (131072)
                    .withSendBufferSize (65536);

    EXPECT_EQ (*opts.getReceiveBufferSize(), 131072);
    EXPECT_EQ (*opts.getSendBufferSize(), 65536);
}

TEST (SocketOptionsTests, WithImmutableStyleDoesNotModifyOriginal)
{
    SocketOptions original;
    auto modified = original.withReceiveBufferSize (65536);

    EXPECT_FALSE (original.getReceiveBufferSize().has_value());
    EXPECT_TRUE (modified.getReceiveBufferSize().has_value());
}

#if ! YUP_WASM

// =============================================================================
// StreamingSocket Tests
// =============================================================================

TEST (StreamingSocketTests, DefaultConstruction)
{
    StreamingSocket socket;
    EXPECT_FALSE (socket.isConnected());
    EXPECT_TRUE (socket.getHostName().isEmpty());
    EXPECT_EQ (socket.getPort(), 0);
    EXPECT_EQ (socket.getBoundPort(), -1);
    EXPECT_EQ (socket.getRawSocketHandle(), -1);
}

TEST (StreamingSocketTests, ConstructionWithOptions)
{
    auto opts = SocketOptions {}.withReceiveBufferSize (131072);
    StreamingSocket socket (opts);
    EXPECT_FALSE (socket.isConnected());
    EXPECT_EQ (socket.getRawSocketHandle(), -1);
}

TEST (StreamingSocketTests, CreateListenerAssignsPort)
{
    StreamingSocket server;
    EXPECT_TRUE (server.createListener (0, "127.0.0.1"));
    EXPECT_NE (server.getBoundPort(), -1);
    EXPECT_NE (server.getBoundPort(), 0);
}

TEST (StreamingSocketTests, ConnectToListener)
{
    StreamingSocket server;
    ASSERT_TRUE (server.createListener (0, "127.0.0.1"));

    StreamingSocket client;
    EXPECT_TRUE (client.connect ("127.0.0.1", server.getBoundPort(), 3000));
    EXPECT_TRUE (client.isConnected());
    EXPECT_EQ (client.getHostName(), "127.0.0.1");
}

TEST (StreamingSocketTests, IsLocalForLocalhostConnection)
{
    StreamingSocket server;
    ASSERT_TRUE (server.createListener (0, "127.0.0.1"));

    StreamingSocket client;
    ASSERT_TRUE (client.connect ("127.0.0.1", server.getBoundPort(), 3000));
    EXPECT_TRUE (client.isLocal());
}

TEST (StreamingSocketTests, CloseResetsState)
{
    StreamingSocket server;
    ASSERT_TRUE (server.createListener (0, "127.0.0.1"));

    StreamingSocket client;
    ASSERT_TRUE (client.connect ("127.0.0.1", server.getBoundPort(), 3000));
    ASSERT_TRUE (client.isConnected());

    client.close();
    EXPECT_FALSE (client.isConnected());
    EXPECT_TRUE (client.getHostName().isEmpty());
    EXPECT_EQ (client.getBoundPort(), -1);
    EXPECT_EQ (client.getRawSocketHandle(), -1);
}

TEST (StreamingSocketTests, ConnectToInvalidHostFails)
{
    StreamingSocket socket;
    EXPECT_FALSE (socket.connect ("invalid.host.local", 12345, 200));
    EXPECT_FALSE (socket.isConnected());
}

TEST (StreamingSocketTests, WaitUntilReadyTimesOutOnUnconnectedSocket)
{
    StreamingSocket socket;
    // Waiting on an unconnected socket should return error or timeout
    int result = socket.waitUntilReady (true, 0);
    EXPECT_NE (result, 1);
}

TEST (StreamingSocketTests, ReadAndWriteDataOverConnection)
{
    StreamingSocket server;
    ASSERT_TRUE (server.createListener (0, "127.0.0.1"));
    const int port = server.getBoundPort();

    struct ClientThread : public Thread
    {
        explicit ClientThread (int p)
            : Thread ("SocketWriteThread")
            , port (p)
        {
        }

        ~ClientThread() override { stopThread (2000); }

        void run() override
        {
            StreamingSocket client;
            if (! client.connect ("127.0.0.1", port, 3000))
                return;

            const int data = 0x12345678;
            client.write (&data, sizeof (data));
        }

        int port;
    };

    ClientThread writer (port);
    writer.startThread();

    std::unique_ptr<StreamingSocket> conn (server.waitForNextConnection());
    ASSERT_NE (conn.get(), nullptr);

    int received = 0;
    int bytesRead = conn->read (&received, sizeof (received), true);

    writer.stopThread (2000);

    EXPECT_EQ (bytesRead, (int) sizeof (received));
    EXPECT_EQ (received, 0x12345678);
}

// =============================================================================
// DatagramSocket Tests
// =============================================================================

TEST (DatagramSocketTests, DefaultConstruction)
{
    DatagramSocket socket;
    EXPECT_EQ (socket.getBoundPort(), -1);
}

TEST (DatagramSocketTests, ConstructionWithBroadcasting)
{
    DatagramSocket socket (true);
    EXPECT_EQ (socket.getBoundPort(), -1);
}

TEST (DatagramSocketTests, ConstructionWithOptions)
{
    auto opts = SocketOptions {}.withReceiveBufferSize (131072);
    DatagramSocket socket (false, opts);
    EXPECT_EQ (socket.getBoundPort(), -1);
}

TEST (DatagramSocketTests, BindToPortZeroAssignsPort)
{
    DatagramSocket socket;
    EXPECT_TRUE (socket.bindToPort (0));
    EXPECT_NE (socket.getBoundPort(), -1);
    EXPECT_NE (socket.getBoundPort(), 0);
}

TEST (DatagramSocketTests, BindToPortWithLocalhostAddress)
{
    DatagramSocket socket;
    EXPECT_TRUE (socket.bindToPort (0, "127.0.0.1"));
    EXPECT_NE (socket.getBoundPort(), -1);
}

TEST (DatagramSocketTests, ShutdownResetsBoundPort)
{
    DatagramSocket socket;
    ASSERT_TRUE (socket.bindToPort (0));
    ASSERT_NE (socket.getBoundPort(), -1);

    socket.shutdown();
    EXPECT_EQ (socket.getBoundPort(), -1);
}

TEST (DatagramSocketTests, SendAndReceiveLoopback)
{
    DatagramSocket receiver;
    ASSERT_TRUE (receiver.bindToPort (0, "127.0.0.1"));
    const int receiverPort = receiver.getBoundPort();

    const int sendData = 0xdeadbeef;
    DatagramSocket sender;
    int bytesSent = sender.write ("127.0.0.1", receiverPort, &sendData, sizeof (sendData));
    EXPECT_EQ (bytesSent, (int) sizeof (sendData));

    EXPECT_EQ (receiver.waitUntilReady (true, 2000), 1);

    int recvData = 0;
    int bytesRead = receiver.read (&recvData, sizeof (recvData), false);

    EXPECT_EQ (bytesRead, (int) sizeof (recvData));
    EXPECT_EQ (recvData, (int) 0xdeadbeef);
}

TEST (DatagramSocketTests, SendAndReceiveWithSenderInfo)
{
    DatagramSocket receiver;
    ASSERT_TRUE (receiver.bindToPort (0, "127.0.0.1"));
    const int receiverPort = receiver.getBoundPort();

    const int sendData = 42;
    DatagramSocket sender;
    sender.write ("127.0.0.1", receiverPort, &sendData, sizeof (sendData));

    ASSERT_EQ (receiver.waitUntilReady (true, 2000), 1);

    int recvData = 0;
    String senderAddr;
    int senderPort = -1;
    int bytesRead = receiver.read (&recvData, sizeof (recvData), false, senderAddr, senderPort);

    EXPECT_EQ (bytesRead, (int) sizeof (recvData));
    EXPECT_EQ (recvData, sendData);
    EXPECT_FALSE (senderAddr.isEmpty());
    EXPECT_GT (senderPort, 0);
}

TEST (DatagramSocketTests, SetEnablePortReuse)
{
    DatagramSocket socket;
    EXPECT_TRUE (socket.setEnablePortReuse (true));
    EXPECT_TRUE (socket.setEnablePortReuse (false));
}

#endif
