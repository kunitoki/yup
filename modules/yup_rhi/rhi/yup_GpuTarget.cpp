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

namespace
{

/** Returns the size of mip level @p level of a base dimension, never below 1. */
int mipExtentOf (int base, uint32_t level) noexcept
{
    return jmax (1, base >> (int) level);
}

} // namespace

//==============================================================================

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

GpuTarget::Ptr GpuTarget::create (GpuDevice::Ptr ctx, const GpuTextureDesc& desc)
{
    auto renderableDesc = desc;
    renderableDesc.renderTarget = true;

    auto texture = GpuTexture::create (ctx, renderableDesc);
    if (texture == nullptr)
        return nullptr;

    return createFromTexture (std::move (ctx), std::move (texture), {});
}

GpuTarget::Ptr GpuTarget::createFromTexture (GpuDevice::Ptr ctx, GpuTexture::Ptr texture, const GpuTextureViewDesc& view)
{
    if (ctx == nullptr || texture == nullptr)
        return nullptr;

    if (! texture->isRenderTarget())
        return nullptr;

    if (view.baseMipLevel >= texture->getMipLevels() || view.baseLayer >= texture->getDepthOrArrayLayers())
        return nullptr;

    GpuTarget::Ptr result = new GpuTarget();
    result->ctx = std::move (ctx);
    result->ownedTexture = std::move (texture);
    result->ownedView = view;
    result->cachedTexture = result->ownedTexture;
    return result;
}

GpuTarget::~GpuTarget()
{
    if (ctx == nullptr)
        return;

    ctx->runOnGraphicsContext ([this]
    {
        cachedTexture = nullptr;
        ownedTexture = nullptr;
        offscreenTarget = nullptr;
    });
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
    if (ownedTexture != nullptr)
        return mipExtentOf (ownedTexture->getWidth(), ownedView.baseMipLevel);

    return offscreenTarget != nullptr ? offscreenTarget->getWidth() : 0;
}

int GpuTarget::getHeight() const noexcept
{
    if (ownedTexture != nullptr)
        return mipExtentOf (ownedTexture->getHeight(), ownedView.baseMipLevel);

    return offscreenTarget != nullptr ? offscreenTarget->getHeight() : 0;
}

//==============================================================================

GpuTexture::Ptr GpuTarget::asTexture()
{
    if (ownedTexture != nullptr)
        return ownedTexture;

    if (offscreenTarget == nullptr)
        return nullptr;

    if (cachedTexture != nullptr)
        return cachedTexture;

    auto& target = *offscreenTarget;
    const int w = target.getWidth();
    const int h = target.getHeight();

    if (auto canvas = target.getRenderCanvas())
    {
        cachedTexture = GpuTexture::fromRenderCanvas (ctx, std::move (canvas), w, h);
        cachedTexture->sampledTexture = target.getSampledTexture();
    }
    else if (auto tex = target.adoptAsTexture())
    {
        cachedTexture = GpuTexture::fromGpuTexture (ctx, std::move (tex), w, h);
    }

    return cachedTexture;
}

bool GpuTarget::readPixels (void* dst, size_t byteSize)
{
    if (offscreenTarget == nullptr || ctx == nullptr)
        return false;

    if (dst == nullptr || byteSize < (size_t) getWidth() * (size_t) getHeight() * 4u)
        return false;

    return ctx->readOffscreenPixels (*offscreenTarget, dst, byteSize);
}

//==============================================================================

GpuRenderPass GpuTarget::beginRenderPass (GpuFrame& frame, const GpuRenderOptions& options)
{
    GpuRenderPass pass;

    if ((offscreenTarget == nullptr && ownedTexture == nullptr) || ! frame.isValid())
        return pass;

    auto tex = asTexture();
    if (tex == nullptr)
        return pass;

    pass.impl = TypeErasedObject (GpuRenderPass::Impl {});

    auto* i = pass.getImpl();
    i->oreCtx = frame.getImpl()->oreCtx;
    i->framePools = frame.getImpl();
    i->colorAttachments[0].texture = tex;
    i->colorAttachments[0].view = ownedView;
    i->colorAttachments[0].options = options;
    i->colorCount = 1;
    i->width = getWidth();
    i->height = getHeight();

    return pass;
}

} // namespace yup
