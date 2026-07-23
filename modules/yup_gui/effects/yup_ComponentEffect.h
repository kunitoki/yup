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

        @param g             The main Graphics context where the result is drawn.
        @param inputTexture  The GPU texture containing the rendered component subtree.
        @param bounds        The destination rectangle in g's coordinate space.
    */
    virtual void apply (Graphics& g, GpuTexture::Ptr inputTexture, Rectangle<float> bounds) = 0;
};

} // namespace yup
