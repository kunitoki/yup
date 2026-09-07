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

} // namespace yup
