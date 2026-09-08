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
    Describes the layout properties of a single item inside a FlexBox container.

    Each FlexItem wraps a Component and specifies how it should be sized and
    positioned within the flex layout. Properties include flex-grow, flex-shrink,
    min/max sizes, alignment, margins, and order.

    A Component* can be implicitly converted to a FlexItem, allowing components
    to be added to a FlexBox directly.

    @see FlexBox

    @tags{GUI}
*/
class YUP_API FlexItem
{
public:
    //==============================================================================
    /** Creates a FlexItem with no associated component. */
    FlexItem() = default;

    /** Creates a FlexItem that controls the layout of the given component. */
    FlexItem (Component& component);

    /** Creates a FlexItem that controls the layout of the given component. */
    FlexItem (Component* component);

    /** Creates a FlexItem with a fixed width and height. */
    FlexItem (float width, float height);

    /** Creates a FlexItem with a fixed width and height, and the given component. */
    FlexItem (Component& component, float width, float height);

    /** Creates a FlexItem with a fixed width and height, and the given component. */
    FlexItem (Component* component, float width, float height);

    //==============================================================================
    /** The component associated with this flex item, or nullptr. */
    Component* associatedComponent = nullptr;

    /** The flex-grow factor. Controls how this item grows relative to others.
        Must be >= 0; negative values are asserted and treated as 0. */
    float flexGrow = 0.0f;

    /** The flex-shrink factor. Controls how this item shrinks relative to others.
        Must be >= 0; negative values are asserted and treated as 0. */
    float flexShrink = 1.0f;

    /** The flex-basis value: the initial main size before growing/shrinking.

        A negative value (the default) means auto, i.e. fall back to the
        percentage size, then the fixed size, then the component's current
        size. A value of 0 is a real zero basis, which is what makes the
        `flex: 1 1 0` idiom - equal shares of the container regardless of
        content - expressible.

        @see withFlexBasis
    */
    float flexBasis = -1.0f;

    //==============================================================================
    /** Minimum width constraint. -1 means no constraint. */
    float minWidth = -1.0f;

    /** Minimum height constraint. -1 means no constraint. */
    float minHeight = -1.0f;

    /** Maximum width constraint. -1 means no constraint. */
    float maxWidth = -1.0f;

    /** Maximum height constraint. -1 means no constraint. */
    float maxHeight = -1.0f;

    //==============================================================================
    /** The width of the item.

        A negative value (the default) means auto: the component's current
        width is used (intrinsic/measured sizing). A value of 0 is a real
        zero width.

        On the cross axis an auto size is stretched by the default
        align-items: stretch; give an explicit size (or set alignSelf) to keep
        it fixed.
    */
    float width = -1.0f;

    /** The height of the item.

        A negative value (the default) means auto: the component's current
        height is used (intrinsic/measured sizing). A value of 0 is a real
        zero height.

        On the cross axis an auto size is stretched by the default
        align-items: stretch; give an explicit size (or set alignSelf) to keep
        it fixed.
    */
    float height = -1.0f;

    /** The flex-basis expressed as a percentage of the container's main-axis
        size. -1 means not set. When set, it takes priority over flexBasis and
        over the fixed/percentage size. */
    float flexBasisPercent = -1.0f;

    /** The width of the item expressed as a percentage of the container's
        width (row layouts) or height (column layouts). -1 means not set.
        When set, it overrides the fixed width. */
    float widthPercent = -1.0f;

    /** The height of the item expressed as a percentage of the container's
        height (row layouts) or width (column layouts). -1 means not set.
        When set, it overrides the fixed height. */
    float heightPercent = -1.0f;

    //==============================================================================
    /** Enumeration of possible alignment values for the cross-axis. */
    enum class AlignSelf
    {
        autoAlign, /**< Use the container's align-items value */
        flexStart, /**< Align to the start of the cross axis */
        flexEnd,   /**< Align to the end of the cross axis */
        center,    /**< Center along the cross axis */
        stretch,   /**< Stretch to fill the cross axis */
        baseline   /**< Align so the item's baseline lines up with the line's baseline */
    };

    /** The alignment of this item on the cross axis. */
    AlignSelf alignSelf = AlignSelf::autoAlign;

    /** The baseline of this item, as an offset from its top edge. Only used
        with AlignSelf::baseline. -1 means the item's own height is used. */
    float baseline = -1.0f;

    //==============================================================================
    /** Margin values for the item (in pixels). */
    float marginLeft = 0.0f;
    float marginRight = 0.0f;
    float marginTop = 0.0f;
    float marginBottom = 0.0f;

    /** Whether the corresponding margin behaves as CSS `margin: auto`.

        An auto margin absorbs its share of the free space on its own axis. On
        the main axis every auto margin on the line splits the free space
        equally and `justifyContent` is left with nothing to distribute - which
        is how `marginLeftAuto` pushes a single item to the far end of a
        toolbar, and how setting both centers it. On the cross axis an auto
        margin pushes the item within its line and suppresses stretching, since
        the item can no longer be sized to fill the line.

        When there is no free space to take, an auto margin resolves to 0.

        @see withAutoMargins
    */
    bool marginLeftAuto = false;
    bool marginRightAuto = false;
    bool marginTopAuto = false;
    bool marginBottomAuto = false;

    /** Order value. Items with lower order are laid out first. */
    int order = 0;

    //==============================================================================
    /** Returns a copy of this FlexItem with a different flex-grow. */
    FlexItem withFlex (float newFlexGrow) const;

    /** Returns a copy of this FlexItem with a different flex-shrink. */
    FlexItem withFlexShrink (float newFlexShrink) const;

    /** Returns a copy of this FlexItem with a different flex-basis.

        Pass 0 for the `flex: 1 1 0` idiom, or a negative value for auto.
    */
    FlexItem withFlexBasis (float newFlexBasis) const;

    /** Returns a copy of this FlexItem with a different width. */
    FlexItem withWidth (float newWidth) const;

    /** Returns a copy of this FlexItem with a different height. */
    FlexItem withHeight (float newHeight) const;

    /** Returns a copy of this FlexItem with a different width percentage. */
    FlexItem withWidthPercent (float newWidthPercent) const;

    /** Returns a copy of this FlexItem with a different height percentage. */
    FlexItem withHeightPercent (float newHeightPercent) const;

    /** Returns a copy of this FlexItem with a different flex-basis percentage. */
    FlexItem withFlexBasisPercent (float newFlexBasisPercent) const;

    /** Returns a copy of this FlexItem with different minimum dimensions. */
    FlexItem withMinWidth (float newMinWidth) const;
    FlexItem withMinHeight (float newMinHeight) const;

    /** Returns a copy of this FlexItem with different maximum dimensions. */
    FlexItem withMaxWidth (float newMaxWidth) const;
    FlexItem withMaxHeight (float newMaxHeight) const;

    /** Returns a copy of this FlexItem with different margins. */
    FlexItem withMargin (float newMargin) const;

    /** Returns a copy of this FlexItem with the given margins marked as auto.

        @see marginLeftAuto
    */
    FlexItem withAutoMargins (bool left, bool right, bool top, bool bottom) const;

    /** Returns a copy of this FlexItem with a different align-self value. */
    FlexItem withAlignSelf (AlignSelf newAlignSelf) const;

    /** Returns a copy of this FlexItem with a different baseline offset. */
    FlexItem withBaseline (float newBaseline) const;

    /** Returns a copy of this FlexItem with a different order. */
    FlexItem withOrder (int newOrder) const;
};

} // namespace yup
