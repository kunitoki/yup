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

const StringArray directions { "row", "rowReverse", "column", "columnReverse" };
const StringArray wraps { "noWrap", "wrap", "wrapReverse" };
const StringArray justifications { "start", "end", "center", "spaceBetween", "spaceAround", "spaceEvenly" };
const StringArray flexAlignments { "start", "end", "center", "stretch", "baseline" };
const StringArray flexSelfAlignments { "auto", "start", "end", "center", "stretch", "baseline" };

template <class EnumType>
EnumType toFlexEnum (const String& name, const StringArray& names, EnumType fallback)
{
    const auto index = names.indexOf (name);
    return index >= 0 ? static_cast<EnumType> (index) : fallback;
}

FlexItem::AlignSelf toFlexAlignSelf (const String& name)
{
    const auto index = flexSelfAlignments.indexOf (name);
    return index > 0 ? static_cast<FlexItem::AlignSelf> (index) : FlexItem::AlignSelf::autoAlign;
}

} // namespace

//==============================================================================

void FlexBoxComponent::setDirection (FlexBox::Direction newDirection)
{
    setProperty (Properties::directionId, directions[static_cast<int> (newDirection)]);
}

FlexBox::Direction FlexBoxComponent::getDirection() const
{
    return toFlexEnum (findProperty (Properties::directionId).toString(), directions, FlexBox::Direction::row);
}

//==============================================================================

const ComponentTypeInfo& FlexBoxComponent::getClassTypeInfo()
{
    static const ComponentTypeInfo info = []
    {
        std::vector<ComponentProperty> props;

        props.push_back (ComponentProperty {
            .name = Properties::directionId,
            .type = ComponentProperty::Type::enumeration,
            .defaultValue = directions[0],
            .isInherited = false,
            .enumValues = directions,
            .description = "The direction the children are laid out in." });

        props.push_back (ComponentProperty {
            .name = Properties::wrapId,
            .type = ComponentProperty::Type::enumeration,
            .defaultValue = wraps[0],
            .isInherited = false,
            .enumValues = wraps,
            .description = "Whether the children wrap onto more lines." });

        props.push_back (ComponentProperty {
            .name = Properties::justifyId,
            .type = ComponentProperty::Type::enumeration,
            .defaultValue = justifications[0],
            .isInherited = false,
            .enumValues = justifications,
            .description = "How the children are distributed along the main axis." });

        props.push_back (ComponentProperty {
            .name = Properties::alignId,
            .type = ComponentProperty::Type::enumeration,
            .defaultValue = flexAlignments[3],
            .isInherited = false,
            .enumValues = flexAlignments,
            .description = "How the children are placed on the cross axis." });

        return ComponentTypeInfo { Identifier ("FlexBox"), &LayoutComponent::getClassTypeInfo(), std::move (props), {}, []
                                   {
            return std::make_unique<FlexBoxComponent>();
                                  } };
    }();

    return info;
}

const ComponentTypeInfo& FlexBoxComponent::getTypeInfo() const
{
    return getClassTypeInfo();
}

//==============================================================================

void FlexBoxComponent::performLayout (Rectangle<float> area)
{
    FlexBox flexBox;

    flexBox.flexDirection = getDirection();
    flexBox.flexWrap = toFlexEnum (findProperty (Properties::wrapId).toString(), wraps, FlexBox::Wrap::noWrap);
    flexBox.justifyContent = toFlexEnum (findProperty (Properties::justifyId).toString(), justifications, FlexBox::JustifyContent::flexStart);
    flexBox.alignItems = toFlexEnum (findProperty (Properties::alignId).toString(), flexAlignments, FlexBox::AlignItems::stretch);
    flexBox.gap = getGap();

    for (auto* child : getLayoutChildren())
    {
        FlexItem item (*child);

        item.flexGrow = jmax (0.0f, getItemNumber (*child, Properties::flexGrowId));
        item.flexShrink = jmax (0.0f, getItemNumber (*child, Properties::flexShrinkId));
        item.flexBasis = getItemNumber (*child, Properties::flexBasisId);
        item.order = getItemInteger (*child, Properties::orderId);
        item.alignSelf = toFlexAlignSelf (getItemString (*child, Properties::alignSelfId));
        item.margin = getItemMargin (*child);
        item.width = getItemNumber (*child, Properties::preferredWidthId);
        item.height = getItemNumber (*child, Properties::preferredHeightId);
        item.minWidth = getItemNumber (*child, Properties::minWidthId);
        item.maxWidth = getItemNumber (*child, Properties::maxWidthId);
        item.minHeight = getItemNumber (*child, Properties::minHeightId);
        item.maxHeight = getItemNumber (*child, Properties::maxHeightId);

        flexBox.items.add (item);
    }

    flexBox.performLayout (area);
}

//==============================================================================

RowComponent::RowComponent()
{
    setDirection (FlexBox::Direction::row);
}

RowComponent::RowComponent (StringRef componentID)
    : FlexBoxComponent (componentID)
{
    setDirection (FlexBox::Direction::row);
}

const ComponentTypeInfo& RowComponent::getClassTypeInfo()
{
    static const ComponentTypeInfo info { Identifier ("Row"), &FlexBoxComponent::getClassTypeInfo(), {}, {}, []
                                          {
        return std::make_unique<RowComponent>();
                                         } };

    return info;
}

const ComponentTypeInfo& RowComponent::getTypeInfo() const
{
    return getClassTypeInfo();
}

//==============================================================================

ColumnComponent::ColumnComponent()
{
    setDirection (FlexBox::Direction::column);
}

ColumnComponent::ColumnComponent (StringRef componentID)
    : FlexBoxComponent (componentID)
{
    setDirection (FlexBox::Direction::column);
}

const ComponentTypeInfo& ColumnComponent::getClassTypeInfo()
{
    static const ComponentTypeInfo info { Identifier ("Column"), &FlexBoxComponent::getClassTypeInfo(), {}, {}, []
                                          {
        return std::make_unique<ColumnComponent>();
                                         } };

    return info;
}

const ComponentTypeInfo& ColumnComponent::getTypeInfo() const
{
    return getClassTypeInfo();
}

} // namespace yup
