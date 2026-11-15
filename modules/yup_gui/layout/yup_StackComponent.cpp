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

const detail::EnumNames<StackLayout::Align> stackAlignments {
    { "stretch", StackLayout::Align::stretch },
    { "start", StackLayout::Align::start },
    { "center", StackLayout::Align::center },
    { "end", StackLayout::Align::end }
};

} // namespace

//==============================================================================

const ComponentTypeInfo& StackComponent::getClassTypeInfo()
{
    static const ComponentTypeInfo info = []
    {
        std::vector<ComponentProperty> props;

        props.push_back (ComponentProperty {
            .name = Properties::justifyId,
            .type = ComponentProperty::Type::enumeration,
            .defaultValue = stackAlignments.names[0],
            .isInherited = false,
            .enumValues = stackAlignments.names,
            .description = "How the children that don't align themselves are placed horizontally." });

        props.push_back (ComponentProperty {
            .name = Properties::alignId,
            .type = ComponentProperty::Type::enumeration,
            .defaultValue = stackAlignments.names[0],
            .isInherited = false,
            .enumValues = stackAlignments.names,
            .description = "How the children that don't align themselves are placed vertically." });

        return ComponentTypeInfo { Identifier ("Stack"), &LayoutComponent::getClassTypeInfo(), std::move (props), {}, []
                                   {
            return std::make_unique<StackComponent>();
                                  } };
    }();

    return info;
}

const ComponentTypeInfo& StackComponent::getDefaultTypeInfo() const
{
    return getClassTypeInfo();
}

//==============================================================================

void StackComponent::performLayout (Rectangle<float> area)
{
    StackLayout stack;

    stack.alignX = stackAlignments.toEnum (findProperty (Properties::justifyId).toString(), StackLayout::Align::stretch);
    stack.alignY = stackAlignments.toEnum (findProperty (Properties::alignId).toString(), StackLayout::Align::stretch);

    for (auto* child : getLayoutChildren())
    {
        StackLayout::Item item (*child);

        item.width = getItemNumber (*child, Properties::preferredWidthId);
        item.height = getItemNumber (*child, Properties::preferredHeightId);
        item.alignX = stackAlignments.toEnum (getItemString (*child, Properties::justifySelfId), StackLayout::Align::automatic);
        item.alignY = stackAlignments.toEnum (getItemString (*child, Properties::alignSelfId), StackLayout::Align::automatic);
        item.margin = getItemMargin (*child);

        stack.items.add (item);
    }

    stack.performLayout (area);
}

} // namespace yup
