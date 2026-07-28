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
/** MCP server that exposes local YUP tools and resources over an `MCPTransport`.

    The server handles JSON-RPC messages for initialization, `tools/list`,
    `tools/call`, `resources/list`, and `resources/read`.

    @tags{AI}
*/
class YUP_API MCPServer
{
public:
    struct Options
    {
        String serverName = "YUP Application";
        String serverVersion = "1.0.0";
        MCPCapabilities capabilities = {};
    };

    MCPServer();
    explicit MCPServer (Options options);
    ~MCPServer();

    /** Registers a tool definition and handler. */
    void registerTool (MCPToolDefinition tool, LLMTool::Handler handler);

    /** Registers an LLM tool, deriving the MCP tool definition from its JSON Schema. */
    void registerTool (LLMTool tool);

    /** Removes a tool by name. */
    void unregisterTool (const String& name);

    /** Registers a readable MCP resource. */
    void registerResource (MCPResourceDefinition resource, std::function<String()> reader);

    /** Removes a resource by URI. */
    void unregisterResource (const String& uri);

    /** Starts serving messages on the supplied transport. */
    Result start (std::unique_ptr<MCPTransport> transport);

    /** Stops serving and releases the transport. */
    void stop();

    /** Returns true while a transport is active. */
    bool isRunning() const noexcept;

    /** Convenience placeholder for future stdio transport support. */
    Result startStdio();

    /** Convenience placeholder for future HTTP transport support. */
    Result startHttp (int port);

private:
    struct Pimpl;
    std::unique_ptr<Pimpl> pimpl;
};

} // namespace yup
