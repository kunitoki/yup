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
    Describes the layout properties of a single item inside a Grid container.

    Each GridItem wraps a Component and specifies its placement within the grid
    using row/column positions, spans, and alignment properties.

    A Component* can be implicitly converted to a GridItem.

    @see Grid

    @tags{GUI}
*/
class YUP_API GridItem
{
public:
    //==============================================================================
    /** Creates a GridItem with no associated component. */
    GridItem() = default;

    /** Creates a GridItem that controls the layout of the given component. */
    GridItem (Component& component);

    /** Creates a GridItem that controls the layout of the given component. */
    GridItem (Component* component);


    //==============================================================================
    /** The component associated with this grid item, or nullptr. */
    Component* associatedComponent = nullptr;

    //==============================================================================
    /** The value of column/row that asks the grid to position the item itself.

        Note the deliberate divergence from CSS here: CSS numbers grid lines
        from 1 and gives -1 the meaning "the last line", whereas YUP numbers
        them from 0 and uses -1 for automatic placement. An axis can be set
        independently, so an item may pin its row and let the grid choose its
        column. */
    static constexpr int autoPlace = -1;

    /** The column position (0-based), or autoPlace. */
    int column = autoPlace;

    /** The row position (0-based), or autoPlace. */
    int row = autoPlace;

    /** Number of columns this item spans. Must be >= 1. */
    int columnSpan = 1;

    /** Number of rows this item spans. Must be >= 1. */
    int rowSpan = 1;

    //==============================================================================
    /** The name of a grid area declared with Grid::setTemplateAreas().

        When set and the name is known to the grid, the area supplies the item's
        row, column and both spans, overriding all four fields above. An unknown
        name is ignored and the numeric placement applies instead.

        @see withArea, Grid::setTemplateAreas
    */
    String area;

    /** The name of the column line this item starts at.

        Resolved against Grid::setColumnLineName(). Takes priority over the
        numeric `column` but not over `area`.

        @see withColumnStart, Grid::setColumnLineName
    */
    String columnStartName;

    /** The name of the row line this item starts at.

        @see withRowStart, Grid::setRowLineName
    */
    String rowStartName;

    //==============================================================================
    /** The width of the item. -1 means the cell's width is used (fill). */
    float width = -1.0f;

    /** The height of the item. -1 means the cell's height is used (fill). */
    float height = -1.0f;

    /** The width of the item expressed as a percentage of the cell's width.
        -1 means not set. When set, it overrides the fixed width. */
    float widthPercent = -1.0f;

    /** The height of the item expressed as a percentage of the cell's height.
        -1 means not set. When set, it overrides the fixed height. */
    float heightPercent = -1.0f;

    /** Minimum width constraint. -1 means no constraint. */
    float minWidth = -1.0f;

    /** Minimum height constraint. -1 means no constraint. */
    float minHeight = -1.0f;

    /** Maximum width constraint. -1 means no constraint. */
    float maxWidth = -1.0f;

    /** Maximum height constraint. -1 means no constraint. */
    float maxHeight = -1.0f;

    //==============================================================================
    /** Enumeration of alignment values. */
    enum class AlignSelf
    {
        autoAlign, /**< Use the container's default */
        flexStart, /**< Align to start */
        flexEnd,   /**< Align to end */
        center,    /**< Center */
        stretch,   /**< Stretch to fill */
        baseline   /**< Align with the other baseline items in the same row */
    };

    /** Horizontal alignment within the cell. */
    AlignSelf justifySelf = AlignSelf::autoAlign;

    /** Vertical alignment within the cell. */
    AlignSelf alignSelf = AlignSelf::autoAlign;

    //==============================================================================
    /** Margin values for the item (in pixels). */
    float marginLeft = 0.0f;
    float marginRight = 0.0f;
    float marginTop = 0.0f;
    float marginBottom = 0.0f;

    //==============================================================================
    /** Returns a copy of this GridItem with the given column/row. */
    GridItem withColumn (int newColumn) const;
    GridItem withRow (int newRow) const;
    GridItem withColumnSpan (int newSpan) const;
    GridItem withRowSpan (int newSpan) const;
    GridItem withMargin (float newMargin) const;
    GridItem withWidth (float newWidth) const;
    GridItem withHeight (float newHeight) const;
    GridItem withWidthPercent (float newWidthPercent) const;
    GridItem withHeightPercent (float newHeightPercent) const;
    GridItem withMinWidth (float newMinWidth) const;
    GridItem withMinHeight (float newMinHeight) const;
    GridItem withMaxWidth (float newMaxWidth) const;
    GridItem withMaxHeight (float newMaxHeight) const;
    GridItem withJustifySelf (AlignSelf newJustifySelf) const;
    GridItem withAlignSelf (AlignSelf newAlignSelf) const;

    /** Returns a copy of this GridItem placed in the named grid area. */
    GridItem withArea (const String& areaName) const;

    /** Returns a copy of this GridItem starting at the named column line. */
    GridItem withColumnStart (const String& lineName) const;

    /** Returns a copy of this GridItem starting at the named row line. */
    GridItem withRowStart (const String& lineName) const;
};

} // namespace yup
