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

// Regenerate after changing shaders/yup_ShadowPass.vert / .frag or the libraries they include with: just yup3d_shaders
constexpr uint8_t shadowPassShaderBundle[] = {
#include "../shaders/yup_ShadowPass.inc"
};
constexpr uint8_t shadowPassMaskedShaderBundle[] = {
#include "../shaders/yup_ShadowPass_masked.inc"
};

// Receivers compare against a depth offset towards the light by this many texels of the cascade,
// times LightNode::shadowBias, and are moved along their normal by as many where the light grazes them
constexpr float shadowBiasTexels = 1.5f;

// The share of each cascade's depth range over which it hands over to the next one
constexpr float cascadeBlendBand = 0.1f;

// filterInfo.w flag: the shadow noise rotates with the frame, for temporal anti-aliasing to average
constexpr float temporalNoiseFlag = 2.0f;

int findShadowedSun (Span<const SceneDrawList::Light> lights) noexcept
{
    for (size_t i = 0; i < lights.size(); ++i)
    {
        const auto& node = *lights[i].node;
        if (node.castsShadows && node.type == LightNode::Type::directional && ! GpuSceneData::isDark (node))
            return static_cast<int> (i);
    }

    return -1;
}

} // namespace

//==============================================================================
/** What the cascade texture holds: maps are drawn again only when one of these changes. */
struct ShadowRenderer::RenderedState
{
    uint64 signature = 0;
    int resolution = 0;
    int numCascades = 0;
    std::array<Matrix4, ShadowCascades::maxCascades> viewProjections;

    bool operator== (const RenderedState&) const = default;
};

//==============================================================================
ShadowRenderer::ShadowRenderer() = default;

ShadowRenderer::~ShadowRenderer() = default;

//==============================================================================
Result ShadowRenderer::prepare (const GpuDevice::Ptr& device)
{
    if (preparation)
        return *preparation;

    preparation = geometry.prepare (device);
    if (preparation->failed())
        return *preparation;

    for (const auto masked : { false, true })
    {
        auto bundle = masked ? ShaderBundle::loadFromData (shadowPassMaskedShaderBundle, sizeof (shadowPassMaskedShaderBundle))
                             : ShaderBundle::loadFromData (shadowPassShaderBundle, sizeof (shadowPassShaderBundle));
        if (bundle.failed())
            return *(preparation = Result::fail ("Unable to load the shadow shader bundle: " + bundle.getErrorMessage()));

        // Depth only, pushed away from the light by the slope of each triangle. The body of a model
        // can mix windings, so nothing is culled
        GpuPipelineOptions options;
        GeometryBindings::setPipelineGeometry (options, 0);
        options.cullMode = GpuCullMode::none;
        options.depthStencil.enabled = true;
        options.depthStencil.format = GpuTextureFormat::depth32float;
        options.depthStencil.depthCompare = GpuCompareFunction::lessEqual;
        options.depthStencil.depthWriteEnabled = true;
        options.depthStencil.depthBias = 1;
        options.depthStencil.depthBiasSlopeScale = 1.5f;

        auto compiled = GpuPipeline::compileFromBundle (device, bundle.getReference(), options);
        if (compiled.failed())
            return *(preparation = Result::fail ("Unable to compile the shadow pipeline: " + compiled.getErrorMessage()));

        (masked ? maskedPipeline : opaquePipeline) = compiled.getValue();
    }

    return *preparation;
}

GpuTexture::Ptr ShadowRenderer::getCascadeTexture() const noexcept
{
    return cascades != nullptr ? cascades : placeholder;
}

void ShadowRenderer::invalidate() noexcept
{
    rendered.reset();
    placeholderCleared = false;
}

//==============================================================================
Result ShadowRenderer::preparePlaceholder (const GpuDevice::Ptr& device, GpuFrame& frame)
{
    // A one layer array standing in for the cascades: the shaders never read it, since no light
    // then has a cascade record, but every declared texture must be bound with its type
    if (placeholder == nullptr)
    {
        GpuTextureDesc desc (1, 1, GpuTextureFormat::depth32float, true);
        desc.type = GpuTextureType::array2D;
        desc.label = "SceneRenderer shadow placeholder";

        placeholder = GpuTexture::create (device, desc);
        placeholderCleared = false;

        if (placeholder == nullptr)
            return Result::fail ("Unable to create the shadow placeholder");
    }

    if (placeholderCleared)
        return Result::ok();

    auto target = GpuTarget::createFromTexture (device, placeholder, GpuTextureViewDesc (0, 0));
    if (target == nullptr)
        return Result::fail ("Unable to clear the shadow placeholder");

    auto pass = target->beginRenderPass (frame, GpuDepthStencilOptions (1.0f));
    if (! pass.isValid() || ! pass.finish())
        return Result::fail ("Unable to clear the shadow placeholder");

    placeholderCleared = true;
    return Result::ok();
}

bool ShadowRenderer::prepareCascades (const GpuDevice::Ptr& device, int resolution, int numCascades)
{
    const auto size = static_cast<uint32_t> (resolution);
    const auto layers = static_cast<uint32_t> (numCascades);

    if (cascades != nullptr && cascades->getWidth() == size && layerTargets.size() == layers)
        return true;

    rendered.reset();
    layerTargets.clear();

    GpuTextureDesc desc (size, size, GpuTextureFormat::depth32float, true);
    desc.type = GpuTextureType::array2D;
    desc.depthOrArrayLayers = layers;
    desc.label = "SceneRenderer sun cascades";

    cascades = GpuTexture::create (device, desc);
    if (cascades == nullptr)
        return false;

    for (uint32_t layer = 0; layer < layers; ++layer)
    {
        auto target = GpuTarget::createFromTexture (device, cascades, GpuTextureViewDesc (0, layer));
        if (target == nullptr)
        {
            cascades = nullptr;
            layerTargets.clear();
            return false;
        }

        layerTargets.push_back (std::move (target));
    }

    return true;
}

Result ShadowRenderer::renderCascade (const GpuDevice::Ptr& device, GpuFrame& frame, const SceneDrawList& drawList, int layer, const Matrix4& viewProjection)
{
    auto pass = layerTargets[static_cast<size_t> (layer)]->beginRenderPass (frame, GpuDepthStencilOptions (1.0f));
    if (! pass.isValid())
        return Result::fail ("Unable to begin the shadow pass");

    // lib/geometry.glsl transforms by projection * view: the whole light transform goes in the projection
    ViewData::Block block {};
    std::copy_n (Matrix4::identity().getData(), 16, block.view);
    std::copy_n (viewProjection.getData(), 16, block.projection);
    std::copy_n (viewProjection.getData(), 16, block.viewProjection);
    pass.setUniformBuffer (0, 0, &block, sizeof (block));

    GpuPipeline* currentPipeline = nullptr;

    for (const auto& item : drawList.getOpaqueItems())
    {
        const auto& pipeline = item.material->alphaMode == Material::AlphaMode::mask ? maskedPipeline : opaquePipeline;
        if (pipeline.get() != currentPipeline)
        {
            pass.setPipeline (pipeline);
            currentPipeline = pipeline.get();
        }

        geometry.draw (pass, device, item, 0);
    }

    if (! pass.finish())
        return Result::fail ("Unable to record the shadow pass");

    return Result::ok();
}

//==============================================================================
Result ShadowRenderer::render (const GpuDevice::Ptr& device, GpuFrame& frame, const Inputs& inputs, GpuShadowData& out)
{
    out = {};

    const auto lights = inputs.drawList.getLights();
    recordIndices.assign (lights.size(), -1);

    if (opaquePipeline == nullptr || maskedPipeline == nullptr)
        return Result::fail ("The shadow pipelines are not prepared");

    const auto sun = inputs.enabled ? findShadowedSun (lights) : -1;
    const auto resolution = RenderCapabilities::getShadowResolution (inputs.settings);

    ShadowCascades::Inputs cascadeInputs;
    std::array<ShadowCascades::Cascade, ShadowCascades::maxCascades> fitted;
    auto numCascades = 0;

    if (sun >= 0)
    {
        cascadeInputs.setView (inputs.view);
        cascadeInputs.lightDirection = lights[static_cast<size_t> (sun)].direction;
        cascadeInputs.resolution = resolution;

        // An orthographic camera sees every depth at the same scale: one cascade fits it
        const auto orthographic = cascadeInputs.projection.getData()[15] == 1.0f;
        cascadeInputs.count = orthographic ? 1 : inputs.capabilities.getCascadeCount (inputs.settings);

        for (const auto& item : inputs.drawList.getOpaqueItems())
        {
            const auto bounds = item.primitive->bounds.transformedBy (item.world);
            cascadeInputs.casterBounds.expand (bounds);
            cascadeInputs.receiverBounds.expand (bounds);
        }

        for (const auto& item : inputs.drawList.getBlendItems())
            cascadeInputs.receiverBounds.expand (item.primitive->bounds.transformedBy (item.world));

        numCascades = ShadowCascades::build (cascadeInputs, fitted);
    }

    // Nothing casts: the cascades are released, and the placeholder stands in for them
    if (numCascades == 0)
    {
        cascades = nullptr;
        layerTargets.clear();
        rendered.reset();
        return preparePlaceholder (device, frame);
    }

    if (! prepareCascades (device, resolution, numCascades))
        return Result::fail ("Unable to create the shadow cascades");

    const auto& node = *lights[static_cast<size_t> (sun)].node;
    const auto contactShadows = node.contactShadows && inputs.capabilities.isEnabled (RenderCapabilities::Feature::contactShadows, inputs.settings);

    RenderedState state;
    state.signature = inputs.signature;
    state.resolution = resolution;
    state.numCascades = numCascades;

    for (int c = 0; c < numCascades; ++c)
    {
        const auto& cascade = fitted[static_cast<size_t> (c)];
        auto& record = out.records[c];

        // A constant world distance becomes a fraction of the cascade's depth range, which changes with the fit
        const auto texel = cascade.texelWorldSize;
        std::copy_n (cascade.viewProjection.getData(), 16, record.viewProjection);
        record.atlasRect[0] = static_cast<float> (c);
        record.atlasRect[2] = 1.0f;
        record.atlasRect[3] = 1.0f / static_cast<float> (resolution);
        record.params[0] = node.shadowBias * shadowBiasTexels * texel / cascade.depthRange;
        record.params[1] = shadowBiasTexels * texel;
        record.params[3] = cascade.depthRange;

        out.cascadeSplits[c] = cascade.splitFar;
        out.cascadeTexel[c] = texel;
        state.viewProjections[static_cast<size_t> (c)] = cascade.viewProjection;
    }

    out.records[0].contact[0] = contactShadows ? jmax (0.0f, node.contactShadowDistance) : 0.0f;
    out.records[0].contact[1] = jmax (0.0f, node.contactShadowThickness);

    out.sunInfo[0] = static_cast<float> (numCascades);
    out.sunInfo[2] = cascadeBlendBand;
    out.sunInfo[3] = ShadowCascades::cutsOffReceivers (cascadeInputs) ? 1.0f : 0.0f;
    out.filterInfo[3] = inputs.capabilities.isEnabled (RenderCapabilities::Feature::temporalAntiAliasing, inputs.settings) ? temporalNoiseFlag : 0.0f;

    recordIndices[static_cast<size_t> (sun)] = 0;

    if (rendered != nullptr && *rendered == state)
        return Result::ok();

    rendered.reset();

    for (int c = 0; c < numCascades; ++c)
        if (auto result = renderCascade (device, frame, inputs.drawList, c, fitted[static_cast<size_t> (c)].viewProjection); result.failed())
            return result;

    rendered = std::make_unique<RenderedState> (state);
    ++numRenderedFrames;
    return Result::ok();
}

} // namespace yup
