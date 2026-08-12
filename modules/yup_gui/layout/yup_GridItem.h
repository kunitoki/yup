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
    /** The column position (0-based). -1 places the item automatically. */
    int column = -1;

    /** The row position (0-based). -1 places the item automatically. */
    int row = -1;

    /** Number of columns this item spans. */
    int columnSpan = 1;

    /** Number of rows this item spans. */
    int rowSpan = 1;

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
        stretch    /**< Stretch to fill */
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
};

} // namespace yup
