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
/** An OffscreenTarget backed by a dedicated Rive render context.

    Created by GraphicsContext::createRenderableTarget(). Unlike a plain
    OffscreenTarget, a RenderableTarget reserves a backend-owned RenderContext
    exclusively, for its whole lifetime, and returns it to the pool when
    destroyed. This dedicated context is what enables 2D drawing through a
    Graphics frame (GpuCanvas::beginDraw) and what makes recursive offscreen
    rendering safe (TransparencyLayer inside GpuCanvas, a Lottie matte inside a
    precomp inside another matte) without re-entering beginFrame.

    The reservation deliberately outlives any single frame. Targets are commonly
    pooled and reused across frames, and the nesting order between two pooled
    targets can differ from one frame to the next; sharing one context between
    them would make the inner target skip beginFrame and then be flushed against
    the outer frame's descriptor, producing undefined contents.

    The GraphicsContext must outlive every RenderableTarget it creates.
*/
class YUP_API RenderableTarget : public OffscreenTarget
{
public:
    /** Returns the dedicated Rive render context backing this target. Never nullptr. */
    virtual rive::gpu::RenderContext* getRenderContext() noexcept = 0;
};

} // namespace yup
