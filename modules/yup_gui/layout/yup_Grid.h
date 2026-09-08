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
    to CSS Grid Layout. Items whose column and/or row are left at
    GridItem::autoPlace are positioned automatically, flowing row by row into
    the first free cell; implicit tracks are created as needed to fit every
    item.

    Placement runs in the CSS order: every explicitly positioned item is
    recorded first, then items locked to a row pick a column within it, then
    the remainder flows from a cursor. An auto-placed item therefore never
    lands on a cell that an explicitly placed item further down the list owns.

    Note that grid lines are 0-based here, whereas CSS numbers them from 1.

    Usage:
    @code
    Grid grid;
    grid.templateColumns.add (Grid::TrackInfo::fr (1));
    grid.templateColumns.add (Grid::TrackInfo::fr (2));
    grid.templateRows.add (Grid::TrackInfo::px (50));
    grid.templateRows.add (Grid::TrackInfo::fr (1));
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
    /**
        A grid track (row or column) sizing specification.

        A track is a min/max pair, exactly as in CSS: the minimum is the size
        the track will never go below, and the maximum is the size it is allowed
        to grow to. The familiar single-value forms are just pairs -
        `px (100)` is `minmax(100px, 100px)` and `fr (1)` is
        `minmax(0, 1fr)` - so one code path sizes every kind of track.

        Instances are only obtainable from the factory functions below.
    */
    struct YUP_API TrackInfo
    {
        /** How one half of a track's sizing is expressed. */
        enum class SizeType
        {
            pixels,   /**< An absolute length. */
            percent,  /**< A percentage of the grid's size on that axis. */
            fraction, /**< A share of the space left over (the `fr` unit). */
            autoSize  /**< The container's autoRows / autoColumns size. */
        };

        /** One half of a track's sizing - its minimum or its maximum. */
        struct SizingFunction
        {
            SizeType type = SizeType::pixels;
            float value = 0.0f;
        };

        //==============================================================================
        /** Creates a track with a fixed pixel size. */
        static TrackInfo px (float pixelSize);

        /** Creates a track sized as a percentage of the grid's size on that axis.

            Percentages resolve against the container's full size, not against
            what is left after the gaps - so two 25% columns in a 300px grid are
            75px wide whatever the gap is. */
        static TrackInfo percent (float percentage);

        /** Creates a track with a fractional size (the `fr` unit).

            Fractional tracks share out whatever space is left after the fixed
            tracks and the gaps have been accounted for.

            Note the divergence from CSS: there, `1fr` means `minmax(auto, 1fr)`
            and so never shrinks below its contents. YUP has no way to measure
            an item's content yet, so the floor stays at 0 and the division is
            purely proportional. Use `minmax (px (n), fr (1))` when a floor
            matters. */
        static TrackInfo fr (float fraction);

        /** Creates a track sized by the container's autoRows / autoColumns.

            Note that this is a fixed size, not CSS's content-driven `auto`. */
        static TrackInfo auto_();

        /** Creates a track that will not go below `minimum` nor above `maximum`.

            The minimum's *min* sizing function and the maximum's *max* sizing
            function are taken, so `minmax (px (100), fr (1))` is a track that
            takes its share of the leftover space but never drops below 100px -
            the idiom `fr` alone cannot express. */
        static TrackInfo minmax (TrackInfo minimum, TrackInfo maximum);

        /** Creates a track clamped to at most `maximumSize`.

            Note the divergence from CSS: `fit-content()` is defined as
            `minmax(auto, max-content)` clamped by the argument, and both of
            those are content measurements YUP cannot make. This resolves as
            `minmax (px (0), px (maximumSize))`. */
        static TrackInfo fitContent (float maximumSize);

        //==============================================================================
        /** The size this track will not go below. */
        SizingFunction minimum;

        /** The size this track is allowed to grow to. */
        SizingFunction maximum;

        /** Returns true when the track takes a share of the leftover space. */
        bool isFractional() const noexcept { return maximum.type == SizeType::fraction; }

    private:
        TrackInfo() = default;
    };

    //==============================================================================
    /** Returns `count` copies of a track, for building a template.

        @code
        grid.templateColumns.addArray (Grid::repeat (3, Grid::TrackInfo::fr (1)));
        @endcode
    */
    static Array<TrackInfo> repeat (int count, TrackInfo track);

    /**
        Returns as many copies of a track as will fit in `availableSize`.

        This is CSS's `repeat(auto-fill, ...)`: the count is
        `floor((availableSize + gap) / (trackMinimum + gap))`, at least 1.

        Note that `auto-fit` is deliberately NOT offered as a separate function.
        It differs from `auto-fill` only by collapsing tracks that end up with
        no items in them, which requires knowing each track's content - so under
        the current sizing stub the two would be indistinguishable, and offering
        both would promise a difference that is not there.

        @param track          the track to repeat.
        @param availableSize  the grid's size on that axis.
        @param gap            the gap between tracks on that axis.
        @param defaultSize    the autoRows / autoColumns size, for `auto` tracks.
    */
    static Array<TrackInfo> repeatToFill (TrackInfo track,
                                          float availableSize,
                                          float gap,
                                          float defaultSize);

    //==============================================================================
    /** Alignment of items within their cells. */
    enum class AlignItems
    {
        flexStart, /**< Align to the start of the cell. */
        flexEnd,   /**< Align to the end of the cell. */
        center,    /**< Center within the cell. */
        stretch,   /**< Fill the cell. */
        baseline   /**< Align items in the same row by their baselines. */
    };

    /** Alignment of the whole grid when the tracks do not fill the container. */
    enum class AlignContent
    {
        flexStart,    /**< Pack the tracks at the start. */
        flexEnd,      /**< Pack the tracks at the end. */
        center,       /**< Center the tracks. */
        spaceBetween, /**< Even space between tracks, none at the edges. */
        spaceAround,  /**< A full space between tracks and a half space at each edge. */
        spaceEvenly   /**< An equal space between tracks and at both edges. */
    };

    /** The order automatically placed items are flowed in. */
    enum class AutoFlow
    {
        row,          /**< Fill each row before moving to the next. */
        column,       /**< Fill each column before moving to the next. */
        rowDense,     /**< As row, but backfill holes left by larger items. */
        columnDense   /**< As column, but backfill holes left by larger items. */
    };

    //==============================================================================
    Grid() = default;

    //==============================================================================
    /** Column track definitions. */
    Array<TrackInfo> templateColumns;

    /** Row track definitions. */
    Array<TrackInfo> templateRows;

    /** The height used for any row track that is not a fixed or fractional
        entry in templateRows.

        This covers implicit rows created past the end of the template to fit a
        placed item, and rows the template declares as TrackInfo::auto_().

        Note that this makes `auto_()` a synonym for `px (autoRows)` rather than
        CSS's content-driven `auto`: sizing a track to its contents needs a
        content-measurement hook that YUP does not have yet. */
    float autoRows = 40.0f;

    /** The width used for any column track that is not a fixed or fractional
        entry in templateColumns.

        This covers implicit columns created past the end of the template to fit
        a placed item, and columns the template declares as TrackInfo::auto_().

        Note that this makes `auto_()` a synonym for `px (autoColumns)` rather
        than CSS's content-driven `auto`: sizing a track to its contents needs a
        content-measurement hook that YUP does not have yet. */
    float autoColumns = 100.0f;

    /** The gap between both rows and columns, as in the CSS `gap` shorthand.

        This is only used for whichever of rowGap / columnGap is left unset. */
    float gap = 0.0f;

    /** Gap between columns. -1 (the default) means "use the gap shorthand".

        The gap is consumed before the fractional tracks divide up the
        remaining space, so an `fr` grid fits its container exactly. */
    float columnGap = -1.0f;

    /** Gap between rows. -1 (the default) means "use the gap shorthand".

        The gap is consumed before the fractional tracks divide up the
        remaining space, so an `fr` grid fits its container exactly. */
    float rowGap = -1.0f;

    /** Default horizontal alignment for items. */
    AlignItems justifyItems = AlignItems::stretch;

    /** Default vertical alignment for items. */
    AlignItems alignItems = AlignItems::stretch;

    /** How the columns are positioned when they do not fill the container.

        Without this there is no way to center a fixed-track grid in a larger
        box - the leftover space always stays at the right. */
    AlignContent justifyContent = AlignContent::flexStart;

    /** How the rows are positioned when they do not fill the container. */
    AlignContent alignContent = AlignContent::flexStart;

    /** The order automatically placed items flow in. Default is row. */
    AutoFlow autoFlow = AutoFlow::row;

    //==============================================================================
    /** The items to be laid out. */
    Array<GridItem> items;

    //==============================================================================
    /**
        Names the grid areas, as in CSS `grid-template-areas`.

        Each string is one row of whitespace-separated area names, and a `.`
        marks a cell that belongs to no area. An item then claims an area with
        GridItem::withArea(), which sets its position and span in one go.

        @code
        grid.setTemplateAreas ({ "header header",
                                 "side   main",
                                 "side   footer" });
        grid.items.add (GridItem (headerComponent).withArea ("header"));
        @endcode

        Every occurrence of a name must form a solid rectangle and every row
        must have the same number of cells, otherwise the call fails and the
        previous areas are left untouched.

        @param rowPatterns  one string per row of the grid.

        @returns ok, or a failure describing the first problem found.
    */
    Result setTemplateAreas (const StringArray& rowPatterns);

    /** Removes every area defined by setTemplateAreas(). */
    void clearTemplateAreas();

    /** Returns the area names currently defined. */
    StringArray getTemplateAreaNames() const;

    //==============================================================================
    /**
        Names a column line so items can be placed against it by name.

        Lines are numbered from 0, so line 0 is the leading edge of the first
        column and line n is the trailing edge of the last one. Note that CSS
        numbers its grid lines from 1; the conversion belongs here and nowhere
        else.

        @see GridItem::withColumnStart
    */
    void setColumnLineName (int lineIndex, const String& name);

    /** Names a row line so items can be placed against it by name.

        @see setColumnLineName, GridItem::withRowStart
    */
    void setRowLineName (int lineIndex, const String& name);

    /** Removes every column and row line name. */
    void clearLineNames();

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
    /** One rectangle named by setTemplateAreas(), already 0-based. */
    struct NamedArea
    {
        String name;
        int row = 0;
        int column = 0;
        int rowSpan = 1;
        int columnSpan = 1;
    };

    /** One line name from setColumnLineName() / setRowLineName(), already 0-based. */
    struct NamedLine
    {
        String name;
        int index = 0;
    };

    // Both are looked up linearly: a template has a handful of names, so a hash
    // map would cost more than it saves.
    Array<NamedArea> templateAreas;
    Array<NamedLine> columnLineNames;
    Array<NamedLine> rowLineNames;

    int templateAreaColumns = 0;
    int templateAreaRows = 0;
};

} // namespace yup
