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

    Rive renders into a persistent offscreen canvas, which a fullscreen draw copies to
    the acquired swapchain image: swapchain images do not keep their contents between
    frames, and the window repaints only its dirty regions on top of the previous
    frame. The draw also applies the surface pre-transform, so a rotated Android
    display needs no rotation pass in the compositor, and encodes for whatever format
    the surface offers, sRGB ones included.

    The surface comes from Options::vulkan, so nothing here touches a window system.
    The swapchain is created lazily and recreated when it goes out of date, when the
    display rotates, when the window is resized or when the vsync mode changes.
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
            destroyPresentObjects();
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
    /** The per-frame state of a presentation, reusable once the work that used it completed. */
    struct PresentSlot
    {
        VkSemaphore acquireSemaphore = VK_NULL_HANDLE;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        VkCommandBuffer lastUse = VK_NULL_HANDLE;
        uint64_t lastUseGeneration = 0;
    };

    /** Matches the push constant block of yup_PresentShader_vulkan.vert / .frag. */
    struct PresentParameters
    {
        float rotation[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
        int32_t decodeSrgb = 0;
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

        // Flushed even on a lost device, which closes the frame Rive opened
        getRenderContext()->flush ({ .renderTarget = renderTarget,
                                     .externalCommandBuffer = commands.commandBuffer,
                                     .currentFrameNumber = commands.frameNumber,
                                     .safeFrameNumber = commands.safeFrameNumber });

        if (! present || vulkanDevice->isDeviceLost() || ! ensureSwapchain())
        {
            vulkanDevice->submitCommands (commands.commandBuffer);
            return;
        }

        const auto& fn = vulkanDevice->getFunctions();
        const auto device = vulkanDevice->getDevice();

        auto& slot = presentSlots[nextPresentSlot];
        nextPresentSlot = (nextPresentSlot + 1) % presentSlots.size();

        // The submission that last used this slot must have consumed its semaphore and descriptor set
        vulkanDevice->waitForCommands (slot.lastUse, slot.lastUseGeneration);

        uint32_t imageIndex = 0;
        const auto acquired = vulkanDevice->checkResult (fn.AcquireNextImageKHR (device, swapchain, UINT64_MAX, slot.acquireSemaphore, VK_NULL_HANDLE, &imageIndex));

        if (acquired != VK_SUCCESS && acquired != VK_SUBOPTIMAL_KHR)
        {
            if (acquired == VK_ERROR_OUT_OF_DATE_KHR || acquired == VK_ERROR_SURFACE_LOST_KHR)
                swapchainDirty = true;

            vulkanDevice->submitCommands (commands.commandBuffer);
            return;
        }

        // Suboptimal is reported while the swapchain transform no longer matches the display.
        // Android reports it for as long as that lasts, so only a real rotation recreates
        if (acquired == VK_SUBOPTIMAL_KHR && currentSurfaceTransform() != swapchainTransform)
            swapchainDirty = true;

        slot.lastUse = commands.commandBuffer;
        slot.lastUseGeneration = commands.frameNumber;

        recordPresentPass (commands.commandBuffer, *renderTarget, slot, imageIndex);

        const auto renderFinished = renderFinishedSemaphores[imageIndex];
        if (! vulkanDevice->submitCommands (commands.commandBuffer, slot.acquireSemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, renderFinished))
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

        const auto presented = vulkanDevice->checkResult (fn.QueuePresentKHR (vulkanDevice->getQueue(), &presentInfo));
        if (presented == VK_ERROR_OUT_OF_DATE_KHR || presented == VK_ERROR_SURFACE_LOST_KHR)
            swapchainDirty = true;
    }

    void recordPresentPass (VkCommandBuffer commandBuffer, rive::gpu::RenderTargetVulkan& renderTarget, const PresentSlot& slot, uint32_t imageIndex)
    {
        const auto& fn = vulkanDevice->getFunctions();

        const VkImageView canvasView = renderTarget.accessTargetImageView (commandBuffer,
                                                                           { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });

        // The canvas view changes with the window size, so the set is written every frame
        VkDescriptorImageInfo imageInfo {};
        imageInfo.sampler = presentSampler;
        imageInfo.imageView = canvasView;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet write {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = slot.descriptorSet;
        write.dstBinding = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &imageInfo;

        fn.UpdateDescriptorSets (vulkanDevice->getDevice(), 1, &write, 0, nullptr);

        VkRenderPassBeginInfo passInfo {};
        passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        passInfo.renderPass = presentRenderPass;
        passInfo.framebuffer = swapchainFramebuffers[imageIndex];
        passInfo.renderArea.extent = swapchainExtent;

        fn.CmdBeginRenderPass (commandBuffer, &passInfo, VK_SUBPASS_CONTENTS_INLINE);
        fn.CmdBindPipeline (commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, presentPipeline);

        VkViewport viewport {};
        viewport.width = static_cast<float> (swapchainExtent.width);
        viewport.height = static_cast<float> (swapchainExtent.height);
        viewport.maxDepth = 1.0f;

        VkRect2D scissor {};
        scissor.extent = swapchainExtent;

        fn.CmdSetViewport (commandBuffer, 0, 1, &viewport);
        fn.CmdSetScissor (commandBuffer, 0, 1, &scissor);
        fn.CmdBindDescriptorSets (commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, presentPipelineLayout, 0, 1, &slot.descriptorSet, 0, nullptr);
        fn.CmdPushConstants (commandBuffer,
                             presentPipelineLayout,
                             VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                             0,
                             sizeof (PresentParameters),
                             &presentParameters);
        fn.CmdDraw (commandBuffer, 3, 1, 0, 0);
        fn.CmdEndRenderPass (commandBuffer);
    }

    //==============================================================================

    static bool isSrgbFormat (VkFormat format) noexcept
    {
        return format == VK_FORMAT_R8G8B8A8_SRGB
            || format == VK_FORMAT_B8G8R8A8_SRGB
            || format == VK_FORMAT_A8B8G8R8_SRGB_PACK32;
    }

    /** Rive renders sRGB-encoded 8-bit color: a UNORM format stores it as is, an sRGB one
        encodes on write, so the present shader decodes for it first. */
    VkSurfaceFormatKHR chooseSurfaceFormat() const
    {
        const auto& fn = vulkanDevice->getFunctions();
        const auto physicalDevice = vulkanDevice->getPhysicalDevice();

        uint32_t count = 0;
        fn.GetPhysicalDeviceSurfaceFormatsKHR (physicalDevice, surface, &count, nullptr);
        std::vector<VkSurfaceFormatKHR> formats (count);
        fn.GetPhysicalDeviceSurfaceFormatsKHR (physicalDevice, surface, &count, formats.data());

        for (const auto preferred : { VK_FORMAT_R8G8B8A8_UNORM,
                                      VK_FORMAT_B8G8R8A8_UNORM,
                                      VK_FORMAT_A2B10G10R10_UNORM_PACK32,
                                      VK_FORMAT_A2R10G10B10_UNORM_PACK32,
                                      VK_FORMAT_R8G8B8A8_SRGB,
                                      VK_FORMAT_B8G8R8A8_SRGB,
                                      VK_FORMAT_A8B8G8R8_SRGB_PACK32 })
        {
            for (const auto& format : formats)
            {
                if (format.format == preferred && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                    return format;
            }
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

    VkCompositeAlphaFlagBitsKHR chooseCompositeAlpha (VkCompositeAlphaFlagsKHR supported) const
    {
        // Rive renders premultiplied alpha, which is what a transparent window wants composited
        const auto candidates = options.vulkan.transparent
                                  ? std::array { VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                                                 VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
                                                 VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
                                                 VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR }
                                  : std::array { VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                                                 VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
                                                 VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                                                 VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR };

        for (const auto candidate : candidates)
        {
            if ((supported & candidate) != 0)
                return candidate;
        }

        return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    }

    VkSurfaceTransformFlagBitsKHR currentSurfaceTransform() const
    {
        VkSurfaceCapabilitiesKHR capabilities {};
        vulkanDevice->getFunctions().GetPhysicalDeviceSurfaceCapabilitiesKHR (vulkanDevice->getPhysicalDevice(), surface, &capabilities);
        return capabilities.currentTransform;
    }

    /** Rotation applied to clip space positions so the swapchain image, kept in the display's
        native orientation, shows the canvas the right way up (Android's pre-rotation). */
    static std::array<float, 4> rotationForTransform (VkSurfaceTransformFlagBitsKHR transform) noexcept
    {
        switch (transform)
        {
            case VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR:
                return { 0.0f, -1.0f, 1.0f, 0.0f };

            case VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR:
                return { -1.0f, 0.0f, 0.0f, -1.0f };

            case VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR:
                return { 0.0f, 1.0f, -1.0f, 0.0f };

            default:
                return { 1.0f, 0.0f, 0.0f, 1.0f };
        }
    }

    static bool isQuarterTurn (VkSurfaceTransformFlagBitsKHR transform) noexcept
    {
        return transform == VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR || transform == VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR;
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
        if (vulkanDevice->checkResult (fn.GetPhysicalDeviceSurfaceCapabilitiesKHR (vulkanDevice->getPhysicalDevice(), surface, &capabilities)) != VK_SUCCESS)
            return false;

        // Pre-rotate when the display is turned by a multiple of 90 degrees, any other
        // transform is left to the compositor
        auto transform = capabilities.currentTransform;
        const bool canPreRotate = transform == VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
                               || transform == VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR
                               || transform == VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR
                               || transform == VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR;

        if (! canPreRotate && (capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) != 0)
            transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

        // The current extent is the size as displayed; a pre-rotated swapchain keeps the
        // display's native orientation, which swaps it for a quarter turn
        VkExtent2D extent = capabilities.currentExtent;
        if (extent.width == UINT32_MAX)
        {
            extent.width = jlimit (capabilities.minImageExtent.width, capabilities.maxImageExtent.width, static_cast<uint32_t> (width));
            extent.height = jlimit (capabilities.minImageExtent.height, capabilities.maxImageExtent.height, static_cast<uint32_t> (height));
        }

        if (isQuarterTurn (transform))
            std::swap (extent.width, extent.height);

        // A minimized window has no area to present to
        if (extent.width == 0 || extent.height == 0)
            return false;

        const auto surfaceFormat = chooseSurfaceFormat();
        if (surfaceFormat.format == VK_FORMAT_UNDEFINED)
            return false;

        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0)
            imageCount = jmin (imageCount, capabilities.maxImageCount);

        // The old images may still be read by work in flight
        vulkanDevice->waitIdle();

        if (surfaceFormat.format != presentFormat)
        {
            destroyPresentPipeline();

            if (! createPresentPipeline (surfaceFormat.format))
                return false;
        }

        const auto oldSwapchain = swapchain;

        VkSwapchainCreateInfoKHR swapchainInfo {};
        swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainInfo.surface = surface;
        swapchainInfo.minImageCount = imageCount;
        swapchainInfo.imageFormat = surfaceFormat.format;
        swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
        swapchainInfo.imageExtent = extent;
        swapchainInfo.imageArrayLayers = 1;
        swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainInfo.preTransform = transform;
        swapchainInfo.compositeAlpha = chooseCompositeAlpha (capabilities.supportedCompositeAlpha);
        swapchainInfo.presentMode = choosePresentMode();
        swapchainInfo.clipped = VK_TRUE;
        swapchainInfo.oldSwapchain = oldSwapchain;

        VkSwapchainKHR newSwapchain = VK_NULL_HANDLE;
        const auto created = vulkanDevice->checkResult (fn.CreateSwapchainKHR (device, &swapchainInfo, nullptr, &newSwapchain));

        destroySwapchain();

        if (created != VK_SUCCESS)
        {
            Logger::outputDebugString ("Vulkan: unable to create the swapchain (VkResult " + String (static_cast<int> (created)) + ")");
            return false;
        }

        swapchain = newSwapchain;
        swapchainExtent = extent;
        swapchainTransform = transform;

        const auto rotation = rotationForTransform (transform);
        std::copy (rotation.begin(), rotation.end(), presentParameters.rotation);
        presentParameters.decodeSrgb = isSrgbFormat (surfaceFormat.format) ? 1 : 0;

        return createSwapchainResources (surfaceFormat.format);
    }

    bool createSwapchainResources (VkFormat format)
    {
        const auto& fn = vulkanDevice->getFunctions();
        const auto device = vulkanDevice->getDevice();

        uint32_t count = 0;
        fn.GetSwapchainImagesKHR (device, swapchain, &count, nullptr);
        swapchainImages.resize (count);
        fn.GetSwapchainImagesKHR (device, swapchain, &count, swapchainImages.data());

        for (auto image : swapchainImages)
        {
            VkImageViewCreateInfo viewInfo {};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = format;
            viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

            VkImageView view = VK_NULL_HANDLE;
            if (fn.CreateImageView (device, &viewInfo, nullptr, &view) != VK_SUCCESS)
                return false;

            swapchainImageViews.push_back (view);

            VkFramebufferCreateInfo framebufferInfo {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = presentRenderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = &view;
            framebufferInfo.width = swapchainExtent.width;
            framebufferInfo.height = swapchainExtent.height;
            framebufferInfo.layers = 1;

            VkFramebuffer framebuffer = VK_NULL_HANDLE;
            if (fn.CreateFramebuffer (device, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS)
                return false;

            swapchainFramebuffers.push_back (framebuffer);
        }

        VkSemaphoreCreateInfo semaphoreInfo {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        // One render-finished semaphore per image, reused only once that image is acquired again
        renderFinishedSemaphores.resize (count, VK_NULL_HANDLE);
        for (auto& semaphore : renderFinishedSemaphores)
            fn.CreateSemaphore (device, &semaphoreInfo, nullptr, &semaphore);

        // One more slot than images, so acquiring never waits on the frame just submitted
        const auto slotCount = count + 1;

        VkDescriptorPoolSize poolSize { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, slotCount };

        VkDescriptorPoolCreateInfo poolInfo {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.maxSets = slotCount;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;

        if (fn.CreateDescriptorPool (device, &poolInfo, nullptr, &presentDescriptorPool) != VK_SUCCESS)
            return false;

        presentSlots.resize (slotCount);
        for (auto& slot : presentSlots)
        {
            fn.CreateSemaphore (device, &semaphoreInfo, nullptr, &slot.acquireSemaphore);

            VkDescriptorSetAllocateInfo allocateInfo {};
            allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocateInfo.descriptorPool = presentDescriptorPool;
            allocateInfo.descriptorSetCount = 1;
            allocateInfo.pSetLayouts = &presentSetLayout;

            if (fn.AllocateDescriptorSets (device, &allocateInfo, &slot.descriptorSet) != VK_SUCCESS)
                return false;
        }

        nextPresentSlot = 0;
        swapchainDirty = false;
        return true;
    }

    void destroySwapchain()
    {
        const auto& fn = vulkanDevice->getFunctions();
        const auto device = vulkanDevice->getDevice();

        for (auto semaphore : renderFinishedSemaphores)
            fn.DestroySemaphore (device, semaphore, nullptr);

        for (const auto& slot : presentSlots)
            fn.DestroySemaphore (device, slot.acquireSemaphore, nullptr);

        // Destroying the pool frees its sets
        if (presentDescriptorPool != VK_NULL_HANDLE)
            fn.DestroyDescriptorPool (device, presentDescriptorPool, nullptr);

        for (auto framebuffer : swapchainFramebuffers)
            fn.DestroyFramebuffer (device, framebuffer, nullptr);

        for (auto view : swapchainImageViews)
            fn.DestroyImageView (device, view, nullptr);

        presentDescriptorPool = VK_NULL_HANDLE;
        renderFinishedSemaphores.clear();
        presentSlots.clear();
        swapchainFramebuffers.clear();
        swapchainImageViews.clear();
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

    /** Builds the render pass and pipeline drawing the canvas into a swapchain image of
        @p format, plus the format-independent objects on first use. */
    bool createPresentPipeline (VkFormat format)
    {
        const auto& fn = vulkanDevice->getFunctions();
        const auto device = vulkanDevice->getDevice();

        if (presentSampler == VK_NULL_HANDLE)
        {
            VkSamplerCreateInfo samplerInfo {};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

            if (fn.CreateSampler (device, &samplerInfo, nullptr, &presentSampler) != VK_SUCCESS)
                return false;

            VkDescriptorSetLayoutBinding binding {};
            binding.binding = 0;
            binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            binding.descriptorCount = 1;
            binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            VkDescriptorSetLayoutCreateInfo setLayoutInfo {};
            setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            setLayoutInfo.bindingCount = 1;
            setLayoutInfo.pBindings = &binding;

            if (fn.CreateDescriptorSetLayout (device, &setLayoutInfo, nullptr, &presentSetLayout) != VK_SUCCESS)
                return false;

            VkPushConstantRange pushConstants {};
            pushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            pushConstants.size = sizeof (PresentParameters);

            VkPipelineLayoutCreateInfo layoutInfo {};
            layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            layoutInfo.setLayoutCount = 1;
            layoutInfo.pSetLayouts = &presentSetLayout;
            layoutInfo.pushConstantRangeCount = 1;
            layoutInfo.pPushConstantRanges = &pushConstants;

            if (fn.CreatePipelineLayout (device, &layoutInfo, nullptr, &presentPipelineLayout) != VK_SUCCESS)
                return false;
        }

        // Every pixel is overwritten, and the image goes straight to the presentation engine
        VkAttachmentDescription attachment {};
        attachment.format = format;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorReference { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        VkSubpassDescription subpass {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorReference;

        // Chains the layout transition to the acquire semaphore, waited on at this stage
        VkSubpassDependency dependency {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &attachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (fn.CreateRenderPass (device, &renderPassInfo, nullptr, &presentRenderPass) != VK_SUCCESS)
            return false;

        auto makeModule = [&] (const uint32_t* code, size_t byteSize)
        {
            VkShaderModuleCreateInfo moduleInfo {};
            moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            moduleInfo.codeSize = byteSize;
            moduleInfo.pCode = code;

            VkShaderModule module = VK_NULL_HANDLE;
            fn.CreateShaderModule (device, &moduleInfo, nullptr, &module);
            return module;
        };

        const auto vertexModule = makeModule (yup_PresentShader_vulkan_vert, sizeof (yup_PresentShader_vulkan_vert));
        const auto fragmentModule = makeModule (yup_PresentShader_vulkan_frag, sizeof (yup_PresentShader_vulkan_frag));

        VkPipelineShaderStageCreateInfo stages[2] {};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertexModule;
        stages[0].pName = "main";
        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fragmentModule;
        stages[1].pName = "main";

        VkPipelineVertexInputStateCreateInfo vertexInput {};
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo viewportState {};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterization {};
        rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization.polygonMode = VK_POLYGON_MODE_FILL;
        rasterization.cullMode = VK_CULL_MODE_NONE;
        rasterization.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo multisample {};
        multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState blendAttachment {};
        blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo blend {};
        blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blend.attachmentCount = 1;
        blend.pAttachments = &blendAttachment;

        const VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

        VkPipelineDynamicStateCreateInfo dynamicState {};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates;

        VkGraphicsPipelineCreateInfo pipelineInfo {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = stages;
        pipelineInfo.pVertexInputState = &vertexInput;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterization;
        pipelineInfo.pMultisampleState = &multisample;
        pipelineInfo.pColorBlendState = &blend;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = presentPipelineLayout;
        pipelineInfo.renderPass = presentRenderPass;

        const auto created = (vertexModule != VK_NULL_HANDLE && fragmentModule != VK_NULL_HANDLE)
                               ? fn.CreateGraphicsPipelines (device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &presentPipeline)
                               : VK_ERROR_INITIALIZATION_FAILED;

        fn.DestroyShaderModule (device, vertexModule, nullptr);
        fn.DestroyShaderModule (device, fragmentModule, nullptr);

        if (created != VK_SUCCESS)
        {
            Logger::outputDebugString ("Vulkan: unable to create the present pipeline");
            return false;
        }

        presentFormat = format;
        return true;
    }

    void destroyPresentPipeline()
    {
        const auto& fn = vulkanDevice->getFunctions();
        const auto device = vulkanDevice->getDevice();

        if (presentPipeline != VK_NULL_HANDLE)
            fn.DestroyPipeline (device, presentPipeline, nullptr);

        if (presentRenderPass != VK_NULL_HANDLE)
            fn.DestroyRenderPass (device, presentRenderPass, nullptr);

        presentPipeline = VK_NULL_HANDLE;
        presentRenderPass = VK_NULL_HANDLE;
        presentFormat = VK_FORMAT_UNDEFINED;
    }

    void destroyPresentObjects()
    {
        const auto& fn = vulkanDevice->getFunctions();
        const auto device = vulkanDevice->getDevice();

        destroyPresentPipeline();

        if (presentPipelineLayout != VK_NULL_HANDLE)
            fn.DestroyPipelineLayout (device, presentPipelineLayout, nullptr);

        if (presentSetLayout != VK_NULL_HANDLE)
            fn.DestroyDescriptorSetLayout (device, presentSetLayout, nullptr);

        if (presentSampler != VK_NULL_HANDLE)
            fn.DestroySampler (device, presentSampler, nullptr);

        presentPipelineLayout = VK_NULL_HANDLE;
        presentSetLayout = VK_NULL_HANDLE;
        presentSampler = VK_NULL_HANDLE;
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
    VkSurfaceTransformFlagBitsKHR swapchainTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    bool swapchainDirty = true;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> swapchainFramebuffers;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<PresentSlot> presentSlots;
    size_t nextPresentSlot = 0;

    VkFormat presentFormat = VK_FORMAT_UNDEFINED;
    VkSampler presentSampler = VK_NULL_HANDLE;
    VkDescriptorSetLayout presentSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout presentPipelineLayout = VK_NULL_HANDLE;
    VkRenderPass presentRenderPass = VK_NULL_HANDLE;
    VkPipeline presentPipeline = VK_NULL_HANDLE;
    VkDescriptorPool presentDescriptorPool = VK_NULL_HANDLE;
    PresentParameters presentParameters;
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
