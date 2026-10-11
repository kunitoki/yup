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

namespace
{

//==============================================================================
// Precompiled bundles (do not edit the .inc by hand). Regenerate them after changing
// shaders/yup_GtaoTrace.frag, shaders/yup_GtaoDenoise.frag or shaders/lib/gtao.glsl with: just yup3d_shaders

constexpr uint8_t gtaoTraceShaderBundle[] = {
#include "../shaders/yup_GtaoTrace.inc"
};
constexpr uint8_t gtaoDenoiseShaderBundle[] = {
#include "../shaders/yup_GtaoDenoise.inc"
};

constexpr auto ambientOcclusionFormat = GpuTextureFormat::rgba8unorm;

// std140 mirror of GtaoBlock in lib/gtao.glsl
struct GtaoBlock
{
    float params[4];  // occlusion distance, slices, steps per side, input texels per output texel
    float noise[4];   // 1 for the fixed 4x4 pattern, 1 when the noise turns with the frame
    float denoise[4]; // the output texel step of the 1D denoise pass
};

static_assert (sizeof (GtaoBlock) == 48);

} // namespace

//==============================================================================
AmbientOcclusionPass::AmbientOcclusionPass (GpuDevice::Ptr deviceToUse)
    : device (std::move (deviceToUse))
{
}

AmbientOcclusionPass::~AmbientOcclusionPass() = default;

//==============================================================================
Result AmbientOcclusionPass::prepare()
{
    if (prepared)
        return *prepared;

    prepared = [&]
    {
        if (device == nullptr)
            return Result::fail ("No device");

        if (auto result = trace.prepare (device, gtaoTraceShaderBundle, sizeof (gtaoTraceShaderBundle), ambientOcclusionFormat); result.failed())
            return Result::fail ("Unable to compile the ambient occlusion trace: " + result.getErrorMessage());

        if (auto result = denoise.prepare (device, gtaoDenoiseShaderBundle, sizeof (gtaoDenoiseShaderBundle), ambientOcclusionFormat); result.failed())
            return Result::fail ("Unable to compile the ambient occlusion denoise: " + result.getErrorMessage());

        return Result::ok();
    }();

    if (prepared->failed())
        YUP_DBG ("AmbientOcclusionPass: " << prepared->getErrorMessage() << ", the frames render without it");

    return *prepared;
}

bool AmbientOcclusionPass::render (GpuFrame& frame, const ViewData& view, RenderCapabilities::Tier tier, bool temporal,
                                   const GpuTexture::Ptr& depth, const GpuTexture::Ptr& normalRoughness, float distance, FrameResources& resources)
{
    output = nullptr;

    const auto* samplers = resources.getSamplers();
    if (prepare().failed() || depth == nullptr || normalRoughness == nullptr || samplers == nullptr)
        return false;

    const auto full = tier == RenderCapabilities::Tier::full;

    // Half resolution on both tiers, ping-ponged: trace into one, denoise across into the other, then down back into the first
    GpuTextureDesc desc ((depth->getWidth() + 1) / 2, (depth->getHeight() + 1) / 2, ambientOcclusionFormat, true);
    desc.label = "AmbientOcclusionPass output";
    auto traced = resources.acquire (desc);

    desc.label = "AmbientOcclusionPass denoise";
    const auto across = resources.acquire (desc);

    // Like the shadow maps, targets the device cannot create leave the frame without it
    if (traced == nullptr || across == nullptr)
    {
        if (! targetsFailed)
            YUP_DBG ("AmbientOcclusionPass: unable to create the targets, the frames render without it");

        targetsFailed = true;
        return true;
    }

    targetsFailed = false;

    GtaoBlock block {};
    block.params[0] = std::isfinite (distance) && distance > 0.0f ? distance : 0.0f;
    block.params[1] = full && ! temporal ? 4.0f : 2.0f;
    block.params[2] = 6.0f;
    block.params[3] = 2.0f;
    block.noise[0] = full ? 0.0f : 1.0f;
    block.noise[1] = full && temporal ? 1.0f : 0.0f;

    const auto bindInputs = [&] (GpuRenderPass& pass)
    {
        pass.setUniformBuffer (0, 0, &view.getBlock(), sizeof (ViewData::Block));
        pass.setSampler (0, 3, samplers->pointClamp);
        pass.setUniformBuffer (1, 0, &block, sizeof (block));
        pass.setTexture (1, 1, depth);
        pass.setTexture (1, 2, normalRoughness);
    };

    const auto denoiseAlong = [&] (int x, int y, const GpuTexture::Ptr& source, const GpuTexture::Ptr& destination)
    {
        block.denoise[0] = static_cast<float> (x);
        block.denoise[1] = static_cast<float> (y);

        return denoise.draw (frame, destination, {}, [&] (GpuRenderPass& pass)
        {
            bindInputs (pass);
            pass.setTexture (1, 3, source);
        });
    };

    // Every draw writes every texel, so the targets need no clear
    if (! trace.draw (frame, traced, {}, bindInputs) || ! denoiseAlong (1, 0, traced, across) || ! denoiseAlong (0, 1, across, traced))
        return false;

    output = std::move (traced);
    return true;
}

void AmbientOcclusionPass::trimUnusedTargets()
{
    trace.trimUnusedTargets();
    denoise.trimUnusedTargets();
}

void AmbientOcclusionPass::releaseTargets() noexcept
{
    output = nullptr;
    trace.releaseTargets();
    denoise.releaseTargets();
}

} // namespace yup
