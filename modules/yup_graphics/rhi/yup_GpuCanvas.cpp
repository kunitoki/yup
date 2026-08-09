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

GpuCanvas::Ptr GpuCanvas::create (GraphicsContext& ctx, int width, int height)
{
    if (width <= 0 || height <= 0)
        return nullptr;

    auto gpuCtx = ctx.getGpuDevice();
    if (gpuCtx == nullptr)
        return nullptr;

    // GpuCanvas needs a dedicated render context for the 2D drawing path.
    auto renderable = gpuCtx->createRenderableTarget (width, height);
    if (renderable == nullptr)
        return nullptr;

    auto target = GpuTarget::createFromTarget (gpuCtx, std::move (renderable));
    if (target == nullptr)
        return nullptr;

    GpuCanvas::Ptr canvas = new GpuCanvas();
    canvas->ctx = &ctx;
    canvas->target = std::move (target);
    return canvas;
}

//==============================================================================

GpuTarget::Ptr GpuCanvas::getTarget() const noexcept
{
    return target;
}

//==============================================================================

int GpuCanvas::getWidth() const noexcept
{
    return target != nullptr ? target->getWidth() : 0;
}

int GpuCanvas::getHeight() const noexcept
{
    return target != nullptr ? target->getHeight() : 0;
}

//==============================================================================

Graphics& GpuCanvas::beginDraw()
{
    jassert (ctx != nullptr && target != nullptr);

    // Drop the previous frame's Graphics so a fresh offscreen 2D frame opens on
    // the existing (already-allocated) target. The cached texture wraps the same
    // GPU render target whose contents are overwritten by the new frame, so it is
    // reset to force a rewrap on the next asTexture().
    graphics.reset();
    frameOpen = false;
    committed = false;

    graphics = std::make_unique<Graphics> (*ctx, *target->getRenderableTarget(), 0u);
    frameOpen = true;

    return *graphics;
}

//==============================================================================

bool GpuCanvas::commit()
{
    if (! frameOpen || committed || ctx == nullptr || target == nullptr)
        return false;

    auto* renderableTarget = target->getRenderableTarget();
    if (renderableTarget == nullptr)
        return false;

    // Ensure the Y-flipped sampled mirror exists before the flush so that
    // blitMirrorIfRegistered (called inside endOffscreen) can update it.
    // No-op on non-GL backends and for canvases never used as sampled inputs.
    renderableTarget->getOrCreateSampledTexture();

    if (graphics == nullptr || ! graphics->commitOffscreenTarget())
        return false;

    committed = true;
    return true;
}

//==============================================================================

GpuTexture::Ptr GpuCanvas::asTexture()
{
    if (target == nullptr)
        return nullptr;

    // Auto-commit a 2D frame opened via beginDraw() so callers don't need to
    // call commit() explicitly. Render-pass-only usage never sets frameOpen.
    if (frameOpen && ! committed)
        commit();

    return target->asTexture();
}

Image GpuCanvas::asImage()
{
    auto img = Image::fromTexture (asTexture());

    if (img.isValid())
    {
        auto span = img.getRawData();
        readPixels (span.data(), span.size());
    }

    return img;
}

bool GpuCanvas::readPixels (void* dst, size_t byteSize)
{
    if (target == nullptr || ctx == nullptr)
        return false;

    if (frameOpen && ! committed)
        return false;

    return target->readPixels (dst, byteSize);
}

//==============================================================================

GpuRenderPass GpuCanvas::beginRenderPass (GpuFrame& frame, const GpuRenderOptions& options)
{
    if (target == nullptr)
        return {};

    return target->beginRenderPass (frame, options);
}

} // namespace yup
