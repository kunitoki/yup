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

void expectGridBounds (const Component& component, float x, float y, float width, float height)
{
    auto bounds = component.getBounds();
    EXPECT_FLOAT_EQ (x, bounds.getX());
    EXPECT_FLOAT_EQ (y, bounds.getY());
    EXPECT_FLOAT_EQ (width, bounds.getWidth());
    EXPECT_FLOAT_EQ (height, bounds.getHeight());
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
