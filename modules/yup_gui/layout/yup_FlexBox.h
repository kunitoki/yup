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
    A CSS-flexbox-style layout container for arranging components.

    FlexBox provides a flexible layout system for positioning child components
    within a given area. It follows the CSS Flexbox layout algorithm, supporting
    row/column direction, wrapping, alignment, and flexible sizing via FlexItem.

    Components can be added directly (implicitly converted to FlexItem), or as
    FlexItem objects with specific layout properties.

    Usage:
    @code
        FlexBox fb;
        fb.flexDirection = FlexBox::Direction::row;
        fb.justifyContent = FlexBox::JustifyContent::spaceBetween;
        fb.items.add (FlexItem (component1).withFlex (1));
        fb.items.add (FlexItem (component2).withFlex (2));
        fb.performLayout (getLocalBounds());
    @endcode

    @see FlexItem

    @tags{GUI}
*/
class YUP_API FlexBox
{
public:
    //==============================================================================
    /** Direction of the flex layout. */
    enum class Direction
    {
        row,          /**< Left to right */
        rowReverse,   /**< Right to left */
        column,       /**< Top to bottom */
        columnReverse /**< Bottom to top */
    };

    /** Wrapping behavior for items that overflow. */
    enum class Wrap
    {
        noWrap,     /**< All items on one line */
        wrap,       /**< Wrap to next line */
        wrapReverse /**< Wrap to next line in reverse */
    };

    /** Alignment of items along the main axis. */
    enum class JustifyContent
    {
        flexStart,    /**< Pack at start */
        flexEnd,      /**< Pack at end */
        center,       /**< Pack centered */
        spaceBetween, /**< Even spacing between items, none at the edges */
        spaceAround,  /**< A full space between items and a half space at each edge */
        spaceEvenly   /**< An equal space between items and at both edges */
    };

    /** Alignment of items along the cross axis. */
    enum class AlignItems
    {
        flexStart, /**< Align to start */
        flexEnd,   /**< Align to end */
        center,    /**< Center */
        stretch,   /**< Stretch auto-sized items to fill the line's cross size. In a single-line (nowrap) container the line spans the container's full cross size */
        baseline   /**< Align by the items' baselines */
    };

    /** Alignment of lines when there is extra space on the cross axis. */
    enum class AlignContent
    {
        flexStart,    /**< Pack at start */
        flexEnd,      /**< Pack at end */
        center,       /**< Pack centered */
        spaceBetween, /**< Even spacing between lines, none at the edges */
        spaceAround,  /**< A full space between lines and a half space at each edge */
        spaceEvenly,  /**< An equal space between lines and at both edges */
        stretch       /**< Grow every line equally to fill the cross axis */
    };

    //==============================================================================
    FlexBox() = default;

    /** Creates a FlexBox with a direction. */
    explicit FlexBox (Direction direction);

    /** Creates a FlexBox with a direction, wrap and alignment settings. */
    FlexBox (Direction direction, Wrap wrap, AlignItems alignItems, JustifyContent justifyContent, AlignContent alignContent);

    //==============================================================================
    /** The flex direction. Default is row. */
    Direction flexDirection = Direction::row;

    /** The wrap mode. Default is no wrap. */
    Wrap flexWrap = Wrap::noWrap;

    /** How items are aligned on the cross axis. Default is stretch. */
    AlignItems alignItems = AlignItems::stretch;

    /** How items are justified on the main axis. Default is flex-start. */
    JustifyContent justifyContent = JustifyContent::flexStart;

    /** How wrapped lines are aligned on the cross axis. Default is stretch.

        Per CSS this has no effect on a single-line (`Wrap::noWrap`) container,
        whose only line always spans the container's whole cross size. It does
        apply to a wrapping container that happens to produce a single line. */
    AlignContent alignContent = AlignContent::stretch;

    /** The gap between items and between lines, as in the CSS `gap` shorthand.

        This is only used for whichever of rowGap / columnGap is left unset.
        Must be >= 0; a negative value is asserted and treated as 0. */
    float gap = 0.0f;

    /** The gap between rows, as in CSS `row-gap`. -1 (the default) means "use
        the gap shorthand".

        In a row container this separates the wrapped lines; in a column
        container it separates the items. */
    float rowGap = -1.0f;

    /** The gap between columns, as in CSS `column-gap`. -1 (the default) means
        "use the gap shorthand".

        In a row container this separates the items; in a column container it
        separates the wrapped lines. */
    float columnGap = -1.0f;

    //==============================================================================
    /** Padding inside the container, which shrinks the area the items are laid
        out in. Must be >= 0; negative values are asserted and treated as 0. */
    float paddingLeft = 0.0f;
    float paddingRight = 0.0f;
    float paddingTop = 0.0f;
    float paddingBottom = 0.0f;

    /** Sets all four padding values at once. */
    void setPadding (float newPadding) noexcept;

    /** Sets the horizontal and vertical padding. */
    void setPadding (float horizontal, float vertical) noexcept;

    //==============================================================================
    /** The items to be laid out. */
    Array<FlexItem> items;

    //==============================================================================
    /**
        Performs the flexbox layout, positioning child components within the
        given rectangle.

        @param targetArea   the area in which to lay out the items.
    */
    void performLayout (Rectangle<float> targetArea);

    /**
        Performs the flexbox layout using an integer rectangle.
        @param targetArea   the area in which to lay out the items.
    */
    void performLayout (Rectangle<int> targetArea);
};

} // namespace yup
