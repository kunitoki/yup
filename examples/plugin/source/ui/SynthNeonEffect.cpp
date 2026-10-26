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
#include "SynthNeonEffect.h"

#include <ShaderBundles/SynthBlurShaderBundle.h>
#include <ShaderBundles/SynthGlowPrefilterShaderBundle.h>
#include <ShaderBundles/SynthNeonCompositeShaderBundle.h>

namespace
{
yup::GpuPipeline::Ptr compile (yup::GpuDevice::Ptr device, const void* data, std::size_t size, const char* name)
{
    yup::GpuPipelineOptions pipelineOptions;
    pipelineOptions.colorTargets.emplace_back().blendEnabled = false;

    auto bundle = yup::ShaderBundle::loadFromData (data, size);

    if (bundle.failed())
    {
        yup::Logger::outputDebugString (yup::String ("SynthNeonEffect: ") + name + " bundle failed to load: " + bundle.getErrorMessage());
        return nullptr;
    }

    auto result = yup::GpuPipeline::compileFromBundle (device, bundle.getReference(), pipelineOptions);

    if (result.failed())
    {
        yup::Logger::outputDebugString (yup::String ("SynthNeonEffect: ") + name + " failed to compile: " + result.getErrorMessage());
        return nullptr;
    }

    return result.getValue();
}

bool matches (const yup::GpuTarget::Ptr& target, int width, int height)
{
    return target != nullptr && target->getWidth() == width && target->getHeight() == height;
}
} // namespace

SynthNeonEffect::SynthNeonEffect (Options optionsToUse)
    : options (optionsToUse)
{
}

// The curve maps a point of the screen, in -1 to 1 across the bounds, to the point of the
// flat picture shown there, as the composite shader does. Options never change, so the
// message thread reads them safely while the render thread draws.
std::optional<yup::Point<float>> SynthNeonEffect::displayToContent (yup::Point<float> displayPoint, yup::Rectangle<float> bounds) const
{
    const auto curvature = getCurvature();

    if (curvature <= 0.0f || bounds.isEmpty())
        return displayPoint;

    const auto halfWidth = bounds.getWidth() * 0.5f;
    const auto halfHeight = bounds.getHeight() * 0.5f;
    const auto x = (displayPoint.getX() - bounds.getCenterX()) / halfWidth;
    const auto y = (displayPoint.getY() - bounds.getCenterY()) / halfHeight;

    return yup::Point<float> { bounds.getCenterX() + x * (1.0f + curvature * y * y) * halfWidth,
                               bounds.getCenterY() + y * (1.0f + curvature * x * x) * halfHeight };
}

std::optional<yup::Point<float>> SynthNeonEffect::contentToDisplay (yup::Point<float> contentPoint, yup::Rectangle<float> bounds) const
{
    const auto curvature = getCurvature();

    if (curvature <= 0.0f || bounds.isEmpty())
        return contentPoint;

    const auto halfWidth = bounds.getWidth() * 0.5f;
    const auto halfHeight = bounds.getHeight() * 0.5f;
    const auto targetX = (contentPoint.getX() - bounds.getCenterX()) / halfWidth;
    const auto targetY = (contentPoint.getY() - bounds.getCenterY()) / halfHeight;

    // The curve has no closed form inverse; a few fixed point steps converge for a gentle one.
    auto x = targetX;
    auto y = targetY;

    for (int step = 0; step < 6; ++step)
    {
        const auto nextX = targetX / (1.0f + curvature * y * y);
        const auto nextY = targetY / (1.0f + curvature * x * x);
        x = nextX;
        y = nextY;
    }

    return yup::Point<float> { bounds.getCenterX() + x * halfWidth, bounds.getCenterY() + y * halfHeight };
}

bool SynthNeonEffect::ensurePipelines (yup::GraphicsContext& context)
{
    const auto device = context.getGpuDevice();

    // A device replacing a lost one needs pipelines and targets of its own.
    if (device != pipelineDevice)
    {
        pipelineDevice = device;
        prefilter = blur = composite = nullptr;
        smallA = smallB = output = nullptr;
        compileAttempted = false;
    }

    if (device == nullptr || device->isDeviceLost())
        return false;

    if (prefilter != nullptr && blur != nullptr && composite != nullptr)
        return true;

    if (compileAttempted || ! context.isGpuAvailable())
        return false;

    compileAttempted = true;

    prefilter = compile (device, retrace::synthGlowPrefilterShader_data, retrace::synthGlowPrefilterShader_size, "glow prefilter");
    blur = compile (device, retrace::synthBlurShader_data, retrace::synthBlurShader_size, "blur");
    composite = compile (device, retrace::synthNeonCompositeShader_data, retrace::synthNeonCompositeShader_size, "composite");

    return prefilter != nullptr && blur != nullptr && composite != nullptr;
}

bool SynthNeonEffect::ensureTargets (yup::GpuDevice::Ptr device, int width, int height, int smallWidth, int smallHeight)
{
    for (auto* target : { &smallA, &smallB })
        if (! matches (*target, smallWidth, smallHeight))
            *target = yup::GpuTarget::create (device, smallWidth, smallHeight);

    if (! matches (output, width, height))
        output = yup::GpuTarget::create (device, width, height);

    return smallA != nullptr && smallB != nullptr && output != nullptr;
}

void SynthNeonEffect::apply (yup::Graphics& g, yup::GpuTexture::Ptr input, yup::Rectangle<float> bounds)
{
    if (input == nullptr || ! input->isValid())
        return;

    auto& context = g.getGraphicsContext();
    const auto width = input->getWidth();
    const auto height = input->getHeight();
    const auto pixelScale = bounds.getWidth() > 0.0f ? static_cast<float> (width) / bounds.getWidth() : 1.0f;
    const auto downsample = yup::jmax (1, yup::roundToInt (downsamplePerPoint * pixelScale));
    const auto smallWidth = yup::jmax (1, width / downsample);
    const auto smallHeight = yup::jmax (1, height / downsample);

    if (! ensurePipelines (context) || ! ensureTargets (context.getGpuDevice(), width, height, smallWidth, smallHeight))
    {
        g.drawTexture (input, bounds);
        return;
    }

    auto frame = yup::GpuFrame::begin (context.getGpuDevice());

    if (! frame.isValid())
    {
        g.drawTexture (input, bounds);
        return;
    }

    const auto run = [&] (yup::GpuTarget& target, const yup::GpuPipeline::Ptr& pipeline, const yup::GpuTexture::Ptr& source, const void* params, std::size_t size)
    {
        auto pass = target.beginRenderPass (frame, { true, yup::Colors::black });
        pass.setPipeline (pipeline);
        pass.setTexture (0, 0, source);
        pass.setUniformBuffer (0, 2, params, size);
        pass.draw (3);
        pass.finish();
        return target.asTexture();
    };

    const auto w = static_cast<float> (smallWidth);
    const auto h = static_cast<float> (smallHeight);
    const auto radius = std::ceil (glowSigma * 3.0f);
    const float prefilterParams[] { glowThreshold, glowKnee, w, h, whiteGlow, 0.0f, 0.0f, 0.0f };
    const float horizontal[] { glowSigma, radius, w, h, 1.0f, 0.0f, 0.0f, 0.0f };
    const float vertical[] { glowSigma, radius, w, h, 0.0f, 1.0f, 0.0f, 0.0f };

    // Without glow the composite adds none, so it reads the input in its place.
    auto glow = input;

    if (options.glow)
    {
        glow = run (*smallA, prefilter, input, prefilterParams, sizeof (prefilterParams));
        glow = run (*smallB, blur, glow, horizontal, sizeof (horizontal));
        glow = run (*smallA, blur, glow, vertical, sizeof (vertical));
    }

    const auto amount = [] (bool enabled, float value) { return enabled ? value : 0.0f; };
    const float compositeParams[] { static_cast<float> (width),
                                    static_cast<float> (height),
                                    amount (options.aberration, aberrationAmount * pixelScale),
                                    amount (options.glow, glowAmount),
                                    amount (options.grade, gradeAmount),
                                    amount (options.scanlines, scanlineAmount),
                                    amount (options.vignette, vignetteAmount),
                                    amount (options.grain, grainAmount),
                                    getCurvature(),
                                    amount (options.phosphor, phosphorAmount),
                                    amount (options.curvature, cornerRadius),
                                    pixelScale };

    auto pass = output->beginRenderPass (frame, { true, yup::Colors::black });
    pass.setPipeline (composite);
    pass.setUniformBuffer (0, 0, compositeParams, sizeof (compositeParams));
    pass.setTexture (0, 1, input);
    pass.setTexture (0, 3, glow);
    pass.draw (3);
    pass.finish();
    frame.submit();

    g.drawTexture (output->asTexture(), bounds);
}
