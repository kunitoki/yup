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

    @internal
*/
class YUP_API AmbientOcclusionPass
{
public:
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

        @returns False in two cases, which SceneRenderer tells apart by checking the first itself:
                 - the frame cannot have ambient occlusion: the pass did not compile (see prepare()),
                   @p depth or @p normalRoughness is missing, or @p resources has no samplers;
                 - a draw could not be recorded, which fails the frame.
                 getOutput() is nullptr after either.
    */
    bool render (GpuFrame& frame, const ViewData& view, RenderCapabilities::Tier tier, bool temporal,
                 const GpuTexture::Ptr& depth, const GpuTexture::Ptr& normalRoughness, float distance, FrameResources& resources);

    /** The rgba8 output of the last render, or nullptr. */
    GpuTexture::Ptr getOutput() const noexcept { return output; }

    /** Releases the render targets not drawn into since the previous call: call it every frame the pass runs. */
    void trimUnusedTargets();

    /** Releases the output and every render target, for a frame without the pass, a resize or a failed frame. */
    void releaseTargets() noexcept;

    /** @internal For tests: the number of render targets kept for the next draws. */
    size_t getNumCachedTargets() const noexcept { return trace.getNumCachedTargets() + denoise.getNumCachedTargets(); }

private:
    GpuDevice::Ptr device;
    std::optional<Result> prepared;
    FullscreenPass trace;
    FullscreenPass denoise;
    GpuTexture::Ptr output;
    bool targetsFailed = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmbientOcclusionPass)
};

} // namespace yup
