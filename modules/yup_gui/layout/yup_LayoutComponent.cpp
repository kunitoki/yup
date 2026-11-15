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
    setProperty (Properties::paddingId, VariantConverter<EdgeSpacing<float>>::toVar (newPadding));
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
    performLayout (getLocalBounds().reduced (getPadding()));
}

//==============================================================================

const ComponentTypeInfo& LayoutComponent::getClassTypeInfo()
{
    using Type = ComponentProperty::Type;
    using LayoutEffect = ComponentProperty::LayoutEffect;

    static const auto info = ComponentTypeBuilder<LayoutComponent> (Identifier ("Layout"))
                                 .layoutProperty (Properties::paddingId, Type::any, 0.0, LayoutEffect::own, "The space kept inside the bounds: a number, or 1, 2 or 4 values (top right bottom left).")
                                 .layoutProperty (Properties::gapId, Type::number, 0.0, LayoutEffect::own, "The space kept between the children.")
                                 .build();

    return info;
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

void LayoutComponent::layoutPropertyChanged()
{
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

    if (value.isArray())
        return VariantConverter<EdgeSpacing<float>>::fromVar (value);

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
