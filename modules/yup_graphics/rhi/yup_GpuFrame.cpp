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

GpuFrame GpuFrame::begin (GraphicsContext& ctx)
{
    GpuFrame frame;

    auto* oreCtx = ctx.gpuContext();
    if (oreCtx == nullptr)
        return frame;

    frame.impl = std::make_unique<Impl>();
    frame.impl->oreCtx = oreCtx;

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
        if (impl != nullptr && ! impl->submitted && impl->oreCtx != nullptr)
            impl->oreCtx->endFrame();

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
    return impl != nullptr && impl->oreCtx != nullptr;
}

bool GpuFrame::submit()
{
    if (impl == nullptr || impl->oreCtx == nullptr || impl->submitted)
        return false;

    impl->oreCtx->endFrame();
    impl->submitted = true;
    return true;
}

void GpuFrame::waitForGPU()
{
    if (impl == nullptr || impl->oreCtx == nullptr)
        return;

    impl->oreCtx->waitForGPU();

    // GPU has finished; safe to release all transient resources.
    impl->liveBuffers.clear();
    impl->liveViews.clear();
    impl->liveSamplers.clear();
}

} // namespace yup
