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
/** JSON-RPC 2.0 and MCP error codes.

    @tags{AI}
*/
namespace MCPErrorCodes
{
constexpr int parseError = -32700;
constexpr int invalidRequest = -32600;
constexpr int methodNotFound = -32601;
constexpr int invalidParams = -32602;
constexpr int internalError = -32603;
} // namespace MCPErrorCodes

//==============================================================================
/** JSON-RPC 2.0 error object.

    @tags{AI}
*/
struct YUP_API JsonRpcError
{
    int code = MCPErrorCodes::internalError;
    String message;
    std::optional<var> data;

    /** Serialises this error to `{ "code", "message", "data" }`. */
    var toVar() const;

    /** Parses a JSON-RPC error object. */
    static std::optional<JsonRpcError> fromVar (const var& value);
};

//==============================================================================
/** JSON-RPC 2.0 request or notification envelope.

    Requests have an id. Notifications omit it.

    @tags{AI}
*/
struct YUP_API JsonRpcRequest
{
    String jsonrpc = "2.0";
    std::optional<var> id;
    String method;
    std::optional<var> params;

    /** Returns true if this message omits an id and therefore expects no response. */
    bool isNotification() const noexcept { return ! id.has_value(); }

    /** Serialises this request to a JSON-compatible object. */
    var toVar() const;

    /** Parses a JSON-RPC request or notification. */
    static std::optional<JsonRpcRequest> fromVar (const var& value);
};

//==============================================================================
/** JSON-RPC 2.0 response envelope.

    @tags{AI}
*/
struct YUP_API JsonRpcResponse
{
    String jsonrpc = "2.0";
    var id;
    std::optional<var> result;
    std::optional<JsonRpcError> error;

    /** Returns true if this response contains an error object. */
    bool isError() const noexcept { return error.has_value(); }

    /** Serialises this response to a JSON-compatible object. */
    var toVar() const;

    /** Parses a JSON-RPC response. */
    static std::optional<JsonRpcResponse> fromVar (const var& value);
};

//==============================================================================
/** MCP client or server capability flags.

    @tags{AI}
*/
struct YUP_API MCPCapabilities
{
    bool supportsTools = false;
    bool supportsResources = false;
    bool supportsPrompts = false;
    bool supportsLogging = false;

    /** Serialises this capability set to an MCP capabilities object. */
    var toVar() const;

    /** Parses an MCP capabilities object. */
    static MCPCapabilities fromVar (const var& value);
};

//==============================================================================
/** MCP tool definition returned by `tools/list`.

    @tags{AI}
*/
struct YUP_API MCPToolDefinition
{
    String name;
    String description;
    var inputSchema;

    /** Serialises this tool definition to MCP's `tools/list` shape. */
    var toVar() const;

    /** Parses an MCP tool definition. */
    static std::optional<MCPToolDefinition> fromVar (const var& value);
};

//==============================================================================
/** MCP resource definition returned by `resources/list`.

    @tags{AI}
*/
struct YUP_API MCPResourceDefinition
{
    String uri;
    String name;
    String description;
    String mimeType = "application/json";

    /** Serialises this resource definition to MCP's `resources/list` shape. */
    var toVar() const;

    /** Parses an MCP resource definition. */
    static std::optional<MCPResourceDefinition> fromVar (const var& value);
};

} // namespace yup
