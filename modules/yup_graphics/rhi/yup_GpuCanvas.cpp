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

    // Graphics takes ownership of the offscreen target and begins the GPU frame.
    auto g = std::make_unique<Graphics> (ctx, std::move (target), 0u);
    if (! g->isOffscreen())
        return nullptr;

    GpuCanvas::Ptr canvas = new GpuCanvas();
    canvas->ctx = &ctx;
    canvas->graphics = std::move (g);
    return canvas;
}

//==============================================================================

int GpuCanvas::getWidth() const noexcept
{
    // Access private member via friend class Graphics
    if (graphics != nullptr && graphics->offscreenTarget != nullptr)
        return graphics->offscreenTarget->getWidth();

    return 0;
}

int GpuCanvas::getHeight() const noexcept
{
    if (graphics != nullptr && graphics->offscreenTarget != nullptr)
        return graphics->offscreenTarget->getHeight();

    return 0;
}

//==============================================================================

Graphics& GpuCanvas::getGraphics() noexcept
{
    jassert (graphics != nullptr);
    return *graphics;
}

//==============================================================================

bool GpuCanvas::commit()
{
    if (graphics == nullptr || committed)
        return false;

    // commitOffscreenTarget is private - accessible via friend class Graphics
    if (! graphics->commitOffscreenTarget())
        return false;

    committed = true;
    return true;
}

//==============================================================================

GpuTexture::Ptr GpuCanvas::asTexture()
{
    if (! committed || graphics == nullptr || graphics->offscreenTarget == nullptr)
        return nullptr;

    if (cachedTexture != nullptr)
        return cachedTexture;

    auto& target = *graphics->offscreenTarget;
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
    if (! committed || graphics == nullptr || graphics->offscreenTarget == nullptr || ctx == nullptr)
        return false;

    return ctx->readOffscreenPixels (*graphics->offscreenTarget, dst, byteSize);
}

bool GpuCanvas::withOreAttachment (rive::ore::Context* oreCtx,
                                   std::function<void (rive::ore::TextureView*)> renderFunc) noexcept
{
    if (oreCtx == nullptr || ! renderFunc)
        return false;

    auto tex = asTexture();
    if (tex == nullptr)
        return false;

    rive::rcp<rive::ore::TextureView> view;

    if (auto rc = tex->getInternalRenderCanvas())
        view = oreCtx->wrapCanvasTexture (rc.get());
    else if (auto gpuTex = tex->getOrAdoptGpuTexture())
        view = oreCtx->wrapRiveTexture (gpuTex.get(), (uint32_t) tex->getWidth(), (uint32_t) tex->getHeight());

    if (view == nullptr)
        return false;

    renderFunc (view.get()); // rcp keeps view alive for the call's duration
    return true;
}

} // namespace yup
