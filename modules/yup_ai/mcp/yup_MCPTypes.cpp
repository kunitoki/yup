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

    if (inputSchema.isObject())
    {
        setMCPProperty (object, "inputSchema", inputSchema);
    }
    else
    {
        auto emptySchema = makeMCPObject();
        setMCPProperty (emptySchema, "type", "object");
        setMCPProperty (object, "inputSchema", emptySchema);
    }

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

//==============================================================================
MCPContent MCPContent::makeText (const String& text)
{
    MCPContent content;
    content.text = text;
    return content;
}

MCPContent MCPContent::makeImage (const MemoryBlock& encodedImage, const String& mimeType)
{
    MCPContent content;
    content.type = Type::image;
    content.data = Base64::toBase64 (encodedImage.getData(), encodedImage.getSize());
    content.mimeType = mimeType;
    return content;
}

MCPContent MCPContent::makeAudio (const MemoryBlock& encodedAudio, const String& mimeType)
{
    auto content = makeImage (encodedAudio, mimeType);
    content.type = Type::audio;
    return content;
}

var MCPContent::toVar() const
{
    auto object = makeMCPObject();

    if (type == Type::text)
    {
        setMCPProperty (object, "type", "text");
        setMCPProperty (object, "text", text);
        return object;
    }

    setMCPProperty (object, "type", type == Type::image ? "image" : "audio");
    setMCPProperty (object, "data", data);
    setMCPProperty (object, "mimeType", mimeType);
    return object;
}

std::optional<MCPContent> MCPContent::fromVar (const var& value)
{
    if (! value.isObject())
        return std::nullopt;

    const auto typeName = value["type"].toString();
    if (typeName == "text")
        return makeText (value["text"].toString());

    if (typeName != "image" && typeName != "audio")
        return std::nullopt;

    MCPContent content;
    content.type = typeName == "image" ? Type::image : Type::audio;
    content.data = value["data"].toString();
    content.mimeType = value["mimeType"].toString();
    return content;
}

//==============================================================================
MCPToolResult MCPToolResult::fromText (const String& text)
{
    MCPToolResult result;
    result.content.push_back (MCPContent::makeText (text));
    return result;
}

MCPToolResult MCPToolResult::fromJson (const var& value)
{
    auto result = fromText (JSON::toString (value, true));

    if (value.getDynamicObject() != nullptr)
        result.structuredContent = value;

    return result;
}

MCPToolResult MCPToolResult::fromValue (const var& value)
{
    return value.isString() ? fromText (value.toString()) : fromJson (value);
}

MCPToolResult MCPToolResult::fromError (const String& message)
{
    auto result = fromText (message);
    result.isError = true;
    return result;
}

var MCPToolResult::toVar() const
{
    Array<var> contentArray;

    for (const auto& block : content)
        contentArray.add (block.toVar());

    auto object = makeMCPObject();
    setMCPProperty (object, "content", var (std::move (contentArray)));

    if (structuredContent.getDynamicObject() != nullptr)
        setMCPProperty (object, "structuredContent", structuredContent);

    if (isError)
        setMCPProperty (object, "isError", true);

    return object;
}

std::optional<MCPToolResult> MCPToolResult::fromVar (const var& value)
{
    if (! value.isObject())
        return std::nullopt;

    MCPToolResult result;
    result.isError = static_cast<bool> (value["isError"]);

    if (hasPresentProperty (value, "structuredContent"))
        result.structuredContent = value["structuredContent"];

    if (auto* blocks = value["content"].getArray())
        for (const auto& block : *blocks)
            if (auto parsed = MCPContent::fromVar (block))
                result.content.push_back (std::move (*parsed));

    return result;
}

} // namespace yup
