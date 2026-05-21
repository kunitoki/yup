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
/** Describes a function call requested by a chat model.

    Tool calls use the OpenAI-compatible shape where the model supplies an id, a
    function name, and a JSON-compatible arguments value. String arguments returned
    by remote APIs are parsed into var objects when possible.

    @tags{AI}
*/
struct YUP_API LLMToolCall
{
    String id;
    String name;
    var arguments;

    /** Converts this tool call to an OpenAI-compatible JSON var object. */
    var toVar() const;

    /** Attempts to parse an OpenAI-compatible JSON var object into a tool call. */
    static std::optional<LLMToolCall> fromVar (const var& value);
};

//==============================================================================
/** A chat message for OpenAI-compatible completion APIs.

    The message can represent system, user, assistant, or tool-result content.
    Assistant messages may carry tool calls, while tool messages use toolCallId
    to correlate results with the requested call.

    @tags{AI}
*/
class YUP_API LLMMessage
{
public:
    enum class Role
    {
        system,
        user,
        assistant,
        tool
    };

    Role role = Role::user;
    String content;
    String name;
    std::optional<std::vector<LLMToolCall>> toolCalls;
    std::optional<String> toolCallId;

    /** Creates a system message. */
    static LLMMessage system (const String& content);

    /** Creates a user message. */
    static LLMMessage user (const String& content);

    /** Creates an assistant message. */
    static LLMMessage assistant (const String& content);

    /** Creates a tool-result message correlated with a tool call id. */
    static LLMMessage toolResult (const String& toolCallId, const String& content);

    /** Converts this message to an OpenAI ChatML-compatible JSON var object. */
    var toVar() const;

    /** Attempts to parse an OpenAI ChatML-compatible JSON var object into a message. */
    static std::optional<LLMMessage> fromVar (const var& value);

    /** Converts a role enum to its API string representation. */
    static String roleToString (Role role);

    /** Parses an API role string. */
    static std::optional<Role> roleFromString (const String& role);
};

} // namespace yup
