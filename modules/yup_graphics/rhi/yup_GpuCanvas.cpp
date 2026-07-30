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

GpuCanvas::Ptr GpuCanvas::create (GraphicsContext& context, int width, int height)
{
    if (width <= 0 || height <= 0)
        return nullptr;

    auto gpuDevice = context.getGpuDevice();
    if (gpuDevice == nullptr)
        return nullptr;

    // GpuCanvas needs a dedicated render context for the 2D drawing path.
    auto renderable = gpuDevice->createRenderableTarget (width, height);
    if (renderable == nullptr)
        return nullptr;

    auto target = GpuTarget::createFromTarget (gpuDevice, std::move (renderable));
    if (target == nullptr)
        return nullptr;

    GpuCanvas::Ptr canvas = new GpuCanvas();
    canvas->context = &context;
    canvas->target = std::move (target);

    gpuDevice->clearOffscreen (*canvas->target->getRenderableTarget(), GpuColor::transparentBlack());

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
    jassert (context != nullptr && target != nullptr);

    graphics.reset();
    frameOpen = false;
    committed = false;

    target->invalidateCachedTexture();

    graphics = std::make_unique<Graphics> (*context, *target->getRenderableTarget(), 0u);
    frameOpen = true;

    return *graphics;
}

//==============================================================================

bool GpuCanvas::commit()
{
    if (! frameOpen || committed || context == nullptr || target == nullptr)
        return false;

    auto* renderableTarget = target->getRenderableTarget();
    if (renderableTarget == nullptr)
        return false;

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
    if (target == nullptr || context == nullptr)
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
