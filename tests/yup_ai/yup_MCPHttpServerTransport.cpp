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

#include <yup_ai/yup_ai.h>

using namespace yup;

#if ! YUP_WASM

class MCPHttpServerTransportTests : public ::testing::Test
{
protected:
    struct HttpReply
    {
        int status = 0;
        String headers;
        String body;
    };

    void SetUp() override
    {
        MCPToolDefinition echo;
        echo.name = "echo";
        echo.description = "Echoes text.";

        server.registerTool (echo, [] (const var& arguments)
        {
            return arguments["value"];
        });

        auto newTransport = std::make_unique<MCPHttpServerTransport>();
        transport = newTransport.get();
        ASSERT_TRUE (server.start (std::move (newTransport)).wasOk());
        ASSERT_GT (transport->getPort(), 0);
    }

    HttpReply send (const String& method, const String& path, const String& body, const String& extraHeaders = {}) const
    {
        StreamingSocket socket;
        if (! socket.connect ("127.0.0.1", transport->getPort(), 2000))
            return {};

        String request;
        request << method << " " << path << " HTTP/1.1\r\n"
                << "Host: 127.0.0.1:" << transport->getPort() << "\r\n"
                << "Content-Type: application/json\r\n"
                << "Accept: application/json, text/event-stream\r\n"
                << "Content-Length: " << static_cast<int> (body.getNumBytesAsUTF8()) << "\r\n"
                << extraHeaders
                << "\r\n"
                << body;

        const auto bytes = request.toStdString();
        socket.write (bytes.data(), static_cast<int> (bytes.size()));

        std::string received;
        char chunk[4096];

        for (;;)
        {
            if (socket.waitUntilReady (true, 5000) <= 0)
                break;

            const auto numRead = socket.read (chunk, static_cast<int> (sizeof (chunk)), false);
            if (numRead <= 0)
                break;

            received.append (chunk, static_cast<std::size_t> (numRead));
        }

        const auto text = String::fromUTF8 (received.data(), static_cast<int> (received.size()));

        HttpReply reply;
        reply.status = text.fromFirstOccurrenceOf (" ", false, false).getIntValue();
        reply.headers = text.upToFirstOccurrenceOf ("\r\n\r\n", false, false);
        reply.body = text.fromFirstOccurrenceOf ("\r\n\r\n", false, false);
        return reply;
    }

    MCPServer server;
    MCPHttpServerTransport* transport = nullptr;
};

TEST_F (MCPHttpServerTransportTests, AnswersRequestWithJson)
{
    const auto reply = send ("POST", "/mcp", R"({"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"echo","arguments":{"value":"over http"}}})");

    EXPECT_EQ (200, reply.status);
    EXPECT_TRUE (reply.headers.containsIgnoreCase ("Content-Type: application/json"));

    const auto response = JSON::parse (reply.body);
    EXPECT_EQ (1, static_cast<int> (response["id"]));
    EXPECT_EQ ("over http", response["result"]["content"][0]["text"].toString());
}

TEST_F (MCPHttpServerTransportTests, AcceptsNotificationsWithoutBody)
{
    const auto reply = send ("POST", "/mcp", R"({"jsonrpc":"2.0","method":"notifications/initialized"})");

    EXPECT_EQ (202, reply.status);
    EXPECT_TRUE (reply.body.isEmpty());
}

TEST_F (MCPHttpServerTransportTests, AnswersBatchWithArray)
{
    const auto reply = send ("POST", "/mcp", R"([{"jsonrpc":"2.0","id":1,"method":"ping"},{"jsonrpc":"2.0","method":"notifications/initialized"},{"jsonrpc":"2.0","id":2,"method":"ping"}])");

    EXPECT_EQ (200, reply.status);

    const auto response = JSON::parse (reply.body);
    ASSERT_TRUE (response.isArray());
    EXPECT_EQ (2, response.size());
}

TEST_F (MCPHttpServerTransportTests, RejectsInvalidJson)
{
    const auto reply = send ("POST", "/mcp", "{ not json");

    EXPECT_EQ (400, reply.status);
    EXPECT_EQ (MCPErrorCodes::parseError, static_cast<int> (JSON::parse (reply.body)["error"]["code"]));
}

TEST_F (MCPHttpServerTransportTests, RejectsOtherMethodsAndPaths)
{
    EXPECT_EQ (405, send ("GET", "/mcp", {}).status);
    EXPECT_EQ (405, send ("DELETE", "/mcp", {}).status);
    EXPECT_EQ (404, send ("POST", "/other", R"({"jsonrpc":"2.0","id":1,"method":"ping"})").status);
}

TEST_F (MCPHttpServerTransportTests, RejectsForeignOrigin)
{
    const auto ping = R"({"jsonrpc":"2.0","id":1,"method":"ping"})";

    EXPECT_EQ (403, send ("POST", "/mcp", ping, "Origin: http://evil.example.com\r\n").status);
    EXPECT_EQ (200, send ("POST", "/mcp", ping, "Origin: http://localhost:3000\r\n").status);
}

TEST_F (MCPHttpServerTransportTests, RejectsMessagesOutsideRequests)
{
    EXPECT_TRUE (transport->sendMessage (var ("unsolicited")).failed());
}

TEST_F (MCPHttpServerTransportTests, TriesNextPortWhenTaken)
{
    MCPHttpServerTransport::Options options;
    options.port = transport->getPort();
    options.numPortsToTry = 8;

    MCPHttpServerTransport second (options);
    ASSERT_TRUE (second.start().wasOk());
    EXPECT_NE (transport->getPort(), second.getPort());
    EXPECT_EQ ("http://127.0.0.1:" + String (second.getPort()) + "/mcp", second.getUrl());

    second.stop();
    EXPECT_FALSE (second.isConnected());
    EXPECT_TRUE (second.getUrl().isEmpty());
}

TEST_F (MCPHttpServerTransportTests, FailsWhenOnlyPortIsTaken)
{
    MCPHttpServerTransport::Options options;
    options.port = transport->getPort();

    MCPHttpServerTransport second (options);
    EXPECT_TRUE (second.start().failed());
}

#endif
