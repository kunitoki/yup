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

ComponentTypeRegistry& ComponentTypeRegistry::getInstance()
{
    static ComponentTypeRegistry instance;
    return instance;
}

void ComponentTypeRegistry::registerType (const ComponentTypeInfo& typeInfo)
{
    const ScopedLock sl (lock);
    types.insert_or_assign (typeInfo.getTypeName(), &typeInfo);
}

void ComponentTypeRegistry::unregisterType (const Identifier& typeName)
{
    const ScopedLock sl (lock);
    types.erase (typeName);
}

const ComponentTypeInfo* ComponentTypeRegistry::findType (const Identifier& typeName) const
{
    const ScopedLock sl (lock);

    if (auto it = types.find (typeName); it != types.end())
        return it->second;

    return nullptr;
}

bool ComponentTypeRegistry::isRegistered (const Identifier& typeName) const
{
    const ScopedLock sl (lock);
    return types.find (typeName) != types.end();
}

StringArray ComponentTypeRegistry::getRegisteredTypeNames() const
{
    const ScopedLock sl (lock);

    StringArray names;
    for (const auto& entry : types)
        names.add (entry.first.toString());

    names.sort (false);
    return names;
}

std::unique_ptr<Component> ComponentTypeRegistry::createInstance (const Identifier& typeName) const
{
    if (auto* typeInfo = findType (typeName))
        return typeInfo->createInstance();

    return nullptr;
}

void ComponentTypeRegistry::clear()
{
    const ScopedLock sl (lock);
    types.clear();
}

//==============================================================================

void registerBuiltInComponentTypes()
{
    auto& registry = ComponentTypeRegistry::getInstance();

    registry.registerType (Component::getClassTypeInfo());
    registry.registerType (Label::getClassTypeInfo());
    registry.registerType (Slider::getClassTypeInfo());
    registry.registerType (TextButton::getClassTypeInfo());
    registry.registerType (ToggleButton::getClassTypeInfo());
    registry.registerType (SwitchButton::getClassTypeInfo());
    registry.registerType (ComboBox::getClassTypeInfo());
    registry.registerType (TextEditor::getClassTypeInfo());
    registry.registerType (ProgressBar::getClassTypeInfo());
    registry.registerType (ScrollBar::getClassTypeInfo());
    registry.registerType (ListBox::getClassTypeInfo());
    registry.registerType (FlexBoxComponent::getClassTypeInfo());
    registry.registerType (RowComponent::getClassTypeInfo());
    registry.registerType (ColumnComponent::getClassTypeInfo());
    registry.registerType (GridComponent::getClassTypeInfo());
    registry.registerType (StackComponent::getClassTypeInfo());
}

} // namespace yup
