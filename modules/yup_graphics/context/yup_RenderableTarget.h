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
    OffscreenTarget, a RenderableTarget reserves a backend-owned RenderContext.
    A context is reserved while a target frame is active, allowing recursive
    offscreen rendering (TransparencyLayer inside GpuCanvas, nested precomps)
    without re-entering beginFrame and allowing sequential targets to reuse an
    idle context. This dedicated context is what enables 2D drawing through a
    Graphics frame (GpuCanvas::beginDraw).

    The GraphicsContext must outlive every RenderableTarget it creates.
*/
class YUP_API RenderableTarget : public OffscreenTarget
{
public:
    /** Returns the dedicated Rive render context backing this target. Never nullptr. */
    virtual rive::gpu::RenderContext* getRenderContext() noexcept = 0;
};

} // namespace yup
