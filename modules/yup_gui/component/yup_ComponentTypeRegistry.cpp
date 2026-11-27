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

void ComponentTypeRegistry::registerDefaultType (const ComponentTypeInfo& typeInfo)
{
    registerDefaultTypes ({ typeInfo });
}

void ComponentTypeRegistry::registerDefaultTypes (std::initializer_list<std::reference_wrapper<const ComponentTypeInfo>> typeInfos)
{
    const ScopedLock sl (lock);

    for (const ComponentTypeInfo& typeInfo : typeInfos)
        types.try_emplace (typeInfo.getTypeName(), &typeInfo);
}

void ComponentTypeRegistry::unregisterType (const ComponentTypeInfo& typeInfo)
{
    const ScopedLock sl (lock);

    if (const auto found = types.find (typeInfo.getTypeName()); found != types.end() && found->second == &typeInfo)
        types.erase (found);
}

const ComponentTypeInfo* ComponentTypeRegistry::findType (const Identifier& typeName) const
{
    const ScopedLock sl (lock);

    if (auto it = types.find (typeName); it != types.end())
        return it->second;

    return nullptr;
}

std::vector<const ComponentTypeInfo*> ComponentTypeRegistry::getRegisteredTypes() const
{
    const ScopedLock sl (lock);

    std::vector<const ComponentTypeInfo*> result;
    result.reserve (types.size());

    for (const auto& [name, info] : types)
        result.push_back (info);

    std::sort (result.begin(), result.end(), [] (const ComponentTypeInfo* a, const ComponentTypeInfo* b)
    {
        return a->getTypeName().toString() < b->getTypeName().toString();
    });

    return result;
}

//==============================================================================

void registerBuiltInComponentTypes()
{
    static std::once_flag once;
    std::call_once (once, []
    {
        ComponentTypeRegistry::getInstance().registerDefaultTypes ({
            Component::getClassTypeInfo(),
            Button::getClassTypeInfo(),
            Label::getClassTypeInfo(),
            Slider::getClassTypeInfo(),
            TextButton::getClassTypeInfo(),
            ToggleButton::getClassTypeInfo(),
            SwitchButton::getClassTypeInfo(),
            ComboBox::getClassTypeInfo(),
            PopupMenu::getClassTypeInfo(),
            TextEditor::getClassTypeInfo(),
            ProgressBar::getClassTypeInfo(),
            ImageComponent::getClassTypeInfo(),
            ScrollBar::getClassTypeInfo(),
            ListBoxItem::getClassTypeInfo(),
            ListBox::getClassTypeInfo(),
            TreeViewRow::getClassTypeInfo(),
            TreeView::getClassTypeInfo(),
            Viewport::getClassTypeInfo(),
            TabButton::getClassTypeInfo(),
            TabBar::getClassTypeInfo(),
            TabComponent::getClassTypeInfo(),
            ResizableCorner::getClassTypeInfo(),
            ResizableDivider::getClassTypeInfo(),
            JoystickComponent::getClassTypeInfo(),
            CodeEditor::getClassTypeInfo(),
            DocumentWindow::getClassTypeInfo(),
            LayoutComponent::getClassTypeInfo(),
            FlexBoxComponent::getClassTypeInfo(),
            RowComponent::getClassTypeInfo(),
            ColumnComponent::getClassTypeInfo(),
            GridComponent::getClassTypeInfo(),
            StackComponent::getClassTypeInfo(),
            WindowComponent::getClassTypeInfo(),
        });
    });
}

} // namespace yup
