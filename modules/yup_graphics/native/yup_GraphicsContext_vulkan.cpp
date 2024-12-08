/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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
#include "yup_BootstrapVulkan.hpp"

#include "rive/renderer/rive_renderer.hpp"
#include "rive/renderer/vulkan/render_context_vulkan_impl.hpp"
#include "rive/renderer/vulkan/vkutil_resource_pool.hpp"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h>
#include <vk_mem_alloc.h>

namespace yup {

using namespace rive;
using namespace rive::gpu;

class LowLevelRenderContextVulkan : public GraphicsContext
{
public:
    LowLevelRenderContextVulkan (Options options)
        : m_options (options)
    {
        rive_vkb::load_vulkan();

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions (&glfwExtensionCount);

        m_instance =
            VKB_CHECK(vkb::InstanceBuilder()
                          .set_app_name ("path_fiddle")
                          .set_engine_name ("Rive Renderer")
#ifdef DEBUG
                          .set_debug_callback (rive_vkb::default_debug_callback)
                          .enable_validation_layers (true) // (m_options.enableVulkanValidationLayers)
#endif
                          .enable_extensions (glfwExtensionCount, glfwExtensions)
                          .build());

        m_instanceTable = m_instance.make_table();

        VulkanFeatures vulkanFeatures;

        std::tie (m_physicalDevice, vulkanFeatures) =
            rive_vkb::select_physical_device(
                vkb::PhysicalDeviceSelector (m_instance).defer_surface_initialization(),
                /*m_options.coreFeaturesOnly*/ false ? rive_vkb::FeatureSet::coreOnly
                                                     : rive_vkb::FeatureSet::allAvailable,
                /*m_options.gpuNameFilter*/ nullptr);

        m_device = VKB_CHECK (vkb::DeviceBuilder (m_physicalDevice).build());

        m_vkbTable = m_device.make_table();

        m_queue = VKB_CHECK (m_device.get_queue (vkb::QueueType::graphics));

        m_renderContext = RenderContextVulkanImpl::MakeContext(
            m_instance,
            m_physicalDevice,
            m_device,
            vulkanFeatures,
            m_instance.fp_vkGetInstanceProcAddr,
            m_instance.fp_vkGetDeviceProcAddr);

        m_commandBufferPool =
            make_rcp<vkutil::ResourcePool<vkutil::CommandBuffer>>(
                ref_rcp(vk()),
                *m_device.get_queue_index(vkb::QueueType::graphics));

        m_semaphorePool =
            make_rcp<vkutil::ResourcePool<vkutil::Semaphore>>(ref_rcp(vk()));

        m_fencePool =
            make_rcp<vkutil::ResourcePool<vkutil::Fence>>(ref_rcp(vk()));
    }

    ~LowLevelRenderContextVulkan()
    {
        // Destroy these before destroying the VkDevice.
        m_renderContext.reset();
        m_renderTarget.reset();
        m_pixelReadBuffer.reset();
        m_swapchainImageViews.clear();
        m_fencePool.reset();
        m_frameFence.reset();

        VK_CHECK(m_vkbTable.queueWaitIdle(m_queue));

        m_swapchainSemaphore = nullptr;
        m_frameFence = nullptr;
        m_frameCommandBuffer = nullptr;

        m_commandBufferPool = nullptr;
        m_semaphorePool = nullptr;
        m_fencePool = nullptr;

        if (m_swapchain != VK_NULL_HANDLE)
            vkb::destroy_swapchain(m_swapchain);

        if (m_windowSurface != VK_NULL_HANDLE)
            m_instanceTable.destroySurfaceKHR(m_windowSurface, nullptr);

        vkb::destroy_device(m_device);
        vkb::destroy_instance(m_instance);
    }

    float dpiScale(void* window) const override
    {
#ifdef __APPLE__
        return 2;
#else
        return 1;
#endif
    }

    rive::Factory* factory() override { return m_renderContext.get(); }

    rive::gpu::RenderContext* plsContextOrNull() override
    {
        return m_renderContext.get();
    }

    rive::gpu::RenderTarget* plsRenderTargetOrNull() override
    {
        return m_renderTarget.get();
    }

    void onSizeChanged(void* window, int width, int height, uint32_t sampleCount) override
    {
        VK_CHECK(m_vkbTable.queueWaitIdle(m_queue));

        if (m_swapchain != VK_NULL_HANDLE)
        {
            vkb::destroy_swapchain(m_swapchain);
        }

        if (m_windowSurface != VK_NULL_HANDLE)
        {
            m_instanceTable.destroySurfaceKHR(m_windowSurface, nullptr);
        }

        VK_CHECK(glfwCreateWindowSurface(m_instance,
                                         (GLFWwindow*)window,
                                         nullptr,
                                         &m_windowSurface));

        VkSurfaceCapabilitiesKHR windowCapabilities;
        VK_CHECK(m_instanceTable.fp_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            m_physicalDevice,
            m_windowSurface,
            &windowCapabilities));

        vkb::SwapchainBuilder swapchainBuilder(m_device, m_windowSurface);
        swapchainBuilder
            .set_desired_format({
                // Swap the target format in "vkcore" mode, just for fun so we
                // test both
                // configurations.
                .format = /*m_options.coreFeaturesOnly*/ false ? VK_FORMAT_B8G8R8A8_UNORM
                                                     : VK_FORMAT_R8G8B8A8_UNORM,
                .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
            })
            .add_fallback_format({
                .format = /*m_options.coreFeaturesOnly*/ false ? VK_FORMAT_R8G8B8A8_UNORM
                                                     : VK_FORMAT_B8G8R8A8_UNORM,
                .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
            })
            .set_desired_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR)
            .add_fallback_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
            .add_fallback_present_mode(VK_PRESENT_MODE_FIFO_RELAXED_KHR)
            .add_fallback_present_mode(VK_PRESENT_MODE_FIFO_KHR);
        if (! /*m_options.coreFeaturesOnly*/ false &&
            (windowCapabilities.supportedUsageFlags &
             VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT))
        {
            swapchainBuilder.add_image_usage_flags(
                VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
            if (m_options.enableReadPixels)
            {
                swapchainBuilder.add_image_usage_flags(
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
            }
        }
        else
        {
            swapchainBuilder
                .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
                .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        }
        m_swapchain = VKB_CHECK(swapchainBuilder.build());
        m_swapchainImages = *m_swapchain.get_images();

        m_swapchainImageViews.clear();
        m_swapchainImageViews.reserve(m_swapchainImages.size());
        for (VkImage image : m_swapchainImages)
        {
            m_swapchainImageViews.push_back(vk()->makeExternalTextureView(
                m_swapchain.image_usage_flags,
                {
                    .image = image,
                    .viewType = VK_IMAGE_VIEW_TYPE_2D,
                    .format = m_swapchain.image_format,
                    .subresourceRange =
                        {
                            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                            .levelCount = 1,
                            .layerCount = 1,
                        },
                }));
        }

        m_renderTarget =
            impl()->makeRenderTarget(width, height, m_swapchain.image_format);

        m_pixelReadBuffer = nullptr;
    }

    std::unique_ptr<rive::Renderer> makeRenderer (int width, int height) override
    {
        return std::make_unique<rive::RiveRenderer>(m_renderContext.get());
    }

    void begin (const rive::gpu::RenderContext::FrameDescriptor& frameDescriptor) override
    {
        m_swapchainSemaphore = m_semaphorePool->make();
        m_vkbTable.acquireNextImageKHR(m_swapchain,
                                       UINT64_MAX,
                                       *m_swapchainSemaphore,
                                       VK_NULL_HANDLE,
                                       &m_swapchainImageIndex);

        m_renderContext->beginFrame(std::move(frameDescriptor));

        m_frameCommandBuffer = m_commandBufferPool->make();

        VkCommandBufferBeginInfo commandBufferBeginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        };
        m_vkbTable.beginCommandBuffer(*m_frameCommandBuffer,
                                      &commandBufferBeginInfo);

        m_renderTarget->setTargetTextureView(
            m_swapchainImageViews[m_swapchainImageIndex],
            {});

        m_frameFence = m_fencePool->make();
    }

    void end (void* window) final
    {
        m_renderContext->flush({
            .renderTarget = m_renderTarget.get(),
            .externalCommandBuffer = *m_frameCommandBuffer,
            .frameCompletionFence = m_frameFence.get(),
        });

        uint32_t w = m_renderTarget->width();
        uint32_t h = m_renderTarget->height();

        m_renderTarget->setTargetLastAccess(vk()->simpleImageMemoryBarrier(
            *m_frameCommandBuffer,
            m_renderTarget->targetLastAccess(),
            {
                .pipelineStages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                .accessMask = VK_ACCESS_NONE,
                .layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            },
            m_swapchainImages[m_swapchainImageIndex]));

        VK_CHECK(m_vkbTable.endCommandBuffer(*m_frameCommandBuffer));

        auto flushSemaphore = m_semaphorePool->make();
        VkPipelineStageFlags waitDstStageMask =
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

        VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = m_swapchainSemaphore->vkSemaphoreAddressOf(),
            .pWaitDstStageMask = &waitDstStageMask,
            .commandBufferCount = 1,
            .pCommandBuffers = m_frameCommandBuffer->vkCommandBufferAddressOf(),
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = flushSemaphore->vkSemaphoreAddressOf(),
        };

        VK_CHECK(
            m_vkbTable.queueSubmit(m_queue, 1, &submitInfo, *m_frameFence));

        VkPresentInfoKHR presentInfo = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = flushSemaphore->vkSemaphoreAddressOf(),
            .swapchainCount = 1,
            .pSwapchains = &m_swapchain.swapchain,
            .pImageIndices = &m_swapchainImageIndex,
        };

        m_vkbTable.queuePresentKHR(m_queue, &presentInfo);

        m_swapchainSemaphore = nullptr;
        m_frameFence = nullptr;
        m_frameCommandBuffer = nullptr;
    }

private:
    RenderContextVulkanImpl* impl() const
    {
        return m_renderContext->static_impl_cast<RenderContextVulkanImpl>();
    }

    VulkanContext* vk() const { return impl()->vulkanContext(); }

    const Options m_options;
    vkb::Instance m_instance;
    vkb::InstanceDispatchTable m_instanceTable;
    vkb::PhysicalDevice m_physicalDevice;
    vkb::Device m_device;
    vkb::DispatchTable m_vkbTable;
    VkQueue m_queue;

    VkSurfaceKHR m_windowSurface = VK_NULL_HANDLE;
    vkb::Swapchain m_swapchain;
    std::vector<VkImage> m_swapchainImages;
    std::vector<rcp<vkutil::TextureView>> m_swapchainImageViews;
    uint32_t m_swapchainImageIndex = 0;

    rcp<vkutil::ResourcePool<vkutil::CommandBuffer>> m_commandBufferPool;
    rcp<vkutil::CommandBuffer> m_frameCommandBuffer;

    rcp<vkutil::ResourcePool<vkutil::Semaphore>> m_semaphorePool;
    rcp<vkutil::Semaphore> m_swapchainSemaphore;

    rcp<vkutil::ResourcePool<vkutil::Fence>> m_fencePool;
    rcp<vkutil::Fence> m_frameFence;

    std::unique_ptr<RenderContext> m_renderContext;
    rcp<RenderTargetVulkan> m_renderTarget;
    rcp<vkutil::Buffer> m_pixelReadBuffer;
};

std::unique_ptr<GraphicsContext> juce_constructVulkanGraphicsContext (GraphicsContext::Options options)
{
    return std::make_unique<LowLevelRenderContextVulkan> (options);
}

#endif

} // namespace yup
