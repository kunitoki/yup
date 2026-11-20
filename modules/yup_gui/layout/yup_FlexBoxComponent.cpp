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

const detail::EnumNames<FlexBox::Direction> flexDirections {
    { "row", FlexBox::Direction::row },
    { "rowReverse", FlexBox::Direction::rowReverse },
    { "column", FlexBox::Direction::column },
    { "columnReverse", FlexBox::Direction::columnReverse }
};

const detail::EnumNames<FlexBox::Wrap> flexWraps {
    { "noWrap", FlexBox::Wrap::noWrap },
    { "wrap", FlexBox::Wrap::wrap },
    { "wrapReverse", FlexBox::Wrap::wrapReverse }
};

const detail::EnumNames<FlexBox::JustifyContent> flexJustifications {
    { "start", FlexBox::JustifyContent::flexStart },
    { "end", FlexBox::JustifyContent::flexEnd },
    { "center", FlexBox::JustifyContent::center },
    { "spaceBetween", FlexBox::JustifyContent::spaceBetween },
    { "spaceAround", FlexBox::JustifyContent::spaceAround },
    { "spaceEvenly", FlexBox::JustifyContent::spaceEvenly }
};

const detail::EnumNames<FlexBox::AlignItems> flexAlignments {
    { "start", FlexBox::AlignItems::flexStart },
    { "end", FlexBox::AlignItems::flexEnd },
    { "center", FlexBox::AlignItems::center },
    { "stretch", FlexBox::AlignItems::stretch },
    { "baseline", FlexBox::AlignItems::baseline }
};

const detail::EnumNames<FlexItem::AlignSelf> flexSelfAlignments {
    { "auto", FlexItem::AlignSelf::autoAlign },
    { "start", FlexItem::AlignSelf::flexStart },
    { "end", FlexItem::AlignSelf::flexEnd },
    { "center", FlexItem::AlignSelf::center },
    { "stretch", FlexItem::AlignSelf::stretch },
    { "baseline", FlexItem::AlignSelf::baseline }
};

} // namespace

//==============================================================================

void FlexBoxComponent::setDirection (FlexBox::Direction newDirection)
{
    setProperty (Properties::directionId, flexDirections.toString (newDirection));
}

FlexBox::Direction FlexBoxComponent::getDirection() const
{
    return flexDirections.toEnum (findProperty (Properties::directionId).toString(), FlexBox::Direction::row);
}

//==============================================================================

const ComponentTypeInfo& FlexBoxComponent::getClassTypeInfo()
{
    using Type = ComponentProperty::Type;
    using LayoutEffect = ComponentProperty::LayoutEffect;

    static const auto info = ComponentTypeBuilder<FlexBoxComponent> (Identifier ("FlexBox"), LayoutComponent::getClassTypeInfo())
                                 .layoutProperty (Properties::directionId, Type::enumeration, flexDirections.names[0], LayoutEffect::own, "The direction the children are laid out in.", flexDirections.names)
                                 .layoutProperty (Properties::wrapId, Type::enumeration, flexWraps.names[0], LayoutEffect::own, "Whether the children wrap onto more lines.", flexWraps.names)
                                 .layoutProperty (Properties::justifyId, Type::enumeration, flexJustifications.names[0], LayoutEffect::own, "How the children are distributed along the main axis.", flexJustifications.names)
                                 .layoutProperty (Properties::alignId, Type::enumeration, flexAlignments.names[3], LayoutEffect::own, "How the children are placed on the cross axis.", flexAlignments.names)
                                 .build();

    return info;
}

//==============================================================================

void FlexBoxComponent::performLayout (Rectangle<float> area)
{
    FlexBox flexBox;

    flexBox.flexDirection = getDirection();
    flexBox.flexWrap = flexWraps.toEnum (findProperty (Properties::wrapId).toString(), FlexBox::Wrap::noWrap);
    flexBox.justifyContent = flexJustifications.toEnum (findProperty (Properties::justifyId).toString(), FlexBox::JustifyContent::flexStart);
    flexBox.alignItems = flexAlignments.toEnum (findProperty (Properties::alignId).toString(), FlexBox::AlignItems::stretch);
    flexBox.gap = getGap();

    for (auto* child : getLayoutChildren())
    {
        FlexItem item (*child);

        item.flexGrow = jmax (0.0f, getItemNumber (*child, Properties::flexGrowId));
        item.flexShrink = jmax (0.0f, getItemNumber (*child, Properties::flexShrinkId));
        item.flexBasis = getItemNumber (*child, Properties::flexBasisId);
        item.order = getItemInteger (*child, Properties::orderId);
        item.alignSelf = flexSelfAlignments.toEnum (getItemString (*child, Properties::alignSelfId), FlexItem::AlignSelf::autoAlign);
        readItemSizing (item, *child);

        flexBox.items.add (item);
    }

    flexBox.performLayout (area);
}

//==============================================================================

const ComponentTypeInfo& RowComponent::getClassTypeInfo()
{
    static const auto info = ComponentTypeBuilder<RowComponent> (Identifier ("Row"), FlexBoxComponent::getClassTypeInfo()).build();
    return info;
}

//==============================================================================

const ComponentTypeInfo& ColumnComponent::getClassTypeInfo()
{
    static const auto info = ComponentTypeBuilder<ColumnComponent> (Identifier ("Column"), FlexBoxComponent::getClassTypeInfo())
                                 .layoutProperty (Properties::directionId, ComponentProperty::Type::enumeration, flexDirections.names[2], ComponentProperty::LayoutEffect::own, "The direction the children are laid out in.", flexDirections.names)
                                 .build();

    return info;
}

} // namespace yup
