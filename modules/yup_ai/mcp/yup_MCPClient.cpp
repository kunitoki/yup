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
namespace
{
var makeMCPClientObject()
{
    return var (std::make_unique<DynamicObject>());
}

void setMCPClientProperty (var& object, const Identifier& name, const var& value)
{
    if (auto* dynamicObject = object.getDynamicObject())
        dynamicObject->setProperty (name, value);
}

var makeRequestParamsWithNameAndArguments (const String& toolName, const var& arguments)
{
    auto params = makeMCPClientObject();
    setMCPClientProperty (params, "name", toolName);
    setMCPClientProperty (params, "arguments", arguments);
    return params;
}

var makeResourceReadParams (const String& uri)
{
    auto params = makeMCPClientObject();
    setMCPClientProperty (params, "uri", uri);
    return params;
}

var makeInitializeParams (const MCPCapabilities& capabilities)
{
    auto params = makeMCPClientObject();
    setMCPClientProperty (params, "protocolVersion", "2024-11-05");
    setMCPClientProperty (params, "capabilities", capabilities.toVar());

    auto clientInfo = makeMCPClientObject();
    setMCPClientProperty (clientInfo, "name", "YUP");
    setMCPClientProperty (clientInfo, "version", "1.0.0");
    setMCPClientProperty (params, "clientInfo", clientInfo);

    return params;
}

var unwrapToolCallResult (const var& result)
{
    if (auto* content = result["content"].getArray())
    {
        if (content->isEmpty())
            return {};

        const auto& firstContent = content->getReference (0);

        if (firstContent["type"].toString() == "text")
            return firstContent["text"];

        if (! firstContent["json"].isVoid())
            return firstContent["json"];
    }

    return result;
}

ResultValue<String> unwrapResourceReadResult (const var& result)
{
    if (auto* contents = result["contents"].getArray())
    {
        if (contents->isEmpty())
            return makeResultValueFail ("MCP resource response did not contain content");

        const auto& firstContent = contents->getReference (0);
        if (firstContent.hasProperty ("text"))
            return makeResultValueOk (firstContent["text"].toString());

        if (firstContent.hasProperty ("blob"))
            return makeResultValueOk (firstContent["blob"].toString());
    }

    if (result.isString())
        return makeResultValueOk (result.toString());

    return makeResultValueFail ("MCP resource response did not contain readable text");
}

bool schemaMarksParameterRequired (const var& schema, const String& parameterName)
{
    if (auto* required = schema["required"].getArray())
        for (const auto& requiredName : *required)
            if (parameterName == requiredName.toString())
                return true;

    return false;
}

std::optional<std::vector<LLMTool::Parameter>> schemaPropertiesToParameters (const var& schema);

LLMTool::Parameter schemaPropertyToParameter (const Identifier& name, const var& schema, bool required)
{
    LLMTool::Parameter parameter;
    parameter.name = name.toString();
    parameter.type = schema["type"].toString();
    parameter.description = schema["description"].toString();
    parameter.required = required;

    if (schema.hasProperty ("enum"))
        parameter.enumValues = schema["enum"];

    if (schema.hasProperty ("default"))
        parameter.defaultValue = schema["default"];

    if (auto nestedProperties = schemaPropertiesToParameters (schema); nestedProperties.has_value())
        parameter.properties = std::move (*nestedProperties);
    else if (auto nestedItems = schemaPropertiesToParameters (schema["items"]); nestedItems.has_value())
        parameter.properties = std::move (*nestedItems);

    return parameter;
}

std::optional<std::vector<LLMTool::Parameter>> schemaPropertiesToParameters (const var& schema)
{
    auto* properties = schema["properties"].getDynamicObject();
    if (properties == nullptr)
        return std::nullopt;

    std::vector<LLMTool::Parameter> parameters;

    for (const auto& property : properties->getProperties())
    {
        const auto propertyName = property.name.toString();
        parameters.push_back (schemaPropertyToParameter (property.name,
                                                         property.value,
                                                         schemaMarksParameterRequired (schema, propertyName)));
    }

    return parameters;
}
} // namespace

struct MCPClient::Pimpl
{
    explicit Pimpl (std::unique_ptr<MCPTransport> transportToUse)
        : transport (std::move (transportToUse))
    {
    }

    ResultValue<JsonRpcResponse> sendRequest (const String& method, std::optional<var> params)
    {
        if (transport == nullptr)
            return makeResultValueFail ("MCP client has no transport");

        if (! transport->isConnected())
        {
            if (auto startResult = transport->start(); startResult.failed())
                return makeResultValueFail (startResult.getErrorMessage());
        }

        JsonRpcRequest request;
        request.id = static_cast<int> (nextRequestId++);
        request.method = method;
        request.params = std::move (params);

        if (auto sendResult = transport->sendMessage (request.toVar()); sendResult.failed())
            return makeResultValueFail (sendResult.getErrorMessage());

        for (;;)
        {
            auto received = transport->receiveMessage();
            if (received.failed())
                return makeResultValueFail (received.getErrorMessage());

            auto response = JsonRpcResponse::fromVar (received.getReference());
            if (! response.has_value())
                continue;

            if (response->id.equals (*request.id))
                return makeResultValueOk (std::move (*response));
        }
    }

    Result sendNotification (const String& method, std::optional<var> params)
    {
        if (transport == nullptr)
            return Result::fail ("MCP client has no transport");

        JsonRpcRequest notification;
        notification.method = method;
        notification.params = std::move (params);

        return transport->sendMessage (notification.toVar());
    }

    std::unique_ptr<MCPTransport> transport;
    int64 nextRequestId = 1;
};

MCPClient::MCPClient (std::unique_ptr<MCPTransport> transport)
    : pimpl (std::make_unique<Pimpl> (std::move (transport)))
{
}

MCPClient::~MCPClient() = default;

Result MCPClient::initialize (MCPCapabilities clientCapabilities)
{
    auto response = pimpl->sendRequest ("initialize", makeInitializeParams (clientCapabilities));
    if (response.failed())
        return Result::fail (response.getErrorMessage());

    if (response.getReference().isError())
        return Result::fail (response.getReference().error->message);

    return pimpl->sendNotification ("notifications/initialized", std::nullopt);
}

std::vector<MCPToolDefinition> MCPClient::listTools()
{
    std::vector<MCPToolDefinition> result;

    auto response = pimpl->sendRequest ("tools/list", std::nullopt);
    if (response.failed() || response.getReference().isError() || ! response.getReference().result.has_value())
        return result;

    if (auto* tools = (*response.getReference().result)["tools"].getArray())
        for (const auto& toolVar : *tools)
            if (auto tool = MCPToolDefinition::fromVar (toolVar))
                result.push_back (std::move (*tool));

    return result;
}

ResultValue<var> MCPClient::callTool (const String& toolName, const var& arguments)
{
    auto response = pimpl->sendRequest ("tools/call", makeRequestParamsWithNameAndArguments (toolName, arguments));
    if (response.failed())
        return makeResultValueFail (response.getErrorMessage());

    if (response.getReference().isError())
        return makeResultValueFail (response.getReference().error->message);

    if (! response.getReference().result.has_value())
        return makeResultValueFail ("MCP tool call response did not contain a result");

    return makeResultValueOk (unwrapToolCallResult (*response.getReference().result));
}

std::vector<MCPResourceDefinition> MCPClient::listResources()
{
    std::vector<MCPResourceDefinition> result;

    auto response = pimpl->sendRequest ("resources/list", std::nullopt);
    if (response.failed() || response.getReference().isError() || ! response.getReference().result.has_value())
        return result;

    if (auto* resources = (*response.getReference().result)["resources"].getArray())
        for (const auto& resourceVar : *resources)
            if (auto resource = MCPResourceDefinition::fromVar (resourceVar))
                result.push_back (std::move (*resource));

    return result;
}

ResultValue<String> MCPClient::readResource (const String& uri)
{
    auto response = pimpl->sendRequest ("resources/read", makeResourceReadParams (uri));
    if (response.failed())
        return makeResultValueFail (response.getErrorMessage());

    if (response.getReference().isError())
        return makeResultValueFail (response.getReference().error->message);

    if (! response.getReference().result.has_value())
        return makeResultValueFail ("MCP resource response did not contain a result");

    return unwrapResourceReadResult (*response.getReference().result);
}

void MCPClient::registerToolsWith (LLMToolRegistry& registry)
{
    for (auto toolDefinition : listTools())
    {
        LLMTool tool;
        tool.name = toolDefinition.name;
        tool.description = toolDefinition.description;

        if (auto parameters = schemaPropertiesToParameters (toolDefinition.inputSchema))
            tool.parameters = std::move (*parameters);

        tool.setHandler ([this, toolName = tool.name] (const var& arguments)
        {
            auto callResult = callTool (toolName, arguments);
            return callResult.wasOk() ? callResult.getValue()
                                      : var (callResult.getErrorMessage());
        });

        registry.registerTool (std::move (tool));
    }
}

MCPTransport* MCPClient::getTransport() noexcept
{
    return pimpl->transport.get();
}

} // namespace yup
