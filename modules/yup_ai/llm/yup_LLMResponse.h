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
/** Parsed chat completion response.

    @tags{AI}
*/
class YUP_API LLMResponse
{
public:
    struct Choice
    {
        int index = 0;
        LLMMessage message;
        std::optional<String> finishReason;
    };

    struct Usage
    {
        int promptTokens = 0;
        int completionTokens = 0;
        int totalTokens = 0;
    };

    std::vector<Choice> choices;
    std::optional<Usage> usage;
    String model;

    /** Returns true if any choice contains assistant tool calls. */
    bool hasToolCalls() const noexcept;

    /** Returns all tool calls from all choices. */
    std::vector<LLMToolCall> getToolCalls() const;

    /** Parses a non-streaming OpenAI-compatible chat completion response. */
    static LLMResponse fromOpenAiJson (const var& json);

    /** Parses a streaming OpenAI-compatible chat completion delta chunk. */
    static LLMResponse fromStreamChunk (const var& json);
};

} // namespace yup
