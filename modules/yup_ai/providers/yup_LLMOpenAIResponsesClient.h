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
/** OpenAI Responses API client — for GPT-5 and newer reasoning models.

    Connects to the /v1/responses endpoint.  This API uses a different JSON
    structure from Chat Completions: system prompt → "instructions", user
    messages → "input", and structured output via text.format rather than
    response_format.

    Reasoning effort is controlled via Options::reasoningEffort, which maps to
    the reasoning.effort field ("none", "low", "medium", "high").

    CFG grammar-constrained output is supported via Request::grammar (Lark
    syntax) using a custom tool.  Request::grammarToolName and
    Request::grammarToolDescription configure the tool's identity.

    Streaming SSE format:
    @code
      data: {"type":"response.output_text.delta","delta":"token","item_id":"…"}
      data: {"type":"response.completed",…}
    @endcode

    @tags{AI}
*/
class YUP_API LLMOpenAIResponsesClient : public LLMHttpClient
{
public:
    explicit LLMOpenAIResponsesClient (Options options);
    ~LLMOpenAIResponsesClient() override;

protected:
    String getEndpointUrl() const override;
    String buildHeaders() const override;
    String buildPayload (const Request& request) const override;
    LLMResponse parseResponse (const var& json) const override;
    LLMResponse parseChunk (const var& json) const override;
};

} // namespace yup
