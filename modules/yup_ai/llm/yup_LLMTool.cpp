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
var makeLLMToolObject()
{
    return var (std::make_unique<DynamicObject>());
}

void setLLMToolProperty (var& object, const Identifier& name, const var& value)
{
    if (auto* dynamicObject = object.getDynamicObject())
        dynamicObject->setProperty (name, value);
}

var makeErrorObject (const String& message)
{
    auto object = makeLLMToolObject();
    setLLMToolProperty (object, "error", true);
    setLLMToolProperty (object, "message", message);
    return object;
}

var parameterToSchema (const LLMTool::Parameter& parameter)
{
    auto schema = makeLLMToolObject();
    setLLMToolProperty (schema, "type", parameter.type);

    if (parameter.description.isNotEmpty())
        setLLMToolProperty (schema, "description", parameter.description);

    if (parameter.enumValues.has_value())
        setLLMToolProperty (schema, "enum", *parameter.enumValues);

    if (parameter.defaultValue.has_value())
        setLLMToolProperty (schema, "default", *parameter.defaultValue);

    if (parameter.properties.has_value())
    {
        auto properties = makeLLMToolObject();
        var required;

        for (const auto& child : *parameter.properties)
        {
            setLLMToolProperty (properties, child.name, parameterToSchema (child));

            if (child.required)
                required.append (child.name);
        }

        setLLMToolProperty (schema, "properties", properties);

        if (required.size() > 0)
            setLLMToolProperty (schema, "required", required);
    }

    return schema;
}
} // namespace

var LLMTool::toJsonSchema() const
{
    auto properties = makeLLMToolObject();
    var required;

    for (const auto& parameter : parameters)
    {
        setLLMToolProperty (properties, parameter.name, parameterToSchema (parameter));

        if (parameter.required)
            required.append (parameter.name);
    }

    auto parameterSchema = makeLLMToolObject();
    setLLMToolProperty (parameterSchema, "type", "object");
    setLLMToolProperty (parameterSchema, "properties", properties);

    if (required.size() > 0)
        setLLMToolProperty (parameterSchema, "required", required);

    auto functionObject = makeLLMToolObject();
    setLLMToolProperty (functionObject, "name", name);
    setLLMToolProperty (functionObject, "description", description);
    setLLMToolProperty (functionObject, "parameters", parameterSchema);

    auto toolObject = makeLLMToolObject();
    setLLMToolProperty (toolObject, "type", "function");
    setLLMToolProperty (toolObject, "function", functionObject);

    return toolObject;
}

var LLMTool::execute (const var& arguments) const
{
    if (! handler)
        return makeErrorObject ("No handler registered for tool '" + name + "'");

    return handler (arguments);
}

void LLMTool::setHandler (Handler newHandler)
{
    handler = std::move (newHandler);
}

} // namespace yup
