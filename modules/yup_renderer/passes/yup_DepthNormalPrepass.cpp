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

constexpr uint8_t depthNormalPrepassShaderBundle[] = {
#include "yup_renderer_shaders/yup_DepthNormalPrepass.inc"
};

constexpr auto prepassNormalFormat = GpuTextureFormat::rgba16float;
constexpr auto prepassDepthFormat = GpuTextureFormat::depth32float;

} // namespace

//==============================================================================
DepthNormalPrepass::DepthNormalPrepass (GpuDevice::Ptr deviceToUse)
    : device (std::move (deviceToUse))
{
}

DepthNormalPrepass::~DepthNormalPrepass() = default;

//==============================================================================
bool DepthNormalPrepass::isSupported (const GpuDevice& gpuDevice)
{
    return gpuDevice.isFormatRenderable (prepassNormalFormat) && gpuDevice.isFormatSupported (prepassDepthFormat) && gpuDevice.isFormatRenderable (prepassDepthFormat);
}

Result DepthNormalPrepass::prepare()
{
    if (prepared)
        return *prepared;

    prepared = [&]
    {
        if (device == nullptr)
            return Result::fail ("No device");

        if (pipelinesFailForTesting)
            return Result::fail ("Unable to compile the depth and normal prepass pipeline: failed for testing");

        if (auto result = geometry.prepare (device); result.failed())
            return result;

        auto bundle = ShaderBundle::loadFromData (depthNormalPrepassShaderBundle, sizeof (depthNormalPrepassShaderBundle));
        if (bundle.failed())
            return Result::fail ("Unable to load the depth and normal prepass shader bundle: " + bundle.getErrorMessage());

        for (size_t state = 0; state < pipelines.size(); ++state)
        {
            GpuPipelineOptions options;
            GeometryBindings::setPipelineGeometry (options, state);

            auto& target = options.colorTargets.emplace_back();
            target.format = prepassNormalFormat;
            target.blendEnabled = false;

            // Strictly in front of the cleared far plane, as in GBufferPass
            options.depthStencil.enabled = true;
            options.depthStencil.format = prepassDepthFormat;
            options.depthStencil.depthCompare = GpuCompareFunction::less;
            options.depthStencil.depthWriteEnabled = true;

            auto pipeline = GpuPipeline::compileFromBundle (device, bundle.getReference(), options);
            if (pipeline.failed())
                return Result::fail ("Unable to compile the depth and normal prepass pipeline: " + pipeline.getErrorMessage());

            pipelines[state] = pipeline.getValue();
        }

        return Result::ok();
    }();

    if (prepared->failed())
        YUP_DBG ("DepthNormalPrepass: " << prepared->getErrorMessage() << ", the frames render without it");

    return *prepared;
}

bool DepthNormalPrepass::render (GpuFrame& frame, const ViewData& view, const SceneDrawList& drawList, FrameResources& resources, int width, int height,
                                 const std::function<void()>& beginScope)
{
    normalRoughness = nullptr;
    depth = nullptr;

    if (prepare().failed() || width <= 0 || height <= 0)
        return false;

    const auto acquire = [&] (GpuTextureFormat format, const char* label)
    {
        GpuTextureDesc desc (static_cast<uint32_t> (width), static_cast<uint32_t> (height), format, true);
        desc.label = label;
        return resources.acquire (desc);
    };

    const auto failTargets = targetFailures > 0;
    if (failTargets)
        --targetFailures;

    auto normals = failTargets ? nullptr : acquire (prepassNormalFormat, "DepthNormalPrepass normal and roughness");
    auto depthTexture = failTargets ? nullptr : acquire (prepassDepthFormat, "DepthNormalPrepass depth");

    // Like the shadow maps, targets the device cannot create leave the frame without the pass
    if (normals == nullptr || depthTexture == nullptr)
    {
        if (! targetsFailed)
            YUP_DBG ("DepthNormalPrepass: unable to create the targets, the frames render without it");

        targetsFailed = true;
        return true;
    }

    targetsFailed = false;

    if (surface == nullptr || surface->asTexture() != normals)
        surface = GpuTarget::createFromTexture (device, normals);

    if (surface == nullptr)
        return false;

    if (beginScope != nullptr)
        beginScope();

    auto pass = surface->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
    if (! pass.isValid())
        return false;

    pass.setDepthStencilAttachment (depthTexture);
    pass.setUniformBuffer (0, 0, &view.getBlock(), sizeof (ViewData::Block));

    GpuPipeline* currentPipeline = nullptr;

    for (const auto& item : drawList.getOpaqueItems())
    {
        const auto& pipeline = pipelines[GeometryBindings::getRasterState (item)];

        if (pipeline.get() != currentPipeline)
        {
            pass.setPipeline (pipeline);
            currentPipeline = pipeline.get();
        }

        geometry.draw (pass, device, item, 0);
    }

    if (! pass.finish())
        return false;

    normalRoughness = std::move (normals);
    depth = std::move (depthTexture);
    return true;
}

void DepthNormalPrepass::releaseTargets() noexcept
{
    surface = nullptr;
    normalRoughness = nullptr;
    depth = nullptr;
}

} // namespace yup
