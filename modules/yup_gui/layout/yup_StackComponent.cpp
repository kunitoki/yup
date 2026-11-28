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
    using Type = ComponentProperty::Type;
    using LayoutEffect = ComponentProperty::LayoutEffect;

    static const auto info = ComponentTypeBuilder<StackComponent> (Identifier ("Stack"), LayoutComponent::getClassTypeInfo())
                                 .layoutProperty (Properties::justifyId, Type::enumeration, stackAlignments.names[0], LayoutEffect::own, "How the children that don't align themselves are placed horizontally.", stackAlignments.names)
                                 .layoutProperty (Properties::alignId, Type::enumeration, stackAlignments.names[0], LayoutEffect::own, "How the children that don't align themselves are placed vertically.", stackAlignments.names)
                                 .build();

    return info;
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

        readItemSizing (item, *child);
        item.alignX = stackAlignments.toEnum (getItemString (*child, Properties::justifySelfId), StackLayout::Align::automatic);
        item.alignY = stackAlignments.toEnum (getItemString (*child, Properties::alignSelfId), StackLayout::Align::automatic);

        stack.items.add (item);
    }

    stack.performLayout (area);
}

std::optional<Size<float>> StackComponent::measureContent() const
{
    Size<float> content;

    for (const auto* child : getLayoutChildren())
    {
        const auto size = getMeasuredItemSize (*child);
        content = { jmax (content.getWidth(), size.getWidth()), jmax (content.getHeight(), size.getHeight()) };
    }

    return content;
}

} // namespace yup
