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
String getOpenAiErrorMessage (const var& json)
{
    if (json["error"].isString())
        return json["error"].toString();

    if (json["error"].isObject())
    {
        auto message = json["error"]["message"].toString();
        if (message.isNotEmpty())
            return message;
    }

    return {};
}
} // namespace

bool LLMResponse::hasToolCalls() const noexcept
{
    for (const auto& choice : choices)
        if (choice.message.toolCalls.has_value() && ! choice.message.toolCalls->empty())
            return true;

    return false;
}

bool LLMResponse::failed() const noexcept
{
    return errorMessage.has_value();
}

std::vector<LLMToolCall> LLMResponse::getToolCalls() const
{
    std::vector<LLMToolCall> result;

    for (const auto& choice : choices)
        if (choice.message.toolCalls.has_value())
            result.insert (result.end(), choice.message.toolCalls->begin(), choice.message.toolCalls->end());

    return result;
}

LLMResponse LLMResponse::fromError (const String& message)
{
    LLMResponse response;
    response.errorMessage = message.isEmpty() ? String ("Unknown AI response error") : message;
    return response;
}

LLMResponse LLMResponse::fromOpenAiJson (const var& json)
{
    LLMResponse response;

    if (json.isVoid())
        return fromError ("Unable to parse chat completion response JSON");

    if (auto error = getOpenAiErrorMessage (json); error.isNotEmpty())
        return fromError (error);

    response.model = json["model"].toString();

    if (auto* choicesArray = json["choices"].getArray())
    {
        for (const auto& choiceVar : *choicesArray)
        {
            Choice choice;
            choice.index = static_cast<int> (choiceVar["index"]);

            if (auto message = LLMMessage::fromVar (choiceVar["message"]))
                choice.message = *message;

            if (choiceVar.hasProperty ("finish_reason") && ! choiceVar["finish_reason"].isVoid())
                choice.finishReason = choiceVar["finish_reason"].toString();

            response.choices.push_back (std::move (choice));
        }
    }

    if (json["usage"].isObject())
    {
        Usage usage;
        usage.promptTokens = static_cast<int> (json["usage"]["prompt_tokens"]);
        usage.completionTokens = static_cast<int> (json["usage"]["completion_tokens"]);
        usage.totalTokens = static_cast<int> (json["usage"]["total_tokens"]);
        response.usage = usage;
    }

    return response;
}

LLMResponse LLMResponse::fromStreamChunk (const var& json)
{
    LLMResponse response;

    if (json.isVoid())
        return fromError ("Unable to parse chat completion stream JSON");

    if (auto error = getOpenAiErrorMessage (json); error.isNotEmpty())
        return fromError (error);

    response.model = json["model"].toString();

    if (auto* choicesArray = json["choices"].getArray())
    {
        for (const auto& choiceVar : *choicesArray)
        {
            Choice choice;
            choice.index = static_cast<int> (choiceVar["index"]);
            choice.message.role = LLMMessage::Role::assistant;

            const auto& delta = choiceVar["delta"];
            if (delta.isObject())
            {
                if (auto role = LLMMessage::roleFromString (delta["role"].toString()))
                    choice.message.role = *role;

                choice.message.content = delta["content"].toString();

                if (auto* toolCallsArray = delta["tool_calls"].getArray())
                {
                    std::vector<LLMToolCall> toolCalls;

                    for (const auto& callVar : *toolCallsArray)
                        if (auto toolCall = LLMToolCall::fromVar (callVar))
                            toolCalls.push_back (*toolCall);

                    choice.message.toolCalls = std::move (toolCalls);
                }
            }

            if (choiceVar.hasProperty ("finish_reason") && ! choiceVar["finish_reason"].isVoid())
                choice.finishReason = choiceVar["finish_reason"].toString();

            response.choices.push_back (std::move (choice));
        }
    }

    return response;
}

} // namespace yup
