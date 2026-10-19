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
/** Stochastic screen-space reflections of the full tier (Stachowiak 2015, "Stochastic Screen-Space
    Reflections", SIGGRAPH Advances in Real-Time Rendering), two FullscreenPass draws:

    - Trace, at half resolution (texel i at the input pixel 2i, like AmbientOcclusionPass): each
      texel samples one GGX normal from the distribution of the visible normals (Heitz 2018) with
      interleaved gradient noise turning every frame, reflects the view around it, and marches the
      ray through the Hi-Z pyramid of the closest depth (HiZPyramid), at most maxIterations cells,
      accepting a hit only behind a surface by less than its thickness and on a surface facing the
      ray. Output: rgba32float, the hit uv, the depth of the surface hit and the pdf of the ray, 0
      for a miss or a ray leaving the screen.
    - Resolve, at full resolution: the ratio estimator of the talk over the rays of the 3 x 3
      trace texels around the pixel, each weighted by the GGX lobe of the pixel over the pdf of its
      ray (at most 1, so a rare ray of a neighbor does not sparkle) and by the depth and normal similarity of the texel it was traced from, so rays do not leak across
      edges. The radiance of a ray is the previous scene color (the history of TemporalAA, exposed)
      at the hit, reprojected by the motion of the hit and rescaled from the previous exposure to
      the current one. The result joins the resolve of the previous frame, reprojected by the
      motion of the reflected point (the pixel's motion plus the camera parallax of the mean hit
      distance behind the surface) and clipped towards the YCoCg box of the rays' radiance, by an
      exponential blend. Output: rgba16float, the reflected radiance in rgb (the prefiltered radiance of the split
      sum, exposed, so a bright scene at a low exposure fits the half floats) and the confidence in
      a, 0 where the probes and the environment reflect alone. It is also the history of the next
      frame.
    - Denoise, at full resolution: a bilateral 3 x 3 over the resolve, its taps spread by one pixel
      every 0.15 of roughness (at most 3, none below 0.075), weighted by depth, normal and
      roughness similarity, the radiance averaged reversibly tone mapped. Output: rgba16float,
      like the resolve.

    The confidence fades over the last screenEdgeFade of the screen around the hits, for rays
    turning towards the camera, and above the roughness fadeStart, to 0 at roughnessCut, above which
    nothing is traced. A hit whose reprojection leaves the screen counts as a miss. The noise turns
    with the frame: the resolve's own history keeps the reflections still while the scene changes
    and TemporalAA restarts its running mean, which then averages what is left (preflight R13), so
    the pass only runs with temporal anti-aliasing, whose history it reads.

    Bindings: set 0 binding 0 the ViewBlock, 1 linearClamp and 3 pointClamp; set 1 binding 0 the
    pass block, 1 the Hi-Z pyramid (its mip 0 is the depth), 2 the normals in the G-buffer RT1
    encoding; the resolve also binds 3 the trace, 4 the velocity, 5 the previous scene color and 6
    its own history; the denoise binds 3 the resolve and not the pass block. The trace and output
    targets and the "ssr" history pair come from the frame's
    FrameResources; its pipelines live as long as this object.

    @internal
*/
class YUP_API ReflectionPass
{
public:
    /** Perceptual roughness from which the confidence fades out, to 0 at roughnessCut. */
    static constexpr float fadeStart = 0.5f;

    /** Perceptual roughness at and above which nothing is traced. */
    static constexpr float roughnessCut = 0.6f;

    /** Cells the trace marches at most per ray. */
    static constexpr int maxIterations = 64;

    /** The width of the screen border, in uv, over which the hits fade out. */
    static constexpr float screenEdgeFade = 0.05f;

    //==============================================================================
    /** Creates a pass for @p device. The pipelines are compiled by prepare(). */
    explicit ReflectionPass (GpuDevice::Ptr device);

    /** Destructor. */
    ~ReflectionPass();

    //==============================================================================
    /** Compiles the pipelines once, remembering a failure instead of retrying every frame. */
    Result prepare();

    /** Traces and resolves the reflections of the G-buffer.

        @param frame               The frame to record into, after the G-buffer and @p hiZ.
        @param view                The camera the G-buffer was drawn with.
        @param hiZ                 The HiZPyramid of the G-buffer depth.
        @param normalRoughness     The G-buffer RT1, of the size of mip 0 of @p hiZ.
        @param velocity            The G-buffer velocity, current uv minus previous uv.
        @param previousSceneColor  The scene color of the previous frame: the TemporalAA history, exposed.
        @param previousExposure    The exposure @p previousSceneColor was rendered with, positive.
        @param resources           The frame's pool, for the targets.

        @returns The rgba16float reflections, of the size of mip 0 of @p hiZ, or nullptr when the
                 pass did not compile, @p hiZ is missing (the pyramid could not be built), another
                 input is missing, the targets could not be created or a draw could not be
                 recorded. Such a failure leaves the frame without the reflections; the second in a
                 row makes the pass give up (hasGivenUp()), logged once with YUP_DBG, except for a
                 missing pyramid, which HiZPyramid logged itself.
    */
    GpuTexture::Ptr render (GpuFrame& frame, const ViewData& view, const GpuTexture::Ptr& hiZ,
                            const GpuTexture::Ptr& normalRoughness, const GpuTexture::Ptr& velocity,
                            const GpuTexture::Ptr& previousSceneColor, float previousExposure, FrameResources& resources);

    /** True after two failed renders in a row: the frames no longer trace screen-space reflections. */
    bool hasGivenUp() const noexcept { return givenUp; }

    /** @internal Why the pass gave up, for tests: the message it logged, or the missing Hi-Z
        pyramid, which HiZPyramid logged itself. Empty while it has not given up. */
    const String& getGiveUpReason() const noexcept { return giveUpReason; }

    /** Releases the render targets not drawn into since the previous call: call it every frame the pass runs. */
    void trimUnusedTargets();

    /** Releases every render target, for a frame without the pass, a resize or a failed frame. */
    void releaseTargets() noexcept;

    /** @internal For tests: the number of render targets kept for the next draws. */
    size_t getNumCachedTargets() const noexcept { return trace.getNumCachedTargets() + resolve.getNumCachedTargets() + denoise.getNumCachedTargets(); }

    /** @internal Test hook: every render() from now on fails, as if the device could not record it. */
    void failRendersForTesting() noexcept { failRenders = true; }

    /** @internal Test hook: the pipelines fail to compile, as if the device rejected them. Call it before the first prepare(). */
    void failPipelinesForTesting() noexcept { failPipelines = true; }

private:
    GpuDevice::Ptr device;
    std::optional<Result> prepared;
    FullscreenPass trace;
    FullscreenPass resolve;
    FullscreenPass denoise;
    int consecutiveFailures = 0;
    bool givenUp = false;
    bool failRenders = false;
    bool failPipelines = false;
    String giveUpReason;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReflectionPass)
};

} // namespace yup
