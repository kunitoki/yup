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

    auto target = ctx.createOffscreenTarget (width, height);
    if (target == nullptr)
        return nullptr;

    GpuCanvas::Ptr canvas = new GpuCanvas();
    canvas->ctx = &ctx;
    canvas->offscreenTarget = std::move (target);
    return canvas;
}

//==============================================================================

int GpuCanvas::getWidth() const noexcept
{
    return offscreenTarget != nullptr ? offscreenTarget->getWidth() : 0;
}

int GpuCanvas::getHeight() const noexcept
{
    return offscreenTarget != nullptr ? offscreenTarget->getHeight() : 0;
}

//==============================================================================

Graphics& GpuCanvas::beginDraw()
{
    jassert (ctx != nullptr && offscreenTarget != nullptr);

    // Drop the previous frame's Graphics so a fresh offscreen 2D frame opens on
    // the existing (already-allocated) target. cachedTexture is kept: it wraps
    // the same GPU render target whose contents are overwritten by the new frame.
    graphics.reset();
    frameOpen = false;
    committed = false;

    graphics = std::make_unique<Graphics> (*ctx, *offscreenTarget, 0u);
    frameOpen = true;

    return *graphics;
}

//==============================================================================

bool GpuCanvas::commit()
{
    if (! frameOpen || committed || ctx == nullptr || offscreenTarget == nullptr)
        return false;

    // Ensure the Y-flipped sampled mirror exists before the flush so that
    // blitMirrorIfRegistered (called inside endOffscreen) can update it.
    // No-op on non-GL backends and for canvases never used as sampled inputs.
    offscreenTarget->getOrCreateSampledTexture();

    if (graphics == nullptr || ! graphics->commitOffscreenTarget())
        return false;

    committed = true;
    return true;
}

//==============================================================================

GpuTexture::Ptr GpuCanvas::asTexture()
{
    if (offscreenTarget == nullptr)
        return nullptr;

    // Auto-commit a 2D frame opened via beginDraw() so callers don't need to
    // call commit() explicitly. Render-pass-only canvases never set frameOpen,
    // so this is a no-op for them.
    if (frameOpen && ! committed)
        commit();

    if (cachedTexture != nullptr)
        return cachedTexture;

    auto& target = *offscreenTarget;
    const int w = target.getWidth();
    const int h = target.getHeight();

    if (auto canvas = target.getRenderCanvas())
    {
        cachedTexture = GpuTexture::fromRenderCanvas (std::move (canvas), w, h);
        // Only attach the Y-flip mirror if commit() already created it.
        // GPU render-pass canvases never call commit().
        cachedTexture->sampledTexture = target.getSampledTexture();
    }
    else if (auto tex = target.adoptAsTexture())
    {
        cachedTexture = GpuTexture::fromGpuTexture (std::move (tex), w, h);
    }

    return cachedTexture;
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
    if (offscreenTarget == nullptr || ctx == nullptr)
        return false;

    if (frameOpen && ! committed)
        return false;

    return ctx->readOffscreenPixels (*offscreenTarget, dst, byteSize);
}

//==============================================================================

GpuRenderPass GpuCanvas::beginRenderPass (GpuFrame& frame, const GpuRenderOptions& options)
{
    GpuRenderPass pass;

    if (offscreenTarget == nullptr || ! frame.isValid())
        return pass;

    auto tex = asTexture();
    if (tex == nullptr)
        return pass;

    pass.impl = TypeErasedObject (GpuRenderPass::Impl {});

    auto* i = pass.getImpl();
    i->oreCtx = frame.getImpl()->oreCtx;
    i->framePools = frame.getImpl();
    i->outputTexture = tex;
    i->width = getWidth();
    i->height = getHeight();
    i->options = options;

    return pass;
}

} // namespace yup
