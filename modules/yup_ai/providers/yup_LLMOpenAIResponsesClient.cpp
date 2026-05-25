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

LLMOpenAIResponsesClient::LLMOpenAIResponsesClient (Options options)
    : LLMHttpClient (std::move (options))
{
}

LLMOpenAIResponsesClient::~LLMOpenAIResponsesClient() = default;

//==============================================================================
String LLMOpenAIResponsesClient::getEndpointUrl() const
{
    return makeProviderUrl (options.baseUrl, "/responses");
}

String LLMOpenAIResponsesClient::buildHeaders() const
{
    String headers = "Content-Type: application/json\r\nAccept: application/json\r\n";

    if (options.apiKey.isNotEmpty())
        headers += "Authorization: Bearer " + options.apiKey + "\r\n";

    if (options.userAgent.isNotEmpty())
        headers += "User-Agent: " + options.userAgent + "\r\n";

    return headers;
}

String LLMOpenAIResponsesClient::buildPayload (const Request& request) const
{
    auto payload = var (std::make_unique<DynamicObject>());
    auto* obj = payload.getDynamicObject();

    obj->setProperty ("model", options.model);

    // System prompt → "instructions" field in the Responses API.
    const auto systemText = request.systemPrompt.value_or (String());
    if (systemText.isNotEmpty())
        obj->setProperty ("instructions", systemText);

    // Build input from messages.  The Responses API accepts a string for single-turn
    // and an array of message objects for multi-turn.
    if (! request.messages.empty())
    {
        // Use a formatted string representation of the conversation.
        // For simple single-user-message use, just pass the content.
        if (request.messages.size() == 1 && request.messages[0].role == LLMMessage::Role::user)
        {
            obj->setProperty ("input", request.messages[0].content);
        }
        else
        {
            // Multi-turn: convert to the Responses API messages array format.
            var inputArray;

            for (const auto& message : request.messages)
            {
                auto msgObj = var (std::make_unique<DynamicObject>());
                const String role = (message.role == LLMMessage::Role::user) ? "user" : "assistant";
                msgObj.getDynamicObject()->setProperty ("role", role);
                msgObj.getDynamicObject()->setProperty ("content", message.content);
                inputArray.append (msgObj);
            }

            obj->setProperty ("input", inputArray);
        }
    }

    if (! options.noTemperature)
    {
        if (request.temperature.has_value())
            obj->setProperty ("temperature", static_cast<double> (*request.temperature));
    }

    // Reasoning effort.
    if (options.reasoningEffort.isNotEmpty())
    {
        auto reasoning = var (std::make_unique<DynamicObject>());
        reasoning.getDynamicObject()->setProperty ("effort", options.reasoningEffort);
        obj->setProperty ("reasoning", reasoning);
    }

    // Max output tokens.
    const int effectiveMaxTokens = request.maxTokens.value_or (options.maxTokens);
    if (effectiveMaxTokens > 0)
        obj->setProperty ("max_output_tokens", effectiveMaxTokens);

    // Prompt caching.
    if (options.userAgent.isNotEmpty())
    {
        obj->setProperty ("prompt_cache_key", options.userAgent);
        obj->setProperty ("prompt_cache_retention", String ("24h"));
    }

    // CFG grammar-constrained output via custom tool (Lark syntax).
    // Request grammar overrides config grammar for per-call flexibility.
    const auto& effectiveGrammar = request.grammar.isNotEmpty() ? request.grammar : options.grammar;

    if (effectiveGrammar.isNotEmpty())
    {
        const auto toolName = request.grammarToolName.isNotEmpty()
                                ? request.grammarToolName
                                : String ("grammar_tool");
        const auto toolDesc = request.grammarToolDescription.isNotEmpty()
                                ? request.grammarToolDescription
                                : systemText;

        auto format = var (std::make_unique<DynamicObject>());
        format.getDynamicObject()->setProperty ("type", String ("grammar"));
        format.getDynamicObject()->setProperty ("syntax", String ("lark"));
        format.getDynamicObject()->setProperty ("definition", effectiveGrammar);

        auto tool = var (std::make_unique<DynamicObject>());
        tool.getDynamicObject()->setProperty ("type", String ("custom"));
        tool.getDynamicObject()->setProperty ("name", toolName);
        tool.getDynamicObject()->setProperty ("description", toolDesc);
        tool.getDynamicObject()->setProperty ("format", format);

        var tools;
        tools.append (tool);
        obj->setProperty ("tools", tools);
        obj->setProperty ("parallel_tool_calls", false);
    }
    // Structured output via JSON Schema (flat format under text.format — different
    // from the Chat Completions response_format shape).
    else if (! request.schema.isVoid())
    {
        auto format = var (std::make_unique<DynamicObject>());
        format.getDynamicObject()->setProperty ("type", String ("json_schema"));
        format.getDynamicObject()->setProperty ("name", String ("response"));
        format.getDynamicObject()->setProperty ("strict", true);
        format.getDynamicObject()->setProperty ("schema", request.schema);

        auto text = var (std::make_unique<DynamicObject>());
        text.getDynamicObject()->setProperty ("format", format);
        obj->setProperty ("text", text);
    }

    return JSON::toString (payload, true);
}

LLMResponse LLMOpenAIResponsesClient::parseResponse (const var& json) const
{
    if (json.isVoid())
        return LLMResponse::fromError ("Unable to parse OpenAI Responses response JSON");

    if (json["error"].isObject())
    {
        auto message = json["error"]["message"].toString();
        return LLMResponse::fromError (message.isNotEmpty() ? message : "Unknown OpenAI Responses API error");
    }

    if (auto* output = json["output"].getArray())
    {
        for (const auto& item : *output)
        {
            const auto type = item["type"].toString();

            // Grammar tool response — text in item["input"].
            if (type == "custom_tool_call")
            {
                const auto input = item["input"].toString().trim();
                if (input.isNotEmpty())
                {
                    LLMResponse response;
                    LLMResponse::Choice choice;
                    choice.index = 0;
                    choice.message = LLMMessage::assistant (input);
                    response.choices.push_back (std::move (choice));
                    return response;
                }
            }

            // Standard text response — output[].content[].text.
            if (type == "message")
            {
                if (auto* content = item["content"].getArray())
                {
                    for (const auto& c : *content)
                    {
                        if (c["type"].toString() == "output_text")
                        {
                            LLMResponse response;
                            LLMResponse::Choice choice;
                            choice.index = 0;
                            choice.message = LLMMessage::assistant (c["text"].toString().trim());
                            response.choices.push_back (std::move (choice));
                            return response;
                        }
                    }
                }
            }
        }
    }

    return LLMResponse::fromError ("No content found in OpenAI Responses output");
}

LLMResponse LLMOpenAIResponsesClient::parseChunk (const var& json) const
{
    // Responses API SSE:
    //   data: {"type":"response.output_text.delta","delta":"token","item_id":"…"}
    //   data: {"type":"response.completed",…}
    const auto type = json["type"].toString();
    if (type != "response.output_text.delta")
        return LLMResponse {}; // non-content events

    LLMResponse chunk;
    LLMResponse::Choice choice;
    choice.index = 0;
    choice.message.role = LLMMessage::Role::assistant;
    choice.message.content = json["delta"].toString();
    chunk.choices.push_back (std::move (choice));

    return chunk;
}

} // namespace yup
