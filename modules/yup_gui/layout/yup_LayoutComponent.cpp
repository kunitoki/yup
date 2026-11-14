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

const Identifier LayoutComponent::Properties::paddingId ("padding");
const Identifier LayoutComponent::Properties::gapId ("gap");
const Identifier LayoutComponent::Properties::directionId ("direction");
const Identifier LayoutComponent::Properties::wrapId ("wrap");
const Identifier LayoutComponent::Properties::justifyId ("justify");
const Identifier LayoutComponent::Properties::alignId ("align");
const Identifier LayoutComponent::Properties::columnsId ("columns");
const Identifier LayoutComponent::Properties::rowsId ("rows");
const Identifier LayoutComponent::Properties::areasId ("areas");

const Identifier LayoutComponent::Properties::flexGrowId ("flexGrow");
const Identifier LayoutComponent::Properties::flexShrinkId ("flexShrink");
const Identifier LayoutComponent::Properties::flexBasisId ("flexBasis");
const Identifier LayoutComponent::Properties::orderId ("order");
const Identifier LayoutComponent::Properties::alignSelfId ("alignSelf");
const Identifier LayoutComponent::Properties::justifySelfId ("justifySelf");
const Identifier LayoutComponent::Properties::marginId ("margin");
const Identifier LayoutComponent::Properties::preferredWidthId ("preferredWidth");
const Identifier LayoutComponent::Properties::preferredHeightId ("preferredHeight");
const Identifier LayoutComponent::Properties::minWidthId ("minWidth");
const Identifier LayoutComponent::Properties::maxWidthId ("maxWidth");
const Identifier LayoutComponent::Properties::minHeightId ("minHeight");
const Identifier LayoutComponent::Properties::maxHeightId ("maxHeight");
const Identifier LayoutComponent::Properties::gridAreaId ("gridArea");
const Identifier LayoutComponent::Properties::gridColumnId ("gridColumn");
const Identifier LayoutComponent::Properties::gridRowId ("gridRow");
const Identifier LayoutComponent::Properties::gridColumnSpanId ("gridColumnSpan");
const Identifier LayoutComponent::Properties::gridRowSpanId ("gridRowSpan");

//==============================================================================

void LayoutComponent::setPadding (const EdgeSpacing<float>& newPadding)
{
    setProperty (Properties::paddingId, String (newPadding.getTop()) + " " + String (newPadding.getRight()) + " " + String (newPadding.getBottom()) + " " + String (newPadding.getLeft()));
}

EdgeSpacing<float> LayoutComponent::getPadding() const
{
    return parseSpacing (findProperty (Properties::paddingId));
}

void LayoutComponent::setGap (float newGap)
{
    setProperty (Properties::gapId, newGap);
}

float LayoutComponent::getGap() const
{
    return jmax (0.0f, findPropertyAs<float> (Properties::gapId));
}

//==============================================================================

void LayoutComponent::updateLayout()
{
    if (isPerformingLayout)
        return;

    const ScopedValueSetter<bool> scope (isPerformingLayout, true);

    const auto padding = getPadding();
    auto area = getLocalBounds();

    area = Rectangle<float> (area.getX() + padding.getLeft(),
                             area.getY() + padding.getTop(),
                             jmax (0.0f, area.getWidth() - padding.getLeft() - padding.getRight()),
                             jmax (0.0f, area.getHeight() - padding.getTop() - padding.getBottom()));

    performLayout (area);
}

//==============================================================================

const ComponentTypeInfo& LayoutComponent::getClassTypeInfo()
{
    static const ComponentTypeInfo info = []
    {
        std::vector<ComponentProperty> props;

        props.push_back (ComponentProperty {
            .name = Properties::paddingId,
            .type = ComponentProperty::Type::any,
            .defaultValue = 0.0,
            .isInherited = false,
            .description = "The space kept inside the bounds: a number, or 1, 2 or 4 values (top right bottom left)." });

        props.push_back (ComponentProperty {
            .name = Properties::gapId,
            .type = ComponentProperty::Type::number,
            .defaultValue = 0.0,
            .isInherited = false,
            .description = "The space kept between the children." });

        return ComponentTypeInfo { Identifier ("Layout"), &Component::getClassTypeInfo(), std::move (props), {}, nullptr };
    }();

    return info;
}

const ComponentTypeInfo& LayoutComponent::getDefaultTypeInfo() const
{
    return getClassTypeInfo();
}

void LayoutComponent::resized()
{
    updateLayout();
}

void LayoutComponent::childrenChanged()
{
    updateLayout();
}

void LayoutComponent::childLayoutChanged ([[maybe_unused]] Component& child)
{
    updateLayout();
}

void LayoutComponent::styleChanged()
{
    Component::styleChanged();

    updateLayout();
}

//==============================================================================

std::vector<Component*> LayoutComponent::getLayoutChildren() const
{
    std::vector<Component*> children;

    for (int i = 0; i < getNumChildComponents(); ++i)
    {
        if (auto* child = getChildComponent (i); child != nullptr && child->isVisible())
            children.push_back (child);
    }

    return children;
}

float LayoutComponent::getItemNumber (const Component& child, const Identifier& name)
{
    return child.findPropertyAs<float> (name);
}

int LayoutComponent::getItemInteger (const Component& child, const Identifier& name)
{
    return child.findPropertyAs<int> (name);
}

String LayoutComponent::getItemString (const Component& child, const Identifier& name)
{
    return child.findProperty (name).toString();
}

EdgeSpacing<float> LayoutComponent::getItemMargin (const Component& child)
{
    return parseSpacing (child.findProperty (Properties::marginId));
}

EdgeSpacing<float> LayoutComponent::parseSpacing (const var& value)
{
    if (value.isVoid() || value.isUndefined())
        return {};

    if (! value.isString())
    {
        const auto amount = static_cast<float> (static_cast<double> (value));
        return EdgeSpacing<float> (amount);
    }

    const auto tokens = splitTokens (value.toString());

    const auto valueAt = [&tokens] (int index)
    {
        return tokens[index].getFloatValue();
    };

    switch (tokens.size())
    {
        case 0:
            return {};

        case 1:
            return EdgeSpacing<float> (valueAt (0));

        case 2:
            return EdgeSpacing<float> (valueAt (1), valueAt (0));

        case 3:
            return EdgeSpacing<float> (valueAt (1), valueAt (0), valueAt (1), valueAt (2));

        default:
            return EdgeSpacing<float> (valueAt (3), valueAt (0), valueAt (1), valueAt (2));
    }
}

StringArray LayoutComponent::splitTokens (const String& text)
{
    auto tokens = StringArray::fromTokens (text.trim(), " \t,", "");
    tokens.removeEmptyStrings();
    tokens.trim();
    return tokens;
}

} // namespace yup
