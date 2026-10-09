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

#if YUP_RIVE_USE_VULKAN

namespace yup
{

namespace
{

//==============================================================================

/** Returns vkGetInstanceProcAddr from the system Vulkan loader, opened once for the
    process and kept, or nullptr when there is no loader. */
PFN_vkGetInstanceProcAddr getSystemInstanceProcAddr()
{
    static DynamicLibrary vulkanLibrary;

    static const auto getInstanceProcAddr = []() -> PFN_vkGetInstanceProcAddr
    {
        const char* const names[] = {
#if YUP_WINDOWS
            "vulkan-1.dll",
#elif YUP_APPLE
            "libvulkan.1.dylib",
            "libvulkan.dylib",
            "libMoltenVK.dylib",
#else
            "libvulkan.so.1",
            "libvulkan.so",
#endif
        };

        for (const auto* name : names)
        {
            if (! vulkanLibrary.open (name))
                continue;

            if (auto* function = vulkanLibrary.getFunction ("vkGetInstanceProcAddr"))
                return reinterpret_cast<PFN_vkGetInstanceProcAddr> (function);
        }

        vulkanLibrary.close();
        return nullptr;
    }();

    return getInstanceProcAddr;
}

VulkanDevice::Options toVulkanDeviceOptions (const GpuDevice::Options& options)
{
    VulkanDevice::Options result;
    result.getInstanceProcAddr = options.vulkan.getInstanceProcAddr != nullptr
                                   ? reinterpret_cast<PFN_vkGetInstanceProcAddr> (options.vulkan.getInstanceProcAddr)
                                   : getSystemInstanceProcAddr();
    result.instanceExtensions = options.vulkan.instanceExtensions;

    if (options.vulkan.presentationSupport)
    {
        result.presentationSupport = [support = options.vulkan.presentationSupport] (VkInstance instance, VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex)
        {
            return support (instance, physicalDevice, queueFamilyIndex);
        };
    }

#if YUP_DEBUG
    result.enableValidation = true;
#endif

    return result;
}

} // namespace

//==============================================================================
/** The Vulkan GpuDevice.

    Owns the VulkanDevice, the Rive render contexts and the ore context. Every
    submission goes through the VulkanDevice command ring, taking its generation
    from the GpuDevice frame counter, so Rive and ore share one monotonic frame
    number and a fence-derived safe frame number.

    Entry points that may run off the render thread go through
    Options::contextActivator and the device lock, so they serialize with a frame
    in progress. GpuFrames, like the ore context, are used from one thread at a time.
*/
class GpuDeviceVulkan : public GpuDevice
{
public:
    GpuDeviceVulkan (Options optionsToUse, std::unique_ptr<VulkanDevice> device)
        : options (std::move (optionsToUse))
        , vulkanDevice (std::move (device))
    {
        vulkanDevice->setFrameClock ([this]
                                     {
            return beginFrameGeneration();
        });

        // Every render context has its own resource manager, and a resource drawn by one may be
        // released by another, so they all count frames on the device clock: each release is
        // then tagged with a generation no earlier than the last work that used it
        vulkanDevice->setFrameObserver ([this] (uint64_t generation, uint64_t safeGeneration)
                                        {
            advanceResourceManagers (generation, safeGeneration);
        });

        const ScopedLock sl (vulkanDevice->getLock());

        renderContext = makeRenderContext();
        if (renderContext == nullptr)
            return;

        // The ore context shares the main render context's resource manager, which is what
        // lets Rive and ore advance one frame number
        vulkanContext = rive::ref_rcp (renderContext->static_impl_cast<rive::gpu::RenderContextVulkanImpl>()->vulkanContext());
        oreContext = rive::ore::ContextVulkan::Make (vulkanContext);
    }

    ~GpuDeviceVulkan() override
    {
        const ScopedLock sl (vulkanDevice->getLock());

        vulkanDevice->waitIdle();

        releasePooledResources();

        oreContext.reset();
        offscreenContextPool.clear();
        vulkanContext = nullptr;
        renderContext.reset();

        vulkanDevice->setFrameClock (nullptr);
        vulkanDevice->setFrameObserver (nullptr);
    }

    bool isValid() const noexcept { return renderContext != nullptr && oreContext != nullptr; }

    /** The instance, device, queue and command ring. */
    VulkanDevice& getVulkanDevice() const noexcept { return *vulkanDevice; }

    /** The Rive Vulkan context of the main render context, which the ore context shares. */
    rive::gpu::VulkanContext& getVulkanContext() const noexcept { return *vulkanContext; }

    //==============================================================================

    GpuPlatform getPlatform() const noexcept override { return GpuPlatform::Vulkan; }

    rive::gpu::RenderContext* getRenderContext() const override { return renderContext.get(); }

    rive::ore::Context* getGpuContext() const noexcept override { return oreContext.get(); }

    void* getNativeDevice() const noexcept override { return vulkanDevice.get(); }

    void* getNativeCommandQueue() const noexcept override { return vulkanDevice->getQueue(); }

    bool isComputeAvailable() const noexcept override { return true; }

    bool isDeviceLost() const noexcept override { return vulkanDevice->isDeviceLost(); }

    void runOnGraphicsContext (const std::function<void()>& fn) const override
    {
        withDevice ([&]
                    {
            fn();
        });
    }

    //==============================================================================

    ReferenceCountedObjectPtr<GpuBuffer> createBuffer (GpuBufferType type, const void* data, size_t byteSize) override
    {
        if (type != GpuBufferType::storage)
        {
            return withDevice ([&]
                               {
                return GpuDevice::createBuffer (type, data, byteSize);
            });
        }

        jassert (data != nullptr && byteSize > 0);
        if (data == nullptr || byteSize == 0)
            return nullptr;

        return withDevice ([&]() -> ReferenceCountedObjectPtr<GpuBuffer>
                           {
            VkBufferCreateInfo bufferInfo {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = static_cast<VkDeviceSize> (byteSize);
            bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

            // Host visible, so reads and updates map it directly instead of staging through a copy
            auto buffer = vulkanContext->makeBuffer (bufferInfo, rive::gpu::vkutil::Mappability::readWrite);
            if (buffer == nullptr || ! buffer->hasContents())
                return nullptr;

            std::memcpy (buffer->contents(), data, byteSize);
            buffer->flushContents();

            return GpuBuffer::createWithImpl (GpuBuffer::Impl { .type = type, .byteSize = byteSize, .vkStorageBuffer = { std::move (buffer), this } });
        });
    }

    bool readBuffer (GpuBuffer::Ptr buffer, void* dst, size_t dstSize) override
    {
        if (buffer == nullptr || dst == nullptr)
            return false;

        auto* impl = buffer->getImpl();
        if (impl == nullptr || impl->vkStorageBuffer.buffer == nullptr)
            return false;

        const auto byteSize = buffer->getSizeInBytes();
        if (dstSize < byteSize)
            return false;

        return withDevice ([&]
                           {
            const auto& storageBuffer = impl->vkStorageBuffer;

            // The pass or frame that last used the buffer must be submitted before reading it
            const auto stillRecording = vulkanDevice->isRecording (storageBuffer.lastUseCommands, storageBuffer.lastUseGeneration);
            jassert (! stillRecording);
            if (stillRecording)
                return false;

            // Compute passes end with a barrier making shader writes visible to the host,
            // waiting for the last one that used the buffer makes them available
            vulkanDevice->waitForCommands (storageBuffer.lastUseCommands, storageBuffer.lastUseGeneration);

            auto& storage = *impl->vkStorageBuffer.buffer;
            storage.invalidateContents();
            std::memcpy (dst, storage.contents(), byteSize);
            return true;
        });
    }

    bool updateBuffer (GpuBuffer::Ptr buffer, const void* data, size_t byteSize) override
    {
        if (buffer == nullptr || data == nullptr || byteSize == 0)
            return false;

        auto* impl = buffer->getImpl();
        if (impl == nullptr)
            return false;

        if (impl->vkStorageBuffer.buffer == nullptr)
        {
            return withDevice ([&]
                               {
                return GpuDevice::updateBuffer (buffer, data, byteSize);
            });
        }

        if (byteSize > buffer->getSizeInBytes())
            return false;

        return withDevice ([&]
                           {
            // A dispatch still executing may read the previous contents
            vulkanDevice->waitForCommands (impl->vkStorageBuffer.lastUseCommands, impl->vkStorageBuffer.lastUseGeneration);

            auto& storage = *impl->vkStorageBuffer.buffer;
            std::memcpy (storage.contents(), data, byteSize);
            storage.flushContents (static_cast<VkDeviceSize> (byteSize));
            return true;
        });
    }

    //==============================================================================

    std::unique_ptr<OffscreenTarget> createOffscreenTarget (int width, int height) override
    {
        if (width <= 0 || height <= 0 || renderContext == nullptr)
            return nullptr;

        return withDevice ([&]() -> std::unique_ptr<OffscreenTarget>
                           {
            auto target = std::make_unique<OffscreenTargetVulkan>();
            target->width = width;
            target->height = height;
            target->renderCanvas = renderContext->makeRenderCanvas (static_cast<uint32_t> (width), static_cast<uint32_t> (height));
            if (target->renderCanvas == nullptr || target->renderCanvas->renderTarget() == nullptr)
                return nullptr;

            return target;
        });
    }

    std::unique_ptr<RenderableTarget> createRenderableTarget (int width, int height) override
    {
        if (width <= 0 || height <= 0)
            return nullptr;

        return withDevice ([&]() -> std::unique_ptr<RenderableTarget>
                           {
            auto* contextSlot = acquireOffscreenContext();
            if (contextSlot == nullptr)
                return nullptr;

            auto target = std::make_unique<OffscreenTargetVulkan>();
            target->width = width;
            target->height = height;
            target->renderContext = contextSlot->renderContext.get();
            target->contextSlot = contextSlot;
            target->renderCanvas = target->renderContext->makeRenderCanvas (static_cast<uint32_t> (width), static_cast<uint32_t> (height));
            if (target->renderCanvas == nullptr || target->renderCanvas->renderTarget() == nullptr)
                return nullptr;

            return target;
        });
    }

    void beginOffscreen (OffscreenTarget& baseTarget, const GpuFrameDescriptor& frameDesc) override
    {
        withDevice ([&]
                    {
            auto& target = static_cast<OffscreenTargetVulkan&> (baseTarget);
            auto* rc = target.getRenderContext();

            if (rc == nullptr || target.contextSlot == nullptr || target.contextSlot->frameActive)
                return;

            rc->beginFrame (toRiveFrameDescriptor (frameDesc));
            target.contextSlot->frameActive = true;
        });
    }

    void endOffscreen (OffscreenTarget& baseTarget) override
    {
        withDevice ([&]
                    {
            auto& target = static_cast<OffscreenTargetVulkan&> (baseTarget);
            auto* rc = target.getRenderContext();

            if (rc == nullptr || target.contextSlot == nullptr || ! target.contextSlot->frameActive)
                return;

            target.contextSlot->frameActive = false;

            // The generation is taken now, at flush time, so frames nested in the paint
            // that opened this one keep the frame numbers increasing
            const auto commands = vulkanDevice->beginCommands();
            jassert (commands.isValid());
            if (! commands.isValid())
                return;

            rc->flush ({ .renderTarget = target.getRenderTarget(),
                         .externalCommandBuffer = commands.commandBuffer,
                         .currentFrameNumber = commands.frameNumber,
                         .safeFrameNumber = commands.safeFrameNumber });

            vulkanDevice->submitCommands (commands.commandBuffer);
        });
    }

    bool clearOffscreen (OffscreenTarget& baseTarget, GpuColor color) override
    {
        return withDevice ([&]
                           {
            auto& target = static_cast<OffscreenTargetVulkan&> (baseTarget);

            auto* renderTarget = static_cast<rive::gpu::RenderTargetVulkan*> (target.getRenderTarget());
            if (renderTarget == nullptr)
                return false;

            const auto commands = vulkanDevice->beginCommands();
            if (! commands.isValid())
                return false;

            const VkImage image = renderTarget->accessTargetImage (commands.commandBuffer,
                                                                   { VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
                                                                   rive::gpu::vkutil::ImageAccessAction::invalidateContents);

            VkClearColorValue clearValue {};
            clearValue.float32[0] = color.red;
            clearValue.float32[1] = color.green;
            clearValue.float32[2] = color.blue;
            clearValue.float32[3] = color.alpha;

            VkImageSubresourceRange range {};
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            range.levelCount = 1;
            range.layerCount = 1;

            vulkanDevice->getFunctions().CmdClearColorImage (commands.commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearValue, 1, &range);

            return vulkanDevice->submitCommands (commands.commandBuffer);
        });
    }

    bool readOffscreenPixels (OffscreenTarget& baseTarget, void* dst, size_t dstSize) override
    {
        auto& target = static_cast<OffscreenTargetVulkan&> (baseTarget);

        auto* renderTarget = static_cast<rive::gpu::RenderTargetVulkan*> (target.getRenderTarget());
        if (renderTarget == nullptr || dst == nullptr)
            return false;

        const auto width = static_cast<uint32_t> (target.width);
        const auto height = static_cast<uint32_t> (target.height);
        const size_t byteSize = static_cast<size_t> (width) * height * 4u;
        if (dstSize < byteSize)
            return false;

        return withDevice ([&]
                           {
            if (target.readbackBuffer == nullptr || target.readbackBuffer->info().size < byteSize)
            {
                VkBufferCreateInfo bufferInfo {};
                bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferInfo.size = static_cast<VkDeviceSize> (byteSize);
                bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

                target.readbackBuffer = vulkanContext->makeBuffer (bufferInfo, rive::gpu::vkutil::Mappability::readWrite);
            }

            if (target.readbackBuffer == nullptr || ! target.readbackBuffer->hasContents())
                return false;

            const auto commands = vulkanDevice->beginCommands();
            if (! commands.isValid())
                return false;

            const VkImage image = renderTarget->accessTargetImage (commands.commandBuffer,
                                                                   { VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL });

            VkBufferImageCopy region {};
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.layerCount = 1;
            region.imageExtent = { width, height, 1 };

            vulkanDevice->getFunctions().CmdCopyImageToBuffer (commands.commandBuffer,
                                                               image,
                                                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                               *target.readbackBuffer,
                                                               1,
                                                               &region);

            VkBufferMemoryBarrier toHost {};
            toHost.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            toHost.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            toHost.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
            toHost.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toHost.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toHost.buffer = *target.readbackBuffer;
            toHost.size = VK_WHOLE_SIZE;

            vulkanContext->bufferMemoryBarrier (commands.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, toHost);

            if (! vulkanDevice->submitCommands (commands.commandBuffer))
                return false;

            vulkanDevice->waitForCommands (commands.commandBuffer);

            // Canvases are RGBA8 and Vulkan's origin is the top left, rows are already in order
            target.readbackBuffer->invalidateContents();
            std::memcpy (dst, target.readbackBuffer->contents(), byteSize);
            return true;
        });
    }

protected:
    //==============================================================================

    uint64_t getSafeFrameGeneration() const noexcept override
    {
        return vulkanDevice->getSafeFrameNumber (getCurrentFrameGeneration());
    }

    void* beginFrameCommands (uint64_t generation) override
    {
        return vulkanDevice->beginCommands (generation);
    }

    void submitFrameCommands (void* commandBuffer) override
    {
        if (commandBuffer != nullptr)
            vulkanDevice->submitCommands (static_cast<VkCommandBuffer> (commandBuffer));
    }

    void waitFrameCommands (void* commandBuffer) override
    {
        if (commandBuffer != nullptr)
            vulkanDevice->waitForCommands (static_cast<VkCommandBuffer> (commandBuffer));
    }

private:
    //==============================================================================

    struct OffscreenContextSlot
    {
        std::unique_ptr<rive::gpu::RenderContext> renderContext;
        bool frameActive = false;
        std::atomic<bool> leased = false;
    };

    struct OffscreenTargetVulkan : public RenderableTarget
    {
        ~OffscreenTargetVulkan() override
        {
            if (contextSlot != nullptr)
                contextSlot->leased = false;
        }

        int width = 0;
        int height = 0;
        rive::rcp<rive::gpu::RenderCanvas> renderCanvas;
        rive::rcp<rive::gpu::vkutil::Buffer> readbackBuffer;
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

    //==============================================================================

    template <class Fn>
    std::invoke_result_t<Fn> withDevice (Fn&& fn) const
    {
        using Result = std::invoke_result_t<Fn>;

        const auto run = [&]() -> Result
        {
            const ScopedLock sl (vulkanDevice->getLock());
            return std::invoke (std::forward<Fn> (fn));
        };

        if (! options.contextActivator)
            return run();

        if constexpr (std::is_void_v<Result>)
        {
            options.contextActivator ([&]
                                      {
                run();
            });
        }
        else
        {
            std::optional<Result> result;
            options.contextActivator ([&]
                                      {
                result.emplace (run());
            });

            if (! result.has_value())
                return Result {};

            return std::move (*result);
        }
    }

    void advanceResourceManagers (uint64_t generation, uint64_t safeGeneration)
    {
        auto advance = [&] (rive::gpu::RenderContext* context)
        {
            if (context != nullptr)
                context->static_impl_cast<rive::gpu::RenderContextVulkanImpl>()->vulkanContext()->advanceFrameNumber (generation, safeGeneration);
        };

        advance (renderContext.get());

        for (const auto& slot : offscreenContextPool)
            advance (slot->renderContext.get());
    }

    std::unique_ptr<rive::gpu::RenderContext> makeRenderContext() const
    {
        rive::gpu::RenderContextVulkanImpl::ContextOptions contextOptions;
        if (options.synchronousShaderCompilations)
            contextOptions.shaderCompilationMode = rive::gpu::ShaderCompilationMode::alwaysSynchronous;

        auto context = rive::gpu::RenderContextVulkanImpl::MakeContext (vulkanDevice->getInstance(),
                                                                        vulkanDevice->getPhysicalDevice(),
                                                                        vulkanDevice->getDevice(),
                                                                        vulkanDevice->getFeatures(),
                                                                        vulkanDevice->getInstanceProcAddr(),
                                                                        contextOptions);

        // Scripted canvases flush their pre-pass through a one-shot command buffer on this queue
        if (context != nullptr)
            context->static_impl_cast<rive::gpu::RenderContextVulkanImpl>()->setCanvasQueue (vulkanDevice->getQueue(), vulkanDevice->getQueueFamilyIndex());

        return context;
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

        auto slot = std::make_unique<OffscreenContextSlot>();
        slot->renderContext = makeRenderContext();
        if (slot->renderContext == nullptr)
            return nullptr;

        slot->leased = true;

        auto* result = slot.get();
        offscreenContextPool.push_back (std::move (slot));
        return result;
    }

    //==============================================================================

    const Options options;
    std::unique_ptr<VulkanDevice> vulkanDevice;
    std::unique_ptr<rive::gpu::RenderContext> renderContext;
    rive::rcp<rive::gpu::VulkanContext> vulkanContext;
    std::vector<std::unique_ptr<OffscreenContextSlot>> offscreenContextPool;
    std::unique_ptr<rive::ore::ContextVulkan> oreContext;
};

//==============================================================================

std::unique_ptr<GpuDevice> yup_constructVulkanGpuDevice (GpuDevice::Options options)
{
    auto device = VulkanDevice::create (toVulkanDeviceOptions (options));
    if (device == nullptr)
        return nullptr;

    auto result = std::make_unique<GpuDeviceVulkan> (std::move (options), std::move (device));
    if (! result->isValid())
        return nullptr;

    return result;
}

bool yup_isVulkanGpuDeviceSupported (const GpuDevice::Options& options)
{
    return VulkanDevice::isSupported (toVulkanDeviceOptions (options));
}

} // namespace yup

#endif // YUP_RIVE_USE_VULKAN
