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

#include "ore/vulkan/ore_sampler_vulkan.hpp"
#include "ore/vulkan/ore_texture_vulkan.hpp"

namespace yup
{

//==============================================================================
/** Records dispatches into a command buffer of the device ring, submitted by finish().

    A pass begun on a device owns a ring command buffer, submitted by finish(): like
    on Metal, the queue orders it before any frame submitted later. A pass begun on a
    GpuFrame records into the frame's command buffer instead, and is submitted with it.
    Everything its commands reference (pipelines, storage buffers, uniform buffers,
    texture views, samplers, descriptor pools) is kept alive until the GPU completes them.

    Textures move through the image layouts the ore context tracks: each dispatch takes
    the images it accesses from the tracked layout into the one it needs, and leaves
    the ones it wrote SHADER_READ_ONLY_OPTIMAL for later passes, reporting that back to
    ore. Storage textures need a pass begun on a GpuFrame, so that ore and the pass
    record into the same command buffer.
*/
class GpuComputePassImplVulkan final : public GpuComputePass::Impl
{
public:
    explicit GpuComputePassImplVulkan (GpuDeviceVulkan& deviceToUse)
        : device (deviceToUse)
        , vulkanDevice (deviceToUse.getVulkanDevice())
        , oreCtx (deviceToUse.getGpuContext())
    {
    }

    /** Records into a frame's command buffer, which the frame submits. */
    GpuComputePassImplVulkan (GpuDeviceVulkan& deviceToUse, VkCommandBuffer frameCommandBuffer, uint64_t frameGeneration)
        : GpuComputePassImplVulkan (deviceToUse)
    {
        commands.commandBuffer = frameCommandBuffer;
        commands.frameNumber = frameGeneration;
        joinsFrame = true;
    }

    ~GpuComputePassImplVulkan() override
    {
        if (! finished)
            finish();
    }

    //==========================================================================

    bool isValid() const override { return true; }

    //==========================================================================

    bool dispatch (uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ) override
    {
        auto* pipe = dynamic_cast<GpuComputePipelineVulkan*> (pipelineRef.get());
        if (pipe == nullptr || pipe->pipeline == VK_NULL_HANDLE)
            return false;

        // A standalone pass submits its own command buffer, which would need a semaphore to
        // take over the image layouts ore tracks for the frame
        const bool bindsStorageTexture = std::any_of (storageTexBindings.begin(), storageTexBindings.end(), [] (const auto& binding)
        {
            return binding.texture != nullptr;
        });

        jassert (joinsFrame || ! bindsStorageTexture);
        if (bindsStorageTexture && ! joinsFrame)
            return false;

        // Rive images and canvases are only wrapped as ore textures by an open frame
        if (! joinsFrame)
        {
            for (const auto& binding : texBindings)
            {
                if (binding.texture != nullptr && binding.texture->oreTexture == nullptr)
                {
                    jassertfalse; // Sample it on a pass begun on a GpuFrame
                    return false;
                }
            }
        }

        bool dispatched = false;
        device.runOnGraphicsContext ([&]
                                     {
            dispatched = record (*pipe, groupsX, groupsY, groupsZ);
        });

        return dispatched;
    }

    //==========================================================================

    void finish() override
    {
        device.runOnGraphicsContext ([this]
                                     {
            submit();
        });

        // The activator drops the work once its window is gone, but the command buffer must still
        // be submitted, or the device would count it as in flight forever
        if (commands.isValid())
        {
            const ScopedLock sl (vulkanDevice.getLock());
            submit();
        }
    }

private:
    //==========================================================================

    void submit()
    {
        if (! commands.isValid())
            return;

        // A frame submitted before its pass finished already carries the dispatches
        if (joinsFrame && ! vulkanDevice.isRecording (commands.commandBuffer, commands.frameNumber))
        {
            releaseWhenComplete();
            return;
        }

        const auto& fn = vulkanDevice.getFunctions();

        // Make the results visible to whatever reads them next: draws, copies or the host
        VkMemoryBarrier barrier {};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT
                              | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_HOST_READ_BIT;

        fn.CmdPipelineBarrier (commands.commandBuffer,
                               VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                               VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_HOST_BIT,
                               0,
                               1,
                               &barrier,
                               0,
                               nullptr,
                               0,
                               nullptr);

        // A frame's command buffer is submitted by the frame
        if (! joinsFrame)
            vulkanDevice.submitCommands (commands.commandBuffer);

        releaseWhenComplete();
    }

    void releaseWhenComplete()
    {
        vulkanDevice.runWhenComplete (commands.commandBuffer,
                                      [&vk = vulkanDevice,
                                       pools = std::move (descriptorPools),
                                       referenced = std::move (referencedResources)]
                                      {
            // The pipelines, buffers, views and samplers are released along with this handler
            (void) referenced;

            for (auto pool : pools)
                vk.getFunctions().DestroyDescriptorPool (vk.getDevice(), pool, nullptr);
        });

        descriptorPools.clear();
        referencedResources = {};
        commands = {};
    }

    /** What a pass must keep alive until the GPU finished its commands. */
    struct ReferencedResources
    {
        std::vector<GpuComputePipeline::Ptr> pipelines;
        std::vector<rive::rcp<rive::gpu::vkutil::Buffer>> storageBuffers;
        std::vector<rive::rcp<rive::gpu::vkutil::Buffer>> uniformBuffers;
        std::vector<rive::rcp<rive::ore::TextureView>> views;
        std::vector<rive::rcp<rive::ore::Sampler>> samplers;
    };

    /** An ore image a dispatch accesses, and the layout the dispatch needs it in. */
    struct ImageUse
    {
        rive::ore::TextureVulkan* texture = nullptr;
        VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    };

    /** The images a dispatch accesses, and the storage views it writes. */
    struct DispatchImages
    {
        std::vector<ImageUse> uses;
        std::vector<rive::ore::TextureViewVulkan*> storageViews;
    };

    bool record (GpuComputePipelineVulkan& pipe, uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ)
    {
        const auto& fn = vulkanDevice.getFunctions();

        VkMemoryBarrier barrier {};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

        VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

        if (! commands.isValid())
        {
            commands = vulkanDevice.beginCommands();
            if (! commands.isValid())
                return false;
        }
        else if (joinsFrame && ! vulkanDevice.isRecording (commands.commandBuffer, commands.frameNumber))
        {
            jassertfalse; // The frame this pass records into was already submitted
            return false;
        }

        if (std::exchange (firstDispatch, false))
        {
            // Earlier work may have written the buffers this pass reads; host writes are
            // made visible by the submission itself
            srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        }
        else
        {
            barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        }

        fn.CmdPipelineBarrier (commands.commandBuffer, srcStage, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);

        DispatchImages images;
        auto descriptorSets = makeDescriptorSets (pipe, images);
        if (! descriptorSets.has_value())
            return false;

        transitionImages (images.uses);

        for (auto* view : images.storageViews)
        {
            auto* texture = static_cast<rive::ore::TextureVulkan*> (view->texture());
            for (uint32_t layer = 0; layer < view->layerCount(); ++layer)
                texture->vkMarkWrittenExternally (view->baseMipLevel(), view->baseLayer() + layer);
        }

        fn.CmdBindPipeline (commands.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipe.pipeline);

        if (! descriptorSets->empty())
        {
            fn.CmdBindDescriptorSets (commands.commandBuffer,
                                      VK_PIPELINE_BIND_POINT_COMPUTE,
                                      pipe.pipelineLayout,
                                      0,
                                      static_cast<uint32_t> (descriptorSets->size()),
                                      descriptorSets->data(),
                                      0,
                                      nullptr);
        }

        fn.CmdDispatch (commands.commandBuffer, groupsX, groupsY, groupsZ);

        restoreSampledLayouts (images.uses);

        referencedResources.pipelines.push_back (pipelineRef);
        return true;
    }

    static VkImageAspectFlags aspectOf (rive::ore::TextureFormat format)
    {
        switch (format)
        {
            case rive::ore::TextureFormat::depth16unorm:
            case rive::ore::TextureFormat::depth32float:
                return VK_IMAGE_ASPECT_DEPTH_BIT;

            case rive::ore::TextureFormat::depth24plusStencil8:
            case rive::ore::TextureFormat::depth32floatStencil8:
                return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

            default:
                return VK_IMAGE_ASPECT_COLOR_BIT;
        }
    }

    static VkImageMemoryBarrier makeImageBarrier (const rive::ore::TextureVulkan& texture,
                                                  VkImageLayout oldLayout,
                                                  VkImageLayout newLayout,
                                                  VkImageSubresourceRange range)
    {
        VkImageMemoryBarrier barrier {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = texture.vkImage();
        barrier.subresourceRange = range;
        return barrier;
    }

    /** Moves each image from the layout ore tracks into the one the dispatch needs.

        Ore tracks one layout per texture, which only the subresources something wrote
        are known to be in: the others have no contents to keep and leave UNDEFINED.
    */
    void transitionImages (const std::vector<ImageUse>& uses)
    {
        std::vector<VkImageMemoryBarrier> barriers;
        VkPipelineStageFlags srcStages = 0;

        for (const auto& use : uses)
        {
            auto& texture = *use.texture;
            const auto tracked = texture.vkLayout();
            if (tracked == use.layout)
                continue;

            const auto aspect = aspectOf (texture.format());

            for (uint32_t layer = 0; layer < texture.arrayLayers(); ++layer)
            {
                for (uint32_t mip = 0; mip < texture.numMipmaps(); ++mip)
                {
                    const auto oldLayout = texture.vkWasWritten (mip, layer) ? tracked : VK_IMAGE_LAYOUT_UNDEFINED;

                    auto barrier = makeImageBarrier (texture, oldLayout, use.layout, { aspect, mip, 1, layer, 1 });
                    barrier.srcAccessMask = oldLayout == VK_IMAGE_LAYOUT_UNDEFINED ? 0 : VK_ACCESS_MEMORY_WRITE_BIT;
                    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                    barriers.push_back (barrier);
                }
            }

            srcStages |= tracked == VK_IMAGE_LAYOUT_UNDEFINED ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            texture.vkSetLayout (use.layout);
        }

        if (barriers.empty())
            return;

        vulkanDevice.getFunctions().CmdPipelineBarrier (commands.commandBuffer,
                                                        srcStages,
                                                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                                        0,
                                                        0,
                                                        nullptr,
                                                        0,
                                                        nullptr,
                                                        static_cast<uint32_t> (barriers.size()),
                                                        barriers.data());
    }

    /** Leaves every image the dispatch wrote ready to be sampled, which ore expects of a texture. */
    void restoreSampledLayouts (const std::vector<ImageUse>& uses)
    {
        std::vector<VkImageMemoryBarrier> barriers;

        for (const auto& use : uses)
        {
            if (use.layout != VK_IMAGE_LAYOUT_GENERAL)
                continue;

            auto& texture = *use.texture;

            auto barrier = makeImageBarrier (texture,
                                             VK_IMAGE_LAYOUT_GENERAL,
                                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                             { aspectOf (texture.format()), 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS });
            barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barriers.push_back (barrier);

            texture.vkSetLayout (VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        if (barriers.empty())
            return;

        vulkanDevice.getFunctions().CmdPipelineBarrier (commands.commandBuffer,
                                                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                                        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                                        0,
                                                        0,
                                                        nullptr,
                                                        0,
                                                        nullptr,
                                                        static_cast<uint32_t> (barriers.size()),
                                                        barriers.data());
    }

    /** Allocates and writes one descriptor set per group of the pipeline layout, and
        collects the images the dispatch accesses. */
    std::optional<std::vector<VkDescriptorSet>> makeDescriptorSets (const GpuComputePipelineVulkan& pipe, DispatchImages& images)
    {
        if (pipe.bindings.empty())
            return std::vector<VkDescriptorSet> {};

        const auto& fn = vulkanDevice.getFunctions();
        const auto vkDevice = vulkanDevice.getDevice();

        std::vector<VkDescriptorPoolSize> poolSizes;
        for (const auto& binding : pipe.bindings)
        {
            auto it = std::find_if (poolSizes.begin(), poolSizes.end(), [&] (const auto& size)
                                    {
                return size.type == binding.type;
            });

            if (it != poolSizes.end())
                ++it->descriptorCount;
            else
                poolSizes.push_back ({ binding.type, 1 });
        }

        VkDescriptorPoolCreateInfo poolInfo {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.maxSets = static_cast<uint32_t> (pipe.setLayouts.size());
        poolInfo.poolSizeCount = static_cast<uint32_t> (poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();

        VkDescriptorPool pool = VK_NULL_HANDLE;
        if (fn.CreateDescriptorPool (vkDevice, &poolInfo, nullptr, &pool) != VK_SUCCESS)
            return std::nullopt;

        descriptorPools.push_back (pool);

        std::vector<VkDescriptorSet> sets (pipe.setLayouts.size(), VK_NULL_HANDLE);

        VkDescriptorSetAllocateInfo allocateInfo {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = pool;
        allocateInfo.descriptorSetCount = static_cast<uint32_t> (sets.size());
        allocateInfo.pSetLayouts = pipe.setLayouts.data();

        if (fn.AllocateDescriptorSets (vkDevice, &allocateInfo, sets.data()) != VK_SUCCESS)
            return std::nullopt;

        // Reserved up front: the writes point into these
        std::vector<VkDescriptorBufferInfo> bufferInfos;
        bufferInfos.reserve (pipe.bindings.size());

        std::vector<VkDescriptorImageInfo> imageInfos;
        imageInfos.reserve (pipe.bindings.size());

        std::vector<VkWriteDescriptorSet> writes;

        // Storage images first: a texture the dispatch also samples is then read in GENERAL
        for (const auto& binding : pipe.bindings)
        {
            if (binding.type != VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
                continue;

            auto* view = findStorageTexture (binding.group, binding.binding);
            if (view == nullptr)
                continue;

            useImage (images, *view, VK_IMAGE_LAYOUT_GENERAL);
            images.storageViews.push_back (view);

            imageInfos.push_back ({ VK_NULL_HANDLE, view->vkImageView(), VK_IMAGE_LAYOUT_GENERAL });
            writes.push_back (makeWrite (sets[binding.group], binding, &imageInfos.back(), nullptr));
        }

        for (const auto& binding : pipe.bindings)
        {
            if (binding.type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
            {
                auto* view = findTexture (binding.group, binding.binding);
                if (view == nullptr)
                    continue;

                const auto layout = useImage (images, *view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                // Only a pass recording into the frame may move an image ore tracks for it
                if (! joinsFrame && layout != static_cast<rive::ore::TextureVulkan*> (view->texture())->vkLayout())
                {
                    jassertfalse; // Sample it on a pass begun on a GpuFrame, or once a frame gave it contents
                    return std::nullopt;
                }

                imageInfos.push_back ({ VK_NULL_HANDLE, view->vkImageView(), layout });
                writes.push_back (makeWrite (sets[binding.group], binding, &imageInfos.back(), nullptr));
            }
            else if (binding.type == VK_DESCRIPTOR_TYPE_SAMPLER)
            {
                const auto sampler = findSampler (binding.group, binding.binding);
                if (sampler == VK_NULL_HANDLE)
                    continue;

                imageInfos.push_back ({ sampler, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED });
                writes.push_back (makeWrite (sets[binding.group], binding, &imageInfos.back(), nullptr));
            }
            else if (binding.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER || binding.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            {
                const VkBuffer buffer = binding.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
                                          ? findStorageBuffer (binding.group, binding.binding)
                                          : makeUniformBuffer (binding.group, binding.binding);
                if (buffer == VK_NULL_HANDLE)
                    continue;

                bufferInfos.push_back ({ buffer, 0, VK_WHOLE_SIZE });
                writes.push_back (makeWrite (sets[binding.group], binding, nullptr, &bufferInfos.back()));
            }
        }

        if (! writes.empty())
            fn.UpdateDescriptorSets (vkDevice, static_cast<uint32_t> (writes.size()), writes.data(), 0, nullptr);

        return sets;
    }

    static VkWriteDescriptorSet makeWrite (VkDescriptorSet set,
                                           const GpuComputePipelineVulkan::Binding& binding,
                                           const VkDescriptorImageInfo* imageInfo,
                                           const VkDescriptorBufferInfo* bufferInfo)
    {
        VkWriteDescriptorSet write {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = set;
        write.dstBinding = binding.binding;
        write.descriptorCount = 1;
        write.descriptorType = binding.type;
        write.pImageInfo = imageInfo;
        write.pBufferInfo = bufferInfo;
        return write;
    }

    /** Notes that the dispatch accesses the image of @p view in @p layout, and returns the
        layout its descriptors use: GENERAL once any binding writes the image. */
    static VkImageLayout useImage (DispatchImages& images, const rive::ore::TextureViewVulkan& view, VkImageLayout layout)
    {
        auto* texture = static_cast<rive::ore::TextureVulkan*> (view.texture());

        for (auto& use : images.uses)
        {
            if (use.texture != texture)
                continue;

            if (layout == VK_IMAGE_LAYOUT_GENERAL)
                use.layout = layout;

            return use.layout;
        }

        images.uses.push_back ({ texture, layout });
        return layout;
    }

    /** Keeps an ore view alive for the commands, and returns it as a Vulkan view. */
    rive::ore::TextureViewVulkan* retainView (rive::rcp<rive::ore::TextureView> view)
    {
        auto* viewVulkan = rive::lite_rtti_cast<rive::ore::TextureViewVulkan*> (view.get());
        if (viewVulkan == nullptr || viewVulkan->vkImageView() == VK_NULL_HANDLE)
            return nullptr;

        auto* texture = rive::lite_rtti_cast<rive::ore::TextureVulkan*> (viewVulkan->texture());
        if (texture == nullptr || texture->vkImage() == VK_NULL_HANDLE)
            return nullptr;

        referencedResources.views.push_back (std::move (view));
        return viewVulkan;
    }

    rive::ore::TextureViewVulkan* findTexture (uint32_t group, uint32_t binding)
    {
        for (const auto& texture : texBindings)
        {
            if (static_cast<uint32_t> (texture.group) != group || static_cast<uint32_t> (texture.binding) != binding)
                continue;

            if (texture.texture == nullptr || oreCtx == nullptr)
                return nullptr;

            return retainView (texture.texture->getOrCreateSamplingView (*oreCtx));
        }

        return nullptr;
    }

    rive::ore::TextureViewVulkan* findStorageTexture (uint32_t group, uint32_t binding)
    {
        for (const auto& storage : storageTexBindings)
        {
            if (static_cast<uint32_t> (storage.group) != group || static_cast<uint32_t> (storage.binding) != binding)
                continue;

            if (storage.texture == nullptr || oreCtx == nullptr)
                return nullptr;

            return retainView (storage.texture->getOrCreateView (*oreCtx, false, storage.view));
        }

        return nullptr;
    }

    VkSampler findSampler (uint32_t group, uint32_t binding)
    {
        for (const auto& sampler : samplerBindings)
        {
            if (static_cast<uint32_t> (sampler.group) != group || static_cast<uint32_t> (sampler.binding) != binding || sampler.sampler == nullptr)
                continue;

            auto oreSampler = sampler.sampler->getOreSampler();
            auto* samplerVulkan = rive::lite_rtti_cast<rive::ore::SamplerVulkan*> (oreSampler.get());
            if (samplerVulkan == nullptr || samplerVulkan->vkSampler() == VK_NULL_HANDLE)
                return VK_NULL_HANDLE;

            const auto handle = samplerVulkan->vkSampler();
            referencedResources.samplers.push_back (std::move (oreSampler));
            return handle;
        }

        return VK_NULL_HANDLE;
    }

    VkBuffer findStorageBuffer (uint32_t group, uint32_t binding)
    {
        for (const auto& storage : storageBindings)
        {
            if (static_cast<uint32_t> (storage.group) != group || static_cast<uint32_t> (storage.binding) != binding || storage.buffer == nullptr)
                continue;

            auto* impl = storage.buffer->getImpl();
            if (impl == nullptr || impl->vkStorageBuffer.buffer == nullptr)
                return VK_NULL_HANDLE;

            // The native buffer, not the GpuBuffer: that one holds its device, which a pending
            // handler would then keep alive
            referencedResources.storageBuffers.push_back (impl->vkStorageBuffer.buffer);

            impl->vkStorageBuffer.lastUseCommands = commands.commandBuffer;
            impl->vkStorageBuffer.lastUseGeneration = commands.frameNumber;
            return *impl->vkStorageBuffer.buffer;
        }

        return VK_NULL_HANDLE;
    }

    VkBuffer makeUniformBuffer (uint32_t group, uint32_t binding)
    {
        for (const auto& uniform : uboBindings)
        {
            if (static_cast<uint32_t> (uniform.group) != group || static_cast<uint32_t> (uniform.binding) != binding || uniform.data.empty())
                continue;

            VkBufferCreateInfo bufferInfo {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = static_cast<VkDeviceSize> (uniform.data.size());
            bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

            auto buffer = device.getVulkanContext().makeBuffer (bufferInfo, rive::gpu::vkutil::Mappability::writeOnly);
            if (buffer == nullptr || ! buffer->hasContents())
                return VK_NULL_HANDLE;

            std::memcpy (buffer->contents(), uniform.data.data(), uniform.data.size());
            buffer->flushContents();

            const VkBuffer handle = *buffer;
            referencedResources.uniformBuffers.push_back (std::move (buffer));
            return handle;
        }

        return VK_NULL_HANDLE;
    }

    //==========================================================================

    GpuDeviceVulkan& device;
    VulkanDevice& vulkanDevice;
    rive::ore::Context* oreCtx = nullptr;
    VulkanDevice::Commands commands;
    bool joinsFrame = false;
    bool firstDispatch = true;
    std::vector<VkDescriptorPool> descriptorPools;
    ReferencedResources referencedResources;
};

//==============================================================================

std::unique_ptr<GpuComputePass::Impl> yup_createComputePassImplVulkan (GpuDevice& ctx)
{
    return std::make_unique<GpuComputePassImplVulkan> (static_cast<GpuDeviceVulkan&> (ctx));
}

std::unique_ptr<GpuComputePass::Impl> yup_createComputePassImplVulkan (GpuDevice& ctx, void* commandBuffer, uint64_t generation)
{
    return std::make_unique<GpuComputePassImplVulkan> (static_cast<GpuDeviceVulkan&> (ctx), static_cast<VkCommandBuffer> (commandBuffer), generation);
}

} // namespace yup

#endif // YUP_RIVE_USE_VULKAN
