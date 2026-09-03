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
/**
    A CSS-grid-style layout container for arranging components.

    Grid provides a two-dimensional layout system based on rows and columns.
    Items are placed using explicit row/column positions with spans, similar
    to CSS Grid Layout.

    Usage:
    @code
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo (Grid::Fr (1)));
    grid.templateColumns.add (Grid::TrackInfo (Grid::Fr (2)));
    grid.templateRows.add (Grid::TrackInfo (50));
    grid.templateRows.add (Grid::TrackInfo (Grid::Fr (1)));
    grid.items.add (component1.withColumn (0).withRow (0));
    grid.items.add (component2.withColumn (1).withRow (0).withRowSpan (2));
    grid.performLayout (getLocalBounds());
    @endcode

    @see GridItem

    @tags{GUI}
*/
class YUP_API Grid
{
public:
    //==============================================================================
    /** Represents a grid track (row or column) sizing specification. */
    struct YUP_API TrackInfo
    {
        /** Creates a track with a fixed pixel size. */
        static TrackInfo px (float pixelSize);

        /** Creates a track with a fractional size (fr unit). */
        static TrackInfo fr (float fraction);

        /** Creates a track with auto sizing. */
        static TrackInfo auto_();

        /** The pixel size for fixed tracks. */
        float pixelSize = 0.0f;

        /** The fractional size (fr units). */
        float fraction = 0.0f;

        /** Whether this track uses auto-sizing. */
        bool isAuto = false;

    private:
        TrackInfo() = default;
    };

    //==============================================================================
    /** Alignment of items within cells. */
    enum class AlignItems
    {
        flexStart,
        flexEnd,
        center,
        stretch
    };

    //==============================================================================
    Grid() = default;

    //==============================================================================
    /** Column track definitions. */
    Array<TrackInfo> templateColumns;

    /** Row track definitions. */
    Array<TrackInfo> templateRows;

    /** Auto-generated row height (when templateRows is empty). */
    float autoRows = 40.0f;

    /** Auto-generated column width (when templateColumns is empty). */
    float autoColumns = 100.0f;

    /** Gap between columns. */
    float columnGap = 0.0f;

    /** Gap between rows. */
    float rowGap = 0.0f;

    /** Default horizontal alignment for items. */
    AlignItems justifyItems = AlignItems::stretch;

    /** Default vertical alignment for items. */
    AlignItems alignItems = AlignItems::stretch;

    //==============================================================================
    /** The items to be laid out. */
    Array<GridItem> items;

    //==============================================================================
    /**
        Performs the grid layout, positioning child components within the
        given rectangle.

        @param targetArea   the area in which to lay out the items.
    */
    void performLayout (Rectangle<float> targetArea);

    /**
        Performs the grid layout using an integer rectangle.
    */
    void performLayout (Rectangle<int> targetArea);

private:
    //==============================================================================
    Array<float> calculateTrackSizes (const Array<TrackInfo>& tracks, float totalSize, float defaultSize);
};

} // namespace yup
