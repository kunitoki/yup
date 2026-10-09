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

        // Scripted canvases and their GPU passes are submitted through the render context's own
        // queue, which must be the one the window frame is flushed on so Metal orders them.
        if (renderContext != nullptr)
            renderContext->static_impl_cast<rive::gpu::RenderContextMetalImpl>()->setCommandQueue (queue);
    }

    //==============================================================================

    ~GpuDeviceMetal() override
    {
        waitForCompletedHandlers();
        releasePooledResources();
    }

    GpuPlatform getPlatform() const noexcept override { return GpuPlatform::Metal; }

    rive::gpu::RenderContext* getRenderContext() const override { return renderContext.get(); }

    rive::ore::Context* getGpuContext() const noexcept override { return oreContext.get(); }

    bool isComputeAvailable() const noexcept override { return true; }

    bool isFragmentStorageBufferAvailable() const noexcept override { return true; }

    bool isStorageTextureAvailable() const noexcept override { return true; }

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

        // Storage buffers: write into the shared MTLBuffer, or into another backing when work
        // that may still read it is in flight, so the update never waits for the GPU
        if (impl->mtlStorageBuffer != nil)
        {
            if (byteSize > buffer->getSizeInBytes())
                return false;

            if (impl->mtlUses.isInFlight() && ! swapStorageBacking (*impl, byteSize))
                return false;

            std::memcpy ([impl->mtlStorageBuffer contents], data, byteSize);
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

    void beginOffscreen (OffscreenTarget& baseTarget, const GpuFrameDescriptor& frameDesc) override
    {
        auto& target = static_cast<OffscreenTargetMetal&> (baseTarget);
        auto* rc = target.getRenderContext();

        if (rc == nullptr || target.contextSlot == nullptr || target.contextSlot->frameActive)
            return;

        rc->beginFrame (toRiveFrameDescriptor (frameDesc));
        target.contextSlot->frameActive = true;
    }

    void endOffscreen (OffscreenTarget& baseTarget) override
    {
        auto& target = static_cast<OffscreenTargetMetal&> (baseTarget);
        auto* rc = target.getRenderContext();

        if (rc == nullptr || target.contextSlot == nullptr || ! target.contextSlot->frameActive)
            return;

        YUP_AUTORELEASEPOOL
        {
            id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
            rc->flush ({ .renderTarget = target.getRenderTarget(), .externalCommandBuffer = (__bridge void*) commandBuffer });
            [commandBuffer commit];
        }

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

        YUP_AUTORELEASEPOOL
        {
            id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
            if (commandBuffer == nil)
                return false;

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

            if (commandBuffer.status == MTLCommandBufferStatusError)
            {
                YUP_DBG ("GpuDeviceMetal: readback failed: " << describeFailure (commandBuffer));
                return false;
            }
        }

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

    void* getNativeDevice() const noexcept override { return (__bridge void*) device; }

    void* getNativeCommandQueue() const noexcept override { return (__bridge void*) queue; }

    /** True when the device and its command queue were created. */
    bool isUsable() const noexcept { return device != nil && queue != nil && oreContext != nullptr; }

protected:
    //==============================================================================
    /** Frames record into command buffers owned here rather than by ore, so their
        GPU status can be checked once they complete. */
    bool reportsFrameCompletion() const noexcept override { return true; }

    void* beginFrameCommands (uint64_t generation) override
    {
        id<MTLCommandBuffer> commandBuffer = nil;

        YUP_AUTORELEASEPOOL
        {
#if YUP_DEBUG
            // Names the encoder that faulted, at a runtime cost only debug builds pay
            if (@available (macOS 11.0, iOS 14.0, *))
            {
                MTLCommandBufferDescriptor* descriptor = [MTLCommandBufferDescriptor new];
                descriptor.errorOptions = MTLCommandBufferErrorOptionEncoderExecutionStatus;
                commandBuffer = [queue commandBufferWithDescriptor:descriptor];
            }
#endif

            if (commandBuffer == nil)
                commandBuffer = [queue commandBuffer];
        }

        if (commandBuffer == nil)
            return nullptr;

        // Not the device: the handler may run after it was destroyed, when the destructor could
        // not create the command buffer it waits for the handlers with
        const auto completeFrame = getFrameCompletion();

        [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> completed)
        {
            completeFrame (generation, completed.status == MTLCommandBufferStatusError
                                           ? Result::fail (describeFailure (completed))
                                           : Result::ok());
        }];

        return (__bridge_retained void*) commandBuffer;
    }

    Result submitFrameCommands (void* commandBuffer) override
    {
        if (commandBuffer == nullptr)
            return Result::ok();

        YUP_AUTORELEASEPOOL
        {
            [(__bridge id<MTLCommandBuffer>) commandBuffer commit];
        }

        return Result::ok();
    }

    Result waitFrameCommands (void* commandBuffer) override
    {
        if (commandBuffer == nullptr)
            return Result::ok();

        id<MTLCommandBuffer> frameCommands = (__bridge id<MTLCommandBuffer>) commandBuffer;

        // Waiting on a command buffer that was never committed would never return
        if (frameCommands.status == MTLCommandBufferStatusNotEnqueued)
            return Result::ok();

        [frameCommands waitUntilCompleted];

        if (frameCommands.status == MTLCommandBufferStatusError)
            return Result::fail (describeFailure (frameCommands));

        return Result::ok();
    }

    void releaseFrameCommands (void* commandBuffer) override
    {
        if (commandBuffer != nullptr)
            CFRelease (commandBuffer);
    }

private:
    /** Makes a backing no in-flight work uses the storage buffer's current one. Earlier backings
        are recycled once their last use completed, so steady updates stop allocating after a
        pool miss or two. The old contents are carried over when the update does not cover the
        whole buffer. At most framesInFlight spares are kept: a dropped one stays alive as long
        as a command buffer still uses it, since Metal retains what it encoded. */
    bool swapStorageBacking (GpuBuffer::Impl& impl, size_t updateSize)
    {
        auto& spares = impl.mtlSpareBackings;
        auto idle = std::find_if (spares.begin(), spares.end(), [] (const auto& spare)
        {
            return ! spare.uses.isInFlight();
        });

        id<MTLBuffer> fresh = nil;
        if (idle != spares.end())
        {
            fresh = idle->buffer;
            spares.erase (idle);
        }
        else
        {
            fresh = [device newBufferWithLength:impl.byteSize options:MTLResourceStorageModeShared];
            if (fresh == nil)
                return false;
        }

        if (updateSize < impl.byteSize)
            std::memcpy ([fresh contents], [impl.mtlStorageBuffer contents], impl.byteSize);

        spares.push_back ({ impl.mtlStorageBuffer, std::move (impl.mtlUses) });
        impl.mtlStorageBuffer = fresh;
        impl.mtlUses = {};

        if (spares.size() > framesInFlight)
            spares.erase (spares.begin());

        return true;
    }

    /** Describes why a command buffer failed, naming the encoders that faulted when known. */
    static String describeFailure (id<MTLCommandBuffer> commandBuffer)
    {
        String message ("Metal command buffer failed");

        YUP_AUTORELEASEPOOL
        {
            NSError* error = commandBuffer.error;
            if (error == nil)
                return message;

            message << ": " << String::fromUTF8 (error.localizedDescription.UTF8String);

            if (@available (macOS 11.0, iOS 14.0, *))
            {
                NSArray* encoderInfos = error.userInfo[MTLCommandBufferEncoderInfoErrorKey];
                for (id<MTLCommandBufferEncoderInfo> info in encoderInfos)
                {
                    if (info.errorState == MTLCommandEncoderErrorStateFaulted)
                        message << " (encoder '" << String::fromUTF8 (info.label.UTF8String) << "' faulted)";
                }
            }
        }

        return message;
    }

    struct OffscreenContextSlot
    {
        std::unique_ptr<rive::gpu::RenderContext> renderContext;
        bool frameActive = false;
        std::atomic<bool> leased = false;
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

    /** Rive's completion handlers reference the render contexts that flushed into this queue, so they
        must all have run before the contexts are destroyed. A queue completes its command buffers in
        commit order and runs their handlers serially, so the handler of a last empty command buffer
        runs after every earlier one. */
    void waitForCompletedHandlers()
    {
        dispatch_semaphore_t done = dispatch_semaphore_create (0);

        YUP_AUTORELEASEPOOL
        {
            id<MTLCommandBuffer> fence = [queue commandBuffer];
            if (fence == nil)
                return;

            [fence addCompletedHandler:^(id<MTLCommandBuffer>)
            {
                dispatch_semaphore_signal (done);
            }];
            [fence commit];
        }

        dispatch_semaphore_wait (done, DISPATCH_TIME_FOREVER);
    }

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
    auto device = std::make_unique<GpuDeviceMetal> (options);
    if (! device->isUsable())
        return nullptr;

    return device;
}

} // namespace yup
#endif
