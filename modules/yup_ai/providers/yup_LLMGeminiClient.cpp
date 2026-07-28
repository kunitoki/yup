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
namespace
{

//==============================================================================
/** Builds a Gemini functionDeclaration object from an LLMTool.

    The Gemini REST API uses camelCase keys (functionDeclarations, toolConfig …).
    The parameters block is reused from LLMTool::toJsonSchema() — the JSON Schema
    shape is identical to what OpenAI uses.
*/
var buildGeminiFunctionDeclaration (const LLMTool& tool)
{
    // toJsonSchema() returns { "type":"function", "function":{ "name","description","parameters" } }
    const auto openAiSchema = tool.toJsonSchema();

    auto funcDecl = var (std::make_unique<DynamicObject>());
    auto* obj = funcDecl.getDynamicObject();
    obj->setProperty ("name", tool.name);

    if (tool.description.isNotEmpty())
        obj->setProperty ("description", tool.description);

    // The parameters schema is the same format for both providers.
    obj->setProperty ("parameters", openAiSchema["function"]["parameters"]);

    return funcDecl;
}

//==============================================================================
/** Normalises a tool-result content string into a JSON object for
    Gemini's functionResponse.response field.

    JSON objects and arrays are passed through directly; scalars and raw strings
    are wrapped in { "result": value }.
*/
var parseToolResultForGemini (const String& content)
{
    const auto parsed = JSON::parse (content);

    if (parsed.isObject() || parsed.isArray())
        return parsed;

    auto wrapper = var (std::make_unique<DynamicObject>());
    wrapper.getDynamicObject()->setProperty ("result", parsed.isVoid() ? var (content) : parsed);
    return wrapper;
}

//==============================================================================
/** Shared response-parsing logic used by both parseResponse and parseChunk.

    Iterates over candidates/parts.  Parts with a "functionCall" key are
    converted to LLMToolCall objects (including the call id for parallel-call
    correlation).  "text" parts are concatenated into the message content.
    If any function calls are present they take priority and the choice carries
    them in toolCalls (content is left empty).
*/
LLMResponse geminiCandidatesToResponse (const var& json)
{
    if (json.isVoid())
        return LLMResponse::fromError ("Unable to parse Gemini response JSON");

    if (json["error"].isObject())
    {
        const auto message = json["error"]["message"].toString();
        return LLMResponse::fromError (message.isNotEmpty() ? message : "Unknown Gemini API error");
    }

    LLMResponse response;

    if (auto* candidates = json["candidates"].getArray())
    {
        int choiceIndex = 0;

        for (const auto& candidate : *candidates)
        {
            auto* parts = candidate["content"]["parts"].getArray();
            if (parts == nullptr || parts->isEmpty())
                continue;

            LLMResponse::Choice choice;
            choice.index = choiceIndex++;

            String textContent;
            std::vector<LLMToolCall> toolCalls;

            for (const auto& part : *parts)
            {
                if (part.hasProperty ("functionCall"))
                {
                    const auto& fc = part["functionCall"];

                    LLMToolCall toolCall;
                    toolCall.index = static_cast<int> (toolCalls.size());
                    toolCall.name = fc["name"].toString();

                    // Gemini may include a call id for parallel function calling.
                    // Store it so runToolLoop can round-trip it via toolCallId; fall
                    // back to the function name when absent (sequential calling).
                    const auto callId = fc["id"].toString();
                    toolCall.id = callId.isNotEmpty() ? callId : toolCall.name;

                    toolCall.arguments = fc["args"].isVoid() ? var() : fc["args"];
                    toolCalls.push_back (std::move (toolCall));
                }
                else if (part.hasProperty ("text"))
                {
                    textContent += part["text"].toString();
                }
            }

            if (! toolCalls.empty())
            {
                choice.message = LLMMessage::assistant ("");
                choice.message.toolCalls = std::move (toolCalls);
            }
            else
            {
                choice.message = LLMMessage::assistant (textContent.trim());
            }

            const auto finishReason = candidate["finishReason"].toString();
            if (finishReason.isNotEmpty())
                choice.finishReason = finishReason;

            response.choices.push_back (std::move (choice));
        }
    }

    return response;
}

} // namespace

//==============================================================================
LLMGeminiClient::LLMGeminiClient (Options options)
    : LLMHttpClient (std::move (options))
{
}

LLMGeminiClient::~LLMGeminiClient() = default;

//==============================================================================
String LLMGeminiClient::getEndpointUrl() const
{
    return options.baseUrl + "/v1beta/models/" + options.model + ":generateContent";
}

String LLMGeminiClient::getStreamingEndpointUrl() const
{
    return options.baseUrl + "/v1beta/models/" + options.model + ":streamGenerateContent?alt=sse";
}

String LLMGeminiClient::buildHeaders() const
{
    String headers = "Content-Type: application/json\r\nAccept: application/json\r\n";
    headers += "x-goog-api-key: " + options.apiKey + "\r\n";

    if (options.userAgent.isNotEmpty())
        headers += "User-Agent: " + options.userAgent + "\r\n";

    return headers;
}

String LLMGeminiClient::buildPayload (const Request& request) const
{
    // System instruction.
    auto sysPart = var (std::make_unique<DynamicObject>());
    sysPart.getDynamicObject()->setProperty ("text", request.systemPrompt.value_or (String()));

    var sysPartsArray;
    sysPartsArray.append (sysPart);

    auto sysInstruction = var (std::make_unique<DynamicObject>());
    sysInstruction.getDynamicObject()->setProperty ("parts", sysPartsArray);

    // Contents array.
    // - system   → skipped (goes in system_instruction above).
    // - tool     → "user" turn with a functionResponse part.
    //              message.toolCallId holds the Gemini call id (or function name
    //              as fallback).  message.name holds the function name, set by
    //              the updated runToolLoop.
    // - assistant with toolCalls → "model" turn with functionCall parts.
    // - user / plain assistant  → "user" / "model" turn with a text part.
    var contentsArray;

    for (const auto& message : request.messages)
    {
        if (message.role == LLMMessage::Role::system)
            continue;

        // Tool-result message → user turn with functionResponse.
        if (message.role == LLMMessage::Role::tool)
        {
            // name is the function name (set by updated runToolLoop);
            // fall back to toolCallId when absent for backward compatibility.
            const auto callId = message.toolCallId.value_or (String());
            const auto functionName = message.name.isNotEmpty() ? message.name : callId;

            if (functionName.isEmpty())
                continue;

            auto functionResponse = var (std::make_unique<DynamicObject>());
            auto* frObj = functionResponse.getDynamicObject();

            if (callId.isNotEmpty())
                frObj->setProperty ("id", callId);

            frObj->setProperty ("name", functionName);
            frObj->setProperty ("response", parseToolResultForGemini (message.content));

            auto part = var (std::make_unique<DynamicObject>());
            part.getDynamicObject()->setProperty ("functionResponse", functionResponse);

            var partsArray;
            partsArray.append (part);

            auto contentObj = var (std::make_unique<DynamicObject>());
            contentObj.getDynamicObject()->setProperty ("role", String ("user"));
            contentObj.getDynamicObject()->setProperty ("parts", partsArray);
            contentsArray.append (contentObj);
            continue;
        }

        // Assistant message with pending tool calls → model turn with functionCall parts.
        if (message.role == LLMMessage::Role::assistant
            && message.toolCalls.has_value()
            && ! message.toolCalls->empty())
        {
            var partsArray;

            for (const auto& toolCall : *message.toolCalls)
            {
                auto functionCall = var (std::make_unique<DynamicObject>());
                auto* fcObj = functionCall.getDynamicObject();

                if (toolCall.id.isNotEmpty() && toolCall.id != toolCall.name)
                    fcObj->setProperty ("id", toolCall.id);

                fcObj->setProperty ("name", toolCall.name);
                fcObj->setProperty ("args",
                                    toolCall.arguments.isVoid()
                                        ? var (std::make_unique<DynamicObject>())
                                        : toolCall.arguments);

                auto part = var (std::make_unique<DynamicObject>());
                part.getDynamicObject()->setProperty ("functionCall", functionCall);
                partsArray.append (part);
            }

            auto contentObj = var (std::make_unique<DynamicObject>());
            contentObj.getDynamicObject()->setProperty ("role", String ("model"));
            contentObj.getDynamicObject()->setProperty ("parts", partsArray);
            contentsArray.append (contentObj);
            continue;
        }

        // Ordinary user / assistant text message.
        auto textPart = var (std::make_unique<DynamicObject>());
        textPart.getDynamicObject()->setProperty ("text", message.content);

        var partsArray;
        partsArray.append (textPart);

        const auto geminiRole = (message.role == LLMMessage::Role::assistant)
                                  ? String ("model")
                                  : String ("user");

        auto contentObj = var (std::make_unique<DynamicObject>());
        contentObj.getDynamicObject()->setProperty ("role", geminiRole);
        contentObj.getDynamicObject()->setProperty ("parts", partsArray);
        contentsArray.append (contentObj);
    }

    // Generation config.
    auto genConfig = var (std::make_unique<DynamicObject>());
    genConfig.getDynamicObject()->setProperty ("temperature",
                                               static_cast<double> (request.temperature.value_or (0.1f)));

    const int effectiveMaxTokens = request.maxTokens.value_or (options.maxTokens);
    if (effectiveMaxTokens > 0)
        genConfig.getDynamicObject()->setProperty ("maxOutputTokens", effectiveMaxTokens);

    // Thinking config for Gemini 2.5 models.
    if (options.reasoningEffort.isNotEmpty())
    {
        int budget = 4096;
        if (options.reasoningEffort == "low")
            budget = 1024;
        else if (options.reasoningEffort == "high")
            budget = 16384;

        auto thinkingConfig = var (std::make_unique<DynamicObject>());
        thinkingConfig.getDynamicObject()->setProperty ("thinkingBudget", budget);
        genConfig.getDynamicObject()->setProperty ("thinkingConfig", thinkingConfig);
    }

    // Structured output via JSON Schema.
    if (! request.schema.isVoid())
    {
        genConfig.getDynamicObject()->setProperty ("responseMimeType", String ("application/json"));
        genConfig.getDynamicObject()->setProperty ("responseSchema", request.schema);
    }

    auto payload = var (std::make_unique<DynamicObject>());
    payload.getDynamicObject()->setProperty ("system_instruction", sysInstruction);
    payload.getDynamicObject()->setProperty ("contents", contentsArray);
    payload.getDynamicObject()->setProperty ("generationConfig", genConfig);

    // Function declarations and tool config.
    // Gemini REST API uses camelCase for these composite keys.
    if (! request.tools.empty())
    {
        var functionDeclarations;
        for (const auto& tool : request.tools)
            functionDeclarations.append (buildGeminiFunctionDeclaration (tool));

        auto toolsGroup = var (std::make_unique<DynamicObject>());
        toolsGroup.getDynamicObject()->setProperty ("functionDeclarations", functionDeclarations);

        var toolsArray;
        toolsArray.append (toolsGroup);
        payload.getDynamicObject()->setProperty ("tools", toolsArray);

        // Translate OpenAI-style toolChoice to Gemini functionCallingConfig mode.
        auto funcCallingConfig = var (std::make_unique<DynamicObject>());

        if (request.toolChoice.has_value())
        {
            const auto& choice = *request.toolChoice;

            if (choice == "none")
            {
                funcCallingConfig.getDynamicObject()->setProperty ("mode", String ("NONE"));
            }
            else if (choice == "required")
            {
                funcCallingConfig.getDynamicObject()->setProperty ("mode", String ("ANY"));
            }
            else if (choice != "auto")
            {
                // Specific function name — force the model to call exactly that tool.
                funcCallingConfig.getDynamicObject()->setProperty ("mode", String ("ANY"));
                var allowedNames;
                allowedNames.append (choice);
                funcCallingConfig.getDynamicObject()->setProperty ("allowedFunctionNames", allowedNames);
            }
            else
            {
                funcCallingConfig.getDynamicObject()->setProperty ("mode", String ("AUTO"));
            }
        }
        else
        {
            funcCallingConfig.getDynamicObject()->setProperty ("mode", String ("AUTO"));
        }

        auto toolConfig = var (std::make_unique<DynamicObject>());
        toolConfig.getDynamicObject()->setProperty ("functionCallingConfig", funcCallingConfig);
        payload.getDynamicObject()->setProperty ("toolConfig", toolConfig);
    }

    return JSON::toString (payload, true);
}

String LLMGeminiClient::buildStreamingPayload (const Request& request) const
{
    // Gemini streaming uses the same body as non-streaming; the streaming
    // behaviour is selected by the :streamGenerateContent?alt=sse endpoint URL.
    return buildPayload (request);
}

LLMResponse LLMGeminiClient::parseResponse (const var& json) const
{
    return geminiCandidatesToResponse (json);
}

LLMResponse LLMGeminiClient::parseChunk (const var& json) const
{
    // Gemini SSE chunks use the same candidates/parts structure as full responses,
    // including functionCall parts for tool-calling chunks.
    return geminiCandidatesToResponse (json);
}

} // namespace yup
