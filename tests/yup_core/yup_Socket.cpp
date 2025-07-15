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

#if 0
TEST (SocketTests, StreamingSocket)
{
    auto localHost = IPAddress::local();
    int portNum = 12345;

    StreamingSocket socketServer;

    EXPECT_FALSE (socketServer.isConnected());
    EXPECT_TRUE (socketServer.getHostName().isEmpty());
    EXPECT_EQ (socketServer.getBoundPort(), -1);
    EXPECT_EQ (static_cast<SocketHandle> (socketServer.getRawSocketHandle()), invalidSocket);

    EXPECT_TRUE (socketServer.createListener (portNum, localHost.toString()));

    StreamingSocket socket;

    EXPECT_TRUE (socket.connect (localHost.toString(), portNum));

    EXPECT_TRUE (socket.isConnected());
    EXPECT_EQ (socket.getHostName(), localHost.toString());
    EXPECT_NE (socket.getBoundPort(), -1);
    EXPECT_NE (static_cast<SocketHandle> (socket.getRawSocketHandle()), invalidSocket);

    socket.close();

    EXPECT_FALSE (socket.isConnected());
    EXPECT_TRUE (socket.getHostName().isEmpty());
    EXPECT_EQ (socket.getBoundPort(), -1);
    EXPECT_EQ (static_cast<SocketHandle> (socket.getRawSocketHandle()), invalidSocket);
}

TEST (SocketTests, DatagramSocket)
{
    auto localHost = IPAddress::local();
    int portNum = 12345;

    DatagramSocket socket;

    EXPECT_EQ (socket.getBoundPort(), -1);
    EXPECT_NE (static_cast<SocketHandle> (socket.getRawSocketHandle()), invalidSocket);

    EXPECT_TRUE (socket.bindToPort (portNum, localHost.toString()));

    EXPECT_EQ (socket.getBoundPort(), portNum);
    EXPECT_NE (static_cast<SocketHandle> (socket.getRawSocketHandle()), invalidSocket);

    socket.shutdown();

    EXPECT_EQ (socket.getBoundPort(), -1);
    EXPECT_EQ (static_cast<SocketHandle> (socket.getRawSocketHandle()), invalidSocket);
}
#endif
