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

Graphics& GpuCanvas::ensureGraphics()
{
    jassert (ctx != nullptr && offscreenTarget != nullptr);

    if (graphics == nullptr)
    {
        graphics = std::make_unique<Graphics> (*ctx, *offscreenTarget, 0u);
        frameOpen = true;
    }

    return *graphics;
}

Graphics& GpuCanvas::getGraphics() noexcept
{
    return ensureGraphics();
}

//==============================================================================

bool GpuCanvas::commit()
{
    if (! frameOpen || committed || ctx == nullptr || offscreenTarget == nullptr)
        return false;

    ctx->endOffscreen (*offscreenTarget);
    committed = true;
    return true;
}

//==============================================================================

GpuTexture::Ptr GpuCanvas::asTexture()
{
    if (offscreenTarget == nullptr)
        return nullptr;

    // A canvas used purely as a GpuRenderPass target has no 2D frame to commit,
    // but one opened via getGraphics() must be committed first.
    if (frameOpen && ! committed)
        return nullptr;

    if (cachedTexture != nullptr)
        return cachedTexture;

    auto& target = *offscreenTarget;
    const int w = target.getWidth();
    const int h = target.getHeight();

    if (auto canvas = target.getRenderCanvas())
        cachedTexture = GpuTexture::fromRenderCanvas (std::move (canvas), w, h);
    else if (auto tex = target.adoptAsTexture())
        cachedTexture = GpuTexture::fromGpuTexture (std::move (tex), w, h);

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

    pass.impl = std::make_unique<GpuRenderPass::Impl>();
    pass.impl->oreCtx = frame.getImpl()->oreCtx;
    pass.impl->framePools = frame.getImpl();
    pass.impl->outputTexture = tex;
    pass.impl->width = getWidth();
    pass.impl->height = getHeight();
    pass.impl->options = options;

    return pass;
}

} // namespace yup
