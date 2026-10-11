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

#pragma once

namespace yup
{

//==============================================================================
/** Decides which rendering tier and which optional features a device can run.

    The decision is made from plain DeviceFacts so it can be tested without a GPU.
*/
class YUP_API RenderCapabilities
{
public:
    /** The rendering path in use. */
    enum class Tier
    {
        raster, ///< The classic forward path rendering straight to the target.
        full    ///< HDR scene color, temporal anti-aliasing and post effects.
    };

    /** Optional rendering features. */
    enum class Feature
    {
        hdrSceneColor,        ///< Scene rendered to a half-float target before tone mapping.
        temporalAntiAliasing, ///< Jittered frames accumulated over time.
        bloom,                ///< Glow around highlights.
        softShadows,          ///< Penumbrae from the light size: both tiers, on by default on full only.
        contactShadows,       ///< Screen-space contact shadows: full tier only.
        ambientOcclusion      ///< Screen-space ambient occlusion: on by default on full, opt-in on raster where the device can draw the depth and normal prepass. Off whenever RenderSettings::aoDistance is not a positive finite number.
    };

    /** Shadow cascades of the sun on the raster tier when RenderSettings::sunCascades is 0. */
    static constexpr int defaultRasterCascades = 2;

    /** The smallest shadow map resolution, see getShadowResolution(). */
    static constexpr int minShadowResolution = 256;

    /** The largest shadow map resolution, see getShadowResolution(). */
    static constexpr int maxShadowResolution = 4096;

    /** What the rendering decisions need to know about a device. */
    struct DeviceFacts
    {
        GpuPlatform platform = GpuPlatform::Headless; ///< The graphics backend.
        bool mobile = false;                          ///< True on iOS and Android.
        bool compute = false;                         ///< Compute passes are available.
        bool fragmentStorageBuffers = false;          ///< Fragment shaders can read storage buffers.
        bool storageTextures = false;                 ///< Compute can write storage textures.
        bool rgba16floatRenderable = false;           ///< rgba16float can be a render target.
        uint32_t rgba8Samples = 1;                    ///< Maximum MSAA sample count for rgba8unorm.
        uint32_t rgba16floatSamples = 0;              ///< Maximum MSAA sample count for rgba16float.
        uint32_t colorAttachments = 1;                ///< Maximum simultaneous color attachments.
        bool depthNormalPrepass = false;              ///< The raster depth and normal prepass can be drawn: rgba16float and depth32float render targets.
    };

    /** Reads the facts from a device. */
    static DeviceFacts queryDevice (const GpuDevice& device);

    /** Returns true when the backend passed the interop spike required by the full tier. */
    static bool passedInteropSpike (GpuPlatform platform) noexcept;

    /** Picks the tier a device runs for a given preference.

        The full tier needs rgba16float render targets, four color attachments, compute and storage
        buffers readable from fragment shaders, on a backend that passed its interop spike.
    */
    static Tier selectTier (const DeviceFacts& facts, RenderSettings::TierPreference preference) noexcept;

    /** Creates the capabilities for a device and settings.

        @param facts            The device facts.
        @param settings         The settings, whose tier preference is honored.
        @param requestedSamples The raster MSAA count: raster HDR post effects need rgba16float
                                multisampled at that count (DeviceFacts::rgba16floatSamples).
    */
    RenderCapabilities (const DeviceFacts& facts, const RenderSettings& settings, uint32_t requestedSamples);

    /** Returns the selected tier. */
    Tier getTier() const noexcept { return tier; }

    /** Returns true when the technique exists for this tier and device. */
    bool isAvailable (Feature feature) const noexcept;

    /** Returns true when the feature is available and switched on by the settings or the tier default. */
    bool isEnabled (Feature feature, const RenderSettings& settings) const noexcept;

    /** Returns the number of sun shadow cascades, from 1 to 4.

        RenderSettings::sunCascades 0 picks the tier default (4 on full, defaultRasterCascades on
        raster); any other value is clamped to [1, 4].
    */
    int getCascadeCount (const RenderSettings& settings) const noexcept;

    /** Returns RenderSettings::shadowResolution clamped to [minShadowResolution, maxShadowResolution].

        Values inside the range are used as given: a resolution that is not a power of two is not
        rounded. The cap bounds memory: one 4096 x 4096 depth32float map takes 64 MB and the sun
        uses up to 4 of them, and there is no device query for a larger safe size.
    */
    static int getShadowResolution (const RenderSettings& settings) noexcept;

private:
    DeviceFacts facts;
    uint32_t requestedSamples = 1;
    Tier tier = Tier::raster;
};

} // namespace yup
