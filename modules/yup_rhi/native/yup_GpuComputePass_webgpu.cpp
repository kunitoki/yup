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

#if (YUP_EMSCRIPTEN && RIVE_WEBGPU) || YUP_RIVE_USE_DAWN

namespace yup
{

//==============================================================================

class GpuComputePassImplWebGPU final : public GpuComputePass::Impl
{
public:
    GpuComputePassImplWebGPU (wgpu::Device dev, wgpu::Queue q)
        : device (dev)
        , queue (q)
    {
        if (device == nullptr || queue == nullptr)
            return;

        wgpu::CommandEncoderDescriptor encDesc {};
        encDesc.label = "GpuComputePass";
        encoder = device.CreateCommandEncoder (&encDesc);
        if (encoder == nullptr)
            return;

        wgpu::ComputePassDescriptor passDesc {};
        passDesc.label = "GpuComputePass";
        pass = encoder.BeginComputePass (&passDesc);
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

        pass.SetPipeline (pipe->getPipeline());

        std::vector<wgpu::BindGroupEntry> entries;

        for (auto& sb : storageBindings)
        {
            if (sb.buffer == nullptr)
                continue;

            auto* bufImpl = sb.buffer->getImpl();
            if (bufImpl == nullptr || bufImpl->webgpuStorageBuffer == nullptr)
                continue;

            wgpu::BindGroupEntry e {};
            e.binding = static_cast<uint32_t> (sb.binding);
            e.buffer = bufImpl->webgpuStorageBuffer;
            e.offset = 0;
            e.size = bufImpl->webgpuStorageBuffer.GetSize();
            entries.push_back (e);
        }

        for (auto& ub : uboBindings)
        {
            if (ub.data.empty())
                continue;

            wgpu::BufferDescriptor bd {};
            bd.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
            bd.size = (ub.data.size() + 15) & ~15u;
            bd.label = "GpuComputePass UBO";

            wgpu::Buffer buf = device.CreateBuffer (&bd);
            if (buf == nullptr)
                continue;

            queue.WriteBuffer (buf, 0, ub.data.data(), ub.data.size());

            wgpu::BindGroupEntry e {};
            e.binding = static_cast<uint32_t> (ub.binding);
            e.buffer = buf;
            e.offset = 0;
            e.size = ub.data.size();
            entries.push_back (e);

            tempUniformBuffers.push_back (std::move (buf));
        }

        if (! entries.empty())
        {
            // When the pipeline was created with an implicit layout (the default),
            // we must use its layout for bind groups — not a manually-built one.
            wgpu::BindGroupLayout layout = pipe->getPipeline().GetBindGroupLayout (0);
            if (layout != nullptr)
            {
                wgpu::BindGroupDescriptor bgDesc {};
                bgDesc.layout = layout;
                bgDesc.entryCount = entries.size();
                bgDesc.entries = entries.data();

                wgpu::BindGroup bg = device.CreateBindGroup (&bgDesc);
                if (bg != nullptr)
                    pass.SetBindGroup (0, bg, 0, nullptr);
            }
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

        if (encoder != nullptr)
        {
            wgpu::CommandBuffer commands = encoder.Finish();
            encoder = nullptr;

            if (commands != nullptr)
            {
                wgpu::CommandBuffer cmds[] = { commands };
                queue.Submit (1, cmds);
            }
        }

        tempUniformBuffers.clear();
    }

private:
    wgpu::Device device;
    wgpu::Queue queue;
    wgpu::CommandEncoder encoder;
    wgpu::ComputePassEncoder pass;
    std::vector<wgpu::Buffer> tempUniformBuffers;
};

//==============================================================================

std::unique_ptr<GpuComputePass::Impl> yup_createComputePassImplWebGPU (GpuDevice& ctx)
{
#if YUP_EMSCRIPTEN && RIVE_WEBGPU
    auto& wc = static_cast<GpuDeviceWebGPU&> (ctx);
    return std::make_unique<GpuComputePassImplWebGPU> (wc.getWgpuDevice(), wc.getWgpuQueue());
#elif YUP_RIVE_USE_DAWN
    auto& dc = static_cast<GpuDeviceDawn&> (ctx);
    return std::make_unique<GpuComputePassImplWebGPU> (dc.getDevice(), dc.getQueue());
#endif
}

} // namespace yup

#endif // WebGPU / Dawn
