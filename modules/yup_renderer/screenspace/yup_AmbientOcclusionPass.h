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
/** Screen-space ambient occlusion with bent normals: ground-truth ambient occlusion (Jimenez et
    al. 2016, "Practical Real-Time Strategies for Accurate Indirect Occlusion"), traced then
    denoised by three FullscreenPass draws, at half resolution on both tiers.

    It reads the depth32float depth and the rgba16float normals in the G-buffer RT1 encoding: of the
    G-buffer on the full tier and of the DepthNormalPrepass on raster. The output is rgba8, half the
    size of the depth rounded up: the view-space bent normal * 0.5 + 0.5 in rgb, the visibility in
    a. Texel i is traced at the input texel 2i; lib/probes.glsl gtaoUpsampleUv() places it for the
    readers. LightingPass upsamples it with depth- and normal-aware weights, ForwardPass (raster)
    bilinearly, where the occlusion can bleed over about one full resolution texel at depth edges.

    - Full tier: 2 slices per texel of 6 steps per side, turned per texel by interleaved gradient
      noise, which also turns with the frame under temporal anti-aliasing so its running mean
      accumulates them. Without temporal anti-aliasing the noise stays still and 4 slices are traced.
    - Raster: 2 slices of 6 steps per side, turned by a fixed 4x4 pattern.

    The marched footprint is capped at a tenth of the view height. The trace is then filtered by a
    separable depth- and normal-aware bilateral mean, two 1D passes of 5 texels centered on each,
    the ends weighing half, which averages every rotation of the 4x4 pattern once without a shift.

    Bindings: set 0 binding 0 the ViewBlock and 3 pointClamp; set 1 binding 0 the pass block, 1 the
    depth, 2 the normals and, for the denoise, 3 the texture it filters. Its two targets come from
    the frame's FrameResources.

    Screen-space diffuse GI (full tier, RenderCapabilities::Feature::screenSpaceGI): given the
    previous frame's radiance (Radiance), the trace and the denoise run their
    YUP_3D_SCREEN_SPACE_GI variants, which also gather the light of the arcs the horizon search
    finds occluded, from the previous scene color at the samples that raise the horizon, weighted by
    the share of the cosine-weighted hemisphere each occludes: Horizon-Based Indirect Lighting
    (Mayaux 2018) over the history of TemporalAA, see yup_GtaoTrace.frag. The result, rgba16float at the size of the ambient
    occlusion, holds the gathered irradiance in rgb and the share it came from in a
    (getIndirectDiffuse()); addIndirectDiffuse() then adds it to the scene color after LightingPass,
    weighted by each texel's diffuse color as the environment's diffuse light is, in one additive
    pass (yup_SsgiComposite.frag). The probes, the volumes and the environment keep lighting the
    visible arcs, darkened by the visibility, so the two add up. Its pipelines are compiled by the
    first frame with GI and kept; its two rgba16float targets are released on the frames without
    it. A failure leaves the frame with the ambient occlusion alone, and the second in a row gives
    it up for the life of the pass (hasGivenUpIndirectDiffuse()), logged once.

    @internal
*/
class YUP_API AmbientOcclusionPass
{
public:
    //==============================================================================
    /** The previous frame's radiance the screen-space diffuse GI gathers. */
    struct Radiance
    {
        GpuTexture::Ptr previousSceneColor; ///< The TemporalAA history: exposed radiance, premultiplied by its coverage. nullptr for no GI.
        GpuTexture::Ptr velocity;           ///< The G-buffer velocity, current uv minus previous uv, of the size of the depth.
        float previousExposure = 0.0f;      ///< The exposure previousSceneColor was rendered with; no GI unless positive.
    };

    //==============================================================================
    /** Creates a pass for @p device. The pipelines are compiled by prepare(). */
    explicit AmbientOcclusionPass (GpuDevice::Ptr device);

    /** Destructor. */
    ~AmbientOcclusionPass();

    //==============================================================================
    /** Compiles the pipelines once: a failure is logged once and remembered instead of retried every frame. */
    Result prepare();

    /** Traces and denoises the ambient occlusion of @p depth and @p normalRoughness.

        Targets the device cannot create leave the frame without it: it is logged once, nothing is
        drawn and getOutput() returns nullptr, which is not a failure.

        @param frame            The frame to record into.
        @param view             The camera the depth was drawn with.
        @param tier             Full: interleaved gradient noise. Raster: the 4x4 pattern.
        @param temporal         Full tier: the noise turns with the frame, for temporal anti-aliasing to average.
        @param depth            The depth32float depth, at the viewport size of @p view.
        @param normalRoughness  The rgba16float normals in the G-buffer RT1 encoding, of the size of @p depth.
        @param distance         The world-space distance within which surfaces occlude each other
                                (RenderSettings::aoDistance). One that is not a positive finite number occludes nothing;
                                SceneRenderer does not run the pass then.
        @param resources        The frame's pool, for the targets.
        @param radiance         Full tier: the radiance the screen-space diffuse GI gathers, a default Radiance without it.
                                Without it, or once the GI has given up, the frame traces the ambient
                                occlusion alone and releases the GI targets.

        @returns False in two cases, which SceneRenderer tells apart by checking the first itself:
                 - the frame cannot have ambient occlusion: the pass did not compile (see prepare()),
                   @p depth or @p normalRoughness is missing, or @p resources has no samplers;
                 - a draw could not be recorded, which fails the frame.
                 getOutput() is nullptr after either. A GI failure is not one of them: the frame
                 then traces the ambient occlusion alone.
    */
    bool render (GpuFrame& frame, const ViewData& view, RenderCapabilities::Tier tier, bool temporal,
                 const GpuTexture::Ptr& depth, const GpuTexture::Ptr& normalRoughness, float distance, FrameResources& resources,
                 const Radiance& radiance);

    /** The rgba8 output of the last render, or nullptr. */
    GpuTexture::Ptr getOutput() const noexcept { return output; }

    /** The rgba16float screen-space diffuse GI of the last render, of the size of getOutput(): the
        gathered irradiance in rgb, the share of the hemisphere it came from in a. nullptr without it.
    */
    GpuTexture::Ptr getIndirectDiffuse() const noexcept { return indirectDiffuse; }

    /** Adds getIndirectDiffuse() to @p sceneColor, the rgba16float output of LightingPass, with
        additive blending: each G-buffer texel its diffuse color times the gathered irradiance, weighted
        as LightingPass weighs the environment's diffuse light (the dielectric specular, the material
        occlusion, the clearcoat and the sheen, whose sheen reflects it too), exposed. Alpha does not change.

        @param frame       The frame to record into, after LightingPass.
        @param view        The camera of the G-buffer.
        @param gbuffer     The G-buffer LightingPass shaded.
        @param lighting    The frame's material table, lookup tables and samplers.
        @param sceneColor  The scene color LightingPass wrote.

        @returns True when it was added. Without getIndirectDiffuse() nothing is drawn; a pipeline
                 that cannot compile, a missing input or a draw that cannot be recorded leave the frame
                 without the GI and count as a GI failure (see hasGivenUpIndirectDiffuse()).
    */
    bool addIndirectDiffuse (GpuFrame& frame, const ViewData& view, const GBufferPass::Targets& gbuffer,
                             const LightingInputs& lighting, const GpuTexture::Ptr& sceneColor);

    /** True after two screen-space diffuse GI failures in a row: the frames no longer gather it. */
    bool hasGivenUpIndirectDiffuse() const noexcept { return indirectGivenUp; }

    /** Releases the render targets not drawn into since the previous call: call it every frame the pass runs. */
    void trimUnusedTargets();

    /** Releases the output and every render target, for a frame without the pass, a resize or a failed frame. */
    void releaseTargets() noexcept;

    /** Releases the screen-space diffuse GI output and render targets only, for a frame without it. */
    void releaseIndirectDiffuseTargets() noexcept;

    /** @internal For tests: the number of render targets kept for the next draws, the GI ones included. */
    size_t getNumCachedTargets() const noexcept
    {
        return trace.getNumCachedTargets() + denoise.getNumCachedTargets() + getNumCachedIndirectDiffuseTargets();
    }

    /** @internal For tests: the number of render targets of the screen-space diffuse GI kept for the next draws. */
    size_t getNumCachedIndirectDiffuseTargets() const noexcept
    {
        return traceIndirect.getNumCachedTargets() + denoiseIndirect.getNumCachedTargets() + composite.getNumCachedTargets();
    }

    /** @internal Test hook: every screen-space diffuse GI trace and composite from now on fails, as if the device could not record it. */
    void failIndirectDiffuseForTesting() noexcept { failIndirect = true; }

private:
    Result prepareIndirectDiffuse();
    void noteIndirectDiffuseFailure (const String& failure);

    GpuDevice::Ptr device;
    std::optional<Result> prepared;
    FullscreenPass trace;
    FullscreenPass denoise;
    GpuTexture::Ptr output;
    bool targetsFailed = false;

    std::optional<Result> indirectPrepared; // The GI pipelines, compiled by the first frame with GI
    FullscreenPass traceIndirect;
    FullscreenPass denoiseIndirect;
    FullscreenPass composite;
    GpuTexture::Ptr indirectDiffuse;
    int indirectFailures = 0;     // GI failures in a row, reset by a GI added to the scene color
    bool indirectGivenUp = false; // Two GI failures in a row: no more GI
    bool failIndirect = false;    // failIndirectDiffuseForTesting()

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmbientOcclusionPass)
};

} // namespace yup
