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
        fb.items.add (component1.withFlex (1));
        fb.items.add (component2.withFlex (2));
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
        spaceBetween, /**< Even spacing between items */
        spaceAround   /**< Even spacing around items */
    };

    /** Alignment of items along the cross axis. */
    enum class AlignItems
    {
        flexStart, /**< Align to start */
        flexEnd,   /**< Align to end */
        center,    /**< Center */
        stretch,   /**< Stretch to fill */
        baseline   /**< Align by the items' baselines */
    };

    /** Alignment of lines when there is extra space on the cross axis. */
    enum class AlignContent
    {
        flexStart,    /**< Pack at start */
        flexEnd,      /**< Pack at end */
        center,       /**< Pack centered */
        spaceBetween, /**< Even spacing between lines */
        spaceAround,  /**< Even spacing around lines */
        stretch       /**< Stretch lines to fill */
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

    /** How wrapped lines are aligned on the cross axis. Default is stretch. */
    AlignContent alignContent = AlignContent::stretch;

    /** The gap between items on the main axis. */
    float gap = 0.0f;

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

private:
    //==============================================================================
    struct LineInfo
    {
        Array<FlexItem*> items;
        float totalMainSize;
        float crossSize;
    };

    void calculateLayout (Rectangle<float> targetArea, Array<LineInfo>& lines);
};

} // namespace yup
