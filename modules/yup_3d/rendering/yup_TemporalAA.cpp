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

// Precompiled bundle (do not edit the .inc by hand). Regenerate it after changing
// shaders/yup_TemporalAA.frag with: just yup3d_shaders
constexpr uint8_t temporalAAShaderBundle[] = {
#include "../shaders/yup_TemporalAA.inc"
};

// std140 mirror of TemporalAABlock in yup_TemporalAA.frag
struct TemporalAABlock
{
    float params[4]; // x weight of the current frame, y 1 when the history is valid
};

// The weight of the current frame: a running average up to 10 frames, then an exponential one,
// and 0 once sampleCount frames have accumulated
float getTemporalBlendWeight (int sampleIndex, int sampleCount) noexcept
{
    if (sampleIndex >= sampleCount)
        return 0.0f;

    return jmax (1.0f / static_cast<float> (jmax (0, sampleIndex) + 1), 0.1f);
}

} // namespace

//==============================================================================
TemporalAA::TemporalAA() = default;

TemporalAA::~TemporalAA() = default;

//==============================================================================
Result TemporalAA::prepare (const GpuDevice::Ptr& device, const FrameResources::Samplers* samplers)
{
    if (samplers == nullptr)
        return Result::fail ("Unable to create the temporal anti-aliasing samplers");

    // One color target for the output, one for the history of the next frame
    return pass.prepare (device, temporalAAShaderBundle, sizeof (temporalAAShaderBundle), GpuTextureFormat::rgba16float, false, 2);
}

bool TemporalAA::render (const GpuDevice::Ptr& device, GpuFrame& frame, const ViewData& view, FrameResources& resources,
                         const GpuTexture::Ptr& current, const GpuTexture::Ptr& depth, const GpuTexture::Ptr& velocity,
                         int sampleIndex, int sampleCount, const GpuTexture::Ptr& output)
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

    GpuTextureDesc historyDesc (static_cast<uint32_t> (current->getWidth()), static_cast<uint32_t> (current->getHeight()), GpuTextureFormat::rgba16float, true);
    historyDesc.label = "TemporalAA history";

    const auto history = resources.getHistory (historyName, historyDesc);
    if (history.read == nullptr || history.write == nullptr)
        return false;

    TemporalAABlock block {};
    block.params[0] = getTemporalBlendWeight (sampleIndex, sampleCount);
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

} // namespace yup
