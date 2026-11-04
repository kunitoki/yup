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
│  (stdio,     │                    │  (HTTP,      │
│   HTTP,      │                    │   stdio,     │
│   inproc)    │                    │   inproc)    │
└──────────────┘                    └──────────────┘
```

The server speaks MCP protocol versions `2025-06-18`, `2025-03-26` and
`2024-11-05`, answering with the version the client asks for, or the newest one.

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
    YUP_DBG (JSON::toString (resp->result.value()));
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
| `MCPContent` | One block of a tool result: text, or base64 `image` or `audio` data with its MIME type |
| `MCPToolResult` | A tool result: content blocks, optional `structuredContent` and the `isError` flag |

```cpp
// Text, JSON (as text and structured content), an error the model can read
auto text  = yup::MCPToolResult::fromText ("Done");
auto json  = yup::MCPToolResult::fromJson (someObject);
auto error = yup::MCPToolResult::fromError ("No such preset");

// Binary content: a PNG screenshot or a WAV file, base64 encoded
auto result = yup::MCPToolResult::fromJson (details);
result.content.push_back (yup::MCPContent::makeImage (pngBytes, "image/png"));
result.content.push_back (yup::MCPContent::makeAudio (wavBytes, "audio/wav"));
```

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

### MCPHttpServerTransport

A server transport for the MCP Streamable HTTP transport. Clients `POST` JSON-RPC
messages (or batches) to one endpoint and get the answer as `application/json`;
notifications get `202 Accepted`. It never streams, so `GET` answers `405`.

- Listens on `127.0.0.1` by default and rejects requests whose `Host` or `Origin`
  is not a loopback address, guarding against DNS rebinding.
- Tries `numPortsToTry` consecutive ports from `port`, so several instances can
  run at once; `port = 0` picks any free port.
- Serves requests on a small thread pool, so tool handlers run on those threads,
  possibly several at once. Hop to the message thread for anything UI related.

```cpp
yup::MCPHttpServerTransport::Options options;
options.port = 7380;
options.numPortsToTry = 16;

auto transport = std::make_unique<yup::MCPHttpServerTransport> (options);
auto* http = transport.get();

if (server.start (std::move (transport)).wasOk())
    YUP_DBG ("Serving at " << http->getUrl()); // http://127.0.0.1:7380/mcp
```

`MCPServer::startHttp (port)` is a shortcut for the same with default options.
Clients configure it as an HTTP server:

```json
{ "type": "http", "url": "http://127.0.0.1:7380/mcp" }
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
    YUP_DBG ("MCP handshake failed: " << result.getErrorMessage());
    return;
}

// List and call tools; a result flagged isError fails with the tool's message
auto tools = client.listTools();
for (auto& tool : tools)
    YUP_DBG (tool.name << ": " << tool.description);

auto answer = client.callTool ("get_weather", yup::DynamicObject::Ptr (
    new yup::DynamicObject ({ { "city", "Rome" } })));
if (answer.wasOk())
    YUP_DBG (JSON::toString (*answer));

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
opts.instructions  = "Call get_weather with a city name."; // Optional hints for the model
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

// Return an MCPToolResult for errors, images, audio or structured content
server.registerTool (tool, [](const yup::var& args)
{
    if (args["city"].toString().isEmpty())
        return yup::MCPToolResult::fromError ("Pass a city");

    return yup::MCPToolResult::fromText ("Sunny");
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

A handler returning `var` is wrapped as a tool result: a string becomes a text
block, anything else becomes its JSON text plus `structuredContent`.

## Lifecycle

The MCP lifecycle follows three phases:

1. **Initialization** — client sends `initialize` request with capabilities;
   server responds with its own capabilities. Client sends `notifications/initialized`.
2. **Operation** — normal message exchange: `tools/list`, `tools/call`,
   `resources/list`, `resources/read`, `ping`, etc.
3. **Shutdown** — transport closes or `stop()` is called.

```{note}
`MCPClient::initialize()` performs the full handshake automatically. After a
successful `initialize()`, the client is ready for operation.
```
