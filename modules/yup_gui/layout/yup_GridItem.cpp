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

GridItem::GridItem (Component& c)
    : associatedComponent (&c)
{
}

GridItem::GridItem (Component* c)
    : associatedComponent (c)
{
}

GridItem GridItem::withColumn (int newColumn) const
{
    auto copy = *this;
    copy.column = newColumn;
    return copy;
}

GridItem GridItem::withRow (int newRow) const
{
    auto copy = *this;
    copy.row = newRow;
    return copy;
}

GridItem GridItem::withColumnSpan (int newSpan) const
{
    auto copy = *this;
    copy.columnSpan = newSpan;
    return copy;
}

GridItem GridItem::withRowSpan (int newSpan) const
{
    auto copy = *this;
    copy.rowSpan = newSpan;
    return copy;
}

GridItem GridItem::withMargin (float newMargin) const
{
    auto copy = *this;
    copy.marginLeft = newMargin;
    copy.marginRight = newMargin;
    copy.marginTop = newMargin;
    copy.marginBottom = newMargin;
    return copy;
}

GridItem GridItem::withWidth (float newWidth) const
{
    auto copy = *this;
    copy.width = newWidth;
    return copy;
}

GridItem GridItem::withHeight (float newHeight) const
{
    auto copy = *this;
    copy.height = newHeight;
    return copy;
}

GridItem GridItem::withWidthPercent (float newWidthPercent) const
{
    auto copy = *this;
    copy.widthPercent = newWidthPercent;
    return copy;
}

GridItem GridItem::withHeightPercent (float newHeightPercent) const
{
    auto copy = *this;
    copy.heightPercent = newHeightPercent;
    return copy;
}

GridItem GridItem::withMinWidth (float newMinWidth) const
{
    auto copy = *this;
    copy.minWidth = newMinWidth;
    return copy;
}

GridItem GridItem::withMinHeight (float newMinHeight) const
{
    auto copy = *this;
    copy.minHeight = newMinHeight;
    return copy;
}

GridItem GridItem::withMaxWidth (float newMaxWidth) const
{
    auto copy = *this;
    copy.maxWidth = newMaxWidth;
    return copy;
}

GridItem GridItem::withMaxHeight (float newMaxHeight) const
{
    auto copy = *this;
    copy.maxHeight = newMaxHeight;
    return copy;
}

GridItem GridItem::withJustifySelf (AlignSelf newJustifySelf) const
{
    auto copy = *this;
    copy.justifySelf = newJustifySelf;
    return copy;
}

GridItem GridItem::withAlignSelf (AlignSelf newAlignSelf) const
{
    auto copy = *this;
    copy.alignSelf = newAlignSelf;
    return copy;
}

} // namespace yup
