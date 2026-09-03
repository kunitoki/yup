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
FlexBox::FlexBox (Direction d)
    : flexDirection (d)
{
}

FlexBox::FlexBox (Direction d, Wrap w, AlignItems ai, JustifyContent jc, AlignContent ac)
    : flexDirection (d)
    , flexWrap (w)
    , alignItems (ai)
    , justifyContent (jc)
    , alignContent (ac)
{
}

//==============================================================================
static bool isRowDirection (FlexBox::Direction direction)
{
    return direction == FlexBox::Direction::row || direction == FlexBox::Direction::rowReverse;
}

static bool isReverseDirection (FlexBox::Direction direction)
{
    return direction == FlexBox::Direction::rowReverse || direction == FlexBox::Direction::columnReverse;
}

//==============================================================================
void FlexBox::performLayout (Rectangle<float> targetArea)
{
    if (items.size() == 0)
        return;

    // Sort items by order
    Array<FlexItem*> sortedItems;
    sortedItems.ensureStorageAllocated (items.size());

    for (auto& item : items)
        sortedItems.add (&item);

    std::sort (sortedItems.begin(), sortedItems.end(), [] (const FlexItem* a, const FlexItem* b)
    {
        return a->order < b->order;
    });

    const bool isRow = isRowDirection (flexDirection);
    const bool isReverse = isReverseDirection (flexDirection);

    const float containerMainSize = isRow ? targetArea.getWidth() : targetArea.getHeight();
    const float containerCrossSize = isRow ? targetArea.getHeight() : targetArea.getWidth();
    const float containerMainStart = isRow ? targetArea.getX() : targetArea.getY();
    const float containerCrossStart = isRow ? targetArea.getY() : targetArea.getX();

    // Build lines
    Array<FlexBox::LineInfo> lines;
    FlexBox::LineInfo currentLine;
    float currentMainSize = 0.0f;

    for (int i = 0; i < sortedItems.size(); ++i)
    {
        auto* item = sortedItems.getUnchecked (i);

        float itemMainSize = isRow ? item->width : item->height;
        float itemMainMarginStart = isRow ? item->marginLeft : item->marginTop;
        float itemMainMarginEnd = isRow ? item->marginRight : item->marginBottom;

        // If flexBasis is set, use it as the initial main size
        if (item->flexBasis > 0.0f)
            itemMainSize = item->flexBasis;

        const float totalItemMainSize = itemMainSize + itemMainMarginStart + itemMainMarginEnd;

        if (flexWrap != Wrap::noWrap && ! currentLine.items.isEmpty()
            && currentMainSize + totalItemMainSize > containerMainSize)
        {
            // Start a new line
            lines.add (currentLine);
            currentLine = {};
            currentMainSize = 0.0f;
        }

        currentLine.items.add (item);
        currentMainSize += totalItemMainSize;
    }

    if (! currentLine.items.isEmpty())
        lines.add (currentLine);

    // Calculate cross sizes for lines
    for (auto& line : lines)
    {
        float maxCrossSize = 0.0f;

        for (auto* item : line.items)
        {
            float itemCrossSize = isRow ? item->height : item->width;
            float crossMarginStart = isRow ? item->marginTop : item->marginLeft;
            float crossMarginEnd = isRow ? item->marginBottom : item->marginRight;
            maxCrossSize = std::max (maxCrossSize, itemCrossSize + crossMarginStart + crossMarginEnd);
        }

        line.crossSize = maxCrossSize;
        line.totalMainSize = 0.0f;

        for (auto* item : line.items)
        {
            float itemMainSize = isRow ? item->width : item->height;
            if (item->flexBasis > 0.0f)
                itemMainSize = item->flexBasis;

            float mainMarginStart = isRow ? item->marginLeft : item->marginTop;
            float mainMarginEnd = isRow ? item->marginRight : item->marginBottom;
            line.totalMainSize += itemMainSize + mainMarginStart + mainMarginEnd;
        }
    }

    // Calculate total cross size
    float totalCrossSize = 0.0f;
    for (const auto& line : lines)
        totalCrossSize += line.crossSize + gap;

    if (lines.size() > 0)
        totalCrossSize -= gap;

    // Align lines on cross axis
    float crossOffset;

    switch (alignContent)
    {
        case AlignContent::flexStart:
            crossOffset = 0.0f;
            break;
        case AlignContent::flexEnd:
            crossOffset = containerCrossSize - totalCrossSize;
            break;
        case AlignContent::center:
            crossOffset = (containerCrossSize - totalCrossSize) / 2.0f;
            break;
        case AlignContent::spaceBetween:
            crossOffset = 0.0f;
            break;
        case AlignContent::spaceAround:
            crossOffset = (containerCrossSize - totalCrossSize) / (float) (lines.size() + 1);
            break;
        case AlignContent::stretch:
            crossOffset = 0.0f;
            break;
    }

    // Position items
    float currentCrossPos = containerCrossStart + crossOffset;

    for (int lineIdx = 0; lineIdx < lines.size(); ++lineIdx)
    {
        auto& line = lines.getReference (lineIdx);

        // Calculate cross size for this line
        float lineCrossSize = line.crossSize;

        if (alignContent == AlignContent::stretch && lines.size() > 1)
            lineCrossSize = (containerCrossSize - totalCrossSize) / (float) lines.size() + line.crossSize;

        if (alignContent == AlignContent::spaceBetween && lines.size() > 1)
        {
            if (lineIdx == 0)
                currentCrossPos = containerCrossStart;
            else if (lineIdx == lines.size() - 1)
                currentCrossPos = containerCrossStart + containerCrossSize - lineCrossSize;
            else
                currentCrossPos = containerCrossStart + (containerCrossSize - totalCrossSize) * (float) lineIdx / (float) (lines.size() - 1);
        }

        // Calculate flex-grow
        float totalFlexGrow = 0.0f;
        float totalFixedSize = 0.0f;

        for (auto* item : line.items)
        {
            float itemMainSize = isRow ? item->width : item->height;
            float mainMarginStart = isRow ? item->marginLeft : item->marginTop;
            float mainMarginEnd = isRow ? item->marginRight : item->marginBottom;

            if (item->flexBasis > 0.0f)
                itemMainSize = item->flexBasis;

            totalFixedSize += itemMainSize + mainMarginStart + mainMarginEnd;

            if (item->flexGrow > 0.0f)
                totalFlexGrow += item->flexGrow;
        }

        float extraSpace = containerMainSize - totalFixedSize;

        // Calculate main axis offset
        float mainOffset;

        switch (justifyContent)
        {
            case JustifyContent::flexStart:
                mainOffset = 0.0f;
                break;
            case JustifyContent::flexEnd:
                mainOffset = extraSpace;
                break;
            case JustifyContent::center:
                mainOffset = extraSpace / 2.0f;
                break;
            case JustifyContent::spaceBetween:
                mainOffset = 0.0f;
                break;
            case JustifyContent::spaceAround:
                mainOffset = extraSpace / (float) (line.items.size() + 1);
                break;
        }

        float currentMainPos = containerMainStart + mainOffset;
        int gapCount = 0;

        for (auto* item : line.items)
        {
            float itemMainSize = isRow ? item->width : item->height;
            float itemCrossSize = isRow ? item->height : item->width;
            float mainMarginStart = isRow ? item->marginLeft : item->marginTop;
            float mainMarginEnd = isRow ? item->marginRight : item->marginBottom;
            float crossMarginStart = isRow ? item->marginTop : item->marginLeft;
            float crossMarginEnd = isRow ? item->marginBottom : item->marginRight;

            if (item->flexBasis > 0.0f)
                itemMainSize = item->flexBasis;

            // Apply flex-grow
            if (totalFlexGrow > 0 && item->flexGrow > 0)
                itemMainSize += extraSpace * item->flexGrow / totalFlexGrow;

            // Apply min/max constraints
            float constraintMinMainSize = isRow ? item->minWidth : item->minHeight;
            float constraintMaxMainSize = isRow ? item->maxWidth : item->maxHeight;

            if (constraintMinMainSize >= 0)
                itemMainSize = std::max (itemMainSize, constraintMinMainSize);
            if (constraintMaxMainSize >= 0)
                itemMainSize = std::min (itemMainSize, constraintMaxMainSize);

            // Align on cross axis
            FlexItem::AlignSelf align = item->alignSelf;
            if (align == FlexItem::AlignSelf::autoAlign)
            {
                switch (alignItems)
                {
                    case AlignItems::flexStart:
                        align = FlexItem::AlignSelf::flexStart;
                        break;
                    case AlignItems::flexEnd:
                        align = FlexItem::AlignSelf::flexEnd;
                        break;
                    case AlignItems::center:
                        align = FlexItem::AlignSelf::center;
                        break;
                    case AlignItems::stretch:
                        align = FlexItem::AlignSelf::stretch;
                        break;
                }
            }

            float itemCrossPos;

            if (align == FlexItem::AlignSelf::stretch)
            {
                itemCrossSize = lineCrossSize - crossMarginStart - crossMarginEnd;

                float constraintMinCrossSize = isRow ? item->minHeight : item->minWidth;
                float constraintMaxCrossSize = isRow ? item->maxHeight : item->maxWidth;

                if (constraintMinCrossSize >= 0)
                    itemCrossSize = std::max (itemCrossSize, constraintMinCrossSize);
                if (constraintMaxCrossSize >= 0)
                    itemCrossSize = std::min (itemCrossSize, constraintMaxCrossSize);

                itemCrossPos = currentCrossPos + crossMarginStart;
            }
            else
            {
                switch (align)
                {
                    case FlexItem::AlignSelf::flexStart:
                        itemCrossPos = currentCrossPos + crossMarginStart;
                        break;
                    case FlexItem::AlignSelf::flexEnd:
                        itemCrossPos = currentCrossPos + lineCrossSize - itemCrossSize - crossMarginEnd;
                        break;
                    case FlexItem::AlignSelf::center:
                    default:
                        itemCrossPos = currentCrossPos + (lineCrossSize - itemCrossSize) / 2.0f;
                        break;
                }
            }

            // Apply spacing
            if (justifyContent == JustifyContent::spaceBetween && line.items.size() > 1 && gapCount > 0)
                currentMainPos += extraSpace / (float) (line.items.size() - 1);
            else if (justifyContent == JustifyContent::spaceAround && gapCount > 0)
                currentMainPos += mainOffset;

            currentMainPos += mainMarginStart;

            // Set bounds
            if (item->associatedComponent != nullptr)
            {
                float x, y, w, h;

                if (isRow)
                {
                    x = currentMainPos;
                    y = itemCrossPos;
                    w = itemMainSize;
                    h = itemCrossSize;
                }
                else
                {
                    x = itemCrossPos;
                    y = currentMainPos;
                    w = itemCrossSize;
                    h = itemMainSize;
                }

                if (isReverse)
                {
                    if (isRow)
                        x = targetArea.getRight() - (x - targetArea.getX()) - w;
                    else
                        y = targetArea.getBottom() - (y - targetArea.getY()) - h;
                }

                item->associatedComponent->setBounds (Rectangle<float> (x, y, w, h).toNearestInt());
            }

            currentMainPos += itemMainSize + mainMarginEnd + gap;
            ++gapCount;
        }

        currentCrossPos += lineCrossSize + gap;
    }
}

void FlexBox::performLayout (Rectangle<int> targetArea)
{
    performLayout (targetArea.to<float>());
}

} // namespace yup
