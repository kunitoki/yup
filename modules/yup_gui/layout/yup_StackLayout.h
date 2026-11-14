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

#pragma once

namespace yup
{

//==============================================================================
/**
    Lays items out on top of each other, in the order they were added.

    A stack gives every item the same area, so they overlap, each one placed in it by its
    own alignment, size and margin. It is the layout behind overlays, badges, placeholders
    drawn under an editor, and anything that needs a z order rather than a flow.

    It is a layout algorithm, like FlexBox and Grid: it owns no components, and is usually
    rebuilt and run from a `resized()` callback, or by StackComponent from the layout
    properties of its children.

    @code
    StackLayout stack;
    stack.setPadding (8.0f);
    stack.items.add (StackLayout::Item (background));
    stack.items.add (StackLayout::Item (badge).withSize (16.0f, 16.0f).withAlign (StackLayout::Align::end, StackLayout::Align::start));
    stack.performLayout (getLocalBounds());
    @endcode

    @see FlexBox, Grid, StackComponent

    @tags{UI}
*/
class YUP_API StackLayout
{
public:
    //==============================================================================
    /** How an item is placed on one axis. */
    enum class Align
    {
        automatic, /**< Use the container's alignment. */
        stretch,   /**< Fill the area, unless the item has a size of its own. */
        start,     /**< Place at the left or top edge. */
        center,    /**< Place in the middle. */
        end        /**< Place at the right or bottom edge. */
    };

    //==============================================================================
    /** One item of a stack. */
    struct YUP_API Item
    {
        /** Creates an empty item. */
        Item() = default;

        /** Creates an item laying out a component. */
        Item (Component& component);

        /** Creates an item laying out a component, which may be null. */
        Item (Component* component);

        //==============================================================================
        /** The component the item positions, which may be null. */
        Component* associatedComponent = nullptr;

        /** The width of the item, or -1 to take it from the alignment. */
        float width = -1.0f;

        /** The height of the item, or -1 to take it from the alignment. */
        float height = -1.0f;

        /** How the item is placed horizontally. */
        Align alignX = Align::automatic;

        /** How the item is placed vertically. */
        Align alignY = Align::automatic;

        /** The space kept around the item, inside the stack's content area. */
        EdgeSpacing<float> margin;

        //==============================================================================
        /** Returns a copy with a fixed size. */
        Item withSize (float newWidth, float newHeight) const;

        /** Returns a copy with a fixed width. */
        Item withWidth (float newWidth) const;

        /** Returns a copy with a fixed height. */
        Item withHeight (float newHeight) const;

        /** Returns a copy aligned on both axes. */
        Item withAlign (Align newAlignX, Align newAlignY) const;

        /** Returns a copy with a margin on every edge. */
        Item withMargin (float newMargin) const;

        /** Returns a copy with a margin. */
        Item withMargin (const EdgeSpacing<float>& newMargin) const;
    };

    //==============================================================================
    /** Creates an empty stack. */
    StackLayout() = default;

    //==============================================================================
    /** How items with no horizontal alignment of their own are placed. Stretch by default. */
    Align alignX = Align::stretch;

    /** How items with no vertical alignment of their own are placed. Stretch by default. */
    Align alignY = Align::stretch;

    /** Padding inside the container, which shrinks the area the items are laid out in. */
    EdgeSpacing<float> padding;

    /** Sets all four padding values at once. */
    void setPadding (float newPadding) noexcept;

    /** Sets the horizontal and vertical padding. */
    void setPadding (float horizontal, float vertical) noexcept;

    /** Sets the padding from an edge spacing. */
    void setPadding (const EdgeSpacing<float>& newPadding) noexcept;

    //==============================================================================
    /** The items to lay out, from the bottom of the stack to its top. */
    Array<Item> items;

    //==============================================================================
    /** Positions the items inside an area.

        @param targetArea The area to lay the items out in.
    */
    void performLayout (Rectangle<float> targetArea);

    /** Positions the items inside an area.

        @param targetArea The area to lay the items out in.
    */
    void performLayout (Rectangle<int> targetArea);
};

} // namespace yup
