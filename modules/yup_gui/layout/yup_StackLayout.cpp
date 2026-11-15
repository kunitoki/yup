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

/* Places one axis of an item inside the area left by its margin. */
void resolveAxis (float areaStart, float areaSize, float marginStart, float marginEnd, float requestedSize, StackLayout::Align align, float& position, float& size)
{
    const auto availableStart = areaStart + marginStart;
    const auto available = std::max (0.0f, areaSize - marginStart - marginEnd);

    if (align == StackLayout::Align::stretch && requestedSize < 0.0f)
    {
        position = availableStart;
        size = available;
        return;
    }

    size = requestedSize >= 0.0f ? std::min (requestedSize, available) : available;

    switch (align)
    {
        case StackLayout::Align::end:    position = availableStart + available - size; break;
        case StackLayout::Align::center: position = availableStart + (available - size) * 0.5f; break;

        default:                         position = availableStart; break;
    }
}

} // namespace

//==============================================================================

StackLayout::Item::Item (Component& component)
    : associatedComponent (&component)
{
}

StackLayout::Item::Item (Component* component)
    : associatedComponent (component)
{
}

StackLayout::Item StackLayout::Item::withSize (float newWidth, float newHeight) const
{
    auto copy = *this;
    copy.width = newWidth;
    copy.height = newHeight;
    return copy;
}

StackLayout::Item StackLayout::Item::withWidth (float newWidth) const
{
    auto copy = *this;
    copy.width = newWidth;
    return copy;
}

StackLayout::Item StackLayout::Item::withHeight (float newHeight) const
{
    auto copy = *this;
    copy.height = newHeight;
    return copy;
}

StackLayout::Item StackLayout::Item::withAlign (Align newAlignX, Align newAlignY) const
{
    auto copy = *this;
    copy.alignX = newAlignX;
    copy.alignY = newAlignY;
    return copy;
}

StackLayout::Item StackLayout::Item::withMargin (float newMargin) const
{
    return withMargin (EdgeSpacing<float> (newMargin));
}

StackLayout::Item StackLayout::Item::withMargin (const EdgeSpacing<float>& newMargin) const
{
    auto copy = *this;
    copy.margin = newMargin;
    return copy;
}

//==============================================================================

void StackLayout::setPadding (float newPadding) noexcept
{
    padding = EdgeSpacing<float> (newPadding);
}

void StackLayout::setPadding (float horizontal, float vertical) noexcept
{
    padding = EdgeSpacing<float> (horizontal, vertical);
}

void StackLayout::setPadding (const EdgeSpacing<float>& newPadding) noexcept
{
    padding = newPadding;
}

//==============================================================================

void StackLayout::performLayout (Rectangle<float> targetArea)
{
    jassert (padding.getLeft() >= 0.0f && padding.getRight() >= 0.0f);
    jassert (padding.getTop() >= 0.0f && padding.getBottom() >= 0.0f);

    const auto contentArea = targetArea.reduced (std::max (0.0f, padding.getLeft()),
                                                 std::max (0.0f, padding.getTop()),
                                                 std::max (0.0f, padding.getRight()),
                                                 std::max (0.0f, padding.getBottom()));

    for (auto& item : items)
    {
        float x = 0.0f, y = 0.0f, width = 0.0f, height = 0.0f;

        const auto itemAlignX = item.alignX == Align::automatic ? alignX : item.alignX;
        const auto itemAlignY = item.alignY == Align::automatic ? alignY : item.alignY;

        resolveAxis (contentArea.getX(), contentArea.getWidth(), item.margin.getLeft(), item.margin.getRight(), item.width, itemAlignX, x, width);
        resolveAxis (contentArea.getY(), contentArea.getHeight(), item.margin.getTop(), item.margin.getBottom(), item.height, itemAlignY, y, height);

        if (item.associatedComponent != nullptr)
            item.associatedComponent->setBounds (Rectangle<float> (x, y, width, height));
    }
}

void StackLayout::performLayout (Rectangle<int> targetArea)
{
    performLayout (targetArea.to<float>());
}

} // namespace yup
