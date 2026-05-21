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
var makeLLMMessageObject()
{
    return var (std::make_unique<DynamicObject>());
}

void setLLMMessageProperty (var& object, const Identifier& name, const var& value)
{
    if (auto* dynamicObject = object.getDynamicObject())
        dynamicObject->setProperty (name, value);
}

String argumentsToApiString (const var& arguments)
{
    if (arguments.isObject() || arguments.isArray())
        return JSON::toString (arguments, true);

    return arguments.toString();
}

var parseArguments (const var& arguments)
{
    if (! arguments.isString())
        return arguments;

    auto parsed = JSON::parse (arguments.toString());
    return parsed.isVoid() ? arguments : parsed;
}
} // namespace

var LLMToolCall::toVar() const
{
    auto functionObject = makeLLMMessageObject();
    setLLMMessageProperty (functionObject, "name", name);
    setLLMMessageProperty (functionObject, "arguments", argumentsToApiString (arguments));

    auto object = makeLLMMessageObject();
    setLLMMessageProperty (object, "id", id);
    setLLMMessageProperty (object, "type", "function");
    setLLMMessageProperty (object, "function", functionObject);

    return object;
}

std::optional<LLMToolCall> LLMToolCall::fromVar (const var& value)
{
    if (! value.isObject())
        return std::nullopt;

    LLMToolCall result;
    result.index = static_cast<int> (value["index"]);
    result.id = value["id"].toString();

    if (auto* functionObject = value["function"].getDynamicObject())
    {
        result.name = functionObject->getProperty ("name").toString();
        result.arguments = parseArguments (functionObject->getProperty ("arguments"));
    }
    else
    {
        result.name = value["name"].toString();
        result.arguments = parseArguments (value["arguments"]);
    }

    const auto hasArguments = ! result.arguments.isVoid()
                           && ! result.arguments.isUndefined()
                           && result.arguments.toString().isNotEmpty();

    if (result.name.isEmpty() && result.id.isEmpty() && ! hasArguments)
        return std::nullopt;

    return result;
}

LLMMessage LLMMessage::system (const String& content)
{
    LLMMessage result;
    result.role = Role::system;
    result.content = content;
    return result;
}

LLMMessage LLMMessage::user (const String& content)
{
    LLMMessage result;
    result.role = Role::user;
    result.content = content;
    return result;
}

LLMMessage LLMMessage::assistant (const String& content)
{
    LLMMessage result;
    result.role = Role::assistant;
    result.content = content;
    return result;
}

LLMMessage LLMMessage::toolResult (const String& toolCallId, const String& content)
{
    LLMMessage result;
    result.role = Role::tool;
    result.toolCallId = toolCallId;
    result.content = content;
    return result;
}

var LLMMessage::toVar() const
{
    auto object = makeLLMMessageObject();
    setLLMMessageProperty (object, "role", roleToString (role));
    setLLMMessageProperty (object, "content", content);

    if (name.isNotEmpty())
        setLLMMessageProperty (object, "name", name);

    if (toolCallId.has_value())
        setLLMMessageProperty (object, "tool_call_id", *toolCallId);

    if (toolCalls.has_value())
    {
        var calls;

        for (const auto& toolCall : *toolCalls)
            calls.append (toolCall.toVar());

        setLLMMessageProperty (object, "tool_calls", calls);
    }

    return object;
}

std::optional<LLMMessage> LLMMessage::fromVar (const var& value)
{
    if (! value.isObject())
        return std::nullopt;

    auto role = roleFromString (value["role"].toString());
    if (! role.has_value())
        return std::nullopt;

    LLMMessage result;
    result.role = *role;
    result.content = value["content"].toString();
    result.name = value["name"].toString();

    if (value.hasProperty ("tool_call_id"))
        result.toolCallId = value["tool_call_id"].toString();

    if (auto* calls = value["tool_calls"].getArray())
    {
        std::vector<LLMToolCall> parsedCalls;

        for (const auto& call : *calls)
            if (auto parsed = LLMToolCall::fromVar (call))
                parsedCalls.push_back (*parsed);

        result.toolCalls = std::move (parsedCalls);
    }

    return result;
}

String LLMMessage::roleToString (Role role)
{
    switch (role)
    {
        case Role::system:
            return "system";
        case Role::user:
            return "user";
        case Role::assistant:
            return "assistant";
        case Role::tool:
            return "tool";
    }

    jassertfalse;
    return "user";
}

std::optional<LLMMessage::Role> LLMMessage::roleFromString (const String& role)
{
    if (role == "system")
        return Role::system;

    if (role == "user")
        return Role::user;

    if (role == "assistant")
        return Role::assistant;

    if (role == "tool")
        return Role::tool;

    return std::nullopt;
}

} // namespace yup
