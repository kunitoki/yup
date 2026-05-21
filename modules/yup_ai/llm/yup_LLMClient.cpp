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
var makeLLMClientObject()
{
    return var (std::make_unique<DynamicObject>());
}

void setLLMClientProperty (var& object, const Identifier& name, const var& value)
{
    if (auto* dynamicObject = object.getDynamicObject())
        dynamicObject->setProperty (name, value);
}
} // namespace

LLMClient::LLMClient (Options optionsToUse)
    : options (std::move (optionsToUse))
{
}

LLMClient::~LLMClient() = default;

LLMResponse LLMClient::chat (const String& userMessage)
{
    Request request;
    request.messages.push_back (LLMMessage::user (userMessage));
    return complete (request);
}

LLMResponse LLMClient::chatWithTools (const String& userMessage, const LLMToolRegistry& tools)
{
    Request request;
    request.messages.push_back (LLMMessage::user (userMessage));
    request.tools = tools.getAllTools();
    request.toolChoice = "auto";
    return complete (request);
}

LLMResponse LLMClient::runToolLoop (const Request& request, LLMToolRegistry& tools)
{
    constexpr int maxToolIterations = 8;

    Request current = request;
    if (current.tools.empty())
        current.tools = tools.getAllTools();

    auto response = complete (current);

    for (int iteration = 0; iteration < maxToolIterations && response.hasToolCalls(); ++iteration)
    {
        for (const auto& choice : response.choices)
            current.messages.push_back (choice.message);

        for (const auto& toolCall : response.getToolCalls())
        {
            auto result = tools.dispatchToolCall (toolCall.name, toolCall.arguments);
            current.messages.push_back (LLMMessage::toolResult (toolCall.id, JSON::toString (result, true)));
        }

        response = complete (current);
    }

    return response;
}

String LLMClient::buildChatCompletionBody (const Request& request, bool stream) const
{
    auto object = makeLLMClientObject();

    if (options.model.isNotEmpty())
        setLLMClientProperty (object, "model", options.model);

    std::vector<LLMMessage> messages;
    messages.reserve (request.messages.size() + (request.systemPrompt.has_value() ? 1u : 0u));

    if (request.systemPrompt.has_value())
        messages.push_back (LLMMessage::system (*request.systemPrompt));

    messages.insert (messages.end(), request.messages.begin(), request.messages.end());

    setLLMClientProperty (object, "messages", messagesToVar (messages));
    setLLMClientProperty (object, "stream", stream);

    if (! request.tools.empty())
        setLLMClientProperty (object, "tools", toolsToVar (request.tools));

    if (request.toolChoice.has_value())
        setLLMClientProperty (object, "tool_choice", *request.toolChoice);

    if (request.temperature.has_value())
        setLLMClientProperty (object, "temperature", static_cast<double> (*request.temperature));

    if (request.topP.has_value())
        setLLMClientProperty (object, "top_p", static_cast<double> (*request.topP));

    if (request.maxTokens.has_value())
        setLLMClientProperty (object, "max_tokens", *request.maxTokens);

    if (request.stopSequences.has_value())
    {
        var stop;

        for (const auto& stopSequence : *request.stopSequences)
            stop.append (stopSequence);

        setLLMClientProperty (object, "stop", stop);
    }

    return JSON::toString (object, true);
}

var LLMClient::messagesToVar (const std::vector<LLMMessage>& messages) const
{
    var result;

    for (const auto& message : messages)
        result.append (message.toVar());

    return result;
}

var LLMClient::toolsToVar (const std::vector<LLMTool>& tools) const
{
    var result;

    for (const auto& tool : tools)
        result.append (tool.toJsonSchema());

    return result;
}

} // namespace yup
