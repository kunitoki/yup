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

#if ! YUP_WASM

namespace
{
struct MCPHttpReplyCollector
{
    const void* owner = nullptr;
    std::vector<var> messages;
};

MCPHttpReplyCollector*& getCurrentMCPHttpReplyCollector() noexcept
{
    static thread_local MCPHttpReplyCollector* collector = nullptr;
    return collector;
}

bool isLoopbackHost (const String& hostAndPort)
{
    auto host = hostAndPort.trim().toLowerCase();

    if (host.startsWithChar ('['))
        host = host.upToFirstOccurrenceOf ("]", true, false);
    else
        host = host.upToFirstOccurrenceOf (":", false, false);

    return host == "localhost" || host == "127.0.0.1" || host == "[::1]";
}

bool isLoopbackOrigin (const String& origin)
{
    if (! origin.contains ("://"))
        return false;

    return isLoopbackHost (origin.fromFirstOccurrenceOf ("://", false, false).upToFirstOccurrenceOf ("/", false, false));
}

void preventBrokenPipeSignal ([[maybe_unused]] StreamingSocket& socket)
{
#if defined(SO_NOSIGPIPE)
    int enabled = 1;
    ::setsockopt (socket.getRawSocketHandle(), SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof (enabled));
#endif
}

int sendWithoutBrokenPipeSignal (StreamingSocket& socket, const char* data, int numBytes)
{
#if defined(MSG_NOSIGNAL)
    return static_cast<int> (::send (socket.getRawSocketHandle(), data, static_cast<size_t> (numBytes), MSG_NOSIGNAL));
#else
    return socket.write (data, numBytes);
#endif
}

String makeJsonRpcErrorBody (int code, const String& message)
{
    JsonRpcResponse response;
    response.error = JsonRpcError { code, message, std::nullopt };
    return JSON::toString (response.toVar(), true);
}
} // namespace

//==============================================================================
struct MCPHttpServerTransport::Pimpl : private Thread
{
    struct Request
    {
        String method;
        String path;
        StringPairArray headers;
        String body;
    };

    struct Response
    {
        int status = 200;
        String reason = "OK";
        String body;
        String extraHeaders;
    };

    explicit Pimpl (Options optionsToUse)
        : Thread ("MCP HTTP Listener")
        , options (std::move (optionsToUse))
    {
    }

    ~Pimpl() override
    {
        stop();
    }

    Result start()
    {
        if (listener != nullptr)
            return Result::ok();

        const auto attempts = options.port == 0 ? 1 : jmax (1, options.numPortsToTry);

        for (int attempt = 0; attempt < attempts && listener == nullptr; ++attempt)
        {
            auto socket = std::make_unique<StreamingSocket>();
            const auto portToTry = options.port == 0 ? 0 : options.port + attempt;

            if (socket->createListener (portToTry, options.host))
                listener = std::move (socket);
        }

        if (listener == nullptr)
            return Result::fail ("Could not listen for MCP connections on " + options.host + " from port " + String (options.port));

        boundPort = listener->getBoundPort();
        stopping = false;
        pool = std::make_unique<ThreadPool> (ThreadPoolOptions()
                                                 .withThreadName ("MCP HTTP")
                                                 .withNumberOfThreads (jmax (1, options.numThreads)));

        startThread();
        return Result::ok();
    }

    void stop()
    {
        if (listener == nullptr)
            return;

        stopping = true;
        stopThread (2000);
        listener->close();

        pool->removeAllJobs (true, 5000);
        pool.reset();
        listener.reset();
        boundPort = 0;
    }

    Result sendMessage (const var& message) const
    {
        auto* collector = getCurrentMCPHttpReplyCollector();
        if (collector == nullptr || collector->owner != this)
            return Result::fail ("The MCP HTTP transport only sends replies to the request being handled");

        collector->messages.push_back (message);
        return Result::ok();
    }

    void setMessageHandler (MessageHandler newHandler)
    {
        const ScopedLock lock (handlerLock);
        handler = std::move (newHandler);
    }

    void run() override
    {
        while (! threadShouldExit())
        {
            const auto ready = listener->waitUntilReady (true, 100);
            if (ready < 0)
                break;

            if (ready == 0 || threadShouldExit())
                continue;

            std::shared_ptr<StreamingSocket> connection (listener->waitForNextConnection());
            if (connection == nullptr)
                continue;

            pool->addJob ([this, connection]
            {
                preventBrokenPipeSignal (*connection);
                serve (*connection);
                connection->close();
            });
        }
    }

    //==============================================================================
    void serve (StreamingSocket& socket)
    {
        Request request;
        const auto failure = readRequest (socket, request);
        writeResponse (socket, failure.has_value() ? *failure : respond (request));
    }

    Response respond (const Request& request)
    {
        if (isLoopbackHost (options.host))
        {
            const auto origin = request.headers["Origin"];
            if (! isLoopbackHost (request.headers["Host"]) || (origin.isNotEmpty() && ! isLoopbackOrigin (origin)))
                return { 403, "Forbidden", makeJsonRpcErrorBody (MCPErrorCodes::invalidRequest, "Host or origin not allowed"), {} };
        }

        if (request.path != options.endpoint)
            return { 404, "Not Found", {}, {} };

        if (request.method != "POST")
            return { 405, "Method Not Allowed", {}, "Allow: POST\r\n" };

        var message;
        if (JSON::parse (request.body, message).failed() || ! (message.isObject() || message.isArray()))
            return { 400, "Bad Request", makeJsonRpcErrorBody (MCPErrorCodes::parseError, "Invalid JSON"), {} };

        MCPHttpReplyCollector collector { this, {} };
        getCurrentMCPHttpReplyCollector() = &collector;

        if (auto* batch = message.getArray())
        {
            for (const auto& item : *batch)
                dispatch (item);
        }
        else
        {
            dispatch (message);
        }

        getCurrentMCPHttpReplyCollector() = nullptr;

        if (collector.messages.empty())
            return { 202, "Accepted", {}, {} };

        if (message.isArray())
            return { 200, "OK", JSON::toString (var (Array<var> (collector.messages.data(), static_cast<int> (collector.messages.size()))), true), {} };

        return { 200, "OK", JSON::toString (collector.messages.front(), true), {} };
    }

    void dispatch (const var& message)
    {
        MessageHandler currentHandler;
        {
            const ScopedLock lock (handlerLock);
            currentHandler = handler;
        }

        if (currentHandler)
            currentHandler (message);
    }

    //==============================================================================
    std::optional<Response> readRequest (StreamingSocket& socket, Request& request) const
    {
        constexpr std::size_t maxHeaderBytes = 64 * 1024;
        const auto deadline = Time::getMillisecondCounter() + static_cast<uint32> (jmax (0, options.requestTimeoutMs));

        std::string data;
        char chunk[8192];

        const auto readMore = [&]
        {
            while (! stopping && Time::getMillisecondCounter() < deadline)
            {
                const auto ready = socket.waitUntilReady (true, 50);
                if (ready < 0)
                    return false;

                if (ready == 0)
                    continue;

                const auto numRead = socket.read (chunk, static_cast<int> (sizeof (chunk)), false);
                if (numRead <= 0)
                    return false;

                data.append (chunk, static_cast<std::size_t> (numRead));
                return true;
            }

            return false;
        };

        auto headerEnd = std::string::npos;
        while ((headerEnd = data.find ("\r\n\r\n")) == std::string::npos)
        {
            if (data.size() > maxHeaderBytes)
                return Response { 431, "Request Header Fields Too Large", {}, {} };

            if (! readMore())
                return Response { 408, "Request Timeout", {}, {} };
        }

        auto lines = StringArray::fromLines (String::fromUTF8 (data.data(), static_cast<int> (headerEnd)));
        const auto requestLine = StringArray::fromTokens (lines[0], " ", "");
        if (requestLine.size() < 3)
            return Response { 400, "Bad Request", {}, {} };

        request.method = requestLine[0].toUpperCase();
        request.path = requestLine[1].upToFirstOccurrenceOf ("?", false, false);

        for (int index = 1; index < lines.size(); ++index)
            if (lines[index].containsChar (':'))
                request.headers.set (lines[index].upToFirstOccurrenceOf (":", false, false).trim(),
                                     lines[index].fromFirstOccurrenceOf (":", false, false).trim());

        if (request.headers["Transfer-Encoding"].containsIgnoreCase ("chunked"))
            return Response { 411, "Length Required", {}, {} };

        const auto contentLength = request.headers["Content-Length"].getLargeIntValue();
        if (contentLength < 0 || contentLength > options.maxRequestBytes)
            return Response { 413, "Content Too Large", {}, {} };

        const auto bodyStart = headerEnd + 4;
        const auto bodyLength = static_cast<std::size_t> (contentLength);

        while (data.size() - bodyStart < bodyLength)
            if (! readMore())
                return Response { 408, "Request Timeout", {}, {} };

        request.body = String::fromUTF8 (data.data() + bodyStart, static_cast<int> (bodyLength));
        return std::nullopt;
    }

    static void writeResponse (StreamingSocket& socket, const Response& response)
    {
        const auto body = response.body.toStdString();

        String head;
        head << "HTTP/1.1 " << response.status << " " << response.reason << "\r\n";

        if (! body.empty())
            head << "Content-Type: application/json\r\n";

        head << "Content-Length: " << static_cast<int64> (body.size()) << "\r\n"
             << "Cache-Control: no-store\r\n"
             << "Connection: close\r\n"
             << response.extraHeaders
             << "\r\n";

        const auto bytes = head.toStdString() + body;

        for (std::size_t written = 0; written < bytes.size();)
        {
            const auto numToWrite = static_cast<int> (jmin<std::size_t> (bytes.size() - written, 1 << 20));
            const auto numWritten = sendWithoutBrokenPipeSignal (socket, bytes.data() + written, numToWrite);
            if (numWritten <= 0)
                return;

            written += static_cast<std::size_t> (numWritten);
        }
    }

    //==============================================================================
    const Options options;
    std::unique_ptr<StreamingSocket> listener;
    std::unique_ptr<ThreadPool> pool;
    std::atomic<int> boundPort { 0 };
    std::atomic<bool> stopping { false };
    CriticalSection handlerLock;
    MessageHandler handler;
};

#else

struct MCPHttpServerTransport::Pimpl
{
    explicit Pimpl (Options optionsToUse)
        : options (std::move (optionsToUse))
    {
    }

    Result start() { return Result::fail ("The MCP HTTP transport needs sockets, which WebAssembly does not provide"); }

    void stop() {}

    Result sendMessage (const var&) { return Result::fail ("The MCP HTTP transport is not running"); }

    void setMessageHandler (MessageHandler) {}

    const Options options;
    std::atomic<int> boundPort { 0 };
};

#endif

//==============================================================================
MCPHttpServerTransport::MCPHttpServerTransport()
    : MCPHttpServerTransport (Options {})
{
}

MCPHttpServerTransport::MCPHttpServerTransport (Options options)
    : pimpl (std::make_unique<Pimpl> (std::move (options)))
{
}

MCPHttpServerTransport::~MCPHttpServerTransport() = default;

Result MCPHttpServerTransport::sendMessage (const var& message)
{
    return pimpl->sendMessage (message);
}

ResultValue<var> MCPHttpServerTransport::receiveMessage (int)
{
    return makeResultValueFail ("The MCP HTTP transport delivers messages to the message handler");
}

void MCPHttpServerTransport::setMessageHandler (MessageHandler handler)
{
    pimpl->setMessageHandler (std::move (handler));
}

Result MCPHttpServerTransport::start()
{
    return pimpl->start();
}

void MCPHttpServerTransport::stop()
{
    pimpl->stop();
}

bool MCPHttpServerTransport::isConnected() const noexcept
{
    return pimpl->boundPort > 0;
}

int MCPHttpServerTransport::getPort() const noexcept
{
    return pimpl->boundPort;
}

String MCPHttpServerTransport::getUrl() const
{
    const auto port = getPort();
    if (port <= 0)
        return {};

    return "http://" + pimpl->options.host + ":" + String (port) + pimpl->options.endpoint;
}

} // namespace yup
