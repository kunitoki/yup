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
/** Describes an LLM-callable function and its JSON Schema parameter model.

    Tools are serialised to the OpenAI function-calling format. The handler is a
    local callable that receives JSON-compatible arguments and returns a
    JSON-compatible result.

    @tags{AI}
*/
class YUP_API LLMTool
{
public:
    /** A JSON Schema parameter description. */
    struct Parameter
    {
        String name;
        String type;
        String description;
        bool required = false;
        std::optional<var> enumValues;
        std::optional<var> defaultValue;
        std::optional<std::vector<Parameter>> properties;
    };

    using Handler = std::function<var (const var& arguments)>;

    String name;
    String description;
    std::vector<Parameter> parameters;

    /** Converts this tool to the OpenAI function-calling schema. */
    var toJsonSchema() const;

    /** Executes the registered handler.

        If no handler is installed, the returned value is an error object.
    */
    var execute (const var& arguments) const;

    /** Installs or replaces the handler for this tool. */
    void setHandler (Handler newHandler);

private:
    Handler handler;
};

} // namespace yup
