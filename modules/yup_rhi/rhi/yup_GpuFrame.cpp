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

namespace yup
{

//==============================================================================

struct GpuFrame::Impl
{
    GpuDevice::Ptr device;
    rive::ore::Context* oreCtx = nullptr;
    uint64_t generation = 0;
    bool submitted = false;
    bool released = false;
    std::vector<rive::rcp<rive::ore::Buffer>> liveBuffers;
    std::vector<rive::rcp<rive::ore::TextureView>> liveViews;
    std::vector<rive::rcp<rive::ore::Sampler>> liveSamplers;

    rive::rcp<rive::ore::Buffer> acquireUniformBuffer (const void* data, size_t byteSize);

    /** Hands the transient resources to the device, to be freed once enough later
        frames have begun that the GPU cannot still be reading them. */
    void retire();

    /** Frees the transient resources immediately. Only valid once the GPU is
        known to be idle. */
    void releaseNow();
};

//==============================================================================

rive::rcp<rive::ore::Buffer> GpuFrame::Impl::acquireUniformBuffer (const void* data, size_t byteSize)
{
    if (device == nullptr || oreCtx == nullptr || data == nullptr || byteSize == 0)
        return nullptr;

    auto buffer = device->uniformBufferPool.acquire (*oreCtx, byteSize);
    if (buffer == nullptr)
        return nullptr;

    buffer->update (data, static_cast<uint32_t> (byteSize), 0);

    liveBuffers.push_back (buffer);
    return buffer;
}

void GpuFrame::Impl::retire()
{
    if (released)
        return;

    released = true;

    if (device != nullptr)
    {
        device->retireFrameResources (generation,
                                      std::move (liveBuffers),
                                      std::move (liveViews),
                                      std::move (liveSamplers));
    }

    liveBuffers.clear();
    liveViews.clear();
    liveSamplers.clear();
}

void GpuFrame::Impl::releaseNow()
{
    if (released)
        return;

    released = true;

    if (device != nullptr)
        for (auto& buffer : liveBuffers)
            device->uniformBufferPool.release (std::move (buffer));

    liveBuffers.clear();
    liveViews.clear();
    liveSamplers.clear();
}

//==============================================================================

GpuFrame::Impl* GpuFrame::getImpl() noexcept
{
    static_assert (sizeof (Impl) <= ImplSizeBytes,
                   "GpuFrame::ImplSizeBytes is too small for GpuFrame::Impl");

    return impl.getPayload<Impl>();
}

const GpuFrame::Impl* GpuFrame::getImpl() const noexcept
{
    return impl.getPayload<Impl>();
}

//==============================================================================

GpuFrame GpuFrame::begin (GpuDevice::Ptr ctx)
{
    GpuFrame frame;

    auto* oreCtx = ctx->getGpuContext();
    if (oreCtx == nullptr)
        return frame;

    frame.impl = TypeErasedObject (GpuFrame::Impl {});

    auto* i = frame.getImpl();
    i->device = ctx;
    i->oreCtx = oreCtx;
    i->generation = ctx->beginFrameGeneration();

    rive::ore::Context::FrameDescriptor frameDesc;
    frameDesc.externalCommandBuffer = nullptr;
    frameDesc.safeFrameNumber = ctx->getSafeFrameGeneration();
    frameDesc.currentFrameNumber = i->generation;

    oreCtx->beginFrame (frameDesc);
    return frame;
}

//==============================================================================

GpuFrame::GpuFrame (GpuFrame&&) noexcept = default;

GpuFrame& GpuFrame::operator= (GpuFrame&& other) noexcept
{
    if (this != &other)
    {
        submit();

        if (auto* i = getImpl())
            i->retire();

        impl = std::move (other.impl);
    }

    return *this;
}

GpuFrame::~GpuFrame()
{
    submit();

    if (auto* i = getImpl())
        i->retire();
}

//==============================================================================

bool GpuFrame::isValid() const noexcept
{
    auto* i = getImpl();
    return i != nullptr && i->oreCtx != nullptr;
}

bool GpuFrame::submit()
{
    auto* i = getImpl();
    if (i == nullptr || i->oreCtx == nullptr || i->submitted)
        return false;

    i->oreCtx->finishActiveRenderPass();

    i->oreCtx->endFrame();
    i->submitted = true;

    return true;
}

void GpuFrame::waitForGPU()
{
    auto* i = getImpl();
    if (i == nullptr || i->oreCtx == nullptr || i->released)
        return;

    i->oreCtx->waitForGPU();

    i->releaseNow();
}

} // namespace yup
