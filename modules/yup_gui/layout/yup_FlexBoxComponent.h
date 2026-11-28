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
    A layout component laying its children out with the flexbox algorithm.

    The container properties are `direction`, `wrap`, `justify` and `align`, on top of the `padding`
    and `gap` of @ref LayoutComponent. Each child is read through its layout item properties:
    `flexGrow`, `flexShrink`, `flexBasis`, `order`, `alignSelf`, `margin`, `preferredWidth`,
    `preferredHeight` and the minimum and maximum sizes.

    @see FlexBox, RowComponent, ColumnComponent

    @tags{GUI}
*/
class YUP_API FlexBoxComponent : public LayoutComponent
{
public:
    //==============================================================================
    /** Creates a flexbox container laying its children out in a row. */
    FlexBoxComponent() = default;

    /** Creates a flexbox container with an identifier. */
    explicit FlexBoxComponent (StringRef componentID)
        : LayoutComponent (componentID)
    {
    }

    //==============================================================================
    /** Sets the direction the children are laid out in. */
    void setDirection (FlexBox::Direction newDirection);

    /** Returns the direction the children are laid out in. */
    FlexBox::Direction getDirection() const;

    //==============================================================================
    /** Returns the static type info describing the FlexBox type. */
    static const ComponentTypeInfo& getClassTypeInfo();

    /** @internal */
    const ComponentTypeInfo& getDefaultTypeInfo() const override { return getClassTypeInfo(); }

protected:
    /** @internal */
    void performLayout (Rectangle<float> area) override;

    /** @internal */
    std::optional<Size<float>> measureContent() const override;

private:
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FlexBoxComponent)
};

//==============================================================================
/**
    A flexbox container laying its children out from left to right.

    @see FlexBoxComponent, ColumnComponent

    @tags{GUI}
*/
class YUP_API RowComponent : public FlexBoxComponent
{
public:
    /** Creates a row. */
    RowComponent() = default;

    /** Creates a row with an identifier. */
    explicit RowComponent (StringRef componentID)
        : FlexBoxComponent (componentID)
    {
    }

    /** Returns the static type info describing the Row type. */
    static const ComponentTypeInfo& getClassTypeInfo();

    /** @internal */
    const ComponentTypeInfo& getDefaultTypeInfo() const override { return getClassTypeInfo(); }

private:
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RowComponent)
};

//==============================================================================
/**
    A flexbox container laying its children out from top to bottom.

    @see FlexBoxComponent, RowComponent

    @tags{GUI}
*/
class YUP_API ColumnComponent : public FlexBoxComponent
{
public:
    /** Creates a column. */
    ColumnComponent() = default;

    /** Creates a column with an identifier. */
    explicit ColumnComponent (StringRef componentID)
        : FlexBoxComponent (componentID)
    {
    }

    /** Returns the static type info describing the Column type. */
    static const ComponentTypeInfo& getClassTypeInfo();

    /** @internal */
    const ComponentTypeInfo& getDefaultTypeInfo() const override { return getClassTypeInfo(); }

private:
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ColumnComponent)
};

} // namespace yup
