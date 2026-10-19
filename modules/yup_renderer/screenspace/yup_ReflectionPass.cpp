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

constexpr uint8_t ssrTraceShaderBundle[] = {
#include "yup_renderer_shaders/yup_SsrTrace.inc"
};
constexpr uint8_t ssrResolveShaderBundle[] = {
#include "yup_renderer_shaders/yup_SsrResolve.inc"
};
constexpr uint8_t ssrDenoiseShaderBundle[] = {
#include "yup_renderer_shaders/yup_SsrDenoise.inc"
};

// The hit uv needs more than a half float: near uv 1 its step is a pixel and a half at 3456 wide
constexpr auto traceFormat = GpuTextureFormat::rgba32float;
constexpr auto reflectionsFormat = GpuTextureFormat::rgba16float;

// The hits behind a surface by less than this fraction of its view depth count, the farther ones pass behind it
constexpr float hitThickness = 0.05f;

// std140 mirror of SsrBlock in lib/ssr.glsl
struct SsrBlock
{
    float params[4]; // roughness fade start, roughness cut, hit thickness, cells marched at most
    float frame[4];  // 1 / previous exposure, Hi-Z mip levels, screen edge fade, history valid
};

static_assert (sizeof (SsrBlock) == 32);

} // namespace

//==============================================================================
ReflectionPass::ReflectionPass (GpuDevice::Ptr deviceToUse)
    : device (std::move (deviceToUse))
{
}

ReflectionPass::~ReflectionPass() = default;

//==============================================================================
Result ReflectionPass::prepare()
{
    if (prepared)
        return *prepared;

    prepared = [&]
    {
        if (device == nullptr)
            return Result::fail ("No device");

        if (failPipelines)
            return Result::fail ("The pipelines failed for testing");

        if (! device->isFormatRenderable (traceFormat))
            return Result::fail ("rgba32float is not renderable");

        if (auto result = trace.prepare (device, ssrTraceShaderBundle, sizeof (ssrTraceShaderBundle), traceFormat); result.failed())
            return Result::fail ("Unable to compile the trace: " + result.getErrorMessage());

        if (auto result = resolve.prepare (device, ssrResolveShaderBundle, sizeof (ssrResolveShaderBundle), reflectionsFormat); result.failed())
            return Result::fail ("Unable to compile the resolve: " + result.getErrorMessage());

        if (auto result = denoise.prepare (device, ssrDenoiseShaderBundle, sizeof (ssrDenoiseShaderBundle), reflectionsFormat); result.failed())
            return Result::fail ("Unable to compile the denoise: " + result.getErrorMessage());

        return Result::ok();
    }();

    return *prepared;
}

GpuTexture::Ptr ReflectionPass::render (GpuFrame& frame, const ViewData& view, const GpuTexture::Ptr& hiZ,
                                        const GpuTexture::Ptr& normalRoughness, const GpuTexture::Ptr& velocity,
                                        const GpuTexture::Ptr& previousSceneColor, float previousExposure, FrameResources& resources)
{
    String failure;
    auto hiZFailed = false;
    auto output = [&]() -> GpuTexture::Ptr
    {
        if (failRenders)
        {
            failure = "the renders failed for testing";
            return nullptr;
        }

        // First, since no pyramid is built without the pipelines
        if (auto result = prepare(); result.failed())
        {
            failure = result.getErrorMessage();
            return nullptr;
        }

        // HiZPyramid logged why it could not build the pyramid
        if (hiZ == nullptr)
        {
            failure = "the Hi-Z pyramid could not be built";
            hiZFailed = true;
            return nullptr;
        }

        const auto* samplers = resources.getSamplers();
        if (normalRoughness == nullptr || velocity == nullptr || previousSceneColor == nullptr || samplers == nullptr
            || ! (previousExposure > 0.0f))
        {
            failure = "an input is missing";
            return nullptr;
        }

        GpuTextureDesc traceDesc ((hiZ->getWidth() + 1) / 2, (hiZ->getHeight() + 1) / 2, traceFormat, true);
        traceDesc.label = "ReflectionPass trace";
        const auto traced = resources.acquire (traceDesc);

        GpuTextureDesc outputDesc (hiZ->getWidth(), hiZ->getHeight(), reflectionsFormat, true);
        outputDesc.label = "ReflectionPass history";
        const auto history = resources.getHistory ("ssr", outputDesc);
        outputDesc.label = "ReflectionPass output";
        auto reflections = resources.acquire (outputDesc);

        if (traced == nullptr || reflections == nullptr || history.read == nullptr || history.write == nullptr)
        {
            failure = "unable to create the targets";
            return nullptr;
        }

        SsrBlock block {};
        block.params[0] = fadeStart;
        block.params[1] = roughnessCut;
        block.params[2] = hitThickness;
        block.params[3] = static_cast<float> (maxIterations);
        block.frame[0] = 1.0f / previousExposure;
        block.frame[1] = static_cast<float> (hiZ->getMipLevels());
        block.frame[2] = screenEdgeFade;
        block.frame[3] = history.valid ? 1.0f : 0.0f;

        const auto bindInputs = [&] (GpuRenderPass& pass)
        {
            pass.setUniformBuffer (0, 0, &view.getBlock(), sizeof (ViewData::Block));
            pass.setSampler (0, 3, samplers->pointClamp);
            pass.setUniformBuffer (1, 0, &block, sizeof (block));
            pass.setTexture (1, 1, hiZ);
            pass.setTexture (1, 2, normalRoughness);
        };

        // Every draw writes every texel, so the targets need no clear
        const auto drawn = trace.draw (frame, traced, {}, bindInputs)
                        && resolve.draw (frame, history.write, {}, [&] (GpuRenderPass& pass)
                           {
                               bindInputs (pass);
                               pass.setSampler (0, 1, samplers->linearClamp);
                               pass.setTexture (1, 3, traced);
                               pass.setTexture (1, 4, velocity);
                               pass.setTexture (1, 5, previousSceneColor);
                               pass.setTexture (1, 6, history.read);
                           })
                        && denoise.draw (frame, reflections, {}, [&] (GpuRenderPass& pass)
                           {
                               bindInputs (pass);
                               pass.setTexture (1, 3, history.write);
                           });

        if (! drawn)
        {
            failure = "unable to record a draw";
            return nullptr;
        }

        return reflections;
    }();

    if (output != nullptr)
    {
        consecutiveFailures = 0;
        return output;
    }

    // A failure leaves the frame without the reflections; the second in a row gives up for good
    if (++consecutiveFailures >= 2 && ! std::exchange (givenUp, true))
    {
        giveUpReason = failure;

        if (! hiZFailed)
            YUP_DBG ("ReflectionPass: " << failure << ", the frames render without screen-space reflections");
    }

    return nullptr;
}

void ReflectionPass::trimUnusedTargets()
{
    trace.trimUnusedTargets();
    resolve.trimUnusedTargets();
    denoise.trimUnusedTargets();
}

void ReflectionPass::releaseTargets() noexcept
{
    trace.releaseTargets();
    resolve.releaseTargets();
    denoise.releaseTargets();
}

} // namespace yup
