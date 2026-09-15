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
/** The semi-transparent ghost that follows the cursor during a drag.

    A borderless, transparent, non-focusable, always-on-top window, so it can float over every YUP
    window - and over the gap between them - without ever being clicked or stealing keyboard focus.

    This is an implementation detail of DragAndDropManager, which owns the single instance and moves
    it; nothing else needs to construct one.

    @see DragAndDropManager
*/
class DragImageComponent final : public Component
{
public:
    //==============================================================================
    /** Creates the ghost window, hidden. */
    DragImageComponent();

    /** Destructor. */
    ~DragImageComponent() override;

    //==============================================================================
    /** Shows a static image as the drag image.

        @param newImage   The image to show.
        @param hotspot    The point within the image that sits under the cursor.
        @param newOpacity The opacity applied to the whole window.
    */
    void setDragImage (const Image& newImage, const Point<float>& hotspot, float newOpacity);

    /** Shows a live component as the drag image.

        The caller keeps ownership and must keep @a newComponent alive until the drag ends; the ghost
        only reparents and positions it.

        @param newComponent The component to host.
        @param hotspot      The point within the component that sits under the cursor.
        @param newOpacity   The opacity applied to the whole window.
    */
    void setDragImageComponent (Component* newComponent, const Point<float>& hotspot, float newOpacity);

    /** Hides the ghost and forgets whatever it was showing. */
    void clearDragImage();

    /** Moves the ghost so its hotspot sits on @a screenPosition. */
    void moveToScreenPosition (const Point<float>& screenPosition);

    //==============================================================================
    void paint (Graphics& g) override;

private:
    //==============================================================================
    void updateWindowSize();

    Image dragImage;
    Component* hostedComponent = nullptr;
    Point<float> hotspot;
    float opacity = 0.7f;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DragImageComponent)
};

} // namespace yup
