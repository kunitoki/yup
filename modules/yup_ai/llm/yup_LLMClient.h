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
/** Abstract base class for chat-completion backends.

    @tags{AI}
*/
class YUP_API LLMClient
{
public:
    /** The LLM provider type, used by LLMClientFactory to create the correct client. */
    enum class Provider
    {
        OpenAIChat,      ///< OpenAI Chat Completions — also works with DeepSeek, OpenRouter, Ollama, llama-server.
        OpenAIResponses, ///< OpenAI Responses API (GPT-5+).
        Anthropic,       ///< Anthropic Messages API — Claude models.
        Gemini           ///< Google Gemini generateContent API.
    };

    struct Request
    {
        std::vector<LLMMessage> messages;
        std::optional<String> systemPrompt;
        std::vector<LLMTool> tools;
        std::optional<String> toolChoice; ///< "auto", "none", "required", or a specific function name.

        std::optional<float> temperature;
        std::optional<float> topP;
        std::optional<int> maxTokens; ///< Per-request override; falls back to Options::maxTokens.
        std::optional<std::vector<String>> stopSequences;

        var schema;                    ///< Optional JSON Schema for structured output (built with LLMSchema).
        String grammar;                ///< Optional per-request GBNF (llama-server) or Lark (OpenAI Responses) grammar.
        String grammarToolName;        ///< Tool name for grammar-constrained output (OpenAI Responses API only).
        String grammarToolDescription; ///< Tool description for grammar output; defaults to system prompt if empty.
    };

    struct Options
    {
        Provider provider = Provider::OpenAIChat; ///< LLM backend provider — used by LLMClientFactory.

        String model;
        String baseUrl = "http://localhost:11434/v1";
        String apiKey;
        int timeoutMs = 120000;
        int maxRetries = 2;
        int maxTokens = 0; ///< Default max output tokens (0 = provider default); per-request value overrides.

        String reasoningEffort;     ///< "none", "low", "medium", "high" — for OpenAI o-series and Gemini 2.5 models.
        String grammar;             ///< Default GBNF grammar for llama-server constrained decoding (per-request overrides).
        bool noTemperature = false; ///< Set true for models that reject the temperature parameter (e.g. GPT-5 series).
        String userAgent;           ///< Application identifier used for User-Agent header and prompt cache key.
        String appUrl;              ///< Application URL sent as HTTP-Referer on OpenRouter requests.
    };

    explicit LLMClient (Options options);
    virtual ~LLMClient();

    /** Performs a non-streaming completion request. */
    virtual LLMResponse complete (const Request& request) = 0;

    using ChunkCallback = std::function<void (const LLMResponse& chunk)>;

    /** Performs a streaming completion request and invokes onChunk for deltas. */
    virtual bool completeStreaming (const Request& request, ChunkCallback onChunk) = 0;

    /** Convenience helper for a single user message. */
    LLMResponse chat (const String& userMessage);

    /** Convenience helper for a single user message with all registered tools. */
    LLMResponse chatWithTools (const String& userMessage, const LLMToolRegistry& tools);

    /** Repeatedly completes and dispatches tool calls until the model stops requesting tools. */
    LLMResponse runToolLoop (const Request& request, LLMToolRegistry& tools);

    /** Returns immutable client options. */
    const Options& getOptions() const noexcept { return options; }

protected:
    Options options;

    String buildChatCompletionBody (const Request& request, bool stream) const;
    var messagesToVar (const std::vector<LLMMessage>& messages) const;
    var toolsToVar (const std::vector<LLMTool>& tools) const;
};

} // namespace yup
