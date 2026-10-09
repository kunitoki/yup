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

void yup_finishComputePass (GpuComputePass::Impl& pass);

//==============================================================================

struct GpuFrame::Impl
{
    GpuDevice::Ptr device;
    rive::ore::Context* oreCtx = nullptr;
    uint64_t generation = 0;
    void* commandBuffer = nullptr;
    bool submitted = false;
    bool released = false;

    /** What waitForGPU() reported, returned again by later calls. */
    Result gpuResult = Result::ok();

    /** The GPU outcome of the frame, completed by the device. */
    GpuFrameStatus status;

    /** Marks the outcome as read by its own owner, so the device does not report it. */
    void claimStatus();

    /** The ore pass currently open in this frame. Ore no longer finishes a previous pass by
        itself, and Metal and D3D12 allow one open encoder at a time. */
    rive::ore::RenderPass* openPass = nullptr;

    /** The compute pass recording into this frame. Dispatches cannot run inside a render
        pass, so beginning one finishes it, and so does submitting the frame. */
    GpuComputePass::Impl* openComputePass = nullptr;

    std::vector<rive::rcp<rive::ore::Buffer>> liveBuffers;
    std::vector<rive::rcp<rive::ore::TextureView>> liveViews;
    std::vector<rive::rcp<rive::ore::Sampler>> liveSamplers;

    /** Storage buffers bound by render passes. Ore only holds their native handles. */
    std::vector<GpuBuffer::Ptr> liveStorageBuffers;

    rive::rcp<rive::ore::Buffer> acquireUniformBuffer (const void* data, size_t byteSize);

    /** Hands the transient resources to the device, to be freed once enough later
        frames have begun that the GPU cannot still be reading them. */
    void retire();

    /** Frees the transient resources immediately. Only valid once the GPU is
        known to be idle. */
    void releaseNow();

    /** Hands the frame's command buffer back to the device. */
    void releaseCommands();
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
    releaseCommands();

    if (device != nullptr)
    {
        device->retireFrameResources (generation,
                                      std::move (liveBuffers),
                                      std::move (liveViews),
                                      std::move (liveSamplers),
                                      std::move (liveStorageBuffers));
    }

    liveBuffers.clear();
    liveViews.clear();
    liveSamplers.clear();
    liveStorageBuffers.clear();
}

void GpuFrame::Impl::releaseNow()
{
    if (released)
        return;

    released = true;
    releaseCommands();

    if (device != nullptr)
        for (auto& buffer : liveBuffers)
            device->uniformBufferPool.release (std::move (buffer));

    liveBuffers.clear();
    liveViews.clear();
    liveSamplers.clear();
    liveStorageBuffers.clear();
}

void GpuFrame::Impl::claimStatus()
{
    if (auto& block = status.block)
    {
        const ScopedLock sl (*block->lock);
        block->claimed = true;
    }
}

void GpuFrame::Impl::releaseCommands()
{
    if (device != nullptr && commandBuffer != nullptr)
        device->releaseFrameCommands (std::exchange (commandBuffer, nullptr));
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
    i->commandBuffer = ctx->beginFrameCommands (i->generation);

    // A Metal frame records into the device's command buffer, so it has nothing to record into
    if (i->commandBuffer == nullptr && ctx->getPlatform() == GpuPlatform::Metal)
    {
        YUP_DBG ("GpuFrame: no command buffer for the frame");
        i->submitted = true;
        return {};
    }

    rive::ore::Context::FrameDescriptor frameDesc;
    frameDesc.externalCommandBuffer = i->commandBuffer;
    frameDesc.safeFrameNumber = ctx->getSafeFrameGeneration();
    frameDesc.currentFrameNumber = i->generation;

    oreCtx->clearLastError();

    YUP_AUTORELEASEPOOL
    {
        oreCtx->beginFrame (frameDesc);
    }

    if (! oreCtx->lastError().empty())
    {
        YUP_DBG ("GpuFrame: " << String (oreCtx->lastError()));
        i->submitted = true;
        return {};
    }

    i->status = ctx->beginFrameStatus (i->generation);
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

    if (auto& block = i->status.block)
    {
        const ScopedLock sl (*block->lock);
        block->submitted = true;
    }

    auto submitted = Result::ok();

    // Encoders and command buffers are autoreleased, and would otherwise keep their
    // queue and every resource they reference alive until the caller's pool drains
    YUP_AUTORELEASEPOOL
    {
        if (i->openPass != nullptr)
            i->openPass->finish();

        if (i->openComputePass != nullptr)
            yup_finishComputePass (*i->openComputePass);

        i->oreCtx->endFrame();
        submitted = i->device->submitFrameCommands (i->commandBuffer);
    }

    i->submitted = true;

    if (submitted.failed() || ! i->device->reportsFrameCompletion())
        i->device->completeFrameStatus (i->generation, submitted);

    return true;
}

Result GpuFrame::waitForGPU()
{
    auto* i = getImpl();
    if (i == nullptr || i->oreCtx == nullptr)
        return Result::fail ("The frame is not valid");

    if (i->released)
        return i->gpuResult;

    i->claimStatus();
    submit();

    i->oreCtx->waitForGPU();
    const auto waited = i->device->waitFrameCommands (i->commandBuffer);
    const auto completed = i->status.wait();
    i->gpuResult = waited.failed() ? waited : completed;

    i->releaseNow();
    return i->gpuResult;
}

GpuFrameStatus GpuFrame::getStatus()
{
    auto* i = getImpl();
    if (i == nullptr)
        return {};

    i->claimStatus();
    return i->status;
}

} // namespace yup
