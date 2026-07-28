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

class PyMCPTransport : public MCPTransport
{
public:
    Result sendMessage (const var& message) override
    {
        py::gil_scoped_acquire gil;
        auto method = py::get_override (this, "sendMessage");

        if (! method)
            py::pybind11_fail ("Tried to call pure virtual function \"MCPTransport.sendMessage\"");

        auto result = method (message);

        if (py::isinstance<Result> (result))
            return result.cast<Result>();

        if (result.is_none() || result.cast<bool>())
            return Result::ok();

        return Result::fail ("Python MCPTransport.sendMessage returned false");
    }

    ResultValue<var> receiveMessage (int timeoutMs = -1) override
    {
        py::gil_scoped_acquire gil;
        auto method = py::get_override (this, "receiveMessage");

        if (! method)
            py::pybind11_fail ("Tried to call pure virtual function \"MCPTransport.receiveMessage\"");

        auto result = method (timeoutMs);
        if (result.is_none())
            return makeResultValueFail ("Python MCPTransport.receiveMessage returned None");

        return makeResultValueOk (result.cast<var>());
    }

    void setMessageHandler (MessageHandler handler) override
    {
        py::gil_scoped_acquire gil;
        auto method = py::get_override (this, "setMessageHandler");

        if (! method)
            py::pybind11_fail ("Tried to call pure virtual function \"MCPTransport.setMessageHandler\"");

        method (std::move (handler));
    }

    Result start() override
    {
        py::gil_scoped_acquire gil;
        auto method = py::get_override (this, "start");

        if (! method)
            py::pybind11_fail ("Tried to call pure virtual function \"MCPTransport.start\"");

        auto result = method();

        if (py::isinstance<Result> (result))
            return result.cast<Result>();

        if (result.is_none() || result.cast<bool>())
            return Result::ok();

        return Result::fail ("Python MCPTransport.start returned false");
    }

    void stop() override
    {
        py::gil_scoped_acquire gil;
        auto method = py::get_override (this, "stop");

        if (! method)
            py::pybind11_fail ("Tried to call pure virtual function \"MCPTransport.stop\"");

        method();
    }

    bool isConnected() const noexcept override
    {
        py::gil_scoped_acquire gil;
        auto method = py::get_override (this, "isConnected");

        if (! method)
            return false;

        try
        {
            return method().cast<bool>();
        }
        catch (...)
        {
            return false;
        }
    }
};

String messageRepr (const LLMMessage& message)
{
    return "LLMMessage(role='" + LLMMessage::roleToString (message.role) + "', content='" + message.content + "')";
}

py::object optionalJsonRpcRequestToPython (const std::optional<JsonRpcRequest>& value)
{
    return value.has_value() ? py::cast (*value) : py::none();
}

py::object optionalJsonRpcResponseToPython (const std::optional<JsonRpcResponse>& value)
{
    return value.has_value() ? py::cast (*value) : py::none();
}

py::object optionalJsonRpcErrorToPython (const std::optional<JsonRpcError>& value)
{
    return value.has_value() ? py::cast (*value) : py::none();
}

py::object optionalMCPToolDefinitionToPython (const std::optional<MCPToolDefinition>& value)
{
    return value.has_value() ? py::cast (*value) : py::none();
}

py::object optionalMCPResourceDefinitionToPython (const std::optional<MCPResourceDefinition>& value)
{
    return value.has_value() ? py::cast (*value) : py::none();
}
} // namespace

void registerYupAiBindings (py::module_& m)
{
    auto ai = m.def_submodule ("ai");

    ai.attr ("MCP_PARSE_ERROR") = MCPErrorCodes::parseError;
    ai.attr ("MCP_INVALID_REQUEST") = MCPErrorCodes::invalidRequest;
    ai.attr ("MCP_METHOD_NOT_FOUND") = MCPErrorCodes::methodNotFound;
    ai.attr ("MCP_INVALID_PARAMS") = MCPErrorCodes::invalidParams;
    ai.attr ("MCP_INTERNAL_ERROR") = MCPErrorCodes::internalError;

    py::enum_<LLMClient::Provider> (ai, "LLMProvider")
        .value ("OpenAIChat", LLMClient::Provider::OpenAIChat)
        .value ("OpenAIResponses", LLMClient::Provider::OpenAIResponses)
        .value ("Anthropic", LLMClient::Provider::Anthropic)
        .value ("Gemini", LLMClient::Provider::Gemini)
        .export_values();

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
        .def ("appendStreamChunk", &LLMResponse::appendStreamChunk, "chunk"_a)
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
        .def_readwrite ("stopSequences", &LLMClient::Request::stopSequences)
        .def_readwrite ("schema", &LLMClient::Request::schema)
        .def_readwrite ("grammar", &LLMClient::Request::grammar)
        .def_readwrite ("grammarToolName", &LLMClient::Request::grammarToolName)
        .def_readwrite ("grammarToolDescription", &LLMClient::Request::grammarToolDescription);

    py::class_<LLMClient::Options> (ai, "LLMOptions")
        .def (py::init<>())
        .def_readwrite ("provider", &LLMClient::Options::provider)
        .def_readwrite ("model", &LLMClient::Options::model)
        .def_readwrite ("baseUrl", &LLMClient::Options::baseUrl)
        .def_readwrite ("apiKey", &LLMClient::Options::apiKey)
        .def_readwrite ("timeoutMs", &LLMClient::Options::timeoutMs)
        .def_readwrite ("maxRetries", &LLMClient::Options::maxRetries)
        .def_readwrite ("maxTokens", &LLMClient::Options::maxTokens)
        .def_readwrite ("reasoningEffort", &LLMClient::Options::reasoningEffort)
        .def_readwrite ("grammar", &LLMClient::Options::grammar)
        .def_readwrite ("noTemperature", &LLMClient::Options::noTemperature)
        .def_readwrite ("userAgent", &LLMClient::Options::userAgent)
        .def_readwrite ("appUrl", &LLMClient::Options::appUrl);

    py::class_<LLMClient, PyLLMClient> (ai, "LLMClient")
        .def (py::init<LLMClient::Options>())
        .def ("complete", &LLMClient::complete)
        .def ("completeStreaming", &LLMClient::completeStreaming)
        .def ("chat", &LLMClient::chat)
        .def ("chatWithTools", &LLMClient::chatWithTools)
        .def ("runToolLoop", &LLMClient::runToolLoop)
        .def ("getOptions", &LLMClient::getOptions, py::return_value_policy::reference_internal);

    // LLMHttpClient is an abstract base — not directly constructible from Python.
    // Use LLMClientFactory to create provider-specific clients.
    py::class_<LLMHttpClient, LLMClient> (ai, "LLMHttpClient");

    py::class_<LLMClientFactory> (ai, "LLMClientFactory")
        .def_static ("create", &LLMClientFactory::create, "options"_a)
        .def_static ("openAIChat",
                     &LLMClientFactory::openAIChat,
                     "model"_a,
                     "baseUrl"_a = String ("http://localhost:11434/v1"),
                     "apiKey"_a = String {})
        .def_static ("openAIResponses",
                     &LLMClientFactory::openAIResponses,
                     "model"_a,
                     "apiKey"_a,
                     "baseUrl"_a = String ("https://api.openai.com/v1"))
        .def_static ("anthropic",
                     &LLMClientFactory::anthropic,
                     "model"_a,
                     "apiKey"_a,
                     "baseUrl"_a = String ("https://api.anthropic.com/v1"))
        .def_static ("gemini",
                     &LLMClientFactory::gemini,
                     "model"_a,
                     "apiKey"_a,
                     "baseUrl"_a = String ("https://generativelanguage.googleapis.com"));

    py::class_<LLMSchema> (ai, "LLMSchema")
        .def_static ("string", &LLMSchema::string)
        .def_static ("number", &LLMSchema::number)
        .def_static ("integer", &LLMSchema::integer)
        .def_static ("boolean", &LLMSchema::boolean)
        .def_static ("array", &LLMSchema::array, "itemSchema"_a)
        .def_static ("object", [] (const std::vector<std::pair<String, var>>& fields)
    {
        // Convert from Python list-of-tuples to the initializer_list-based helper.
        // We replicate the helper logic to avoid the initializer_list limitation.
        auto properties = var (std::make_unique<DynamicObject>());
        var requiredArray;

        for (const auto& [name, fieldSchema] : fields)
        {
            if (auto* obj = properties.getDynamicObject())
                obj->setProperty (name, fieldSchema);
            requiredArray.append (name);
        }

        auto result = var (std::make_unique<DynamicObject>());
        auto* obj = result.getDynamicObject();
        obj->setProperty ("type", String ("object"));
        obj->setProperty ("properties", properties);
        obj->setProperty ("required", requiredArray);
        obj->setProperty ("additionalProperties", false);
        return result;
    },
                     "fields"_a)
        .def_static ("oneOf", [] (const std::vector<String>& values)
    {
        var enumArray;

        for (const auto& v : values)
            enumArray.append (v);

        auto result = var (std::make_unique<DynamicObject>());
        auto* obj = result.getDynamicObject();
        obj->setProperty ("type", String ("string"));
        obj->setProperty ("enum", enumArray);
        return result;
    },
                     "values"_a)
        .def_static ("toJsonString", &LLMSchema::toJsonString, "schema"_a);

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

    py::class_<JsonRpcError> (ai, "JsonRpcError")
        .def (py::init<>())
        .def_readwrite ("code", &JsonRpcError::code)
        .def_readwrite ("message", &JsonRpcError::message)
        .def_readwrite ("data", &JsonRpcError::data)
        .def ("toVar", &JsonRpcError::toVar)
        .def_static ("fromVar", [] (const var& value)
    {
        return optionalJsonRpcErrorToPython (JsonRpcError::fromVar (value));
    });

    py::class_<JsonRpcRequest> (ai, "JsonRpcRequest")
        .def (py::init<>())
        .def_readwrite ("jsonrpc", &JsonRpcRequest::jsonrpc)
        .def_readwrite ("id", &JsonRpcRequest::id)
        .def_readwrite ("method", &JsonRpcRequest::method)
        .def_readwrite ("params", &JsonRpcRequest::params)
        .def ("isNotification", &JsonRpcRequest::isNotification)
        .def ("toVar", &JsonRpcRequest::toVar)
        .def_static ("fromVar", [] (const var& value)
    {
        return optionalJsonRpcRequestToPython (JsonRpcRequest::fromVar (value));
    });

    py::class_<JsonRpcResponse> (ai, "JsonRpcResponse")
        .def (py::init<>())
        .def_readwrite ("jsonrpc", &JsonRpcResponse::jsonrpc)
        .def_readwrite ("id", &JsonRpcResponse::id)
        .def_readwrite ("result", &JsonRpcResponse::result)
        .def_readwrite ("error", &JsonRpcResponse::error)
        .def ("isError", &JsonRpcResponse::isError)
        .def ("toVar", &JsonRpcResponse::toVar)
        .def_static ("fromVar", [] (const var& value)
    {
        return optionalJsonRpcResponseToPython (JsonRpcResponse::fromVar (value));
    });

    py::class_<MCPCapabilities> (ai, "MCPCapabilities")
        .def (py::init<>())
        .def_readwrite ("supportsTools", &MCPCapabilities::supportsTools)
        .def_readwrite ("supportsResources", &MCPCapabilities::supportsResources)
        .def_readwrite ("supportsPrompts", &MCPCapabilities::supportsPrompts)
        .def_readwrite ("supportsLogging", &MCPCapabilities::supportsLogging)
        .def ("toVar", &MCPCapabilities::toVar)
        .def_static ("fromVar", &MCPCapabilities::fromVar);

    py::class_<MCPToolDefinition> (ai, "MCPToolDefinition")
        .def (py::init<>())
        .def_readwrite ("name", &MCPToolDefinition::name)
        .def_readwrite ("description", &MCPToolDefinition::description)
        .def_readwrite ("inputSchema", &MCPToolDefinition::inputSchema)
        .def ("toVar", &MCPToolDefinition::toVar)
        .def_static ("fromVar", [] (const var& value)
    {
        return optionalMCPToolDefinitionToPython (MCPToolDefinition::fromVar (value));
    });

    py::class_<MCPResourceDefinition> (ai, "MCPResourceDefinition")
        .def (py::init<>())
        .def_readwrite ("uri", &MCPResourceDefinition::uri)
        .def_readwrite ("name", &MCPResourceDefinition::name)
        .def_readwrite ("description", &MCPResourceDefinition::description)
        .def_readwrite ("mimeType", &MCPResourceDefinition::mimeType)
        .def ("toVar", &MCPResourceDefinition::toVar)
        .def_static ("fromVar", [] (const var& value)
    {
        return optionalMCPResourceDefinitionToPython (MCPResourceDefinition::fromVar (value));
    });

    py::class_<MCPTransport, PyMCPTransport> (ai, "MCPTransport")
        .def (py::init<>())
        .def ("sendMessage", &MCPTransport::sendMessage)
        .def ("receiveMessage", [] (MCPTransport& self, int timeoutMs)
    {
        auto result = self.receiveMessage (timeoutMs);
        if (result.failed())
            py::pybind11_fail (result.getErrorMessage().toRawUTF8());

        return result.getValue();
    },
              "timeoutMs"_a = -1)
        .def ("setMessageHandler", &MCPTransport::setMessageHandler)
        .def ("start", &MCPTransport::start)
        .def ("stop", &MCPTransport::stop)
        .def ("isConnected", &MCPTransport::isConnected);

    py::class_<MCPClient> (ai, "MCPClient")
        .def (py::init<std::unique_ptr<MCPTransport>>(), "transport"_a)
        .def ("initialize", &MCPClient::initialize, "clientCapabilities"_a = MCPCapabilities {})
        .def ("listTools", &MCPClient::listTools)
        .def ("callTool", [] (MCPClient& self, const String& toolName, const var& arguments)
    {
        auto result = self.callTool (toolName, arguments);
        if (result.failed())
            py::pybind11_fail (result.getErrorMessage().toRawUTF8());

        return result.getValue();
    },
              "toolName"_a,
              "arguments"_a)
        .def ("listResources", &MCPClient::listResources)
        .def ("readResource", [] (MCPClient& self, const String& uri)
    {
        auto result = self.readResource (uri);
        if (result.failed())
            py::pybind11_fail (result.getErrorMessage().toRawUTF8());

        return result.getValue();
    },
              "uri"_a)
        .def ("registerToolsWith", &MCPClient::registerToolsWith)
        .def ("getTransport", &MCPClient::getTransport, py::return_value_policy::reference_internal);

    py::class_<MCPServer::Options> (ai, "MCPServerOptions")
        .def (py::init<>())
        .def_readwrite ("serverName", &MCPServer::Options::serverName)
        .def_readwrite ("serverVersion", &MCPServer::Options::serverVersion)
        .def_readwrite ("capabilities", &MCPServer::Options::capabilities);

    py::class_<MCPServer> (ai, "MCPServer")
        .def (py::init<>())
        .def (py::init<MCPServer::Options>())
        .def ("registerTool", [] (MCPServer& self, MCPToolDefinition tool, py::function function)
    {
        self.registerTool (std::move (tool), [function = std::move (function)] (const var& arguments) -> var
        {
            py::gil_scoped_acquire gil;
            return function (arguments).cast<var>();
        });
    },
              "tool"_a,
              "function"_a)
        .def ("registerLLMTool", static_cast<void (MCPServer::*) (LLMTool)> (&MCPServer::registerTool), "tool"_a)
        .def ("unregisterTool", &MCPServer::unregisterTool)
        .def ("registerResource", [] (MCPServer& self, MCPResourceDefinition resource, py::function function)
    {
        self.registerResource (std::move (resource), [function = std::move (function)]() -> String
        {
            py::gil_scoped_acquire gil;
            return py::str (function()).cast<String>();
        });
    },
              "resource"_a,
              "function"_a)
        .def ("unregisterResource", &MCPServer::unregisterResource)
        .def ("start", &MCPServer::start, "transport"_a)
        .def ("stop", &MCPServer::stop)
        .def ("isRunning", &MCPServer::isRunning)
        .def ("startStdio", &MCPServer::startStdio)
        .def ("startHttp", &MCPServer::startHttp, "port"_a);
}

} // namespace yup::Bindings
