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

//==============================================================================
/** Presents Rive frames on a Vulkan swapchain.

    Rive renders into a persistent offscreen canvas, which is blitted to the acquired
    swapchain image: swapchain images do not keep their contents between frames, and
    the window repaints only its dirty regions on top of the previous frame. The blit
    also converts to whatever format the surface offers.

    The surface comes from Options::vulkan, so nothing here touches a window system.
    The swapchain is created lazily and recreated when it goes out of date, when the
    window is resized or when the vsync mode changes.
*/
class GraphicsContextVulkan : public GraphicsContext
{
public:
    GraphicsContextVulkan (Options options, GpuDevice::Ptr existingGpu = {})
        : options (std::move (options))
    {
        if (existingGpu != nullptr)
            gpuDevice = std::move (existingGpu);
        else
            gpuDevice = GpuDevice::create (GpuPlatform::Vulkan, this->options);

        if (gpuDevice != nullptr && gpuDevice->getPlatform() == GpuPlatform::Vulkan)
            vulkanDevice = static_cast<VulkanDevice*> (gpuDevice->getNativeDevice());
    }

    ~GraphicsContextVulkan() override
    {
        detachFromWindow();

        if (vulkanDevice != nullptr)
        {
            const ScopedLock sl (vulkanDevice->getLock());
            canvas = nullptr;
        }
    }

    bool isValid() const noexcept { return vulkanDevice != nullptr && gpuDevice->getRenderContext() != nullptr; }

    //==============================================================================

    GpuPlatform getPlatform() const noexcept override { return GpuPlatform::Vulkan; }

    GpuDevice::Ptr getGpuDevice() const noexcept override { return gpuDevice; }

    rive::Factory* getFactory() override { return gpuDevice->getRenderContext(); }

    rive::gpu::RenderContext* getRenderContext() override { return gpuDevice->getRenderContext(); }

    rive::gpu::RenderTarget* getRenderTarget() override { return canvas != nullptr ? canvas->renderTarget() : nullptr; }

    std::unique_ptr<rive::Renderer> makeRenderer (int, int) override
    {
        return std::make_unique<rive::RiveRenderer> (getRenderContext());
    }

    //==============================================================================

    void attachToWindow (void*, int, int, float) override
    {
        if (vulkanDevice == nullptr || ! options.vulkan.createSurface || ! vulkanDevice->canPresent())
            return;

        const ScopedLock sl (vulkanDevice->getLock());

        if (surface != VK_NULL_HANDLE)
            return;

        surface = reinterpret_cast<VkSurfaceKHR> (options.vulkan.createSurface (vulkanDevice->getInstance()));
        if (surface == VK_NULL_HANDLE)
        {
            Logger::outputDebugString ("Vulkan: unable to create the window surface");
            return;
        }

        VkBool32 supported = VK_FALSE;
        vulkanDevice->getFunctions().GetPhysicalDeviceSurfaceSupportKHR (vulkanDevice->getPhysicalDevice(),
                                                                         vulkanDevice->getQueueFamilyIndex(),
                                                                         surface,
                                                                         &supported);
        if (supported != VK_TRUE)
        {
            Logger::outputDebugString ("Vulkan: the device queue cannot present to the window surface");
            destroySurface();
            return;
        }

        swapchainDirty = true;
    }

    void detachFromWindow() override
    {
        if (vulkanDevice == nullptr)
            return;

        const ScopedLock sl (vulkanDevice->getLock());

        vulkanDevice->waitIdle();

        destroySwapchain();
        destroySurface();
    }

    void onSizeChanged (void*, int newWidth, int newHeight, float, uint32_t) override
    {
        if (! isValid() || newWidth <= 0 || newHeight <= 0)
            return;

        const ScopedLock sl (vulkanDevice->getLock());

        width = newWidth;
        height = newHeight;

        canvas = getRenderContext()->makeRenderCanvas (static_cast<uint32_t> (width), static_cast<uint32_t> (height));
        swapchainDirty = true;

        // Rive creates its GPU scripting context on first use; do it here rather than wherever
        // a scripted file happens to be loaded
        getRenderContext()->ore();
    }

    bool setVsyncEnabled (bool shouldEnable) override
    {
        if (options.vsync != shouldEnable)
        {
            options.vsync = shouldEnable;
            swapchainDirty = true;
        }

        return true;
    }

    //==============================================================================

    void begin (const rive::gpu::RenderContext::FrameDescriptor& descriptor) override
    {
        frameDescriptor = descriptor;
        getRenderContext()->beginFrame (descriptor);
    }

    void end (void*) override
    {
        flushFrame (true);
    }

    void suspendFrame() override
    {
        flushFrame (false);
    }

private:
    //==============================================================================
    /** An acquire semaphore, reusable once the work that waited on it completed. */
    struct AcquireSemaphore
    {
        VkSemaphore semaphore = VK_NULL_HANDLE;
        VkCommandBuffer lastUse = VK_NULL_HANDLE;
    };

    void flushFrame (bool present)
    {
        if (! isValid() || canvas == nullptr)
            return;

        const ScopedLock sl (vulkanDevice->getLock());

        // The generation is taken at flush time, after any GpuFrame nested in the paint
        const auto commands = vulkanDevice->beginCommands();
        jassert (commands.isValid());
        if (! commands.isValid())
            return;

        auto* renderTarget = static_cast<rive::gpu::RenderTargetVulkan*> (canvas->renderTarget());

        getRenderContext()->flush ({ .renderTarget = renderTarget,
                                     .externalCommandBuffer = commands.commandBuffer,
                                     .currentFrameNumber = commands.frameNumber,
                                     .safeFrameNumber = commands.safeFrameNumber });

        if (! present || ! ensureSwapchain())
        {
            vulkanDevice->submitCommands (commands.commandBuffer);
            return;
        }

        const auto& fn = vulkanDevice->getFunctions();
        const auto device = vulkanDevice->getDevice();

        auto& acquire = acquireSemaphores[nextAcquireSemaphore];
        nextAcquireSemaphore = (nextAcquireSemaphore + 1) % acquireSemaphores.size();

        // The submission that last waited on this semaphore must have consumed it
        vulkanDevice->waitForCommands (acquire.lastUse);

        uint32_t imageIndex = 0;
        const auto acquired = fn.AcquireNextImageKHR (device, swapchain, UINT64_MAX, acquire.semaphore, VK_NULL_HANDLE, &imageIndex);

        // Android reports suboptimal while the display is rotated, recreating then would happen every frame
        if (acquired != VK_SUCCESS && acquired != VK_SUBOPTIMAL_KHR)
        {
            if (acquired == VK_ERROR_OUT_OF_DATE_KHR || acquired == VK_ERROR_SURFACE_LOST_KHR)
                swapchainDirty = true;

            vulkanDevice->submitCommands (commands.commandBuffer);
            return;
        }

        acquire.lastUse = commands.commandBuffer;

        recordBlit (commands.commandBuffer, *renderTarget, swapchainImages[imageIndex]);

        const auto renderFinished = renderFinishedSemaphores[imageIndex];
        if (! vulkanDevice->submitCommands (commands.commandBuffer, acquire.semaphore, VK_PIPELINE_STAGE_TRANSFER_BIT, renderFinished))
        {
            // The acquired image never gets presented and its semaphore stays signalled, start over
            swapchainDirty = true;
            return;
        }

        VkPresentInfoKHR presentInfo {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderFinished;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &imageIndex;

        const auto presented = fn.QueuePresentKHR (vulkanDevice->getQueue(), &presentInfo);
        if (presented == VK_ERROR_OUT_OF_DATE_KHR || presented == VK_ERROR_SURFACE_LOST_KHR)
            swapchainDirty = true;
    }

    void recordBlit (VkCommandBuffer commandBuffer, rive::gpu::RenderTargetVulkan& renderTarget, VkImage swapchainImage)
    {
        const auto& fn = vulkanDevice->getFunctions();

        const VkImage source = renderTarget.accessTargetImage (commandBuffer,
                                                               { VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL });

        VkImageMemoryBarrier toTransfer {};
        toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        toTransfer.srcAccessMask = 0;
        toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toTransfer.image = swapchainImage;
        toTransfer.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

        // The acquire semaphore is waited on at the transfer stage, which this barrier chains to
        fn.CmdPipelineBarrier (commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &toTransfer);

        VkImageBlit region {};
        region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        region.srcOffsets[1] = { width, height, 1 };
        region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        region.dstOffsets[1] = { static_cast<int32_t> (swapchainExtent.width), static_cast<int32_t> (swapchainExtent.height), 1 };

        fn.CmdBlitImage (commandBuffer,
                         source,
                         VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                         swapchainImage,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         1,
                         &region,
                         VK_FILTER_NEAREST);

        VkImageMemoryBarrier toPresent = toTransfer;
        toPresent.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        toPresent.dstAccessMask = 0;
        toPresent.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        fn.CmdPipelineBarrier (commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &toPresent);
    }

    //==============================================================================

    VkSurfaceFormatKHR chooseSurfaceFormat() const
    {
        const auto& fn = vulkanDevice->getFunctions();
        const auto physicalDevice = vulkanDevice->getPhysicalDevice();

        uint32_t count = 0;
        fn.GetPhysicalDeviceSurfaceFormatsKHR (physicalDevice, surface, &count, nullptr);
        std::vector<VkSurfaceFormatKHR> formats (count);
        fn.GetPhysicalDeviceSurfaceFormatsKHR (physicalDevice, surface, &count, formats.data());

        auto canBlitTo = [&] (VkFormat format)
        {
            VkFormatProperties properties {};
            fn.GetPhysicalDeviceFormatProperties (physicalDevice, format, &properties);
            return (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT) != 0;
        };

        for (const auto preferred : { VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM })
        {
            for (const auto& format : formats)
            {
                if (format.format == preferred && canBlitTo (format.format))
                    return format;
            }
        }

        for (const auto& format : formats)
        {
            if (canBlitTo (format.format))
                return format;
        }

        return formats.empty() ? VkSurfaceFormatKHR { VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR } : formats.front();
    }

    VkPresentModeKHR choosePresentMode() const
    {
        if (options.vsync)
            return VK_PRESENT_MODE_FIFO_KHR;

        const auto& fn = vulkanDevice->getFunctions();

        uint32_t count = 0;
        fn.GetPhysicalDeviceSurfacePresentModesKHR (vulkanDevice->getPhysicalDevice(), surface, &count, nullptr);
        std::vector<VkPresentModeKHR> modes (count);
        fn.GetPhysicalDeviceSurfacePresentModesKHR (vulkanDevice->getPhysicalDevice(), surface, &count, modes.data());

        for (const auto preferred : { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR })
        {
            if (std::find (modes.begin(), modes.end(), preferred) != modes.end())
                return preferred;
        }

        // The only mode every implementation supports
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    bool ensureSwapchain()
    {
        if (surface == VK_NULL_HANDLE)
            return false;

        if (swapchain != VK_NULL_HANDLE && ! swapchainDirty)
            return true;

        const auto& fn = vulkanDevice->getFunctions();
        const auto device = vulkanDevice->getDevice();

        VkSurfaceCapabilitiesKHR capabilities {};
        if (fn.GetPhysicalDeviceSurfaceCapabilitiesKHR (vulkanDevice->getPhysicalDevice(), surface, &capabilities) != VK_SUCCESS)
            return false;

        VkExtent2D extent = capabilities.currentExtent;
        if (extent.width == UINT32_MAX)
        {
            extent.width = jlimit (capabilities.minImageExtent.width, capabilities.maxImageExtent.width, static_cast<uint32_t> (width));
            extent.height = jlimit (capabilities.minImageExtent.height, capabilities.maxImageExtent.height, static_cast<uint32_t> (height));
        }

        // A minimized window has no area to present to
        if (extent.width == 0 || extent.height == 0)
            return false;

        if ((capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) == 0)
        {
            Logger::outputDebugString ("Vulkan: the surface does not accept transfers, nothing can be presented");
            return false;
        }

        const auto surfaceFormat = chooseSurfaceFormat();
        if (surfaceFormat.format == VK_FORMAT_UNDEFINED)
            return false;

        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0)
            imageCount = jmin (imageCount, capabilities.maxImageCount);

        // Identity lets the compositor rotate. Pre-rotating would need a rotated present pass,
        // a blit cannot turn the image by 90 degrees
        const auto preTransform = (capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) != 0
                                    ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
                                    : capabilities.currentTransform;

        VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        for (const auto candidate : { VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                                      VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
                                      VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                                      VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR })
        {
            if ((capabilities.supportedCompositeAlpha & candidate) != 0)
            {
                compositeAlpha = candidate;
                break;
            }
        }

        // The old images may still be read by work in flight
        vulkanDevice->waitIdle();

        const auto oldSwapchain = swapchain;

        VkSwapchainCreateInfoKHR swapchainInfo {};
        swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainInfo.surface = surface;
        swapchainInfo.minImageCount = imageCount;
        swapchainInfo.imageFormat = surfaceFormat.format;
        swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
        swapchainInfo.imageExtent = extent;
        swapchainInfo.imageArrayLayers = 1;
        swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainInfo.preTransform = preTransform;
        swapchainInfo.compositeAlpha = compositeAlpha;
        swapchainInfo.presentMode = choosePresentMode();
        swapchainInfo.clipped = VK_TRUE;
        swapchainInfo.oldSwapchain = oldSwapchain;

        VkSwapchainKHR newSwapchain = VK_NULL_HANDLE;
        const auto created = fn.CreateSwapchainKHR (device, &swapchainInfo, nullptr, &newSwapchain);

        destroySwapchain();

        if (created != VK_SUCCESS)
        {
            Logger::outputDebugString ("Vulkan: unable to create the swapchain (VkResult " + String (static_cast<int> (created)) + ")");
            return false;
        }

        swapchain = newSwapchain;
        swapchainExtent = extent;

        uint32_t count = 0;
        fn.GetSwapchainImagesKHR (device, swapchain, &count, nullptr);
        swapchainImages.resize (count);
        fn.GetSwapchainImagesKHR (device, swapchain, &count, swapchainImages.data());

        VkSemaphoreCreateInfo semaphoreInfo {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        // One render-finished semaphore per image, reused only once that image is acquired again
        renderFinishedSemaphores.resize (count, VK_NULL_HANDLE);
        for (auto& semaphore : renderFinishedSemaphores)
            fn.CreateSemaphore (device, &semaphoreInfo, nullptr, &semaphore);

        acquireSemaphores.resize (count + 1);
        for (auto& acquire : acquireSemaphores)
            fn.CreateSemaphore (device, &semaphoreInfo, nullptr, &acquire.semaphore);

        nextAcquireSemaphore = 0;
        swapchainDirty = false;
        return true;
    }

    void destroySwapchain()
    {
        const auto& fn = vulkanDevice->getFunctions();
        const auto device = vulkanDevice->getDevice();

        for (auto semaphore : renderFinishedSemaphores)
            fn.DestroySemaphore (device, semaphore, nullptr);

        for (const auto& acquire : acquireSemaphores)
            fn.DestroySemaphore (device, acquire.semaphore, nullptr);

        renderFinishedSemaphores.clear();
        acquireSemaphores.clear();
        swapchainImages.clear();

        if (swapchain != VK_NULL_HANDLE)
            fn.DestroySwapchainKHR (device, swapchain, nullptr);

        swapchain = VK_NULL_HANDLE;
        swapchainDirty = true;
    }

    void destroySurface()
    {
        if (surface == VK_NULL_HANDLE)
            return;

        if (options.vulkan.destroySurface)
            options.vulkan.destroySurface (vulkanDevice->getInstance(), reinterpret_cast<uint64> (surface));

        surface = VK_NULL_HANDLE;
    }

    //==============================================================================

    Options options;
    GpuDevice::Ptr gpuDevice;
    VulkanDevice* vulkanDevice = nullptr;

    rive::rcp<rive::gpu::RenderCanvas> canvas;
    int width = 0;
    int height = 0;

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkExtent2D swapchainExtent {};
    bool swapchainDirty = true;
    std::vector<VkImage> swapchainImages;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<AcquireSemaphore> acquireSemaphores;
    size_t nextAcquireSemaphore = 0;
};

//==============================================================================

std::unique_ptr<GraphicsContext> yup_constructVulkanGraphicsContext (GpuDevice::Options options, GpuDevice::Ptr existingGpu)
{
    auto context = std::make_unique<GraphicsContextVulkan> (std::move (options), std::move (existingGpu));
    if (! context->isValid())
        return nullptr;

    return context;
}

} // namespace yup

#endif // YUP_RIVE_USE_VULKAN
