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
    rive::ore::Context* oreCtx = nullptr;
    bool submitted = false;

    // Resources that must remain alive from a draw call until waitForGPU()
    // completes or the frame is destroyed.
    std::vector<rive::rcp<rive::ore::Buffer>> liveBuffers;
    std::vector<rive::rcp<rive::ore::TextureView>> liveViews;
    std::vector<rive::rcp<rive::ore::Sampler>> liveSamplers;
};

//==============================================================================

GpuFrame::Impl* GpuFrame::getImpl() noexcept
{
    return impl.getPayload<Impl>();
}

const GpuFrame::Impl* GpuFrame::getImpl() const noexcept
{
    return impl.getPayload<Impl>();
}

//==============================================================================

GpuFrame GpuFrame::begin (GraphicsContext& ctx)
{
    GpuFrame frame;

    auto* oreCtx = ctx.gpuContext();
    if (oreCtx == nullptr)
        return frame;

    frame.impl = TypeErasedObject (GpuFrame::Impl {});

    auto* i = frame.getImpl();
    i->oreCtx = oreCtx;

    oreCtx->beginFrame ({});
    return frame;
}

//==============================================================================

GpuFrame::GpuFrame (GpuFrame&&) noexcept = default;

GpuFrame& GpuFrame::operator= (GpuFrame&& other) noexcept
{
    if (this != &other)
    {
        // Submit any pending frame we currently own before taking over.
        if (auto* i = getImpl(); i != nullptr && ! i->submitted && i->oreCtx != nullptr)
            i->oreCtx->endFrame();

        impl = std::move (other.impl);
    }

    return *this;
}

GpuFrame::~GpuFrame()
{
    submit();
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

    i->oreCtx->endFrame();
    i->submitted = true;
    return true;
}

void GpuFrame::waitForGPU()
{
    auto* i = getImpl();
    if (i == nullptr || i->oreCtx == nullptr)
        return;

    i->oreCtx->waitForGPU();

    // GPU has finished; safe to release all transient resources.
    i->liveBuffers.clear();
    i->liveViews.clear();
    i->liveSamplers.clear();
}

} // namespace yup
