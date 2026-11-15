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

#pragma once

namespace yup
{

//==============================================================================
/**
    A process-wide registry mapping type names to component type infos.

    The registry is the single table that drives `.yui` instantiation, the CLI
    schema, editor completions and Python introspection. Built-in widgets register
    their type infos through registerBuiltInComponentTypes(); other modules and
    applications register their own types the same way.

    Entries point at the shared, statically owned ComponentTypeInfo returned by each
    type's Component::getTypeInfo(), so the registry never owns the infos.

    @see ComponentTypeInfo, Component::getTypeInfo

    @tags{UI}
*/
class YUP_API ComponentTypeRegistry
{
public:
    //==============================================================================
    /** Returns the process-wide registry instance. */
    static ComponentTypeRegistry& getInstance();

    //==============================================================================
    /** Registers a type info, keyed by its type name.

        Registering a type name that already exists replaces the previous entry.

        @param typeInfo    The type info to register. It must outlive the registry entry.
    */
    void registerType (const ComponentTypeInfo& typeInfo);

    /** Finds a registered type info by name, or nullptr when it is not registered. */
    const ComponentTypeInfo* findType (const Identifier& typeName) const;

private:
    //==============================================================================
    ComponentTypeRegistry() = default;

    std::unordered_map<Identifier, const ComponentTypeInfo*> types;
    mutable CriticalSection lock;

    YUP_DECLARE_NON_COPYABLE (ComponentTypeRegistry)
};

//==============================================================================
/** Registers the type infos of the built-in yup_gui components into the registry. */
void registerBuiltInComponentTypes();

} // namespace yup
