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
    t.minimum = { SizeType::pixels, pixelSize };
    t.maximum = { SizeType::pixels, pixelSize };
    return t;
}

Grid::TrackInfo Grid::TrackInfo::percent (float percentage)
{
    TrackInfo t;
    t.minimum = { SizeType::percent, percentage };
    t.maximum = { SizeType::percent, percentage };
    return t;
}

Grid::TrackInfo Grid::TrackInfo::fr (float fraction)
{
    TrackInfo t;
    // CSS's `1fr` is `minmax(auto, 1fr)`. The auto floor is a content
    // measurement YUP cannot make, so the floor stays at 0 - see the note on
    // the declaration.
    t.minimum = { SizeType::pixels, 0.0f };
    t.maximum = { SizeType::fraction, fraction };
    return t;
}

Grid::TrackInfo Grid::TrackInfo::auto_()
{
    TrackInfo t;
    t.minimum = { SizeType::autoSize, 0.0f };
    t.maximum = { SizeType::autoSize, 0.0f };
    return t;
}

Grid::TrackInfo Grid::TrackInfo::minmax (TrackInfo minimum, TrackInfo maximum)
{
    // A fractional minimum is not a thing: `minmax(1fr, ...)` is invalid CSS
    // because a track cannot be floored by a share of the space it is competing
    // for.
    jassert (minimum.minimum.type != SizeType::fraction);

    TrackInfo t;
    t.minimum = minimum.minimum;
    t.maximum = maximum.maximum;

    if (t.minimum.type == SizeType::fraction)
        t.minimum = { SizeType::pixels, 0.0f };

    return t;
}

Grid::TrackInfo Grid::TrackInfo::fitContent (float maximumSize)
{
    TrackInfo t;
    t.minimum = { SizeType::pixels, 0.0f };
    t.maximum = { SizeType::pixels, maximumSize };
    return t;
}

//==============================================================================
namespace
{

/** An upper bound on the tracks one layout may allocate, so a pathological
    span or position cannot turn into an unbounded allocation. */
constexpr int maxGridTracks = 4096;

/** An upper bound on the cells the auto-placement scan will visit. */
constexpr int maxPlacementScan = 1 << 20;

/** Resolves one half of a track's sizing to a length. */
float resolveSizingFunction (Grid::TrackInfo::SizingFunction function,
                             float totalSize,
                             float defaultSize,
                             float fractionValue)
{
    switch (function.type)
    {
        case Grid::TrackInfo::SizeType::pixels:
            return function.value;

        case Grid::TrackInfo::SizeType::percent:
            // Percentages resolve against the container's full size on the
            // axis, not against what is left once the gaps are removed.
            return totalSize * function.value / 100.0f;

        case Grid::TrackInfo::SizeType::autoSize:
            return defaultSize;

        case Grid::TrackInfo::SizeType::fraction:
            return fractionValue;
    }

    return 0.0f;
}

/**
    Computes the used size of every track in a template.

    This is CSS §12.4-12.7 in miniature:

    - every track starts at its minimum, with a growth limit taken from its
      maximum (a fractional maximum leaves the limit at the base, so the
      maximize step below leaves those tracks alone);
    - leftover space grows the bases towards their limits, sharing it out
      equally and freezing tracks as they top out;
    - finally the fractional tracks divide up what remains, using the same
      freeze-and-loop shape as flex-grow: a track whose share would come out
      below its own minimum is frozen there and the rest is redistributed. That
      loop is what makes `minmax (px (100), fr (1))` behave.

    The gaps are subtracted before any of this, because the positions the caller
    derives from these sizes advance by `size + gap`.
*/
Array<float> calculateTrackSizes (const Array<Grid::TrackInfo>& tracks,
                                  float totalSize,
                                  float defaultSize,
                                  float gap)
{
    Array<float> sizes;

    if (tracks.isEmpty())
        return sizes;

    const int numTracks = tracks.size();
    const float available = totalSize - gap * static_cast<float> (std::max (0, numTracks - 1));

    Array<float> growthLimits;
    Array<bool> isFlexible;
    Array<bool> isFrozen;
    Array<float> factors;

    sizes.resize (numTracks);
    growthLimits.resize (numTracks);
    isFlexible.resize (numTracks);
    isFrozen.resize (numTracks);
    factors.resize (numTracks);

    for (int i = 0; i < numTracks; ++i)
    {
        const auto& track = tracks.getReference (i);

        const float base = std::max (0.0f, resolveSizingFunction (track.minimum, totalSize, defaultSize, 0.0f));
        const bool flexible = track.isFractional();

        sizes.set (i, base);
        isFlexible.set (i, flexible);
        isFrozen.set (i, false);
        factors.set (i, flexible ? track.maximum.value : 0.0f);
        growthLimits.set (i, flexible
                                 ? base
                                 : std::max (base, resolveSizingFunction (track.maximum, totalSize, defaultSize, 0.0f)));
    }

    // --- maximize tracks ---------------------------------------------------
    {
        float freeSpace = available;

        for (int i = 0; i < numTracks; ++i)
            freeSpace -= sizes.getUnchecked (i);

        while (freeSpace > 1.0e-4f)
        {
            int numGrowable = 0;

            for (int i = 0; i < numTracks; ++i)
                if (growthLimits.getUnchecked (i) - sizes.getUnchecked (i) > 1.0e-4f)
                    ++numGrowable;

            if (numGrowable == 0)
                break;

            const float share = freeSpace / static_cast<float> (numGrowable);
            bool grewAny = false;

            for (int i = 0; i < numTracks; ++i)
            {
                const float room = growthLimits.getUnchecked (i) - sizes.getUnchecked (i);

                if (room <= 1.0e-4f)
                    continue;

                const float taken = std::min (share, room);
                sizes.set (i, sizes.getUnchecked (i) + taken);
                freeSpace -= taken;
                grewAny = true;
            }

            if (! grewAny)
                break;
        }
    }

    // --- expand flexible tracks -------------------------------------------
    bool hasFlexible = false;

    for (int i = 0; i < numTracks; ++i)
        hasFlexible = hasFlexible || isFlexible.getUnchecked (i);

    if (hasFlexible)
    {
        float fractionSize = 0.0f;

        for (int pass = 0; pass <= numTracks; ++pass)
        {
            float leftover = available;
            float factorSum = 0.0f;

            for (int i = 0; i < numTracks; ++i)
            {
                if (isFlexible.getUnchecked (i) && ! isFrozen.getUnchecked (i))
                    factorSum += factors.getUnchecked (i);
                else
                    leftover -= sizes.getUnchecked (i);
            }

            if (factorSum <= 0.0f || leftover <= 0.0f)
            {
                fractionSize = 0.0f;
                break;
            }

            // A flex factor sum below one only claims that fraction of the
            // space, mirroring the same rule in flex-grow.
            fractionSize = leftover / std::max (1.0f, factorSum);

            bool frozeAny = false;

            for (int i = 0; i < numTracks; ++i)
            {
                if (isFlexible.getUnchecked (i)
                    && ! isFrozen.getUnchecked (i)
                    && sizes.getUnchecked (i) > fractionSize * factors.getUnchecked (i))
                {
                    isFrozen.set (i, true);
                    frozeAny = true;
                }
            }

            if (! frozeAny)
                break;
        }

        for (int i = 0; i < numTracks; ++i)
            if (isFlexible.getUnchecked (i) && ! isFrozen.getUnchecked (i))
                sizes.set (i, std::max (sizes.getUnchecked (i), fractionSize * factors.getUnchecked (i)));
    }

    return sizes;
}

//==============================================================================
/** A resolved grid cell (position + span) used for placement bookkeeping. */
struct PlacedCell
{
    int row = 0;
    int column = 0;
    int rowSpan = 1;
    int columnSpan = 1;
};

bool cellsOverlap (const PlacedCell& cell, int major, int minor, int majorSpan, int minorSpan)
{
    return major < cell.row + cell.rowSpan && major + majorSpan > cell.row
        && minor < cell.column + cell.columnSpan && minor + minorSpan > cell.column;
}

bool isAreaFree (const Array<PlacedCell>& placed, int major, int minor, int majorSpan, int minorSpan)
{
    for (const auto& cell : placed)
        if (cellsOverlap (cell, major, minor, majorSpan, minorSpan))
            return false;

    return true;
}

LayoutDistributionMode toDistribution (Grid::AlignContent value)
{
    switch (value)
    {
        case Grid::AlignContent::flexStart:    return LayoutDistributionMode::start;
        case Grid::AlignContent::flexEnd:      return LayoutDistributionMode::end;
        case Grid::AlignContent::center:       return LayoutDistributionMode::center;
        case Grid::AlignContent::spaceBetween: return LayoutDistributionMode::spaceBetween;
        case Grid::AlignContent::spaceAround:  return LayoutDistributionMode::spaceAround;
        case Grid::AlignContent::spaceEvenly:  return LayoutDistributionMode::spaceEvenly;
    }

    return LayoutDistributionMode::start;
}

} // namespace

//==============================================================================
Array<Grid::TrackInfo> Grid::repeat (int count, TrackInfo track)
{
    jassert (count >= 0);

    Array<TrackInfo> tracks;
    const int used = std::clamp (count, 0, maxGridTracks);
    tracks.ensureStorageAllocated (used);

    for (int i = 0; i < used; ++i)
        tracks.add (track);

    return tracks;
}

Array<Grid::TrackInfo> Grid::repeatToFill (TrackInfo track, float availableSize, float gap, float defaultSize)
{
    const float usedGap = std::max (0.0f, gap);
    const float trackMinimum = resolveSizingFunction (track.minimum, availableSize, defaultSize, 0.0f);
    const float step = trackMinimum + usedGap;

    // A track with no minimum would repeat forever; one repetition is the
    // smallest thing that still makes sense.
    if (step <= 0.0f)
        return repeat (1, track);

    const int count = static_cast<int> (std::floor ((availableSize + usedGap) / step));

    return repeat (std::clamp (count, 1, maxGridTracks), track);
}

//==============================================================================
Result Grid::setTemplateAreas (const StringArray& rowPatterns)
{
    if (rowPatterns.isEmpty())
    {
        clearTemplateAreas();
        return Result::ok();
    }

    Array<StringArray> rows;
    int columnCount = -1;

    for (int r = 0; r < rowPatterns.size(); ++r)
    {
        auto tokens = StringArray::fromTokens (rowPatterns[r], false);
        tokens.removeEmptyStrings();

        if (columnCount < 0)
            columnCount = tokens.size();
        else if (tokens.size() != columnCount)
            return Result::fail ("Row " + String (r) + " of the grid areas has " + String (tokens.size())
                                 + " cells but row 0 has " + String (columnCount));

        rows.add (tokens);
    }

    if (columnCount <= 0)
        return Result::fail ("The grid areas have no cells");

    // Collect each name's bounding rectangle, then check the name fills it -
    // CSS only allows rectangular areas, and a non-rectangular one would
    // silently swallow the cells in between.
    Array<NamedArea> parsed;

    for (int r = 0; r < rows.size(); ++r)
    {
        const auto& row = rows.getReference (r);

        for (int c = 0; c < columnCount; ++c)
        {
            const auto& name = row[c];

            if (name == ".")
                continue;

            NamedArea* existing = nullptr;

            for (auto& area : parsed)
                if (area.name == name)
                    existing = &area;

            if (existing == nullptr)
            {
                parsed.add ({ name, r, c, 1, 1 });
                continue;
            }

            existing->rowSpan = std::max (existing->rowSpan, r - existing->row + 1);
            existing->columnSpan = std::max (existing->columnSpan, c - existing->column + 1);
        }
    }

    for (const auto& area : parsed)
    {
        for (int r = area.row; r < area.row + area.rowSpan; ++r)
        {
            for (int c = area.column; c < area.column + area.columnSpan; ++c)
            {
                if (rows.getReference (r)[c] != area.name)
                    return Result::fail ("Grid area '" + area.name + "' is not a rectangle");
            }
        }
    }

    templateAreas = std::move (parsed);
    templateAreaColumns = columnCount;
    templateAreaRows = rows.size();

    return Result::ok();
}

void Grid::clearTemplateAreas()
{
    templateAreas.clear();
    templateAreaColumns = 0;
    templateAreaRows = 0;
}

StringArray Grid::getTemplateAreaNames() const
{
    StringArray names;

    for (const auto& area : templateAreas)
        names.add (area.name);

    return names;
}

//==============================================================================
void Grid::setColumnLineName (int lineIndex, const String& name)
{
    jassert (lineIndex >= 0);

    for (auto& line : columnLineNames)
    {
        if (line.name == name)
        {
            line.index = lineIndex;
            return;
        }
    }

    columnLineNames.add ({ name, lineIndex });
}

void Grid::setRowLineName (int lineIndex, const String& name)
{
    jassert (lineIndex >= 0);

    for (auto& line : rowLineNames)
    {
        if (line.name == name)
        {
            line.index = lineIndex;
            return;
        }
    }

    rowLineNames.add ({ name, lineIndex });
}

void Grid::clearLineNames()
{
    columnLineNames.clear();
    rowLineNames.clear();
}

//==============================================================================
void Grid::performLayout (Rectangle<float> targetArea)
{
    if (items.isEmpty())
        return;

    // columnGap / rowGap legitimately hold -1 to mean "use the gap shorthand",
    // so only the shorthand itself is range-checked.
    jassert (gap >= 0.0f);

    const float shorthandGap = std::max (0.0f, gap);
    const float usedColumnGap = columnGap >= 0.0f ? columnGap : shorthandGap;
    const float usedRowGap = rowGap >= 0.0f ? rowGap : shorthandGap;

    Array<float> columnWidths = calculateTrackSizes (templateColumns, targetArea.getWidth(), autoColumns, usedColumnGap);
    Array<float> rowHeights = calculateTrackSizes (templateRows, targetArea.getHeight(), autoRows, usedRowGap);

    //==============================================================================
    // Resolve every item's requested cell. Named areas and named lines are
    // turned into 0-based indices HERE and nowhere else, so no later stage has
    // to know that CSS numbers its lines from 1.

    struct Request
    {
        int row = GridItem::autoPlace;
        int column = GridItem::autoPlace;
        int rowSpan = 1;
        int columnSpan = 1;
    };

    Array<Request> requests;
    requests.resize (items.size());

    for (int i = 0; i < items.size(); ++i)
    {
        const auto& item = items.getReference (i);

        jassert (item.rowSpan >= 1 && item.columnSpan >= 1);

        Request request;
        request.row = item.row;
        request.column = item.column;
        request.rowSpan = std::clamp (item.rowSpan, 1, maxGridTracks);
        request.columnSpan = std::clamp (item.columnSpan, 1, maxGridTracks);

        bool placedByArea = false;

        if (item.area.isNotEmpty())
        {
            for (const auto& area : templateAreas)
            {
                if (area.name != item.area)
                    continue;

                request.row = area.row;
                request.column = area.column;
                request.rowSpan = area.rowSpan;
                request.columnSpan = area.columnSpan;
                placedByArea = true;
                break;
            }
        }

        if (! placedByArea)
        {
            if (item.columnStartName.isNotEmpty())
                for (const auto& line : columnLineNames)
                    if (line.name == item.columnStartName)
                        request.column = line.index;

            if (item.rowStartName.isNotEmpty())
                for (const auto& line : rowLineNames)
                    if (line.name == item.rowStartName)
                        request.row = line.index;
        }

        requests.set (i, request);
    }

    //==============================================================================
    // Placement, per CSS Grid §8.5. Everything below works in "major/minor"
    // axes so that row flow and column flow are the same code: for row flow the
    // major axis is the row, for column flow it is the column.

    const bool isColumnFlow = (autoFlow == AutoFlow::column || autoFlow == AutoFlow::columnDense);
    const bool isDense = (autoFlow == AutoFlow::rowDense || autoFlow == AutoFlow::columnDense);

    const int explicitColumns = std::max (templateColumns.size(), templateAreaColumns);
    const int explicitRows = std::max (templateRows.size(), templateAreaRows);

    int minorCount = isColumnFlow ? explicitRows : explicitColumns;

    if (minorCount <= 0)
        minorCount = -1; // no template: the implicit grid grows sideways

    Array<PlacedCell> placed;
    Array<PlacedCell> resolvedCells;
    Array<bool> isResolved;

    resolvedCells.resize (items.size());
    isResolved.resize (items.size());

    for (int i = 0; i < items.size(); ++i)
        isResolved.set (i, false);

    auto majorOf = [isColumnFlow] (const Request& r) { return isColumnFlow ? r.column : r.row; };
    auto minorOf = [isColumnFlow] (const Request& r) { return isColumnFlow ? r.row : r.column; };
    auto majorSpanOf = [isColumnFlow] (const Request& r) { return isColumnFlow ? r.columnSpan : r.rowSpan; };
    auto minorSpanOf = [isColumnFlow] (const Request& r) { return isColumnFlow ? r.rowSpan : r.columnSpan; };

    auto occupy = [&] (int index, int major, int minor, int majorSpan, int minorSpan)
    {
        // isAreaFree works in major/minor, so the bookkeeping copy stays in
        // those axes and only the caller-visible cell is transposed back.
        placed.add ({ major, minor, majorSpan, minorSpan });

        resolvedCells.set (index, isColumnFlow
                                      ? PlacedCell { minor, major, minorSpan, majorSpan }
                                      : PlacedCell { major, minor, majorSpan, minorSpan });
        isResolved.set (index, true);
    };

    // Pass 1: both axes definite.
    for (int i = 0; i < items.size(); ++i)
    {
        const auto& r = requests.getReference (i);

        if (majorOf (r) >= 0 && minorOf (r) >= 0)
            occupy (i, majorOf (r), minorOf (r), majorSpanOf (r), minorSpanOf (r));
    }

    // Pass 2: definite major axis, automatic minor axis.
    for (int i = 0; i < items.size(); ++i)
    {
        if (isResolved.getUnchecked (i))
            continue;

        const auto& r = requests.getReference (i);

        if (majorOf (r) < 0 || minorOf (r) >= 0)
            continue;

        int minor = 0;

        while (minor < maxGridTracks
               && ! isAreaFree (placed, majorOf (r), minor, majorSpanOf (r), minorSpanOf (r)))
            ++minor;

        occupy (i, majorOf (r), minor, majorSpanOf (r), minorSpanOf (r));
    }

    // Pass 3: flow the rest from a cursor. A dense flow restarts the cursor for
    // every item so it can backfill the holes a larger item left behind; a
    // sparse one never moves backwards.
    int cursorMajor = 0;
    int cursorMinor = 0;

    for (int i = 0; i < items.size(); ++i)
    {
        if (isResolved.getUnchecked (i))
            continue;

        const auto& r = requests.getReference (i);
        const int majorSpan = majorSpanOf (r);
        const int minorSpan = minorSpanOf (r);

        if (isDense)
        {
            cursorMajor = 0;
            cursorMinor = 0;
        }

        if (minorOf (r) >= 0)
        {
            if (minorOf (r) < cursorMinor)
                ++cursorMajor;

            cursorMinor = minorOf (r);

            int scanned = 0;

            while (scanned++ < maxPlacementScan
                   && cursorMajor < maxGridTracks
                   && ! isAreaFree (placed, cursorMajor, cursorMinor, majorSpan, minorSpan))
                ++cursorMajor;

            occupy (i, cursorMajor, cursorMinor, majorSpan, minorSpan);
        }
        else
        {
            int scanned = 0;

            while (scanned++ < maxPlacementScan && cursorMajor < maxGridTracks)
            {
                const int wrapAt = minorCount >= 0 ? minorCount : maxGridTracks;

                if (cursorMinor + minorSpan > wrapAt)
                {
                    ++cursorMajor;
                    cursorMinor = 0;
                    continue;
                }

                if (isAreaFree (placed, cursorMajor, cursorMinor, majorSpan, minorSpan))
                    break;

                ++cursorMinor;
            }

            occupy (i, cursorMajor, cursorMinor, majorSpan, minorSpan);
            cursorMinor += minorSpan;
        }
    }

    //==============================================================================
    // Grow the implicit tracks so every placed span is covered.

    for (const auto& cell : resolvedCells)
    {
        while (cell.column + cell.columnSpan > columnWidths.size() && columnWidths.size() < maxGridTracks)
            columnWidths.add (autoColumns);

        while (cell.row + cell.rowSpan > rowHeights.size() && rowHeights.size() < maxGridTracks)
            rowHeights.add (autoRows);
    }

    //==============================================================================
    // Track positions, offset by justify-content / align-content over whatever
    // space the tracks did not use.

    float usedWidth = usedColumnGap * static_cast<float> (std::max (0, columnWidths.size() - 1));
    float usedHeight = usedRowGap * static_cast<float> (std::max (0, rowHeights.size() - 1));

    for (auto width : columnWidths)
        usedWidth += width;

    for (auto height : rowHeights)
        usedHeight += height;

    const auto horizontal = LayoutDistribution::calculate (toDistribution (justifyContent),
                                                           targetArea.getWidth() - usedWidth,
                                                           columnWidths.size(),
                                                           usedColumnGap);

    const auto vertical = LayoutDistribution::calculate (toDistribution (alignContent),
                                                         targetArea.getHeight() - usedHeight,
                                                         rowHeights.size(),
                                                         usedRowGap);

    Array<float> columnPositions;
    float currentX = targetArea.getX() + horizontal.leading;

    for (auto width : columnWidths)
    {
        columnPositions.add (currentX);
        currentX += width + horizontal.between;
    }

    Array<float> rowPositions;
    float currentY = targetArea.getY() + vertical.leading;

    for (auto height : rowHeights)
    {
        rowPositions.add (currentY);
        currentY += height + vertical.between;
    }

    //==============================================================================
    // Size and position each item within its cell.

    struct Resolved
    {
        float x = 0.0f, y = 0.0f, width = 0.0f, height = 0.0f;
        float cellY = 0.0f, cellHeight = 0.0f;
        int row = 0;
        bool valid = false;
        bool baseline = false;
    };

    Array<Resolved> results;
    results.resize (items.size());

    auto resolveAlign = [] (GridItem::AlignSelf self, Grid::AlignItems container)
    {
        if (self != GridItem::AlignSelf::autoAlign)
            return self;

        switch (container)
        {
            case Grid::AlignItems::flexStart: return GridItem::AlignSelf::flexStart;
            case Grid::AlignItems::flexEnd:   return GridItem::AlignSelf::flexEnd;
            case Grid::AlignItems::center:    return GridItem::AlignSelf::center;
            case Grid::AlignItems::stretch:   return GridItem::AlignSelf::stretch;
            case Grid::AlignItems::baseline:  return GridItem::AlignSelf::baseline;
        }

        return GridItem::AlignSelf::stretch;
    };

    for (int i = 0; i < items.size(); ++i)
    {
        const auto& item = items.getReference (i);

        if (item.associatedComponent == nullptr)
            continue;

        const auto& cell = resolvedCells.getReference (i);

        if (cell.column >= columnPositions.size() || cell.row >= rowPositions.size())
            continue;

        const int lastColumn = std::min (cell.column + cell.columnSpan, columnPositions.size()) - 1;
        const int lastRow = std::min (cell.row + cell.rowSpan, rowPositions.size()) - 1;

        float cellX = columnPositions.getUnchecked (cell.column);
        float cellY = rowPositions.getUnchecked (cell.row);
        float cellW = columnPositions.getUnchecked (lastColumn) + columnWidths.getUnchecked (lastColumn) - cellX;
        float cellH = rowPositions.getUnchecked (lastRow) + rowHeights.getUnchecked (lastRow) - cellY;

        // Margins eat into the cell, and can legitimately exceed it - a
        // negative cell size would then invert the rectangle.
        cellX += item.marginLeft;
        cellY += item.marginTop;
        cellW = std::max (0.0f, cellW - item.marginLeft - item.marginRight);
        cellH = std::max (0.0f, cellH - item.marginTop - item.marginBottom);

        const auto horizontalAlign = resolveAlign (item.justifySelf, justifyItems);
        const auto verticalAlign = resolveAlign (item.alignSelf, alignItems);

        // Item size: a percentage of the cell, then an explicit size, then the
        // cell itself.
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

        if (item.minWidth >= 0.0f)
            itemW = std::max (itemW, item.minWidth);
        if (item.maxWidth >= 0.0f)
            itemW = std::min (itemW, item.maxWidth);
        if (item.minHeight >= 0.0f)
            itemH = std::max (itemH, item.minHeight);
        if (item.maxHeight >= 0.0f)
            itemH = std::min (itemH, item.maxHeight);

        itemW = std::max (0.0f, itemW);
        itemH = std::max (0.0f, itemH);

        float itemX = cellX;
        float itemY = cellY;

        if (horizontalAlign == GridItem::AlignSelf::center)
            itemX = cellX + (cellW - itemW) / 2.0f;
        else if (horizontalAlign == GridItem::AlignSelf::flexEnd)
            itemX = cellX + cellW - itemW;

        if (verticalAlign == GridItem::AlignSelf::center)
            itemY = cellY + (cellH - itemH) / 2.0f;
        else if (verticalAlign == GridItem::AlignSelf::flexEnd)
            itemY = cellY + cellH - itemH;

        results.set (i, { itemX, itemY, itemW, itemH, cellY, cellH, cell.row, true,
                          verticalAlign == GridItem::AlignSelf::baseline });
    }

    // Baseline-aligned items share a baseline with the others in their row.
    // With no way to measure content the synthesized baseline is the item's
    // bottom edge, which is what a browser also does for a box with no text.
    for (int row = 0; row < rowPositions.size(); ++row)
    {
        float sharedBaseline = 0.0f;
        bool anyInRow = false;

        for (int i = 0; i < results.size(); ++i)
        {
            const auto& r = results.getReference (i);

            if (r.valid && r.baseline && r.row == row)
            {
                sharedBaseline = std::max (sharedBaseline, r.height);
                anyInRow = true;
            }
        }

        if (! anyInRow)
            continue;

        for (int i = 0; i < results.size(); ++i)
        {
            auto r = results.getReference (i);

            if (! (r.valid && r.baseline && r.row == row))
                continue;

            r.y = r.cellY + sharedBaseline - r.height;
            results.set (i, r);
        }
    }

    for (int i = 0; i < items.size(); ++i)
    {
        const auto& r = results.getReference (i);

        if (! r.valid)
            continue;

        // Component::setBounds takes floats; rounding here would only lose
        // precision and make adjacent items disagree about their shared edge.
        items.getReference (i).associatedComponent->setBounds (Rectangle<float> (r.x, r.y, r.width, r.height));
    }
}

void Grid::performLayout (Rectangle<int> targetArea)
{
    performLayout (targetArea.to<float>());
}

} // namespace yup
