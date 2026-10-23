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
#include "SynthBloomEffect.h"

#include <ShaderBundles/SynthBloomBlurShaderBundle.h>
#include <ShaderBundles/SynthBloomPrefilterShaderBundle.h>

namespace
{
yup::GpuPipeline::Ptr compile (yup::GpuDevice::Ptr device, const void* data, std::size_t size, const char* name)
{
    yup::GpuPipelineOptions options;
    options.colorTargets.emplace_back().blendEnabled = false;

    auto bundle = yup::ShaderBundle::loadFromData (data, size);

    if (bundle.failed())
    {
        yup::Logger::outputDebugString (yup::String ("SynthBloomEffect: ") + name + " bundle failed to load: " + bundle.getErrorMessage());
        return nullptr;
    }

    auto result = yup::GpuPipeline::compileFromBundle (device, bundle.getReference(), options);

    if (result.failed())
    {
        yup::Logger::outputDebugString (yup::String ("SynthBloomEffect: ") + name + " failed to compile: " + result.getErrorMessage());
        return nullptr;
    }

    return result.getValue();
}
} // namespace

bool SynthBloomEffect::ensurePipelines (yup::GraphicsContext& context)
{
    if (prefilter != nullptr && blur != nullptr)
        return true;

    if (compileAttempted || ! context.isGpuAvailable())
        return false;

    compileAttempted = true;

    const auto device = context.getGpuDevice();
    prefilter = compile (device, prism::synthBloomPrefilterShader_data, prism::synthBloomPrefilterShader_size, "prefilter");
    blur = compile (device, prism::synthBloomBlurShader_data, prism::synthBloomBlurShader_size, "blur");

    return prefilter != nullptr && blur != nullptr;
}

bool SynthBloomEffect::ensureTargets (yup::GpuDevice::Ptr device, int width, int height)
{
    for (auto* target : { &targetA, &targetB })
        if (*target == nullptr || (*target)->getWidth() != width || (*target)->getHeight() != height)
            *target = yup::GpuTarget::create (device, width, height);

    return targetA != nullptr && targetB != nullptr;
}

void SynthBloomEffect::apply (yup::Graphics& g, yup::GpuTexture::Ptr input, yup::Rectangle<float> bounds)
{
    if (input == nullptr || ! input->isValid())
        return;

    g.drawTexture (input, bounds);

    auto& context = g.getGraphicsContext();
    const auto width = yup::jmax (1, input->getWidth() / downsample);
    const auto height = yup::jmax (1, input->getHeight() / downsample);

    if (! ensurePipelines (context) || ! ensureTargets (context.getGpuDevice(), width, height))
        return;

    auto frame = yup::GpuFrame::begin (context.getGpuDevice());

    if (! frame.isValid())
        return;

    const auto run = [&] (yup::GpuTarget& target, const yup::GpuPipeline::Ptr& pipeline, const yup::GpuTexture::Ptr& source, const void* params, std::size_t size)
    {
        auto pass = target.beginRenderPass (frame, { true, yup::Colors::transparentBlack });
        pass.setPipeline (pipeline);
        pass.setTexture (0, 0, source);
        pass.setUniformBuffer (0, 2, params, size);
        pass.draw (3);
        pass.finish();
        return target.asTexture();
    };

    const auto w = static_cast<float> (width);
    const auto h = static_cast<float> (height);
    const float prefilterParams[] { threshold, knee, w, h, whiteGlow, 0.0f, 0.0f, 0.0f };
    const auto radius = std::ceil (sigma * 3.0f);
    const float horizontal[] { sigma, radius, w, h, 1.0f, 0.0f, 0.0f, 0.0f };
    const float vertical[] { sigma, radius, w, h, 0.0f, 1.0f, 0.0f, 0.0f };

    auto bloom = run (*targetA, prefilter, input, prefilterParams, sizeof (prefilterParams));
    bloom = run (*targetB, blur, bloom, horizontal, sizeof (horizontal));
    bloom = run (*targetA, blur, bloom, vertical, sizeof (vertical));
    frame.submit();

    if (bloom == nullptr)
        return;

    const auto state = g.saveState();
    g.setBlendMode (yup::BlendMode::Additive);
    g.setOpacity (intensity);
    g.drawTexture (bloom, bounds);
}
