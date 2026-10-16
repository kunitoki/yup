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
/** Draws the depth and shading normals of the opaque and masked surfaces for the screen-space
    effects of the raster tier, which has no G-buffer (ambient occlusion, depth of field).

    The targets, single sampled and acquired from the frame's FrameResources:
    - rgba16float: the G-buffer RT1 encoding (see GBufferPass): octahedral shading normal after normal
      mapping, flipped on back faces, in xy; the filtered roughness in z; metallic in w. Cleared to 0;
    - depth32float, cleared to 1. The effects read it with texelFetch through the nearest
      pointClamp sampler (set 0 binding 3), since depth32float is not filterable on GLES 3.0 and
      WebGL2, where a linear sampler leaves the texture incomplete. Raw depth reads of
      depth32float are verified on Metal; on GL and WebGL2 they still wait on the user's runs of
      the Phase 3 shadow tests, and a failure there would move the linear depth into the w of a
      second rgba16float target.

    It shares the vertex stage and the material evaluation of GBufferPass (lib/geometry.glsl,
    lib/material.glsl), so both write the same normals and cut out the same masked texels, and
    its bindings are theirs: set 0 binding 0 the ViewBlock, set 2 the draw and its material (see
    GeometryBindings).

    @internal
*/
class YUP_API DepthNormalPrepass
{
public:
    //==============================================================================
    /** Creates a pass for @p device. The pipelines are compiled by prepare(). */
    explicit DepthNormalPrepass (GpuDevice::Ptr device);

    /** Destructor. */
    ~DepthNormalPrepass();

    //==============================================================================
    /** True when the device can run it: rgba16float renderable.

        The depth32float checks are formal: every device reports the uncompressed formats as
        supported and renderable. Whether GL and WebGL2 read raw depth32float rests on the user's
        runs of the Phase 3 shadow tests, which read the shadow maps the same way.
    */
    static bool isSupported (const GpuDevice& device);

    /** Compiles the pipelines once: a failure is remembered instead of retried every frame. */
    Result prepare();

    /** Draws the opaque and masked items single-sampled at @p width x @p height.

        Targets the device cannot create leave the frame without the pass: it is logged once, nothing
        is drawn and the getters return nullptr, which is not a failure.

        @param beginScope  Called once the targets exist, right before the pass records, to open its
                           GPU scope (see SceneRenderer::setFrameStatsEnabled()); may be empty.

        @returns False if the pass is not prepared or could not be recorded. The targets are then nullptr.
    */
    bool render (GpuFrame& frame, const ViewData& view, const SceneDrawList& drawList, FrameResources& resources, int width, int height,
                 const std::function<void()>& beginScope = {});

    /** The rgba16float normal target of the last render, in the G-buffer RT1 encoding, or nullptr. */
    GpuTexture::Ptr getNormalRoughness() const noexcept { return normalRoughness; }

    /** The depth32float target of the last render, sampled with texelFetch, or nullptr. */
    GpuTexture::Ptr getDepth() const noexcept { return depth; }

    /** Releases the targets of the last render and the render target kept for the next ones, for a
        frame not drawing the pass or a new frame size.
    */
    void releaseTargets() noexcept;

    //==============================================================================
    /** @internal Test hook: the pipelines fail to compile, as if the device rejected them. Call it
        before the first prepare().
    */
    void failPipelinesForTesting() noexcept { pipelinesFailForTesting = true; }

    /** @internal Test hook: the targets of the next @p count renders fail to create, every one from
        now on by default, as if the device ran out of memory.
    */
    void failTargetsForTesting (int count = std::numeric_limits<int>::max()) noexcept { targetFailures = count; }

private:
    GpuDevice::Ptr device;
    std::optional<Result> prepared;
    GeometryBindings geometry;
    std::array<GpuPipeline::Ptr, GeometryBindings::numRasterStates> pipelines;
    GpuTarget::Ptr surface;
    GpuTexture::Ptr normalRoughness;
    GpuTexture::Ptr depth;
    bool pipelinesFailForTesting = false;
    int targetFailures = 0;
    bool targetsFailed = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DepthNormalPrepass)
};

} // namespace yup
