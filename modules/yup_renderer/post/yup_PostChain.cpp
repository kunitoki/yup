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

//==============================================================================
PostChain::PostChain() = default;

PostChain::~PostChain() = default;

//==============================================================================
bool PostChain::willBlur (const RenderCapabilities& caps, const RenderSettings& settings, const CameraNode* camera) const noexcept
{
    return caps.isEnabled (RenderCapabilities::Feature::depthOfField, settings)
        && DepthOfField::canBlur (camera, settings)
        && (depthOfField == nullptr || ! depthOfField->hasGivenUp());
}

bool PostChain::willMotionBlur (const RenderCapabilities& caps, const RenderSettings& settings) const noexcept
{
    return caps.isEnabled (RenderCapabilities::Feature::motionBlur, settings)
        && (motionBlur == nullptr || ! motionBlur->hasGivenUp());
}

void PostChain::releaseTargets() noexcept
{
    temporalAA.releaseTargets();

    if (depthOfField != nullptr)
        depthOfField->releaseTargets();

    if (motionBlur != nullptr)
        motionBlur->releaseTargets();

    bloom.releaseTargets();
    toneMapping.releaseTargets();
    resolvedColor = nullptr;
}

bool PostChain::render (const GpuDevice::Ptr& device, GpuFrame& frame, const RenderCapabilities& caps,
                        const RenderSettings& settings, const ViewData& view, FrameResources& resources,
                        const Inputs& inputs, const GpuTexture::Ptr& output)
{
    resolvedColor = inputs.color;
    if (inputs.color == nullptr || output == nullptr)
        return false;

    const auto beginScope = [&inputs] (const char* name)
    {
        if (inputs.beginScope != nullptr)
            inputs.beginScope (name);
    };

    // A skipped effect releases its targets, which would keep its last textures alive
    if (! caps.isEnabled (RenderCapabilities::Feature::temporalAntiAliasing, settings))
        temporalAA.releaseTargets();

    if (! caps.isEnabled (RenderCapabilities::Feature::bloom, settings))
        bloom.releaseTargets();

    if (caps.isEnabled (RenderCapabilities::Feature::temporalAntiAliasing, settings))
    {
        YUP_PROFILE_NAMED_INTERNAL_TRACE (temporalAA);
        beginScope ("temporalAA");

        // At the output's size: a color shaded smaller is upscaled
        GpuTextureDesc desc (static_cast<uint32_t> (output->getWidth()), static_cast<uint32_t> (output->getHeight()), GpuTextureFormat::rgba16float, true);
        desc.label = "PostChain temporal anti-aliasing";

        const auto sampleCount = inputs.temporalSampleCount > 0 ? inputs.temporalSampleCount : settings.taaSamples;

        auto resolved = resources.acquire (desc);
        if (resolved == nullptr
            || ! temporalAA.render (device, frame, view, resources, inputs.color, inputs.depth, inputs.velocity,
                                    inputs.temporalSampleIndex, sampleCount, resolved, inputs.temporalChanging))
            return false;

        resolvedColor = resolved;
    }

    // The depth of field is created by the first frame blurring. A failure leaves the frame unblurred.
    // blurAmount is the share of each pixel's coverage a blur moved, which tone mapping composites
    // in radiance (preflight R20, R21, R22, R24)
    auto color = resolvedColor;
    auto colorIsReversible = inputs.colorIsReversible;
    GpuTexture::Ptr blurAmount;

    if (inputs.blurring)
    {
        YUP_PROFILE_NAMED_INTERNAL_TRACE (depthOfField);

        if (depthOfField == nullptr)
            depthOfField = std::make_unique<DepthOfField> (device);

        if (depthOfFieldFailures > 0)
            depthOfField->failRendersForTesting (std::exchange (depthOfFieldFailures, 0));

        // A frame missing its depth records nothing, so it opens no scope
        if (inputs.depth != nullptr)
            beginScope ("depthOfField");

        const DepthOfField::Inputs dofInputs { color, colorIsReversible, inputs.depth, inputs.camera };
        if (auto blurred = depthOfField->render (frame, view, settings, resources, dofInputs))
        {
            // The blur writes linear color
            color = std::move (blurred);
            colorIsReversible = false;
            blurAmount = depthOfField->getBlurAmount();
        }
    }

    // A frame without the blur, skipped or failed, releases its targets and its last outputs
    if (depthOfField != nullptr && blurAmount == nullptr)
        depthOfField->releaseTargets();

    // The motion blur is created by the first moving frame it smears, and carries the depth of
    // field's share along. A still frame records nothing; a failure leaves the frame unsmeared
    auto smeared = false;

    if (inputs.moving && willMotionBlur (caps, settings))
    {
        YUP_PROFILE_NAMED_INTERNAL_TRACE (motionBlur);
        beginScope ("motionBlur");

        if (motionBlur == nullptr)
            motionBlur = std::make_unique<MotionBlur> (device);

        if (motionBlurFailures > 0)
            motionBlur->failRendersForTesting (std::exchange (motionBlurFailures, 0));

        const MotionBlur::Inputs motionInputs { color, inputs.depth, inputs.velocity, blurAmount };
        if (auto result = motionBlur->render (frame, view, settings, resources, motionInputs))
        {
            color = std::move (result);
            blurAmount = motionBlur->getBlurAmount();
            smeared = true;
        }
    }

    // A still frame keeps the targets for the next move; a failure, a disabled blur or giving up releases them
    if (motionBlur != nullptr && ! smeared)
    {
        if (! inputs.moving && willMotionBlur (caps, settings) && ! motionBlur->hasGivenUp())
            motionBlur->keepTargets (resources, color);
        else
            motionBlur->releaseTargets();
    }

    GpuTexture::Ptr glow;

    if (caps.isEnabled (RenderCapabilities::Feature::bloom, settings))
    {
        YUP_PROFILE_NAMED_INTERNAL_TRACE (bloom);
        beginScope ("bloom");

        glow = bloom.render (device, frame, resources, settings, color, colorIsReversible);
        if (glow == nullptr)
            return false;
    }

    {
        YUP_PROFILE_NAMED_INTERNAL_TRACE (toneMapping);
        beginScope ("toneMapping");

        // The blurs spread light over partial coverage, which tone mapping then composites in radiance
        // where they wrote (preflight R20, R21, R22)
        if (! toneMapping.render (device, frame, view, resources, color, colorIsReversible, glow, output, inputs.background, blurAmount))
            return false;
    }

    // Targets of textures this frame no longer drew into, like the pool's after a resize, go now
    temporalAA.trimUnusedTargets();

    if (depthOfField != nullptr)
        depthOfField->trimUnusedTargets();

    if (motionBlur != nullptr)
        motionBlur->trimUnusedTargets();

    bloom.trimUnusedTargets();
    toneMapping.trimUnusedTargets();
    return true;
}

} // namespace yup
