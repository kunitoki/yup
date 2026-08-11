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

#if YUP_RIVE_USE_METAL && (YUP_MAC || YUP_IOS)

namespace yup
{

//==============================================================================

class GpuComputePassImplMetal final : public GpuComputePass::Impl
{
public:
    GpuComputePassImplMetal (id<MTLDevice> device, id<MTLCommandQueue> queue)
        : device (device)
    {
        if (device == nil || queue == nil)
            return;

        YUP_AUTORELEASEPOOL
        {
            commandBuffer = [queue commandBuffer];
            if (commandBuffer == nil)
                return;

            encoder = [commandBuffer computeCommandEncoder];
            if (encoder == nil)
                commandBuffer = nil;
        }
    }

    ~GpuComputePassImplMetal() override
    {
        if (! finished)
            finish();
    }

    //==========================================================================

    bool isValid() const override
    {
        return encoder != nil;
    }

    //==========================================================================

    bool dispatch (uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ) override
    {
        if (encoder == nil || pipelineRef == nullptr)
            return false;

        auto* pipe = dynamic_cast<GpuComputePipelineMetal*> (pipelineRef.get());
        if (pipe == nullptr || pipe->getPipelineState() == nil)
            return false;

        YUP_AUTORELEASEPOOL
        {
            [encoder setComputePipelineState:pipe->getPipelineState()];

            for (auto& sb : storageBindings)
            {
                if (sb.buffer == nullptr)
                    continue;

                auto* bufImpl = sb.buffer->getImpl();
                if (bufImpl == nullptr || bufImpl->mtlStorageBuffer == nil)
                    continue;

                [encoder setBuffer:bufImpl->mtlStorageBuffer
                            offset:0
                           atIndex:static_cast<NSUInteger> (sb.group * 16 + sb.binding)];
            }

            for (auto& ub : uboBindings)
            {
                if (ub.data.empty())
                    continue;

                NSUInteger idx = static_cast<NSUInteger> (ub.group * 16 + ub.binding);

                id<MTLBuffer> tmp = [device newBufferWithBytes:ub.data.data()
                                                        length:ub.data.size()
                                                       options:MTLResourceStorageModeShared];
                if (tmp != nil)
                {
                    [encoder setBuffer:tmp offset:0 atIndex:idx];
                    tempBuffers.push_back (tmp);
                }
            }

            auto wgs = pipe->getWorkgroupSize();
            MTLSize tgSize = MTLSizeMake (wgs.x, wgs.y, wgs.z);
            MTLSize tgCount = MTLSizeMake (groupsX, groupsY, groupsZ);

            [encoder dispatchThreadgroups:tgCount threadsPerThreadgroup:tgSize];
        }

        return true;
    }

    //==========================================================================

    void finish() override
    {
        YUP_AUTORELEASEPOOL
        {
            if (encoder != nil)
            {
                [encoder endEncoding];
                encoder = nil;
            }

            if (commandBuffer != nil)
            {
                [commandBuffer commit];
                commandBuffer = nil;
            }

            tempBuffers.clear();
        }
    }

private:
    id<MTLDevice> device = nil;
    id<MTLCommandBuffer> commandBuffer = nil;
    id<MTLComputeCommandEncoder> encoder = nil;
    std::vector<id<MTLBuffer>> tempBuffers;
};

//==============================================================================

std::unique_ptr<GpuComputePass::Impl> yup_createComputePassImplMetal (GpuDevice& ctx)
{
    auto& metalCtx = static_cast<GpuDeviceMetal&> (ctx);

    auto impl = std::make_unique<GpuComputePassImplMetal> (metalCtx.getDevice(),
                                                           metalCtx.getCommandQueue());
    if (! impl->isValid())
        return nullptr;

    return impl;
}

} // namespace yup

#endif // YUP_RIVE_USE_METAL
