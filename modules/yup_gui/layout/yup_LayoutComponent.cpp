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
const Identifier LayoutComponent::Properties::fitContentId ("fitContent");
const Identifier LayoutComponent::Properties::fitWidthId ("fitWidth");
const Identifier LayoutComponent::Properties::fitHeightId ("fitHeight");
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
namespace
{

/* The layouts the batches of a thread hold back: how many batches there are, and the components to lay out when the last ends. */
struct HeldLayouts
{
    int numBatches = 0;
    std::vector<WeakReference<Component>> waiting;
};

HeldLayouts& getHeldLayouts()
{
    thread_local HeldLayouts held;
    return held;
}

} // namespace

LayoutComponent::ScopedLayoutBatch::ScopedLayoutBatch()
{
    ++getHeldLayouts().numBatches;
}

LayoutComponent::ScopedLayoutBatch::~ScopedLayoutBatch()
{
    auto& held = getHeldLayouts();

    if (--held.numBatches > 0)
        return;

    for (const auto& waiting : std::exchange (held.waiting, {}))
        if (auto* layout = static_cast<LayoutComponent*> (waiting.get()); layout != nullptr && layout->isLayoutHeld)
            layout->updateLayout();
}

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

void LayoutComponent::setFitContent (bool shouldFitContent)
{
    setProperty (Properties::fitContentId, shouldFitContent);
}

bool LayoutComponent::getFitContent() const
{
    return findPropertyAs<bool> (Properties::fitContentId);
}

void LayoutComponent::setFitAxes (bool shouldFitWidth, bool shouldFitHeight)
{
    setProperty (Properties::fitWidthId, shouldFitWidth);
    setProperty (Properties::fitHeightId, shouldFitHeight);
}

//==============================================================================

void LayoutComponent::updateLayout()
{
    // Asked again while it lays out, by a child that fitted its content: it lays out once more after
    if (isPerformingLayout)
    {
        isLayoutPending = true;
        return;
    }

    if (auto& held = getHeldLayouts(); held.numBatches > 0)
    {
        if (! std::exchange (isLayoutHeld, true))
            held.waiting.emplace_back (this);

        return;
    }

    isLayoutHeld = false;

    const ScopedValueSetter<bool> scope (isPerformingLayout, true);

    // A pass only follows the sizes the one before changed, so a few settle them
    for (int pass = 0; pass < 4; ++pass)
    {
        isLayoutPending = false;

        // Its parent lays out with the new preferred size here, which gives the container its bounds
        fitToContent();

        performLayout (getLocalBounds().reduced (getPadding()));

        if (! isLayoutPending)
            break;
    }
}

void LayoutComponent::fitToContent()
{
    if (! getFitContent())
        return;

    const auto content = measureContent();
    if (! content.has_value())
        return;

    const auto padding = getPadding();
    const auto width = static_cast<double> (content->getWidth() + padding.getLeft() + padding.getRight());
    const auto height = static_cast<double> (content->getHeight() + padding.getTop() + padding.getBottom());

    if (findPropertyAs<bool> (Properties::fitWidthId) && findPropertyAs<double> (Properties::preferredWidthId) != width)
        setProperty (Properties::preferredWidthId, width);

    if (findPropertyAs<bool> (Properties::fitHeightId) && findPropertyAs<double> (Properties::preferredHeightId) != height)
        setProperty (Properties::preferredHeightId, height);
}

//==============================================================================

const ComponentTypeInfo& LayoutComponent::getClassTypeInfo()
{
    using Type = ComponentProperty::Type;
    using LayoutEffect = ComponentProperty::LayoutEffect;

    static const auto info = ComponentTypeBuilder<LayoutComponent> (Identifier ("Layout"))
                                 .layoutProperty (Properties::paddingId, Type::any, 0.0, LayoutEffect::own, "The space kept inside the bounds: a number, or 1, 2 or 4 values (top right bottom left).")
                                 .layoutProperty (Properties::gapId, Type::number, 0.0, LayoutEffect::own, "The space kept between the children.")
                                 .layoutProperty (Properties::fitContentId, Type::boolean, false, LayoutEffect::own, "Whether the container sets its preferred size to what its children need, plus its padding.")
                                 .layoutProperty (Properties::fitWidthId, Type::boolean, true, LayoutEffect::own, "Whether fitContent fits the width: without it the container keeps its preferredWidth.")
                                 .layoutProperty (Properties::fitHeightId, Type::boolean, true, LayoutEffect::own, "Whether fitContent fits the height: without it the container keeps its preferredHeight.")
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
        if (auto* child = getChildComponent (i);
            child != nullptr && child->isVisible() && ! child->isExcludedFromParentLayout())
        {
            children.push_back (child);
        }
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

Size<float> LayoutComponent::getMeasuredItemSize (const Component& child)
{
    const auto preferredWidth = getItemNumber (child, Properties::preferredWidthId);
    const auto preferredHeight = getItemNumber (child, Properties::preferredHeightId);
    const auto margin = getItemMargin (child);

    return { (preferredWidth >= 0.0f ? preferredWidth : child.getWidth()) + margin.getLeft() + margin.getRight(),
             (preferredHeight >= 0.0f ? preferredHeight : child.getHeight()) + margin.getTop() + margin.getBottom() };
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
