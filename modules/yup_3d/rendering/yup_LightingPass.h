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
/** Deferred shading of the full tier: one fullscreen triangle shading every G-buffer texel into
    the scene color, with the BSDF, lights, ambient light and environment of ForwardPass.

    It writes every pixel: shaded surfaces with alpha 1, the environment with alpha 1 where no
    surface covers the texel and the environment is visible, and transparent black otherwise, for
    tone mapping to composite the background color under it.

    Set 0 binds the shared samplers and lookups of the LightingInputs: 4 shadowCompare, 6 the
    environment with its sampler at 14, and 7 the lookup tables (LookupTables), read through
    linearClamp at binding 1. Set 1 binds the rest of them: 0 the LightingBlock, 6 the lights, 7 the light lists
    of the clusters, 8 the material table; then the G-buffer: 9-11 RT0-RT2 and 12 the depth; 13
    the ambient occlusion (LightingInputs::ambientOcclusion, or white without it). Set 3
    binds the shadows: 0 the sun cascades, 2 the point and spot light atlas and 4 the ShadowBlock.
    The area light variant (YUP_3D_AREA_LIGHTS) shades the frames where LightingInputs::areaLights
    is set; every variant reads the lookup tables, for the environment.

    The shadow filter variant (YUP_3D_SOFT_SHADOWS) shades the frames where
    LightingInputs::shadowFilters is set: it holds the soft shadows and the contact shadows, and
    only it declares and binds the same two shadow maps again at set 3 bindings 1 and 3, read raw
    by the blocker search. A frame using either feature pays for both; the default variant runs
    neither.

    The local indirect variant (YUP_3D_LOCAL_INDIRECT, preflight R27) shades the frames with
    reflection probes or irradiance volumes (LightingInputs::hasReflectionProbes(),
    hasIrradianceVolumes()): only it declares and binds set 1 binding 14, the reflection probes of
    the frame (LightingInputs::reflectionProbeBuffer), and 15, its irradiance volumes
    (LightingInputs::irradianceVolumeBuffer), with their atlases at set 0 bindings 8, 9 and 10
    (black without them). The default variants carry no probe or volume code. All three dimensions
    combine, into eight variants.

    @internal
*/
class YUP_API LightingPass
{
public:
    //==============================================================================
    /** Creates a pass. GPU objects are created by prepare(). */
    LightingPass();

    /** Destructor. */
    ~LightingPass();

    //==============================================================================
    /** Compiles the pipeline, of the area light variant when @p areaLights is set, of the shadow
        filter variant when @p shadowFilters is set and of the local indirect variant when
        @p localIndirect is set, and creates the stand-in textures, once per variant: a failure is
        remembered instead of retried every frame.
    */
    Result prepare (const GpuDevice::Ptr& device, bool areaLights, bool shadowFilters, bool localIndirect = false);

    /** Shades @p gbuffer into @p sceneColor, an rgba16float texture of its size, with the variant
        LightingInputs::areaLights, LightingInputs::shadowFilters and the frame's probes and volumes pick.

        Each variant is compiled on @p device the first time a frame needs it (see prepare()). If the
        local indirect variant fails to compile, the failure is logged once and the frames shade
        without reflection probes and irradiance volumes. If the shadow filter variant fails to
        compile, the failure is logged once and the frames shade with hard shadows and no contact shadows.

        @returns False if the variant cannot be compiled, an input is missing or the draw could not be recorded.
    */
    bool render (const GpuDevice::Ptr& device, GpuFrame& frame, const ViewData& view, const GBufferPass::Targets& gbuffer,
                 const LightingInputs& lighting, const GpuTexture::Ptr& sceneColor);

    /** Releases the render targets not drawn into since the previous call: call it every frame the pass runs. */
    void trimUnusedTargets();

    /** Releases every render target, for a frame without the pass, a resize or a failed frame. */
    void releaseTargets() noexcept;

    /** @internal True when the last render() shaded with a local indirect variant, for tests. */
    bool usesLocalIndirect() const noexcept { return usedLocalIndirect; }

    /** @internal Test hook: the local indirect variants not compiled yet fail to compile, as if the device rejected them. */
    void failLocalIndirectForTesting() noexcept { localIndirectFailsForTesting = true; }

private:
    struct Variant
    {
        std::optional<Result> prepared;
        FullscreenPass pass;
    };

    std::optional<Result> defaults;
    std::array<Variant, 8> variants; // Punctual lights only, then with area lights; then both again with the shadow filters; then all four again with the local indirect code
    GpuTexture::Ptr blackCube; // The environment without one
    GpuTexture::Ptr whiteTexture; // Stands in for the ambient occlusion when there is none
    GpuTexture::Ptr blackTexture; // Stands in for the probe and volume atlases without them
    bool shadowFiltersFailureReported = false; // The shadow filter variant failed to compile, and was logged once
    bool localIndirectFailureReported = false; // The local indirect variant failed to compile, and was logged once
    bool localIndirectFailsForTesting = false; // failLocalIndirectForTesting(): the local indirect variants not compiled yet fail
    bool usedLocalIndirect = false;            // The last render() shaded with the local indirect variant

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LightingPass)
};

} // namespace yup
