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

// Regenerate after changing shaders/yup_LightingPass.frag or the libraries it includes with: just yup3d_shaders
constexpr uint8_t lightingPassShaderBundle[] = {
#include "../shaders/yup_LightingPass_full.inc"
};
constexpr uint8_t lightingPassAreaLightsShaderBundle[] = {
#include "../shaders/yup_LightingPass_full_area_lights.inc"
};

} // namespace

//==============================================================================
LightingPass::LightingPass() = default;

LightingPass::~LightingPass() = default;

//==============================================================================
Result LightingPass::prepare (const GpuDevice::Ptr& device, bool areaLights)
{
    if (! defaults)
    {
        blackTexture = GeometryBindings::createSolidTexture (device, 0x000000ff);
        whiteTexture = GeometryBindings::createSolidTexture (device, 0xffffffff);
        pointClampSampler = GpuSampler::create (device, GpuSamplerDesc (GpuFilter::nearest, GpuWrapMode::clampToEdge));
        linearClampSampler = GpuSampler::create (device, GpuSamplerDesc (GpuFilter::linear, GpuWrapMode::clampToEdge));

        if (blackTexture == nullptr || whiteTexture == nullptr || pointClampSampler == nullptr || linearClampSampler == nullptr)
            defaults = Result::fail ("Unable to create the lighting pass textures");
        else
            defaults = Result::ok();
    }

    if (defaults->failed())
        return *defaults;

    auto& variant = variants[areaLights ? 1 : 0];
    if (! variant.prepared)
    {
        variant.prepared = areaLights ? variant.pass.prepare (device, lightingPassAreaLightsShaderBundle, sizeof (lightingPassAreaLightsShaderBundle), GpuTextureFormat::rgba16float)
                                      : variant.pass.prepare (device, lightingPassShaderBundle, sizeof (lightingPassShaderBundle), GpuTextureFormat::rgba16float);
    }

    return *variant.prepared;
}

bool LightingPass::render (GpuFrame& frame, const ViewData& view, const GBufferPass::Targets& gbuffer,
                           const LightingInputs& lighting, const GpuTexture::Ptr& sceneColor)
{
    auto& variant = variants[lighting.areaLights ? 1 : 0];
    if (! variant.prepared.has_value() || variant.prepared->failed() || sceneColor == nullptr
        || gbuffer.albedoOcclusion == nullptr || gbuffer.normalRoughness == nullptr || gbuffer.emissionMaterial == nullptr || gbuffer.depth == nullptr
        || lighting.lights == nullptr || lighting.clusterLights == nullptr || lighting.materials == nullptr)
        return false;

    // Every pixel is written, so the scene color needs no clear
    return variant.pass.draw (frame, sceneColor, {}, [&] (GpuRenderPass& renderPass)
    {
        renderPass.setUniformBuffer (0, 0, &view.getBlock(), sizeof (ViewData::Block));
        renderPass.setSampler (0, 3, pointClampSampler);
        renderPass.setUniformBuffer (1, 0, &lighting.block, sizeof (lighting.block));
        renderPass.setTexture (1, 1, lighting.environment != nullptr ? lighting.environment : blackTexture);
        renderPass.setSampler (1, 2, lighting.environmentSampler != nullptr ? lighting.environmentSampler : pointClampSampler);
        renderPass.setTexture (1, 3, lighting.shadowMap != nullptr ? lighting.shadowMap : whiteTexture);

        if (lighting.areaLights)
        {
            renderPass.setSampler (0, 1, linearClampSampler);
            renderPass.setTexture (1, 4, lighting.ltcMatrix != nullptr ? lighting.ltcMatrix : blackTexture);
            renderPass.setTexture (1, 5, lighting.ltcAmplitude != nullptr ? lighting.ltcAmplitude : blackTexture);
        }

        renderPass.setStorageBuffer (1, 6, lighting.lights);
        renderPass.setStorageBuffer (1, 7, lighting.clusterLights);
        renderPass.setStorageBuffer (1, 8, lighting.materials);
        renderPass.setTexture (1, 9, gbuffer.albedoOcclusion);
        renderPass.setTexture (1, 10, gbuffer.normalRoughness);
        renderPass.setTexture (1, 11, gbuffer.emissionMaterial);
        renderPass.setTexture (1, 12, gbuffer.depth);
    });
}

void LightingPass::trimUnusedTargets()
{
    // The variant not drawn this frame releases every target it kept
    for (auto& variant : variants)
        variant.pass.trimUnusedTargets();
}

void LightingPass::releaseTargets() noexcept
{
    for (auto& variant : variants)
        variant.pass.releaseTargets();
}

} // namespace yup
