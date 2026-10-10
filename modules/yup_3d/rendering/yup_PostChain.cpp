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
PostChain::PostChain() = default;

PostChain::~PostChain() = default;

//==============================================================================
void PostChain::releaseTargets() noexcept
{
    temporalAA.releaseTargets();
    bloom.releaseTargets();
    toneMapping.releaseTargets();
}

bool PostChain::render (const GpuDevice::Ptr& device, GpuFrame& frame, const RenderCapabilities& caps,
                        const RenderSettings& settings, const ViewData& view, FrameResources& resources,
                        const Inputs& inputs, const GpuTexture::Ptr& output)
{
    resolvedColor = inputs.color;
    if (inputs.color == nullptr)
        return false;

    // A skipped effect releases its targets, which would keep its last textures alive
    if (! caps.isEnabled (RenderCapabilities::Feature::temporalAntiAliasing, settings))
        temporalAA.releaseTargets();

    if (! caps.isEnabled (RenderCapabilities::Feature::bloom, settings))
        bloom.releaseTargets();

    if (caps.isEnabled (RenderCapabilities::Feature::temporalAntiAliasing, settings))
    {
        GpuTextureDesc desc (static_cast<uint32_t> (inputs.color->getWidth()), static_cast<uint32_t> (inputs.color->getHeight()), GpuTextureFormat::rgba16float, true);
        desc.label = "PostChain temporal anti-aliasing";

        auto resolved = resources.acquire (desc);
        if (resolved == nullptr
            || ! temporalAA.render (device, frame, view, resources, inputs.color, inputs.depth, inputs.velocity,
                                    inputs.temporalSampleIndex, settings.taaSamples, resolved, inputs.temporalChanging))
            return false;

        resolvedColor = resolved;
    }

    GpuTexture::Ptr glow;

    if (caps.isEnabled (RenderCapabilities::Feature::bloom, settings))
    {
        glow = bloom.render (device, frame, resources, settings, resolvedColor, inputs.colorIsReversible);
        if (glow == nullptr)
            return false;
    }

    if (! toneMapping.render (device, frame, view, resources, resolvedColor, inputs.colorIsReversible, glow, output, inputs.background))
        return false;

    // Targets of textures this frame no longer drew into, like the pool's after a resize, go now
    temporalAA.trimUnusedTargets();
    bloom.trimUnusedTargets();
    toneMapping.trimUnusedTargets();
    return true;
}

} // namespace yup
