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
    environment with its sampler at 14, and 7 and 11 the area light tables, read through linearClamp
    at binding 1. Set 1 binds the rest of them: 0 the LightingBlock, 6 the lights, 7 the light lists
    of the clusters, 8 the material table; then the G-buffer: 9-11 RT0-RT2 and 12 the depth. Set 3
    binds the shadows: 0 the sun cascades and 4 the ShadowBlock.
    Only the area light variant (YUP_3D_AREA_LIGHTS), which shades the frames where
    LightingInputs::areaLights is set, declares the tables and linearClamp.

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
    /** Compiles the pipeline, of the area light variant when @p areaLights is set, and creates the
        stand-in textures, once per variant: a failure is remembered instead of retried every frame.
    */
    Result prepare (const GpuDevice::Ptr& device, bool areaLights);

    /** Shades @p gbuffer into @p sceneColor, an rgba16float texture of its size, with the variant
        LightingInputs::areaLights picks.

        @returns False if that variant is not prepared, an input is missing or the draw could not be recorded.
    */
    bool render (GpuFrame& frame, const ViewData& view, const GBufferPass::Targets& gbuffer,
                 const LightingInputs& lighting, const GpuTexture::Ptr& sceneColor);

    /** Releases the render targets not drawn into since the previous call: call it every frame the pass runs. */
    void trimUnusedTargets();

    /** Releases every render target, for a frame without the pass, a resize or a failed frame. */
    void releaseTargets() noexcept;

private:
    struct Variant
    {
        std::optional<Result> prepared;
        FullscreenPass pass;
    };

    std::optional<Result> defaults;
    std::array<Variant, 2> variants; // Punctual lights only, then with area lights
    GpuTexture::Ptr blackTexture;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LightingPass)
};

} // namespace yup
