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
Grid::TrackInfo Grid::TrackInfo::px (float pixelSize)
{
    TrackInfo t;
    t.pixelSize = pixelSize;
    return t;
}

Grid::TrackInfo Grid::TrackInfo::fr (float fraction)
{
    TrackInfo t;
    t.fraction = fraction;
    return t;
}

Grid::TrackInfo Grid::TrackInfo::auto_()
{
    TrackInfo t;
    t.isAuto = true;
    return t;
}

//==============================================================================
Array<float> Grid::calculateTrackSizes (const Array<TrackInfo>& tracks,
                                        float totalSize,
                                        float defaultSize)
{
    Array<float> sizes;
    float usedSize = 0.0f;
    float totalFr = 0.0f;

    if (tracks.isEmpty())
    {
        // Auto-track mode: one column per item
        return sizes; // will be handled by the caller
    }

    sizes.resize (tracks.size());

    // First pass: allocate fixed and count fr units
    for (int i = 0; i < tracks.size(); ++i)
    {
        const auto& track = tracks.getReference (i);

        if (track.isAuto)
        {
            sizes.set (i, defaultSize);
            usedSize += defaultSize;
        }
        else if (track.fraction > 0.0f)
        {
            totalFr += track.fraction;
            sizes.set (i, 0.0f);
        }
        else
        {
            sizes.set (i, track.pixelSize);
            usedSize += track.pixelSize;
        }
    }

    // Distribute remaining space by fr units
    if (totalFr > 0.0f)
    {
        float remaining = std::max (0.0f, totalSize - usedSize);

        for (int i = 0; i < tracks.size(); ++i)
        {
            const auto& track = tracks.getReference (i);

            if (track.fraction > 0.0f)
            {
                sizes.set (i, remaining * track.fraction / totalFr);
            }
        }
    }

    return sizes;
}

//==============================================================================
void Grid::performLayout (Rectangle<float> targetArea)
{
    if (items.isEmpty())
        return;

    // Calculate column widths
    Array<float> columnWidths;
    float totalColWidth = targetArea.getWidth();

    if (! templateColumns.isEmpty())
        columnWidths = calculateTrackSizes (templateColumns, totalColWidth, autoColumns);
    else
    {
        // Auto-layout: place items in a single row with fixed width columns
        columnWidths.add (autoColumns);
    }

    // Calculate row heights
    Array<float> rowHeights;
    float totalRowHeight = targetArea.getHeight();

    if (! templateRows.isEmpty())
        rowHeights = calculateTrackSizes (templateRows, totalRowHeight, autoRows);
    else
    {
        // Auto-layout: one row per item
        rowHeights.add (autoRows);
    }

    // Calculate cell positions
    Array<float> columnPositions;
    float currentX = targetArea.getX();

    for (auto width : columnWidths)
    {
        columnPositions.add (currentX);
        currentX += width + columnGap;
    }

    Array<float> rowPositions;
    float currentY = targetArea.getY();

    for (auto height : rowHeights)
    {
        rowPositions.add (currentY);
        currentY += height + rowGap;
    }

    // Position each item
    for (const auto& item : items)
    {
        if (item.associatedComponent == nullptr)
            continue;

        // Calculate cell bounds
        const int col = item.column;
        const int row = item.row;
        const int colSpan = item.columnSpan;
        const int rowSpan = item.rowSpan;

        if (col < 0 || row < 0)
            continue;

        float cellX = 0.0f;
        float cellY = 0.0f;
        float cellW = 100.0f;
        float cellH = 100.0f;

        if (! columnPositions.isEmpty() && col < columnPositions.size())
        {
            cellX = columnPositions[col];

            if (col + colSpan <= columnPositions.size())
            {
                float endX = columnPositions.getUnchecked (col + colSpan - 1) + columnWidths[col + colSpan - 1];
                cellW = endX - cellX;
            }
            else
            {
                cellW = columnWidths[col];
            }
        }

        if (! rowPositions.isEmpty() && row < rowPositions.size())
        {
            cellY = rowPositions[row];

            if (row + rowSpan <= rowPositions.size())
            {
                float endY = rowPositions.getUnchecked (row + rowSpan - 1) + rowHeights[row + rowSpan - 1];
                cellH = endY - cellY;
            }
            else
            {
                cellH = rowHeights[row];
            }
        }

        // Apply margins
        cellX += item.marginLeft;
        cellY += item.marginTop;
        cellW -= item.marginLeft + item.marginRight;
        cellH -= item.marginTop + item.marginBottom;

        // Apply alignment
        GridItem::AlignSelf hAlign = item.justifySelf;
        GridItem::AlignSelf vAlign = item.alignSelf;

        if (hAlign == GridItem::AlignSelf::autoAlign)
        {
            switch (justifyItems)
            {
                case AlignItems::flexStart:
                    hAlign = GridItem::AlignSelf::flexStart;
                    break;
                case AlignItems::flexEnd:
                    hAlign = GridItem::AlignSelf::flexEnd;
                    break;
                case AlignItems::center:
                    hAlign = GridItem::AlignSelf::center;
                    break;
                case AlignItems::stretch:
                    hAlign = GridItem::AlignSelf::stretch;
                    break;
            }
        }

        if (vAlign == GridItem::AlignSelf::autoAlign)
        {
            switch (alignItems)
            {
                case AlignItems::flexStart:
                    vAlign = GridItem::AlignSelf::flexStart;
                    break;
                case AlignItems::flexEnd:
                    vAlign = GridItem::AlignSelf::flexEnd;
                    break;
                case AlignItems::center:
                    vAlign = GridItem::AlignSelf::center;
                    break;
                case AlignItems::stretch:
                    vAlign = GridItem::AlignSelf::stretch;
                    break;
            }
        }

        float itemX = cellX;
        float itemY = cellY;
        float itemW = 100.0f;
        float itemH = 100.0f;

        // Component's preferred size
        if (hAlign == GridItem::AlignSelf::stretch)
            itemW = cellW;
        else
            itemW = 100.0f; // Default width

        if (vAlign == GridItem::AlignSelf::stretch)
            itemH = cellH;
        else
            itemH = 100.0f; // Default height

        // Apply horizontal alignment
        if (hAlign == GridItem::AlignSelf::center)
            itemX = cellX + (cellW - itemW) / 2.0f;
        else if (hAlign == GridItem::AlignSelf::flexEnd)
            itemX = cellX + cellW - itemW;

        // Apply vertical alignment
        if (vAlign == GridItem::AlignSelf::center)
            itemY = cellY + (cellH - itemH) / 2.0f;
        else if (vAlign == GridItem::AlignSelf::flexEnd)
            itemY = cellY + cellH - itemH;

        item.associatedComponent->setBounds (Rectangle<float> (itemX, itemY, itemW, itemH).toNearestInt());
    }
}

void Grid::performLayout (Rectangle<int> targetArea)
{
    performLayout (targetArea.to<float>());
}

} // namespace yup
