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
var makeToolRegistryObject()
{
    return var (std::make_unique<DynamicObject>());
}

void setToolRegistryProperty (var& object, const Identifier& name, const var& value)
{
    if (auto* dynamicObject = object.getDynamicObject())
        dynamicObject->setProperty (name, value);
}

var makeToolRegistryErrorObject (const String& message)
{
    auto object = makeToolRegistryObject();
    setToolRegistryProperty (object, "error", true);
    setToolRegistryProperty (object, "message", message);
    return object;
}
} // namespace

void LLMToolRegistry::registerTool (LLMTool tool)
{
    const ScopedLock lock (mutex);
    tools[tool.name] = std::move (tool);
    lookupCache.reset();
}

void LLMToolRegistry::unregisterTool (const String& name)
{
    const ScopedLock lock (mutex);
    tools.erase (name);
    lookupCache.reset();
}

bool LLMToolRegistry::contains (const String& name) const noexcept
{
    const ScopedLock lock (mutex);
    return tools.find (name) != tools.end();
}

const LLMTool* LLMToolRegistry::findTool (const String& name) const noexcept
{
    const ScopedLock lock (mutex);

    if (auto iter = tools.find (name); iter != tools.end())
    {
        lookupCache = iter->second;
        return std::addressof (*lookupCache);
    }

    lookupCache.reset();
    return nullptr;
}

std::vector<LLMTool> LLMToolRegistry::getAllTools() const
{
    const ScopedLock lock (mutex);

    std::vector<LLMTool> result;
    result.reserve (tools.size());

    for (const auto& entry : tools)
        result.push_back (entry.second);

    return result;
}

var LLMToolRegistry::toToolsArray() const
{
    var result;

    for (const auto& tool : getAllTools())
        result.append (tool.toJsonSchema());

    return result;
}

var LLMToolRegistry::dispatchToolCall (const String& name, const var& arguments) const
{
    std::optional<LLMTool> tool;

    {
        const ScopedLock lock (mutex);

        if (auto iter = tools.find (name); iter != tools.end())
            tool = iter->second;
    }

    if (! tool.has_value())
        return makeToolRegistryErrorObject ("Unknown tool '" + name + "'");

    return tool->execute (arguments);
}

} // namespace yup
