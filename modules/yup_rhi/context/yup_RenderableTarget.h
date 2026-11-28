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
/** An OffscreenTarget a 2D Graphics frame can draw into.

    Created by GraphicsContext::createRenderableTarget(). Unlike a plain
    OffscreenTarget, a RenderableTarget is drawn with a backend-owned Rive
    RenderContext, which it leases from a pool for each frame: from
    GpuDevice::beginOffscreen() to GpuDevice::endOffscreen(). This is what
    enables 2D drawing through a Graphics frame (GpuCanvas::beginDraw) and what
    makes recursive offscreen rendering safe (TransparencyLayer inside GpuCanvas,
    a Lottie matte inside a precomp inside another matte) without re-entering
    beginFrame.

    The lease is exclusive while the frame is open, so a target whose frame
    nests in the frame of another is always drawn with another context. A
    target keeps no context between its frames: the targets of an interface
    share the few contexts their frames need at once.

    The GraphicsContext must outlive every RenderableTarget it creates.
*/
class YUP_API RenderableTarget : public OffscreenTarget
{
public:
    /** Returns the Rive render context the frame of this target is drawn with.

        @returns The context leased for the frame that is open, or nullptr outside a frame.
    */
    virtual rive::gpu::RenderContext* getRenderContext() noexcept = 0;
};

} // namespace yup
