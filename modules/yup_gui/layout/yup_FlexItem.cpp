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

//==============================================================================
FlexItem::FlexItem (Component& c)
    : associatedComponent (&c)
{
}

FlexItem::FlexItem (Component* c)
    : associatedComponent (c)
{
}

FlexItem::FlexItem (float w, float h)
    : width (w)
    , height (h)
{
}

FlexItem::FlexItem (Component& c, float w, float h)
    : associatedComponent (&c)
    , width (w)
    , height (h)
{
}

FlexItem::FlexItem (Component* c, float w, float h)
    : associatedComponent (c)
    , width (w)
    , height (h)
{
}

FlexItem FlexItem::withFlex (float newFlexGrow) const
{
    auto copy = *this;
    copy.flexGrow = newFlexGrow;
    return copy;
}

FlexItem FlexItem::withWidth (float newWidth) const
{
    auto copy = *this;
    copy.width = newWidth;
    return copy;
}

FlexItem FlexItem::withHeight (float newHeight) const
{
    auto copy = *this;
    copy.height = newHeight;
    return copy;
}

FlexItem FlexItem::withWidthPercent (float newWidthPercent) const
{
    auto copy = *this;
    copy.widthPercent = newWidthPercent;
    return copy;
}

FlexItem FlexItem::withHeightPercent (float newHeightPercent) const
{
    auto copy = *this;
    copy.heightPercent = newHeightPercent;
    return copy;
}

FlexItem FlexItem::withMinWidth (float newMinWidth) const
{
    auto copy = *this;
    copy.minWidth = newMinWidth;
    return copy;
}

FlexItem FlexItem::withMinHeight (float newMinHeight) const
{
    auto copy = *this;
    copy.minHeight = newMinHeight;
    return copy;
}

FlexItem FlexItem::withMaxWidth (float newMaxWidth) const
{
    auto copy = *this;
    copy.maxWidth = newMaxWidth;
    return copy;
}

FlexItem FlexItem::withMaxHeight (float newMaxHeight) const
{
    auto copy = *this;
    copy.maxHeight = newMaxHeight;
    return copy;
}

FlexItem FlexItem::withMargin (float newMargin) const
{
    auto copy = *this;
    copy.marginLeft = newMargin;
    copy.marginRight = newMargin;
    copy.marginTop = newMargin;
    copy.marginBottom = newMargin;
    return copy;
}

FlexItem FlexItem::withAlignSelf (AlignSelf newAlignSelf) const
{
    auto copy = *this;
    copy.alignSelf = newAlignSelf;
    return copy;
}

FlexItem FlexItem::withBaseline (float newBaseline) const
{
    auto copy = *this;
    copy.baseline = newBaseline;
    return copy;
}

FlexItem FlexItem::withOrder (int newOrder) const
{
    auto copy = *this;
    copy.order = newOrder;
    return copy;
}

} // namespace yup
