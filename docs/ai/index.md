# AI

LLM chat-completion clients, text embeddings, and MCP (Model Context Protocol)
server and client — all built on `yup_core` and `yup_events`.

**Module:** `yup_ai`.

## In this area

- [LLM clients](llm.md) — chat completions, streaming, tools, structured output,
  and provider setup (OpenAI, Anthropic, Gemini, Ollama, DeepSeek, llama-server).
- [MCP](mcp.md) — Model Context Protocol server and client for tool/resource
  bridging over JSON-RPC transports.
- [Embeddings](embedding.md) — text embeddings and cosine similarity for
  semantic search and retrieval.

## Key building blocks

The `yup_ai` module provides:

- **`LLMClient`** — abstract base for every chat-completion provider with
  `complete()`, `completeStreaming()`, `chat()`, and `runToolLoop()`.
- **`LLMClientFactory`** — a single `create()` call that picks the correct
  provider from `LLMClient::Options::provider`, plus convenience factories
  (`openAIChat()`, `anthropic()`, `gemini()`, etc.).
- **`LLMMessage` / `LLMResponse`** — message and response types compatible
  with OpenAI's ChatML format.
- **`LLMTool` / `LLMToolRegistry`** — define callable functions with JSON
  Schema parameters and register/dispatch them thread-safely.
- **`LLMSchema`** — fluent builder for JSON Schema objects used in
  structured-output requests.
- **`EmbeddingModel`** — embed text via any OpenAI-compatible embeddings
  endpoint.
- **`MCPClient` / `MCPServer`** — JSON-RPC 2.0 bridge for Model Context
  Protocol: list and call tools, register resources.
- **`MCPTransport`** — abstract transport; implementations for stdio, HTTP/SSE,
  and in-process queues.

## Quick start

```cpp
#include <yup_ai/yup_ai.h>

// Create an OpenAI Chat client
auto client = yup::LLMClientFactory::openAIChat (
    "gpt-5",
    "https://api.openai.com/v1",
    "sk-..."
);

// One-shot chat
auto response = client->chat ("What is the capital of France?");
if (! response.failed())
    DBG (response.choices.front().message.content); // "Paris"

// Structured output with JSON Schema
yup::LLMClient::Request request;
request.messages.push_back (yup::LLMMessage::user ("Extract the title and year."));
request.schema = yup::LLMSchema::object ({
    { "title", yup::LLMSchema::string() },
    { "year",  yup::LLMSchema::integer() },
});
auto structured = client->complete (request);
```

## Related areas

- [Scripting](../scripting/index.md) — Python bindings are available for `yup_ai`
  when both modules are linked.

```{toctree}
:hidden:
:maxdepth: 2

llm
mcp
embedding
```
