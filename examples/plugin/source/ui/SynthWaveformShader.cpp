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

#include "SynthWaveformShader.h"

#include <ShaderBundles/SynthWaveformShaderBundle.h>

float SynthWaveformShader::sampleForY (float y, float height) noexcept
{
    return height > 0.0f ? (height * 0.5f - y) / (height * ridgeScale) : 0.0f;
}

yup::GpuTexture::Ptr SynthWaveformShader::render (yup::GraphicsContext& context,
                                                  yup::GpuTarget::Ptr& target,
                                                  int width,
                                                  int height,
                                                  const std::vector<float>& samples,
                                                  float time,
                                                  const Look& look)
{
    jassert (samples.size() == static_cast<std::size_t> (sampleCount));

    if (width < 2 || height < 2 || ! ensurePipeline (context))
        return nullptr;

    if (target == nullptr || target->getWidth() != width || target->getHeight() != height)
        target = yup::GpuTarget::create (device, width, height);

    if (target == nullptr)
        return nullptr;

    const auto& accent = look.accent;
    const auto& ground = look.ground;

    const Params params {
        time,
        static_cast<float> (width),
        static_cast<float> (height),
        look.isLight ? 1.0f : 0.0f,
        { accent.getRedFloat(), accent.getGreenFloat(), accent.getBlueFloat(), 1.0f },
        { ground.getRedFloat(), ground.getGreenFloat(), ground.getBlueFloat(), 1.0f },
        look.scrolls ? 1.0f : 0.0f,
        look.playhead,
        look.trail,
        0.0f
    };

    auto frame = yup::GpuFrame::begin (device);
    if (! frame.isValid())
        return nullptr;

    auto pass = target->beginRenderPass (frame, { true, look.ground });
    if (! pass.isValid())
        return nullptr;

    pass.setPipeline (pipeline);
    pass.setUniformBuffer (0, 0, &params, sizeof (params));
    pass.setUniformBuffer (0, 1, samples.data(), samples.size() * sizeof (float));

    if (! pass.draw (3) || ! pass.finish() || ! frame.submit())
        return nullptr;

    return target->asTexture();
}

bool SynthWaveformShader::ensurePipeline (yup::GraphicsContext& context)
{
    if (pipeline != nullptr)
        return true;

    if (compileAttempted || ! context.isGpuAvailable())
        return false;

    compileAttempted = true;
    device = context.getGpuDevice();

    yup::GpuPipelineOptions options;
    options.colorTargets.emplace_back().blendEnabled = false;

    auto bundle = yup::ShaderBundle::loadFromData (retrace::synthWaveformShader_data, retrace::synthWaveformShader_size);
    if (bundle.failed())
    {
        yup::Logger::outputDebugString ("SynthWaveformShader: shader bundle failed to load: " + bundle.getErrorMessage());
        return false;
    }

    auto result = yup::GpuPipeline::compileFromBundle (device, bundle.getReference(), options);
    if (result.failed())
    {
        yup::Logger::outputDebugString ("SynthWaveformShader: shader compile failed: " + result.getErrorMessage());
        return false;
    }

    pipeline = result.getValue();
    return true;
}
