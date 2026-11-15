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
    static const ComponentTypeInfo info = []
    {
        std::vector<ComponentProperty> props;

        props.push_back (ComponentProperty {
            .name = Properties::directionId,
            .type = ComponentProperty::Type::enumeration,
            .defaultValue = flexDirections.names[0],
            .isInherited = false,
            .enumValues = flexDirections.names,
            .description = "The direction the children are laid out in." });

        props.push_back (ComponentProperty {
            .name = Properties::wrapId,
            .type = ComponentProperty::Type::enumeration,
            .defaultValue = flexWraps.names[0],
            .isInherited = false,
            .enumValues = flexWraps.names,
            .description = "Whether the children wrap onto more lines." });

        props.push_back (ComponentProperty {
            .name = Properties::justifyId,
            .type = ComponentProperty::Type::enumeration,
            .defaultValue = flexJustifications.names[0],
            .isInherited = false,
            .enumValues = flexJustifications.names,
            .description = "How the children are distributed along the main axis." });

        props.push_back (ComponentProperty {
            .name = Properties::alignId,
            .type = ComponentProperty::Type::enumeration,
            .defaultValue = flexAlignments.names[3],
            .isInherited = false,
            .enumValues = flexAlignments.names,
            .description = "How the children are placed on the cross axis." });

        return ComponentTypeInfo { Identifier ("FlexBox"), &LayoutComponent::getClassTypeInfo(), std::move (props), {}, []
                                   {
            return std::make_unique<FlexBoxComponent>();
                                  } };
    }();

    return info;
}

const ComponentTypeInfo& FlexBoxComponent::getDefaultTypeInfo() const
{
    return getClassTypeInfo();
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

const ComponentTypeInfo& RowComponent::getDefaultTypeInfo() const
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

const ComponentTypeInfo& ColumnComponent::getDefaultTypeInfo() const
{
    return getClassTypeInfo();
}

} // namespace yup
