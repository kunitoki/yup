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

// Shader bundles built with the module, see shaderBundles in yup_renderer.h
constexpr uint8_t lightingPassShaderBundle[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full.inc"
};
constexpr uint8_t lightingPassAreaLightsShaderBundle[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_area_lights.inc"
};
constexpr uint8_t lightingPassSoftShadowsShaderBundle[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_soft_shadows.inc"
};
constexpr uint8_t lightingPassAreaLightsSoftShadowsShaderBundle[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_area_lights_soft_shadows.inc"
};
constexpr uint8_t lightingPassLocalIndirectShaderBundle[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_local_indirect.inc"
};
constexpr uint8_t lightingPassAreaLightsLocalIndirectShaderBundle[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_area_lights_local_indirect.inc"
};
constexpr uint8_t lightingPassSoftShadowsLocalIndirectShaderBundle[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_soft_shadows_local_indirect.inc"
};
constexpr uint8_t lightingPassAreaLightsSoftShadowsLocalIndirectShaderBundle[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_area_lights_soft_shadows_local_indirect.inc"
};

constexpr uint8_t lightingPassLocalIndirectLayersShaderBundle[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_local_indirect_layers.inc"
};
constexpr uint8_t lightingPassAreaLightsLocalIndirectLayersShaderBundle[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_area_lights_local_indirect_layers.inc"
};
constexpr uint8_t lightingPassSoftShadowsLocalIndirectLayersShaderBundle[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_soft_shadows_local_indirect_layers.inc"
};
constexpr uint8_t lightingPassAreaLightsSoftShadowsLocalIndirectLayersShaderBundle[] = {
#include "yup_renderer_shaders/yup_LightingPass_tier_full_area_lights_soft_shadows_local_indirect_layers.inc"
};

// The bundle of each variant, indexed by getVariantIndex()
const std::pair<const uint8_t*, size_t> lightingPassShaderBundles[] = {
    { lightingPassShaderBundle, sizeof (lightingPassShaderBundle) },
    { lightingPassAreaLightsShaderBundle, sizeof (lightingPassAreaLightsShaderBundle) },
    { lightingPassSoftShadowsShaderBundle, sizeof (lightingPassSoftShadowsShaderBundle) },
    { lightingPassAreaLightsSoftShadowsShaderBundle, sizeof (lightingPassAreaLightsSoftShadowsShaderBundle) },
    { lightingPassLocalIndirectShaderBundle, sizeof (lightingPassLocalIndirectShaderBundle) },
    { lightingPassAreaLightsLocalIndirectShaderBundle, sizeof (lightingPassAreaLightsLocalIndirectShaderBundle) },
    { lightingPassSoftShadowsLocalIndirectShaderBundle, sizeof (lightingPassSoftShadowsLocalIndirectShaderBundle) },
    { lightingPassAreaLightsSoftShadowsLocalIndirectShaderBundle, sizeof (lightingPassAreaLightsSoftShadowsLocalIndirectShaderBundle) },
    { lightingPassLocalIndirectLayersShaderBundle, sizeof (lightingPassLocalIndirectLayersShaderBundle) },
    { lightingPassAreaLightsLocalIndirectLayersShaderBundle, sizeof (lightingPassAreaLightsLocalIndirectLayersShaderBundle) },
    { lightingPassSoftShadowsLocalIndirectLayersShaderBundle, sizeof (lightingPassSoftShadowsLocalIndirectLayersShaderBundle) },
    { lightingPassAreaLightsSoftShadowsLocalIndirectLayersShaderBundle, sizeof (lightingPassAreaLightsSoftShadowsLocalIndirectLayersShaderBundle) },
};

// The layers variants are local indirect variants too
size_t getVariantIndex (bool areaLights, bool shadowFilters, bool localIndirect, bool layers) noexcept
{
    return (areaLights ? 1u : 0u) + (shadowFilters ? 2u : 0u) + (layers ? 8u : (localIndirect ? 4u : 0u));
}

} // namespace

//==============================================================================
LightingPass::LightingPass() = default;

LightingPass::~LightingPass() = default;

//==============================================================================
Result LightingPass::prepare (const GpuDevice::Ptr& device, bool areaLights, bool shadowFilters, bool localIndirect, bool layers)
{
    if (! defaults)
    {
        blackCube = LightingInputs::createBlackCube (device);
        whiteTexture = GeometryBindings::createSolidTexture (device, 0xffffffff);
        blackTexture = GeometryBindings::createSolidTexture (device, 0x000000ff);
        transparentTexture = GeometryBindings::createSolidTexture (device, 0x00000000);

        if (blackCube == nullptr || whiteTexture == nullptr || blackTexture == nullptr || transparentTexture == nullptr)
            defaults = Result::fail ("Unable to create the lighting pass textures");
        else
            defaults = Result::ok();
    }

    if (defaults->failed())
        return *defaults;

    const auto index = getVariantIndex (areaLights, shadowFilters, localIndirect || layers, layers);
    auto& variant = variants[index];
    if (! variant.prepared)
    {
        const auto& [data, size] = lightingPassShaderBundles[index];
        if ((localIndirect || layers) && localIndirectFailsForTesting)
            variant.prepared = Result::fail ("The local indirect variant failed for testing");
        else if (layers && layersFailForTesting)
            variant.prepared = Result::fail ("The layers variant failed for testing");
        else
            variant.prepared = variant.pass.prepare (device, data, size, GpuTextureFormat::rgba16float);
    }

    return *variant.prepared;
}

bool LightingPass::render (const GpuDevice::Ptr& device, GpuFrame& frame, const ViewData& view, const GBufferPass::Targets& gbuffer,
                           const LightingInputs& lighting, const GpuTexture::Ptr& sceneColor)
{
    // The frames with reflection probes or irradiance volumes shade with the local indirect variant,
    // the only one reading them, and the frames with layered opaque materials with its layers
    // variant, the only one shading the clearcoat and the sheen
    auto layers = lighting.materialLayers;
    auto localIndirect = layers || lighting.hasReflectionProbes() || lighting.hasIrradianceVolumes();
    auto shadowFilters = lighting.shadowFilters;
    auto prepared = prepare (device, lighting.areaLights, shadowFilters, localIndirect, layers);
    usedLocalIndirect = false;
    usedLayers = false;

    // Without its layers variant, the frame shades the materials without their layers
    if (prepared.failed() && layers)
    {
        if (! std::exchange (layersFailureReported, true))
            YUP_DBG ("LightingPass: the material layers variant " << (shadowFilters ? "with shadow filters " : "") << "is unavailable, shading without clearcoat and sheen: " << prepared.getErrorMessage());

        layers = false;
        prepared = prepare (device, lighting.areaLights, shadowFilters, localIndirect, false);
    }

    // Without its local indirect variant, the frame shades without reflection probes, irradiance volumes and material layers
    if (prepared.failed() && localIndirect)
    {
        if (! std::exchange (localIndirectFailureReported, true))
            YUP_DBG ("LightingPass: the local indirect variant " << (shadowFilters ? "with shadow filters " : "") << "is unavailable, shading without reflection probes, irradiance volumes and material layers: " << prepared.getErrorMessage());

        localIndirect = false;
        prepared = prepare (device, lighting.areaLights, shadowFilters, false, false);
    }

    // Without its shadow filter variant the frame shades hard shadows and no contact shadows: the
    // default variants ignore both
    if (prepared.failed() && shadowFilters)
    {
        if (! shadowFiltersFailureReported)
            YUP_DBG ("LightingPass: the shadow filter variant is unavailable, shading hard shadows: " << prepared.getErrorMessage());

        shadowFiltersFailureReported = true;
        shadowFilters = false;
        prepared = prepare (device, lighting.areaLights, false, localIndirect, layers);
    }

    auto& variant = variants[getVariantIndex (lighting.areaLights, shadowFilters, localIndirect, layers)];
    if (prepared.failed() || sceneColor == nullptr
        || gbuffer.albedoOcclusion == nullptr || gbuffer.normalRoughness == nullptr || gbuffer.emissionMaterial == nullptr || gbuffer.depth == nullptr
        || lighting.lights == nullptr || lighting.clusterLights == nullptr || lighting.materials == nullptr
        || (localIndirect && (lighting.reflectionProbeBuffer == nullptr || lighting.irradianceVolumeBuffer == nullptr))
        || lighting.samplers.pointClamp == nullptr || lighting.samplers.linearClamp == nullptr || lighting.lookupTables == nullptr
        || lighting.shadowCascades == nullptr || lighting.shadowAtlas == nullptr)
        return false;

    usedLocalIndirect = localIndirect;
    usedLayers = layers;

    // Every pixel is written, so the scene color needs no clear
    return variant.pass.draw (frame, sceneColor, {}, [&] (GpuRenderPass& renderPass)
    {
        renderPass.setUniformBuffer (0, 0, &view.getBlock(), sizeof (ViewData::Block));
        renderPass.setSampler (0, 1, lighting.samplers.linearClamp);
        renderPass.setSampler (0, 3, lighting.samplers.pointClamp);
        renderPass.setTexture (0, 6, lighting.environment != nullptr ? lighting.environment : blackCube);
        renderPass.setTexture (0, 7, lighting.lookupTables);
        renderPass.setUniformBuffer (1, 0, &lighting.block, sizeof (lighting.block));
        renderPass.setTexture (3, 0, lighting.shadowCascades);
        renderPass.setTexture (3, 2, lighting.shadowAtlas);
        renderPass.setUniformBuffer (3, 4, &lighting.shadows, sizeof (lighting.shadows));

        // The soft shadow blocker search reads the same maps raw
        if (shadowFilters)
        {
            renderPass.setTexture (3, 1, lighting.shadowCascades);
            renderPass.setTexture (3, 3, lighting.shadowAtlas);
        }

        // Without it no light has a shadow record, and the pipeline's own comparison sampler stays unread
        if (lighting.samplers.shadowCompare != nullptr)
            renderPass.setSampler (0, 4, lighting.samplers.shadowCompare);

        renderPass.setStorageBuffer (1, 6, lighting.lights);
        renderPass.setStorageBuffer (1, 7, lighting.clusterLights);
        renderPass.setStorageBuffer (1, 8, lighting.materials);
        renderPass.setTexture (1, 9, gbuffer.albedoOcclusion);
        renderPass.setTexture (1, 10, gbuffer.normalRoughness);
        renderPass.setTexture (1, 11, gbuffer.emissionMaterial);
        renderPass.setTexture (1, 12, gbuffer.depth);
        renderPass.setTexture (1, 13, lighting.ambientOcclusion != nullptr ? lighting.ambientOcclusion : whiteTexture);
        renderPass.setTexture (1, 16, lighting.reflections != nullptr ? lighting.reflections : transparentTexture);

        // The probe and volume atlases stand in black without probes or volumes
        if (localIndirect)
        {
            renderPass.setTexture (0, 8, lighting.reflectionAtlas != nullptr ? lighting.reflectionAtlas : blackTexture);
            renderPass.setTexture (0, 9, lighting.irradianceAtlas != nullptr ? lighting.irradianceAtlas : blackTexture);
            renderPass.setTexture (0, 10, lighting.visibilityAtlas != nullptr ? lighting.visibilityAtlas : blackTexture);
            renderPass.setStorageBuffer (1, 14, lighting.reflectionProbeBuffer);
            renderPass.setStorageBuffer (1, 15, lighting.irradianceVolumeBuffer);
        }
    });
}

void LightingPass::trimUnusedTargets()
{
    // The variants not drawn this frame release every target they kept
    for (auto& variant : variants)
        variant.pass.trimUnusedTargets();
}

void LightingPass::releaseTargets() noexcept
{
    for (auto& variant : variants)
        variant.pass.releaseTargets();
}

} // namespace yup
