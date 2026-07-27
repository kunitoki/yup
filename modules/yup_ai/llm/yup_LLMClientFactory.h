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
/** Factory that creates the correct LLMHttpClient subclass from an Options struct.

    Use LLMClientFactory::create() to instantiate an LLM client for any supported
    provider.  The Provider enum in LLMClient::Options selects the concrete class.

    @code
    yup::LLMClient::Options opts;
    opts.provider  = yup::LLMClient::Provider::Anthropic;
    opts.model     = "claude-opus-4-5";
    opts.apiKey    = "sk-ant-...";
    opts.baseUrl   = "https://api.anthropic.com/v1";

    auto client = yup::LLMClientFactory::create (opts);
    auto response = client->chat ("Hello, Claude!");
    @endcode

    Convenience static methods are provided for the most common provider setups.

    @tags{AI}
*/
class YUP_API LLMClientFactory
{
public:
    /** Creates an LLM client for the provider specified in @p options.

        @param options  Full options struct.  options.provider selects the concrete class.
        @returns        A heap-allocated concrete LLMHttpClient subclass, or nullptr if
                        the provider enum value is unrecognised.
    */
    static std::unique_ptr<LLMClient> create (LLMClient::Options options);

    //==============================================================================
    /** Convenience factory — OpenAI Chat Completions (also Ollama, DeepSeek, OpenRouter, llama-server). */
    static std::unique_ptr<LLMClient> openAIChat (String model,
                                                  String baseUrl = "http://localhost:11434/v1",
                                                  String apiKey = {});

    /** Convenience factory — OpenAI Responses API (GPT-5+, reasoning models). */
    static std::unique_ptr<LLMClient> openAIResponses (String model,
                                                       String apiKey,
                                                       String baseUrl = "https://api.openai.com/v1");

    /** Convenience factory — Anthropic Messages API (Claude models). */
    static std::unique_ptr<LLMClient> anthropic (String model,
                                                 String apiKey,
                                                 String baseUrl = "https://api.anthropic.com/v1");

    /** Convenience factory — Google Gemini generateContent API. */
    static std::unique_ptr<LLMClient> gemini (String model,
                                              String apiKey,
                                              String baseUrl = "https://generativelanguage.googleapis.com");
};

} // namespace yup
