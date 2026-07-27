import yup

#==================================================================================================

def test_server_options_are_exposed():
    options = yup.ai.MCPServerOptions()
    options.serverName = "Python MCP Server"
    options.serverVersion = "2.1"
    options.capabilities.supportsTools = True

    server = yup.ai.MCPServer(options)

    assert not server.isRunning()

#==================================================================================================

def test_server_accepts_python_tool_and_resource_callbacks():
    server = yup.ai.MCPServer()

    tool = yup.ai.MCPToolDefinition()
    tool.name = "echo"
    tool.description = "Echoes text."
    tool.inputSchema = {
        "type": "object",
        "properties": {
            "value": { "type": "string" },
        },
        "required": [ "value" ],
    }

    server.registerTool(tool, lambda arguments: { "echoed": arguments["value"] })

    resource = yup.ai.MCPResourceDefinition()
    resource.uri = "yup://python/status"
    resource.name = "Status"
    resource.description = "Python status."

    server.registerResource(resource, lambda: '{"ok":true}')

    assert not server.isRunning()

