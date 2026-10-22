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
constexpr uint8_t temporalAAShaderBundle[] = {
#include "yup_renderer_shaders/yup_TemporalAA.inc"
};

// The same with YUP_3D_UPSCALE
constexpr uint8_t temporalAAUpscaleShaderBundle[] = {
#include "yup_renderer_shaders/yup_TemporalAA_upscale.inc"
};

// std140 mirror of TemporalAABlock in yup_TemporalAA.frag
struct TemporalAABlock
{
    float params[4]; // x weight of the current frame, y 1 when the history is valid
};

// std140 mirror of TemporalAABlock with YUP_3D_UPSCALE
struct TemporalAAUpscaleBlock
{
    float params[4];  // as TemporalAABlock
    float upscale[4]; // xy the output size, z the frames the history holds, w 1 while the scene changes
};

} // namespace

//==============================================================================
float TemporalAA::getBlendWeight (int sampleIndex, int sampleCount, bool changing) noexcept
{
    if (sampleIndex >= sampleCount)
        return 0.0f;

    const auto mean = 1.0f / static_cast<float> (jmax (0, sampleIndex) + 1);
    return changing ? jmax (mean, 0.1f) : mean;
}

//==============================================================================
TemporalAA::TemporalAA() = default;

TemporalAA::~TemporalAA() = default;

//==============================================================================
Result TemporalAA::prepare (const GpuDevice::Ptr& device, const FrameResources::Samplers* samplers)
{
    if (samplers == nullptr)
        return Result::fail ("Unable to create the temporal anti-aliasing samplers");

    // One color target for the output, one for the history of the next frame
    return pass.prepare (device, temporalAAShaderBundle, sizeof (temporalAAShaderBundle), GpuTextureFormat::rgba16float, FullscreenPass::Blend::replace, 2);
}

bool TemporalAA::prepareUpscaling (const GpuDevice::Ptr& device)
{
    // Compiled once, remembering a failure instead of retrying every frame. The output and the
    // history as without upscaling, and the weight the history accumulated per pixel
    if (! upscalingPrepared.has_value())
    {
        const GpuTextureFormat formats[] = { GpuTextureFormat::rgba16float, GpuTextureFormat::rgba16float, weightFormat };
        upscalingPrepared = device != nullptr ? upscalePass.prepare (device, temporalAAUpscaleShaderBundle, sizeof (temporalAAUpscaleShaderBundle), formats)
                                              : Result::fail ("No device");

        if (upscalingPrepared->failed())
            YUP_DBG ("TemporalAA: unable to compile the upscaling, the frames are shaded at full size: " << upscalingPrepared->getErrorMessage());
    }

    return upscalingPrepared->wasOk();
}

bool TemporalAA::render (const GpuDevice::Ptr& device, GpuFrame& frame, const ViewData& view, FrameResources& resources,
                         const GpuTexture::Ptr& current, const GpuTexture::Ptr& depth, const GpuTexture::Ptr& velocity,
                         int sampleIndex, int sampleCount, const GpuTexture::Ptr& output, bool changing)
{
    if (device == nullptr || current == nullptr || depth == nullptr || velocity == nullptr || output == nullptr)
        return false;

    jassert (output->getFormat() == GpuTextureFormat::rgba16float);
    if (output->getFormat() != GpuTextureFormat::rgba16float)
        return false;

    const auto* samplers = resources.getSamplers();

    // Compiled once, remembering a failure instead of retrying every frame
    if (! prepared.has_value())
        prepared = prepare (device, samplers);

    if (prepared->failed() || samplers == nullptr)
        return false;

    // A current frame smaller than the output is upscaled into a history at the output size
    if (current->getWidth() != output->getWidth() || current->getHeight() != output->getHeight())
        return renderUpscaled (device, frame, view, resources, current, depth, velocity, sampleIndex, sampleCount, output, changing);

    GpuTextureDesc historyDesc (static_cast<uint32_t> (current->getWidth()), static_cast<uint32_t> (current->getHeight()), GpuTextureFormat::rgba16float, true);
    historyDesc.label = "TemporalAA history";

    const auto history = resources.getHistory (historyName, historyDesc);
    if (history.read == nullptr || history.write == nullptr)
        return false;

    TemporalAABlock block {};
    block.params[0] = getBlendWeight (sampleIndex, sampleCount, changing);
    block.params[1] = history.valid ? 1.0f : 0.0f;

    // Every texel of both targets is written, so they are cleared rather than loaded
    return pass.draw (frame, output, {}, [&] (GpuRenderPass& renderPass)
    {
        renderPass.setColorAttachment (1, history.write, { true, GpuColor::transparentBlack() });

        renderPass.setUniformBuffer (0, 0, &view.getBlock(), sizeof (ViewData::Block));
        renderPass.setSampler (0, 1, samplers->linearClamp);
        renderPass.setSampler (0, 3, samplers->pointClamp);
        renderPass.setUniformBuffer (1, 0, &block, sizeof (block));
        renderPass.setTexture (1, 1, current);
        renderPass.setTexture (1, 2, history.valid ? history.read : current);
        renderPass.setTexture (1, 3, depth);
        renderPass.setTexture (1, 4, velocity);
    }, GpuColor::transparentBlack());
}

bool TemporalAA::renderUpscaled (const GpuDevice::Ptr& device, GpuFrame& frame, const ViewData& view, FrameResources& resources,
                                 const GpuTexture::Ptr& current, const GpuTexture::Ptr& depth, const GpuTexture::Ptr& velocity,
                                 int sampleIndex, int sampleCount, const GpuTexture::Ptr& output, bool changing)
{
    const auto* samplers = resources.getSamplers();
    if (samplers == nullptr || ! prepareUpscaling (device))
        return false;

    const auto width = static_cast<uint32_t> (output->getWidth());
    const auto height = static_cast<uint32_t> (output->getHeight());

    GpuTextureDesc historyDesc (width, height, GpuTextureFormat::rgba16float, true);
    historyDesc.label = "TemporalAA history";

    GpuTextureDesc weightDesc (width, height, weightFormat, true);
    weightDesc.label = "TemporalAA history weight";

    const auto history = resources.getHistory (historyName, historyDesc);
    const auto weight = resources.getHistory (weightHistoryName, weightDesc);
    if (history.read == nullptr || history.write == nullptr || weight.read == nullptr || weight.write == nullptr)
        return false;

    const auto valid = history.valid && weight.valid;

    TemporalAAUpscaleBlock block {};
    block.params[0] = getBlendWeight (sampleIndex, sampleCount, changing);
    block.params[1] = valid ? 1.0f : 0.0f;
    block.upscale[0] = static_cast<float> (width);
    block.upscale[1] = static_cast<float> (height);
    block.upscale[2] = static_cast<float> (jmax (0, sampleIndex));
    block.upscale[3] = changing ? 1.0f : 0.0f;

    // Every texel of the three targets is written, so they are cleared rather than loaded
    return upscalePass.draw (frame, output, {}, [&] (GpuRenderPass& renderPass)
    {
        renderPass.setColorAttachment (1, history.write, { true, GpuColor::transparentBlack() });
        renderPass.setColorAttachment (2, weight.write, { true, GpuColor::transparentBlack() });

        renderPass.setUniformBuffer (0, 0, &view.getBlock(), sizeof (ViewData::Block));
        renderPass.setSampler (0, 1, samplers->linearClamp);
        renderPass.setSampler (0, 3, samplers->pointClamp);
        renderPass.setUniformBuffer (1, 0, &block, sizeof (block));
        renderPass.setTexture (1, 1, current);
        renderPass.setTexture (1, 2, valid ? history.read : current);
        renderPass.setTexture (1, 3, depth);
        renderPass.setTexture (1, 4, velocity);
        renderPass.setTexture (1, 5, valid ? weight.read : current);
    }, GpuColor::transparentBlack());
}

} // namespace yup
