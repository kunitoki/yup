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
/** Applies the scene's analytic WorldFog to the scene color, one FullscreenPass draw from the
    camera to the depth of each pixel, before temporal anti-aliasing at the size of the depth.

    Each pixel takes the transmittance T of the fog in front of it, the closed form of
    WorldFog::getOpticalDepth() mirrored in shaders/lib/fog.glsl, and the light S the fog scatters
    towards the camera over that distance: single scattering with no shadowing, lit by the ambient
    radiance (the environment's spherical harmonics L0 times its intensity, plus the ambient color)
    and the irradiance of the first directional light that is not dark, through a normalized
    Henyey-Greenstein phase (1 / (4 pi) for isotropic fog). Pixels where nothing was
    drawn take the fog up to the far plane, or its limit at infinity with an infinite far plane: no
    light gets through fog a ray never leaves, and the optical depth stays finite for rays climbing
    out of fog thinning upwards.

    - Full tier: the scene color is blended in place, dst * T + S and a * T + (1 - T), through the
      premultiplied-over blend of FullscreenPass, so pixels of the background color gain the fog's
      coverage.
    - Raster tier: the resolved scene color is reversibly tone mapped, so it is read, inverted and
      written fogged into a new rgba16float target of its size: the first post pass.

    Blended items take the fog of what lies behind them, since only opaque items write the depth
    read here.

    Bindings: set 0 binding 0 the ViewBlock and 3 pointClamp; set 1 binding 0 the FogBlock, 1 the
    depth and 2 the color read on raster. Its pipelines live as long as this object, each compiled
    by the first frame of its tier.

    @internal
*/
class YUP_API FogApply
{
public:
    //==============================================================================
    /** The textures the fog is applied to. */
    struct Inputs
    {
        GpuTexture::Ptr color;          ///< The premultiplied rgba16float scene color: fogged in place, or read when colorIsReversible.
        bool colorIsReversible = false; ///< True on raster: color is reversibly tone mapped, so the fogged color goes into a new target.
        GpuTexture::Ptr depth;          ///< The single-sampled depth of the opaque items, read by uv.
    };

    //==============================================================================
    /** True when @p fog is drawn: it is enabled with a positive density, and every field is finite. */
    static bool isVisible (const WorldFog& fog) noexcept;

    //==============================================================================
    /** Creates a pass for @p device. The pipelines are compiled by the first render() of each tier. */
    explicit FogApply (GpuDevice::Ptr device);

    /** Destructor. */
    ~FogApply();

    //==============================================================================
    /** Applies @p fog to @p inputs.color, lit by the ambient light and environment of @p lighting
        and by the first directional light of @p lights.

        @returns The fogged color: @p inputs.color itself on full, a new rgba16float texture of its
                 size on raster. nullptr when nothing was drawn: the fog is not visible or an input
                 is missing (skips, which never count as failures), the pass gave up, or it failed to
                 compile, to create its target or to record. A failure leaves the frame without the
                 fog; the second in a row makes the pass give up (hasGivenUp()), logged once with YUP_DBG.
                 Targets and draws are tried again on the next frame; a pipeline that failed to compile
                 is not compiled again, so it gives up on that next frame.
    */
    GpuTexture::Ptr render (GpuFrame& frame, const ViewData& view, const WorldFog& fog, const LightingInputs& lighting, Span<const SceneDrawList::Light> lights, FrameResources& resources, const Inputs& inputs);

    /** True after two failed renders in a row: the frames no longer draw the fog. */
    bool hasGivenUp() const noexcept { return givenUp; }

    /** @internal Why the pass gave up, for tests: the message it logged. Empty while it has not given up. */
    const String& getGiveUpReason() const noexcept { return giveUpReason; }

    /** Releases the render targets not drawn into since the previous call: call it every frame the pass runs. */
    void trimUnusedTargets();

    /** Releases every render target, for a frame without the fog, a resize or a failed frame. */
    void releaseTargets() noexcept;

    /** @internal For tests: the number of render targets kept for the next draws. */
    size_t getNumCachedTargets() const noexcept;

    /** @internal Test hook: the next @p count render() calls drawing the fog fail, as if the device could not record them. */
    void failRendersForTesting (int count = std::numeric_limits<int>::max()) noexcept { failRenders = count; }

private:
    struct Pipeline
    {
        FullscreenPass pass;
        std::optional<Result> prepared;
    };

    Result prepare (Pipeline& pipeline, FullscreenPass::Blend blend);

    GpuDevice::Ptr device;
    Pipeline inPlace;   // Full: blends over the scene color
    Pipeline transient; // Raster: writes the fogged color into a new target
    int consecutiveFailures = 0;
    bool givenUp = false;
    int failRenders = 0; // The renders still to fail, see failRendersForTesting()
    String giveUpReason;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FogApply)
};

} // namespace yup
