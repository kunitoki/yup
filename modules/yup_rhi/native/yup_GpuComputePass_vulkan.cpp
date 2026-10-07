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
/** Records dispatches into a command buffer of the device ring, submitted by finish().

    A pass begun on a device owns a ring command buffer, submitted by finish(): like
    on Metal, the queue orders it before any frame submitted later. A pass begun on a
    GpuFrame records into the frame's command buffer instead, and is submitted with it.
    Everything its commands reference (pipelines, storage buffers, uniform buffers,
    descriptor pools) is kept alive until the GPU completes them.
*/
class GpuComputePassImplVulkan final : public GpuComputePass::Impl
{
public:
    explicit GpuComputePassImplVulkan (GpuDeviceVulkan& deviceToUse)
        : device (deviceToUse)
        , vulkanDevice (deviceToUse.getVulkanDevice())
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
            // The pipelines and buffers are released along with this handler
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

        auto descriptorSets = makeDescriptorSets (pipe);
        if (! descriptorSets.has_value())
            return false;

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

        referencedResources.pipelines.push_back (pipelineRef);
        return true;
    }

    /** Allocates and writes one descriptor set per group of the pipeline layout. */
    std::optional<std::vector<VkDescriptorSet>> makeDescriptorSets (const GpuComputePipelineVulkan& pipe)
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

        std::vector<VkWriteDescriptorSet> writes;

        for (const auto& binding : pipe.bindings)
        {
            VkBuffer buffer = VK_NULL_HANDLE;

            if (binding.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                buffer = findStorageBuffer (binding.group, binding.binding);
            else if (binding.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                buffer = makeUniformBuffer (binding.group, binding.binding);

            // Textures and samplers are not bound by compute passes on any backend
            if (buffer == VK_NULL_HANDLE)
                continue;

            bufferInfos.push_back ({ buffer, 0, VK_WHOLE_SIZE });

            VkWriteDescriptorSet write {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = sets[binding.group];
            write.dstBinding = binding.binding;
            write.descriptorCount = 1;
            write.descriptorType = binding.type;
            write.pBufferInfo = &bufferInfos.back();
            writes.push_back (write);
        }

        if (! writes.empty())
            fn.UpdateDescriptorSets (vkDevice, static_cast<uint32_t> (writes.size()), writes.data(), 0, nullptr);

        return sets;
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
