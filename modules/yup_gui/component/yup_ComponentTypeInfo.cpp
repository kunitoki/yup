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

Rectangle<float> ComponentPart::getArea (const Component& component) const
{
    return area != nullptr ? area (component) : component.getLocalBounds();
}

//==============================================================================

ComponentTypeInfo::ComponentTypeInfo (Identifier typeNameToUse,
                                      const ComponentTypeInfo* baseTypeInfoToUse,
                                      std::vector<ComponentProperty> propertiesToUse,
                                      std::vector<ComponentSignal> signalsToUse,
                                      Factory factoryToUse,
                                      ChildAttacher childAttacherToUse,
                                      std::vector<ComponentPart> partsToUse,
                                      String descriptionToUse,
                                      std::type_index classTypeToUse)
    : typeName (std::move (typeNameToUse))
    , baseTypeInfo (baseTypeInfoToUse)
    , properties (std::move (propertiesToUse))
    , signals (std::move (signalsToUse))
    , parts (std::move (partsToUse))
    , factory (std::move (factoryToUse))
    , childAttacher (std::move (childAttacherToUse))
    , description (std::move (descriptionToUse))
    , classType (classTypeToUse)
{
    rebuildIndices();
}

void ComponentTypeInfo::rebuildIndices()
{
    mergedPropertyIndex.clear();
    storageKeyIndex.clear();

    if (baseTypeInfo != nullptr)
    {
        mergedPropertyIndex = baseTypeInfo->mergedPropertyIndex;
        storageKeyIndex = baseTypeInfo->storageKeyIndex;
    }

    for (const auto& property : properties)
    {
        mergedPropertyIndex.insert_or_assign (property.name, &property);

        if (property.isStoreBacked())
            storageKeyIndex.insert_or_assign (property.getStorageKey(), &property);
    }
}

const ComponentProperty* ComponentTypeInfo::findProperty (const Identifier& name) const
{
    if (const auto it = mergedPropertyIndex.find (name); it != mergedPropertyIndex.end())
        return it->second;

    return nullptr;
}

const ComponentProperty* ComponentTypeInfo::findPropertyByStorageKey (const Identifier& storageKey) const
{
    if (const auto it = storageKeyIndex.find (storageKey); it != storageKeyIndex.end())
        return it->second;

    return nullptr;
}

std::vector<const ComponentProperty*> ComponentTypeInfo::getAllProperties() const
{
    std::vector<const ComponentProperty*> result;
    std::unordered_map<Identifier, bool> seen;

    for (const auto* info = this; info != nullptr; info = info->baseTypeInfo)
    {
        for (const auto& property : info->properties)
        {
            if (seen.find (property.name) != seen.end())
                continue;

            seen.insert_or_assign (property.name, true);
            result.push_back (&property);
        }
    }

    return result;
}

bool ComponentTypeInfo::isSubtypeOf (const Identifier& otherTypeName) const
{
    for (const auto* info = this; info != nullptr; info = info->baseTypeInfo)
        if (info->typeName == otherTypeName)
            return true;

    return false;
}

bool ComponentTypeInfo::isSubtypeOf (const ComponentTypeInfo& other) const
{
    for (const auto* info = this; info != nullptr; info = info->baseTypeInfo)
        if (info == std::addressof (other))
            return true;

    return false;
}

const ComponentSignal* ComponentTypeInfo::findSignal (const Identifier& signalName) const
{
    for (const auto* info = this; info != nullptr; info = info->baseTypeInfo)
        for (const auto& signal : info->signals)
            if (signal.name == signalName)
                return &signal;

    return nullptr;
}

const ComponentPart* ComponentTypeInfo::findPart (const Identifier& partName) const
{
    for (const auto* info = this; info != nullptr; info = info->baseTypeInfo)
        for (const auto& part : info->parts)
            if (part.name == partName)
                return &part;

    return nullptr;
}

std::vector<const ComponentPart*> ComponentTypeInfo::getAllParts() const
{
    std::vector<const ComponentPart*> result;

    for (const auto* info = this; info != nullptr; info = info->baseTypeInfo)
    {
        for (const auto& part : info->parts)
        {
            const auto isOverridden = std::any_of (result.begin(), result.end(), [&part] (const ComponentPart* known)
            {
                return known->name == part.name;
            });

            if (! isOverridden)
                result.push_back (&part);
        }
    }

    return result;
}

std::unique_ptr<Component> ComponentTypeInfo::createInstance() const
{
    if (factory != nullptr)
        return factory();

    return nullptr;
}

bool ComponentTypeInfo::hasChildAttacher() const noexcept
{
    for (const auto* info = this; info != nullptr; info = info->baseTypeInfo)
        if (info->childAttacher != nullptr)
            return true;

    return false;
}

void ComponentTypeInfo::attachChild (Component& parent, Component& child) const
{
    for (const auto* info = this; info != nullptr; info = info->baseTypeInfo)
    {
        if (info->childAttacher != nullptr)
        {
            info->childAttacher (parent, child);
            return;
        }
    }

    parent.addAndMakeVisible (child);
}

} // namespace yup
