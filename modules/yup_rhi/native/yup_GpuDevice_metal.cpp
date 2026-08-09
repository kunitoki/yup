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

#if YUP_RIVE_USE_METAL
#include "rive/renderer/rive_renderer.hpp"
#include "rive/renderer/metal/render_context_metal_impl.h"
#include "rive/renderer/ore/ore_context_metal.hpp"

#include <vector>

namespace yup
{

//==============================================================================

class GpuDeviceMetal : public GpuDevice
{
public:
    GpuDeviceMetal (GpuDevice::Options options)
        : fiddleOptions (options)
    {
        rive::gpu::RenderContextMetalImpl::ContextOptions renderCtxOpts;

        if (fiddleOptions.synchronousShaderCompilations)
            renderCtxOpts.shaderCompilationMode = rive::gpu::ShaderCompilationMode::alwaysSynchronous;

        if (fiddleOptions.disableRasterOrdering)
            renderCtxOpts.disableFramebufferReads = true;

        renderContext = rive::gpu::RenderContextMetalImpl::MakeContext (gpu, renderCtxOpts);
        oreContext = rive::ore::ContextMetal::Make (gpu, queue);
    }

    //==============================================================================

    GpuPlatform getPlatform() const noexcept override { return GpuPlatform::Metal; }

    rive::ore::Context* gpuContext() const noexcept override { return oreContext.get(); }

    bool isComputeAvailable() const noexcept override { return true; }

    //==============================================================================

    std::unique_ptr<OffscreenTarget> createOffscreenTarget (int width, int height) override
    {
        if (width <= 0 || height <= 0 || renderContext == nullptr)
            return nullptr;

        auto target = std::make_unique<OffscreenTargetMetal>();
        target->width = width;
        target->height = height;
        target->renderContext = nullptr;
        target->contextSlot = nullptr;
        target->renderCanvas = renderContext->makeRenderCanvas (static_cast<uint32_t> (width),
                                                                static_cast<uint32_t> (height));
        if (target->renderCanvas == nullptr)
            return nullptr;

        return target;
    }

    std::unique_ptr<RenderableTarget> createRenderableTarget (int width, int height) override
    {
        if (width <= 0 || height <= 0)
            return nullptr;

        auto* contextSlot = acquireOffscreenContext();
        if (contextSlot == nullptr)
            return nullptr;

        auto target = std::make_unique<OffscreenTargetMetal>();
        target->width = width;
        target->height = height;
        target->renderContext = contextSlot->renderContext.get();
        target->contextSlot = contextSlot;
        target->renderCanvas = target->renderContext->makeRenderCanvas (static_cast<uint32_t> (width),
                                                                        static_cast<uint32_t> (height));
        if (target->renderCanvas == nullptr)
            return nullptr;

        return target;
    }

    void beginOffscreen (OffscreenTarget& baseTarget, const rive::gpu::RenderContext::FrameDescriptor& frameDesc) override
    {
        auto& target = static_cast<OffscreenTargetMetal&> (baseTarget);
        auto* rc = target.getRenderContext();

        if (rc == nullptr || target.contextSlot == nullptr || target.contextSlot->frameActive)
            return;

        rc->beginFrame (frameDesc);
        target.contextSlot->frameActive = true;
    }

    void endOffscreen (OffscreenTarget& baseTarget) override
    {
        auto& target = static_cast<OffscreenTargetMetal&> (baseTarget);
        auto* rc = target.getRenderContext();

        if (rc == nullptr || target.contextSlot == nullptr || ! target.contextSlot->frameActive)
            return;

        id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
        rc->flush ({ .renderTarget = target.getRenderTarget(), .externalCommandBuffer = (__bridge void*) commandBuffer });
        [commandBuffer commit];
        target.contextSlot->frameActive = false;
    }

    bool readOffscreenPixels (OffscreenTarget& baseTarget, void* dst, size_t dstSize) override
    {
        auto& target = static_cast<OffscreenTargetMetal&> (baseTarget);

        if (dst == nullptr)
            return false;

        id<MTLTexture> srcTexture = target.targetTexture();
        if (srcTexture == nil)
            return false;

        if (target.stagingTexture == nil)
        {
            MTLTextureDescriptor* stagingDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                                   width:static_cast<NSUInteger> (target.width)
                                                                                                  height:static_cast<NSUInteger> (target.height)
                                                                                               mipmapped:NO];
            stagingDesc.usage = MTLTextureUsageShaderRead;
#if YUP_IOS
            stagingDesc.storageMode = MTLStorageModeShared;
#else
            stagingDesc.storageMode = MTLStorageModeManaged;
#endif
            target.stagingTexture = [gpu newTextureWithDescriptor:stagingDesc];
            if (target.stagingTexture == nil)
                return false;
        }

        const auto w = static_cast<NSUInteger> (target.width);
        const auto h = static_cast<NSUInteger> (target.height);
        const size_t bytesPerRow = w * 4u;

        if (dstSize < bytesPerRow * h)
            return false;

        id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];

        id<MTLBlitCommandEncoder> blitEncoder = [commandBuffer blitCommandEncoder];
        [blitEncoder copyFromTexture:srcTexture
                         sourceSlice:0
                         sourceLevel:0
                        sourceOrigin:MTLOriginMake (0, 0, 0)
                          sourceSize:MTLSizeMake (w, h, 1)
                           toTexture:target.stagingTexture
                    destinationSlice:0
                    destinationLevel:0
                   destinationOrigin:MTLOriginMake (0, 0, 0)];
#if YUP_MAC
        [blitEncoder synchronizeResource:target.stagingTexture];
#endif
        [blitEncoder endEncoding];

        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];

        [target.stagingTexture getBytes:dst
                            bytesPerRow:bytesPerRow
                             fromRegion:MTLRegionMake2D (0, 0, w, h)
                            mipmapLevel:0];

        return true;
    }

    //==============================================================================
    /** Returns the native MTLDevice. Used by GraphicsContextMetal to share the device. */
    id<MTLDevice> getDevice() const noexcept { return gpu; }

    /** Returns the native MTLCommandQueue. Used by GraphicsContextMetal. */
    id<MTLCommandQueue> getCommandQueue() const noexcept { return queue; }

private:
    struct OffscreenContextSlot
    {
        std::unique_ptr<rive::gpu::RenderContext> renderContext;
        bool frameActive = false;
    };

    struct OffscreenTargetMetal : public RenderableTarget
    {
        int width = 0;
        int height = 0;
        id<MTLTexture> stagingTexture = nil;
        rive::rcp<rive::gpu::RenderCanvas> renderCanvas;
        rive::gpu::RenderContext* renderContext = nullptr;
        OffscreenContextSlot* contextSlot = nullptr;

        int getWidth() const noexcept override { return width; }

        int getHeight() const noexcept override { return height; }

        rive::gpu::RenderTarget* getRenderTarget() noexcept override
        {
            return renderCanvas != nullptr ? renderCanvas->renderTarget() : nullptr;
        }

        rive::gpu::RenderContext* getRenderContext() noexcept override
        {
            return renderContext;
        }

        rive::rcp<rive::gpu::RenderCanvas> getRenderCanvas() noexcept override
        {
            return renderCanvas;
        }

        rive::rcp<rive::gpu::Texture> adoptAsTexture() override
        {
            if (renderCanvas == nullptr)
                return nullptr;
            return renderCanvas->renderImage()->refTexture();
        }

        id<MTLTexture> targetTexture() const
        {
            if (renderCanvas == nullptr)
                return nil;
            if (auto* target = static_cast<rive::gpu::RenderTargetMetal*> (renderCanvas->renderTarget()))
                return target->targetTexture();
            return nil;
        }
    };

    OffscreenContextSlot* acquireOffscreenContext()
    {
        for (const auto& slot : offscreenContextPool)
        {
            if (! slot->frameActive)
                return slot.get();
        }

        rive::gpu::RenderContextMetalImpl::ContextOptions renderCtxOpts;
        if (fiddleOptions.synchronousShaderCompilations)
            renderCtxOpts.shaderCompilationMode = rive::gpu::ShaderCompilationMode::alwaysSynchronous;
        if (fiddleOptions.disableRasterOrdering)
            renderCtxOpts.disableFramebufferReads = true;

        auto slot = std::make_unique<OffscreenContextSlot>();
        slot->renderContext = rive::gpu::RenderContextMetalImpl::MakeContext (gpu, renderCtxOpts);
        if (slot->renderContext == nullptr)
            return nullptr;

        auto* result = slot.get();
        offscreenContextPool.push_back (std::move (slot));
        return result;
    }

    const GpuDevice::Options fiddleOptions;
    std::unique_ptr<rive::gpu::RenderContext> renderContext;
    std::vector<std::unique_ptr<OffscreenContextSlot>> offscreenContextPool;
    std::unique_ptr<rive::ore::ContextMetal> oreContext;
    id<MTLDevice> gpu = MTLCreateSystemDefaultDevice();
    id<MTLCommandQueue> queue = [gpu newCommandQueue];
};

//==============================================================================

std::unique_ptr<GpuDevice> yup_constructMetalGpuDevice (GpuDevice::Options options)
{
    return std::make_unique<GpuDeviceMetal> (options);
}

} // namespace yup
#endif
