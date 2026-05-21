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

#include <thread>

using namespace yup;

namespace
{
class TestLLMClient final : public LLMClient
{
public:
    TestLLMClient()
        : LLMClient ({})
    {
    }

    LLMResponse complete (const Request&) override
    {
        LLMResponse response;
        LLMResponse::Choice choice;
        choice.message = LLMMessage::assistant ("done");
        response.choices.push_back (choice);
        return response;
    }

    bool completeStreaming (const Request&, ChunkCallback) override
    {
        return false;
    }

    String buildBody (const Request& request, bool stream) const
    {
        return buildChatCompletionBody (request, stream);
    }
};
} // namespace

TEST (YupAiLLMMessage, SerializesAndParsesChatMessage)
{
    auto message = LLMMessage::user ("hello");
    message.name = "tester";

    auto parsed = LLMMessage::fromVar (message.toVar());

    ASSERT_TRUE (parsed.has_value());
    EXPECT_EQ (LLMMessage::Role::user, parsed->role);
    EXPECT_EQ ("hello", parsed->content);
    EXPECT_EQ ("tester", parsed->name);
}

TEST (YupAiLLMMessage, SerializesToolCalls)
{
    LLMToolCall toolCall;
    toolCall.id = "call_1";
    toolCall.name = "lookup";
    toolCall.arguments = JSON::parse (R"({"query":"abc"})");

    auto message = LLMMessage::assistant ("");
    message.toolCalls = std::vector<LLMToolCall> { toolCall };

    auto parsed = LLMMessage::fromVar (message.toVar());

    ASSERT_TRUE (parsed.has_value());
    ASSERT_TRUE (parsed->toolCalls.has_value());
    ASSERT_EQ (1u, parsed->toolCalls->size());
    EXPECT_EQ ("lookup", parsed->toolCalls->front().name);
    EXPECT_EQ ("abc", parsed->toolCalls->front().arguments["query"].toString());
}

TEST (YupAiLLMTool, GeneratesOpenAiSchema)
{
    LLMTool tool;
    tool.name = "weather";
    tool.description = "Gets weather";
    tool.parameters.push_back ({ "city", "string", "City name", true });
    tool.parameters.push_back ({ "units", "string", "Units", false, JSON::parse (R"(["metric","imperial"])") });

    auto schema = tool.toJsonSchema();

    EXPECT_EQ ("function", schema["type"].toString());
    EXPECT_EQ ("weather", schema["function"]["name"].toString());
    EXPECT_EQ ("string", schema["function"]["parameters"]["properties"]["city"]["type"].toString());
    ASSERT_TRUE (schema["function"]["parameters"]["required"].isArray());
    EXPECT_EQ ("city", schema["function"]["parameters"]["required"][0].toString());
}

TEST (YupAiLLMTool, DispatchesHandlerAndReportsMissingHandler)
{
    LLMTool tool;
    tool.name = "echo";
    tool.setHandler ([] (const var& arguments)
    {
        return arguments["value"];
    });

    EXPECT_EQ ("ok", tool.execute (JSON::parse (R"({"value":"ok"})")).toString());

    LLMTool missing;
    missing.name = "missing";
    EXPECT_TRUE (static_cast<bool> (missing.execute (var())["error"]));
}

TEST (YupAiLLMToolRegistry, RegistersFindsAndDispatchesTools)
{
    LLMToolRegistry registry;

    LLMTool tool;
    tool.name = "double";
    tool.description = "Doubles a number";
    tool.setHandler ([] (const var& arguments)
    {
        return static_cast<int> (arguments["value"]) * 2;
    });

    registry.registerTool (std::move (tool));

    EXPECT_TRUE (registry.contains ("double"));
    ASSERT_NE (nullptr, registry.findTool ("double"));
    EXPECT_EQ (42, static_cast<int> (registry.dispatchToolCall ("double", JSON::parse (R"({"value":21})"))));
    EXPECT_TRUE (registry.toToolsArray().isArray());
}

TEST (YupAiLLMToolRegistry, HandlesConcurrentRegistration)
{
    LLMToolRegistry registry;

    auto registerRange = [&registry] (int start)
    {
        for (int i = 0; i < 16; ++i)
        {
            LLMTool tool;
            tool.name = "tool_" + String (start + i);
            registry.registerTool (std::move (tool));
        }
    };

    std::thread first (registerRange, 0);
    std::thread second (registerRange, 100);
    first.join();
    second.join();

    EXPECT_EQ (32u, registry.getAllTools().size());
}

TEST (YupAiLLMResponse, ParsesOpenAiResponse)
{
    auto json = JSON::parse (R"({
        "model": "test-model",
        "choices": [
            {
                "index": 0,
                "message": { "role": "assistant", "content": "hello" },
                "finish_reason": "stop"
            }
        ],
        "usage": { "prompt_tokens": 2, "completion_tokens": 3, "total_tokens": 5 }
    })");

    auto response = LLMResponse::fromOpenAiJson (json);

    ASSERT_EQ (1u, response.choices.size());
    EXPECT_EQ ("test-model", response.model);
    EXPECT_EQ ("hello", response.choices.front().message.content);
    ASSERT_TRUE (response.usage.has_value());
    EXPECT_EQ (5, response.usage->totalTokens);
}

TEST (YupAiLLMResponse, ExtractsToolCalls)
{
    auto json = JSON::parse (R"({
        "model": "test-model",
        "choices": [
            {
                "index": 0,
                "message": {
                    "role": "assistant",
                    "content": "",
                    "tool_calls": [
                        {
                            "id": "call_1",
                            "type": "function",
                            "function": {
                                "name": "set_background_color",
                                "arguments": "{\"color\":\"darkgreen\"}"
                            }
                        }
                    ]
                },
                "finish_reason": "tool_calls"
            }
        ]
    })");

    auto response = LLMResponse::fromOpenAiJson (json);

    EXPECT_TRUE (response.hasToolCalls());

    auto toolCalls = response.getToolCalls();
    ASSERT_EQ (1u, toolCalls.size());
    EXPECT_EQ ("call_1", toolCalls.front().id);
    EXPECT_EQ ("set_background_color", toolCalls.front().name);
    EXPECT_EQ ("darkgreen", toolCalls.front().arguments["color"].toString());
}

TEST (YupAiLLMResponse, ParsesOpenAiError)
{
    auto json = JSON::parse (R"({
        "error": {
            "message": "model not found",
            "type": "invalid_request_error"
        }
    })");

    auto response = LLMResponse::fromOpenAiJson (json);

    EXPECT_TRUE (response.failed());
    ASSERT_TRUE (response.errorMessage.has_value());
    EXPECT_EQ ("model not found", *response.errorMessage);
}

TEST (YupAiLLMResponse, ParsesStreamingChunk)
{
    auto chunk = JSON::parse (R"({
        "model": "test-model",
        "choices": [
            {
                "index": 0,
                "delta": { "role": "assistant", "content": "hel" },
                "finish_reason": null
            }
        ]
    })");

    auto response = LLMResponse::fromStreamChunk (chunk);

    ASSERT_EQ (1u, response.choices.size());
    EXPECT_EQ (LLMMessage::Role::assistant, response.choices.front().message.role);
    EXPECT_EQ ("hel", response.choices.front().message.content);
}

TEST (YupAiLLMClient, BuildsChatCompletionBody)
{
    TestLLMClient client;

    LLMClient::Request request;
    request.systemPrompt = "be brief";
    request.messages.push_back (LLMMessage::user ("hello"));
    request.temperature = 0.25f;
    request.maxTokens = 32;

    auto body = JSON::parse (client.buildBody (request, true));

    EXPECT_TRUE (static_cast<bool> (body["stream"]));
    EXPECT_EQ ("system", body["messages"][0]["role"].toString());
    EXPECT_EQ ("user", body["messages"][1]["role"].toString());
    EXPECT_EQ (32, static_cast<int> (body["max_tokens"]));
}

TEST (YupAiLLMClient, SerializesSpecificToolChoice)
{
    TestLLMClient client;

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::user ("change the color"));
    request.toolChoice = "set_background_color";

    LLMTool tool;
    tool.name = "set_background_color";
    tool.description = "Changes the component background color.";
    tool.parameters.push_back ({ "color", "string", "CSS color value", true });
    request.tools.push_back (std::move (tool));

    auto body = JSON::parse (client.buildBody (request, false));

    EXPECT_EQ ("function", body["tool_choice"]["type"].toString());
    EXPECT_EQ ("set_background_color", body["tool_choice"]["function"]["name"].toString());
}

TEST (YupAiEmbeddingModel, ComputesCosineSimilarity)
{
    EmbeddingModel::Embedding a;
    a.values = { 1.0f, 0.0f };

    EmbeddingModel::Embedding b;
    b.values = { 0.0f, 1.0f };

    EmbeddingModel::Embedding c;
    c.values = { 2.0f, 0.0f };

    EXPECT_FLOAT_EQ (0.0f, EmbeddingModel::cosineSimilarity (a, b));
    EXPECT_FLOAT_EQ (1.0f, EmbeddingModel::cosineSimilarity (a, c));
}
