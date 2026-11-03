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
    A handle that resizes its parent component when dragged.

    Add it as the last child of the component to resize: it keeps itself in the parent's bottom right
    corner, on top of its siblings. Dragging it resizes the parent, optionally passing the size through
    constrainSize, so the parent (or its native window) can follow aspect ratios and size limits.

    @code
    corner = std::make_unique<ResizableCorner>();
    corner->constrainSize = [this] (Size<float> size) { return size.withWidth (jmax (200.0f, size.getWidth())); };
    addAndMakeVisible (*corner);
    @endcode

    @see Component
*/
class YUP_API ResizableCorner : public Component
{
public:
    //==============================================================================
    /** Style identifiers for theme customization. */
    struct Style
    {
        /** The color of the grip lines. */
        static const Identifier lineColorId;
    };

    //==============================================================================
    /** Creates a ResizableCorner.

        @param componentID  An optional identifier for this component
    */
    explicit ResizableCorner (StringRef componentID = {});

    /** Destructor. */
    ~ResizableCorner() override;

    //==============================================================================
    /** Sets the side length of the square handle, in points. Defaults to 16. */
    void setCornerSize (float newCornerSize);

    /** Returns the side length of the square handle, in points. */
    float getCornerSize() const noexcept;

    //==============================================================================
    /** Called with the size requested by a drag, returning the size to apply to the parent.

        When unset the requested size is applied unchanged.
    */
    std::function<Size<float> (Size<float>)> constrainSize;

    //==============================================================================
    /** @internal */
    void paint (Graphics& g) override;
    /** @internal */
    void mouseDown (const MouseEvent& event) override;
    /** @internal */
    void mouseDrag (const MouseEvent& event) override;
    /** @internal */
    void parentSizeChanged() override;
    /** @internal */
    void parentHierarchyChanged() override;

private:
    void updatePosition();

    float cornerSize = 16.0f;
    Size<float> sizeAtMouseDown;
    Point<float> screenPositionAtMouseDown;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResizableCorner)
};

} // namespace yup
