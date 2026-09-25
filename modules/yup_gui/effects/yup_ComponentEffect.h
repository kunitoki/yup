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

class Graphics;
class GpuTexture;

//==============================================================================
/**
    Base class for GPU-based visual effects that can be applied to a Component.

    When a ComponentEffect is attached to a Component, the component (and its
    entire subtree of visible children) is rendered into an offscreen GPU texture.
    The effect's apply() method is then called to composite the result back onto
    the main Graphics context, allowing arbitrary shader-based post-processing
    such as blurs, drop shadows, colour adjustments, or custom GLSL effects.

    Subclass ComponentEffect and override apply() to implement custom effects.
    Use g.getGraphicsContext() to access the GPU device for creating GpuPipeline
    and GpuCanvas resources.

    Effects that move pixels around (waves, lenses, zooms) should also override
    displayToContent() and contentToDisplay(), so that mouse input lands on the
    widget the user actually sees under the pointer.

    @see Component::setComponentEffect
*/
class YUP_API ComponentEffect : public ReferenceCountedObject
{
public:
    /** A shared pointer to a ComponentEffect. */
    using Ptr = ReferenceCountedObjectPtr<ComponentEffect>;

    /** Destructor. */
    virtual ~ComponentEffect() override = default;

    /**
        Applies the effect.

        The component subtree has been rendered into inputTexture. The effect
        must draw its result into g at the given bounds (in g's coordinate space).

        The texture is at device-pixel resolution, i.e. bounds scaled by the display scale,
        so derive texel-space parameters (blur radii, pixel sizes) from the texture size and
        not from bounds.

        @param g             The main Graphics context where the result is drawn.
        @param inputTexture  The GPU texture containing the rendered component subtree,
                             sized in device pixels.
        @param bounds        The destination rectangle in g's coordinate space.
    */
    virtual void apply (Graphics& g, GpuTexture::Ptr inputTexture, Rectangle<float> bounds) = 0;

    /**
        Maps a point in the displayed (post-effect) local space of the component to the
        content (pre-effect) space its subtree was painted in.

        This is the same per-pixel mapping a distortion shader computes for its sample
        coordinate: given where a pixel is shown, it tells where it was taken from. It is
        used to route mouse and drag-and-drop input to the child that is actually displayed
        under the pointer.

        Return a point even when it falls outside @a bounds, as long as the mapping is
        defined there, so that a captured drag keeps tracking past the edges. Return
        std::nullopt only when the mapping is degenerate at that point.

        This is called on the message thread, while apply() runs on the render thread:
        read the parameters that the last apply() published (through atomics or a lock),
        which also keeps input consistent with what is on screen.

        The default implementation returns the point unchanged.

        @param displayPoint  The point as displayed, in the component's local coordinates.
        @param bounds        The local bounds of the component.

        @return The point in the content space, or std::nullopt if the mapping is degenerate.

        @see contentToDisplay
    */
    virtual std::optional<Point<float>> displayToContent (Point<float> displayPoint, [[maybe_unused]] Rectangle<float> bounds) const
    {
        return displayPoint;
    }

    /**
        Maps a point in the content (pre-effect) local space to where it is displayed.

        This is the inverse of displayToContent(). It is used by Component::localToScreen()
        and everything built on it, such as popup placement and the text input caret
        rectangle. The same threading rules as displayToContent() apply.

        The default implementation returns the point unchanged: an effect that overrides
        displayToContent() but not this one only gets an approximate inverse mapping.

        @param contentPoint  The point in the content space, in the component's local coordinates.
        @param bounds        The local bounds of the component.

        @return The displayed point, or std::nullopt if the mapping is degenerate.

        @see displayToContent
    */
    virtual std::optional<Point<float>> contentToDisplay (Point<float> contentPoint, [[maybe_unused]] Rectangle<float> bounds) const
    {
        return contentPoint;
    }
};

} // namespace yup
