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

#include "yup_YupAi_bindings.h"

#define YUP_PYTHON_INCLUDE_PYBIND11_FUNCTIONAL
#define YUP_PYTHON_INCLUDE_PYBIND11_STL
#include "../utilities/yup_PyBind11Includes.h"

namespace yup::Bindings
{

namespace py = pybind11;
using namespace py::literals;

namespace
{
class PyLLMClient : public LLMClient
{
public:
    using LLMClient::LLMClient;

    LLMResponse complete (const Request& request) override
    {
        PYBIND11_OVERRIDE_PURE (LLMResponse, LLMClient, complete, request);
    }

    bool completeStreaming (const Request& request, ChunkCallback onChunk) override
    {
        PYBIND11_OVERRIDE_PURE (bool, LLMClient, completeStreaming, request, onChunk);
    }
};

String messageRepr (const LLMMessage& message)
{
    return "LLMMessage(role='" + LLMMessage::roleToString (message.role) + "', content='" + message.content + "')";
}
} // namespace

void registerYupAiBindings (py::module_& m)
{
    auto ai = m.def_submodule ("ai");

    py::enum_<LLMMessage::Role> (ai, "LLMMessageRole")
        .value ("system", LLMMessage::Role::system)
        .value ("user", LLMMessage::Role::user)
        .value ("assistant", LLMMessage::Role::assistant)
        .value ("tool", LLMMessage::Role::tool)
        .export_values();

    py::class_<LLMToolCall> (ai, "LLMToolCall")
        .def (py::init<>())
        .def_readwrite ("index", &LLMToolCall::index)
        .def_readwrite ("id", &LLMToolCall::id)
        .def_readwrite ("name", &LLMToolCall::name)
        .def_readwrite ("arguments", &LLMToolCall::arguments)
        .def ("toVar", &LLMToolCall::toVar)
        .def_static ("fromVar", [] (const var& value) -> py::object
    {
        if (auto result = LLMToolCall::fromVar (value))
            return py::cast (*result);

        return py::none();
    });

    py::class_<LLMMessage> (ai, "LLMMessage")
        .def (py::init<>())
        .def_readwrite ("role", &LLMMessage::role)
        .def_readwrite ("content", &LLMMessage::content)
        .def_readwrite ("name", &LLMMessage::name)
        .def_readwrite ("toolCalls", &LLMMessage::toolCalls)
        .def_readwrite ("toolCallId", &LLMMessage::toolCallId)
        .def_static ("system", &LLMMessage::system)
        .def_static ("user", &LLMMessage::user)
        .def_static ("assistant", &LLMMessage::assistant)
        .def_static ("toolResult", &LLMMessage::toolResult)
        .def ("toVar", &LLMMessage::toVar)
        .def_static ("fromVar", [] (const var& value) -> py::object
    {
        if (auto result = LLMMessage::fromVar (value))
            return py::cast (*result);

        return py::none();
    }).def ("__repr__", [] (const LLMMessage& message)
    {
        return messageRepr (message);
    });

    py::class_<LLMTool::Parameter> (ai, "LLMToolParameter")
        .def (py::init<>())
        .def_readwrite ("name", &LLMTool::Parameter::name)
        .def_readwrite ("type", &LLMTool::Parameter::type)
        .def_readwrite ("description", &LLMTool::Parameter::description)
        .def_readwrite ("required", &LLMTool::Parameter::required)
        .def_readwrite ("enumValues", &LLMTool::Parameter::enumValues)
        .def_readwrite ("defaultValue", &LLMTool::Parameter::defaultValue)
        .def_readwrite ("properties", &LLMTool::Parameter::properties);

    py::class_<LLMTool> (ai, "LLMTool")
        .def (py::init<>())
        .def_readwrite ("name", &LLMTool::name)
        .def_readwrite ("description", &LLMTool::description)
        .def_readwrite ("parameters", &LLMTool::parameters)
        .def ("toJsonSchema", &LLMTool::toJsonSchema)
        .def ("execute", &LLMTool::execute)
        .def ("setHandler", [] (LLMTool& self, py::function function)
    {
        self.setHandler ([function = std::move (function)] (const var& arguments) -> var
        {
            py::gil_scoped_acquire gil;
            return function (arguments).cast<var>();
        });
    });

    py::class_<LLMToolRegistry> (ai, "LLMToolRegistry")
        .def (py::init<>())
        .def ("registerTool", &LLMToolRegistry::registerTool)
        .def ("unregisterTool", &LLMToolRegistry::unregisterTool)
        .def ("contains", &LLMToolRegistry::contains)
        .def ("getAllTools", &LLMToolRegistry::getAllTools)
        .def ("toToolsArray", &LLMToolRegistry::toToolsArray)
        .def ("dispatchToolCall", &LLMToolRegistry::dispatchToolCall)
        .def ("register", [] (LLMToolRegistry& self, const String& name, const String& description, py::function function)
    {
        LLMTool tool;
        tool.name = name;
        tool.description = description;
        tool.setHandler ([function = std::move (function)] (const var& arguments) -> var
        {
            py::gil_scoped_acquire gil;
            return function (arguments).cast<var>();
        });

        self.registerTool (std::move (tool));
    });

    py::class_<LLMResponse::Choice> (ai, "LLMResponseChoice")
        .def (py::init<>())
        .def_readwrite ("index", &LLMResponse::Choice::index)
        .def_readwrite ("message", &LLMResponse::Choice::message)
        .def_readwrite ("finishReason", &LLMResponse::Choice::finishReason);

    py::class_<LLMResponse::Usage> (ai, "LLMResponseUsage")
        .def (py::init<>())
        .def_readwrite ("promptTokens", &LLMResponse::Usage::promptTokens)
        .def_readwrite ("completionTokens", &LLMResponse::Usage::completionTokens)
        .def_readwrite ("totalTokens", &LLMResponse::Usage::totalTokens);

    py::class_<LLMResponse> (ai, "LLMResponse")
        .def (py::init<>())
        .def_readwrite ("choices", &LLMResponse::choices)
        .def_readwrite ("usage", &LLMResponse::usage)
        .def_readwrite ("model", &LLMResponse::model)
        .def_readwrite ("errorMessage", &LLMResponse::errorMessage)
        .def ("hasToolCalls", &LLMResponse::hasToolCalls)
        .def ("failed", &LLMResponse::failed)
        .def ("getToolCalls", &LLMResponse::getToolCalls)
        .def_static ("fromError", &LLMResponse::fromError)
        .def_static ("fromOpenAiJson", &LLMResponse::fromOpenAiJson)
        .def_static ("fromStreamChunk", &LLMResponse::fromStreamChunk);

    py::class_<LLMClient::Request> (ai, "LLMRequest")
        .def (py::init<>())
        .def_readwrite ("messages", &LLMClient::Request::messages)
        .def_readwrite ("systemPrompt", &LLMClient::Request::systemPrompt)
        .def_readwrite ("tools", &LLMClient::Request::tools)
        .def_readwrite ("toolChoice", &LLMClient::Request::toolChoice)
        .def_readwrite ("temperature", &LLMClient::Request::temperature)
        .def_readwrite ("topP", &LLMClient::Request::topP)
        .def_readwrite ("maxTokens", &LLMClient::Request::maxTokens)
        .def_readwrite ("stopSequences", &LLMClient::Request::stopSequences);

    py::class_<LLMClient::Options> (ai, "LLMOptions")
        .def (py::init<>())
        .def_readwrite ("model", &LLMClient::Options::model)
        .def_readwrite ("baseUrl", &LLMClient::Options::baseUrl)
        .def_readwrite ("apiKey", &LLMClient::Options::apiKey)
        .def_readwrite ("timeoutMs", &LLMClient::Options::timeoutMs)
        .def_readwrite ("maxRetries", &LLMClient::Options::maxRetries);

    py::class_<LLMClient, PyLLMClient> (ai, "LLMClient")
        .def (py::init<LLMClient::Options>())
        .def ("complete", &LLMClient::complete)
        .def ("completeStreaming", &LLMClient::completeStreaming)
        .def ("chat", &LLMClient::chat)
        .def ("chatWithTools", &LLMClient::chatWithTools)
        .def ("runToolLoop", &LLMClient::runToolLoop)
        .def ("getOptions", &LLMClient::getOptions, py::return_value_policy::reference_internal);

    py::class_<LLMHttpClient, LLMClient> (ai, "LLMHttpClient")
        .def (py::init<LLMClient::Options>());

    py::class_<EmbeddingModel::Options> (ai, "EmbeddingOptions")
        .def (py::init<>())
        .def_readwrite ("model", &EmbeddingModel::Options::model)
        .def_readwrite ("baseUrl", &EmbeddingModel::Options::baseUrl)
        .def_readwrite ("apiKey", &EmbeddingModel::Options::apiKey)
        .def_readwrite ("timeoutMs", &EmbeddingModel::Options::timeoutMs);

    py::class_<EmbeddingModel::Embedding> (ai, "Embedding")
        .def (py::init<>())
        .def_readwrite ("values", &EmbeddingModel::Embedding::values)
        .def_readwrite ("index", &EmbeddingModel::Embedding::index)
        .def ("dimensions", &EmbeddingModel::Embedding::dimensions);

    py::class_<EmbeddingModel> (ai, "EmbeddingModel")
        .def (py::init<EmbeddingModel::Options>())
        .def ("embed", &EmbeddingModel::embed)
        .def ("embedBatch", &EmbeddingModel::embedBatch)
        .def_static ("cosineSimilarity", &EmbeddingModel::cosineSimilarity);
}

} // namespace yup::Bindings
