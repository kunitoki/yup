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
    type's Component::getTypeInfo(), so the registry never owns the infos. A type info
    that does not live as long as the process is removed with unregisterType() before
    it goes.

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

    /** Deleted: the registry would point at a temporary once it is gone. */
    void registerType (const ComponentTypeInfo&&) = delete;

    /** Registers a type info unless one is registered under its type name already.

        @param typeInfo    The type info to register. It must outlive the registry entry.

        @see registerDefaultTypes
    */
    void registerDefaultType (const ComponentTypeInfo& typeInfo);

    /** Deleted: the registry would point at a temporary once it is gone. */
    void registerDefaultType (const ComponentTypeInfo&&) = delete;

    /** Registers type infos, each unless one is registered under its type name already, locking the
        registry once for them all.

        This is how a module registers the types it comes with, as registerBuiltInComponentTypes()
        does: they are registered when something first needs them, which may be after an
        application registered a type of its own under one of their names, and that one stays.

        @code
        ComponentTypeRegistry::getInstance().registerDefaultTypes ({ Knob::getClassTypeInfo(), Fader::getClassTypeInfo() });
        @endcode

        @param typeInfos    The type infos to register. They must outlive their registry entries.
    */
    void registerDefaultTypes (std::initializer_list<std::reference_wrapper<const ComponentTypeInfo>> typeInfos);

    /** Removes a type info from the registry.

        This is for a type info that does not live as long as the process, such as the one of a
        library that is unloaded or of a test, which is removed before it goes. The entry of its
        type name is removed only when it is this very type info: another one registered under
        the name since stays.

        @param typeInfo    The type info to remove, registered or not.
    */
    void unregisterType (const ComponentTypeInfo& typeInfo);

    /** Finds a registered type info by name, or nullptr when it is not registered. */
    const ComponentTypeInfo* findType (const Identifier& typeName) const;

    /** Returns all registered component type infos, sorted by type name. */
    std::vector<const ComponentTypeInfo*> getRegisteredTypes() const;

private:
    //==============================================================================
    ComponentTypeRegistry() = default;

    std::unordered_map<Identifier, const ComponentTypeInfo*> types;
    mutable CriticalSection lock;

    YUP_DECLARE_NON_COPYABLE (ComponentTypeRegistry)
};

//==============================================================================
/** Registers the type infos of the built-in yup_gui components into the registry.

    A type registered before under the name of one of them is left in place (see
    ComponentTypeRegistry::registerDefaultTypes).
*/
YUP_API void registerBuiltInComponentTypes();

} // namespace yup
