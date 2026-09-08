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

#include <yup_gui/yup_gui.h>

#include <gtest/gtest.h>

using namespace yup;

namespace
{

// Layout results are floats now that performLayout no longer rounds to whole
// pixels, so a third of a track comes back as 53.33333 rather than 53.
constexpr float gridTolerance = 0.001f;

void expectGridBounds (const Component& component, float x, float y, float width, float height)
{
    auto bounds = component.getBounds();
    EXPECT_NEAR (x, bounds.getX(), gridTolerance);
    EXPECT_NEAR (y, bounds.getY(), gridTolerance);
    EXPECT_NEAR (width, bounds.getWidth(), gridTolerance);
    EXPECT_NEAR (height, bounds.getHeight(), gridTolerance);
}

Grid makeTwoColumnGrid()
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::px (200));
    grid.templateColumns.add (Grid::TrackInfo::px (200));
    grid.templateRows.add (Grid::TrackInfo::px (100));
    return grid;
}

} // namespace

// =============================================================================
// stretch (existing behavior)
// =============================================================================

TEST (GridTests, StretchFillsTheCell)
{
    auto grid = makeTwoColumnGrid();

    Component c;
    grid.items.add (GridItem (c).withColumn (0).withRow (0));
    grid.performLayout (Rectangle<float> (0, 0, 400, 200));

    expectGridBounds (c, 0, 0, 200, 100);
}

// =============================================================================
// explicit width / height with every alignment
// =============================================================================

TEST (GridTests, ExplicitWidthAndHeightAreHonoredForEveryJustifySelf)
{
    struct JustifyCase
    {
        GridItem::AlignSelf justify;
        float expectedX;
    };

    const JustifyCase cases[] = {
        { GridItem::AlignSelf::flexStart, 0.0f },
        { GridItem::AlignSelf::center, 75.0f },
        { GridItem::AlignSelf::flexEnd, 150.0f },
        { GridItem::AlignSelf::stretch, 0.0f },
    };

    for (const auto& testCase : cases)
    {
        auto grid = makeTwoColumnGrid();

        Component c;
        grid.items.add (GridItem (c)
                            .withColumn (0)
                            .withRow (0)
                            .withWidth (50)
                            .withHeight (30)
                            .withAlignSelf (GridItem::AlignSelf::stretch));
        grid.items.getReference (0).justifySelf = testCase.justify;
        grid.performLayout (Rectangle<float> (0, 0, 400, 200));

        expectGridBounds (c, testCase.expectedX, 0, 50, 30);
    }
}

TEST (GridTests, ExplicitWidthAndHeightAreHonoredForEveryAlignSelf)
{
    struct AlignCase
    {
        GridItem::AlignSelf align;
        float expectedY;
    };

    const AlignCase cases[] = {
        { GridItem::AlignSelf::flexStart, 0.0f },
        { GridItem::AlignSelf::center, 35.0f },
        { GridItem::AlignSelf::flexEnd, 70.0f },
        { GridItem::AlignSelf::stretch, 0.0f },
    };

    for (const auto& testCase : cases)
    {
        auto grid = makeTwoColumnGrid();

        Component c;
        grid.items.add (GridItem (c)
                            .withColumn (0)
                            .withRow (0)
                            .withWidth (50)
                            .withHeight (30)
                            .withJustifySelf (GridItem::AlignSelf::stretch));
        grid.items.getReference (0).alignSelf = testCase.align;
        grid.performLayout (Rectangle<float> (0, 0, 400, 200));

        expectGridBounds (c, 0, testCase.expectedY, 50, 30);
    }
}

TEST (GridTests, JustifyItemsContainerDefaultAppliesToAutoAlignedItems)
{
    auto grid = makeTwoColumnGrid();
    grid.justifyItems = Grid::AlignItems::center;

    Component c1, c2;
    grid.items.add (GridItem (c1).withColumn (0).withRow (0).withWidth (50).withHeight (30));
    grid.items.add (GridItem (c2)
                        .withColumn (1)
                        .withRow (0)
                        .withWidth (50)
                        .withHeight (30)
                        .withJustifySelf (GridItem::AlignSelf::flexStart));
    grid.performLayout (Rectangle<float> (0, 0, 400, 200));

    expectGridBounds (c1, 75, 0, 50, 30);
    expectGridBounds (c2, 200, 0, 50, 30);
}

TEST (GridTests, AlignItemsContainerDefaultAppliesToAutoAlignedItems)
{
    auto grid = makeTwoColumnGrid();
    grid.alignItems = Grid::AlignItems::flexEnd;

    Component c;
    grid.items.add (GridItem (c).withColumn (0).withRow (0).withWidth (50).withHeight (30));
    grid.performLayout (Rectangle<float> (0, 0, 400, 200));

    expectGridBounds (c, 0, 70, 50, 30);
}

// =============================================================================
// min/max clamps
// =============================================================================

TEST (GridTests, MinMaxClampsAreAppliedToExplicitSizes)
{
    auto grid = makeTwoColumnGrid();

    Component c;
    grid.items.add (GridItem (c)
                        .withColumn (0)
                        .withRow (0)
                        .withWidth (150)
                        .withHeight (60)
                        .withMinWidth (180)
                        .withMinHeight (70)
                        .withMaxWidth (190)
                        .withMaxHeight (80)
                        .withJustifySelf (GridItem::AlignSelf::center)
                        .withAlignSelf (GridItem::AlignSelf::center));
    grid.performLayout (Rectangle<float> (0, 0, 400, 200));

    // Width clamped from 150 to 180, height from 60 to 70, then centered
    expectGridBounds (c, 10, 15, 180, 70);
}

TEST (GridTests, PercentageSizingIsClampedByMinMax)
{
    auto grid = makeTwoColumnGrid();

    Component c;
    grid.items.add (GridItem (c)
                        .withColumn (0)
                        .withRow (0)
                        .withWidthPercent (50)
                        .withHeightPercent (50)
                        .withMinWidth (120)
                        .withMinHeight (60)
                        .withJustifySelf (GridItem::AlignSelf::center)
                        .withAlignSelf (GridItem::AlignSelf::center));
    grid.performLayout (Rectangle<float> (0, 0, 400, 200));

    // 50% of 200x100 = 100x50, clamped up to 120x60, then centered
    expectGridBounds (c, 40, 20, 120, 60);
}

// =============================================================================
// spans
// =============================================================================

TEST (GridTests, ColumnAndRowSpansCoverMultipleTracks)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateRows.add (Grid::TrackInfo::px (50));
    grid.templateRows.add (Grid::TrackInfo::px (50));

    Component c1, c2;
    grid.items.add (GridItem (c1).withColumn (0).withRow (0).withColumnSpan (2).withRowSpan (2));
    grid.items.add (GridItem (c2).withColumn (2).withRow (0));
    grid.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectGridBounds (c1, 0, 0, 200, 100);
    expectGridBounds (c2, 200, 0, 100, 50);
}

// =============================================================================
// percentage sizing
// =============================================================================

TEST (GridTests, PercentageSizingResolvesAgainstTheCell)
{
    auto grid = makeTwoColumnGrid();

    Component c;
    grid.items.add (GridItem (c)
                        .withColumn (0)
                        .withRow (0)
                        .withWidthPercent (50)
                        .withHeightPercent (50)
                        .withJustifySelf (GridItem::AlignSelf::center)
                        .withAlignSelf (GridItem::AlignSelf::center));
    grid.performLayout (Rectangle<float> (0, 0, 400, 200));

    // 50% of the 200x100 cell, centered within it
    expectGridBounds (c, 50, 25, 100, 50);
}

// =============================================================================
// track sizing
// =============================================================================

TEST (GridTests, FrTracksShareTheRemainingSpace)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::fr (1));
    grid.templateColumns.add (Grid::TrackInfo::fr (2));
    grid.templateRows.add (Grid::TrackInfo::px (100));

    Component c1, c2;
    grid.items.add (GridItem (c1).withColumn (0).withRow (0));
    grid.items.add (GridItem (c2).withColumn (1).withRow (0));
    grid.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectGridBounds (c1, 0, 0, 100, 100);
    expectGridBounds (c2, 100, 0, 200, 100);
}

TEST (GridTests, FixedTracksTakePriorityOverFrTracks)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::px (50));
    grid.templateColumns.add (Grid::TrackInfo::fr (1));
    grid.templateColumns.add (Grid::TrackInfo::fr (1));
    grid.templateRows.add (Grid::TrackInfo::px (100));

    Component c1, c2, c3;
    grid.items.add (GridItem (c1).withColumn (0).withRow (0));
    grid.items.add (GridItem (c2).withColumn (1).withRow (0));
    grid.items.add (GridItem (c3).withColumn (2).withRow (0));
    grid.performLayout (Rectangle<float> (0, 0, 300, 100));

    // 50 fixed, remaining 250 split equally between the two fr tracks
    expectGridBounds (c1, 0, 0, 50, 100);
    expectGridBounds (c2, 50, 0, 125, 100);
    expectGridBounds (c3, 175, 0, 125, 100);
}

TEST (GridTests, AutoTracksUseTheDefaultTrackSizes)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::auto_());
    grid.templateColumns.add (Grid::TrackInfo::auto_());
    grid.templateRows.add (Grid::TrackInfo::auto_());
    grid.autoRows = 60.0f;

    Component c1, c2;
    grid.items.add (GridItem (c1).withColumn (0).withRow (0));
    grid.items.add (GridItem (c2).withColumn (1).withRow (0));
    grid.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectGridBounds (c1, 0, 0, 100, 60);
    expectGridBounds (c2, 100, 0, 100, 60);
}

TEST (GridTests, MixedTrackTypesResolveInOrder)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::auto_());
    grid.templateColumns.add (Grid::TrackInfo::fr (1));
    grid.templateRows.add (Grid::TrackInfo::px (100));

    Component c1, c2;
    grid.items.add (GridItem (c1).withColumn (0).withRow (0));
    grid.items.add (GridItem (c2).withColumn (1).withRow (0));
    grid.performLayout (Rectangle<float> (0, 0, 300, 100));

    // auto track gets 100, the fr track gets the remaining 200
    expectGridBounds (c1, 0, 0, 100, 100);
    expectGridBounds (c2, 100, 0, 200, 100);
}

TEST (GridTests, CustomAutoTrackSizesAreUsedForImplicitTracks)
{
    Grid grid;
    grid.autoColumns = 50.0f;
    grid.autoRows = 30.0f;

    Component c1, c2;
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (c2));
    grid.performLayout (Rectangle<float> (0, 0, 400, 200));

    expectGridBounds (c1, 0, 0, 50, 30);
    expectGridBounds (c2, 50, 0, 50, 30);
}

TEST (GridTests, ExplicitPlacementBeyondTheTemplateCreatesImplicitTracks)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::px (100));

    Component c;
    grid.items.add (GridItem (c).withColumn (3).withRow (0));
    grid.performLayout (Rectangle<float> (0, 0, 300, 100));

    // Columns 1..3 are created implicitly with autoColumns, no gaps
    expectGridBounds (c, 300, 0, 100, 40);
}

// =============================================================================
// gaps and margins
// =============================================================================

TEST (GridTests, ColumnGapAndRowGapAreRespected)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateRows.add (Grid::TrackInfo::px (50));
    grid.templateRows.add (Grid::TrackInfo::px (50));
    grid.columnGap = 10.0f;
    grid.rowGap = 20.0f;

    Component c1, c2, c3;
    grid.items.add (GridItem (c1).withColumn (0).withRow (0));
    grid.items.add (GridItem (c2).withColumn (1).withRow (0));
    grid.items.add (GridItem (c3).withColumn (0).withRow (1));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    expectGridBounds (c1, 0, 0, 100, 50);
    expectGridBounds (c2, 110, 0, 100, 50);
    expectGridBounds (c3, 0, 70, 100, 50);
}

TEST (GridTests, GridItemMarginsShrinkTheAvailableCell)
{
    auto grid = makeTwoColumnGrid();

    Component c;
    grid.items.add (GridItem (c).withColumn (0).withRow (0).withMargin (10));
    grid.performLayout (Rectangle<float> (0, 0, 400, 200));

    expectGridBounds (c, 10, 10, 180, 80);
}

// =============================================================================
// auto-placement
// =============================================================================

TEST (GridTests, AutoPlacementFlowsItemsIntoImplicitTracks)
{
    Grid grid;

    Component c1, c2, c3;
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (c2));
    grid.items.add (GridItem (c3));
    grid.performLayout (Rectangle<float> (0, 0, 400, 200));

    // No column template: items flow left to right in a growing single row
    expectGridBounds (c1, 0, 0, 100, 40);
    expectGridBounds (c2, 100, 0, 100, 40);
    expectGridBounds (c3, 200, 0, 100, 40);
}

TEST (GridTests, AutoPlacementWrapsAtTheTemplateColumnCount)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateColumns.add (Grid::TrackInfo::px (100));

    Component c1, c2, c3;
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (c2));
    grid.items.add (GridItem (c3));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    expectGridBounds (c1, 0, 0, 100, 40);
    expectGridBounds (c2, 100, 0, 100, 40);
    expectGridBounds (c3, 0, 40, 100, 40);
}

TEST (GridTests, AutoPlacementAvoidsExplicitlyPlacedItems)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateColumns.add (Grid::TrackInfo::px (100));

    Component c0, c1, c2;
    grid.items.add (GridItem (c0).withColumn (0).withRow (0));
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (c2));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    expectGridBounds (c0, 0, 0, 100, 40);
    expectGridBounds (c1, 100, 0, 100, 40);
    expectGridBounds (c2, 200, 0, 100, 40);
}

TEST (GridTests, AutoPlacementSkipsCellsOccupiedBySpans)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateColumns.add (Grid::TrackInfo::px (100));

    Component c0, c1;
    grid.items.add (GridItem (c0).withColumn (0).withRow (0).withColumnSpan (2));
    grid.items.add (GridItem (c1));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    expectGridBounds (c0, 0, 0, 200, 40);
    expectGridBounds (c1, 200, 0, 100, 40);
}

TEST (GridTests, AutoPlacementRespectsRowSpans)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateColumns.add (Grid::TrackInfo::px (100));

    Component c0, c1;
    grid.items.add (GridItem (c0).withRowSpan (2));
    grid.items.add (GridItem (c1));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    // c0 occupies rows 0-1 of column 0, so c1 lands in column 1
    expectGridBounds (c0, 0, 0, 100, 80);
    expectGridBounds (c1, 100, 0, 100, 40);
}

TEST (GridTests, AutoPlacementContinuesOnANewRowWhenTheRowIsFull)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateColumns.add (Grid::TrackInfo::px (100));

    Component c0, c1;
    grid.items.add (GridItem (c0).withColumn (0).withRow (0).withColumnSpan (2));
    grid.items.add (GridItem (c1));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    expectGridBounds (c0, 0, 0, 200, 40);
    expectGridBounds (c1, 0, 40, 100, 40);
}

TEST (GridTests, AutoPlacementWrapsWithSingleColumnTemplate)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::px (100));

    Component c1, c2;
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (c2));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    // With a single template column, auto items stack into successive rows
    expectGridBounds (c1, 0, 0, 100, 40);
    expectGridBounds (c2, 0, 40, 100, 40);
}

// =============================================================================
// edge cases
// =============================================================================

TEST (GridTests, NullComponentItemsStillOccupyCells)
{
    Grid grid;

    Component c1, c2;
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (nullptr));
    grid.items.add (GridItem (c2));
    grid.performLayout (Rectangle<float> (0, 0, 400, 200));

    // The null item occupies the middle cell, pushing c2 to the third column
    expectGridBounds (c1, 0, 0, 100, 40);
    expectGridBounds (c2, 200, 0, 100, 40);
}

TEST (GridTests, EmptyItemListDoesNothing)
{
    Grid grid;
    EXPECT_NO_THROW (grid.performLayout (Rectangle<float> (0, 0, 100, 100)));
}

// =============================================================================
// regressions: fr tracks and the gaps they have to share the container with
// =============================================================================

TEST (GridTests, FrTracksAccountForColumnGap)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::fr (1));
    grid.templateColumns.add (Grid::TrackInfo::fr (1));
    grid.templateColumns.add (Grid::TrackInfo::fr (1));
    grid.templateRows.add (Grid::TrackInfo::px (50));
    grid.columnGap = 30.0f;

    Component c1, c2, c3;
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (c2));
    grid.items.add (GridItem (c3));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    // The two 30px gaps come off the top: (300 - 60) / 3 = 80 per track, so the
    // grid ends exactly on the container's right edge instead of 60px past it.
    expectGridBounds (c1, 0, 0, 80, 50);
    expectGridBounds (c2, 110, 0, 80, 50);
    expectGridBounds (c3, 220, 0, 80, 50);
}

TEST (GridTests, FrTracksAccountForRowGap)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateRows.add (Grid::TrackInfo::fr (1));
    grid.templateRows.add (Grid::TrackInfo::fr (1));
    grid.templateRows.add (Grid::TrackInfo::fr (1));
    grid.rowGap = 20.0f;

    Component c1, c2, c3;
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (c2));
    grid.items.add (GridItem (c3));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    // (200 - 40) / 3 per row
    expectGridBounds (c1, 0, 0, 100, 53.33333f);
    expectGridBounds (c2, 0, 73.33333f, 100, 53.33333f);
    expectGridBounds (c3, 0, 146.66667f, 100, 53.33333f);
}

TEST (GridTests, FrTracksInTemplateRowsShareTheRemainingHeight)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::px (150));
    grid.templateColumns.add (Grid::TrackInfo::px (150));
    grid.templateRows.add (Grid::TrackInfo::fr (1));
    grid.templateRows.add (Grid::TrackInfo::fr (2));
    grid.rowGap = 10.0f;

    Component c1, c2, c3, c4;
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (c2));
    grid.items.add (GridItem (c3));
    grid.items.add (GridItem (c4));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    // 190 left after the gap: 1fr = 63.33, 2fr = 126.67
    expectGridBounds (c1, 0, 0, 150, 63.33333f);
    expectGridBounds (c2, 150, 0, 150, 63.33333f);
    expectGridBounds (c3, 0, 73.33333f, 150, 126.66667f);
    expectGridBounds (c4, 150, 73.33333f, 150, 126.66667f);
}

TEST (GridTests, FrTracksCollapseWhenFixedTracksAndGapsOverflow)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::px (200));
    grid.templateColumns.add (Grid::TrackInfo::px (200));
    grid.templateColumns.add (Grid::TrackInfo::fr (1));
    grid.templateRows.add (Grid::TrackInfo::px (50));
    grid.columnGap = 20.0f;

    Component c1, c2, c3;
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (c2));
    grid.items.add (GridItem (c3));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    // The fixed tracks alone already overflow, so the fr track gets nothing
    // rather than a negative size.
    expectGridBounds (c1, 0, 0, 200, 50);
    expectGridBounds (c2, 220, 0, 200, 50);
    expectGridBounds (c3, 440, 0, 0, 50);
}

// =============================================================================
// regressions: placement
// =============================================================================

TEST (GridTests, PartialPlacementKeepsTheExplicitRow)
{
    Grid grid;

    for (int i = 0; i < 3; ++i)
        grid.templateColumns.add (Grid::TrackInfo::px (100));

    for (int i = 0; i < 3; ++i)
        grid.templateRows.add (Grid::TrackInfo::px (50));

    Component c1, c2, c3;
    grid.items.add (GridItem (c1).withRow (2));                          // row pinned, column auto
    grid.items.add (GridItem (c2));                                      // both auto
    grid.items.add (GridItem (c3).withRow (2));                          // row pinned, column auto
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    // The pinned row must survive: an item with only one definite axis used to
    // have both thrown away and be auto-placed from the cursor.
    expectGridBounds (c1, 0, 100, 100, 50);
    expectGridBounds (c2, 0, 0, 100, 50);
    expectGridBounds (c3, 100, 100, 100, 50);
}

TEST (GridTests, PartialPlacementKeepsTheExplicitColumn)
{
    Grid grid;

    for (int i = 0; i < 3; ++i)
        grid.templateColumns.add (Grid::TrackInfo::px (100));

    for (int i = 0; i < 3; ++i)
        grid.templateRows.add (Grid::TrackInfo::px (50));

    Component c1, c2, c3;
    grid.items.add (GridItem (c1).withColumn (2));
    grid.items.add (GridItem (c2).withColumn (2));
    grid.items.add (GridItem (c3));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    // Both pinned items stay in column 2 and stack downwards; the free item
    // continues from the cursor.
    expectGridBounds (c1, 200, 0, 100, 50);
    expectGridBounds (c2, 200, 50, 100, 50);
    expectGridBounds (c3, 0, 100, 100, 50);
}

TEST (GridTests, AutoPlacementAvoidsLaterExplicitItems)
{
    Grid grid;

    for (int i = 0; i < 3; ++i)
        grid.templateColumns.add (Grid::TrackInfo::px (100));

    grid.templateRows.add (Grid::TrackInfo::px (50));
    grid.templateRows.add (Grid::TrackInfo::px (50));

    Component c1, c2, c3;
    grid.items.add (GridItem (c1));                     // auto, added first
    grid.items.add (GridItem (c2).withColumn (0).withRow (0)); // explicit, added later
    grid.items.add (GridItem (c3));

    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    // Every explicit position is reserved before anything flows, so the auto
    // item cannot take the cell c2 owns even though it is placed first.
    expectGridBounds (c1, 100, 0, 100, 50);
    expectGridBounds (c2, 0, 0, 100, 50);
    expectGridBounds (c3, 200, 0, 100, 50);
}

TEST (GridTests, AutoPlacementAvoidsLaterExplicitSpans)
{
    Grid grid;

    for (int i = 0; i < 3; ++i)
        grid.templateColumns.add (Grid::TrackInfo::px (100));

    grid.templateRows.add (Grid::TrackInfo::px (50));
    grid.templateRows.add (Grid::TrackInfo::px (50));

    Component c1, c2, c3;
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (c2));
    grid.items.add (GridItem (c3).withColumn (0).withRow (0).withColumnSpan (2));

    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    expectGridBounds (c1, 200, 0, 100, 50);
    expectGridBounds (c2, 0, 50, 100, 50);
    expectGridBounds (c3, 0, 0, 200, 50);
}

TEST (GridTests, AutoPlaceConstantMatchesTheDefaultPosition)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateColumns.add (Grid::TrackInfo::px (100));

    Component c1, c2;
    grid.items.add (GridItem (c1).withColumn (GridItem::autoPlace).withRow (GridItem::autoPlace));
    grid.items.add (GridItem (c2));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    EXPECT_EQ (-1, GridItem::autoPlace);
    expectGridBounds (c1, 0, 0, 100, 40);
    expectGridBounds (c2, 100, 0, 100, 40);
}

// =============================================================================
// regressions: degenerate cells
// =============================================================================

TEST (GridTests, MarginsLargerThanTheCellClampToZero)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::px (40));
    grid.templateRows.add (Grid::TrackInfo::px (30));

    Component c;
    grid.items.add (GridItem (c).withColumn (0).withRow (0).withMargin (50));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    // The margins exceed the cell on both axes; the size must not go negative.
    EXPECT_GE (c.getWidth(), 0.0f);
    EXPECT_GE (c.getHeight(), 0.0f);
    expectGridBounds (c, 50, 50, 0, 0);
}

TEST (GridTests, ZeroSizedTargetAreaDoesNotProduceNegativeSizes)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::fr (1));
    grid.templateRows.add (Grid::TrackInfo::fr (1));

    Component c;
    grid.items.add (GridItem (c));
    EXPECT_NO_THROW (grid.performLayout (Rectangle<float> (0, 0, 0, 0)));

    EXPECT_GE (c.getWidth(), 0.0f);
    EXPECT_GE (c.getHeight(), 0.0f);
}

TEST (GridTests, SpanBeyondTheTemplateCreatesImplicitTracks)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.autoColumns = 60.0f;
    grid.templateRows.add (Grid::TrackInfo::px (50));

    Component c;
    grid.items.add (GridItem (c).withColumn (1).withRow (0).withColumnSpan (3));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    // Columns 2 and 3 are implicit at autoColumns: 100 + 60 + 60 = 220
    expectGridBounds (c, 100, 0, 220, 50);
}

TEST (GridTests, LayoutIsIdempotent)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::fr (1));
    grid.templateColumns.add (Grid::TrackInfo::fr (2));
    grid.templateRows.add (Grid::TrackInfo::px (50));
    grid.columnGap = 15.0f;

    Component c1, c2;
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (c2));

    const Rectangle<float> area (0, 0, 300, 200);

    grid.performLayout (area);
    const auto first1 = c1.getBounds();
    const auto first2 = c2.getBounds();

    grid.performLayout (area);

    EXPECT_EQ (first1, c1.getBounds());
    EXPECT_EQ (first2, c2.getBounds());
}

TEST (GridTests, IntegerPerformLayoutMatchesTheFloatOverload)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateRows.add (Grid::TrackInfo::px (50));

    Component c1, c2;
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (c2));
    grid.performLayout (Rectangle<int> (10, 20, 300, 200));

    expectGridBounds (c1, 10, 20, 100, 50);
    expectGridBounds (c2, 110, 20, 100, 50);
}

TEST (GridTests, ComponentConvertsImplicitlyToAGridItem)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateRows.add (Grid::TrackInfo::px (50));

    Component c1, c2;
    grid.items.add (c1);
    grid.items.add (&c2);
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    expectGridBounds (c1, 0, 0, 100, 50);
    expectGridBounds (c2, 100, 0, 100, 50);
}

// =============================================================================
// minmax, percent and repeat tracks
// =============================================================================

TEST (GridTests, MinmaxGrowsToItsMaximum)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::minmax (Grid::TrackInfo::px (50), Grid::TrackInfo::px (200)));
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateRows.add (Grid::TrackInfo::px (50));

    Component c1, c2;
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (c2));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    // 150 of leftover grows the flexible-range track up to its 200 ceiling
    expectGridBounds (c1, 0, 0, 200, 50);
    expectGridBounds (c2, 200, 0, 100, 50);
}

TEST (GridTests, MinmaxStaysAtItsMinimumWhenCramped)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::minmax (Grid::TrackInfo::px (50), Grid::TrackInfo::px (200)));
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateRows.add (Grid::TrackInfo::px (50));

    Component c1, c2;
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (c2));
    grid.performLayout (Rectangle<float> (0, 0, 120, 200));

    expectGridBounds (c1, 0, 0, 50, 50);
    expectGridBounds (c2, 50, 0, 100, 50);
}

TEST (GridTests, MinmaxWithFrFreezesAtItsMinimum)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::minmax (Grid::TrackInfo::px (100), Grid::TrackInfo::fr (1)));
    grid.templateColumns.add (Grid::TrackInfo::minmax (Grid::TrackInfo::px (100), Grid::TrackInfo::fr (1)));
    grid.templateColumns.add (Grid::TrackInfo::fr (1));
    grid.templateRows.add (Grid::TrackInfo::px (50));

    Component c1, c2, c3;
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (c2));
    grid.items.add (GridItem (c3));
    grid.performLayout (Rectangle<float> (0, 0, 250, 200));

    // An equal split would give 83.3 each, which is below the 100 floor of the
    // first two: they freeze at 100 and the third takes what is left.
    expectGridBounds (c1, 0, 0, 100, 50);
    expectGridBounds (c2, 100, 0, 100, 50);
    expectGridBounds (c3, 200, 0, 50, 50);
}

TEST (GridTests, MinmaxWithFrSharesEquallyWhenRoomy)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::minmax (Grid::TrackInfo::px (100), Grid::TrackInfo::fr (1)));
    grid.templateColumns.add (Grid::TrackInfo::minmax (Grid::TrackInfo::px (100), Grid::TrackInfo::fr (1)));
    grid.templateColumns.add (Grid::TrackInfo::fr (1));
    grid.templateRows.add (Grid::TrackInfo::px (50));

    Component c1, c2, c3;
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (c2));
    grid.items.add (GridItem (c3));
    grid.performLayout (Rectangle<float> (0, 0, 600, 200));

    expectGridBounds (c1, 0, 0, 200, 50);
    expectGridBounds (c2, 200, 0, 200, 50);
    expectGridBounds (c3, 400, 0, 200, 50);
}

TEST (GridTests, PercentTracksResolveAgainstTheContainer)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::percent (25));
    grid.templateColumns.add (Grid::TrackInfo::percent (50));
    grid.templateColumns.add (Grid::TrackInfo::percent (25));
    grid.templateRows.add (Grid::TrackInfo::px (50));

    Component c1, c2, c3;
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (c2));
    grid.items.add (GridItem (c3));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    expectGridBounds (c1, 0, 0, 75, 50);
    expectGridBounds (c2, 75, 0, 150, 50);
    expectGridBounds (c3, 225, 0, 75, 50);
}

TEST (GridTests, PercentTracksIgnoreTheGap)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::percent (25));
    grid.templateColumns.add (Grid::TrackInfo::percent (25));
    grid.templateRows.add (Grid::TrackInfo::px (50));
    grid.columnGap = 20.0f;

    Component c1, c2;
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (c2));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    // Unlike fr, a percentage is of the container's full size - the gap does
    // not come off first.
    expectGridBounds (c1, 0, 0, 75, 50);
    expectGridBounds (c2, 95, 0, 75, 50);
}

TEST (GridTests, RepeatExpandsToIdenticalTracks)
{
    Grid grid;
    grid.templateColumns.addArray (Grid::repeat (3, Grid::TrackInfo::fr (1)));
    grid.templateRows.add (Grid::TrackInfo::px (50));
    grid.columnGap = 15.0f;

    EXPECT_EQ (3, grid.templateColumns.size());

    Component c1, c2, c3;
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (c2));
    grid.items.add (GridItem (c3));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    expectGridBounds (c1, 0, 0, 90, 50);
    expectGridBounds (c2, 105, 0, 90, 50);
    expectGridBounds (c3, 210, 0, 90, 50);
}

TEST (GridTests, RepeatWithZeroCountAddsNothing)
{
    auto tracks = Grid::repeat (0, Grid::TrackInfo::px (10));
    EXPECT_TRUE (tracks.isEmpty());
}

TEST (GridTests, AutoFillCountsWhatFits)
{
    Grid grid;
    grid.templateColumns.addArray (Grid::repeatToFill (Grid::TrackInfo::px (100), 350.0f, 0.0f, 100.0f));
    grid.templateRows.add (Grid::TrackInfo::px (50));

    EXPECT_EQ (3, grid.templateColumns.size());

    Component c1, c2, c3;
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (c2));
    grid.items.add (GridItem (c3));
    grid.performLayout (Rectangle<float> (0, 0, 350, 200));

    expectGridBounds (c1, 0, 0, 100, 50);
    expectGridBounds (c2, 100, 0, 100, 50);
    expectGridBounds (c3, 200, 0, 100, 50);
}

TEST (GridTests, AutoFillAccountsForTheGap)
{
    Grid grid;
    grid.columnGap = 10.0f;
    grid.templateColumns.addArray (Grid::repeatToFill (Grid::TrackInfo::px (100), 350.0f, 10.0f, 100.0f));
    grid.templateRows.add (Grid::TrackInfo::px (50));

    // (350 + 10) / (100 + 10) = 3.27, so three tracks fit
    EXPECT_EQ (3, grid.templateColumns.size());

    Component c1, c2, c3;
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (c2));
    grid.items.add (GridItem (c3));
    grid.performLayout (Rectangle<float> (0, 0, 350, 200));

    expectGridBounds (c1, 0, 0, 100, 50);
    expectGridBounds (c2, 110, 0, 100, 50);
    expectGridBounds (c3, 220, 0, 100, 50);
}

TEST (GridTests, AutoFillAlwaysProducesAtLeastOneTrack)
{
    auto tracks = Grid::repeatToFill (Grid::TrackInfo::px (500), 100.0f, 0.0f, 100.0f);
    EXPECT_EQ (1, tracks.size());

    // A track with no minimum would otherwise repeat without bound
    auto zeroSized = Grid::repeatToFill (Grid::TrackInfo::fr (1), 100.0f, 0.0f, 100.0f);
    EXPECT_EQ (1, zeroSized.size());
}

TEST (GridTests, FitContentClampsTheTrack)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::fitContent (60));
    grid.templateColumns.add (Grid::TrackInfo::fr (1));
    grid.templateRows.add (Grid::TrackInfo::px (50));

    Component c1, c2;
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (c2));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    // The stub resolves fit-content(60) as minmax(0, 60): it grows to its
    // ceiling because there is no content measurement to stop it earlier.
    expectGridBounds (c1, 0, 0, 60, 50);
    expectGridBounds (c2, 60, 0, 240, 50);
}

// =============================================================================
// grid-auto-flow
// =============================================================================

TEST (GridTests, ColumnFlowFillsColumnsFirst)
{
    Grid grid;
    grid.autoFlow = Grid::AutoFlow::column;
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateRows.add (Grid::TrackInfo::px (50));
    grid.templateRows.add (Grid::TrackInfo::px (50));

    Component c1, c2, c3, c4;
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (c2));
    grid.items.add (GridItem (c3));
    grid.items.add (GridItem (c4));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    expectGridBounds (c1, 0, 0, 100, 50);
    expectGridBounds (c2, 0, 50, 100, 50);
    expectGridBounds (c3, 100, 0, 100, 50);
    expectGridBounds (c4, 100, 50, 100, 50);
}

TEST (GridTests, SparseFlowLeavesAHoleThatDenseFlowFills)
{
    auto build = [] (Grid& grid, Grid::AutoFlow flow, Component& a, Component& b, Component& c)
    {
        grid.autoFlow = flow;

        for (int i = 0; i < 3; ++i)
            grid.templateColumns.add (Grid::TrackInfo::px (100));

        grid.templateRows.add (Grid::TrackInfo::px (50));
        grid.templateRows.add (Grid::TrackInfo::px (50));

        grid.items.add (GridItem (a));
        grid.items.add (GridItem (b).withColumnSpan (3));
        grid.items.add (GridItem (c));
    };

    Component s1, s2, s3;
    Grid sparse;
    build (sparse, Grid::AutoFlow::row, s1, s2, s3);
    sparse.performLayout (Rectangle<float> (0, 0, 300, 200));

    // The wide item cannot fit next to the first, so it starts a new row and
    // the sparse cursor never goes back for the gap it left behind.
    expectGridBounds (s1, 0, 0, 100, 50);
    expectGridBounds (s2, 0, 50, 300, 50);
    expectGridBounds (s3, 0, 100, 100, 40);

    Component d1, d2, d3;
    Grid dense;
    build (dense, Grid::AutoFlow::rowDense, d1, d2, d3);
    dense.performLayout (Rectangle<float> (0, 0, 300, 200));

    expectGridBounds (d1, 0, 0, 100, 50);
    expectGridBounds (d2, 0, 50, 300, 50);
    expectGridBounds (d3, 100, 0, 100, 50); // backfilled
}

TEST (GridTests, ColumnDenseFlowBackfills)
{
    Grid grid;
    grid.autoFlow = Grid::AutoFlow::columnDense;
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateColumns.add (Grid::TrackInfo::px (100));

    for (int i = 0; i < 3; ++i)
        grid.templateRows.add (Grid::TrackInfo::px (50));

    Component c1, c2, c3;
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (c2).withRowSpan (3));
    grid.items.add (GridItem (c3));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    expectGridBounds (c1, 0, 0, 100, 50);
    expectGridBounds (c2, 100, 0, 100, 150);
    expectGridBounds (c3, 0, 50, 100, 50);
}

// =============================================================================
// container justify-content / align-content
// =============================================================================

TEST (GridTests, JustifyContentCentersTheTracks)
{
    Grid grid;
    grid.justifyContent = Grid::AlignContent::center;

    for (int i = 0; i < 3; ++i)
        grid.templateColumns.add (Grid::TrackInfo::px (60));

    grid.templateRows.add (Grid::TrackInfo::px (50));

    Component c1, c2, c3;
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (c2));
    grid.items.add (GridItem (c3));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    expectGridBounds (c1, 60, 0, 60, 50);
    expectGridBounds (c2, 120, 0, 60, 50);
    expectGridBounds (c3, 180, 0, 60, 50);
}

TEST (GridTests, JustifyContentSpaceAroundTheTracks)
{
    Grid grid;
    grid.justifyContent = Grid::AlignContent::spaceAround;

    for (int i = 0; i < 3; ++i)
        grid.templateColumns.add (Grid::TrackInfo::px (60));

    grid.templateRows.add (Grid::TrackInfo::px (50));

    Component c1, c2, c3;
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (c2));
    grid.items.add (GridItem (c3));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    expectGridBounds (c1, 20, 0, 60, 50);
    expectGridBounds (c2, 120, 0, 60, 50);
    expectGridBounds (c3, 220, 0, 60, 50);
}

TEST (GridTests, AlignContentCentersTheRows)
{
    Grid grid;
    grid.alignContent = Grid::AlignContent::center;
    grid.templateColumns.add (Grid::TrackInfo::px (100));

    for (int i = 0; i < 3; ++i)
        grid.templateRows.add (Grid::TrackInfo::px (30));

    Component c1, c2, c3;
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (c2));
    grid.items.add (GridItem (c3));
    grid.performLayout (Rectangle<float> (0, 0, 300, 240));

    expectGridBounds (c1, 0, 75, 100, 30);
    expectGridBounds (c2, 0, 105, 100, 30);
    expectGridBounds (c3, 0, 135, 100, 30);
}

// =============================================================================
// named lines and template areas
// =============================================================================

TEST (GridTests, NamedLinesPlaceItems)
{
    Grid grid;

    for (int i = 0; i < 3; ++i)
        grid.templateColumns.add (Grid::TrackInfo::px (100));

    grid.templateRows.add (Grid::TrackInfo::px (50));
    grid.templateRows.add (Grid::TrackInfo::px (50));

    grid.setColumnLineName (0, "left");
    grid.setColumnLineName (1, "mid");
    grid.setColumnLineName (2, "right");
    grid.setRowLineName (0, "top");
    grid.setRowLineName (1, "bottom");

    Component c1, c2, c3;
    grid.items.add (GridItem (c1).withColumnStart ("mid").withRowStart ("top"));
    grid.items.add (GridItem (c2).withColumnStart ("left").withRowStart ("bottom"));
    grid.items.add (GridItem (c3).withColumnStart ("right").withRowStart ("bottom"));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    expectGridBounds (c1, 100, 0, 100, 50);
    expectGridBounds (c2, 0, 50, 100, 50);
    expectGridBounds (c3, 200, 50, 100, 50);
}

TEST (GridTests, UnknownLineNameFallsBackToAutoPlacement)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateRows.add (Grid::TrackInfo::px (50));

    Component c1, c2;
    grid.items.add (GridItem (c1).withColumnStart ("nope"));
    grid.items.add (GridItem (c2));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    expectGridBounds (c1, 0, 0, 100, 50);
    expectGridBounds (c2, 100, 0, 100, 50);
}

TEST (GridTests, TemplateAreasPlaceItems)
{
    Grid grid;

    for (int i = 0; i < 3; ++i)
        grid.templateColumns.add (Grid::TrackInfo::px (100));

    for (int i = 0; i < 3; ++i)
        grid.templateRows.add (Grid::TrackInfo::px (50));

    const auto result = grid.setTemplateAreas ({ "header header header",
                                                 "side   main   main",
                                                 "side   foot   foot" });
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    Component header, side, main, foot;
    grid.items.add (GridItem (header).withArea ("header"));
    grid.items.add (GridItem (side).withArea ("side"));
    grid.items.add (GridItem (main).withArea ("main"));
    grid.items.add (GridItem (foot).withArea ("foot"));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    expectGridBounds (header, 0, 0, 300, 50);
    expectGridBounds (side, 0, 50, 100, 100);
    expectGridBounds (main, 100, 50, 200, 50);
    expectGridBounds (foot, 100, 100, 200, 50);
}

TEST (GridTests, TemplateAreasRejectRaggedRows)
{
    Grid grid;
    const auto result = grid.setTemplateAreas ({ "a a", "b b b" });

    EXPECT_FALSE (result.wasOk());
    EXPECT_TRUE (grid.getTemplateAreaNames().isEmpty());
}

TEST (GridTests, TemplateAreasRejectNonRectangularAreas)
{
    Grid grid;
    // "a" would have to be an L shape, which CSS does not allow
    const auto result = grid.setTemplateAreas ({ "a a", "a .", "a a" });

    EXPECT_FALSE (result.wasOk());
    EXPECT_TRUE (grid.getTemplateAreaNames().isEmpty());
}

TEST (GridTests, TemplateAreasLeaveDotCellsFreeForAutoPlacement)
{
    Grid grid;

    for (int i = 0; i < 3; ++i)
        grid.templateColumns.add (Grid::TrackInfo::px (100));

    grid.templateRows.add (Grid::TrackInfo::px (50));
    grid.templateRows.add (Grid::TrackInfo::px (50));

    ASSERT_TRUE (grid.setTemplateAreas ({ "head head head", ". . ." }).wasOk());

    Component head, a, b;
    grid.items.add (GridItem (head).withArea ("head"));
    grid.items.add (GridItem (a));
    grid.items.add (GridItem (b));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    expectGridBounds (head, 0, 0, 300, 50);
    expectGridBounds (a, 0, 50, 100, 50);
    expectGridBounds (b, 100, 50, 100, 50);
}

TEST (GridTests, ClearingTemplateAreasRemovesThem)
{
    Grid grid;
    ASSERT_TRUE (grid.setTemplateAreas ({ "a b" }).wasOk());
    EXPECT_EQ (2, grid.getTemplateAreaNames().size());

    grid.clearTemplateAreas();
    EXPECT_TRUE (grid.getTemplateAreaNames().isEmpty());
}

TEST (GridTests, UnknownAreaNameFallsBackToAutoPlacement)
{
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateRows.add (Grid::TrackInfo::px (50));

    Component c1, c2;
    grid.items.add (GridItem (c1).withArea ("missing"));
    grid.items.add (GridItem (c2));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    expectGridBounds (c1, 0, 0, 100, 50);
    expectGridBounds (c2, 100, 0, 100, 50);
}

// =============================================================================
// baseline alignment
// =============================================================================

TEST (GridTests, BaselineAlignsItemsInTheSameRow)
{
    Grid grid;
    grid.alignItems = Grid::AlignItems::baseline;
    grid.templateColumns.add (Grid::TrackInfo::px (150));
    grid.templateColumns.add (Grid::TrackInfo::px (150));
    grid.templateRows.add (Grid::TrackInfo::px (80));

    Component c1, c2;
    grid.items.add (GridItem (c1).withHeight (20));
    grid.items.add (GridItem (c2).withHeight (50));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    // With nothing to measure the baseline is the item's bottom edge, so the
    // shorter item drops to line up with the taller one.
    expectGridBounds (c1, 0, 30, 150, 20);
    expectGridBounds (c2, 150, 0, 150, 50);
}

// =============================================================================
// gap shorthand
// =============================================================================

TEST (GridTests, GapShorthandFillsBothAxes)
{
    Grid grid;
    grid.gap = 10.0f;
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateRows.add (Grid::TrackInfo::px (50));
    grid.templateRows.add (Grid::TrackInfo::px (50));

    Component c1, c2, c3, c4;
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (c2));
    grid.items.add (GridItem (c3));
    grid.items.add (GridItem (c4));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    expectGridBounds (c1, 0, 0, 100, 50);
    expectGridBounds (c2, 110, 0, 100, 50);
    expectGridBounds (c3, 0, 60, 100, 50);
    expectGridBounds (c4, 110, 60, 100, 50);
}

TEST (GridTests, ColumnGapOverridesTheGapShorthand)
{
    Grid grid;
    grid.gap = 10.0f;
    grid.columnGap = 40.0f;
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateColumns.add (Grid::TrackInfo::px (100));
    grid.templateRows.add (Grid::TrackInfo::px (50));
    grid.templateRows.add (Grid::TrackInfo::px (50));

    Component c1, c2, c3, c4;
    grid.items.add (GridItem (c1));
    grid.items.add (GridItem (c2));
    grid.items.add (GridItem (c3));
    grid.items.add (GridItem (c4));
    grid.performLayout (Rectangle<float> (0, 0, 300, 200));

    expectGridBounds (c1, 0, 0, 100, 50);
    expectGridBounds (c2, 140, 0, 100, 50);
    expectGridBounds (c3, 0, 60, 100, 50);
    expectGridBounds (c4, 140, 60, 100, 50);
}
