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
/** Synchronous MCP client over an `MCPTransport`.

    The client performs JSON-RPC request/response correlation and exposes common
    MCP methods for initialization, tool discovery, tool calls, and resources.

    @tags{AI}
*/
class YUP_API MCPClient
{
public:
    /** Connects this client to an MCP server using the supplied transport. */
    explicit MCPClient (std::unique_ptr<MCPTransport> transport);
    ~MCPClient();

    /** Performs the MCP `initialize` handshake and sends the initialized notification. */
    Result initialize (MCPCapabilities clientCapabilities = {});

    /** Requests the server's available tools. */
    std::vector<MCPToolDefinition> listTools();

    /** Calls a server tool with JSON-compatible arguments. */
    ResultValue<var> callTool (const String& toolName, const var& arguments);

    /** Requests the server's available resources. */
    std::vector<MCPResourceDefinition> listResources();

    /** Reads a resource by URI, returning text content when available. */
    ResultValue<String> readResource (const String& uri);

    /** Imports remote MCP tools into an LLM tool registry. */
    void registerToolsWith (LLMToolRegistry& registry);

    /** Returns the underlying transport. */
    MCPTransport* getTransport() noexcept;

private:
    struct Pimpl;
    std::unique_ptr<Pimpl> pimpl;
};

} // namespace yup
