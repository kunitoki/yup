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
/** Thread-safe storage and dispatch for LLM tools.

    @tags{AI}
*/
class YUP_API LLMToolRegistry
{
public:
    /** Adds or replaces a tool by name. */
    void registerTool (LLMTool tool);

    /** Removes a tool by name if it exists. */
    void unregisterTool (const String& name);

    /** Returns true if a tool with this name exists. */
    bool contains (const String& name) const noexcept;

    /** Returns a pointer to a copied cache entry for immediate read-only use.

        Prefer getAllTools() or dispatchToolCall() for thread-safe ownership
        across longer lifetimes.
    */
    const LLMTool* findTool (const String& name) const noexcept;

    /** Returns a snapshot of all registered tools. */
    std::vector<LLMTool> getAllTools() const;

    /** Converts all registered tools to the OpenAI tools array. */
    var toToolsArray() const;

    /** Dispatches a tool call by name. Missing tools return an error object. */
    var dispatchToolCall (const String& name, const var& arguments) const;

private:
    mutable CriticalSection mutex;
    mutable std::optional<LLMTool> lookupCache;
    std::unordered_map<String, LLMTool> tools;
};

} // namespace yup
