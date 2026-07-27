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

std::unique_ptr<LLMClient> LLMClientFactory::create (LLMClient::Options options)
{
    switch (options.provider)
    {
        case LLMClient::Provider::OpenAIChat:
            return std::make_unique<LLMOpenAIChatClient> (std::move (options));

        case LLMClient::Provider::OpenAIResponses:
            return std::make_unique<LLMOpenAIResponsesClient> (std::move (options));

        case LLMClient::Provider::Anthropic:
            return std::make_unique<LLMAnthropicClient> (std::move (options));

        case LLMClient::Provider::Gemini:
            return std::make_unique<LLMGeminiClient> (std::move (options));

        default:
            jassertfalse; // Unknown provider
            return nullptr;
    }
}

//==============================================================================
std::unique_ptr<LLMClient> LLMClientFactory::openAIChat (String model,
                                                         String baseUrl,
                                                         String apiKey)
{
    LLMClient::Options opts;
    opts.provider = LLMClient::Provider::OpenAIChat;
    opts.model = std::move (model);
    opts.baseUrl = std::move (baseUrl);
    opts.apiKey = std::move (apiKey);
    return create (std::move (opts));
}

std::unique_ptr<LLMClient> LLMClientFactory::openAIResponses (String model,
                                                              String apiKey,
                                                              String baseUrl)
{
    LLMClient::Options opts;
    opts.provider = LLMClient::Provider::OpenAIResponses;
    opts.model = std::move (model);
    opts.apiKey = std::move (apiKey);
    opts.baseUrl = std::move (baseUrl);
    return create (std::move (opts));
}

std::unique_ptr<LLMClient> LLMClientFactory::anthropic (String model,
                                                        String apiKey,
                                                        String baseUrl)
{
    LLMClient::Options opts;
    opts.provider = LLMClient::Provider::Anthropic;
    opts.model = std::move (model);
    opts.apiKey = std::move (apiKey);
    opts.baseUrl = std::move (baseUrl);
    return create (std::move (opts));
}

std::unique_ptr<LLMClient> LLMClientFactory::gemini (String model,
                                                     String apiKey,
                                                     String baseUrl)
{
    LLMClient::Options opts;
    opts.provider = LLMClient::Provider::Gemini;
    opts.model = std::move (model);
    opts.apiKey = std::move (apiKey);
    opts.baseUrl = std::move (baseUrl);
    return create (std::move (opts));
}

} // namespace yup
