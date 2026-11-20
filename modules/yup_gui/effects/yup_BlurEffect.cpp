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

namespace detail
{

constexpr uint8 kComponentBlurShaderBundle[] = {
#include "yup_gui_shaders/yup_ComponentBlurEffect.inc"
};

struct BlurPassUniforms
{
    float sigma;
    float radius;
    float invWidth;
    float invHeight;
    float dirX;
    float dirY;
    float alphaSilhouette;
    float reserved;
};

inline bool ensureComponentEffectPipeline (GraphicsContext& context,
                                           GpuDevice::Ptr& pipelineDevice,
                                           GpuPipeline::Ptr& pipeline,
                                           GpuTarget::Ptr* targetA,
                                           GpuTarget::Ptr* targetB,
                                           bool& compileAttempted,
                                           const void* bundleData,
                                           size_t bundleSize)
{
    const auto device = context.getGpuDevice();

    if (device != pipelineDevice)
    {
        pipelineDevice = device;
        pipeline = nullptr;
        if (targetA != nullptr)
            *targetA = nullptr;
        if (targetB != nullptr)
            *targetB = nullptr;
        compileAttempted = false;
    }

    if (device == nullptr || device->isDeviceLost())
        return false;

    if (pipeline != nullptr)
        return true;

    if (compileAttempted || ! context.isGpuAvailable())
        return false;

    compileAttempted = true;

    auto bundle = ShaderBundle::loadFromData (bundleData, bundleSize);
    if (bundle.failed())
        return false;

    GpuPipelineOptions options;
    options.colorTargets.emplace_back().blendEnabled = false;

    auto compiled = GpuPipeline::compileFromBundle (device, bundle.getReference(), options);
    if (compiled.failed())
        return false;

    pipeline = compiled.getValue();
    return pipeline != nullptr;
}

inline bool ensureComponentEffectTarget (const GpuDevice::Ptr& device, GpuTarget::Ptr& target, int width, int height)
{
    if (target == nullptr || target->getWidth() != width || target->getHeight() != height)
        target = GpuTarget::create (device, width, height);

    return target != nullptr;
}

inline GpuTexture::Ptr runSeparableBlur (GraphicsContext& context,
                                         GpuDevice::Ptr& pipelineDevice,
                                         GpuPipeline::Ptr& pipeline,
                                         GpuTarget::Ptr& targetA,
                                         GpuTarget::Ptr& targetB,
                                         bool& compileAttempted,
                                         const GpuTexture::Ptr& input,
                                         float sigmaPixels,
                                         bool alphaSilhouette)
{
    const int width = input->getWidth();
    const int height = input->getHeight();
    if (width <= 0 || height <= 0)
        return nullptr;

    if (! ensureComponentEffectPipeline (context,
                                         pipelineDevice,
                                         pipeline,
                                         &targetA,
                                         &targetB,
                                         compileAttempted,
                                         kComponentBlurShaderBundle,
                                         sizeof (kComponentBlurShaderBundle)))
    {
        return nullptr;
    }

    const auto device = context.getGpuDevice();
    if (! ensureComponentEffectTarget (device, targetA, width, height)
        || ! ensureComponentEffectTarget (device, targetB, width, height))
    {
        return nullptr;
    }

    auto frame = GpuFrame::begin (device);
    if (! frame.isValid())
        return nullptr;

    const float clampedSigma = jmax (0.0f, sigmaPixels);
    const float kernelRadius = std::ceil (clampedSigma * 3.0f);
    const float invW = 1.0f / static_cast<float> (width);
    const float invH = 1.0f / static_cast<float> (height);

    const auto runPass = [&] (GpuTarget& target, const GpuTexture::Ptr& source, float dx, float dy, float silhouette)
    {
        const BlurPassUniforms uniforms { clampedSigma, kernelRadius, invW, invH, dx, dy, silhouette, 0.0f };
        auto pass = target.beginRenderPass (frame, { true, Colors::transparentBlack });
        pass.setPipeline (pipeline);
        pass.setTexture (0, 0, source);
        pass.setUniformBuffer (0, 2, &uniforms, sizeof (uniforms));
        pass.draw (3);
        pass.finish();
        return target.asTexture();
    };

    auto horizontal = runPass (*targetA, input, 1.0f, 0.0f, alphaSilhouette ? 1.0f : 0.0f);
    auto vertical = runPass (*targetB, horizontal, 0.0f, 1.0f, 0.0f);
    frame.submit();
    return vertical;
}

} // namespace detail

//==============================================================================

BlurEffect::BlurEffect (float radiusPoints) noexcept
    : radius (jmax (0.0f, radiusPoints))
{
}

BlurEffect::~BlurEffect() = default;

void BlurEffect::setRadius (float radiusPoints) noexcept
{
    radius.store (jmax (0.0f, radiusPoints), std::memory_order_relaxed);
}

float BlurEffect::getRadius() const noexcept
{
    return radius.load (std::memory_order_relaxed);
}

void BlurEffect::apply (Graphics& g, GpuTexture::Ptr inputTexture, Rectangle<float> bounds)
{
    if (inputTexture == nullptr || ! inputTexture->isValid() || bounds.isEmpty())
        return;

    const float radiusPoints = getRadius();
    if (radiusPoints <= 0.0001f)
    {
        g.drawTexture (inputTexture, bounds);
        return;
    }

    const float pixelScale = static_cast<float> (inputTexture->getWidth()) / bounds.getWidth();
    if (auto blurred = detail::runSeparableBlur (g.getGraphicsContext(),
                                                 pipelineDevice,
                                                 pipeline,
                                                 targetA,
                                                 targetB,
                                                 compileAttempted,
                                                 inputTexture,
                                                 radiusPoints * pixelScale,
                                                 false))
    {
        g.drawTexture (blurred, bounds);
    }
    else
    {
        g.drawTexture (inputTexture, bounds);
    }
}

} // namespace yup
