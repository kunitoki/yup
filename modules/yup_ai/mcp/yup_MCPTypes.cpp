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
var makeMCPObject()
{
    return var (std::make_unique<DynamicObject>());
}

void setMCPProperty (var& object, const Identifier& name, const var& value)
{
    if (auto* dynamicObject = object.getDynamicObject())
        dynamicObject->setProperty (name, value);
}

bool hasPresentProperty (const var& object, const Identifier& name)
{
    return object.hasProperty (name) && ! object[name].isUndefined();
}
} // namespace

var JsonRpcError::toVar() const
{
    auto object = makeMCPObject();
    setMCPProperty (object, "code", code);
    setMCPProperty (object, "message", message);

    if (data.has_value())
        setMCPProperty (object, "data", *data);

    return object;
}

std::optional<JsonRpcError> JsonRpcError::fromVar (const var& value)
{
    if (! value.isObject())
        return std::nullopt;

    JsonRpcError result;
    result.code = static_cast<int> (value["code"]);
    result.message = value["message"].toString();

    if (hasPresentProperty (value, "data"))
        result.data = value["data"];

    return result;
}

var JsonRpcRequest::toVar() const
{
    auto object = makeMCPObject();
    setMCPProperty (object, "jsonrpc", jsonrpc);
    setMCPProperty (object, "method", method);

    if (id.has_value())
        setMCPProperty (object, "id", *id);

    if (params.has_value())
        setMCPProperty (object, "params", *params);

    return object;
}

std::optional<JsonRpcRequest> JsonRpcRequest::fromVar (const var& value)
{
    if (! value.isObject())
        return std::nullopt;

    const auto version = value["jsonrpc"].toString();
    const auto method = value["method"].toString();
    if (version != "2.0" || method.isEmpty() || value.hasProperty ("result") || value.hasProperty ("error"))
        return std::nullopt;

    JsonRpcRequest result;
    result.jsonrpc = version;
    result.method = method;

    if (hasPresentProperty (value, "id"))
        result.id = value["id"];

    if (hasPresentProperty (value, "params"))
        result.params = value["params"];

    return result;
}

var JsonRpcResponse::toVar() const
{
    auto object = makeMCPObject();
    setMCPProperty (object, "jsonrpc", jsonrpc);
    setMCPProperty (object, "id", id);

    if (error.has_value())
        setMCPProperty (object, "error", error->toVar());
    else
        setMCPProperty (object, "result", result.value_or (var()));

    return object;
}

std::optional<JsonRpcResponse> JsonRpcResponse::fromVar (const var& value)
{
    if (! value.isObject())
        return std::nullopt;

    const auto version = value["jsonrpc"].toString();
    if (version != "2.0" || value.hasProperty ("method") || ! value.hasProperty ("id"))
        return std::nullopt;

    JsonRpcResponse response;
    response.jsonrpc = version;
    response.id = value["id"];

    if (hasPresentProperty (value, "error"))
    {
        auto parsedError = JsonRpcError::fromVar (value["error"]);
        if (! parsedError.has_value())
            return std::nullopt;

        response.error = std::move (*parsedError);
    }
    else if (hasPresentProperty (value, "result"))
    {
        response.result = value["result"];
    }
    else
    {
        return std::nullopt;
    }

    return response;
}

var MCPCapabilities::toVar() const
{
    auto object = makeMCPObject();

    if (supportsTools)
        setMCPProperty (object, "tools", makeMCPObject());

    if (supportsResources)
        setMCPProperty (object, "resources", makeMCPObject());

    if (supportsPrompts)
        setMCPProperty (object, "prompts", makeMCPObject());

    if (supportsLogging)
        setMCPProperty (object, "logging", makeMCPObject());

    return object;
}

MCPCapabilities MCPCapabilities::fromVar (const var& value)
{
    MCPCapabilities capabilities;

    if (! value.isObject())
        return capabilities;

    capabilities.supportsTools = hasPresentProperty (value, "tools");
    capabilities.supportsResources = hasPresentProperty (value, "resources");
    capabilities.supportsPrompts = hasPresentProperty (value, "prompts");
    capabilities.supportsLogging = hasPresentProperty (value, "logging");

    return capabilities;
}

var MCPToolDefinition::toVar() const
{
    auto object = makeMCPObject();
    setMCPProperty (object, "name", name);
    setMCPProperty (object, "description", description);
    setMCPProperty (object, "inputSchema", inputSchema);
    return object;
}

std::optional<MCPToolDefinition> MCPToolDefinition::fromVar (const var& value)
{
    if (! value.isObject())
        return std::nullopt;

    MCPToolDefinition result;
    result.name = value["name"].toString();
    result.description = value["description"].toString();
    result.inputSchema = value["inputSchema"];

    if (result.name.isEmpty())
        return std::nullopt;

    return result;
}

var MCPResourceDefinition::toVar() const
{
    auto object = makeMCPObject();
    setMCPProperty (object, "uri", uri);
    setMCPProperty (object, "name", name);
    setMCPProperty (object, "description", description);
    setMCPProperty (object, "mimeType", mimeType);
    return object;
}

std::optional<MCPResourceDefinition> MCPResourceDefinition::fromVar (const var& value)
{
    if (! value.isObject())
        return std::nullopt;

    MCPResourceDefinition result;
    result.uri = value["uri"].toString();
    result.name = value["name"].toString();
    result.description = value["description"].toString();
    result.mimeType = value["mimeType"].toString();

    if (result.mimeType.isEmpty())
        result.mimeType = "application/json";

    if (result.uri.isEmpty() || result.name.isEmpty())
        return std::nullopt;

    return result;
}

} // namespace yup
