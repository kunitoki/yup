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

constexpr uint8_t forwardPassShaderBundle[] = {
#include "yup_renderer_shaders/yup_ForwardPass.inc"
};
constexpr uint8_t forwardPassAreaLightsShaderBundle[] = {
#include "yup_renderer_shaders/yup_ForwardPass_area_lights.inc"
};
constexpr uint8_t forwardPassSoftShadowsShaderBundle[] = {
#include "yup_renderer_shaders/yup_ForwardPass_soft_shadows.inc"
};
constexpr uint8_t forwardPassAreaLightsSoftShadowsShaderBundle[] = {
#include "yup_renderer_shaders/yup_ForwardPass_area_lights_soft_shadows.inc"
};
constexpr uint8_t forwardPassLocalIndirectShaderBundle[] = {
#include "yup_renderer_shaders/yup_ForwardPass_local_indirect.inc"
};
constexpr uint8_t forwardPassAreaLightsLocalIndirectShaderBundle[] = {
#include "yup_renderer_shaders/yup_ForwardPass_area_lights_local_indirect.inc"
};
constexpr uint8_t forwardPassSoftShadowsLocalIndirectShaderBundle[] = {
#include "yup_renderer_shaders/yup_ForwardPass_soft_shadows_local_indirect.inc"
};
constexpr uint8_t forwardPassAreaLightsSoftShadowsLocalIndirectShaderBundle[] = {
#include "yup_renderer_shaders/yup_ForwardPass_area_lights_soft_shadows_local_indirect.inc"
};
constexpr uint8_t forwardPassLocalIndirectLayersShaderBundle[] = {
#include "yup_renderer_shaders/yup_ForwardPass_local_indirect_layers.inc"
};
constexpr uint8_t forwardPassAreaLightsLocalIndirectLayersShaderBundle[] = {
#include "yup_renderer_shaders/yup_ForwardPass_area_lights_local_indirect_layers.inc"
};
constexpr uint8_t forwardPassSoftShadowsLocalIndirectLayersShaderBundle[] = {
#include "yup_renderer_shaders/yup_ForwardPass_soft_shadows_local_indirect_layers.inc"
};
constexpr uint8_t forwardPassAreaLightsSoftShadowsLocalIndirectLayersShaderBundle[] = {
#include "yup_renderer_shaders/yup_ForwardPass_area_lights_soft_shadows_local_indirect_layers.inc"
};
constexpr uint8_t forwardPassFullShaderBundle[] = {
#include "yup_renderer_shaders/yup_ForwardPass_tier_full.inc"
};
constexpr uint8_t forwardPassFullAreaLightsShaderBundle[] = {
#include "yup_renderer_shaders/yup_ForwardPass_tier_full_area_lights.inc"
};
constexpr uint8_t forwardPassFullSoftShadowsShaderBundle[] = {
#include "yup_renderer_shaders/yup_ForwardPass_tier_full_soft_shadows.inc"
};
constexpr uint8_t forwardPassFullAreaLightsSoftShadowsShaderBundle[] = {
#include "yup_renderer_shaders/yup_ForwardPass_tier_full_area_lights_soft_shadows.inc"
};
constexpr uint8_t backgroundShaderBundle[] = {
#include "yup_renderer_shaders/yup_Background.inc"
};

// The scene bundle of each variant, indexed by getForwardVariantIndex()
const std::pair<const uint8_t*, size_t> forwardPassShaderBundles[] = {
    { forwardPassShaderBundle, sizeof (forwardPassShaderBundle) },
    { forwardPassAreaLightsShaderBundle, sizeof (forwardPassAreaLightsShaderBundle) },
    { forwardPassSoftShadowsShaderBundle, sizeof (forwardPassSoftShadowsShaderBundle) },
    { forwardPassAreaLightsSoftShadowsShaderBundle, sizeof (forwardPassAreaLightsSoftShadowsShaderBundle) },
    { forwardPassFullShaderBundle, sizeof (forwardPassFullShaderBundle) },
    { forwardPassFullAreaLightsShaderBundle, sizeof (forwardPassFullAreaLightsShaderBundle) },
    { forwardPassFullSoftShadowsShaderBundle, sizeof (forwardPassFullSoftShadowsShaderBundle) },
    { forwardPassFullAreaLightsSoftShadowsShaderBundle, sizeof (forwardPassFullAreaLightsSoftShadowsShaderBundle) },
    { forwardPassLocalIndirectShaderBundle, sizeof (forwardPassLocalIndirectShaderBundle) },
    { forwardPassAreaLightsLocalIndirectShaderBundle, sizeof (forwardPassAreaLightsLocalIndirectShaderBundle) },
    { forwardPassSoftShadowsLocalIndirectShaderBundle, sizeof (forwardPassSoftShadowsLocalIndirectShaderBundle) },
    { forwardPassAreaLightsSoftShadowsLocalIndirectShaderBundle, sizeof (forwardPassAreaLightsSoftShadowsLocalIndirectShaderBundle) },
    { forwardPassLocalIndirectLayersShaderBundle, sizeof (forwardPassLocalIndirectLayersShaderBundle) },
    { forwardPassAreaLightsLocalIndirectLayersShaderBundle, sizeof (forwardPassAreaLightsLocalIndirectLayersShaderBundle) },
    { forwardPassSoftShadowsLocalIndirectLayersShaderBundle, sizeof (forwardPassSoftShadowsLocalIndirectLayersShaderBundle) },
    { forwardPassAreaLightsSoftShadowsLocalIndirectLayersShaderBundle, sizeof (forwardPassAreaLightsSoftShadowsLocalIndirectLayersShaderBundle) },
};

// The local indirect variants, and their layers variants, are raster only: the full tier's blended
// surfaces read no ambient occlusion, and always shade the material layers
size_t getForwardVariantIndex (bool velocity, bool areaLights, bool shadowFilters, bool localIndirect, bool layers) noexcept
{
    return (areaLights ? 1u : 0u) + (shadowFilters ? 2u : 0u) + (velocity ? 4u : (layers ? 12u : (localIndirect ? 8u : 0u)));
}

//==============================================================================
// std140 mirror of LightListBlock in lib/lights.glsl, the lights of one raster draw, count in x
// (set 1: LightingInputs, set 2 draw and material: GeometryBindings)
struct ForwardLightListBlock
{
    GpuLight lights[SceneDrawList::maxLightsPerDraw];
    float count[4];
};

static_assert (sizeof (ForwardLightListBlock) == 528);

// std140 mirror of ProbeListBlock in lib/probes.glsl, the reflection probes of one raster draw, count
// in x, and its irradiance volume when count y is 1 (set 2 binding 13, local indirect variants only)
struct ForwardProbeListBlock
{
    GpuReflectionProbe probes[2];
    GpuIrradianceVolume volume;
    float count[4];
};

static_assert (sizeof (ForwardProbeListBlock) == 384);

size_t getForwardPipelineIndex (const SceneDrawList::DrawItem& item) noexcept
{
    return (item.material->alphaMode == Material::AlphaMode::blend ? GeometryBindings::numRasterStates : 0u)
         + GeometryBindings::getRasterState (item);
}

} // namespace

//==============================================================================
struct ForwardPass::Variant
{
    GpuTextureFormat colorFormat;
    GpuTextureFormat depthFormat;
    uint32_t sampleCount = 1;
    bool velocity = false;
    bool areaLights = false;
    bool shadowFilters = false; // The shadow filter variant, holding the soft shadows
    bool localIndirect = false; // Raster: the local indirect variant, reading the ambient occlusion
    bool layers = false;        // Raster: its layers variant, shading the clearcoat and the sheen

    std::array<GpuPipeline::Ptr, 2 * GeometryBindings::numRasterStates> pipelines; // [blend * 4 + raster state], blend only on full
    GpuPipeline::Ptr background; // Raster only: the full tier shows the environment from LightingPass
    String error;

    bool matches (const Targets& targets, uint32_t samples, bool withAreaLights, bool withShadowFilters, bool withLocalIndirect, bool withLayers) const noexcept
    {
        return colorFormat == targets.color->getFormat()
            && depthFormat == targets.depth->getFormat()
            && sampleCount == samples
            && velocity == (targets.velocity != nullptr)
            && areaLights == withAreaLights
            && shadowFilters == withShadowFilters
            && localIndirect == withLocalIndirect
            && layers == withLayers;
    }

    String compile (const GpuDevice::Ptr& device)
    {
        const auto& [data, size] = forwardPassShaderBundles[getForwardVariantIndex (velocity, areaLights, shadowFilters, localIndirect, layers)];
        auto bundle = ShaderBundle::loadFromData (data, size);
        if (bundle.failed())
            return "Unable to load the scene shader bundle: " + bundle.getErrorMessage();

        const auto addColorTargets = [this] (GpuPipelineOptions& options, bool blend)
        {
            auto& target = options.colorTargets.emplace_back();
            target.format = colorFormat;
            target.blendEnabled = blend;

            if (velocity)
            {
                auto& velocityTarget = options.colorTargets.emplace_back();
                velocityTarget.format = GpuTextureFormat::rg16float;
                velocityTarget.blendEnabled = false;
            }
        };

        // The full tier draws its opaque items into the G-buffer, so only its blended ones come here
        for (size_t index = velocity ? GeometryBindings::numRasterStates : 0; index < pipelines.size(); ++index)
        {
            const auto blend = index >= GeometryBindings::numRasterStates;

            GpuPipelineOptions options;
            GeometryBindings::setPipelineGeometry (options, index % GeometryBindings::numRasterStates);

            addColorTargets (options, blend);

            options.depthStencil.enabled = true;
            options.depthStencil.format = depthFormat;
            options.depthStencil.depthCompare = GpuCompareFunction::lessEqual;
            options.depthStencil.depthWriteEnabled = ! blend;
            options.sampleCount = sampleCount;

            auto result = GpuPipeline::compileFromBundle (device, bundle.getReference(), options);
            if (result.failed())
                return "Unable to compile the scene pipeline: " + result.getErrorMessage();

            pipelines[index] = result.getValue();
        }

        if (velocity)
            return {};

        // The environment behind everything: drawn first, without testing or writing depth
        auto backgroundBundle = ShaderBundle::loadFromData (backgroundShaderBundle, sizeof (backgroundShaderBundle));
        if (backgroundBundle.failed())
            return "Unable to load the background shader bundle: " + backgroundBundle.getErrorMessage();

        GpuPipelineOptions options;
        options.topology = GpuPrimitiveTopology::triangleList;
        options.cullMode = GpuCullMode::none;
        addColorTargets (options, false);
        options.depthStencil.enabled = true;
        options.depthStencil.format = depthFormat;
        options.depthStencil.depthCompare = GpuCompareFunction::always;
        options.depthStencil.depthWriteEnabled = false;
        options.sampleCount = sampleCount;

        auto result = GpuPipeline::compileFromBundle (device, backgroundBundle.getReference(), options);
        if (result.failed())
            return "Unable to compile the background pipeline: " + result.getErrorMessage();

        background = result.getValue();
        return {};
    }
};

//==============================================================================
ForwardPass::ForwardPass() = default;

ForwardPass::~ForwardPass() = default;

//==============================================================================
Result ForwardPass::createDefaults (const GpuDevice::Ptr& device)
{
    if (auto result = geometry.prepare (device); result.failed())
        return result;

    blackCube = LightingInputs::createBlackCube (device);

    if (blackCube == nullptr)
        return Result::fail ("Unable to create the default scene textures");

    return Result::ok();
}

ResultValue<ForwardPass::Variant*> ForwardPass::prepareVariant (const GpuDevice::Ptr& device, const Targets& targets, uint32_t sampleCount, bool areaLights, bool shadowFilters, bool localIndirect, bool layers)
{
    if (device == nullptr || targets.color == nullptr || targets.depth == nullptr)
        return makeResultValueFail ("No device or no forward targets");

    if (! defaults)
        defaults = createDefaults (device);

    if (defaults->failed())
        return makeResultValueFail (defaults->getErrorMessage());

    auto existing = std::find_if (variants.begin(), variants.end(), [&] (const auto& variant)
    {
        return variant->matches (targets, sampleCount, areaLights, shadowFilters, localIndirect, layers);
    });

    if (existing == variants.end())
    {
        auto variant = std::make_unique<Variant>();
        variant->colorFormat = targets.color->getFormat();
        variant->depthFormat = targets.depth->getFormat();
        variant->sampleCount = sampleCount;
        variant->velocity = targets.velocity != nullptr;
        variant->areaLights = areaLights;
        variant->shadowFilters = shadowFilters;
        variant->localIndirect = localIndirect;
        variant->layers = layers;
        variant->error = layers && layersFailForTesting ? String ("The layers variant failed for testing") : variant->compile (device);

        variants.push_back (std::move (variant));
        existing = std::prev (variants.end());
    }

    if ((*existing)->error.isNotEmpty())
        return makeResultValueFail ((*existing)->error);

    return makeResultValueOk (existing->get());
}

Result ForwardPass::prepare (const GpuDevice::Ptr& device, const Targets& targets, uint32_t sampleCount, bool areaLights)
{
    const auto variant = prepareVariant (device, targets, sampleCount, areaLights, false, false, false);
    return variant.failed() ? Result::fail (variant.getErrorMessage()) : Result::ok();
}

//==============================================================================
bool ForwardPass::render (const GpuDevice::Ptr& device, GpuFrame& frame, const ViewData& view, const LightingInputs& lighting,
                          Span<const SceneDrawList::DrawItem> opaque, Span<const SceneDrawList::DrawItem> blend,
                          const Targets& targets, Output output, uint32_t sampleCount, std::optional<GpuColor> clear)
{
    usedLocalIndirect = false;
    usedLayers = false;

    // Over the loaded targets, nothing to draw leaves them as they are, and compiles nothing
    if (! clear.has_value() && opaque.empty() && blend.empty())
        return true;

    // Raster frames with ambient occlusion, reflection probes or irradiance volumes shade with the
    // local indirect variant, the only one reading them, and frames drawing a material with a
    // clearcoat or a sheen with its layers variant, the only one shading them
    auto layers = targets.velocity == nullptr && lighting.materialLayers;
    auto localIndirect = layers || (targets.velocity == nullptr && (lighting.ambientOcclusion != nullptr || lighting.hasReflectionProbes() || lighting.hasIrradianceVolumes()));
    auto prepared = prepareVariant (device, targets, sampleCount, lighting.areaLights, lighting.shadowFilters, localIndirect, layers);

    // Without its layers variant, the frame shades the materials without their layers
    if (prepared.failed() && layers)
    {
        if (! std::exchange (layersFailureReported, true))
            YUP_DBG ("ForwardPass: the material layers variant " << (lighting.shadowFilters ? "with shadow filters " : "") << "is unavailable, shading without clearcoat and sheen: " << prepared.getErrorMessage());

        layers = false;
        prepared = prepareVariant (device, targets, sampleCount, lighting.areaLights, lighting.shadowFilters, localIndirect, false);
    }

    // Without its local indirect variant, the frame shades without ambient occlusion, reflection probes, irradiance volumes and material layers
    if (prepared.failed() && localIndirect)
    {
        if (! localIndirectFailureReported)
            YUP_DBG ("ForwardPass: the local indirect variant " << (lighting.shadowFilters ? "with shadow filters " : "") << "is unavailable, shading without ambient occlusion, reflection probes, irradiance volumes and material layers: " << prepared.getErrorMessage());

        localIndirectFailureReported = true;
        localIndirect = false;
        prepared = prepareVariant (device, targets, sampleCount, lighting.areaLights, lighting.shadowFilters, false, false);
    }

    // Without its shadow filter variant, the frame shades hard shadows: the default bundles ignore soft filtering
    if (prepared.failed() && lighting.shadowFilters)
    {
        if (! shadowFiltersFailureReported)
            YUP_DBG ("ForwardPass: the shadow filter variant is unavailable, shading hard shadows: " << prepared.getErrorMessage());

        shadowFiltersFailureReported = true;
        prepared = prepareVariant (device, targets, sampleCount, lighting.areaLights, false, false, false);
    }

    if (prepared.failed())
        return false;

    const auto& variant = *prepared.getValue();
    usedLocalIndirect = variant.localIndirect;
    usedLayers = variant.layers;

    if (lighting.samplers.pointClamp == nullptr || lighting.samplers.linearClamp == nullptr || lighting.lookupTables == nullptr
        || lighting.shadowCascades == nullptr || lighting.shadowAtlas == nullptr)
        return false;

    if (variant.velocity && (lighting.lights == nullptr || lighting.clusterLights == nullptr || lighting.reflectionProbeBuffer == nullptr
                             || lighting.irradianceVolumeBuffer == nullptr || ! opaque.empty()))
    {
        jassert (opaque.empty()); // The full tier shades its opaque items through GBufferPass and LightingPass
        return false;
    }

    // The probe and volume atlases stand in black without probes or volumes, in the variants reading them
    const auto readsProbes = variant.velocity || variant.localIndirect;
    const auto needsBlack = lighting.reflectionAtlas == nullptr || lighting.irradianceAtlas == nullptr || lighting.visibilityAtlas == nullptr;
    if (readsProbes && needsBlack && blackTexture == nullptr)
        blackTexture = GeometryBindings::createSolidTexture (device, 0x000000ff);

    if (readsProbes && needsBlack && blackTexture == nullptr)
        return false;

    if (surface == nullptr || surface->asTexture() != targets.color)
        surface = GpuTarget::createFromTexture (device, targets.color);

    if (surface == nullptr)
        return false;

    auto block = lighting.block;
    block.environmentInfo[3] = static_cast<float> (output);

    // Once resolved, nothing reads the multisampled color and depth again
    const auto storeOp = targets.resolve != nullptr ? GpuStoreOp::discard : GpuStoreOp::store;
    const auto load = GpuRenderOptions (GpuLoadOp::load, GpuStoreOp::store);
    auto pass = surface->beginRenderPass (frame, GpuRenderOptions (clear.has_value() ? GpuLoadOp::clear : GpuLoadOp::load, storeOp, clear.value_or (GpuColor::transparentBlack())));
    if (! pass.isValid())
        return false;

    if (targets.resolve != nullptr)
        pass.setResolveTarget (0, targets.resolve);

    if (targets.velocity != nullptr)
        pass.setColorAttachment (1, targets.velocity, clear.has_value() ? GpuRenderOptions (true, GpuColor::transparentBlack()) : load);

    GpuDepthStencilOptions depthOptions;
    depthOptions.depthStoreOp = storeOp;
    if (! clear.has_value())
        depthOptions.depthLoadOp = GpuLoadOp::load;

    pass.setDepthStencilAttachment (targets.depth, depthOptions);

    // Per-frame sets, bound once: every declared resource must be bound, even when unused
    pass.setUniformBuffer (0, 0, &view.getBlock(), sizeof (ViewData::Block));
    pass.setSampler (0, 1, lighting.samplers.linearClamp);
    pass.setSampler (0, 3, lighting.samplers.pointClamp);
    pass.setTexture (0, 6, lighting.environment != nullptr ? lighting.environment : blackCube);
    pass.setTexture (0, 7, lighting.lookupTables);
    pass.setUniformBuffer (1, 0, &block, sizeof (block));
    pass.setTexture (3, 0, lighting.shadowCascades);
    pass.setTexture (3, 2, lighting.shadowAtlas);
    pass.setUniformBuffer (3, 4, &lighting.shadows, sizeof (lighting.shadows));

    // Without it no light has a shadow record, and the pipeline's own comparison sampler stays unread
    if (lighting.samplers.shadowCompare != nullptr)
        pass.setSampler (0, 4, lighting.samplers.shadowCompare);

    // The soft shadow blocker search reads the same maps raw
    if (variant.shadowFilters)
    {
        pass.setTexture (3, 1, lighting.shadowCascades);
        pass.setTexture (3, 3, lighting.shadowAtlas);
    }

    if (readsProbes)
    {
        pass.setTexture (0, 8, lighting.reflectionAtlas != nullptr ? lighting.reflectionAtlas : blackTexture);
        pass.setTexture (0, 9, lighting.irradianceAtlas != nullptr ? lighting.irradianceAtlas : blackTexture);
        pass.setTexture (0, 10, lighting.visibilityAtlas != nullptr ? lighting.visibilityAtlas : blackTexture);
    }

    if (variant.velocity)
    {
        pass.setStorageBuffer (1, 6, lighting.lights);
        pass.setStorageBuffer (1, 7, lighting.clusterLights);
        pass.setStorageBuffer (1, 8, lighting.reflectionProbeBuffer);
        pass.setStorageBuffer (1, 9, lighting.irradianceVolumeBuffer);
    }

    // Without ambient occlusion white occludes nothing; without probes and volumes one empty list serves every draw
    const auto drawsProbeLists = variant.localIndirect && (lighting.hasReflectionProbes() || lighting.hasIrradianceVolumes());

    if (variant.localIndirect)
    {
        pass.setTexture (1, 13, lighting.ambientOcclusion != nullptr ? lighting.ambientOcclusion : geometry.getWhiteTexture());

        if (! drawsProbeLists)
        {
            const ForwardProbeListBlock noProbes {};
            pass.setUniformBuffer (2, 13, &noProbes, sizeof (noProbes));
        }
    }

    GpuPipeline* currentPipeline = nullptr;

    if (clear.has_value() && variant.background != nullptr && lighting.isEnvironmentVisible())
    {
        pass.setPipeline (variant.background);
        currentPipeline = variant.background.get();

        pass.draw (3);
    }

    const auto drawItems = [&] (Span<const SceneDrawList::DrawItem> items)
    {
        for (const auto& item : items)
        {
            const auto& pipeline = variant.pipelines[getForwardPipelineIndex (item)];

            if (pipeline.get() != currentPipeline)
            {
                pass.setPipeline (pipeline);
                currentPipeline = pipeline.get();
            }

            if (! variant.velocity)
            {
                ForwardLightListBlock lightList {};
                for (size_t i = 0; i < static_cast<size_t> (item.numLights); ++i)
                {
                    lightList.lights[i] = lighting.getPackedLight (item.lightIndices[i]);
                    lightList.lights[i].colorIntensity[3] *= item.lightWeights[i];
                }

                lightList.count[0] = static_cast<float> (item.numLights);

                pass.setUniformBuffer (2, 12, &lightList, sizeof (lightList));
            }

            if (drawsProbeLists)
            {
                ForwardProbeListBlock probeList {};
                auto numProbes = 0;
                for (const auto index : item.reflectionProbes)
                    if (index >= 0 && static_cast<size_t> (index) < lighting.reflectionProbes.size())
                        probeList.probes[numProbes++] = lighting.reflectionProbes[static_cast<size_t> (index)];

                probeList.count[0] = static_cast<float> (numProbes);

                if (item.irradianceVolume >= 0 && static_cast<size_t> (item.irradianceVolume) < lighting.irradianceVolumes.size())
                {
                    probeList.volume = lighting.irradianceVolumes[static_cast<size_t> (item.irradianceVolume)];
                    probeList.count[1] = 1.0f;
                }

                pass.setUniformBuffer (2, 13, &probeList, sizeof (probeList));
            }

            geometry.draw (pass, device, item, 0);
        }
    };

    drawItems (opaque);

    // The ambient occlusion holds the opaque surfaces only, behind the blended ones: white leaves
    // them unoccluded, as on the full tier
    if (variant.localIndirect && ! blend.empty())
        pass.setTexture (1, 13, geometry.getWhiteTexture());

    drawItems (blend);

    return pass.finish();
}

} // namespace yup
