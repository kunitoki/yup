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

#include "ore/wgpu/ore_sampler_wgpu.hpp"
#include "ore/wgpu/ore_texture_wgpu.hpp"

namespace yup
{

//==============================================================================

class GpuComputePassImplWebGPU final : public GpuComputePass::Impl
{
public:
    GpuComputePassImplWebGPU (wgpu::Device dev, wgpu::Queue q, rive::ore::Context* oreCtx)
        : device (dev)
        , queue (q)
        , oreCtx (oreCtx)
    {
        if (device == nullptr || queue == nullptr)
            return;

        wgpu::CommandEncoderDescriptor encDesc {};
        encDesc.label = "GpuComputePass";
        ownedEncoder = device.CreateCommandEncoder (&encDesc);
        beginPass (ownedEncoder);
    }

    /** Records into the command encoder of a frame, which submits it. */
    GpuComputePassImplWebGPU (wgpu::Device dev, wgpu::Queue q, rive::ore::Context* oreCtx, wgpu::CommandEncoder frameEncoder)
        : device (dev)
        , queue (q)
        , oreCtx (oreCtx)
    {
        if (device != nullptr && queue != nullptr)
            beginPass (frameEncoder);
    }

    bool isValid() const override { return pass != nullptr; }

    //==========================================================================

    bool dispatch (uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ) override
    {
        if (pass == nullptr || pipelineRef == nullptr)
            return false;

        auto* pipe = dynamic_cast<GpuComputePipelineWebGPU*> (pipelineRef.get());
        if (pipe == nullptr || pipe->getPipeline() == nullptr)
            return false;

        using Layout = WgslBindingLayout;

        const auto& groupLayouts = pipe->getGroupLayouts();
        std::vector<std::vector<wgpu::BindGroupEntry>> groups (groupLayouts.size());

        // The entry for a binding the shader declares as a resource @p accepts, or nullptr
        auto addEntry = [&] (int group, int binding, auto&& accepts) -> wgpu::BindGroupEntry*
        {
            const auto* resource = pipe->findResource (group, binding);
            if (resource == nullptr || ! accepts (*resource))
            {
                YUP_DBG ("GpuComputePass: the shader declares no matching resource at group " << group << ", binding " << binding);
                return nullptr;
            }

            auto& entry = groups[resource->group].emplace_back();
            entry.binding = resource->binding;
            return std::addressof (entry);
        };

        auto isKind = [] (Layout::ResourceKind kind)
        {
            return [kind] (const Layout::Entry& resource) { return resource.kind == kind; };
        };

        auto isBuffer = [] (bool uniform)
        {
            return [uniform] (const Layout::Entry& resource)
            {
                return resource.kind == Layout::ResourceKind::buffer && (resource.bufferType == Layout::BufferType::uniform) == uniform;
            };
        };

        auto addTextureView = [&] (rive::rcp<rive::ore::TextureView> view, int group, int binding, Layout::ResourceKind kind)
        {
            auto* viewWGPU = rive::lite_rtti_cast<rive::ore::TextureViewWGPU*> (view.get());
            if (viewWGPU == nullptr || viewWGPU->wgpuTextureView() == nullptr)
                return;

            if (auto* entry = addEntry (group, binding, isKind (kind)))
                entry->textureView = viewWGPU->wgpuTextureView();
        };

        for (auto& sb : storageBindings)
        {
            auto* bufImpl = sb.buffer != nullptr ? sb.buffer->getImpl() : nullptr;
            if (bufImpl == nullptr || bufImpl->webgpuStorageBuffer == nullptr)
                continue;

            // WebGPU binds storage buffers in multiples of 4 bytes
            const auto size = bufImpl->webgpuStorageBuffer.GetSize();
            jassert (size % 4 == 0);
            if (size % 4 != 0)
                continue;

            if (auto* entry = addEntry (sb.group, sb.binding, isBuffer (false)))
            {
                entry->buffer = bufImpl->webgpuStorageBuffer;
                entry->offset = 0;
                entry->size = size;
            }
        }

        for (auto& ub : uboBindings)
        {
            if (ub.data.empty())
                continue;

            auto* entry = addEntry (ub.group, ub.binding, isBuffer (true));
            if (entry == nullptr)
                continue;

            wgpu::BufferDescriptor bd {};
            bd.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
            bd.size = (ub.data.size() + 15) & ~15u;
            bd.label = "GpuComputePass UBO";

            wgpu::Buffer buf = device.CreateBuffer (&bd);
            queue.WriteBuffer (buf, 0, ub.data.data(), ub.data.size());

            entry->buffer = buf;
            entry->offset = 0;
            entry->size = ub.data.size();

            tempUniformBuffers.push_back (std::move (buf));
        }

        if (oreCtx != nullptr)
        {
            for (auto& tb : texBindings)
            {
                if (tb.texture != nullptr)
                    addTextureView (tb.texture->getOrCreateSamplingView (*oreCtx), tb.group, tb.binding, Layout::ResourceKind::texture);
            }

            for (auto& sb : storageTexBindings)
            {
                if (sb.texture != nullptr)
                    addTextureView (sb.texture->getOrCreateView (*oreCtx, false, sb.view), sb.group, sb.binding, Layout::ResourceKind::storageTexture);
            }
        }

        for (auto& sb : samplerBindings)
        {
            if (sb.sampler == nullptr)
                continue;

            auto* samplerWGPU = rive::lite_rtti_cast<rive::ore::SamplerWGPU*> (sb.sampler->getOreSampler().get());
            if (samplerWGPU == nullptr || samplerWGPU->wgpuSampler() == nullptr)
                continue;

            if (auto* entry = addEntry (sb.group, sb.binding, isKind (Layout::ResourceKind::sampler)))
                entry->sampler = samplerWGPU->wgpuSampler();
        }

        // A bind group must provide every resource of its layout, or the whole submission is dropped
        for (const auto& resource : pipe->getResources())
        {
            const auto& entries = groups[resource.group];
            const bool bound = std::any_of (entries.begin(), entries.end(), [&] (const auto& entry)
            {
                return entry.binding == resource.binding;
            });

            if (! bound)
            {
                YUP_DBG ("GpuComputePass: '" << resource.name << "' (group " << (int) resource.group << ", binding " << (int) resource.binding << ") is not bound");
                jassertfalse;
                return false;
            }
        }

        pass.SetPipeline (pipe->getPipeline());

        for (size_t group = 0; group < groups.size(); ++group)
        {
            wgpu::BindGroupDescriptor bgDesc {};
            bgDesc.layout = groupLayouts[group];
            bgDesc.entryCount = groups[group].size();
            bgDesc.entries = groups[group].data();

            pass.SetBindGroup (static_cast<uint32_t> (group), device.CreateBindGroup (&bgDesc), 0, nullptr);
        }

        pass.DispatchWorkgroups (groupsX, groupsY, groupsZ);
        return true;
    }

    //==========================================================================

    void finish() override
    {
        if (pass != nullptr)
        {
            pass.End();
            pass = nullptr;
        }

        if (ownedEncoder != nullptr)
        {
            wgpu::CommandBuffer commands = ownedEncoder.Finish();
            ownedEncoder = nullptr;

            if (commands != nullptr)
            {
                wgpu::CommandBuffer cmds[] = { commands };
                queue.Submit (1, cmds);
            }
        }

        tempUniformBuffers.clear();
    }

private:
    void beginPass (wgpu::CommandEncoder& encoder)
    {
        if (encoder == nullptr)
            return;

        wgpu::ComputePassDescriptor passDesc {};
        passDesc.label = "GpuComputePass";
        pass = encoder.BeginComputePass (&passDesc);
    }

    wgpu::Device device;
    wgpu::Queue queue;
    rive::ore::Context* oreCtx = nullptr;
    wgpu::CommandEncoder ownedEncoder;
    wgpu::ComputePassEncoder pass;
    std::vector<wgpu::Buffer> tempUniformBuffers;
};

//==============================================================================

std::unique_ptr<GpuComputePass::Impl> yup_createComputePassImplWebGPU (GpuDevice& ctx)
{
    auto& wc = static_cast<GpuDeviceWebGPU&> (ctx);
    return std::make_unique<GpuComputePassImplWebGPU> (wc.getWgpuDevice(), wc.getWgpuQueue(), wc.getGpuContext());
}

std::unique_ptr<GpuComputePass::Impl> yup_createComputePassImplWebGPU (GpuDevice& ctx, void* commandBuffer)
{
    auto* frameCommands = static_cast<FrameCommandsWebGPU*> (commandBuffer);
    if (frameCommands == nullptr || frameCommands->encoder == nullptr)
        return nullptr;

    auto& wc = static_cast<GpuDeviceWebGPU&> (ctx);
    return std::make_unique<GpuComputePassImplWebGPU> (wc.getWgpuDevice(), wc.getWgpuQueue(), wc.getGpuContext(), frameCommands->encoder);
}

} // namespace yup

#endif // YUP_EMSCRIPTEN && RIVE_WEBGPU
