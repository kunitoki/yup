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
/** Fluent helpers for building JSON Schema objects used in LLMClient::Request::schema.

    Pass the result of these helpers to LLMClient::Request::schema to request
    structured (JSON) output from the LLM.  All major providers (OpenAI Chat,
    OpenAI Responses, Anthropic, Gemini) accept a JSON Schema for their
    respective structured-output mechanisms.

    @code
    yup::LLMClient::Request request;
    request.messages.push_back (yup::LLMMessage::user ("Extract the key facts."));
    request.schema = yup::LLMSchema::object ({
        { "title",   yup::LLMSchema::string() },
        { "summary", yup::LLMSchema::string() },
        { "year",    yup::LLMSchema::integer() },
    });
    auto response = client.complete (request);
    @endcode

    @tags{AI}
*/
class YUP_API LLMSchema
{
public:
    /** Returns a JSON Schema node of type "string". */
    static var string()
    {
        auto obj = makeObj();
        setProperty (obj, "type", String ("string"));
        return obj;
    }

    /** Returns a JSON Schema node of type "number" (floating-point). */
    static var number()
    {
        auto obj = makeObj();
        setProperty (obj, "type", String ("number"));
        return obj;
    }

    /** Returns a JSON Schema node of type "integer". */
    static var integer()
    {
        auto obj = makeObj();
        setProperty (obj, "type", String ("integer"));
        return obj;
    }

    /** Returns a JSON Schema node of type "boolean". */
    static var boolean()
    {
        auto obj = makeObj();
        setProperty (obj, "type", String ("boolean"));
        return obj;
    }

    /** Returns a JSON Schema array node whose items conform to @p itemSchema. */
    static var array (const var& itemSchema)
    {
        auto obj = makeObj();
        setProperty (obj, "type", String ("array"));
        setProperty (obj, "items", itemSchema);
        return obj;
    }

    /** Returns a JSON Schema object node with the given named field schemas.

        All listed fields are marked as required and additionalProperties is
        set to false, which is required for strict mode on OpenAI.

        @code
        auto schema = yup::LLMSchema::object ({
            { "name",  yup::LLMSchema::string() },
            { "score", yup::LLMSchema::number() },
        });
        @endcode
    */
    static var object (std::initializer_list<std::pair<String, var>> fields)
    {
        auto properties = makeObj();
        var requiredArray;

        for (const auto& [name, fieldSchema] : fields)
        {
            setProperty (properties, name, fieldSchema);
            requiredArray.append (name);
        }

        auto obj = makeObj();
        setProperty (obj, "type", String ("object"));
        setProperty (obj, "properties", properties);
        setProperty (obj, "required", requiredArray);
        setProperty (obj, "additionalProperties", false);
        return obj;
    }

    /** Returns a JSON Schema string node restricted to one of the given @p values. */
    static var oneOf (std::initializer_list<String> values)
    {
        var enumArray;

        for (const auto& v : values)
            enumArray.append (v);

        auto obj = makeObj();
        setProperty (obj, "type", String ("string"));
        setProperty (obj, "enum", enumArray);
        return obj;
    }

    /** Serialises a schema node to a compact JSON string. */
    static String toJsonString (const var& schema)
    {
        return JSON::toString (schema, true);
    }

private:
    static var makeObj()
    {
        return var (std::make_unique<DynamicObject>());
    }

    static void setProperty (var& object, const Identifier& name, const var& value)
    {
        if (auto* obj = object.getDynamicObject())
            obj->setProperty (name, value);
    }
};

} // namespace yup
