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

class Component;
class Graphics;
class GpuTexture;

//==============================================================================
/**
    Base class for GPU-based visual effects that can be applied to a Component.

    When a ComponentEffect is attached to a Component, the component (and its
    entire subtree of visible children) is rendered into an offscreen GPU texture.
    The texture covers the paint margin of the component too, so what is painted
    there goes through the effect and the effect has room to draw past the bounds.
    The effect's apply() method is then called to composite the result back onto
    the main Graphics context, allowing arbitrary shader-based post-processing
    such as blurs, drop shadows, colour adjustments, or custom GLSL effects.

    Where no offscreen texture can be made, without a GPU or for a component larger than
    a texture of the device can be, the component is painted directly, without its effect.
    So it is with an effect that tells it has nothing to apply, see isActive().

    Subclass ComponentEffect and override apply() to implement custom effects.
    Use g.getGraphicsContext() to access the GPU device for creating GpuPipeline
    and GpuCanvas resources.

    The offscreen texture is kept from one paint to the next, and painted again only
    where the component or one of its children repainted. An effect knows the components
    it is set on: when what it draws changes, such as one of its parameters, invalidate()
    has each of them drawn through apply() again over the texture as it is. The setters of
    an effect call it, so nothing has to be called on a component for a change of its effect.

    Effects that move pixels around (waves, lenses, zooms) should also override
    displayToContent() and contentToDisplay(), so that mouse input lands on the
    widget the user actually sees under the pointer.

    @see Component::setComponentEffect, Component::setPaintMargin
*/
class YUP_API ComponentEffect : public ReferenceCountedObject
{
public:
    /** A shared pointer to a ComponentEffect. */
    using Ptr = ReferenceCountedObjectPtr<ComponentEffect>;

    /** Creates an effect, which is set on no component yet. */
    ComponentEffect() = default;

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
        @param bounds        The destination rectangle in g's coordinate space, which is the
                             area of the component held by the texture. For a component
                             painted in its parent it is the local bounds enlarged by the
                             paint margin, up to the whole units and pixels around it, so
                             it starts at a negative position when there is a margin. It
                             does not shrink when the margin does, as the texture keeps the
                             room it has. For a snapshot it is the local bounds.
    */
    virtual void apply (Graphics& g, GpuTexture::Ptr inputTexture, Rectangle<float> bounds) = 0;

    /**
        Tells whether the effect has something to apply.

        A component only goes through an effect that is active: it is rendered into an offscreen
        texture, and drawn back by apply(). With an effect that is not active it is painted as if
        it had none: directly, with no texture in between, and with its points mapped by neither
        displayToContent() nor contentToDisplay().

        Return false while apply() would draw the texture as it is, such as with a shader that
        could not be loaded, or with a parameter that turns the effect off. Repaint the component
        when the value changes, with Component::repaint().

        This is called on the message thread and while the window is painted, as
        displayToContent() is: the same threading rules apply.

        The default implementation returns true.

        @return True if the component has to be drawn through apply().

        @see Component::setComponentEffect
    */
    virtual bool isActive() const
    {
        return true;
    }

    /**
        Releases what the effect keeps of the window it was last applied in.

        An effect that keeps GPU objects from one apply() to the next (targets, pipelines,
        textures, canvases, or the GpuDevice they were made with) has to let go of them here:
        they belong to the device of a window, which may be gone by the next apply(). Kept
        longer, they hold that device alive past its window and are released once its
        rendering context is gone, or with the context of another window current.

        A component calls it for its effect when it leaves its window: when it is removed from
        a parent that is in one, and before the window is destroyed when it is taken off the
        desktop, itself or with an ancestor. It is called on the message thread, never while
        apply() runs, and may be called more than once in a row. The next apply() makes what
        the effect needs again, for the device it is then given.

        The default implementation does nothing.

        @see Component::setComponentEffect
    */
    virtual void releaseResources() {}

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
        @param bounds        The local bounds of the component, enlarged by its paint margin.

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
        @param bounds        The local bounds of the component, enlarged by its paint margin.

        @return The displayed point, or std::nullopt if the mapping is degenerate.

        @see displayToContent
    */
    virtual std::optional<Point<float>> contentToDisplay (Point<float> contentPoint, [[maybe_unused]] Rectangle<float> bounds) const
    {
        return contentPoint;
    }

    /**
        Tells the components the effect is set on that what it draws changed.

        Each component the effect is set on is drawn through apply() again at the next frame of
        its window, over the texture it kept of its subtree: neither the component nor its
        children are painted again, which is what makes a change of an effect cheap.

        The setters of an effect call it when the value they are given is not the one the effect
        has (a parameter, a color, the time of an animation), so that who uses the effect has
        nothing to call. Call it yourself for what an effect reads without being told, or for an
        effect that wraps another one and changed it.

        @code
        void MyEffect::setAmount (float newAmount)
        {
            if (std::exchange (amount, newAmount) != newAmount)
                invalidate();
        }
        @endcode

        A component that is not drawn through the effect is painted again as it is, as by
        Component::repaint(): when the effect has nothing to apply (see isActive()), or where
        there is no GPU to keep a texture with. An effect that is set on no component asks for
        nothing.

        Call it on the message thread, or with the message manager locked, as
        Component::repaint(), and never from apply(): a component can't be repainted while it
        is painted.

        @see Component::setComponentEffect, isActive
    */
    void invalidate();

private:
    friend class Component;

    Array<Component*> components;

    YUP_DECLARE_NON_COPYABLE (ComponentEffect)
};

} // namespace yup
