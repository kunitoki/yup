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

namespace
{

/** Bytes per texel for the uncompressed formats, or 0 for block-compressed ones
    (which need block-based stride maths the caller must supply). */
uint32_t bytesPerTexel (GpuTextureFormat format)
{
    return rive::ore::textureFormatBytesPerTexel (GpuPipelineHelpers::toOreTextureFormat (format));
}

/** Returns the size of mip level @p level of a base dimension, never below 1. */
uint32_t mipExtent (uint32_t base, uint32_t level)
{
    return jmax (1u, base >> level);
}

} // namespace

//==============================================================================

GpuTexture::Ptr GpuTexture::create (ReferenceCountedObjectPtr<GpuDevice> device, const GpuTextureDesc& desc)
{
    if (device == nullptr)
        return nullptr;

    auto* oreCtx = device->getGpuContext();
    if (oreCtx == nullptr)
        return nullptr;

    if (desc.width == 0 || desc.height == 0)
        return nullptr;

    if (desc.mipLevels == 0 || desc.sampleCount == 0)
        return nullptr;

    if (desc.type == GpuTextureType::cube && desc.depthOrArrayLayers != 6)
        return nullptr;

    if (desc.type == GpuTextureType::texture2D && desc.depthOrArrayLayers != 1)
        return nullptr;

    if (desc.sampleCount > 1 && ! desc.renderTarget)
        return nullptr;

    rive::ore::TextureDesc oreDesc;
    oreDesc.width = desc.width;
    oreDesc.height = desc.height;
    oreDesc.depthOrArrayLayers = desc.depthOrArrayLayers;
    oreDesc.format = GpuPipelineHelpers::toOreTextureFormat (desc.format);
    oreDesc.type = GpuPipelineHelpers::toOreTextureType (desc.type);
    oreDesc.renderTarget = desc.renderTarget;
    oreDesc.numMipmaps = desc.mipLevels;
    oreDesc.sampleCount = desc.sampleCount;
    oreDesc.label = desc.label.isNotEmpty() ? desc.label.toRawUTF8() : nullptr;

    rive::rcp<rive::ore::Texture> oreTexture;
    device->runOnGraphicsContext ([&]
    {
        oreTexture = oreCtx->makeTexture (oreDesc);
    });

    if (oreTexture == nullptr)
        return nullptr;

    GpuTexture::Ptr t = new GpuTexture();
    t->device = std::move (device);
    t->oreTexture = std::move (oreTexture);
    t->width = (int) desc.width;
    t->height = (int) desc.height;
    t->renderTarget = desc.renderTarget;
    t->format = desc.format;
    t->type = desc.type;
    t->mipLevels = desc.mipLevels;
    t->depthOrArrayLayers = desc.depthOrArrayLayers;
    t->sampleCount = desc.sampleCount;
    return t;
}

GpuTexture::~GpuTexture()
{
    if (device == nullptr)
        return;

    device->runOnGraphicsContext ([this]
    {
        viewCache.clear();

        oreTexture = nullptr;
        gpuTexture = nullptr;
        renderCanvas = nullptr;
        sampledTexture = nullptr;
    });
}

//==============================================================================

GpuTextureViewDesc GpuTexture::getWholeTextureViewDesc() const noexcept
{
    GpuTextureViewDesc desc;
    desc.dimension = GpuPipelineHelpers::defaultViewDimension (type);
    desc.baseMipLevel = 0;
    desc.mipCount = mipLevels;
    desc.baseLayer = 0;
    desc.layerCount = depthOrArrayLayers;
    return desc;
}

rive::rcp<rive::ore::TextureView> GpuTexture::getOrCreateAttachmentView (rive::ore::Context& oreCtx,
                                                                        const GpuTextureViewDesc& viewDesc) const
{
    return getOrCreateView (oreCtx, true, viewDesc);
}

rive::rcp<rive::ore::TextureView> GpuTexture::getOrCreateSamplingView (rive::ore::Context& oreCtx) const
{
    return getOrCreateView (oreCtx, false, getWholeTextureViewDesc());
}

rive::rcp<rive::ore::TextureView> GpuTexture::getOrCreateView (rive::ore::Context& oreCtx,
                                                               bool forRenderTarget,
                                                               const GpuTextureViewDesc& viewDesc) const
{
    for (const auto& cached : viewCache)
    {
        if (cached.forRenderTarget == forRenderTarget && cached.desc == viewDesc)
            return cached.view;
    }

    auto view = [&]() -> rive::rcp<rive::ore::TextureView>
    {
        if (oreTexture != nullptr)
        {
            rive::ore::TextureViewDesc desc;
            desc.texture = oreTexture.get();
            desc.dimension = GpuPipelineHelpers::toOreViewDimension (viewDesc.dimension);
            desc.aspect = GpuPipelineHelpers::toOreTextureAspect (viewDesc.aspect);
            desc.baseMipLevel = viewDesc.baseMipLevel;
            desc.mipCount = viewDesc.mipCount;
            desc.baseLayer = viewDesc.baseLayer;
            desc.layerCount = viewDesc.layerCount;
            return oreCtx.makeTextureView (desc);
        }

        if (forRenderTarget)
        {
            if (renderCanvas != nullptr)
                return oreCtx.wrapCanvasTexture (renderCanvas.get());

            if (auto tex = getOrAdoptGpuTexture())
                return oreCtx.wrapRiveTexture (tex.get(), (uint32_t) width, (uint32_t) height);

            return nullptr;
        }

        if (sampledTexture != nullptr)
            return oreCtx.wrapRiveTexture (sampledTexture.get(), (uint32_t) width, (uint32_t) height);

        if (auto tex = getOrAdoptGpuTexture())
            return oreCtx.wrapRiveTexture (tex.get(), (uint32_t) width, (uint32_t) height);

        if (renderCanvas != nullptr)
            return oreCtx.wrapCanvasTexture (renderCanvas.get());

        return nullptr;
    }();

    if (view != nullptr)
        viewCache.push_back ({ viewDesc, forRenderTarget, view });

    return view;
}

//==============================================================================

bool GpuTexture::upload (const GpuTextureDataDesc& data)
{
    jassert (oreTexture != nullptr);
    if (oreTexture == nullptr || data.data == nullptr)
        return false;

    if (data.mipLevel >= mipLevels || data.layer >= depthOrArrayLayers)
        return false;

    const uint32_t levelWidth = mipExtent ((uint32_t) width, data.mipLevel);
    const uint32_t levelHeight = mipExtent ((uint32_t) height, data.mipLevel);

    rive::ore::TextureDataDesc oreData;
    oreData.data = data.data;
    oreData.mipLevel = data.mipLevel;
    oreData.layer = data.layer;
    oreData.x = data.x;
    oreData.y = data.y;
    oreData.z = data.z;
    oreData.width = data.width != 0 ? data.width : levelWidth;
    oreData.height = data.height != 0 ? data.height : levelHeight;
    oreData.depth = data.depth;

    if (data.x + oreData.width > levelWidth || data.y + oreData.height > levelHeight)
        return false;

    const uint32_t texelSize = bytesPerTexel (format);
    oreData.bytesPerRow = data.bytesPerRow != 0 ? data.bytesPerRow : oreData.width * texelSize;
    oreData.rowsPerImage = data.rowsPerImage != 0 ? data.rowsPerImage : oreData.height;

    if (texelSize == 0 && data.bytesPerRow == 0)
        return false;

    device->runOnGraphicsContext ([&]
    {
        oreTexture->upload (oreData);
    });

    return true;
}

GpuTexture::Ptr GpuTexture::fromGpuTexture (ReferenceCountedObjectPtr<GpuDevice> device, rive::rcp<rive::gpu::Texture> texture, int width, int height)
{
    if (texture == nullptr || width <= 0 || height <= 0)
        return nullptr;

    GpuTexture::Ptr t = new GpuTexture();
    t->device = std::move (device);
    t->gpuTexture = std::move (texture);
    t->width = width;
    t->height = height;
    return t;
}

GpuTexture::Ptr GpuTexture::fromRenderCanvas (ReferenceCountedObjectPtr<GpuDevice> device, rive::rcp<rive::gpu::RenderCanvas> canvas, int width, int height)
{
    if (canvas == nullptr || width <= 0 || height <= 0)
        return nullptr;

    GpuTexture::Ptr t = new GpuTexture();
    t->device = std::move (device);
    t->renderCanvas = std::move (canvas);
    t->renderTarget = true;
    t->width = width;
    t->height = height;
    return t;
}

//==============================================================================

rive::rcp<rive::gpu::Texture> GpuTexture::getOrAdoptGpuTexture() const
{
    if (sampledTexture != nullptr)
        return sampledTexture;

    if (gpuTexture != nullptr)
        return gpuTexture;

    if (renderCanvas != nullptr)
    {
        if (auto* ri = renderCanvas->renderImage())
            gpuTexture = ri->refTexture();
    }

    return gpuTexture;
}

rive::rcp<rive::gpu::RenderCanvas> GpuTexture::getInternalRenderCanvas() const
{
    return renderCanvas;
}

//==============================================================================

int GpuTexture::getWidth() const noexcept
{
    return width;
}

int GpuTexture::getHeight() const noexcept
{
    return height;
}

GpuTextureFormat GpuTexture::getFormat() const noexcept
{
    return format;
}

GpuTextureType GpuTexture::getType() const noexcept
{
    return type;
}

uint32_t GpuTexture::getMipLevels() const noexcept
{
    return mipLevels;
}

uint32_t GpuTexture::getDepthOrArrayLayers() const noexcept
{
    return depthOrArrayLayers;
}

uint32_t GpuTexture::getSampleCount() const noexcept
{
    return sampleCount;
}

bool GpuTexture::isValid() const noexcept
{
    return oreTexture != nullptr || gpuTexture != nullptr || renderCanvas != nullptr;
}

bool GpuTexture::isRenderTarget() const noexcept
{
    return renderTarget;
}

} // namespace yup
