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

std::array<var, 9> LayoutComponent::captureContainerProperties() const
{
    return {
        findProperty (Properties::paddingId),
        findProperty (Properties::gapId),
        findProperty (Properties::directionId),
        findProperty (Properties::wrapId),
        findProperty (Properties::justifyId),
        findProperty (Properties::alignId),
        findProperty (Properties::columnsId),
        findProperty (Properties::rowsId),
        findProperty (Properties::areasId)
    };
}

void LayoutComponent::updateLayout()
{
    if (isPerformingLayout)
        return;

    const ScopedValueSetter<bool> scope (isPerformingLayout, true);

    lastContainerProperties = captureContainerProperties();
    hasLayoutSnapshot = true;

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

namespace
{

bool sameLayoutPropertyValue (const var& a, const var& b)
{
    if (a.isArray() && b.isArray())
    {
        const auto* arrayA = a.getArray();
        const auto* arrayB = b.getArray();

        if (arrayA->size() != arrayB->size())
            return false;

        for (int i = 0; i < arrayA->size(); ++i)
        {
            if (! sameLayoutPropertyValue (arrayA->getReference (i), arrayB->getReference (i)))
                return false;
        }

        return true;
    }

    return a.equalsWithSameType (b);
}

bool sameLayoutProperties (const std::array<var, 9>& a, const std::array<var, 9>& b)
{
    for (std::size_t i = 0; i < a.size(); ++i)
    {
        if (! sameLayoutPropertyValue (a[i], b[i]))
            return false;
    }

    return true;
}

} // namespace

void LayoutComponent::styleChanged()
{
    Component::styleChanged();

    if (hasLayoutSnapshot && usesOnlyStandardLayoutProperties() && sameLayoutProperties (captureContainerProperties(), lastContainerProperties))
        return;

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
    if (value.isVoid() || value.isUndefined() || value.isBool())
        return {};

    if (! value.isString())
    {
        const auto amount = static_cast<float> (static_cast<double> (value));
        return std::isfinite (amount) ? EdgeSpacing<float> (amount) : EdgeSpacing<float>();
    }

    const auto tokens = splitTokens (value.toString());
    if (tokens.isEmpty() || tokens.size() > 4)
        return {};

    float values[4] = {};
    for (int i = 0; i < tokens.size(); ++i)
    {
        const auto parsed = detail::parseStrictFloat (tokens[i], true, true);
        if (! parsed.has_value())
            return {};

        values[i] = *parsed;
    }

    switch (tokens.size())
    {
        case 1:
            return EdgeSpacing<float> (values[0]);

        case 2:
            return EdgeSpacing<float> (values[1], values[0]);

        case 3:
            return EdgeSpacing<float> (values[1], values[0], values[1], values[2]);

        case 4:
            return EdgeSpacing<float> (values[3], values[0], values[1], values[2]);

        default:
            return {};
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
