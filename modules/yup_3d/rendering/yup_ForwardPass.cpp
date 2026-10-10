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
// shaders/yup_ForwardPass.* or shaders/yup_Background.* with: just yup3d_shaders

constexpr uint8_t forwardPassShaderBundle[] = {
#include "../shaders/yup_ForwardPass.inc"
};
constexpr uint8_t forwardPassAreaLightsShaderBundle[] = {
#include "../shaders/yup_ForwardPass_area_lights.inc"
};
constexpr uint8_t forwardPassSoftShadowsShaderBundle[] = {
#include "../shaders/yup_ForwardPass_soft_shadows.inc"
};
constexpr uint8_t forwardPassAreaLightsSoftShadowsShaderBundle[] = {
#include "../shaders/yup_ForwardPass_area_lights_soft_shadows.inc"
};
constexpr uint8_t forwardPassFullShaderBundle[] = {
#include "../shaders/yup_ForwardPass_full.inc"
};
constexpr uint8_t forwardPassFullAreaLightsShaderBundle[] = {
#include "../shaders/yup_ForwardPass_full_area_lights.inc"
};
constexpr uint8_t backgroundShaderBundle[] = {
#include "../shaders/yup_Background.inc"
};

//==============================================================================
// std140 mirror of LightListBlock in lib/lights.glsl, the lights of one raster draw, count in x
// (set 1: LightingInputs, set 2 draw and material: GeometryBindings)
struct ForwardLightListBlock
{
    GpuLight lights[SceneDrawList::maxLightsPerDraw];
    float count[4];
};

static_assert (sizeof (ForwardLightListBlock) == 528);

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
    bool softShadows = false; // Always set with velocity: the full tier bundles hold the soft shadow code

    std::array<GpuPipeline::Ptr, 2 * GeometryBindings::numRasterStates> pipelines; // [blend * 4 + raster state], blend only on full
    GpuPipeline::Ptr background; // Raster only: the full tier shows the environment from LightingPass
    String error;

    bool matches (const Targets& targets, uint32_t samples, bool withAreaLights, bool withSoftShadows) const noexcept
    {
        return colorFormat == targets.color->getFormat()
            && depthFormat == targets.depth->getFormat()
            && sampleCount == samples
            && velocity == (targets.velocity != nullptr)
            && areaLights == withAreaLights
            && softShadows == withSoftShadows;
    }

    String compile (const GpuDevice::Ptr& device)
    {
        const auto load = [] (const auto& data) { return ShaderBundle::loadFromData (data, sizeof (data)); };
        auto bundle = velocity      ? (areaLights ? load (forwardPassFullAreaLightsShaderBundle) : load (forwardPassFullShaderBundle))
                    : softShadows ? (areaLights ? load (forwardPassAreaLightsSoftShadowsShaderBundle) : load (forwardPassSoftShadowsShaderBundle))
                                  : (areaLights ? load (forwardPassAreaLightsShaderBundle) : load (forwardPassShaderBundle));
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

    blackTexture = GeometryBindings::createSolidTexture (device, 0x000000ff);

    if (blackTexture == nullptr)
        return Result::fail ("Unable to create the default scene textures");

    return Result::ok();
}

ResultValue<ForwardPass::Variant*> ForwardPass::prepareVariant (const GpuDevice::Ptr& device, const Targets& targets, uint32_t sampleCount, bool areaLights, bool softShadows)
{
    if (device == nullptr || targets.color == nullptr || targets.depth == nullptr)
        return makeResultValueFail ("No device or no forward targets");

    softShadows = softShadows || targets.velocity != nullptr;

    if (! defaults)
        defaults = createDefaults (device);

    if (defaults->failed())
        return makeResultValueFail (defaults->getErrorMessage());

    auto existing = std::find_if (variants.begin(), variants.end(), [&] (const auto& variant)
    {
        return variant->matches (targets, sampleCount, areaLights, softShadows);
    });

    if (existing == variants.end())
    {
        auto variant = std::make_unique<Variant>();
        variant->colorFormat = targets.color->getFormat();
        variant->depthFormat = targets.depth->getFormat();
        variant->sampleCount = sampleCount;
        variant->velocity = targets.velocity != nullptr;
        variant->areaLights = areaLights;
        variant->softShadows = softShadows;
        variant->error = variant->compile (device);

        variants.push_back (std::move (variant));
        existing = std::prev (variants.end());
    }

    if ((*existing)->error.isNotEmpty())
        return makeResultValueFail ((*existing)->error);

    return makeResultValueOk (existing->get());
}

Result ForwardPass::prepare (const GpuDevice::Ptr& device, const Targets& targets, uint32_t sampleCount, bool areaLights)
{
    const auto variant = prepareVariant (device, targets, sampleCount, areaLights, false);
    return variant.failed() ? Result::fail (variant.getErrorMessage()) : Result::ok();
}

//==============================================================================
bool ForwardPass::render (const GpuDevice::Ptr& device, GpuFrame& frame, const ViewData& view, const LightingInputs& lighting,
                          Span<const SceneDrawList::DrawItem> opaque, Span<const SceneDrawList::DrawItem> blend,
                          const Targets& targets, Output output, uint32_t sampleCount, std::optional<GpuColor> clear)
{
    // The shadow records ask for soft filtering when a shadowed light has a size to soften with
    const auto softShadows = lighting.shadows.filterInfo[0] > 0.5f;
    auto prepared = prepareVariant (device, targets, sampleCount, lighting.areaLights, softShadows);

    // Without its soft shadow variant, raster shades hard shadows: the other bundles ignore soft filtering
    if (prepared.failed() && softShadows && targets.velocity == nullptr)
    {
        if (! softShadowsFailureReported)
            YUP_DBG ("ForwardPass: the soft shadow variant is unavailable, shading hard shadows: " << prepared.getErrorMessage());

        softShadowsFailureReported = true;
        prepared = prepareVariant (device, targets, sampleCount, lighting.areaLights, false);
    }

    if (prepared.failed())
        return false;

    const auto& variant = *prepared.getValue();

    // Over the loaded targets, nothing to draw leaves them as they are
    if (! clear.has_value() && opaque.empty() && blend.empty())
        return true;

    if (lighting.samplers.pointClamp == nullptr || lighting.samplers.linearClamp == nullptr || lighting.shadowCascades == nullptr || lighting.shadowAtlas == nullptr)
        return false;

    if (variant.velocity && (lighting.lights == nullptr || lighting.clusterLights == nullptr || ! opaque.empty()))
    {
        jassert (opaque.empty()); // The full tier shades its opaque items through GBufferPass and LightingPass
        return false;
    }

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
    pass.setSampler (0, 3, lighting.samplers.pointClamp);
    pass.setTexture (0, 6, lighting.environment != nullptr ? lighting.environment : blackTexture);
    pass.setSampler (0, 14, lighting.environmentSampler != nullptr ? lighting.environmentSampler : geometry.getDefaultSampler());
    pass.setUniformBuffer (1, 0, &block, sizeof (block));
    pass.setTexture (3, 0, lighting.shadowCascades);
    pass.setTexture (3, 2, lighting.shadowAtlas);
    pass.setUniformBuffer (3, 4, &lighting.shadows, sizeof (lighting.shadows));

    // Without it no light has a shadow record, and the pipeline's own comparison sampler stays unread
    if (lighting.samplers.shadowCompare != nullptr)
        pass.setSampler (0, 4, lighting.samplers.shadowCompare);

    // The soft shadow blocker search reads the same maps raw
    if (variant.softShadows)
    {
        pass.setTexture (3, 1, lighting.shadowCascades);
        pass.setTexture (3, 3, lighting.shadowAtlas);
    }

    if (variant.areaLights)
    {
        pass.setSampler (0, 1, lighting.samplers.linearClamp);
        pass.setTexture (0, 7, lighting.ltcMatrix != nullptr ? lighting.ltcMatrix : blackTexture);
        pass.setTexture (0, 11, lighting.ltcAmplitude != nullptr ? lighting.ltcAmplitude : blackTexture);
    }

    if (variant.velocity)
    {
        pass.setStorageBuffer (1, 6, lighting.lights);
        pass.setStorageBuffer (1, 7, lighting.clusterLights);
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

            geometry.draw (pass, device, item, 0);
        }
    };

    drawItems (opaque);
    drawItems (blend);

    return pass.finish();
}

} // namespace yup
