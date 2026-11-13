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

ComponentTypeInfo::ComponentTypeInfo (Identifier typeNameToUse,
                                      const ComponentTypeInfo* baseTypeInfoToUse,
                                      std::vector<ComponentProperty> propertiesToUse,
                                      StringArray signalsToUse,
                                      Factory factoryToUse)
    : typeName (std::move (typeNameToUse))
    , baseTypeInfo (baseTypeInfoToUse)
    , properties (std::move (propertiesToUse))
    , signals (std::move (signalsToUse))
    , factory (std::move (factoryToUse))
{
    for (int index = 0; index < static_cast<int> (properties.size()); ++index)
        propertyIndex.insert_or_assign (properties[static_cast<size_t> (index)].name, index);
}

const ComponentProperty* ComponentTypeInfo::findProperty (const Identifier& name) const
{
    if (auto it = propertyIndex.find (name); it != propertyIndex.end())
        return &properties[static_cast<size_t> (it->second)];

    if (baseTypeInfo != nullptr)
        return baseTypeInfo->findProperty (name);

    return nullptr;
}

const ComponentProperty* ComponentTypeInfo::findPropertyByStorageKey (const Identifier& storageKey) const
{
    for (const auto& property : properties)
        if (property.isStoreBacked() && property.getStorageKey() == storageKey)
            return &property;

    if (baseTypeInfo != nullptr)
        return baseTypeInfo->findPropertyByStorageKey (storageKey);

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
    return isSubtypeOf (other.typeName);
}

bool ComponentTypeInfo::hasSignal (const Identifier& signalName) const
{
    for (const auto* info = this; info != nullptr; info = info->baseTypeInfo)
        if (info->signals.contains (signalName.toString()))
            return true;

    return false;
}

std::unique_ptr<Component> ComponentTypeInfo::createInstance() const
{
    if (factory != nullptr)
        return factory();

    return nullptr;
}

} // namespace yup
