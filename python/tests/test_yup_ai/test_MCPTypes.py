import yup

#==================================================================================================

def test_json_rpc_request_round_trip():
    request = yup.ai.JsonRpcRequest()
    request.id = 42
    request.method = "tools/call"
    request.params = {
        "name": "echo",
        "arguments": {
            "value": "hello",
        },
    }

    parsed = yup.ai.JsonRpcRequest.fromVar(request.toVar())

    assert parsed is not None
    assert not parsed.isNotification()
    assert parsed.jsonrpc == "2.0"
    assert parsed.id == 42
    assert parsed.method == "tools/call"
    assert parsed.params["name"] == "echo"
    assert parsed.params["arguments"]["value"] == "hello"

#==================================================================================================

def test_json_rpc_notification_has_no_id():
    parsed = yup.ai.JsonRpcRequest.fromVar({
        "jsonrpc": "2.0",
        "method": "notifications/initialized",
    })

    assert parsed is not None
    assert parsed.isNotification()
    assert parsed.method == "notifications/initialized"

#==================================================================================================

def test_json_rpc_error_response_round_trip():
    error = yup.ai.JsonRpcError()
    error.code = yup.ai.MCP_METHOD_NOT_FOUND
    error.message = "Missing method"
    error.data = { "method": "missing" }

    response = yup.ai.JsonRpcResponse()
    response.id = "abc"
    response.error = error

    parsed = yup.ai.JsonRpcResponse.fromVar(response.toVar())

    assert parsed is not None
    assert parsed.isError()
    assert parsed.id == "abc"
    assert parsed.error.code == yup.ai.MCP_METHOD_NOT_FOUND
    assert parsed.error.message == "Missing method"
    assert parsed.error.data["method"] == "missing"

#==================================================================================================

def test_capabilities_round_trip():
    capabilities = yup.ai.MCPCapabilities()
    capabilities.supportsTools = True
    capabilities.supportsResources = True

    parsed = yup.ai.MCPCapabilities.fromVar(capabilities.toVar())

    assert parsed.supportsTools
    assert parsed.supportsResources
    assert not parsed.supportsPrompts
    assert not parsed.supportsLogging

#==================================================================================================

def test_tool_definition_round_trip():
    tool = yup.ai.MCPToolDefinition()
    tool.name = "set_gain"
    tool.description = "Sets gain."
    tool.inputSchema = {
        "type": "object",
        "properties": {
            "gainDb": {
                "type": "number",
                "description": "Gain in decibels.",
            },
        },
        "required": [ "gainDb" ],
    }

    parsed = yup.ai.MCPToolDefinition.fromVar(tool.toVar())

    assert parsed is not None
    assert parsed.name == "set_gain"
    assert parsed.inputSchema["properties"]["gainDb"]["type"] == "number"
    assert parsed.inputSchema["required"][0] == "gainDb"

#==================================================================================================

def test_resource_definition_round_trip_defaults_mime_type():
    resource = yup.ai.MCPResourceDefinition()
    resource.uri = "yup://test/status"
    resource.name = "Status"
    resource.description = "Current status."

    parsed = yup.ai.MCPResourceDefinition.fromVar(resource.toVar())

    assert parsed is not None
    assert parsed.uri == "yup://test/status"
    assert parsed.name == "Status"
    assert parsed.mimeType == "application/json"

