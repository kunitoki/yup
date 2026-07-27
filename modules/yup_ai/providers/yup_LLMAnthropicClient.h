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
/** Anthropic Messages API client — for Claude models.

    Connects to the Anthropic /v1/messages endpoint.  Handles ephemeral prompt
    caching on the system prompt and translates between the Anthropic JSON
    format and the unified LLMResponse type.

    The default base URL is https://api.anthropic.com/v1.  The API key is sent
    via the x-api-key header (not Bearer).

    Streaming uses Anthropic's SSE format:
    @code
      data: {"type":"content_block_delta","delta":{"type":"text_delta","text":"…"}}
    @endcode

    @tags{AI}
*/
class YUP_API LLMAnthropicClient : public LLMHttpClient
{
public:
    explicit LLMAnthropicClient (Options options);
    ~LLMAnthropicClient() override;

protected:
    String getEndpointUrl() const override;
    String buildHeaders() const override;
    String buildPayload (const Request& request) const override;
    LLMResponse parseResponse (const var& json) const override;
    LLMResponse parseChunk (const var& json) const override;
};

} // namespace yup
