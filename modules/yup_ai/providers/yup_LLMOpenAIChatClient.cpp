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

LLMOpenAIChatClient::LLMOpenAIChatClient (Options options)
    : LLMHttpClient (std::move (options))
{
}

LLMOpenAIChatClient::~LLMOpenAIChatClient() = default;

//==============================================================================
String LLMOpenAIChatClient::getEndpointUrl() const
{
    return makeProviderUrl (options.baseUrl, "/chat/completions");
}

String LLMOpenAIChatClient::buildHeaders() const
{
    String headers = "Content-Type: application/json\r\nAccept: application/json\r\n";

    if (options.apiKey.isNotEmpty())
        headers += "Authorization: Bearer " + options.apiKey + "\r\n";

    if (options.userAgent.isNotEmpty())
        headers += "User-Agent: " + options.userAgent + "\r\n";

    // OpenRouter — application identification.
    if (options.baseUrl.contains ("openrouter.ai"))
    {
        if (options.userAgent.isNotEmpty())
            headers += "X-Title: " + options.userAgent + "\r\n";
        if (options.appUrl.isNotEmpty())
            headers += "HTTP-Referer: " + options.appUrl + "\r\n";
    }

    return headers;
}

String LLMOpenAIChatClient::buildPayload (const Request& request) const
{
    return buildChatCompletionBody (request, false);
}

String LLMOpenAIChatClient::buildStreamingPayload (const Request& request) const
{
    return buildChatCompletionBody (request, true);
}

LLMResponse LLMOpenAIChatClient::parseResponse (const var& json) const
{
    return LLMResponse::fromOpenAiJson (json);
}

LLMResponse LLMOpenAIChatClient::parseChunk (const var& json) const
{
    return LLMResponse::fromStreamChunk (json);
}

} // namespace yup
