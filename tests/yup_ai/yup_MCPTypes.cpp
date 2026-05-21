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

namespace
{
var makeTestObject()
{
    return var (std::make_unique<DynamicObject>());
}

void setTestProperty (var& object, const Identifier& name, const var& value)
{
    if (auto* dynamicObject = object.getDynamicObject())
        dynamicObject->setProperty (name, value);
}

var makeTextToolResult (const String& text)
{
    auto textContent = makeTestObject();
    setTestProperty (textContent, "type", "text");
    setTestProperty (textContent, "text", text);

    var content;
    content.append (textContent);

    auto result = makeTestObject();
    setTestProperty (result, "content", content);
    return result;
}

var makeResourceReadResult (const String& uri, const String& text)
{
    auto resourceContent = makeTestObject();
    setTestProperty (resourceContent, "uri", uri);
    setTestProperty (resourceContent, "mimeType", "application/json");
    setTestProperty (resourceContent, "text", text);

    var contents;
    contents.append (resourceContent);

    auto result = makeTestObject();
    setTestProperty (result, "contents", contents);
    return result;
}

class MockMCPTransport final : public MCPTransport
{
public:
    Result sendMessage (const var& message) override
    {
        sentMessages.push_back (message);

        auto request = JsonRpcRequest::fromVar (message);
        if (! request.has_value() || request->isNotification())
            return Result::ok();

        JsonRpcResponse response;
        response.id = *request->id;

        if (request->method == "initialize")
        {
            auto result = makeTestObject();
            setTestProperty (result, "protocolVersion", "2024-11-05");
            setTestProperty (result, "capabilities", MCPCapabilities { true, true }.toVar());
            response.result = result;
        }
        else if (request->method == "tools/list")
        {
            MCPToolDefinition tool;
            tool.name = "echo";
            tool.description = "Echoes text.";
            tool.inputSchema = JSON::parse (R"({
                "type": "object",
                "properties": {
                    "value": { "type": "string", "description": "Text to echo." }
                },
                "required": [ "value" ]
            })");

            var tools;
            tools.append (tool.toVar());

            auto result = makeTestObject();
            setTestProperty (result, "tools", tools);
            response.result = result;
        }
        else if (request->method == "tools/call")
        {
            response.result = makeTextToolResult ((*request->params)["arguments"]["value"].toString());
        }
        else if (request->method == "resources/list")
        {
            MCPResourceDefinition resource;
            resource.uri = "yup://test/status";
            resource.name = "Status";
            resource.description = "Test status.";

            var resources;
            resources.append (resource.toVar());

            auto result = makeTestObject();
            setTestProperty (result, "resources", resources);
            response.result = result;
        }
        else if (request->method == "resources/read")
        {
            response.result = makeResourceReadResult ((*request->params)["uri"].toString(), R"({"ok":true})");
        }
        else
        {
            response.error = JsonRpcError { MCPErrorCodes::methodNotFound, "Method not found", std::nullopt };
        }

        queuedMessages.push_back (response.toVar());
        return Result::ok();
    }

    ResultValue<var> receiveMessage (int) override
    {
        if (queuedMessages.empty())
            return makeResultValueFail ("No queued MCP messages");

        auto message = queuedMessages.front();
        queuedMessages.erase (queuedMessages.begin());
        return makeResultValueOk (std::move (message));
    }

    void setMessageHandler (MessageHandler handler) override
    {
        messageHandler = std::move (handler);
    }

    Result start() override
    {
        connected = true;
        return Result::ok();
    }

    void stop() override
    {
        connected = false;
    }

    bool isConnected() const noexcept override
    {
        return connected;
    }

    bool connected = false;
    std::vector<var> sentMessages;
    std::vector<var> queuedMessages;
    MessageHandler messageHandler;
};

class ServerCaptureTransport final : public MCPTransport
{
public:
    Result sendMessage (const var& message) override
    {
        sentMessages.push_back (message);
        return Result::ok();
    }

    ResultValue<var> receiveMessage (int) override
    {
        return makeResultValueFail ("ServerCaptureTransport does not support receiveMessage");
    }

    void setMessageHandler (MessageHandler handler) override
    {
        messageHandler = std::move (handler);
    }

    Result start() override
    {
        connected = true;
        return Result::ok();
    }

    void stop() override
    {
        connected = false;
    }

    bool isConnected() const noexcept override
    {
        return connected;
    }

    void deliver (const var& message)
    {
        if (messageHandler)
            messageHandler (message);
    }

    bool connected = false;
    std::vector<var> sentMessages;
    MessageHandler messageHandler;
};

class LinkedMCPTransport final : public MCPTransport
{
public:
    Result sendMessage (const var& message) override
    {
        if (peer == nullptr)
            return Result::fail ("LinkedMCPTransport has no peer");

        if (peer->messageHandler)
            peer->messageHandler (message);
        else
            peer->queuedMessages.push_back (message);

        return Result::ok();
    }

    ResultValue<var> receiveMessage (int) override
    {
        if (queuedMessages.empty())
            return makeResultValueFail ("No queued linked MCP messages");

        auto message = queuedMessages.front();
        queuedMessages.erase (queuedMessages.begin());
        return makeResultValueOk (std::move (message));
    }

    void setMessageHandler (MessageHandler handler) override
    {
        messageHandler = std::move (handler);
    }

    Result start() override
    {
        connected = true;
        return Result::ok();
    }

    void stop() override
    {
        connected = false;
    }

    bool isConnected() const noexcept override
    {
        return connected;
    }

    LinkedMCPTransport* peer = nullptr;
    bool connected = false;
    std::vector<var> queuedMessages;
    MessageHandler messageHandler;
};

JsonRpcRequest makeTestRequest (int id, const String& method, std::optional<var> params = std::nullopt)
{
    JsonRpcRequest request;
    request.id = id;
    request.method = method;
    request.params = std::move (params);
    return request;
}

MCPToolDefinition makeEchoToolDefinition()
{
    MCPToolDefinition tool;
    tool.name = "echo";
    tool.description = "Echoes text.";
    tool.inputSchema = JSON::parse (R"({
        "type": "object",
        "properties": {
            "value": { "type": "string", "description": "Text to echo." }
        },
        "required": [ "value" ]
    })");

    return tool;
}
} // namespace

TEST (YupAiMCPTypes, SerializesAndParsesJsonRpcRequest)
{
    JsonRpcRequest request;
    request.id = 7;
    request.method = "tools/call";
    request.params = JSON::parse (R"({"name":"echo","arguments":{"value":"hello"}})");

    auto parsed = JsonRpcRequest::fromVar (request.toVar());

    ASSERT_TRUE (parsed.has_value());
    EXPECT_FALSE (parsed->isNotification());
    EXPECT_EQ ("2.0", parsed->jsonrpc);
    EXPECT_EQ (7, static_cast<int> (*parsed->id));
    EXPECT_EQ ("tools/call", parsed->method);
    ASSERT_TRUE (parsed->params.has_value());
    EXPECT_EQ ("echo", (*parsed->params)["name"].toString());
    EXPECT_EQ ("hello", (*parsed->params)["arguments"]["value"].toString());
}

TEST (YupAiMCPTypes, ParsesNotificationWithoutId)
{
    auto parsed = JsonRpcRequest::fromVar (JSON::parse (R"({
        "jsonrpc": "2.0",
        "method": "notifications/initialized"
    })"));

    ASSERT_TRUE (parsed.has_value());
    EXPECT_TRUE (parsed->isNotification());
    EXPECT_EQ ("notifications/initialized", parsed->method);
}

TEST (YupAiMCPTypes, SerializesAndParsesJsonRpcErrorResponse)
{
    JsonRpcResponse response;
    response.id = "abc";
    response.error = JsonRpcError {
        MCPErrorCodes::methodNotFound,
        "No such method",
        JSON::parse (R"({"method":"missing"})")
    };

    auto parsed = JsonRpcResponse::fromVar (response.toVar());

    ASSERT_TRUE (parsed.has_value());
    EXPECT_TRUE (parsed->isError());
    EXPECT_EQ ("abc", parsed->id.toString());
    ASSERT_TRUE (parsed->error.has_value());
    EXPECT_EQ (MCPErrorCodes::methodNotFound, parsed->error->code);
    EXPECT_EQ ("No such method", parsed->error->message);
    ASSERT_TRUE (parsed->error->data.has_value());
    EXPECT_EQ ("missing", (*parsed->error->data)["method"].toString());
}

TEST (YupAiMCPTypes, SerializesAndParsesCapabilities)
{
    MCPCapabilities capabilities;
    capabilities.supportsTools = true;
    capabilities.supportsResources = true;

    auto parsed = MCPCapabilities::fromVar (capabilities.toVar());

    EXPECT_TRUE (parsed.supportsTools);
    EXPECT_TRUE (parsed.supportsResources);
    EXPECT_FALSE (parsed.supportsPrompts);
    EXPECT_FALSE (parsed.supportsLogging);
}

TEST (YupAiMCPTypes, SerializesAndParsesToolDefinition)
{
    MCPToolDefinition tool;
    tool.name = "set_background_color";
    tool.description = "Changes the component background color.";
    tool.inputSchema = JSON::parse (R"({
        "type": "object",
        "properties": {
            "color": { "type": "string" }
        },
        "required": [ "color" ]
    })");

    auto parsed = MCPToolDefinition::fromVar (tool.toVar());

    ASSERT_TRUE (parsed.has_value());
    EXPECT_EQ ("set_background_color", parsed->name);
    EXPECT_EQ ("string", parsed->inputSchema["properties"]["color"]["type"].toString());
    EXPECT_EQ ("color", parsed->inputSchema["required"][0].toString());
}

TEST (YupAiMCPTypes, SerializesAndParsesResourceDefinition)
{
    MCPResourceDefinition resource;
    resource.uri = "yup://graph/main/nodes";
    resource.name = "Current Graph";
    resource.description = "Current audio graph nodes.";

    auto parsed = MCPResourceDefinition::fromVar (resource.toVar());

    ASSERT_TRUE (parsed.has_value());
    EXPECT_EQ ("yup://graph/main/nodes", parsed->uri);
    EXPECT_EQ ("Current Graph", parsed->name);
    EXPECT_EQ ("application/json", parsed->mimeType);
}

TEST (YupAiMCPClient, InitializesAndSendsInitializedNotification)
{
    auto transport = std::make_unique<MockMCPTransport>();
    auto* transportPtr = transport.get();
    MCPClient client (std::move (transport));

    EXPECT_TRUE (client.initialize().wasOk());

    ASSERT_EQ (2u, transportPtr->sentMessages.size());
    EXPECT_EQ ("initialize", transportPtr->sentMessages[0]["method"].toString());
    EXPECT_EQ ("notifications/initialized", transportPtr->sentMessages[1]["method"].toString());
}

TEST (YupAiMCPClient, ListsAndCallsTools)
{
    auto transport = std::make_unique<MockMCPTransport>();
    MCPClient client (std::move (transport));

    auto tools = client.listTools();
    ASSERT_EQ (1u, tools.size());
    EXPECT_EQ ("echo", tools.front().name);
    EXPECT_EQ ("string", tools.front().inputSchema["properties"]["value"]["type"].toString());

    auto result = client.callTool ("echo", JSON::parse (R"({"value":"hello"})"));
    ASSERT_TRUE (result.wasOk());
    EXPECT_EQ ("hello", result.getValue().toString());
}

TEST (YupAiMCPClient, ListsAndReadsResources)
{
    auto transport = std::make_unique<MockMCPTransport>();
    MCPClient client (std::move (transport));

    auto resources = client.listResources();
    ASSERT_EQ (1u, resources.size());
    EXPECT_EQ ("yup://test/status", resources.front().uri);

    auto result = client.readResource ("yup://test/status");
    ASSERT_TRUE (result.wasOk());
    EXPECT_EQ (R"({"ok":true})", result.getValue());
}

TEST (YupAiMCPClient, RegistersRemoteToolsWithLLMRegistry)
{
    auto transport = std::make_unique<MockMCPTransport>();
    MCPClient client (std::move (transport));
    LLMToolRegistry registry;

    client.registerToolsWith (registry);

    ASSERT_TRUE (registry.contains ("echo"));
    const auto* tool = registry.findTool ("echo");
    ASSERT_NE (nullptr, tool);
    ASSERT_EQ (1u, tool->parameters.size());
    EXPECT_EQ ("value", tool->parameters.front().name);
    EXPECT_TRUE (tool->parameters.front().required);

    auto result = registry.dispatchToolCall ("echo", JSON::parse (R"({"value":"from registry"})"));
    EXPECT_EQ ("from registry", result.toString());
}

TEST (YupAiMCPServer, StartsAndHandlesInitialize)
{
    MCPServer::Options options;
    options.serverName = "Test Server";
    options.serverVersion = "2.0";

    MCPServer server (options);
    auto transport = std::make_unique<ServerCaptureTransport>();
    auto* transportPtr = transport.get();

    EXPECT_TRUE (server.start (std::move (transport)).wasOk());
    EXPECT_TRUE (server.isRunning());

    transportPtr->deliver (makeTestRequest (1, "initialize").toVar());

    ASSERT_EQ (1u, transportPtr->sentMessages.size());
    auto response = JsonRpcResponse::fromVar (transportPtr->sentMessages.front());
    ASSERT_TRUE (response.has_value());
    ASSERT_TRUE (response->result.has_value());
    EXPECT_EQ ("Test Server", (*response->result)["serverInfo"]["name"].toString());
    EXPECT_EQ ("2.0", (*response->result)["serverInfo"]["version"].toString());
}

TEST (YupAiMCPServer, ListsAndCallsRegisteredTool)
{
    MCPServer server;
    server.registerTool (makeEchoToolDefinition(), [] (const var& arguments)
    {
        return arguments["value"];
    });

    auto transport = std::make_unique<ServerCaptureTransport>();
    auto* transportPtr = transport.get();
    ASSERT_TRUE (server.start (std::move (transport)).wasOk());

    transportPtr->deliver (makeTestRequest (1, "tools/list").toVar());

    ASSERT_EQ (1u, transportPtr->sentMessages.size());
    auto listResponse = JsonRpcResponse::fromVar (transportPtr->sentMessages.back());
    ASSERT_TRUE (listResponse.has_value());
    ASSERT_TRUE (listResponse->result.has_value());
    EXPECT_EQ ("echo", (*listResponse->result)["tools"][0]["name"].toString());
    EXPECT_EQ ("string", (*listResponse->result)["tools"][0]["inputSchema"]["properties"]["value"]["type"].toString());

    transportPtr->deliver (makeTestRequest (2, "tools/call", JSON::parse (R"({
        "name": "echo",
        "arguments": { "value": "hello server" }
    })"))
                               .toVar());

    ASSERT_EQ (2u, transportPtr->sentMessages.size());
    auto callResponse = JsonRpcResponse::fromVar (transportPtr->sentMessages.back());
    ASSERT_TRUE (callResponse.has_value());
    ASSERT_TRUE (callResponse->result.has_value());
    EXPECT_EQ ("hello server", (*callResponse->result)["content"][0]["text"].toString());
}

TEST (YupAiMCPServer, RegistersLLMToolAndDerivesSchema)
{
    MCPServer server;

    LLMTool tool;
    tool.name = "set_gain";
    tool.description = "Sets gain.";
    tool.parameters.push_back ({ "gainDb", "number", "Gain in decibels.", true });
    tool.setHandler ([] (const var& arguments)
    {
        auto result = makeTestObject();
        setTestProperty (result, "gainDb", arguments["gainDb"]);
        return result;
    });

    server.registerTool (std::move (tool));

    auto transport = std::make_unique<ServerCaptureTransport>();
    auto* transportPtr = transport.get();
    ASSERT_TRUE (server.start (std::move (transport)).wasOk());

    transportPtr->deliver (makeTestRequest (1, "tools/list").toVar());

    ASSERT_EQ (1u, transportPtr->sentMessages.size());
    auto response = JsonRpcResponse::fromVar (transportPtr->sentMessages.back());
    ASSERT_TRUE (response.has_value());
    ASSERT_TRUE (response->result.has_value());
    EXPECT_EQ ("set_gain", (*response->result)["tools"][0]["name"].toString());
    EXPECT_EQ ("gainDb", (*response->result)["tools"][0]["inputSchema"]["required"][0].toString());
}

TEST (YupAiMCPServer, ListsAndReadsRegisteredResource)
{
    MCPServer server;

    MCPResourceDefinition resource;
    resource.uri = "yup://test/status";
    resource.name = "Status";
    resource.description = "Test status.";
    server.registerResource (resource, []
    {
        return String (R"({"running":true})");
    });

    auto transport = std::make_unique<ServerCaptureTransport>();
    auto* transportPtr = transport.get();
    ASSERT_TRUE (server.start (std::move (transport)).wasOk());

    transportPtr->deliver (makeTestRequest (1, "resources/list").toVar());

    ASSERT_EQ (1u, transportPtr->sentMessages.size());
    auto listResponse = JsonRpcResponse::fromVar (transportPtr->sentMessages.back());
    ASSERT_TRUE (listResponse.has_value());
    ASSERT_TRUE (listResponse->result.has_value());
    EXPECT_EQ ("yup://test/status", (*listResponse->result)["resources"][0]["uri"].toString());

    transportPtr->deliver (makeTestRequest (2, "resources/read", JSON::parse (R"({
        "uri": "yup://test/status"
    })"))
                               .toVar());

    ASSERT_EQ (2u, transportPtr->sentMessages.size());
    auto readResponse = JsonRpcResponse::fromVar (transportPtr->sentMessages.back());
    ASSERT_TRUE (readResponse.has_value());
    ASSERT_TRUE (readResponse->result.has_value());
    EXPECT_EQ (R"({"running":true})", (*readResponse->result)["contents"][0]["text"].toString());
}

TEST (YupAiMCPServer, ReturnsErrorsForUnknownMethodsAndResources)
{
    MCPServer server;
    auto transport = std::make_unique<ServerCaptureTransport>();
    auto* transportPtr = transport.get();
    ASSERT_TRUE (server.start (std::move (transport)).wasOk());

    transportPtr->deliver (makeTestRequest (1, "unknown/method").toVar());

    ASSERT_EQ (1u, transportPtr->sentMessages.size());
    auto methodResponse = JsonRpcResponse::fromVar (transportPtr->sentMessages.back());
    ASSERT_TRUE (methodResponse.has_value());
    ASSERT_TRUE (methodResponse->error.has_value());
    EXPECT_EQ (MCPErrorCodes::methodNotFound, methodResponse->error->code);

    transportPtr->deliver (makeTestRequest (2, "resources/read", JSON::parse (R"({
        "uri": "yup://missing"
    })"))
                               .toVar());

    ASSERT_EQ (2u, transportPtr->sentMessages.size());
    auto resourceResponse = JsonRpcResponse::fromVar (transportPtr->sentMessages.back());
    ASSERT_TRUE (resourceResponse.has_value());
    ASSERT_TRUE (resourceResponse->error.has_value());
    EXPECT_EQ (MCPErrorCodes::invalidParams, resourceResponse->error->code);
}

TEST (YupAiMCPServer, IgnoresNotifications)
{
    MCPServer server;
    auto transport = std::make_unique<ServerCaptureTransport>();
    auto* transportPtr = transport.get();
    ASSERT_TRUE (server.start (std::move (transport)).wasOk());

    JsonRpcRequest notification;
    notification.method = "notifications/initialized";
    transportPtr->deliver (notification.toVar());

    EXPECT_TRUE (transportPtr->sentMessages.empty());
}

TEST (YupAiMCPIntegration, ClientAndServerCommunicateOverLinkedTransports)
{
    auto clientTransport = std::make_unique<LinkedMCPTransport>();
    auto serverTransport = std::make_unique<LinkedMCPTransport>();
    auto* clientTransportPtr = clientTransport.get();
    auto* serverTransportPtr = serverTransport.get();

    clientTransportPtr->peer = serverTransportPtr;
    serverTransportPtr->peer = clientTransportPtr;

    MCPServer::Options options;
    options.serverName = "Linked Test Server";
    MCPServer server (options);

    server.registerTool (makeEchoToolDefinition(), [] (const var& arguments)
    {
        auto result = makeTestObject();
        setTestProperty (result, "echoed", arguments["value"]);
        return result;
    });

    MCPResourceDefinition resource;
    resource.uri = "yup://linked/status";
    resource.name = "Linked Status";
    resource.description = "Linked transport status.";
    server.registerResource (resource, []
    {
        return String ("linked-ok");
    });

    ASSERT_TRUE (server.start (std::move (serverTransport)).wasOk());

    MCPClient client (std::move (clientTransport));
    ASSERT_TRUE (client.initialize().wasOk());

    auto tools = client.listTools();
    ASSERT_EQ (1u, tools.size());
    EXPECT_EQ ("echo", tools.front().name);

    auto toolResult = client.callTool ("echo", JSON::parse (R"({"value":"round trip"})"));
    ASSERT_TRUE (toolResult.wasOk());
    EXPECT_EQ ("round trip", toolResult.getValue()["echoed"].toString());

    auto resources = client.listResources();
    ASSERT_EQ (1u, resources.size());
    EXPECT_EQ ("yup://linked/status", resources.front().uri);

    auto resourceResult = client.readResource ("yup://linked/status");
    ASSERT_TRUE (resourceResult.wasOk());
    EXPECT_EQ ("linked-ok", resourceResult.getValue());
}
