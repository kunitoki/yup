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
    A layout component stacking its children on top of each other.

    The container properties are `justify` and `align`, which place the children that don't align
    themselves, on top of the `padding` of @ref LayoutComponent. Each child is read through its
    `justifySelf`, `alignSelf`, `preferredWidth`, `preferredHeight` and `margin` properties.

    @see StackLayout

    @tags{GUI}
*/
class YUP_API StackComponent : public LayoutComponent
{
public:
    //==============================================================================
    /** Creates an empty stack. */
    StackComponent() = default;

    /** Creates a stack with an identifier. */
    explicit StackComponent (StringRef componentID)
        : LayoutComponent (componentID)
    {
    }

    //==============================================================================
    /** Returns the static type info describing the Stack type. */
    static const ComponentTypeInfo& getClassTypeInfo();

    /** @internal */
    const ComponentTypeInfo& getDefaultTypeInfo() const override { return getClassTypeInfo(); }

protected:
    /** @internal */
    void performLayout (Rectangle<float> area) override;

    /** @internal */
    std::optional<Size<float>> measureContent() const override;

private:
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StackComponent)
};

} // namespace yup
