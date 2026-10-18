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
RenderCapabilities::DeviceFacts RenderCapabilities::queryDevice (const GpuDevice& device)
{
    DeviceFacts facts;
    facts.platform = device.getPlatform();
#if YUP_IOS || YUP_ANDROID
    facts.mobile = true;
#elif YUP_EMSCRIPTEN
    const auto userAgent = SystemStats::getDeviceDescription();
    facts.mobile = userAgent.containsIgnoreCase ("Android") || userAgent.containsIgnoreCase ("iPhone") || userAgent.containsIgnoreCase ("iPad");
#endif
    facts.compute = device.isComputeAvailable();
    facts.fragmentStorageBuffers = device.isFragmentStorageBufferAvailable();
    facts.storageTextures = device.isStorageTextureAvailable();
    facts.rgba16floatRenderable = device.isFormatRenderable (GpuTextureFormat::rgba16float);
    facts.rgba8Samples = device.getMaximumSampleCount (GpuTextureFormat::rgba8unorm);
    facts.rgba16floatSamples = device.getMaximumSampleCount (GpuTextureFormat::rgba16float);
    facts.colorAttachments = device.getMaximumColorAttachments();
    facts.depthNormalPrepass = DepthNormalPrepass::isSupported (device);
    return facts;
}

bool RenderCapabilities::passedInteropSpike (GpuPlatform platform) noexcept
{
    // Direct3D flips once its spike passes on the user's devices.
    switch (platform)
    {
        case GpuPlatform::Metal:
        case GpuPlatform::Vulkan:
        case GpuPlatform::WebGPU:
            return true;

        default:
            return false;
    }
}

RenderCapabilities::Tier RenderCapabilities::selectTier (const DeviceFacts& facts, RenderSettings::TierPreference preference) noexcept
{
    if (facts.platform == GpuPlatform::OpenGL || facts.platform == GpuPlatform::OpenGLES)
        return Tier::raster;

    if (preference == RenderSettings::TierPreference::raster)
        return Tier::raster;

    // Clustered lights need compute and storage buffers readable from fragment shaders
    if (! (facts.rgba16floatRenderable && facts.colorAttachments >= 4 && facts.compute && facts.fragmentStorageBuffers
           && passedInteropSpike (facts.platform)))
        return Tier::raster;

    if (preference == RenderSettings::TierPreference::full)
        return Tier::full;

    return facts.mobile ? Tier::raster : Tier::full;
}

RenderCapabilities::RenderCapabilities (const DeviceFacts& deviceFacts, const RenderSettings& settings, uint32_t samples)
    : facts (deviceFacts)
    , requestedSamples (samples)
    , tier (selectTier (deviceFacts, settings.tier))
{
}

bool RenderCapabilities::isAvailable (Feature feature) const noexcept
{
    if (tier == Tier::full)
        return true;

    switch (feature)
    {
        case Feature::temporalAntiAliasing:
        case Feature::contactShadows:
        case Feature::screenSpaceReflections:
        case Feature::screenSpaceGI:
        case Feature::motionBlur:
        case Feature::upscaling:
            return false;

        case Feature::softShadows:
            return true;

        case Feature::hdrSceneColor:
        case Feature::bloom:
            return facts.rgba16floatRenderable && facts.rgba16floatSamples >= requestedSamples;

        case Feature::ambientOcclusion:
            return facts.depthNormalPrepass;

        // The blur and the fog read the scene color through the HDR path and its depth from the prepass
        case Feature::depthOfField:
        case Feature::worldFog:
            return isAvailable (Feature::hdrSceneColor) && facts.depthNormalPrepass;
    }

    return false;
}

bool RenderCapabilities::isEnabled (Feature feature, const RenderSettings& settings) const noexcept
{
    switch (feature)
    {
        case Feature::hdrSceneColor:
            return tier == Tier::full || isEnabled (Feature::bloom, settings) || isEnabled (Feature::depthOfField, settings);

        case Feature::temporalAntiAliasing:
            return isAvailable (feature) && settings.taaSamples > 1;

        case Feature::bloom:
            return isAvailable (feature) && settings.bloom;

        case Feature::softShadows:
            return isAvailable (feature) && settings.softShadows.value_or (tier == Tier::full);

        case Feature::contactShadows:
            return isAvailable (feature) && settings.contactShadows;

        case Feature::ambientOcclusion:
            return isAvailable (feature) && settings.ambientOcclusion.value_or (tier == Tier::full)
                && std::isfinite (settings.aoDistance) && settings.aoDistance > 0.0f;

        case Feature::screenSpaceReflections:
            return isAvailable (feature) && settings.screenSpaceReflections.value_or (tier == Tier::full)
                && isEnabled (Feature::temporalAntiAliasing, settings);

        case Feature::screenSpaceGI:
            return isAvailable (feature) && settings.screenSpaceGI.value_or (false)
                && isEnabled (Feature::ambientOcclusion, settings) && isEnabled (Feature::temporalAntiAliasing, settings);

        case Feature::depthOfField:
            return isAvailable (feature) && settings.depthOfField;

        case Feature::motionBlur:
            return isAvailable (feature) && settings.motionBlur && std::isfinite (settings.motionBlurShutter) && settings.motionBlurShutter > 0.0f;

        // The scene switches the fog on
        case Feature::worldFog:
            return isAvailable (feature);

        // At a scale of 1 the frame is shaded at its size, as without upscaling
        case Feature::upscaling:
            return isAvailable (feature) && settings.upscaling != RenderSettings::Upscaling::off
                && isEnabled (Feature::temporalAntiAliasing, settings) && getClampedRenderScale (settings) < 1.0f;
    }

    return false;
}

float RenderCapabilities::getRenderScale (const RenderSettings& settings) const noexcept
{
    return isEnabled (Feature::upscaling, settings) ? getClampedRenderScale (settings) : 1.0f;
}

float RenderCapabilities::getClampedRenderScale (const RenderSettings& settings) noexcept
{
    return std::isfinite (settings.renderScale) ? jlimit (minRenderScale, 1.0f, settings.renderScale) : 1.0f;
}

int RenderCapabilities::getCascadeCount (const RenderSettings& settings) const noexcept
{
    const auto tierDefault = tier == Tier::full ? ShadowCascades::maxCascades : defaultRasterCascades;
    return jlimit (1, ShadowCascades::maxCascades, settings.sunCascades != 0 ? settings.sunCascades : tierDefault);
}

int RenderCapabilities::getShadowResolution (const RenderSettings& settings) noexcept
{
    return jlimit (minShadowResolution, maxShadowResolution, settings.shadowResolution);
}

} // namespace yup
