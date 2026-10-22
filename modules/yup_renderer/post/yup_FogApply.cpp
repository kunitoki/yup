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
// Shader bundles built with the module, see shaderBundles in yup_renderer.h

constexpr uint8_t fogApplyShaderBundle[] = {
#include "yup_renderer_shaders/yup_FogApply.inc"
};

// std140 mirror of FogBlock in lib/fog.glsl
struct FogBlock
{
    float medium[4];       // density, heightFalloff, baseHeight, anisotropy
    float albedo[4];       // rgb scattering albedo
    float ambient[4];      // rgb ambient radiance
    float sunRadiance[4];  // rgb the first directional light's color times its intensity
    float sunDirection[4]; // xyz the unit direction towards that light
    float options[4];      // x 1 for a reversibly tone mapped color (the raster mode), y 1 for an infinite far plane
};

static_assert (sizeof (FogBlock) == 96);

// The anisotropy the phase function stays finite with
constexpr float maxFogAnisotropy = 0.95f;

} // namespace

//==============================================================================
bool FogApply::isVisible (const WorldFog& fog) noexcept
{
    const auto isFinite = [] (float value) { return std::isfinite (value); };

    return fog.enabled && fog.density > 0.0f
        && std::all_of (fog.color.begin(), fog.color.end(), isFinite)
        && isFinite (fog.density) && isFinite (fog.heightFalloff) && isFinite (fog.baseHeight) && isFinite (fog.anisotropy);
}

//==============================================================================
FogApply::FogApply (GpuDevice::Ptr deviceToUse)
    : device (std::move (deviceToUse))
{
}

FogApply::~FogApply() = default;

//==============================================================================
Result FogApply::prepare (Pipeline& pipeline, FullscreenPass::Blend blend)
{
    if (! pipeline.prepared)
    {
        pipeline.prepared = device == nullptr ? Result::fail ("No device")
                                              : pipeline.pass.prepare (device, fogApplyShaderBundle, sizeof (fogApplyShaderBundle), GpuTextureFormat::rgba16float, blend);
    }

    return *pipeline.prepared;
}

GpuTexture::Ptr FogApply::render (GpuFrame& frame, const ViewData& view, const WorldFog& fog, const LightingInputs& lighting, Span<const SceneDrawList::Light> lights, FrameResources& resources, const Inputs& inputs)
{
    // A missing input, like a raster prepass that could not be drawn, skips the frame without counting as a failure
    const auto* samplers = resources.getSamplers();
    if (givenUp || ! isVisible (fog) || inputs.color == nullptr || inputs.depth == nullptr || samplers == nullptr)
        return nullptr;

    String failure;
    auto result = [&]() -> GpuTexture::Ptr
    {
        if (failRenders > 0)
        {
            --failRenders;
            failure = "the renders failed for testing";
            return nullptr;
        }

        auto& pipeline = inputs.colorIsReversible ? transient : inPlace;
        if (auto prepared = prepare (pipeline, inputs.colorIsReversible ? FullscreenPass::Blend::replace : FullscreenPass::Blend::premultipliedOver); prepared.failed())
        {
            failure = "unable to compile the fog: " + prepared.getErrorMessage();
            return nullptr;
        }

        auto target = inputs.color;
        if (inputs.colorIsReversible)
        {
            GpuTextureDesc desc (static_cast<uint32_t> (inputs.color->getWidth()), static_cast<uint32_t> (inputs.color->getHeight()), GpuTextureFormat::rgba16float, true);
            desc.label = "FogApply output";

            target = resources.acquire (desc);
            if (target == nullptr)
            {
                failure = "unable to create the target";
                return nullptr;
            }
        }

        // The light reaching the fog, on the CPU (preflight R19): the ambient color and the L0 of the
        // environment's irradiance, which is its mean radiance, then the first directional light
        const auto& lightingBlock = lighting.block;
        const auto anisotropy = jlimit (-maxFogAnisotropy, maxFogAnisotropy, fog.anisotropy);

        FogBlock block {};
        block.medium[0] = fog.density;
        block.medium[1] = fog.heightFalloff;
        block.medium[2] = fog.baseHeight;
        block.medium[3] = anisotropy;

        for (size_t i = 0; i < 3; ++i)
        {
            block.albedo[i] = fog.color[i];
            block.ambient[i] = lightingBlock.ambient[i] + lightingBlock.irradiance[0][i] * lightingBlock.environmentInfo[0];
        }

        const auto sun = std::find_if (lights.begin(), lights.end(), [] (const SceneDrawList::Light& light)
                                       {
            return light.node != nullptr && light.node->type == LightNode::Type::directional && ! GpuSceneData::isDark (*light.node);
        });

        if (sun != lights.end())
        {
            for (size_t i = 0; i < 3; ++i)
                block.sunRadiance[i] = sun->node->color[i] * sun->node->intensity;

            block.sunDirection[0] = -sun->direction.getX();
            block.sunDirection[1] = -sun->direction.getY();
            block.sunDirection[2] = -sun->direction.getZ();
        }

        block.options[0] = inputs.colorIsReversible ? 1.0f : 0.0f;
        // A perspective projection is infinite whenever its far plane does not lie past the near one (CameraNode::getProjectionMatrix())
        const auto& viewBlock = view.getBlock();
        const auto perspective = viewBlock.projection[15] == 0.0f;
        block.options[1] = perspective && ! (viewBlock.clipPlanes[1] > viewBlock.clipPlanes[0]) ? 1.0f : 0.0f;

        // In place, the color is the target: the depth stands in for it, never read
        const auto clear = inputs.colorIsReversible ? std::optional<GpuColor> (GpuColor::transparentBlack()) : std::nullopt;
        const auto drawn = pipeline.pass.draw (
            frame, target, {}, [&] (GpuRenderPass& pass)
            {
            pass.setUniformBuffer (0, 0, &view.getBlock(), sizeof (ViewData::Block));
            pass.setSampler (0, 3, samplers->pointClamp);
            pass.setUniformBuffer (1, 0, &block, sizeof (block));
            pass.setTexture (1, 1, inputs.depth);
            pass.setTexture (1, 2, inputs.colorIsReversible ? inputs.color : inputs.depth);
            },
            clear);

        if (! drawn)
        {
            failure = "unable to record the draw";
            return nullptr;
        }

        return target;
    }();

    if (result != nullptr)
    {
        consecutiveFailures = 0;
        return result;
    }

    // A failure leaves the frame without the fog; the second in a row gives up for good
    if (++consecutiveFailures >= 2 && ! std::exchange (givenUp, true))
    {
        giveUpReason = failure;
        YUP_DBG ("FogApply: " << failure << ", the frames render without the world fog");
    }

    return nullptr;
}

void FogApply::trimUnusedTargets()
{
    inPlace.pass.trimUnusedTargets();
    transient.pass.trimUnusedTargets();
}

void FogApply::releaseTargets() noexcept
{
    inPlace.pass.releaseTargets();
    transient.pass.releaseTargets();
}

size_t FogApply::getNumCachedTargets() const noexcept
{
    return inPlace.pass.getNumCachedTargets() + transient.pass.getNumCachedTargets();
}

} // namespace yup
