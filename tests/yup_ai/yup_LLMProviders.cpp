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

#include <gtest/gtest.h>

#include <yup_ai/yup_ai.h>

using namespace yup;

namespace
{

//==============================================================================
// Test wrappers that expose the protected virtual methods of each provider.

class TestableChatClient final : public LLMOpenAIChatClient
{
public:
    explicit TestableChatClient (Options options)
        : LLMOpenAIChatClient (std::move (options))
    {
    }

    String endpointUrl() const { return getEndpointUrl(); }

    String streamingEndpointUrl() const { return getStreamingEndpointUrl(); }

    String headers() const { return buildHeaders(); }

    String payload (const Request& r) const { return buildPayload (r); }

    String streamingPayload (const Request& r) const { return buildStreamingPayload (r); }

    LLMResponse response (const var& j) const { return parseResponse (j); }

    LLMResponse chunk (const var& j) const { return parseChunk (j); }
};

class TestableAnthropicClient final : public LLMAnthropicClient
{
public:
    explicit TestableAnthropicClient (Options options)
        : LLMAnthropicClient (std::move (options))
    {
    }

    String endpointUrl() const { return getEndpointUrl(); }

    String headers() const { return buildHeaders(); }

    String payload (const Request& r) const { return buildPayload (r); }

    LLMResponse response (const var& j) const { return parseResponse (j); }

    LLMResponse chunk (const var& j) const { return parseChunk (j); }
};

class TestableGeminiClient final : public LLMGeminiClient
{
public:
    explicit TestableGeminiClient (Options options)
        : LLMGeminiClient (std::move (options))
    {
    }

    String endpointUrl() const { return getEndpointUrl(); }

    String streamingEndpointUrl() const { return getStreamingEndpointUrl(); }

    String headers() const { return buildHeaders(); }

    String payload (const Request& r) const { return buildPayload (r); }

    String streamingPayload (const Request& r) const { return buildStreamingPayload (r); }

    LLMResponse response (const var& j) const { return parseResponse (j); }

    LLMResponse chunk (const var& j) const { return parseChunk (j); }
};

class TestableResponsesClient final : public LLMOpenAIResponsesClient
{
public:
    explicit TestableResponsesClient (Options options)
        : LLMOpenAIResponsesClient (std::move (options))
    {
    }

    String endpointUrl() const { return getEndpointUrl(); }

    String headers() const { return buildHeaders(); }

    String payload (const Request& r) const { return buildPayload (r); }

    LLMResponse response (const var& j) const { return parseResponse (j); }

    LLMResponse chunk (const var& j) const { return parseChunk (j); }
};

//==============================================================================
LLMClient::Options makeOptions (LLMClient::Provider provider,
                                const String& model,
                                const String& baseUrl,
                                const String& apiKey = {})
{
    LLMClient::Options opts;
    opts.provider = provider;
    opts.model = model;
    opts.baseUrl = baseUrl;
    opts.apiKey = apiKey;
    return opts;
}

} // namespace

//==============================================================================
// LLMOpenAIChatClient

TEST (YupAiOpenAIChatClient, GetEndpointUrl_AppendsChatCompletionsPath)
{
    TestableChatClient client (makeOptions (LLMClient::Provider::OpenAIChat,
                                            "gpt-4o",
                                            "https://api.openai.com/v1",
                                            "sk-key"));

    EXPECT_EQ ("https://api.openai.com/v1/chat/completions", client.endpointUrl());
}

TEST (YupAiOpenAIChatClient, GetEndpointUrl_StripsTrailingSlashFromBaseUrl)
{
    TestableChatClient client (makeOptions (LLMClient::Provider::OpenAIChat,
                                            "gpt-4o",
                                            "https://api.openai.com/v1/",
                                            "sk-key"));

    EXPECT_EQ ("https://api.openai.com/v1/chat/completions", client.endpointUrl());
}

TEST (YupAiOpenAIChatClient, GetStreamingEndpointUrl_SameAsNonStreaming)
{
    TestableChatClient client (makeOptions (LLMClient::Provider::OpenAIChat,
                                            "gpt-4o",
                                            "https://api.openai.com/v1",
                                            "sk-key"));

    EXPECT_EQ (client.endpointUrl(), client.streamingEndpointUrl());
}

TEST (YupAiOpenAIChatClient, BuildHeaders_IncludesBearerToken)
{
    TestableChatClient client (makeOptions (LLMClient::Provider::OpenAIChat,
                                            "gpt-4o",
                                            "https://api.openai.com/v1",
                                            "sk-test-key"));

    EXPECT_TRUE (client.headers().contains ("Authorization: Bearer sk-test-key"));
}

TEST (YupAiOpenAIChatClient, BuildHeaders_OmitsBearerWhenApiKeyEmpty)
{
    TestableChatClient client (makeOptions (LLMClient::Provider::OpenAIChat,
                                            "gemma4",
                                            "http://localhost:11434/v1"));

    EXPECT_FALSE (client.headers().contains ("Authorization:"));
}

TEST (YupAiOpenAIChatClient, BuildHeaders_OpenRouter_InjectsTitleAndReferer)
{
    LLMClient::Options opts = makeOptions (LLMClient::Provider::OpenAIChat,
                                           "openai/gpt-4o",
                                           "https://openrouter.ai/api/v1",
                                           "or-key");
    opts.userAgent = "MyApp";
    opts.appUrl = "https://myapp.com";
    TestableChatClient client (opts);

    const auto h = client.headers();
    EXPECT_TRUE (h.contains ("X-Title: MyApp"));
    EXPECT_TRUE (h.contains ("HTTP-Referer: https://myapp.com"));
}

TEST (YupAiOpenAIChatClient, BuildPayload_ContainsModelName)
{
    TestableChatClient client (makeOptions (LLMClient::Provider::OpenAIChat,
                                            "gpt-4o",
                                            "https://api.openai.com/v1",
                                            "sk-key"));

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::user ("hello"));

    const auto body = JSON::parse (client.payload (request));
    EXPECT_EQ ("gpt-4o", body["model"].toString());
}

TEST (YupAiOpenAIChatClient, BuildPayload_StreamFlagIsFalse)
{
    TestableChatClient client (makeOptions (LLMClient::Provider::OpenAIChat,
                                            "gpt-4o",
                                            "https://api.openai.com/v1",
                                            "sk-key"));

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::user ("hello"));

    const auto body = JSON::parse (client.payload (request));
    EXPECT_FALSE (static_cast<bool> (body["stream"]));
}

TEST (YupAiOpenAIChatClient, BuildStreamingPayload_StreamFlagIsTrue)
{
    TestableChatClient client (makeOptions (LLMClient::Provider::OpenAIChat,
                                            "gpt-4o",
                                            "https://api.openai.com/v1",
                                            "sk-key"));

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::user ("hello"));

    const auto body = JSON::parse (client.streamingPayload (request));
    EXPECT_TRUE (static_cast<bool> (body["stream"]));
}

TEST (YupAiOpenAIChatClient, BuildPayload_SetsMaxCompletionTokens)
{
    TestableChatClient client (makeOptions (LLMClient::Provider::OpenAIChat,
                                            "gpt-4o",
                                            "https://api.openai.com/v1",
                                            "sk-key"));

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::user ("hello"));
    request.maxTokens = 100;

    const auto body = JSON::parse (client.payload (request));
    EXPECT_EQ (100, static_cast<int> (body["max_completion_tokens"]));
}

TEST (YupAiOpenAIChatClient, BuildPayload_SystemPromptAppearsFirst)
{
    TestableChatClient client (makeOptions (LLMClient::Provider::OpenAIChat,
                                            "gpt-4o",
                                            "https://api.openai.com/v1",
                                            "sk-key"));

    LLMClient::Request request;
    request.systemPrompt = "be brief";
    request.messages.push_back (LLMMessage::user ("hello"));

    const auto body = JSON::parse (client.payload (request));
    ASSERT_TRUE (body["messages"].isArray());
    EXPECT_EQ ("system", body["messages"][0]["role"].toString());
    EXPECT_EQ ("user", body["messages"][1]["role"].toString());
}

TEST (YupAiOpenAIChatClient, ParseResponse_ExtractsChoiceContent)
{
    TestableChatClient client (makeOptions (LLMClient::Provider::OpenAIChat,
                                            "gpt-4o",
                                            "https://api.openai.com/v1",
                                            "sk-key"));

    const auto json = JSON::parse (R"({
        "model": "gpt-4o",
        "choices": [
            {
                "index": 0,
                "message": { "role": "assistant", "content": "world" },
                "finish_reason": "stop"
            }
        ]
    })");

    const auto response = client.response (json);
    ASSERT_EQ (1u, response.choices.size());
    EXPECT_EQ ("world", response.choices.front().message.content);
}

TEST (YupAiOpenAIChatClient, ParseChunk_ExtractsDeltaContent)
{
    TestableChatClient client (makeOptions (LLMClient::Provider::OpenAIChat,
                                            "gpt-4o",
                                            "https://api.openai.com/v1",
                                            "sk-key"));

    const auto json = JSON::parse (R"({
        "choices": [
            {
                "index": 0,
                "delta": { "role": "assistant", "content": "tok" },
                "finish_reason": null
            }
        ]
    })");

    const auto response = client.chunk (json);
    ASSERT_EQ (1u, response.choices.size());
    EXPECT_EQ ("tok", response.choices.front().message.content);
}

//==============================================================================
// LLMAnthropicClient

TEST (YupAiAnthropicClient, GetEndpointUrl_AppendsMessagesPath)
{
    TestableAnthropicClient client (makeOptions (LLMClient::Provider::Anthropic,
                                                 "claude-opus-4-5",
                                                 "https://api.anthropic.com/v1",
                                                 "ant-key"));

    EXPECT_EQ ("https://api.anthropic.com/v1/messages", client.endpointUrl());
}

TEST (YupAiAnthropicClient, BuildHeaders_IncludesApiKey)
{
    TestableAnthropicClient client (makeOptions (LLMClient::Provider::Anthropic,
                                                 "claude-opus-4-5",
                                                 "https://api.anthropic.com/v1",
                                                 "ant-key"));

    EXPECT_TRUE (client.headers().contains ("x-api-key: ant-key"));
}

TEST (YupAiAnthropicClient, BuildHeaders_IncludesAnthropicVersion)
{
    TestableAnthropicClient client (makeOptions (LLMClient::Provider::Anthropic,
                                                 "claude-opus-4-5",
                                                 "https://api.anthropic.com/v1",
                                                 "ant-key"));

    EXPECT_TRUE (client.headers().contains ("anthropic-version: 2023-06-01"));
}

TEST (YupAiAnthropicClient, BuildHeaders_DoesNotUseBearerScheme)
{
    TestableAnthropicClient client (makeOptions (LLMClient::Provider::Anthropic,
                                                 "claude-opus-4-5",
                                                 "https://api.anthropic.com/v1",
                                                 "ant-key"));

    EXPECT_FALSE (client.headers().contains ("Authorization: Bearer"));
}

TEST (YupAiAnthropicClient, BuildPayload_DefaultsMaxTokensTo4096)
{
    TestableAnthropicClient client (makeOptions (LLMClient::Provider::Anthropic,
                                                 "claude-opus-4-5",
                                                 "https://api.anthropic.com/v1",
                                                 "ant-key"));

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::user ("hello"));

    const auto body = JSON::parse (client.payload (request));
    EXPECT_EQ (4096, static_cast<int> (body["max_tokens"]));
}

TEST (YupAiAnthropicClient, BuildPayload_RespectsRequestMaxTokens)
{
    TestableAnthropicClient client (makeOptions (LLMClient::Provider::Anthropic,
                                                 "claude-opus-4-5",
                                                 "https://api.anthropic.com/v1",
                                                 "ant-key"));

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::user ("hello"));
    request.maxTokens = 512;

    const auto body = JSON::parse (client.payload (request));
    EXPECT_EQ (512, static_cast<int> (body["max_tokens"]));
}

TEST (YupAiAnthropicClient, BuildPayload_SystemPromptGoesToSystemFieldWithCacheControl)
{
    TestableAnthropicClient client (makeOptions (LLMClient::Provider::Anthropic,
                                                 "claude-opus-4-5",
                                                 "https://api.anthropic.com/v1",
                                                 "ant-key"));

    LLMClient::Request request;
    request.systemPrompt = "You are helpful.";
    request.messages.push_back (LLMMessage::user ("hello"));

    const auto body = JSON::parse (client.payload (request));

    ASSERT_TRUE (body["system"].isArray());
    EXPECT_EQ ("text", body["system"][0]["type"].toString());
    EXPECT_EQ ("You are helpful.", body["system"][0]["text"].toString());
    EXPECT_EQ ("ephemeral", body["system"][0]["cache_control"]["type"].toString());
}

TEST (YupAiAnthropicClient, BuildPayload_FiltersSystemMessagesFromMessagesArray)
{
    TestableAnthropicClient client (makeOptions (LLMClient::Provider::Anthropic,
                                                 "claude-opus-4-5",
                                                 "https://api.anthropic.com/v1",
                                                 "ant-key"));

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::system ("system directive"));
    request.messages.push_back (LLMMessage::user ("question"));
    request.messages.push_back (LLMMessage::assistant ("answer"));

    const auto body = JSON::parse (client.payload (request));
    auto* messages = body["messages"].getArray();

    ASSERT_NE (nullptr, messages);
    EXPECT_EQ (2u, messages->size());
    EXPECT_EQ ("user", (*messages)[0]["role"].toString());
    EXPECT_EQ ("assistant", (*messages)[1]["role"].toString());
}

TEST (YupAiAnthropicClient, ParseResponse_ExtractsContentArrayText)
{
    TestableAnthropicClient client (makeOptions (LLMClient::Provider::Anthropic,
                                                 "claude-opus-4-5",
                                                 "https://api.anthropic.com/v1",
                                                 "ant-key"));

    const auto json = JSON::parse (R"({
        "model": "claude-opus-4-5",
        "content": [{ "type": "text", "text": "  hello  " }],
        "stop_reason": "end_turn",
        "usage": { "input_tokens": 5, "output_tokens": 3 }
    })");

    const auto response = client.response (json);
    ASSERT_EQ (1u, response.choices.size());
    EXPECT_EQ ("hello", response.choices.front().message.content);
    ASSERT_TRUE (response.usage.has_value());
    EXPECT_EQ (8, response.usage->totalTokens);
}

TEST (YupAiAnthropicClient, ParseResponse_ReportsApiError)
{
    TestableAnthropicClient client (makeOptions (LLMClient::Provider::Anthropic,
                                                 "claude-opus-4-5",
                                                 "https://api.anthropic.com/v1",
                                                 "ant-key"));

    const auto json = JSON::parse (R"({ "error": { "message": "invalid api key" } })");

    const auto response = client.response (json);
    EXPECT_TRUE (response.failed());
    ASSERT_TRUE (response.errorMessage.has_value());
    EXPECT_EQ ("invalid api key", *response.errorMessage);
}

TEST (YupAiAnthropicClient, ParseChunk_ExtractsContentBlockDeltaText)
{
    TestableAnthropicClient client (makeOptions (LLMClient::Provider::Anthropic,
                                                 "claude-opus-4-5",
                                                 "https://api.anthropic.com/v1",
                                                 "ant-key"));

    const auto json = JSON::parse (R"({
        "type": "content_block_delta",
        "index": 0,
        "delta": { "type": "text_delta", "text": "tok" }
    })");

    const auto response = client.chunk (json);
    ASSERT_EQ (1u, response.choices.size());
    EXPECT_EQ ("tok", response.choices.front().message.content);
    EXPECT_EQ (LLMMessage::Role::assistant, response.choices.front().message.role);
}

TEST (YupAiAnthropicClient, ParseChunk_IgnoresNonContentBlockDeltaEvents)
{
    TestableAnthropicClient client (makeOptions (LLMClient::Provider::Anthropic,
                                                 "claude-opus-4-5",
                                                 "https://api.anthropic.com/v1",
                                                 "ant-key"));

    const auto json = JSON::parse (R"({ "type": "message_stop" })");

    const auto response = client.chunk (json);
    EXPECT_TRUE (response.choices.empty());
}

//==============================================================================
// LLMGeminiClient

TEST (YupAiGeminiClient, GetEndpointUrl_IncludesModelNameAndGenerateContent)
{
    TestableGeminiClient client (makeOptions (LLMClient::Provider::Gemini,
                                              "gemini-2.5-flash",
                                              "https://generativelanguage.googleapis.com",
                                              "gemini-key"));

    EXPECT_EQ ("https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent",
               client.endpointUrl());
}

TEST (YupAiGeminiClient, GetStreamingEndpointUrl_UsesStreamGenerateContentWithSse)
{
    TestableGeminiClient client (makeOptions (LLMClient::Provider::Gemini,
                                              "gemini-2.5-flash",
                                              "https://generativelanguage.googleapis.com",
                                              "gemini-key"));

    EXPECT_EQ ("https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:streamGenerateContent?alt=sse",
               client.streamingEndpointUrl());
}

TEST (YupAiGeminiClient, BuildHeaders_IncludesGoogApiKey)
{
    TestableGeminiClient client (makeOptions (LLMClient::Provider::Gemini,
                                              "gemini-2.5-flash",
                                              "https://generativelanguage.googleapis.com",
                                              "gemini-key"));

    EXPECT_TRUE (client.headers().contains ("x-goog-api-key: gemini-key"));
}

TEST (YupAiGeminiClient, BuildPayload_MapsAssistantRoleToModel)
{
    TestableGeminiClient client (makeOptions (LLMClient::Provider::Gemini,
                                              "gemini-2.5-flash",
                                              "https://generativelanguage.googleapis.com",
                                              "gemini-key"));

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::user ("hello"));
    request.messages.push_back (LLMMessage::assistant ("hi there"));

    const auto body = JSON::parse (client.payload (request));
    auto* contents = body["contents"].getArray();

    ASSERT_NE (nullptr, contents);
    ASSERT_EQ (2u, contents->size());
    EXPECT_EQ ("user", (*contents)[0]["role"].toString());
    EXPECT_EQ ("model", (*contents)[1]["role"].toString());
}

TEST (YupAiGeminiClient, BuildPayload_FiltersSystemMessages)
{
    TestableGeminiClient client (makeOptions (LLMClient::Provider::Gemini,
                                              "gemini-2.5-flash",
                                              "https://generativelanguage.googleapis.com",
                                              "gemini-key"));

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::system ("system text"));
    request.messages.push_back (LLMMessage::user ("question"));

    const auto body = JSON::parse (client.payload (request));
    auto* contents = body["contents"].getArray();

    ASSERT_NE (nullptr, contents);
    EXPECT_EQ (1u, contents->size());
    EXPECT_EQ ("user", (*contents)[0]["role"].toString());
}

TEST (YupAiGeminiClient, BuildPayload_SetsSystemInstruction)
{
    TestableGeminiClient client (makeOptions (LLMClient::Provider::Gemini,
                                              "gemini-2.5-flash",
                                              "https://generativelanguage.googleapis.com",
                                              "gemini-key"));

    LLMClient::Request request;
    request.systemPrompt = "You are a helpful assistant.";
    request.messages.push_back (LLMMessage::user ("hello"));

    const auto body = JSON::parse (client.payload (request));
    auto* parts = body["system_instruction"]["parts"].getArray();

    ASSERT_NE (nullptr, parts);
    ASSERT_FALSE (parts->isEmpty());
    EXPECT_EQ ("You are a helpful assistant.", (*parts)[0]["text"].toString());
}

TEST (YupAiGeminiClient, BuildPayload_ReasoningEffortLow_SetsThinkingBudget1024)
{
    LLMClient::Options opts = makeOptions (LLMClient::Provider::Gemini,
                                           "gemini-2.5-flash",
                                           "https://generativelanguage.googleapis.com",
                                           "gemini-key");
    opts.reasoningEffort = "low";
    TestableGeminiClient client (opts);

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::user ("hello"));

    const auto body = JSON::parse (client.payload (request));
    EXPECT_EQ (1024,
               static_cast<int> (body["generationConfig"]["thinkingConfig"]["thinkingBudget"]));
}

TEST (YupAiGeminiClient, BuildPayload_ReasoningEffortHigh_SetsThinkingBudget16384)
{
    LLMClient::Options opts = makeOptions (LLMClient::Provider::Gemini,
                                           "gemini-2.5-flash",
                                           "https://generativelanguage.googleapis.com",
                                           "gemini-key");
    opts.reasoningEffort = "high";
    TestableGeminiClient client (opts);

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::user ("hello"));

    const auto body = JSON::parse (client.payload (request));
    EXPECT_EQ (16384,
               static_cast<int> (body["generationConfig"]["thinkingConfig"]["thinkingBudget"]));
}

TEST (YupAiGeminiClient, BuildPayload_ReasoningEffortDefault_SetsThinkingBudget4096)
{
    LLMClient::Options opts = makeOptions (LLMClient::Provider::Gemini,
                                           "gemini-2.5-flash",
                                           "https://generativelanguage.googleapis.com",
                                           "gemini-key");
    opts.reasoningEffort = "medium";
    TestableGeminiClient client (opts);

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::user ("hello"));

    const auto body = JSON::parse (client.payload (request));
    EXPECT_EQ (4096,
               static_cast<int> (body["generationConfig"]["thinkingConfig"]["thinkingBudget"]));
}

TEST (YupAiGeminiClient, BuildStreamingPayload_MatchesNonStreamingPayload)
{
    TestableGeminiClient client (makeOptions (LLMClient::Provider::Gemini,
                                              "gemini-2.5-flash",
                                              "https://generativelanguage.googleapis.com",
                                              "gemini-key"));

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::user ("hello"));

    EXPECT_EQ (client.payload (request), client.streamingPayload (request));
}

TEST (YupAiGeminiClient, ParseResponse_ExtractsCandidateText)
{
    TestableGeminiClient client (makeOptions (LLMClient::Provider::Gemini,
                                              "gemini-2.5-flash",
                                              "https://generativelanguage.googleapis.com",
                                              "gemini-key"));

    const auto json = JSON::parse (R"({
        "candidates": [
            {
                "content": { "parts": [{ "text": "  response text  " }] },
                "finishReason": "STOP"
            }
        ]
    })");

    const auto response = client.response (json);
    ASSERT_EQ (1u, response.choices.size());
    EXPECT_EQ ("response text", response.choices.front().message.content);
    ASSERT_TRUE (response.choices.front().finishReason.has_value());
    EXPECT_EQ ("STOP", *response.choices.front().finishReason);
}

TEST (YupAiGeminiClient, ParseResponse_ReportsApiError)
{
    TestableGeminiClient client (makeOptions (LLMClient::Provider::Gemini,
                                              "gemini-2.5-flash",
                                              "https://generativelanguage.googleapis.com",
                                              "gemini-key"));

    const auto json = JSON::parse (R"({ "error": { "message": "quota exceeded" } })");

    const auto response = client.response (json);
    EXPECT_TRUE (response.failed());
    ASSERT_TRUE (response.errorMessage.has_value());
    EXPECT_EQ ("quota exceeded", *response.errorMessage);
}

TEST (YupAiGeminiClient, ParseChunk_UsesSameCandidatesStructure)
{
    TestableGeminiClient client (makeOptions (LLMClient::Provider::Gemini,
                                              "gemini-2.5-flash",
                                              "https://generativelanguage.googleapis.com",
                                              "gemini-key"));

    const auto json = JSON::parse (R"({
        "candidates": [
            { "content": { "parts": [{ "text": "delta" }] } }
        ]
    })");

    const auto response = client.chunk (json);
    ASSERT_EQ (1u, response.choices.size());
    EXPECT_EQ ("delta", response.choices.front().message.content);
}

TEST (YupAiGeminiClient, BuildPayload_ToolsUseFunctionDeclarationsKey)
{
    TestableGeminiClient client (makeOptions (LLMClient::Provider::Gemini,
                                              "gemini-2.5-flash",
                                              "https://generativelanguage.googleapis.com",
                                              "gemini-key"));

    LLMTool tool;
    tool.name = "get_weather";
    tool.description = "Returns current weather.";
    tool.parameters.push_back ({ "city", "string", "City name", true });

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::user ("what's the weather?"));
    request.tools.push_back (std::move (tool));

    const auto body = JSON::parse (client.payload (request));

    ASSERT_TRUE (body["tools"].isArray());
    EXPECT_EQ (1, body["tools"].size());

    // Key must be camelCase "functionDeclarations", not snake_case.
    const auto& decls = body["tools"][0]["functionDeclarations"];
    ASSERT_TRUE (decls.isArray());
    ASSERT_EQ (1, decls.size());
    EXPECT_EQ ("get_weather", decls[0]["name"].toString());
    EXPECT_EQ ("string", decls[0]["parameters"]["properties"]["city"]["type"].toString());
}

TEST (YupAiGeminiClient, BuildPayload_ToolConfigDefaultsToAutoMode)
{
    TestableGeminiClient client (makeOptions (LLMClient::Provider::Gemini,
                                              "gemini-2.5-flash",
                                              "https://generativelanguage.googleapis.com",
                                              "gemini-key"));

    LLMTool tool;
    tool.name = "echo";

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::user ("hi"));
    request.tools.push_back (std::move (tool));

    const auto body = JSON::parse (client.payload (request));

    // Keys must be camelCase: toolConfig → functionCallingConfig → mode.
    EXPECT_EQ ("AUTO", body["toolConfig"]["functionCallingConfig"]["mode"].toString());
}

TEST (YupAiGeminiClient, BuildPayload_ToolChoiceNone_SetsNoneMode)
{
    TestableGeminiClient client (makeOptions (LLMClient::Provider::Gemini,
                                              "gemini-2.5-flash",
                                              "https://generativelanguage.googleapis.com",
                                              "gemini-key"));

    LLMTool tool;
    tool.name = "echo";

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::user ("hi"));
    request.tools.push_back (std::move (tool));
    request.toolChoice = "none";

    const auto body = JSON::parse (client.payload (request));
    EXPECT_EQ ("NONE", body["toolConfig"]["functionCallingConfig"]["mode"].toString());
}

TEST (YupAiGeminiClient, BuildPayload_ToolChoiceRequired_SetsAnyMode)
{
    TestableGeminiClient client (makeOptions (LLMClient::Provider::Gemini,
                                              "gemini-2.5-flash",
                                              "https://generativelanguage.googleapis.com",
                                              "gemini-key"));

    LLMTool tool;
    tool.name = "echo";

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::user ("hi"));
    request.tools.push_back (std::move (tool));
    request.toolChoice = "required";

    const auto body = JSON::parse (client.payload (request));
    EXPECT_EQ ("ANY", body["toolConfig"]["functionCallingConfig"]["mode"].toString());
}

TEST (YupAiGeminiClient, BuildPayload_SpecificToolChoice_SetsAllowedFunctionNames)
{
    TestableGeminiClient client (makeOptions (LLMClient::Provider::Gemini,
                                              "gemini-2.5-flash",
                                              "https://generativelanguage.googleapis.com",
                                              "gemini-key"));

    LLMTool tool;
    tool.name = "get_weather";

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::user ("weather?"));
    request.tools.push_back (std::move (tool));
    request.toolChoice = "get_weather";

    const auto body = JSON::parse (client.payload (request));

    const auto& fcc = body["toolConfig"]["functionCallingConfig"];
    EXPECT_EQ ("ANY", fcc["mode"].toString());
    ASSERT_TRUE (fcc["allowedFunctionNames"].isArray());
    EXPECT_EQ ("get_weather", fcc["allowedFunctionNames"][0].toString());
}

TEST (YupAiGeminiClient, BuildPayload_ToolResultMessage_BecomesUserFunctionResponse)
{
    TestableGeminiClient client (makeOptions (LLMClient::Provider::Gemini,
                                              "gemini-2.5-flash",
                                              "https://generativelanguage.googleapis.com",
                                              "gemini-key"));

    auto toolResult = LLMMessage::toolResult ("get_weather", R"({"result":"sunny"})");
    toolResult.name = "get_weather"; // set by updated runToolLoop

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::user ("weather?"));
    request.messages.push_back (toolResult);

    const auto body = JSON::parse (client.payload (request));
    auto* contents = body["contents"].getArray();

    ASSERT_NE (nullptr, contents);
    ASSERT_EQ (2u, contents->size());

    const auto& turn = (*contents)[1];
    EXPECT_EQ ("user", turn["role"].toString());

    const auto& fr = turn["parts"][0]["functionResponse"];
    EXPECT_EQ ("get_weather", fr["name"].toString());
    EXPECT_EQ ("sunny", fr["response"]["result"].toString());
}

TEST (YupAiGeminiClient, BuildPayload_AssistantWithToolCalls_BecomesModelFunctionCallTurn)
{
    TestableGeminiClient client (makeOptions (LLMClient::Provider::Gemini,
                                              "gemini-2.5-flash",
                                              "https://generativelanguage.googleapis.com",
                                              "gemini-key"));

    LLMToolCall toolCall;
    toolCall.index = 0;
    toolCall.id = "get_weather";
    toolCall.name = "get_weather";
    toolCall.arguments = JSON::parse (R"({"city":"London"})");

    auto assistantMsg = LLMMessage::assistant ("");
    assistantMsg.toolCalls = std::vector<LLMToolCall> { toolCall };

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::user ("weather?"));
    request.messages.push_back (assistantMsg);

    const auto body = JSON::parse (client.payload (request));
    auto* contents = body["contents"].getArray();

    ASSERT_NE (nullptr, contents);
    ASSERT_EQ (2u, contents->size());

    const auto& modelTurn = (*contents)[1];
    EXPECT_EQ ("model", modelTurn["role"].toString());

    const auto& fc = modelTurn["parts"][0]["functionCall"];
    EXPECT_EQ ("get_weather", fc["name"].toString());
    EXPECT_EQ ("London", fc["args"]["city"].toString());
}

TEST (YupAiGeminiClient, ParseResponse_ExtractsFunctionCallPart)
{
    TestableGeminiClient client (makeOptions (LLMClient::Provider::Gemini,
                                              "gemini-2.5-flash",
                                              "https://generativelanguage.googleapis.com",
                                              "gemini-key"));

    const auto json = JSON::parse (R"({
        "candidates": [{
            "content": {
                "parts": [{
                    "functionCall": {
                        "id": "call_abc",
                        "name": "get_weather",
                        "args": { "city": "Paris" }
                    }
                }]
            },
            "finishReason": "STOP"
        }]
    })");

    const auto response = client.response (json);
    ASSERT_EQ (1u, response.choices.size());
    ASSERT_TRUE (response.hasToolCalls());

    const auto toolCalls = response.getToolCalls();
    ASSERT_EQ (1u, toolCalls.size());
    EXPECT_EQ ("get_weather", toolCalls.front().name);
    EXPECT_EQ ("call_abc", toolCalls.front().id);
    EXPECT_EQ ("Paris", toolCalls.front().arguments["city"].toString());
}

TEST (YupAiGeminiClient, ParseResponse_FunctionCallId_FallsBackToNameWhenAbsent)
{
    TestableGeminiClient client (makeOptions (LLMClient::Provider::Gemini,
                                              "gemini-2.5-flash",
                                              "https://generativelanguage.googleapis.com",
                                              "gemini-key"));

    const auto json = JSON::parse (R"({
        "candidates": [{
            "content": {
                "parts": [{
                    "functionCall": {
                        "name": "echo",
                        "args": {}
                    }
                }]
            }
        }]
    })");

    const auto response = client.response (json);
    const auto toolCalls = response.getToolCalls();
    ASSERT_EQ (1u, toolCalls.size());
    // id should fall back to the function name when no "id" field is present
    EXPECT_EQ ("echo", toolCalls.front().id);
    EXPECT_EQ ("echo", toolCalls.front().name);
}

//==============================================================================
// LLMOpenAIResponsesClient

TEST (YupAiOpenAIResponsesClient, GetEndpointUrl_AppendsResponsesPath)
{
    TestableResponsesClient client (makeOptions (LLMClient::Provider::OpenAIResponses,
                                                 "gpt-4.1",
                                                 "https://api.openai.com/v1",
                                                 "sk-key"));

    EXPECT_EQ ("https://api.openai.com/v1/responses", client.endpointUrl());
}

TEST (YupAiOpenAIResponsesClient, BuildHeaders_IncludesBearerToken)
{
    TestableResponsesClient client (makeOptions (LLMClient::Provider::OpenAIResponses,
                                                 "gpt-4.1",
                                                 "https://api.openai.com/v1",
                                                 "sk-resp-key"));

    EXPECT_TRUE (client.headers().contains ("Authorization: Bearer sk-resp-key"));
}

TEST (YupAiOpenAIResponsesClient, BuildPayload_SingleUserMessage_UsesStringInput)
{
    TestableResponsesClient client (makeOptions (LLMClient::Provider::OpenAIResponses,
                                                 "gpt-4.1",
                                                 "https://api.openai.com/v1",
                                                 "sk-key"));

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::user ("single message"));

    const auto body = JSON::parse (client.payload (request));
    EXPECT_EQ ("single message", body["input"].toString());
    EXPECT_FALSE (body["input"].isArray());
}

TEST (YupAiOpenAIResponsesClient, BuildPayload_MultiTurnMessages_UsesArrayInput)
{
    TestableResponsesClient client (makeOptions (LLMClient::Provider::OpenAIResponses,
                                                 "gpt-4.1",
                                                 "https://api.openai.com/v1",
                                                 "sk-key"));

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::user ("question"));
    request.messages.push_back (LLMMessage::assistant ("answer"));
    request.messages.push_back (LLMMessage::user ("follow-up"));

    const auto body = JSON::parse (client.payload (request));
    ASSERT_TRUE (body["input"].isArray());
    EXPECT_EQ (3, body["input"].size());
    EXPECT_EQ ("user", body["input"][0]["role"].toString());
    EXPECT_EQ ("assistant", body["input"][1]["role"].toString());
    EXPECT_EQ ("user", body["input"][2]["role"].toString());
}

TEST (YupAiOpenAIResponsesClient, BuildPayload_SetsSystemPromptAsInstructions)
{
    TestableResponsesClient client (makeOptions (LLMClient::Provider::OpenAIResponses,
                                                 "gpt-4.1",
                                                 "https://api.openai.com/v1",
                                                 "sk-key"));

    LLMClient::Request request;
    request.systemPrompt = "Be concise.";
    request.messages.push_back (LLMMessage::user ("hello"));

    const auto body = JSON::parse (client.payload (request));
    EXPECT_EQ ("Be concise.", body["instructions"].toString());
}

TEST (YupAiOpenAIResponsesClient, BuildPayload_SetsReasoningEffort)
{
    LLMClient::Options opts = makeOptions (LLMClient::Provider::OpenAIResponses,
                                           "gpt-4.1",
                                           "https://api.openai.com/v1",
                                           "sk-key");
    opts.reasoningEffort = "high";
    TestableResponsesClient client (opts);

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::user ("solve this"));

    const auto body = JSON::parse (client.payload (request));
    EXPECT_EQ ("high", body["reasoning"]["effort"].toString());
}

TEST (YupAiOpenAIResponsesClient, BuildPayload_NoTemperatureFlag_OmitsTemperatureField)
{
    LLMClient::Options opts = makeOptions (LLMClient::Provider::OpenAIResponses,
                                           "gpt-4.1",
                                           "https://api.openai.com/v1",
                                           "sk-key");
    opts.noTemperature = true;
    TestableResponsesClient client (opts);

    LLMClient::Request request;
    request.temperature = 0.5f;
    request.messages.push_back (LLMMessage::user ("hello"));

    const auto body = JSON::parse (client.payload (request));
    EXPECT_TRUE (body["temperature"].isVoid());
}

TEST (YupAiOpenAIResponsesClient, ParseResponse_ExtractsOutputTextFromMessageItem)
{
    TestableResponsesClient client (makeOptions (LLMClient::Provider::OpenAIResponses,
                                                 "gpt-4.1",
                                                 "https://api.openai.com/v1",
                                                 "sk-key"));

    const auto json = JSON::parse (R"({
        "output": [
            {
                "type": "message",
                "content": [
                    { "type": "output_text", "text": "  result  " }
                ]
            }
        ]
    })");

    const auto response = client.response (json);
    ASSERT_EQ (1u, response.choices.size());
    EXPECT_EQ ("result", response.choices.front().message.content);
}

TEST (YupAiOpenAIResponsesClient, ParseResponse_ExtractsGrammarToolCallInput)
{
    TestableResponsesClient client (makeOptions (LLMClient::Provider::OpenAIResponses,
                                                 "gpt-4.1",
                                                 "https://api.openai.com/v1",
                                                 "sk-key"));

    const auto json = JSON::parse (R"({
        "output": [
            { "type": "custom_tool_call", "input": "constrained output" }
        ]
    })");

    const auto response = client.response (json);
    ASSERT_EQ (1u, response.choices.size());
    EXPECT_EQ ("constrained output", response.choices.front().message.content);
}

TEST (YupAiOpenAIResponsesClient, ParseResponse_ReportsApiError)
{
    TestableResponsesClient client (makeOptions (LLMClient::Provider::OpenAIResponses,
                                                 "gpt-4.1",
                                                 "https://api.openai.com/v1",
                                                 "sk-key"));

    const auto json = JSON::parse (R"({ "error": { "message": "rate limit exceeded" } })");

    const auto response = client.response (json);
    EXPECT_TRUE (response.failed());
    ASSERT_TRUE (response.errorMessage.has_value());
    EXPECT_EQ ("rate limit exceeded", *response.errorMessage);
}

TEST (YupAiOpenAIResponsesClient, ParseChunk_ExtractsOutputTextDelta)
{
    TestableResponsesClient client (makeOptions (LLMClient::Provider::OpenAIResponses,
                                                 "gpt-4.1",
                                                 "https://api.openai.com/v1",
                                                 "sk-key"));

    const auto json = JSON::parse (R"({
        "type": "response.output_text.delta",
        "delta": "streamed token",
        "item_id": "item_123"
    })");

    const auto response = client.chunk (json);
    ASSERT_EQ (1u, response.choices.size());
    EXPECT_EQ ("streamed token", response.choices.front().message.content);
    EXPECT_EQ (LLMMessage::Role::assistant, response.choices.front().message.role);
}

TEST (YupAiOpenAIResponsesClient, ParseChunk_IgnoresNonDeltaEvents)
{
    TestableResponsesClient client (makeOptions (LLMClient::Provider::OpenAIResponses,
                                                 "gpt-4.1",
                                                 "https://api.openai.com/v1",
                                                 "sk-key"));

    const auto json = JSON::parse (R"({ "type": "response.completed" })");

    const auto response = client.chunk (json);
    EXPECT_TRUE (response.choices.empty());
}

//==============================================================================
// LLMClientFactory

TEST (YupAiLLMClientFactory, Create_OpenAIChat_ReturnsNonNull)
{
    LLMClient::Options opts;
    opts.provider = LLMClient::Provider::OpenAIChat;
    opts.model = "gpt-4o";
    opts.baseUrl = "https://api.openai.com/v1";

    auto client = LLMClientFactory::create (opts);
    ASSERT_NE (nullptr, client);
    EXPECT_EQ (LLMClient::Provider::OpenAIChat, client->getOptions().provider);
}

TEST (YupAiLLMClientFactory, Create_Anthropic_ReturnsNonNull)
{
    LLMClient::Options opts;
    opts.provider = LLMClient::Provider::Anthropic;
    opts.model = "claude-opus-4-5";
    opts.apiKey = "ant-key";

    auto client = LLMClientFactory::create (opts);
    ASSERT_NE (nullptr, client);
    EXPECT_EQ (LLMClient::Provider::Anthropic, client->getOptions().provider);
}

TEST (YupAiLLMClientFactory, Create_Gemini_ReturnsNonNull)
{
    LLMClient::Options opts;
    opts.provider = LLMClient::Provider::Gemini;
    opts.model = "gemini-2.5-flash";
    opts.apiKey = "gemini-key";

    auto client = LLMClientFactory::create (opts);
    ASSERT_NE (nullptr, client);
    EXPECT_EQ (LLMClient::Provider::Gemini, client->getOptions().provider);
}

TEST (YupAiLLMClientFactory, Create_OpenAIResponses_ReturnsNonNull)
{
    LLMClient::Options opts;
    opts.provider = LLMClient::Provider::OpenAIResponses;
    opts.model = "gpt-4.1";
    opts.apiKey = "sk-key";

    auto client = LLMClientFactory::create (opts);
    ASSERT_NE (nullptr, client);
    EXPECT_EQ (LLMClient::Provider::OpenAIResponses, client->getOptions().provider);
}

TEST (YupAiLLMClientFactory, OpenAIChat_HelperSetsModelAndBaseUrl)
{
    auto client = LLMClientFactory::openAIChat ("gpt-4o", "https://api.openai.com/v1", "sk-key");

    ASSERT_NE (nullptr, client);
    EXPECT_EQ (LLMClient::Provider::OpenAIChat, client->getOptions().provider);
    EXPECT_EQ ("gpt-4o", client->getOptions().model);
    EXPECT_EQ ("https://api.openai.com/v1", client->getOptions().baseUrl);
    EXPECT_EQ ("sk-key", client->getOptions().apiKey);
}

TEST (YupAiLLMClientFactory, Anthropic_HelperUsesDefaultAnthropicBaseUrl)
{
    auto client = LLMClientFactory::anthropic ("claude-opus-4-5", "ant-key");

    ASSERT_NE (nullptr, client);
    EXPECT_EQ (LLMClient::Provider::Anthropic, client->getOptions().provider);
    EXPECT_EQ ("https://api.anthropic.com/v1", client->getOptions().baseUrl);
    EXPECT_EQ ("ant-key", client->getOptions().apiKey);
}

TEST (YupAiLLMClientFactory, Gemini_HelperUsesDefaultGeminiBaseUrl)
{
    auto client = LLMClientFactory::gemini ("gemini-2.5-flash", "gemini-key");

    ASSERT_NE (nullptr, client);
    EXPECT_EQ (LLMClient::Provider::Gemini, client->getOptions().provider);
    EXPECT_EQ ("https://generativelanguage.googleapis.com", client->getOptions().baseUrl);
    EXPECT_EQ ("gemini-key", client->getOptions().apiKey);
}

TEST (YupAiLLMClientFactory, OpenAIResponses_HelperUsesDefaultOpenAIBaseUrl)
{
    auto client = LLMClientFactory::openAIResponses ("gpt-4.1", "sk-key");

    ASSERT_NE (nullptr, client);
    EXPECT_EQ (LLMClient::Provider::OpenAIResponses, client->getOptions().provider);
    EXPECT_EQ ("https://api.openai.com/v1", client->getOptions().baseUrl);
    EXPECT_EQ ("sk-key", client->getOptions().apiKey);
}

//==============================================================================
// LLMSchema

TEST (YupAiLLMSchema, String_HasTypeString)
{
    const auto schema = LLMSchema::string();
    EXPECT_EQ ("string", schema["type"].toString());
}

TEST (YupAiLLMSchema, Number_HasTypeNumber)
{
    const auto schema = LLMSchema::number();
    EXPECT_EQ ("number", schema["type"].toString());
}

TEST (YupAiLLMSchema, Integer_HasTypeInteger)
{
    const auto schema = LLMSchema::integer();
    EXPECT_EQ ("integer", schema["type"].toString());
}

TEST (YupAiLLMSchema, Boolean_HasTypeBoolean)
{
    const auto schema = LLMSchema::boolean();
    EXPECT_EQ ("boolean", schema["type"].toString());
}

TEST (YupAiLLMSchema, Array_HasTypeArrayAndItems)
{
    const auto schema = LLMSchema::array (LLMSchema::string());
    EXPECT_EQ ("array", schema["type"].toString());
    EXPECT_EQ ("string", schema["items"]["type"].toString());
}

TEST (YupAiLLMSchema, Object_HasTypePropertiesAndRequiredArray)
{
    const auto schema = LLMSchema::object ({
        { "name", LLMSchema::string() },
        { "score", LLMSchema::number() },
    });

    EXPECT_EQ ("object", schema["type"].toString());
    EXPECT_EQ ("string", schema["properties"]["name"]["type"].toString());
    EXPECT_EQ ("number", schema["properties"]["score"]["type"].toString());
    ASSERT_TRUE (schema["required"].isArray());
    EXPECT_EQ (2, schema["required"].size());
}

TEST (YupAiLLMSchema, Object_DisallowsAdditionalProperties)
{
    const auto schema = LLMSchema::object ({
        { "x", LLMSchema::integer() },
    });

    EXPECT_FALSE (static_cast<bool> (schema["additionalProperties"]));
}

TEST (YupAiLLMSchema, OneOf_HasTypeStringAndEnumValues)
{
    const auto schema = LLMSchema::oneOf ({ "red", "green", "blue" });

    EXPECT_EQ ("string", schema["type"].toString());
    ASSERT_TRUE (schema["enum"].isArray());
    EXPECT_EQ (3, schema["enum"].size());
    EXPECT_EQ ("red", schema["enum"][0].toString());
    EXPECT_EQ ("green", schema["enum"][1].toString());
    EXPECT_EQ ("blue", schema["enum"][2].toString());
}

TEST (YupAiLLMSchema, ToJsonString_ProducesReparseableJson)
{
    const auto schema = LLMSchema::string();
    const auto jsonStr = LLMSchema::toJsonString (schema);
    const auto reparsed = JSON::parse (jsonStr);

    EXPECT_EQ ("string", reparsed["type"].toString());
}

TEST (YupAiLLMSchema, NestedObject_WorksInsideArray)
{
    const auto itemSchema = LLMSchema::object ({
        { "id", LLMSchema::integer() },
        { "label", LLMSchema::string() },
    });
    const auto schema = LLMSchema::array (itemSchema);

    EXPECT_EQ ("array", schema["type"].toString());
    EXPECT_EQ ("object", schema["items"]["type"].toString());
    EXPECT_EQ ("integer", schema["items"]["properties"]["id"]["type"].toString());
}
