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

// The screen-space diffuse GI: the same passes gathering the light too, and the composite. Regenerate
// them after changing those shaders or shaders/yup_SsgiComposite.frag with: just yup3d_shaders
constexpr uint8_t gtaoTraceIndirectShaderBundle[] = {
#include "../shaders/yup_GtaoTrace_screen_space_gi.inc"
};
constexpr uint8_t gtaoDenoiseIndirectShaderBundle[] = {
#include "../shaders/yup_GtaoDenoise_screen_space_gi.inc"
};
constexpr uint8_t ssgiCompositeShaderBundle[] = {
#include "../shaders/yup_SsgiComposite.inc"
};

constexpr auto ambientOcclusionFormat = GpuTextureFormat::rgba8unorm;
constexpr auto indirectDiffuseFormat = GpuTextureFormat::rgba16float;
constexpr std::array indirectDiffuseFormats { ambientOcclusionFormat, indirectDiffuseFormat };

// std140 mirror of GtaoBlock in lib/gtao.glsl
struct GtaoBlock
{
    float params[4];  // occlusion distance, slices, steps per side, input texels per output texel
    float noise[4];   // 1 for the fixed 4x4 pattern, 1 when the noise turns with the frame, the GI trace: 1 / the previous exposure
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

Result AmbientOcclusionPass::prepareIndirectDiffuse()
{
    if (indirectPrepared)
        return *indirectPrepared;

    indirectPrepared = [&]
    {
        if (auto result = traceIndirect.prepare (device, gtaoTraceIndirectShaderBundle, sizeof (gtaoTraceIndirectShaderBundle), indirectDiffuseFormats); result.failed())
            return Result::fail ("Unable to compile the GI trace: " + result.getErrorMessage());

        if (auto result = denoiseIndirect.prepare (device, gtaoDenoiseIndirectShaderBundle, sizeof (gtaoDenoiseIndirectShaderBundle), indirectDiffuseFormats); result.failed())
            return Result::fail ("Unable to compile the GI denoise: " + result.getErrorMessage());

        if (auto result = composite.prepare (device, ssgiCompositeShaderBundle, sizeof (ssgiCompositeShaderBundle), indirectDiffuseFormat, true); result.failed())
            return Result::fail ("Unable to compile the GI composite: " + result.getErrorMessage());

        return Result::ok();
    }();

    return *indirectPrepared;
}

void AmbientOcclusionPass::noteIndirectDiffuseFailure (const String& failure)
{
    releaseIndirectDiffuseTargets();

    // A failure leaves the frame with the ambient occlusion alone; the second in a row gives up for good
    if (++indirectFailures >= 2 && ! std::exchange (indirectGivenUp, true))
        YUP_DBG ("AmbientOcclusionPass: " << failure << ", the frames render without screen-space diffuse GI");
}

bool AmbientOcclusionPass::render (GpuFrame& frame, const ViewData& view, RenderCapabilities::Tier tier, bool temporal,
                                   const GpuTexture::Ptr& depth, const GpuTexture::Ptr& normalRoughness, float distance, FrameResources& resources,
                                   const Radiance& radiance)
{
    output = nullptr;
    indirectDiffuse = nullptr;

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
        releaseIndirectDiffuseTargets();
        return true;
    }

    targetsFailed = false;

    const auto gathering = radiance.previousSceneColor != nullptr && radiance.velocity != nullptr
                        && radiance.previousExposure > 0.0f && ! indirectGivenUp;

    GtaoBlock block {};
    block.params[0] = std::isfinite (distance) && distance > 0.0f ? distance : 0.0f;
    block.params[1] = full && ! temporal ? 4.0f : 2.0f;
    block.params[2] = 6.0f;
    block.params[3] = 2.0f;
    block.noise[0] = full ? 0.0f : 1.0f;
    block.noise[1] = full && temporal ? 1.0f : 0.0f;
    block.noise[2] = gathering ? 1.0f / radiance.previousExposure : 0.0f;

    const auto bindInputs = [&] (GpuRenderPass& pass)
    {
        pass.setUniformBuffer (0, 0, &view.getBlock(), sizeof (ViewData::Block));
        pass.setSampler (0, 3, samplers->pointClamp);
        pass.setUniformBuffer (1, 0, &block, sizeof (block));
        pass.setTexture (1, 1, depth);
        pass.setTexture (1, 2, normalRoughness);
    };

    // The GI variants trace and denoise the ambient occlusion as the others do, and the gathered
    // light alongside into a second pair of targets
    const auto gatherIndirectDiffuse = [&]() -> String
    {
        if (failIndirect)
            return "the GI renders failed for testing";

        if (auto result = prepareIndirectDiffuse(); result.failed())
            return result.getErrorMessage();

        GpuTextureDesc indirectDesc (desc.width, desc.height, indirectDiffuseFormat, true);
        indirectDesc.label = "AmbientOcclusionPass indirect diffuse";
        auto gathered = resources.acquire (indirectDesc);

        indirectDesc.label = "AmbientOcclusionPass indirect diffuse denoise";
        const auto gatheredAcross = resources.acquire (indirectDesc);

        if (gathered == nullptr || gatheredAcross == nullptr)
            return "unable to create the GI targets";

        const GpuRenderOptions writesEveryTexel (GpuLoadOp::dontCare, GpuStoreOp::store);

        const auto denoiseAlong = [&] (int x, int y, const GpuTexture::Ptr& source, const GpuTexture::Ptr& sourceIndirect,
                                       const GpuTexture::Ptr& destination, const GpuTexture::Ptr& destinationIndirect)
        {
            block.denoise[0] = static_cast<float> (x);
            block.denoise[1] = static_cast<float> (y);

            return denoiseIndirect.draw (frame, destination, {}, [&] (GpuRenderPass& pass)
            {
                pass.setColorAttachment (1, destinationIndirect, writesEveryTexel);
                bindInputs (pass);
                pass.setTexture (1, 3, source);
                pass.setTexture (1, 4, sourceIndirect);
            });
        };

        const auto drawn = traceIndirect.draw (frame, traced, {}, [&] (GpuRenderPass& pass)
                           {
                               pass.setColorAttachment (1, gathered, writesEveryTexel);
                               bindInputs (pass);
                               pass.setSampler (0, 1, samplers->linearClamp);
                               pass.setTexture (1, 4, radiance.velocity);
                               pass.setTexture (1, 5, radiance.previousSceneColor);
                           })
                        && denoiseAlong (1, 0, traced, gathered, across, gatheredAcross)
                        && denoiseAlong (0, 1, across, gatheredAcross, traced, gathered);

        if (! drawn)
            return "unable to record a GI draw";

        indirectDiffuse = std::move (gathered);
        return {};
    };

    // A GI failure traces the ambient occlusion alone below
    if (gathering)
    {
        const auto failure = gatherIndirectDiffuse();
        if (failure.isEmpty())
        {
            output = std::move (traced);
            return true;
        }

        noteIndirectDiffuseFailure (failure);
    }
    else
    {
        releaseIndirectDiffuseTargets();
    }

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

bool AmbientOcclusionPass::addIndirectDiffuse (GpuFrame& frame, const ViewData& view, const GBufferPass::Targets& gbuffer,
                                               const LightingInputs& lighting, const GpuTexture::Ptr& sceneColor)
{
    if (indirectDiffuse == nullptr)
        return false;

    const auto failure = [&]() -> String
    {
        if (failIndirect)
            return "the GI renders failed for testing";

        if (auto result = prepareIndirectDiffuse(); result.failed())
            return result.getErrorMessage();

        if (sceneColor == nullptr || gbuffer.albedoOcclusion == nullptr || gbuffer.normalRoughness == nullptr
            || gbuffer.emissionMaterial == nullptr || gbuffer.depth == nullptr || lighting.materials == nullptr
            || lighting.lookupTables == nullptr || lighting.samplers.pointClamp == nullptr || lighting.samplers.linearClamp == nullptr)
            return "a GI input is missing";

        const auto drawn = composite.draw (frame, sceneColor, {}, [&] (GpuRenderPass& pass)
        {
            pass.setUniformBuffer (0, 0, &view.getBlock(), sizeof (ViewData::Block));
            pass.setSampler (0, 1, lighting.samplers.linearClamp);
            pass.setSampler (0, 3, lighting.samplers.pointClamp);
            pass.setTexture (0, 7, lighting.lookupTables);
            pass.setStorageBuffer (1, 8, lighting.materials);
            pass.setTexture (1, 9, gbuffer.albedoOcclusion);
            pass.setTexture (1, 10, gbuffer.normalRoughness);
            pass.setTexture (1, 11, gbuffer.emissionMaterial);
            pass.setTexture (1, 12, gbuffer.depth);
            pass.setTexture (1, 13, indirectDiffuse);
        });

        return drawn ? String() : String ("unable to record the GI composite");
    }();

    if (failure.isEmpty())
    {
        indirectFailures = 0;
        return true;
    }

    noteIndirectDiffuseFailure (failure);
    return false;
}

void AmbientOcclusionPass::trimUnusedTargets()
{
    trace.trimUnusedTargets();
    denoise.trimUnusedTargets();
    traceIndirect.trimUnusedTargets();
    denoiseIndirect.trimUnusedTargets();
    composite.trimUnusedTargets();
}

void AmbientOcclusionPass::releaseTargets() noexcept
{
    output = nullptr;
    trace.releaseTargets();
    denoise.releaseTargets();
    releaseIndirectDiffuseTargets();
}

void AmbientOcclusionPass::releaseIndirectDiffuseTargets() noexcept
{
    indirectDiffuse = nullptr;
    traceIndirect.releaseTargets();
    denoiseIndirect.releaseTargets();
    composite.releaseTargets();
}

} // namespace yup
