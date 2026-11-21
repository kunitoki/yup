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
/** Server side of the MCP stdio transport: one JSON-RPC message per line.

    A client that starts the server as a child process writes its messages to the standard
    input of the server, each on a line of its own, and reads the answers from its standard
    output in the same way. Nothing else may be written to that output while the transport
    is in use.

    The input is read on a thread of the transport, which calls the message handler with one
    message at a time. Answers can be sent from any thread. The input ending is how a client
    leaves: isConnected() turns false and waitForInputToEnd() returns.

    @code
    MCPServer server;
    // ... register tools ...

    auto transport = std::make_unique<MCPStdioServerTransport>();
    auto* stdio = transport.get();

    if (server.start (std::move (transport)).wasOk())
        stdio->waitForInputToEnd();
    @endcode

    @see MCPServer

    @tags{AI}
*/
class YUP_API MCPStdioServerTransport : public MCPTransport
{
public:
    /** Creates a transport on the standard input and output of the process. */
    MCPStdioServerTransport();

    /** Creates a transport on a pair of streams, which must outlive it.

        A transport stopped while its input is still open leaves its thread waiting for the
        next line, so the input must then stay alive until that line arrives or it ends.

        @param input  The stream the messages of the client are read from.
        @param output The stream the answers are written to.
    */
    MCPStdioServerTransport (std::istream& input, std::ostream& output);

    ~MCPStdioServerTransport() override;

    //==============================================================================
    /** Writes a message to the output, on a line of its own. */
    Result sendMessage (const var& message) override;

    /** Always fails: incoming messages are delivered to the message handler. */
    ResultValue<var> receiveMessage (int timeoutMs = -1) override;

    void setMessageHandler (MessageHandler handler) override;

    /** Starts reading the input. A transport can only be started once. */
    Result start() override;

    /** Stops delivering messages, once the one being delivered is.

        This never waits for the input: a thread still waiting for it is left to end by itself,
        with the next line or with the input.
    */
    void stop() override;

    /** True from start() until the input ends or stop() is called. */
    bool isConnected() const noexcept override;

    //==============================================================================
    /** Waits for the input to end, which is how a client leaves.

        @param timeoutMs How long to wait in milliseconds, or a negative value to wait forever.

        @returns True when the input ended, false when the time ran out first.
    */
    bool waitForInputToEnd (int timeoutMs = -1) const;

private:
    struct Pimpl;
    std::shared_ptr<Pimpl> pimpl;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MCPStdioServerTransport)
};

} // namespace yup
