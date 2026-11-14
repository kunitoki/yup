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

class Component;

//==============================================================================
/**
    Describes a component type for the reflected component model.

    A ComponentTypeInfo carries the type name (for example "Slider"), a pointer to
    its base type's info, the property descriptors declared on the type, the signals
    it emits (events such as "onValueChanged"), and an optional factory used to
    create instances from `.yui`.

    Each widget returns a single shared instance from Component::getDefaultTypeInfo(),
    and a single component can be given its own with Component::setTypeInfo(). Type
    infos form a chain through @ref getBaseTypeInfo, so property and signal lookups
    walk up to the base. Unregistered user subclasses simply inherit the info of the
    nearest registered base.

    @see Component, ComponentProperty, ComponentTypeRegistry

    @tags{UI}
*/
class YUP_API ComponentTypeInfo
{
public:
    //==============================================================================
    /** A factory that creates a component instance of this type. */
    using Factory = std::function<std::unique_ptr<Component>()>;

    //==============================================================================
    /** Creates a type info.

        @param typeName        The type name, for example "Slider".
        @param baseTypeInfo    The base type's info, or nullptr for the root type.
        @param properties      The property descriptors declared on this type.
        @param signals         The signals (events) this type emits.
        @param factory         An optional factory used to instantiate this type.
    */
    ComponentTypeInfo (Identifier typeName,
                       const ComponentTypeInfo* baseTypeInfo,
                       std::vector<ComponentProperty> properties,
                       std::vector<ComponentSignal> signals = {},
                       Factory factory = {});

    //==============================================================================
    /** Returns the type name, for example "Slider". */
    const Identifier& getTypeName() const noexcept { return typeName; }

    /** Returns the base type's info, or nullptr for the root type. */
    const ComponentTypeInfo* getBaseTypeInfo() const noexcept { return baseTypeInfo; }

    //==============================================================================
    /** Returns the property descriptors declared directly on this type (not inherited). */
    const std::vector<ComponentProperty>& getOwnProperties() const noexcept { return properties; }

    /** Finds a property by name, walking up the base chain.

        A property declared on a derived type overrides one with the same name on a base.

        @param name    The property name to find.

        @return The matching descriptor, or nullptr if the type does not declare it.
    */
    const ComponentProperty* findProperty (const Identifier& name) const;

    /** Finds a store-backed property by its storage key, walking up the base chain.

        @param storageKey    The storage id (or name) a store-backed property uses.

        @return The matching descriptor, or nullptr if none is found.
    */
    const ComponentProperty* findPropertyByStorageKey (const Identifier& storageKey) const;

    /** Returns true when a property with the given name is declared on this type or a base. */
    bool hasProperty (const Identifier& name) const { return findProperty (name) != nullptr; }

    /** Returns every property visible on this type, with derived declarations overriding base ones. */
    std::vector<const ComponentProperty*> getAllProperties() const;

    //==============================================================================
    /** Returns true when this type is, or derives from, a type with the given name. */
    bool isSubtypeOf (const Identifier& otherTypeName) const;

    /** Returns true when this type is, or derives from, the given type. */
    bool isSubtypeOf (const ComponentTypeInfo& other) const;

    //==============================================================================
    /** Returns the signals (events) declared directly on this type (not inherited). */
    const std::vector<ComponentSignal>& getOwnSignals() const noexcept { return signals; }

    /** Finds a signal by name, walking up the base chain.

        @param signalName    The signal name to find.

        @return The matching descriptor, or nullptr if the type does not emit it.
    */
    const ComponentSignal* findSignal (const Identifier& signalName) const;

    /** Returns true when this type or a base emits the given signal. */
    bool hasSignal (const Identifier& signalName) const { return findSignal (signalName) != nullptr; }

    //==============================================================================
    /** Returns true when a factory is available to instantiate this type. */
    bool hasFactory() const noexcept { return factory != nullptr; }

    /** Creates a new instance of this type, or nullptr when no factory is available. */
    std::unique_ptr<Component> createInstance() const;

private:
    //==============================================================================
    Identifier typeName;
    const ComponentTypeInfo* baseTypeInfo = nullptr;
    std::vector<ComponentProperty> properties;
    std::unordered_map<Identifier, int> propertyIndex;
    std::vector<ComponentSignal> signals;
    Factory factory;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComponentTypeInfo)
};

} // namespace yup
