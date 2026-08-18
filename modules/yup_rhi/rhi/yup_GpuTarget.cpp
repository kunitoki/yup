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

GpuTarget::Ptr GpuTarget::create (GpuDevice::Ptr ctx, int width, int height)
{
    if (width <= 0 || height <= 0)
        return nullptr;

    auto target = ctx->createOffscreenTarget (width, height);
    if (target == nullptr)
        return nullptr;

    GpuTarget::Ptr result = new GpuTarget();
    result->ctx = ctx;
    result->offscreenTarget = std::move (target);
    return result;
}

GpuTarget::Ptr GpuTarget::createFromTarget (GpuDevice::Ptr ctx, std::unique_ptr<RenderableTarget> target)
{
    if (target == nullptr)
        return nullptr;

    GpuTarget::Ptr result = new GpuTarget();
    result->ctx = ctx;
    result->renderableTarget = target.get();
    result->offscreenTarget = std::move (target);
    return result;
}

//==============================================================================

int GpuTarget::getWidth() const noexcept
{
    return offscreenTarget != nullptr ? offscreenTarget->getWidth() : 0;
}

int GpuTarget::getHeight() const noexcept
{
    return offscreenTarget != nullptr ? offscreenTarget->getHeight() : 0;
}

//==============================================================================

GpuTexture::Ptr GpuTarget::asTexture()
{
    if (offscreenTarget == nullptr)
        return nullptr;

    if (cachedTexture != nullptr)
        return cachedTexture;

    auto& target = *offscreenTarget;
    const int w = target.getWidth();
    const int h = target.getHeight();

    if (auto canvas = target.getRenderCanvas())
    {
        cachedTexture = GpuTexture::fromRenderCanvas (std::move (canvas), w, h);
        cachedTexture->sampledTexture = target.getSampledTexture();
    }
    else if (auto tex = target.adoptAsTexture())
    {
        cachedTexture = GpuTexture::fromGpuTexture (std::move (tex), w, h);
    }

    return cachedTexture;
}

bool GpuTarget::readPixels (void* dst, size_t byteSize)
{
    if (offscreenTarget == nullptr || ctx == nullptr)
        return false;

    return ctx->readOffscreenPixels (*offscreenTarget, dst, byteSize);
}

//==============================================================================

GpuRenderPass GpuTarget::beginRenderPass (GpuFrame& frame, const GpuRenderOptions& options)
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
