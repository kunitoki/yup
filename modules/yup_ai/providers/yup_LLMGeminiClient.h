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
/** Google Gemini generateContent API client.

    Connects to the Gemini /v1beta/models/{model}:generateContent endpoint.
    The API key is sent via the x-goog-api-key header.  The default base URL
    is https://generativelanguage.googleapis.com.

    Structured output is requested via generationConfig.responseMimeType and
    generationConfig.responseSchema.

    Thinking budget (reasoning) is controlled via Options::reasoningEffort:
      - "low"  → 1024 tokens
      - "high" → 16384 tokens
      - any other non-empty value → 4096 tokens (default)

    Streaming uses :streamGenerateContent?alt=sse with the same candidates/parts
    JSON structure as the non-streaming response.

    @tags{AI}
*/
class YUP_API LLMGeminiClient : public LLMHttpClient
{
public:
    explicit LLMGeminiClient (Options options);
    ~LLMGeminiClient() override;

protected:
    String getEndpointUrl() const override;
    String getStreamingEndpointUrl() const override;
    String buildHeaders() const override;
    String buildPayload (const Request& request) const override;
    String buildStreamingPayload (const Request& request) const override;
    LLMResponse parseResponse (const var& json) const override;
    LLMResponse parseChunk (const var& json) const override;
};

} // namespace yup
