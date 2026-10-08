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

#if YUP_RIVE_USE_METAL && YUP_APPLE

#include "ore/metal/ore_sampler_metal.hpp"
#include "ore/metal/ore_texture_metal.hpp"

namespace yup
{

//==============================================================================

class GpuComputePassImplMetal final : public GpuComputePass::Impl
{
public:
    /** Encodes into @p commandBufferToEncode, and commits it on finish() when
        @p commitOnFinish is set: a frame commits its own command buffer. */
    GpuComputePassImplMetal (id<MTLDevice> device, rive::ore::Context* oreCtx, id<MTLCommandBuffer> commandBufferToEncode, bool commitOnFinish)
        : device (device)
        , oreCtx (oreCtx)
    {
        if (device == nil || commandBufferToEncode == nil)
            return;

        YUP_AUTORELEASEPOOL
        {
            encoder = [commandBufferToEncode computeCommandEncoder];
            if (encoder != nil && commitOnFinish)
                commandBuffer = commandBufferToEncode;
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
                           atIndex:slotIndex (sb.group, sb.binding)];
            }

            for (auto& ub : uboBindings)
            {
                if (ub.data.empty())
                    continue;

                NSUInteger idx = slotIndex (ub.group, ub.binding);

                id<MTLBuffer> tmp = [device newBufferWithBytes:ub.data.data()
                                                        length:ub.data.size()
                                                       options:MTLResourceStorageModeShared];
                if (tmp != nil)
                {
                    [encoder setBuffer:tmp offset:0 atIndex:idx];
                    tempBuffers.push_back (tmp);
                }
            }

            for (auto& tb : texBindings)
            {
                if (tb.texture == nullptr || oreCtx == nullptr)
                    continue;

                bindTexture (tb.texture->getOrCreateSamplingView (*oreCtx), tb.group, tb.binding);
            }

            for (auto& sb : storageTexBindings)
            {
                if (sb.texture == nullptr || oreCtx == nullptr)
                    continue;

                bindTexture (sb.texture->getOrCreateView (*oreCtx, false, sb.view), sb.group, sb.binding);
            }

            for (auto& sb : samplerBindings)
            {
                if (sb.sampler == nullptr)
                    continue;

                auto* samplerMetal = rive::lite_rtti_cast<rive::ore::SamplerMetal*> (sb.sampler->getOreSampler().get());
                if (samplerMetal == nullptr || samplerMetal->mtlSampler() == nil)
                    continue;

                [encoder setSamplerState:samplerMetal->mtlSampler()
                                 atIndex:slotIndex (sb.group, sb.binding)];
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
            boundViews.clear();
        }
    }

private:
    /** The argument index of a (group, binding) slot, the same for buffers, textures and samplers.
        The transpiled MSL indexes every resource by its binding alone, ignoring the set, so only
        group 0 lines up with it. */
    static NSUInteger slotIndex (int group, int binding)
    {
        jassert (group == 0);
        return static_cast<NSUInteger> (group * 16 + binding);
    }

    void bindTexture (rive::rcp<rive::ore::TextureView> view, int group, int binding)
    {
        auto* viewMetal = rive::lite_rtti_cast<rive::ore::TextureViewMetal*> (view.get());
        if (viewMetal == nullptr || viewMetal->mtlTexture() == nil)
            return;

        [encoder setTexture:viewMetal->mtlTexture() atIndex:slotIndex (group, binding)];
        boundViews.push_back (std::move (view));
    }

    id<MTLDevice> device = nil;
    rive::ore::Context* oreCtx = nullptr;
    id<MTLCommandBuffer> commandBuffer = nil;
    id<MTLComputeCommandEncoder> encoder = nil;
    std::vector<id<MTLBuffer>> tempBuffers;
    std::vector<rive::rcp<rive::ore::TextureView>> boundViews;
};

//==============================================================================

std::unique_ptr<GpuComputePass::Impl> yup_createComputePassImplMetal (GpuDevice& ctx)
{
    auto& metalCtx = static_cast<GpuDeviceMetal&> (ctx);

    id<MTLCommandBuffer> commandBuffer = nil;
    YUP_AUTORELEASEPOOL
    {
        commandBuffer = [metalCtx.getCommandQueue() commandBuffer];
    }

    auto impl = std::make_unique<GpuComputePassImplMetal> (metalCtx.getDevice(), metalCtx.getGpuContext(), commandBuffer, true);
    if (! impl->isValid())
        return nullptr;

    return impl;
}

std::unique_ptr<GpuComputePass::Impl> yup_createComputePassImplMetal (GpuDevice& ctx, rive::ore::Context& frameContext)
{
    auto& metalCtx = static_cast<GpuDeviceMetal&> (ctx);

    id<MTLCommandBuffer> frameCommandBuffer = static_cast<rive::ore::ContextMetal&> (frameContext).commandBuffer();
    jassert (frameCommandBuffer != nil); // Only set between the frame's begin and its submission
    if (frameCommandBuffer == nil)
        return nullptr;

    auto impl = std::make_unique<GpuComputePassImplMetal> (metalCtx.getDevice(), &frameContext, frameCommandBuffer, false);
    if (! impl->isValid())
        return nullptr;

    return impl;
}

} // namespace yup

#endif // YUP_RIVE_USE_METAL
