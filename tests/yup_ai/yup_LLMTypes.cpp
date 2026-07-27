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

    explicit TestLLMClient (Options optionsToUse)
        : LLMClient (std::move (optionsToUse))
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

TEST (LLMMessage, SerializesAndParsesChatMessage)
{
    auto message = LLMMessage::user ("hello");
    message.name = "tester";

    auto parsed = LLMMessage::fromVar (message.toVar());

    ASSERT_TRUE (parsed.has_value());
    EXPECT_EQ (LLMMessage::Role::user, parsed->role);
    EXPECT_EQ ("hello", parsed->content);
    EXPECT_EQ ("tester", parsed->name);
}

TEST (LLMMessage, SerializesToolCalls)
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

TEST (LLMMessage, CreatesToolResultMessage)
{
    auto message = LLMMessage::toolResult ("call_42", "done");

    EXPECT_EQ (LLMMessage::Role::tool, message.role);
    EXPECT_EQ ("done", message.content);
    ASSERT_TRUE (message.toolCallId.has_value());
    EXPECT_EQ ("call_42", *message.toolCallId);
}

TEST (LLMMessage, SerializesToolResultMessage)
{
    auto message = LLMMessage::toolResult ("call_42", "done");

    auto parsed = LLMMessage::fromVar (message.toVar());

    ASSERT_TRUE (parsed.has_value());
    EXPECT_EQ (LLMMessage::Role::tool, parsed->role);
    EXPECT_EQ ("done", parsed->content);
    ASSERT_TRUE (parsed->toolCallId.has_value());
    EXPECT_EQ ("call_42", *parsed->toolCallId);
}

TEST (LLMMessage, ParsesFlatToolCall)
{
    auto message = LLMMessage::fromVar (JSON::parse (R"({
        "role": "assistant",
        "content": "",
        "tool_calls": [
            {
                "id": "call_flat",
                "type": "function",
                "name": "lookup",
                "arguments": { "query": "abc" }
            }
        ]
    })"));

    ASSERT_TRUE (message.has_value());
    ASSERT_TRUE (message->toolCalls.has_value());
    ASSERT_EQ (1u, message->toolCalls->size());
    EXPECT_EQ ("lookup", message->toolCalls->front().name);
}

TEST (LLMMessage, FromVarRejectsNonObject)
{
    EXPECT_FALSE (LLMMessage::fromVar (var (42)).has_value());
    EXPECT_FALSE (LLMMessage::fromVar (var ("string")).has_value());
}

TEST (LLMMessage, FromVarRejectsUnknownRole)
{
    EXPECT_FALSE (LLMMessage::fromVar (JSON::parse (R"({
        "role": "bogus",
        "content": "hello"
    })"))
                      .has_value());
}

TEST (LLMMessage, RoleFromStringReturnsNulloptForUnknown)
{
    EXPECT_FALSE (LLMMessage::roleFromString ("unknown").has_value());
    EXPECT_FALSE (LLMMessage::roleFromString ("").has_value());
}

TEST (LLMToolCall, FromVarRejectsNonObject)
{
    EXPECT_FALSE (LLMToolCall::fromVar (var ("not an object")).has_value());
}

TEST (LLMTool, GeneratesOpenAiSchema)
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

TEST (LLMTool, DispatchesHandlerAndReportsMissingHandler)
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

TEST (LLMTool, GeneratesSchemaWithDefaultValue)
{
    LLMTool tool;
    tool.name = "counter";
    tool.description = "Counts items";
    tool.parameters.push_back ({ "count", "integer", "Number of items", true, std::nullopt, var (10) });

    auto schema = tool.toJsonSchema();

    EXPECT_EQ (10, static_cast<int> (schema["function"]["parameters"]["properties"]["count"]["default"]));
}

TEST (LLMTool, GeneratesSchemaWithNestedProperties)
{
    LLMTool tool;
    tool.name = "set_color";
    tool.description = "Sets color";

    LLMTool::Parameter colorParam;
    colorParam.name = "color";
    colorParam.type = "object";
    colorParam.description = "Color object";
    colorParam.properties = std::vector<LLMTool::Parameter> {
        { "r", "integer", "Red channel", true },
        { "g", "integer", "Green channel", true },
        { "b", "integer", "Blue channel", true },
        { "a", "number", "Alpha channel", false }
    };

    tool.parameters.push_back (colorParam);

    auto schema = tool.toJsonSchema();

    auto& colorSchema = schema["function"]["parameters"]["properties"]["color"];
    EXPECT_EQ ("object", colorSchema["type"].toString());

    auto& nestedProps = colorSchema["properties"];
    EXPECT_EQ ("integer", nestedProps["r"]["type"].toString());
    EXPECT_EQ ("integer", nestedProps["g"]["type"].toString());
    EXPECT_EQ ("integer", nestedProps["b"]["type"].toString());
    EXPECT_EQ ("number", nestedProps["a"]["type"].toString());

    auto& required = colorSchema["required"];
    EXPECT_TRUE (required.isArray());
    EXPECT_EQ (3, required.size());
    EXPECT_EQ ("r", required[0].toString());
    EXPECT_EQ ("g", required[1].toString());
    EXPECT_EQ ("b", required[2].toString());
}

TEST (LLMToolRegistry, UnregistersAndFindsTools)
{
    LLMToolRegistry registry;

    LLMTool tool;
    tool.name = "double";
    tool.setHandler ([] (const var& arguments)
    {
        return static_cast<int> (arguments["value"]) * 2;
    });
    registry.registerTool (std::move (tool));

    EXPECT_TRUE (registry.contains ("double"));

    registry.unregisterTool ("double");

    EXPECT_FALSE (registry.contains ("double"));
    EXPECT_EQ (nullptr, registry.findTool ("double"));
}

TEST (LLMToolRegistry, DispatchUnknownToolReturnsError)
{
    LLMToolRegistry registry;

    auto result = registry.dispatchToolCall ("nonexistent", var());

    ASSERT_TRUE (result.isObject());
    EXPECT_TRUE (static_cast<bool> (result["error"]));
    EXPECT_TRUE (result["message"].toString().contains ("Unknown tool"));
}

TEST (LLMToolRegistry, FindToolReturnsNullForMissing)
{
    LLMToolRegistry registry;

    EXPECT_EQ (nullptr, registry.findTool ("missing"));
}

TEST (LLMToolRegistry, RegistersFindsAndDispatchesTools)
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

TEST (LLMToolRegistry, HandlesConcurrentRegistration)
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

TEST (LLMResponse, ParsesOpenAiResponse)
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

TEST (LLMResponse, ExtractsToolCalls)
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

TEST (LLMResponse, ParsesOpenAiError)
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

TEST (LLMResponse, ParsesStreamingChunk)
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

TEST (LLMResponse, AccumulatesStreamingToolCallArguments)
{
    auto first = LLMResponse::fromStreamChunk (JSON::parse (R"({
        "model": "test-model",
        "choices": [
            {
                "index": 0,
                "delta": {
                    "role": "assistant",
                    "tool_calls": [
                        {
                            "index": 0,
                            "id": "call_1",
                            "type": "function",
                            "function": {
                                "name": "set_background_color",
                                "arguments": "{\"color\""
                            }
                        }
                    ]
                },
                "finish_reason": null
            }
        ]
    })"));

    auto second = LLMResponse::fromStreamChunk (JSON::parse (R"({
        "model": "test-model",
        "choices": [
            {
                "index": 0,
                "delta": {
                    "tool_calls": [
                        {
                            "index": 0,
                            "function": {
                                "arguments": ":\"darkgreen\"}"
                            }
                        }
                    ]
                },
                "finish_reason": "tool_calls"
            }
        ]
    })"));

    LLMResponse accumulated;
    accumulated.appendStreamChunk (first);
    accumulated.appendStreamChunk (second);

    ASSERT_TRUE (accumulated.hasToolCalls());

    auto toolCalls = accumulated.getToolCalls();
    ASSERT_EQ (1u, toolCalls.size());
    EXPECT_EQ ("call_1", toolCalls.front().id);
    EXPECT_EQ ("set_background_color", toolCalls.front().name);
    EXPECT_EQ ("darkgreen", toolCalls.front().arguments["color"].toString());
}

TEST (LLMResponse, ParsesOpenAiErrorAsString)
{
    auto json = JSON::parse (R"({
        "error": "unauthorized"
    })");

    auto response = LLMResponse::fromOpenAiJson (json);

    EXPECT_TRUE (response.failed());
    ASSERT_TRUE (response.errorMessage.has_value());
    EXPECT_EQ ("unauthorized", *response.errorMessage);
}

TEST (LLMResponse, ParsesOpenAiErrorObjectWithoutMessage)
{
    auto json = JSON::parse (R"({
        "error": {
            "type": "invalid_request_error"
        }
    })");

    auto response = LLMResponse::fromOpenAiJson (json);

    EXPECT_FALSE (response.failed());
    EXPECT_FALSE (response.errorMessage.has_value());
}

TEST (LLMResponse, ParsesVoidJsonReturnsError)
{
    auto response = LLMResponse::fromOpenAiJson (var());

    EXPECT_TRUE (response.failed());
    ASSERT_TRUE (response.errorMessage.has_value());
    EXPECT_TRUE (response.errorMessage->containsIgnoreCase ("Unable to parse"));
}

TEST (LLMResponse, ParsesVoidStreamChunkReturnsError)
{
    auto response = LLMResponse::fromStreamChunk (var());

    EXPECT_TRUE (response.failed());
    ASSERT_TRUE (response.errorMessage.has_value());
    EXPECT_TRUE (response.errorMessage->containsIgnoreCase ("Unable to parse"));
}

TEST (LLMResponse, AppendsStreamChunkWithError)
{
    auto errorChunk = LLMResponse::fromError ("stream error");

    LLMResponse accumulated;
    accumulated.model = "test-model";
    accumulated.appendStreamChunk (errorChunk);

    EXPECT_TRUE (accumulated.failed());
    ASSERT_TRUE (accumulated.errorMessage.has_value());
    EXPECT_EQ ("stream error", *accumulated.errorMessage);
}

TEST (LLMResponse, AppendsStreamChunkNonAssistantRoleRetained)
{
    auto first = LLMResponse::fromStreamChunk (JSON::parse (R"({
        "model": "test-model",
        "choices": [
            {
                "index": 0,
                "delta": { "role": "system", "content": "system message" },
                "finish_reason": null
            }
        ]
    })"));

    auto second = LLMResponse::fromStreamChunk (JSON::parse (R"({
        "model": "test-model",
        "choices": [
            {
                "index": 0,
                "delta": { "content": " more text" },
                "finish_reason": "stop"
            }
        ]
    })"));

    LLMResponse accumulated;
    accumulated.appendStreamChunk (first);
    accumulated.appendStreamChunk (second);

    ASSERT_EQ (1u, accumulated.choices.size());
    EXPECT_EQ (LLMMessage::Role::system, accumulated.choices.front().message.role);
    EXPECT_EQ ("system message more text", accumulated.choices.front().message.content);
}

TEST (LLMResponse, AppendsStreamChunkNegativeToolCallIndexSkipped)
{
    auto chunk = LLMResponse::fromStreamChunk (JSON::parse (R"({
        "model": "test-model",
        "choices": [
            {
                "index": 0,
                "delta": {
                    "role": "assistant",
                    "tool_calls": [
                        {
                            "index": -1,
                            "id": "call_bad",
                            "type": "function",
                            "function": { "name": "bad_tool", "arguments": "{}" }
                        }
                    ]
                },
                "finish_reason": null
            }
        ]
    })"));

    LLMResponse accumulated;
    accumulated.appendStreamChunk (chunk);

    EXPECT_FALSE (accumulated.hasToolCalls());
}

TEST (LLMResponse, AppendsStreamChunkEmptyMergedArguments)
{
    auto chunk = LLMResponse::fromStreamChunk (JSON::parse (R"({
        "model": "test-model",
        "choices": [
            {
                "index": 0,
                "delta": {
                    "role": "assistant",
                    "tool_calls": [
                        {
                            "index": 0,
                            "id": "call_1",
                            "type": "function",
                            "function": { "name": "lookup", "arguments": "" }
                        }
                    ]
                },
                "finish_reason": null
            }
        ]
    })"));

    LLMResponse accumulated;
    accumulated.appendStreamChunk (chunk);

    ASSERT_TRUE (accumulated.hasToolCalls());
    auto toolCalls = accumulated.getToolCalls();
    ASSERT_EQ (1u, toolCalls.size());
    EXPECT_EQ ("lookup", toolCalls.front().name);
    EXPECT_EQ ("", toolCalls.front().arguments.toString());
}

TEST (LLMResponse, FromErrorWithEmptyString)
{
    auto response = LLMResponse::fromError ("");

    EXPECT_TRUE (response.failed());
    ASSERT_TRUE (response.errorMessage.has_value());
    EXPECT_EQ ("Unknown AI response error", *response.errorMessage);
}

TEST (LLMResponse, ParsesErrorAsStringInStreamChunk)
{
    auto chunk = JSON::parse (R"({
        "error": "rate limit exceeded"
    })");

    auto response = LLMResponse::fromStreamChunk (chunk);

    EXPECT_TRUE (response.failed());
    ASSERT_TRUE (response.errorMessage.has_value());
    EXPECT_EQ ("rate limit exceeded", *response.errorMessage);
}

TEST (LLMClient, BuildsChatCompletionBody)
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
    EXPECT_EQ (32, static_cast<int> (body["max_completion_tokens"]));
}

TEST (LLMClient, SerializesSpecificToolChoice)
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

TEST (LLMClient, BuildsChatCompletionBodyWithAllOptions)
{
    TestLLMClient client;

    LLMClient::Request request;
    request.systemPrompt = "be brief";
    request.messages.push_back (LLMMessage::user ("hello"));
    request.temperature = 0.5f;
    request.topP = 0.9f;
    request.maxTokens = 64;
    request.toolChoice = "none";
    request.stopSequences = std::vector<String> { "\n\n", "END" };

    auto body = JSON::parse (client.buildBody (request, false));

    EXPECT_NEAR (0.9, static_cast<double> (body["top_p"]), 1e-5);
    EXPECT_EQ ("none", body["tool_choice"].toString());
    ASSERT_TRUE (body["stop"].isArray());
    EXPECT_EQ ("\n\n", body["stop"][0].toString());
    EXPECT_EQ ("END", body["stop"][1].toString());
}

TEST (LLMClient, BuildsChatCompletionBodyWithGrammarAndCache)
{
    LLMClient::Options opts;
    opts.grammar = "root ::= ...";
    opts.userAgent = "my-app";
    opts.reasoningEffort = "high";
    TestLLMClient client (opts);

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::user ("hello"));

    auto body = JSON::parse (client.buildBody (request, false));

    EXPECT_EQ ("high", body["reasoning_effort"].toString());
    EXPECT_EQ ("root ::= ...", body["grammar"].toString());
    EXPECT_EQ ("my-app", body["prompt_cache_key"].toString());
    EXPECT_EQ ("24h", body["prompt_cache_retention"].toString());
}

TEST (LLMClient, BuildsChatCompletionBodyWithPerRequestGrammar)
{
    LLMClient::Options opts;
    opts.grammar = "default_grammar";
    TestLLMClient client (opts);

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::user ("hello"));
    request.grammar = "per_request_grammar";

    auto body = JSON::parse (client.buildBody (request, false));

    EXPECT_EQ ("per_request_grammar", body["grammar"].toString());
}

TEST (LLMClient, BuildsChatCompletionBodyWithJsonSchema)
{
    TestLLMClient client;

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::user ("hello"));
    request.schema = JSON::parse (R"({"type":"object","properties":{"name":{"type":"string"}}})");

    auto body = JSON::parse (client.buildBody (request, false));

    EXPECT_EQ ("json_schema", body["response_format"]["type"].toString());
    EXPECT_EQ ("response", body["response_format"]["json_schema"]["name"].toString());
    EXPECT_TRUE (static_cast<bool> (body["response_format"]["json_schema"]["strict"]));
}

TEST (LLMClient, BuildsChatCompletionBodyWithModel)
{
    LLMClient::Options opts;
    opts.model = "gpt-4";
    TestLLMClient client (opts);

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::user ("hello"));

    auto body = JSON::parse (client.buildBody (request, false));

    EXPECT_EQ ("gpt-4", body["model"].toString());
}

TEST (LLMClient, BuildsChatCompletionBodyWithNoTemperature)
{
    LLMClient::Options opts;
    opts.noTemperature = true;
    TestLLMClient client (opts);

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::user ("hello"));
    request.temperature = 0.5f;

    auto body = JSON::parse (client.buildBody (request, false));

    EXPECT_FALSE (body.hasProperty ("temperature"));
}

TEST (LLMClient, ChatMethodCreatesRequestAndReturnsResponse)
{
    TestLLMClient client;

    auto response = client.chat ("hello");

    ASSERT_EQ (1u, response.choices.size());
    EXPECT_EQ ("done", response.choices.front().message.content);
}

TEST (LLMClient, ChatWithToolsCreatesRequestWithAllTools)
{
    TestLLMClient client;

    LLMToolRegistry registry;
    LLMTool tool;
    tool.name = "echo";
    tool.description = "Echoes";
    registry.registerTool (std::move (tool));

    auto response = client.chatWithTools ("hello", registry);

    ASSERT_EQ (1u, response.choices.size());
    EXPECT_EQ ("done", response.choices.front().message.content);
}

TEST (LLMClient, RunToolLoopCompletesWithoutToolCalls)
{
    TestLLMClient client;

    LLMClient::Request request;
    request.messages.push_back (LLMMessage::user ("hello"));
    LLMToolRegistry registry;

    auto response = client.runToolLoop (request, registry);

    ASSERT_EQ (1u, response.choices.size());
    EXPECT_EQ ("done", response.choices.front().message.content);
}

TEST (LLMClient, GetOptionsReturnsReference)
{
    TestLLMClient client;

    EXPECT_TRUE (client.getOptions().model.isEmpty());
}

TEST (EmbeddingModel, ComputesCosineSimilarity)
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

TEST (EmbeddingModel, CosineSimilarity_EmptyVectors_ReturnsZero)
{
    EmbeddingModel::Embedding a;
    EmbeddingModel::Embedding b;

    EXPECT_FLOAT_EQ (0.0f, EmbeddingModel::cosineSimilarity (a, b));
}

TEST (EmbeddingModel, CosineSimilarity_OneEmptyVector_ReturnsZero)
{
    EmbeddingModel::Embedding a;
    a.values = { 1.0f, 2.0f, 3.0f };
    EmbeddingModel::Embedding b;

    EXPECT_FLOAT_EQ (0.0f, EmbeddingModel::cosineSimilarity (a, b));
}

TEST (EmbeddingModel, CosineSimilarity_DifferentSizes_UsesMinimum)
{
    EmbeddingModel::Embedding a;
    a.values = { 1.0f, 0.0f, 0.0f };

    EmbeddingModel::Embedding b;
    b.values = { 0.0f, 1.0f };

    EXPECT_FLOAT_EQ (0.0f, EmbeddingModel::cosineSimilarity (a, b));
}

TEST (EmbeddingModel, CosineSimilarity_NegativeCorrelation)
{
    EmbeddingModel::Embedding a;
    a.values = { 1.0f, 0.0f };

    EmbeddingModel::Embedding b;
    b.values = { -1.0f, 0.0f };

    EXPECT_FLOAT_EQ (-1.0f, EmbeddingModel::cosineSimilarity (a, b));
}

TEST (EmbeddingModel, CosineSimilarity_ZeroMagnitude_ReturnsZero)
{
    EmbeddingModel::Embedding a;
    a.values = { 0.0f, 0.0f };

    EmbeddingModel::Embedding b;
    b.values = { 1.0f, 0.0f };

    EXPECT_FLOAT_EQ (0.0f, EmbeddingModel::cosineSimilarity (a, b));
}

TEST (EmbeddingModel, Embedding_HasDimensions)
{
    EmbeddingModel::Embedding e;
    e.values = { 1.0f, 2.0f, 3.0f, 4.0f };
    e.index = 5;

    EXPECT_EQ (5, e.index);
    EXPECT_EQ (4, e.dimensions());
}

TEST (EmbeddingModel, Embedding_EmptyHasZeroDimensions)
{
    EmbeddingModel::Embedding e;

    EXPECT_EQ (0, e.dimensions());
}
