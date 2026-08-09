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

LLMAnthropicClient::LLMAnthropicClient (Options options)
    : LLMHttpClient (std::move (options))
{
}

LLMAnthropicClient::~LLMAnthropicClient() = default;

//==============================================================================
String LLMAnthropicClient::getEndpointUrl() const
{
    return makeProviderUrl (options.baseUrl, "/messages");
}

String LLMAnthropicClient::buildHeaders() const
{
    String headers = "Content-Type: application/json\r\nAccept: application/json\r\n";

    if (options.apiKey.isNotEmpty())
        headers += "x-api-key: " + options.apiKey + "\r\n";

    headers += "anthropic-version: 2023-06-01\r\n";

    if (options.userAgent.isNotEmpty())
        headers += "User-Agent: " + options.userAgent + "\r\n";

    return headers;
}

String LLMAnthropicClient::buildPayload (const Request& request) const
{
    // Build user messages array (Anthropic excludes system prompt from messages[]).
    var messagesArray;

    for (const auto& message : request.messages)
    {
        switch (message.role)
        {
            case LLMMessage::Role::user:
            case LLMMessage::Role::assistant:
                messagesArray.append (message.toVar());
                break;

            default:
                break; // system messages go in the top-level "system" field
        }
    }

    auto payload = var (std::make_unique<DynamicObject>());
    auto* payloadObj = payload.getDynamicObject();

    payloadObj->setProperty ("model", options.model);

    // Anthropic always requires max_tokens; default to 4096 if unset.
    const int effectiveMaxTokens = request.maxTokens.value_or (options.maxTokens > 0 ? options.maxTokens : 4096);
    payloadObj->setProperty ("max_tokens", effectiveMaxTokens);

    payloadObj->setProperty ("temperature", static_cast<double> (request.temperature.value_or (0.1f)));
    payloadObj->setProperty ("messages", messagesArray);

    // System prompt with ephemeral cache control (cached for the session lifetime).
    const auto& systemText = request.systemPrompt.has_value() ? *request.systemPrompt : String();
    if (systemText.isNotEmpty())
    {
        auto cacheControl = var (std::make_unique<DynamicObject>());
        cacheControl.getDynamicObject()->setProperty ("type", String ("ephemeral"));

        auto sysBlock = var (std::make_unique<DynamicObject>());
        sysBlock.getDynamicObject()->setProperty ("type", String ("text"));
        sysBlock.getDynamicObject()->setProperty ("text", systemText);
        sysBlock.getDynamicObject()->setProperty ("cache_control", cacheControl);

        var systemArray;
        systemArray.append (sysBlock);
        payloadObj->setProperty ("system", systemArray);
    }

    // Application identification for usage tracking.
    if (options.userAgent.isNotEmpty())
    {
        auto metadata = var (std::make_unique<DynamicObject>());
        metadata.getDynamicObject()->setProperty ("user_id", options.userAgent);
        payloadObj->setProperty ("metadata", metadata);
    }

    // NOTE: Anthropic does not support an `effort` / `reasoning_effort` field in the
    // Messages API — that is an OpenAI-ism.  Extended thinking uses a separate
    // `thinking` block on models that support it, which is not yet implemented here.

    return JSON::toString (payload, true);
}

LLMResponse LLMAnthropicClient::parseResponse (const var& json) const
{
    if (json.isVoid())
        return LLMResponse::fromError ("Unable to parse Anthropic response JSON");

    // Anthropic wraps errors in an "error" object with a "message" field.
    if (json["error"].isObject())
    {
        auto message = json["error"]["message"].toString();
        return LLMResponse::fromError (message.isNotEmpty() ? message : "Unknown Anthropic API error");
    }

    LLMResponse response;
    response.model = json["model"].toString();

    if (auto* contentArray = json["content"].getArray())
    {
        if (! contentArray->isEmpty())
        {
            const auto text = (*contentArray)[0]["text"].toString().trim();

            LLMResponse::Choice choice;
            choice.index = 0;
            choice.message = LLMMessage::assistant (text);

            const auto stopReason = json["stop_reason"].toString();
            if (stopReason.isNotEmpty())
                choice.finishReason = stopReason;

            response.choices.push_back (std::move (choice));
        }
    }

    // Usage: Anthropic uses input_tokens / output_tokens.
    if (json["usage"].isObject())
    {
        LLMResponse::Usage usage;
        usage.promptTokens = static_cast<int> (json["usage"]["input_tokens"]);
        usage.completionTokens = static_cast<int> (json["usage"]["output_tokens"]);
        usage.totalTokens = usage.promptTokens + usage.completionTokens;
        response.usage = usage;
    }

    return response;
}

LLMResponse LLMAnthropicClient::parseChunk (const var& json) const
{
    // Anthropic SSE format:
    //   data: {"type":"content_block_delta","index":0,"delta":{"type":"text_delta","text":"token"}}
    //   data: {"type":"message_delta","usage":{"output_tokens":42}}
    //   data: {"type":"message_stop"}

    const auto type = json["type"].toString();
    if (type != "content_block_delta")
        return LLMResponse {}; // non-content events produce an empty (no-op) chunk

    const auto text = json["delta"]["text"].toString();

    LLMResponse chunk;
    LLMResponse::Choice choice;
    choice.index = 0;
    choice.message.role = LLMMessage::Role::assistant;
    choice.message.content = text;
    chunk.choices.push_back (std::move (choice));

    return chunk;
}

} // namespace yup
