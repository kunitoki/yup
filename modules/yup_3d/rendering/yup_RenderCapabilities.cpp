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
#endif
    facts.compute = device.isComputeAvailable();
    facts.fragmentStorageBuffers = device.isFragmentStorageBufferAvailable();
    facts.storageTextures = device.isStorageTextureAvailable();
    facts.rgba16floatRenderable = device.isFormatRenderable (GpuTextureFormat::rgba16float);
    facts.rgba8Samples = device.getMaximumSampleCount (GpuTextureFormat::rgba8unorm);
    facts.rgba16floatSamples = device.getMaximumSampleCount (GpuTextureFormat::rgba16float);
    facts.colorAttachments = device.getMaximumColorAttachments();
    return facts;
}

bool RenderCapabilities::passedInteropSpike (GpuPlatform platform) noexcept
{
    // Direct3D, Vulkan and WebGPU flip once their spikes pass on the user's devices.
    switch (platform)
    {
        case GpuPlatform::Metal:
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
            return false;

        case Feature::softShadows:
            return true;

        case Feature::hdrSceneColor:
        case Feature::bloom:
            return facts.rgba16floatRenderable && facts.rgba16floatSamples >= requestedSamples;
    }

    return false;
}

bool RenderCapabilities::isEnabled (Feature feature, const RenderSettings& settings) const noexcept
{
    switch (feature)
    {
        case Feature::hdrSceneColor:
            return tier == Tier::full || isEnabled (Feature::bloom, settings);

        case Feature::temporalAntiAliasing:
            return isAvailable (feature) && settings.taaSamples > 1;

        case Feature::bloom:
            return isAvailable (feature) && settings.bloom;

        case Feature::softShadows:
            return isAvailable (feature) && settings.softShadows.value_or (tier == Tier::full);

        case Feature::contactShadows:
            return isAvailable (feature) && settings.contactShadows;
    }

    return false;
}

int RenderCapabilities::getCascadeCount (const RenderSettings& settings) const noexcept
{
    constexpr int maxCascades = 4;

    const auto tierDefault = tier == Tier::full ? maxCascades : defaultRasterCascades;
    return jlimit (1, maxCascades, settings.sunCascades != 0 ? settings.sunCascades : tierDefault);
}

int RenderCapabilities::getShadowResolution (const RenderSettings& settings) noexcept
{
    return jlimit (minShadowResolution, maxShadowResolution, settings.shadowResolution);
}

} // namespace yup
