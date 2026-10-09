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
// shaders/yup_GBufferPass.* or shaders/yup_GBufferBackground.frag with: just yup3d_shaders

constexpr uint8_t gbufferPassShaderBundle[] = {
#include "../shaders/yup_GBufferPass_full.inc"
};
constexpr uint8_t gbufferBackgroundShaderBundle[] = {
#include "../shaders/yup_GBufferBackground.inc"
};

constexpr GpuTextureFormat gbufferFormats[] = {
    GpuTextureFormat::rgba8unorm,
    GpuTextureFormat::rgba16float,
    GpuTextureFormat::rgba16float,
    GpuTextureFormat::rg16float
};

void setGBufferTargets (GpuPipelineOptions& options, bool depthWrite, GpuCompareFunction depthCompare)
{
    for (const auto format : gbufferFormats)
    {
        auto& target = options.colorTargets.emplace_back();
        target.format = format;
        target.blendEnabled = false;
    }

    options.depthStencil.enabled = true;
    options.depthStencil.format = GpuTextureFormat::depth32float;
    options.depthStencil.depthCompare = depthCompare;
    options.depthStencil.depthWriteEnabled = depthWrite;
}

ResultValue<GpuPipeline::Ptr> compileGBufferPipeline (const GpuDevice::Ptr& device, const uint8* data, size_t size, const GpuPipelineOptions& options)
{
    auto bundle = ShaderBundle::loadFromData (data, size);
    if (bundle.failed())
        return makeResultValueFail ("Unable to load the G-buffer shader bundle: " + bundle.getErrorMessage());

    auto result = GpuPipeline::compileFromBundle (device, bundle.getReference(), options);
    if (result.failed())
        return makeResultValueFail ("Unable to compile the G-buffer pipeline: " + result.getErrorMessage());

    return makeResultValueOk (result.getValue());
}

} // namespace

//==============================================================================
GBufferPass::GBufferPass() = default;

GBufferPass::~GBufferPass() = default;

//==============================================================================
Result GBufferPass::prepare (const GpuDevice::Ptr& newDevice)
{
    if (prepared)
        return *prepared;

    prepared = [&]
    {
        if (newDevice == nullptr)
            return Result::fail ("No device");

        if (auto result = geometry.prepare (newDevice); result.failed())
            return result;

        for (size_t state = 0; state < pipelines.size(); ++state)
        {
            GpuPipelineOptions options;
            GeometryBindings::setPipelineGeometry (options, state);
            // Strictly in front of the cleared far plane, so a depth of 1 always means no surface
            setGBufferTargets (options, true, GpuCompareFunction::less);

            auto pipeline = compileGBufferPipeline (newDevice, gbufferPassShaderBundle, sizeof (gbufferPassShaderBundle), options);
            if (pipeline.failed())
                return Result::fail (pipeline.getErrorMessage());

            pipelines[state] = pipeline.getValue();
        }

        // A fullscreen triangle behind the surfaces, see render()
        GpuPipelineOptions options;
        options.cullMode = GpuCullMode::none;
        setGBufferTargets (options, false, GpuCompareFunction::lessEqual);

        auto pipeline = compileGBufferPipeline (newDevice, gbufferBackgroundShaderBundle, sizeof (gbufferBackgroundShaderBundle), options);
        if (pipeline.failed())
            return Result::fail (pipeline.getErrorMessage());

        background = pipeline.getValue();
        device = newDevice;
        return Result::ok();
    }();

    return *prepared;
}

GBufferPass::Targets GBufferPass::acquireTargets (FrameResources& resources, GpuTexture::Ptr velocity, GpuTexture::Ptr depth)
{
    Targets targets;
    targets.velocity = std::move (velocity);
    targets.depth = std::move (depth);

    if (targets.velocity == nullptr)
        return targets;

    const auto acquire = [&] (GpuTextureFormat format, const char* label)
    {
        GpuTextureDesc desc (targets.velocity->getWidth(), targets.velocity->getHeight(), format, true);
        desc.label = label;
        return resources.acquire (desc);
    };

    targets.albedoOcclusion = acquire (gbufferFormats[0], "GBufferPass albedo and occlusion");
    targets.normalRoughness = acquire (gbufferFormats[1], "GBufferPass normal and roughness");
    targets.emissionMaterial = acquire (gbufferFormats[2], "GBufferPass emission and material");
    return targets;
}

//==============================================================================
bool GBufferPass::render (GpuFrame& frame, const ViewData& view, Span<const SceneDrawList::DrawItem> items,
                          MaterialTable& materials, const Targets& targets)
{
    if (device == nullptr || targets.albedoOcclusion == nullptr || targets.normalRoughness == nullptr
        || targets.emissionMaterial == nullptr || targets.velocity == nullptr || targets.depth == nullptr)
        return false;

    if (surface == nullptr || surface->asTexture() != targets.albedoOcclusion)
        surface = GpuTarget::createFromTexture (device, targets.albedoOcclusion);

    if (surface == nullptr)
        return false;

    auto pass = surface->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
    if (! pass.isValid())
        return false;

    pass.setColorAttachment (1, targets.normalRoughness, { true, GpuColor::transparentBlack() });
    pass.setColorAttachment (2, targets.emissionMaterial, { true, GpuColor::transparentBlack() });
    pass.setColorAttachment (3, targets.velocity, { true, GpuColor::transparentBlack() });
    pass.setDepthStencilAttachment (targets.depth);

    pass.setUniformBuffer (0, 0, &view.getBlock(), sizeof (ViewData::Block));

    materials.clear();

    GpuPipeline* currentPipeline = nullptr;

    for (const auto& item : items)
    {
        const auto& pipeline = pipelines[GeometryBindings::getRasterState (item)];

        if (pipeline.get() != currentPipeline)
        {
            pass.setPipeline (pipeline);
            currentPipeline = pipeline.get();
        }

        geometry.draw (pass, device, item, materials.getId (*item.material));
    }

    // At the far plane, through a depth range of [1, 1], so the lessEqual test keeps only the
    // texels whose depth is still cleared
    pass.setPipeline (background);
    pass.setViewport (0.0f, 0.0f, static_cast<float> (targets.velocity->getWidth()), static_cast<float> (targets.velocity->getHeight()), 1.0f, 1.0f);
    pass.draw (3);

    return pass.finish();
}

} // namespace yup
