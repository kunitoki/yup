# LLM clients

The `yup_ai` module provides a uniform chat-completion interface across OpenAI,
Anthropic, Google Gemini, Ollama, DeepSeek, OpenRouter, and llama-server.

All providers are created through `LLMClientFactory` and share the same
`LLMClient` API — change the provider by changing one option.

## Creating a client

Use `LLMClientFactory::create()` with an `LLMClient::Options` struct, or one of
the convenience factories:

```cpp
// OpenAI Chat Completions (also Ollama, DeepSeek, OpenRouter, llama-server)
auto client = yup::LLMClientFactory::openAIChat (
    "gpt-4o-mini",
    "https://api.openai.com/v1",
    "sk-..."
);

// Anthropic Messages API
auto client = yup::LLMClientFactory::anthropic (
    "claude-opus-4-5",
    "sk-ant-...",
    "https://api.anthropic.com/v1"
);

// Google Gemini
auto client = yup::LLMClientFactory::gemini (
    "gemini-2.5-flash",
    "AIza...",
    "https://generativelanguage.googleapis.com"
);

// OpenAI Responses API (GPT-5+, reasoning models)
auto client = yup::LLMClientFactory::openAIResponses (
    "gpt-5",
    "sk-..."
);

// Full control with LLMClientFactory::create
yup::LLMClient::Options opts;
opts.provider    = yup::LLMClient::Provider::OpenAIChat;
opts.model       = "llama3.2";
opts.baseUrl     = "http://localhost:11434/v1";  // Ollama
opts.timeoutMs   = 60000;
auto client = yup::LLMClientFactory::create (opts);
```

```{note}
When targeting a local server (Ollama, llama-server), set `baseUrl` to the
server's `/v1` endpoint and omit `apiKey`.
```

## Provider options

| Option | Description |
|--------|-------------|
| `provider` | Backend selector (`OpenAIChat`, `OpenAIResponses`, `Anthropic`, `Gemini`) |
| `model` | Model name string |
| `baseUrl` | API base URL |
| `apiKey` | API key or bearer token |
| `timeoutMs` | HTTP timeout in milliseconds (default 120000) |
| `maxRetries` | Number of retries on transient failure (default 2) |
| `maxTokens` | Default max output tokens (0 = provider default) |
| `reasoningEffort` | `"none"`, `"low"`, `"medium"`, `"high"` — for o-series and Gemini 2.5 |
| `grammar` | Default GBNF grammar for llama-server constrained decoding |
| `noTemperature` | Set `true` for models that reject temperature (GPT-5 series) |
| `userAgent` | Application identifier for User-Agent header and prompt cache key |

## Chat completions

### Non-streaming

```cpp
yup::LLMClient::Request request;
request.messages.push_back (yup::LLMMessage::system ("You are a helpful assistant."));
request.messages.push_back (yup::LLMMessage::user ("Tell me a joke."));
request.temperature = 0.7f;
request.maxTokens   = 256;

auto response = client->complete (request);

if (response.failed())
    DBG ("Error: " << *response.errorMessage);
else
    DBG (response.choices.front().message.content);
```

### Streaming

```cpp
yup::LLMClient::Request request;
request.messages.push_back (yup::LLMMessage::user ("Write a haiku about C++."));

client->completeStreaming (request, [](const yup::LLMResponse& chunk)
{
    if (! chunk.choices.empty())
        std::cout << chunk.choices.front().message.content << std::flush;
});
```

### Convenience helpers

```cpp
// Single user message
auto response = client->chat ("Hello!");

// Single user message with all tools from a registry
auto response = client->chatWithTools ("What time is it?", toolRegistry);
```

## Messages

`LLMMessage` represents one turn in the conversation with four roles:

```cpp
auto systemMsg    = yup::LLMMessage::system ("You are a calculator.");
auto userMsg      = yup::LLMMessage::user ("What is 2 + 3?");
auto assistantMsg = yup::LLMMessage::assistant ("The answer is 5.");
auto toolMsg      = yup::LLMMessage::toolResult ("call_123", "5");
```

Messages serialise to OpenAI ChatML format with `toVar()` and parse from it with
`fromVar()`.

## Tools (function calling)

Define a callable function with `LLMTool`:

```cpp
yup::LLMTool tool;
tool.name        = "get_weather";
tool.description = "Get current weather for a city";
tool.parameters  = {
    { "city",     "string", "City name",      true },
    { "country",  "string", "Country code",   false }
};
tool.setHandler ([](const yup::var& args) -> yup::var
{
    auto city = args["city"].toString();
    return yup::DynamicObject::Ptr (new yup::DynamicObject ({
        { "temperature", 22 },
        { "condition",   "sunny" }
    }));
});
```

Register tools in a thread-safe `LLMToolRegistry`:

```cpp
yup::LLMToolRegistry registry;
registry.registerTool (std::move (tool));

yup::LLMClient::Request request;
request.messages.push_back (yup::LLMMessage::user ("What's the weather in Rome?"));
request.tools = registry.getAllTools();

auto response = client->complete (request);
if (response.hasToolCalls())
{
    for (auto& tc : response.getToolCalls())
    {
        auto result = registry.dispatchToolCall (tc.name, tc.arguments);
        request.messages.push_back (yup::LLMMessage::toolResult (tc.id, JSON::toString (result)));
    }
    // Continue conversation with tool results...
}
```

### Automatic tool loop

`runToolLoop()` automates the tool round-trip:

```cpp
yup::LLMClient::Request request;
request.messages = { yup::LLMMessage::user ("What's the weather in Rome and Paris?") };

auto finalResponse = client->runToolLoop (request, registry);
// finalResponse contains the model's final answer after all tool calls
```

## Structured output

Request JSON output with `LLMSchema`:

```cpp
yup::LLMClient::Request request;
request.messages.push_back (yup::LLMMessage::user ("Extract key facts from this article..."));
request.schema = yup::LLMSchema::object ({
    { "title",        yup::LLMSchema::string() },
    { "summary",      yup::LLMSchema::string() },
    { "year",         yup::LLMSchema::integer() },
    { "categories",   yup::LLMSchema::array (yup::LLMSchema::string()) },
    { "sentiment",    yup::LLMSchema::oneOf ({ "positive", "negative", "neutral" }) },
});

auto response = client->complete (request);
// response is guaranteed to match the schema (on supporting providers)
```

```{note}
Structured output is supported on all four providers: OpenAI Chat (response_format),
OpenAI Responses (text.format), Anthropic (tool_use with JSON Schema), and
Gemini (response_schema).
```

## Reasoning effort

For reasoning models (OpenAI o-series, Gemini 2.5), set `reasoningEffort`:

```cpp
yup::LLMClient::Options opts;
opts.provider       = yup::LLMClient::Provider::OpenAIResponses;
opts.model          = "gpt-5";
opts.apiKey         = "sk-...";
opts.reasoningEffort = "medium";

auto client = yup::LLMClientFactory::create (opts);
```

## Constrained decoding

For llama-server, set a GBNF grammar for token-level output constraints:

```cpp
yup::LLMClient::Options opts;
opts.provider = yup::LLMClient::Provider::OpenAIChat;
opts.model    = "llama3.2";
opts.baseUrl  = "http://localhost:8080/v1";
opts.grammar  = R"(root ::= "yes" | "no")";
```
