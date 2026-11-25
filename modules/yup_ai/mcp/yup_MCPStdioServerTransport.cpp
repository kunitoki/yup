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

// Shared with the reading thread, which outlives a transport stopped while its input is still open
struct MCPStdioServerTransport::Pimpl
{
    Pimpl (std::istream& inputToUse, std::ostream& outputToUse)
        : input (inputToUse)
        , output (outputToUse)
    {
    }

    Result start (std::shared_ptr<Pimpl> self)
    {
        if (started.exchange (true))
            return Result::fail ("An MCP stdio transport can only be started once");

        const ScopedLock delivering (deliveryLock);

        connected = true;
        reader = std::thread ([self = std::move (self)]
        {
            self->readMessages();
        });

        return Result::ok();
    }

    void stop()
    {
        connected = false;

        const ScopedLock delivering (deliveryLock);

        if (! reader.joinable())
            return;

        if (inputEnded.wait (0.0))
            reader.join();
        else
            reader.detach();
    }

    Result sendMessage (const var& message)
    {
        const ScopedLock lock (outputLock);

        output << JSON::toString (message, true).toStdString() << '\n';
        output.flush();

        return output.good() ? Result::ok() : Result::fail ("The MCP stdio transport can't write to its output");
    }

    void setMessageHandler (MessageHandler newHandler)
    {
        const ScopedLock lock (handlerLock);
        handler = std::move (newHandler);
    }

    void readMessages()
    {
        std::string line;

        while (connected && std::getline (input, line))
        {
            const ScopedLock delivering (deliveryLock);

            if (! connected)
                break;

            deliver (String::fromUTF8 (line.data(), static_cast<int> (line.size())).trim());
        }

        connected = false;
        inputEnded.signal();
    }

    void deliver (const String& text)
    {
        if (text.isEmpty())
            return;

        var message;
        const auto parsed = JSON::parse (text, message);

        if (parsed.failed() || ! message.isObject())
        {
            JsonRpcResponse response;
            response.error = JsonRpcError { parsed.failed() ? MCPErrorCodes::parseError : MCPErrorCodes::invalidRequest,
                                            parsed.failed() ? "Invalid JSON" : "Expected one JSON-RPC message per line",
                                            std::nullopt };

            sendMessage (response.toVar());
            return;
        }

        MessageHandler currentHandler;

        {
            const ScopedLock lock (handlerLock);
            currentHandler = handler;
        }

        if (currentHandler != nullptr)
            currentHandler (message);
    }

    std::istream& input;
    std::ostream& output;
    CriticalSection deliveryLock;
    CriticalSection outputLock;
    CriticalSection handlerLock;
    MessageHandler handler;
    std::atomic<bool> started { false };
    std::atomic<bool> connected { false };
    WaitableEvent inputEnded { true };
    std::thread reader;
};

#else

struct MCPStdioServerTransport::Pimpl
{
    Pimpl (std::istream&, std::ostream&) {}

    Result start (std::shared_ptr<Pimpl>) { return Result::fail ("The MCP stdio transport needs a thread, which this WebAssembly build does not have"); }

    void stop() {}

    Result sendMessage (const var&) { return Result::fail ("The MCP stdio transport is not running"); }

    void setMessageHandler (MessageHandler) {}

    std::atomic<bool> connected { false };
    WaitableEvent inputEnded { true };
};

#endif

//==============================================================================
MCPStdioServerTransport::MCPStdioServerTransport()
    : MCPStdioServerTransport (std::cin, std::cout)
{
}

MCPStdioServerTransport::MCPStdioServerTransport (std::istream& input, std::ostream& output)
    : pimpl (std::make_shared<Pimpl> (input, output))
{
}

MCPStdioServerTransport::~MCPStdioServerTransport()
{
    pimpl->stop();
}

Result MCPStdioServerTransport::sendMessage (const var& message)
{
    return pimpl->sendMessage (message);
}

ResultValue<var> MCPStdioServerTransport::receiveMessage (int)
{
    return makeResultValueFail ("The MCP stdio transport delivers messages to the message handler");
}

void MCPStdioServerTransport::setMessageHandler (MessageHandler handler)
{
    pimpl->setMessageHandler (std::move (handler));
}

Result MCPStdioServerTransport::start()
{
    return pimpl->start (pimpl);
}

void MCPStdioServerTransport::stop()
{
    pimpl->stop();
}

bool MCPStdioServerTransport::isConnected() const noexcept
{
    return pimpl->connected;
}

bool MCPStdioServerTransport::waitForInputToEnd (int timeoutMs) const
{
    return pimpl->inputEnded.wait (static_cast<double> (timeoutMs));
}

} // namespace yup
