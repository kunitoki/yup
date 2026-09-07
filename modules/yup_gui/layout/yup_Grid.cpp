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
namespace
{

// A resolved grid cell (position + span) used for auto-placement bookkeeping.
struct PlacedCell
{
    int row = 0;
    int col = 0;
    int rowSpan = 1;
    int colSpan = 1;
};

bool overlapsPlacedCell (const PlacedCell& cell, int row, int col, int rowSpan, int colSpan)
{
    return row < cell.row + cell.rowSpan && row + rowSpan > cell.row
        && col < cell.col + cell.colSpan && col + colSpan > cell.col;
}

bool isCellFree (const Array<PlacedCell>& placed, int row, int col, int rowSpan, int colSpan)
{
    for (const auto& cell : placed)
    {
        if (overlapsPlacedCell (cell, row, col, rowSpan, colSpan))
            return false;
    }

    return true;
}

// Sparse auto-placement: scans from the cursor position onward, row by row,
// and places the item in the first free cell. When maxCol is >= 0 (an explicit
// column template exists) the scan wraps to the next row at that column count;
// otherwise implicit tracks grow without bound.
void findAutoPlacementCell (int& outRow, int& outCol, int& cursorRow, int& cursorCol, int rowSpan, int colSpan, int maxCol, const Array<PlacedCell>& placed)
{
    for (int row = cursorRow;; ++row)
    {
        const int startCol = (row == cursorRow) ? cursorCol : 0;

        if (maxCol >= 0)
        {
            for (int col = startCol; col < maxCol; ++col)
            {
                if (isCellFree (placed, row, col, rowSpan, colSpan))
                {
                    outRow = row;
                    outCol = col;
                    cursorRow = row;
                    cursorCol = col + colSpan;
                    return;
                }
            }

            // Row exhausted: advance the cursor to the next row.
            cursorRow = row + 1;
            cursorCol = 0;
        }
        else
        {
            for (int col = startCol;; ++col)
            {
                if (isCellFree (placed, row, col, rowSpan, colSpan))
                {
                    outRow = row;
                    outCol = col;
                    cursorRow = row;
                    cursorCol = col + colSpan;
                    return;
                }
            }
        }
    }
}

} // namespace

//==============================================================================
void Grid::performLayout (Rectangle<float> targetArea)
{
    if (items.isEmpty())
        return;

    // Column track sizes: from template definitions, or grown implicitly.
    Array<float> columnWidths;

    if (! templateColumns.isEmpty())
        columnWidths = calculateTrackSizes (templateColumns, targetArea.getWidth(), autoColumns);

    // Row track sizes: from template definitions, or grown implicitly.
    Array<float> rowHeights;

    if (! templateRows.isEmpty())
        rowHeights = calculateTrackSizes (templateRows, targetArea.getHeight(), autoRows);

    // Resolve each item's cell (explicit or auto-placed) and grow the implicit
    // tracks so every item's span is covered.
    const int maxAutoColumn = templateColumns.isEmpty() ? -1 : templateColumns.size();

    Array<PlacedCell> placed;
    placed.ensureStorageAllocated (items.size());

    int cursorRow = 0;
    int cursorCol = 0;

    for (const auto& item : items)
    {
        int col = item.column;
        int row = item.row;

        // Defensively cap spans so a pathological value cannot make the
        // implicit-track growth allocate without bound.
        const int colSpan = std::clamp (item.columnSpan, 1, 10000);
        const int rowSpan = std::clamp (item.rowSpan, 1, 10000);

        if (col < 0 || row < 0)
            findAutoPlacementCell (row, col, cursorRow, cursorCol, rowSpan, colSpan, maxAutoColumn, placed);

        while (col + colSpan > columnWidths.size())
            columnWidths.add (autoColumns);

        while (row + rowSpan > rowHeights.size())
            rowHeights.add (autoRows);

        placed.add ({ row, col, rowSpan, colSpan });
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
    for (int i = 0; i < items.size(); ++i)
    {
        const auto& item = items.getReference (i);

        if (item.associatedComponent == nullptr)
            continue;

        const auto& cell = placed.getReference (i);
        const int col = cell.col;
        const int row = cell.row;
        const int colSpan = cell.colSpan;
        const int rowSpan = cell.rowSpan;

        // Calculate cell bounds
        float cellX = columnPositions.getUnchecked (col);
        float cellY = rowPositions.getUnchecked (row);
        float cellW = columnWidths.getUnchecked (col);
        float cellH = rowHeights.getUnchecked (row);

        if (col + colSpan <= columnPositions.size())
        {
            float endX = columnPositions.getUnchecked (col + colSpan - 1) + columnWidths.getUnchecked (col + colSpan - 1);
            cellW = endX - cellX;
        }

        if (row + rowSpan <= rowPositions.size())
        {
            float endY = rowPositions.getUnchecked (row + rowSpan - 1) + rowHeights.getUnchecked (row + rowSpan - 1);
            cellH = endY - cellY;
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

        // Resolve the item size: percentage > explicit size > fill the cell.
        float itemW = cellW;

        if (item.widthPercent >= 0.0f)
            itemW = cellW * item.widthPercent / 100.0f;
        else if (item.width >= 0.0f)
            itemW = item.width;

        float itemH = cellH;

        if (item.heightPercent >= 0.0f)
            itemH = cellH * item.heightPercent / 100.0f;
        else if (item.height >= 0.0f)
            itemH = item.height;

        // Apply min/max constraints
        if (item.minWidth >= 0.0f)
            itemW = std::max (itemW, item.minWidth);
        if (item.maxWidth >= 0.0f)
            itemW = std::min (itemW, item.maxWidth);
        if (item.minHeight >= 0.0f)
            itemH = std::max (itemH, item.minHeight);
        if (item.maxHeight >= 0.0f)
            itemH = std::min (itemH, item.maxHeight);

        float itemX = cellX;
        float itemY = cellY;

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
