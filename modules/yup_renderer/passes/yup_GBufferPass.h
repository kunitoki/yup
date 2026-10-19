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
/** Writes the opaque and masked surfaces of the full tier into a G-buffer, for a lighting pass to
    shade them once per pixel.

    The targets (normative for the lighting pass):
    - RT0 rgba8unorm: sRGB-encoded base color, material occlusion (1 + strength * (texture - 1));
    - RT1 rgba16float: octahedral shading normal (after normal mapping, flipped on back faces), the
      roughness widened by the normal variance within the pixel (sqrt (sqrt (filtered alpha2))),
      metallic;
    - RT2 rgba16float: emission (exposed, times the emissive strength), material id in the
      frame's MaterialTable;
    - RT3 rg16float: velocity, current uv minus previous uv, both unjittered;
    - depth32float.

    RT0-RT2 are cleared to 0 and depth to 1. Masked texels below their cutoff write nothing. Texels
    no surface covers keep their cleared material and get the velocity of the environment.

    Bindings follow ForwardPass: set 0 binding 0 the ViewBlock, set 2 the draw and its material
    (see GeometryBindings), with the material id in the DrawBlock.

    @internal
*/
class YUP_API GBufferPass
{
public:
    //==============================================================================
    /** The attachments of the pass. */
    struct Targets
    {
        GpuTexture::Ptr albedoOcclusion;  ///< RT0 rgba8unorm.
        GpuTexture::Ptr normalRoughness;  ///< RT1 rgba16float.
        GpuTexture::Ptr emissionMaterial; ///< RT2 rgba16float.
        GpuTexture::Ptr velocity;         ///< RT3 rg16float, shared with the passes drawing after it.
        GpuTexture::Ptr depth;            ///< depth32float, shared with the passes drawing after it.
    };

    //==============================================================================
    /** Creates a pass. GPU objects are created by prepare(). */
    GBufferPass();

    /** Destructor. */
    ~GBufferPass();

    //==============================================================================
    /** Creates the default textures and compiles the pipelines, once: a failure is remembered
        instead of retried every frame.
    */
    Result prepare (const GpuDevice::Ptr& device);

    /** Acquires RT0-RT2 for this frame from @p resources, at the size of @p velocity, which is
        used as RT3 with @p depth.
    */
    static Targets acquireTargets (FrameResources& resources, GpuTexture::Ptr velocity, GpuTexture::Ptr depth);

    /** Draws @p items, the opaque and masked ones, then fills the texels they left uncovered with
        the motion of the environment.

        @p materials is cleared, then numbers the materials of the items for RT2.

        @returns False if the pass is not prepared, a target is missing or the pass could not begin.
    */
    bool render (GpuFrame& frame, const ViewData& view, Span<const SceneDrawList::DrawItem> items,
                 MaterialTable& materials, const Targets& targets);

    /** Releases the render target kept for the next frames, for a frame not drawing the G-buffer. */
    void releaseTargets() noexcept { surface = nullptr; }

private:
    GpuDevice::Ptr device;
    std::optional<Result> prepared;
    GeometryBindings geometry;
    std::array<GpuPipeline::Ptr, GeometryBindings::numRasterStates> pipelines;
    GpuPipeline::Ptr background;
    GpuTarget::Ptr surface;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GBufferPass)
};

} // namespace yup
