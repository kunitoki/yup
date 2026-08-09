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

class HeadlessGpuDevice : public GpuDevice
{
public:
    HeadlessGpuDevice() = default;

    ~HeadlessGpuDevice() override { releasePooledResources(); }

    GpuPlatform getPlatform() const noexcept override { return GpuPlatform::Headless; }

    std::unique_ptr<OffscreenTarget> createOffscreenTarget (int, int) override
    {
        return nullptr;
    }

    std::unique_ptr<RenderableTarget> createRenderableTarget (int, int) override
    {
        return nullptr;
    }

    void beginOffscreen (OffscreenTarget&, const rive::gpu::RenderContext::FrameDescriptor&) override
    {
    }

    void endOffscreen (OffscreenTarget&) override
    {
    }

    bool readOffscreenPixels (OffscreenTarget&, void*, size_t) override
    {
        return false;
    }
};

//==============================================================================

std::unique_ptr<GpuDevice> yup_constructHeadlessGpuDevice (GpuDevice::Options)
{
    return std::make_unique<HeadlessGpuDevice>();
}

} // namespace yup
