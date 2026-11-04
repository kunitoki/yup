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
/** Server side of the MCP Streamable HTTP transport, answering with JSON.

    Clients POST JSON-RPC messages, or batches of them, to one endpoint. A request is
    answered in the HTTP response with `application/json`, and notifications with
    `202 Accepted`. The server never streams (no SSE), so GET answers
    `405 Method Not Allowed`, as the specification allows.

    Every connection is served on a small thread pool, so the message handler is called
    from those threads, possibly several at once, and must reply through sendMessage()
    before returning. Messages sent from any other thread are rejected, since there is
    no stream to push them on.

    To guard against DNS rebinding, it listens on the loopback interface by default and
    rejects requests whose `Host` or `Origin` names a host other than the loopback one.

    @code
    MCPServer server;
    // ... register tools ...

    MCPHttpServerTransport::Options options;
    options.port = 7380;
    options.numPortsToTry = 16;

    auto transport = std::make_unique<MCPHttpServerTransport> (options);
    auto* http = transport.get();

    if (server.start (std::move (transport)).wasOk())
        YUP_DBG ("Serving at " << http->getUrl());
    @endcode

    @see MCPServer

    @tags{AI}
*/
class YUP_API MCPHttpServerTransport : public MCPTransport
{
public:
    struct Options
    {
        String host = "127.0.0.1";          /**< The local address to listen on. */
        int port = 0;                        /**< The first port to try, or 0 for any free port. */
        int numPortsToTry = 1;               /**< How many consecutive ports to try from `port`. */
        String endpoint = "/mcp";            /**< The path clients POST to. */
        int maxRequestBytes = 64 * 1024 * 1024;
        int numThreads = 4;                  /**< How many requests are served at once. */
        int requestTimeoutMs = 30000;        /**< How long a client may take to send a request. */
    };

    MCPHttpServerTransport();
    explicit MCPHttpServerTransport (Options options);
    ~MCPHttpServerTransport() override;

    //==============================================================================
    /** Queues a reply to the request being handled on the calling thread. */
    Result sendMessage (const var& message) override;

    /** Always fails: incoming messages are delivered to the message handler. */
    ResultValue<var> receiveMessage (int timeoutMs = -1) override;

    void setMessageHandler (MessageHandler handler) override;

    /** Starts listening on the first free port of the configured range. */
    Result start() override;

    /** Stops listening and waits for the requests being served to finish. */
    void stop() override;

    /** True while listening. */
    bool isConnected() const noexcept override;

    //==============================================================================
    /** Returns the port being listened on, or 0 when not listening. */
    int getPort() const noexcept;

    /** Returns the endpoint's URL, like `http://127.0.0.1:7380/mcp`, or empty when not listening. */
    String getUrl() const;

private:
    struct Pimpl;
    std::unique_ptr<Pimpl> pimpl;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MCPHttpServerTransport)
};

} // namespace yup
