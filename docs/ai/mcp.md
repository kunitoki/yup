# MCP (Model Context Protocol)

The `yup_ai` module includes a complete [Model Context
Protocol](https://modelcontextprotocol.io/) implementation — JSON-RPC 2.0
types, an abstract transport layer, and client/server classes for tool and
resource bridging.

## Architecture

```
┌──────────────┐    JSON-RPC 2.0     ┌──────────────┐
│  MCPClient   │◄──────────────────►│  MCPServer   │
└──────┬───────┘    over transport   └──────┬───────┘
       │                                    │
       │  sendMessage() / receiveMessage()  │
       ▼                                    ▼
┌──────────────┐                    ┌──────────────┐
│ MCPTransport │                    │ MCPTransport │
│  (stdio,     │                    │  (stdio,     │
│   HTTP/SSE,  │                    │   HTTP/SSE,  │
│   inproc)    │                    │   inproc)    │
└──────────────┘                    └──────────────┘
```

## JSON-RPC 2.0 types

The foundation is standard JSON-RPC 2.0:

```cpp
#include <yup_ai/yup_ai.h>

// Build a request
yup::JsonRpcRequest req;
req.id     = yup::var (1);
req.method = "tools/list";
req.params = yup::var();  // no params

auto json = req.toVar();  // → {"jsonrpc":"2.0","id":1,"method":"tools/list"}

// Parse a response from JSON
auto resp = yup::JsonRpcResponse::fromVar (responseJson);
if (resp && ! resp->isError())
    DBG (JSON::toString (resp->result.value()));
```

Notifications omit the `id` field and expect no response:

```cpp
yup::JsonRpcRequest notification;
notification.method = "notifications/initialized";
// notification.id remains std::nullopt
assert (notification.isNotification());
```

## MCPTypes

Protocol-level types used by both client and server:

| Type | Description |
|------|-------------|
| `MCPCapabilities` | Capability flags: `supportsTools`, `supportsResources`, `supportsPrompts`, `supportsLogging` |
| `MCPToolDefinition` | A tool's name, description, and `inputSchema` (JSON Schema) |
| `MCPResourceDefinition` | A resource's URI, name, description, and MIME type |

## MCPTransport

`MCPTransport` is the abstract interface connecting client and server.
Implementations handle the wire format (stdio, HTTP, in-process queues).

```cpp
class MCPTransport
{
public:
    virtual Result sendMessage (const var& message) = 0;
    virtual ResultValue<var> receiveMessage (int timeoutMs = -1) = 0;
    virtual void setMessageHandler (MessageHandler handler) = 0;
    virtual Result start() = 0;
    virtual void stop() = 0;
    virtual bool isConnected() const noexcept = 0;
};
```

```{note}
The transport only moves JSON-compatible `var` objects. Serialisation to/from
the wire format is the transport's responsibility.
```

## MCPClient

A synchronous client that connects to an MCP server through a transport.
It handles the `initialize` handshake and exposes common MCP methods:

```cpp
// Connect via stdio or in-process transport
auto transport = std::make_unique<MyStdioTransport> ("my-server --mcp");
yup::MCPClient client (std::move (transport));

// Perform the MCP handshake
auto result = client.initialize();
if (result.failed())
{
    DBG ("MCP handshake failed: " << result.getErrorMessage());
    return;
}

// List and call tools
auto tools = client.listTools();
for (auto& tool : tools)
    DBG (tool.name << ": " << tool.description);

auto answer = client.callTool ("get_weather", yup::DynamicObject::Ptr (
    new yup::DynamicObject ({ { "city", "Rome" } })));
if (answer.wasOk())
    DBG (JSON::toString (*answer));

// List and read resources
auto resources = client.listResources();
auto content = client.readResource ("file:///tmp/data.json");
```

### Bridge to LLM tools

`registerToolsWith()` imports remote MCP tools into a local `LLMToolRegistry`:

```cpp
yup::LLMToolRegistry registry;
client.registerToolsWith (registry);

// Now use the registry with any LLMClient
auto response = llmClient->chatWithTools ("What's the weather?", registry);
```

## MCPServer

Expose local YUP tools and resources to MCP clients. Register handlers, then
start serving on a transport:

```cpp
yup::MCPServer::Options opts;
opts.serverName    = "Weather Service";
opts.serverVersion = "1.0.0";
opts.capabilities  = { .supportsTools = true, .supportsResources = true };

yup::MCPServer server (opts);

// Register an MCP tool from a definition + handler
yup::MCPToolDefinition tool;
tool.name        = "get_weather";
tool.description = "Get current weather for a city";
tool.inputSchema = yup::LLMSchema::object ({
    { "city", yup::LLMSchema::string() },
});
server.registerTool (tool, [](const yup::var& args) -> yup::var
{
    auto city = args["city"].toString();
    // ... fetch weather ...
    return yup::DynamicObject::Ptr (new yup::DynamicObject ({
        { "temperature", 22 }
    }));
});

// Or register an LLMTool directly (MCP definition derived from JSON Schema)
yup::LLMTool myTool;
myTool.name        = "greet";
myTool.description = "Return a greeting";
myTool.parameters  = { { "name", "string", "Person's name", true } };
myTool.setHandler ([](const yup::var& args)
{
    return yup::var ("Hello, " + args["name"].toString() + "!");
});
server.registerTool (std::move (myTool));

// Register a readable resource
yup::MCPResourceDefinition resource;
resource.uri        = "config://app";
resource.name       = "Application Config";
resource.mimeType   = "application/json";
server.registerResource (resource, []() -> yup::String
{
    return R"({"theme": "dark", "language": "en"})";
});

// Start serving
auto transport = std::make_unique<MyTransport>();
server.start (std::move (transport));
```

## Lifecycle

The MCP lifecycle follows three phases:

1. **Initialization** — client sends `initialize` request with capabilities;
   server responds with its own capabilities. Client sends `notifications/initialized`.
2. **Operation** — normal message exchange: `tools/list`, `tools/call`,
   `resources/list`, `resources/read`, etc.
3. **Shutdown** — transport closes or `stop()` is called.

```{note}
`MCPClient::initialize()` performs the full handshake automatically. After a
successful `initialize()`, the client is ready for operation.
```
