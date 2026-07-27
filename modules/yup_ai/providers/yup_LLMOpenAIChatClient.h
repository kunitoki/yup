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
/** OpenAI Chat Completions API client.

    Compatible with any OpenAI-compatible endpoint including:
      - OpenAI (https://api.openai.com/v1)
      - DeepSeek (https://api.deepseek.com/v1)
      - OpenRouter (https://openrouter.ai/api/v1)
      - Ollama (http://localhost:11434/v1) — the default base URL
      - llama-server and any other OpenAI-compatible local server

    Supports the full yup_ai feature set: multi-turn messages, LLMTool
    definitions and the tool-calling loop, streaming, structured output via
    LLMSchema (response_format.json_schema), GBNF grammar for llama-server,
    prompt caching, and per-model reasoning effort.

    For OpenRouter requests, X-Title and HTTP-Referer headers are injected
    automatically when Options::userAgent and Options::appUrl are set.

    @tags{AI}
*/
class YUP_API LLMOpenAIChatClient : public LLMHttpClient
{
public:
    explicit LLMOpenAIChatClient (Options options);
    ~LLMOpenAIChatClient() override;

protected:
    String getEndpointUrl() const override;
    String buildHeaders() const override;
    String buildPayload (const Request& request) const override;
    String buildStreamingPayload (const Request& request) const override;
    LLMResponse parseResponse (const var& json) const override;
    LLMResponse parseChunk (const var& json) const override;
};

} // namespace yup
