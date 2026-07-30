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

#if YUP_EMSCRIPTEN && RIVE_WEBGPU
#include "rive/renderer/webgpu/render_context_webgpu_impl.hpp"
#include "rive/renderer/ore/ore_context.hpp"

#include <emscripten/emscripten.h>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu_cpp.h>

#include <cstdio>
#include <memory>
#include <vector>

namespace yup
{

class GpuDeviceWebGPU : public GpuDevice
{
public:
    GpuDeviceWebGPU (Options options)
        : options (options)
    {
        device = wgpu::Device::Acquire (emscripten_webgpu_get_device());
        if (device == nullptr)
        {
            fprintf (stderr, "WebGPU: no device. Ensure Module.preinitializedWebGPUDevice is set before main().\n");
            return;
        }

        queue = device.GetQueue();

        renderContext = rive::gpu::RenderContextWebGPUImpl::MakeContext (
            {}, device, queue, rive::gpu::RenderContextWebGPUImpl::ContextOptions());

        if (renderContext == nullptr)
        {
            fprintf (stderr, "WebGPU: failed to create a render context.\n");
            return;
        }

        oreContext = renderContext->static_impl_cast<rive::gpu::RenderContextWebGPUImpl>()->makeOreContext();
    }

    GpuPlatform getPlatform() const noexcept override { return GpuPlatform::WebGPU; }

    rive::gpu::RenderContext* getRenderContext() const override { return renderContext.get(); }

    rive::ore::Context* getGpuContext() const noexcept override { return oreContext.get(); }

    bool isComputeAvailable() const noexcept override { return true; }

    /** Returns the native wgpu::Device for compute operations. */
    wgpu::Device getWgpuDevice() const noexcept { return device; }

    /** Returns the native wgpu::Queue for compute operations. */
    wgpu::Queue getWgpuQueue() const noexcept { return queue; }

    //==============================================================================

    ReferenceCountedObjectPtr<GpuBuffer> createBuffer (GpuBufferType type, const void* data, size_t byteSize) override
    {
        if (type == GpuBufferType::storage)
        {
            jassert (data != nullptr && byteSize > 0);
            if (data == nullptr || byteSize == 0)
                return nullptr;

            wgpu::BufferDescriptor bufDesc {};
            bufDesc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc;
            bufDesc.size = byteSize;
            bufDesc.label = "GpuBuffer storage";

            wgpu::Buffer wgpuBuffer = device.CreateBuffer (&bufDesc);
            if (wgpuBuffer == nullptr)
                return nullptr;

            queue.WriteBuffer (wgpuBuffer, 0, data, byteSize);

            return GpuBuffer::createWithImpl (GpuBuffer::Impl { type, byteSize, {}, std::move (wgpuBuffer) });
        }

        return GpuDevice::createBuffer (type, data, byteSize);
    }

    //==============================================================================

    bool updateBuffer (GpuBuffer::Ptr buffer, const void* data, size_t byteSize) override
    {
        if (buffer == nullptr || data == nullptr || byteSize == 0)
            return false;

        auto* impl = buffer->getImpl();
        if (impl == nullptr || impl->webgpuStorageBuffer == nullptr)
            return false;

        if (byteSize > buffer->getSizeInBytes())
            return false;

        queue.WriteBuffer (impl->webgpuStorageBuffer, 0, data, byteSize);
        return true;
    }

    //==============================================================================

    bool readBuffer (GpuBuffer::Ptr buffer, void* dst, size_t dstSize) override
    {
        if (buffer == nullptr || dst == nullptr || dstSize == 0)
            return false;

        auto* impl = buffer->getImpl();
        if (impl == nullptr || impl->webgpuStorageBuffer == nullptr)
            return false;

        const auto byteSize = buffer->getSizeInBytes();
        if (dstSize < byteSize)
            return false;

        // Create a staging buffer for readback
        wgpu::BufferDescriptor stagingDesc {};
        stagingDesc.usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst;
        stagingDesc.size = byteSize;
        stagingDesc.label = "GpuBuffer readback staging";

        wgpu::Buffer staging = device.CreateBuffer (&stagingDesc);
        if (staging == nullptr)
            return false;

        // Copy storage buffer → staging buffer
        {
            wgpu::CommandEncoderDescriptor encDesc {};
            encDesc.label = "GpuBuffer readback copy";
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder (&encDesc);
            if (encoder == nullptr)
                return false;

            encoder.CopyBufferToBuffer (impl->webgpuStorageBuffer, 0, staging, 0, byteSize);

            wgpu::CommandBuffer commands = encoder.Finish();
            if (commands != nullptr)
                queue.Submit (1, &commands);
        }

        // Map the staging buffer and read back.
        struct ReadbackContext
        {
            bool done = false;
            WGPUMapAsyncStatus status = WGPUMapAsyncStatus_Success;
        } ctx;

        WGPUBufferMapCallbackInfo callbackInfo = WGPU_BUFFER_MAP_CALLBACK_INFO_INIT;
        callbackInfo.callback = [] (WGPUMapAsyncStatus status, WGPUStringView, void* userdata1, void*)
        {
            auto* c = static_cast<ReadbackContext*> (userdata1);
            c->status = status;
            c->done = true;
        };
        callbackInfo.userdata1 = &ctx;
        callbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
        wgpuBufferMapAsync (staging.Get(), WGPUMapMode_Read, 0, byteSize, callbackInfo);

        // On Emscripten, MapAsync may be asynchronous; if the callback hasn't
        // fired synchronously, the readback fails gracefully.
        if (! ctx.done || ctx.status != WGPUMapAsyncStatus_Success)
            return false;

        const void* mapped = staging.GetConstMappedRange (0, byteSize);
        if (mapped == nullptr)
        {
            staging.Unmap();
            return false;
        }

        memcpy (dst, mapped, byteSize);
        staging.Unmap();
        return true;
    }

    //==============================================================================

    struct OffscreenContextSlot
    {
        std::unique_ptr<rive::gpu::RenderContext> renderContext;
        bool frameActive = false;
        bool leased = false;
    };

    struct OffscreenTargetWebGPU : public RenderableTarget
    {
        ~OffscreenTargetWebGPU() override
        {
            if (contextSlot != nullptr)
                contextSlot->leased = false;
        }

        int width = 0;
        int height = 0;
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
    };

    std::unique_ptr<OffscreenTarget> createOffscreenTarget (int width, int height) override
    {
        if (width <= 0 || height <= 0 || renderContext == nullptr)
            return nullptr;

        auto target = std::make_unique<OffscreenTargetWebGPU>();
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

        auto target = std::make_unique<OffscreenTargetWebGPU>();
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
        auto& target = static_cast<OffscreenTargetWebGPU&> (baseTarget);
        auto* renderContext = target.getRenderContext();

        if (renderContext == nullptr || target.contextSlot == nullptr || target.contextSlot->frameActive)
            return;

        renderContext->beginFrame (frameDesc);
        target.contextSlot->frameActive = true;
    }

    void endOffscreen (OffscreenTarget& baseTarget) override
    {
        auto& target = static_cast<OffscreenTargetWebGPU&> (baseTarget);
        auto* renderContext = target.getRenderContext();

        if (renderContext == nullptr || target.contextSlot == nullptr || ! target.contextSlot->frameActive)
            return;

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

        renderContext->flush ({ .renderTarget = target.getRenderTarget(),
                                .externalCommandBuffer = encoder.Get() });

        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit (1, &commands);

        target.contextSlot->frameActive = false;
    }

    bool clearOffscreen (OffscreenTarget& baseTarget, GpuColor color) override
    {
        auto& target = static_cast<OffscreenTargetWebGPU&> (baseTarget);

        auto* renderTarget = static_cast<rive::gpu::RenderTargetWebGPU*> (target.getRenderTarget());
        if (renderTarget == nullptr)
            return false;

        wgpu::RenderPassColorAttachment attachment {};
        attachment.view = renderTarget->targetTextureView();
        attachment.loadOp = wgpu::LoadOp::Clear;
        attachment.storeOp = wgpu::StoreOp::Store;
        attachment.clearValue = { color.red, color.green, color.blue, color.alpha };

        wgpu::RenderPassDescriptor descriptor {};
        descriptor.colorAttachmentCount = 1;
        descriptor.colorAttachments = std::addressof (attachment);

        // A pass with no draws still performs the attachment's load operation.
        auto encoder = device.CreateCommandEncoder();
        auto pass = encoder.BeginRenderPass (std::addressof (descriptor));
        pass.End();

        auto commands = encoder.Finish();
        queue.Submit (1, std::addressof (commands));

        return true;
    }

    bool readOffscreenPixels (OffscreenTarget&, void*, size_t) override
    {
        return false; // GPU-to-CPU buffer mapping is async-only on the web.
    }

private:
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

        auto slot = std::make_unique<OffscreenContextSlot>();
        slot->renderContext = rive::gpu::RenderContextWebGPUImpl::MakeContext (
            {}, device, queue, rive::gpu::RenderContextWebGPUImpl::ContextOptions());
        if (slot->renderContext == nullptr)
            return nullptr;

        slot->leased = true;

        auto* result = slot.get();
        offscreenContextPool.push_back (std::move (slot));
        return result;
    }

    Options options;
    wgpu::Device device;
    wgpu::Queue queue;
    std::unique_ptr<rive::gpu::RenderContext> renderContext;
    std::vector<std::unique_ptr<OffscreenContextSlot>> offscreenContextPool;
    std::unique_ptr<rive::ore::Context> oreContext;
};

//==============================================================================

std::unique_ptr<GpuDevice> yup_constructWebGPUGpuDevice (GpuDevice::Options options)
{
    return std::make_unique<GpuDeviceWebGPU> (options);
}

} // namespace yup
#endif
