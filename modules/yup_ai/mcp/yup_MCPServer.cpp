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
var makeMCPServerObject()
{
    return var (std::make_unique<DynamicObject>());
}

void setMCPServerProperty (var& object, const Identifier& name, const var& value)
{
    if (auto* dynamicObject = object.getDynamicObject())
        dynamicObject->setProperty (name, value);
}

JsonRpcResponse makeMCPErrorResponse (const var& id, int code, const String& message)
{
    JsonRpcResponse response;
    response.id = id;
    response.error = JsonRpcError { code, message, std::nullopt };
    return response;
}

bool isSupportedProtocolVersion (const String& version)
{
    return version == "2025-06-18" || version == "2025-03-26" || version == "2024-11-05";
}

MCPToolDefinition toolDefinitionFromLLMTool (const LLMTool& tool)
{
    auto schema = tool.toJsonSchema();

    MCPToolDefinition definition;
    definition.name = tool.name;
    definition.description = tool.description;
    definition.inputSchema = schema["function"]["parameters"];
    return definition;
}
} // namespace

struct MCPServer::Pimpl
{
    struct ResourceEntry
    {
        MCPResourceDefinition definition;
        std::function<String()> reader;
    };

    explicit Pimpl (Options optionsToUse)
        : options (std::move (optionsToUse))
    {
    }

    struct ToolEntry
    {
        MCPToolDefinition definition;
        ToolHandler handler;
    };

    void registerTool (MCPToolDefinition definition, ToolHandler handler)
    {
        const ScopedLock lock (mutex);
        const auto name = definition.name;
        tools[name] = ToolEntry { std::move (definition), std::move (handler) };
        options.capabilities.supportsTools = true;
    }

    void sendResponse (const JsonRpcResponse& response)
    {
        auto* currentTransport = transport.get();
        if (currentTransport != nullptr)
            currentTransport->sendMessage (response.toVar());
    }

    var makeInitializeResult (const var& params) const
    {
        const auto requestedVersion = params["protocolVersion"].toString();

        auto result = makeMCPServerObject();
        setMCPServerProperty (result, "protocolVersion", isSupportedProtocolVersion (requestedVersion) ? requestedVersion : String ("2025-06-18"));
        setMCPServerProperty (result, "capabilities", options.capabilities.toVar());

        auto serverInfo = makeMCPServerObject();
        setMCPServerProperty (serverInfo, "name", options.serverName);
        setMCPServerProperty (serverInfo, "version", options.serverVersion);
        setMCPServerProperty (result, "serverInfo", serverInfo);

        if (options.instructions.isNotEmpty())
            setMCPServerProperty (result, "instructions", options.instructions);

        return result;
    }

    var makeToolsListResult() const
    {
        Array<var> toolList;

        {
            const ScopedLock lock (mutex);
            for (const auto& entry : tools)
                toolList.add (entry.second.definition.toVar());
        }

        auto result = makeMCPServerObject();
        setMCPServerProperty (result, "tools", var (std::move (toolList)));
        return result;
    }

    var callTool (const var& params) const
    {
        const auto toolName = params["name"].toString();
        if (toolName.isEmpty())
            return MCPToolResult::fromError ("Missing MCP tool name").toVar();

        ToolHandler handler;
        {
            const ScopedLock lock (mutex);
            if (auto iter = tools.find (toolName); iter != tools.end())
                handler = iter->second.handler;
        }

        if (! handler)
            return MCPToolResult::fromError ("Unknown MCP tool '" + toolName + "'").toVar();

        return handler (params["arguments"]).toVar();
    }

    var makeResourcesListResult() const
    {
        Array<var> resources;

        {
            const ScopedLock lock (mutex);
            for (const auto& entry : resourcesByUri)
                resources.add (entry.second.definition.toVar());
        }

        auto result = makeMCPServerObject();
        setMCPServerProperty (result, "resources", var (std::move (resources)));
        return result;
    }

    ResultValue<var> readResource (const var& params) const
    {
        const auto uri = params["uri"].toString();

        ResourceEntry entry;
        {
            const ScopedLock lock (mutex);
            auto iter = resourcesByUri.find (uri);
            if (iter == resourcesByUri.end())
                return makeResultValueFail ("Unknown MCP resource '" + uri + "'");

            entry = iter->second;
        }

        auto content = makeMCPServerObject();
        setMCPServerProperty (content, "uri", entry.definition.uri);
        setMCPServerProperty (content, "mimeType", entry.definition.mimeType);
        setMCPServerProperty (content, "text", entry.reader ? entry.reader() : String());

        var contents;
        contents.append (content);

        auto result = makeMCPServerObject();
        setMCPServerProperty (result, "contents", contents);
        return makeResultValueOk (result);
    }

    std::optional<JsonRpcResponse> handleRequest (const JsonRpcRequest& request)
    {
        if (request.isNotification())
            return std::nullopt;

        JsonRpcResponse response;
        response.id = *request.id;

        if (request.method == "initialize")
            response.result = makeInitializeResult (request.params.value_or (var()));
        else if (request.method == "ping")
            response.result = makeMCPServerObject();
        else if (request.method == "tools/list")
            response.result = makeToolsListResult();
        else if (request.method == "tools/call")
            response.result = callTool (request.params.value_or (var()));
        else if (request.method == "resources/list")
            response.result = makeResourcesListResult();
        else if (request.method == "resources/read")
        {
            auto result = readResource (request.params.value_or (var()));
            if (result.failed())
                return makeMCPErrorResponse (*request.id, MCPErrorCodes::invalidParams, result.getErrorMessage());

            response.result = result.getValue();
        }
        else
        {
            response.error = JsonRpcError { MCPErrorCodes::methodNotFound, "Unknown MCP method '" + request.method + "'", std::nullopt };
        }

        return response;
    }

    void handleMessage (const var& message)
    {
        auto request = JsonRpcRequest::fromVar (message);
        if (! request.has_value())
        {
            sendResponse (makeMCPErrorResponse (message["id"], MCPErrorCodes::invalidRequest, "Invalid JSON-RPC request"));
            return;
        }

        if (auto response = handleRequest (*request))
            sendResponse (*response);
    }

    Options options;
    mutable CriticalSection mutex;
    std::unordered_map<String, ToolEntry> tools;
    std::unordered_map<String, ResourceEntry> resourcesByUri;
    std::unique_ptr<MCPTransport> transport;
    bool running = false;
};

MCPServer::MCPServer()
    : MCPServer (Options {})
{
}

MCPServer::MCPServer (Options options)
    : pimpl (std::make_unique<Pimpl> (std::move (options)))
{
}

MCPServer::~MCPServer()
{
    stop();
}

void MCPServer::registerTool (MCPToolDefinition tool, LLMTool::Handler handler)
{
    if (! handler)
    {
        pimpl->registerTool (std::move (tool), nullptr);
        return;
    }

    pimpl->registerTool (std::move (tool), [handler = std::move (handler)] (const var& arguments)
    {
        return MCPToolResult::fromValue (handler (arguments));
    });
}

void MCPServer::registerTool (MCPToolDefinition tool, ToolHandler handler)
{
    pimpl->registerTool (std::move (tool), std::move (handler));
}

void MCPServer::registerTool (LLMTool tool)
{
    auto definition = toolDefinitionFromLLMTool (tool);
    pimpl->registerTool (std::move (definition), [tool = std::move (tool)] (const var& arguments)
    {
        return MCPToolResult::fromValue (tool.execute (arguments));
    });
}

void MCPServer::unregisterTool (const String& name)
{
    const ScopedLock lock (pimpl->mutex);
    pimpl->tools.erase (name);
}

void MCPServer::registerResource (MCPResourceDefinition resource, std::function<String()> reader)
{
    const ScopedLock lock (pimpl->mutex);
    const auto uri = resource.uri;
    pimpl->resourcesByUri[uri] = Pimpl::ResourceEntry { std::move (resource), std::move (reader) };
    pimpl->options.capabilities.supportsResources = true;
}

void MCPServer::unregisterResource (const String& uri)
{
    const ScopedLock lock (pimpl->mutex);
    pimpl->resourcesByUri.erase (uri);
}

Result MCPServer::start (std::unique_ptr<MCPTransport> transport)
{
    if (transport == nullptr)
        return Result::fail ("Cannot start MCP server without a transport");

    stop();

    pimpl->transport = std::move (transport);
    pimpl->transport->setMessageHandler ([this] (const var& message)
    {
        pimpl->handleMessage (message);
    });

    auto result = pimpl->transport->start();
    if (result.failed())
    {
        pimpl->transport.reset();
        return result;
    }

    pimpl->running = true;
    return Result::ok();
}

void MCPServer::stop()
{
    if (pimpl->transport != nullptr)
    {
        pimpl->transport->stop();
        pimpl->transport.reset();
    }

    pimpl->running = false;
}

bool MCPServer::isRunning() const noexcept
{
    return pimpl->running;
}

Result MCPServer::startStdio()
{
    return Result::fail ("MCP stdio transport is not implemented yet");
}

Result MCPServer::startHttp (int port)
{
    MCPHttpServerTransport::Options transportOptions;
    transportOptions.port = port;
    return start (std::make_unique<MCPHttpServerTransport> (transportOptions));
}

} // namespace yup
