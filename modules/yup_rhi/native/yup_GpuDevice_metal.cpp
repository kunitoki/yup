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
        : options (options)
    {
        rive::gpu::RenderContextMetalImpl::ContextOptions renderContexOptions;

        if (options.synchronousShaderCompilations)
            renderContexOptions.shaderCompilationMode = rive::gpu::ShaderCompilationMode::alwaysSynchronous;

        if (options.disableRasterOrdering)
            renderContexOptions.disableFramebufferReads = true;

        renderContext = rive::gpu::RenderContextMetalImpl::MakeContext (device, renderContexOptions);
        oreContext = rive::ore::ContextMetal::Make (device, queue);
    }

    //==============================================================================

    ~GpuDeviceMetal() override { releasePooledResources(); }

    GpuPlatform getPlatform() const noexcept override { return GpuPlatform::Metal; }

    rive::gpu::RenderContext* getRenderContext() const override { return renderContext.get(); }

    rive::ore::Context* getGpuContext() const noexcept override { return oreContext.get(); }

    bool isComputeAvailable() const noexcept override { return true; }

    //==============================================================================

    ReferenceCountedObjectPtr<GpuBuffer> createBuffer (GpuBufferType type, const void* data, size_t byteSize) override
    {
        if (type == GpuBufferType::storage)
        {
            jassert (data != nullptr && byteSize > 0);
            if (data == nullptr || byteSize == 0)
                return nullptr;

            MTLResourceOptions options = MTLResourceStorageModeShared;
            id<MTLBuffer> mtlBuffer = [device newBufferWithBytes:data
                                                          length:byteSize
                                                         options:options];
            if (mtlBuffer == nil)
                return nullptr;

            return GpuBuffer::createWithImpl (GpuBuffer::Impl { .type = type, .byteSize = byteSize, .mtlStorageBuffer = mtlBuffer });
        }

        return GpuDevice::createBuffer (type, data, byteSize);
    }

    //==============================================================================

    bool readBuffer (GpuBuffer::Ptr buffer, void* dst, size_t dstSize) override
    {
        if (buffer == nullptr || dst == nullptr)
            return false;

        auto* impl = buffer->getImpl();
        if (impl == nullptr || impl->mtlStorageBuffer == nil)
            return false;

        const auto byteSize = buffer->getSizeInBytes();
        if (dstSize < byteSize)
            return false;

        YUP_AUTORELEASEPOOL
        {
            // Fence the GPU queue with a tiny fill so we know the compute
            // dispatch has finished, then read directly from the shared buffer.
            // Avoids a full-size staging allocation + blit (~2 MB for the
            // particle demo) by exploiting Apple Silicon's unified memory.
            id<MTLBuffer> fenceBuf = [device newBufferWithLength:4
                                                         options:MTLResourceStorageModeShared];
            if (fenceBuf == nil)
                return false;

            id<MTLCommandBuffer> cmd = [queue commandBuffer];
            id<MTLBlitCommandEncoder> blit = [cmd blitCommandEncoder];
            [blit fillBuffer:fenceBuf range:NSMakeRange (0, 4) value:0];
            [blit endEncoding];
            [cmd commit];
            [cmd waitUntilCompleted];

            std::memcpy (dst, [impl->mtlStorageBuffer contents], byteSize);
        }

        return true;
    }

    //==============================================================================

    bool updateBuffer (GpuBuffer::Ptr buffer, const void* data, size_t byteSize) override
    {
        if (buffer == nullptr || data == nullptr || byteSize == 0)
            return false;

        auto* impl = buffer->getImpl();
        if (impl == nullptr)
            return false;

        // Storage buffers: write directly into the shared MTLBuffer.
        if (impl->mtlStorageBuffer != nil)
        {
            if (byteSize > buffer->getSizeInBytes())
                return false;

            YUP_AUTORELEASEPOOL
            {
                // Serialise after all prior GPU work with a blit, then write.
                id<MTLBuffer> stagingBuf = [device newBufferWithLength:4
                                                               options:MTLResourceStorageModeShared];

                id<MTLCommandBuffer> cmd = [queue commandBuffer];
                id<MTLBlitCommandEncoder> blit = [cmd blitCommandEncoder];
                [blit fillBuffer:stagingBuf range:NSMakeRange (0, 4) value:0];
                [blit endEncoding];
                [cmd commit];
                [cmd waitUntilCompleted];

                std::memcpy ([impl->mtlStorageBuffer contents], data, byteSize);
            }

            return true;
        }

        // Vertex, index, and uniform buffers go through ore's update().
        return GpuDevice::updateBuffer (buffer, data, byteSize);
    }

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

    bool clearOffscreen (OffscreenTarget& baseTarget, GpuColor color) override
    {
        auto& target = static_cast<OffscreenTargetMetal&> (baseTarget);

        id<MTLTexture> texture = target.targetTexture();
        if (texture == nil)
            return false;

        YUP_AUTORELEASEPOOL
        {
            MTLRenderPassDescriptor* descriptor = [MTLRenderPassDescriptor renderPassDescriptor];
            descriptor.colorAttachments[0].texture = texture;
            descriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
            descriptor.colorAttachments[0].clearColor = MTLClearColorMake (color.red, color.green, color.blue, color.alpha);
            descriptor.colorAttachments[0].storeAction = MTLStoreActionStore;

            // An encoder with no draws still performs the attachment's load action.
            id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
            id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:descriptor];
            [encoder endEncoding];
            [commandBuffer commit];
        }

        return true;
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
            target.stagingTexture = [device newTextureWithDescriptor:stagingDesc];
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
    id<MTLDevice> getDevice() const noexcept { return device; }

    /** Returns the native MTLCommandQueue. Used by GraphicsContextMetal. */
    id<MTLCommandQueue> getCommandQueue() const noexcept { return queue; }

private:
    struct OffscreenContextSlot
    {
        std::unique_ptr<rive::gpu::RenderContext> renderContext;
        bool frameActive = false;
        bool leased = false;
    };

    struct OffscreenTargetMetal : public RenderableTarget
    {
        ~OffscreenTargetMetal() override
        {
            if (contextSlot != nullptr)
                contextSlot->leased = false;
        }

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
            if (! slot->leased)
            {
                slot->leased = true;
                return slot.get();
            }
        }

        rive::gpu::RenderContextMetalImpl::ContextOptions renderCtxOpts;
        if (options.synchronousShaderCompilations)
            renderCtxOpts.shaderCompilationMode = rive::gpu::ShaderCompilationMode::alwaysSynchronous;
        if (options.disableRasterOrdering)
            renderCtxOpts.disableFramebufferReads = true;

        auto slot = std::make_unique<OffscreenContextSlot>();
        slot->renderContext = rive::gpu::RenderContextMetalImpl::MakeContext (device, renderCtxOpts);
        if (slot->renderContext == nullptr)
            return nullptr;

        slot->leased = true;

        auto* result = slot.get();
        offscreenContextPool.push_back (std::move (slot));
        return result;
    }

    const GpuDevice::Options options;
    std::unique_ptr<rive::gpu::RenderContext> renderContext;
    std::vector<std::unique_ptr<OffscreenContextSlot>> offscreenContextPool;
    std::unique_ptr<rive::ore::ContextMetal> oreContext;
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    id<MTLCommandQueue> queue = [device newCommandQueue];
};

//==============================================================================

std::unique_ptr<GpuDevice> yup_constructMetalGpuDevice (GpuDevice::Options options)
{
    return std::make_unique<GpuDeviceMetal> (options);
}

} // namespace yup
#endif
