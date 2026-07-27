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

namespace yup
{

//==============================================================================
/** Abstract transport for JSON-RPC messages used by MCP.

    Implementations may use stdio, HTTP/SSE, sockets, or in-process queues. The
    payload is always a JSON-compatible `var` object.

    @tags{AI}
*/
class YUP_API MCPTransport
{
public:
    using MessageHandler = std::function<void (const var& message)>;

    virtual ~MCPTransport() = default;

    /** Sends one JSON-RPC message. */
    virtual Result sendMessage (const var& message) = 0;

    /** Receives the next JSON-RPC message, blocking until timeout when supported. */
    virtual ResultValue<var> receiveMessage (int timeoutMs = -1) = 0;

    /** Installs a callback for asynchronous incoming messages. */
    virtual void setMessageHandler (MessageHandler handler) = 0;

    /** Starts the transport. */
    virtual Result start() = 0;

    /** Stops the transport and releases any underlying connection. */
    virtual void stop() = 0;

    /** Returns true while the transport can send and receive messages. */
    virtual bool isConnected() const noexcept = 0;
};

} // namespace yup
