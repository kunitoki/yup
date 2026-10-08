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
/** Renders the sun shadow map of a frame.

    The first directional light with LightNode::castsShadows, among the lights shading the frame,
    is fitted with an orthographic projection around the bounding sphere of the opaque items. Its
    depth is packed into the four channels of a SceneRenderer::shadowMapSize square rgba8unorm
    map, which every backend can render to and read with texelFetch.

    @internal
*/
class YUP_API ShadowRenderer
{
public:
    //==============================================================================
    /** The map of a frame and how to read it. */
    struct Result
    {
        GpuTexture::Ptr map;           ///< The packed depth, or nullptr when nothing casts.
        Matrix4 shadowMatrix;          ///< World to shadow map clip space, depth from 0 to 1.
        float shadowInfo[4] = { -1.0f, 0.0f, 0.0f, 0.0f }; ///< Shadowed light index (-1 for none), texel size, normal offset, depth bias.
        String error;                  ///< Why the shadow pass could not be recorded, empty on success.
    };

    //==============================================================================
    /** Creates a renderer. GPU objects are created by prepare(). */
    ShadowRenderer();

    /** Destructor. */
    ~ShadowRenderer();

    //==============================================================================
    /** Compiles the shadow pipeline once, remembering a failure instead of retrying every frame. */
    yup::Result prepare (const GpuDevice::Ptr& device);

    /** Today's single packed sun map, moved verbatim. Returns an empty map when nothing casts, and
        sets Result::error when the shadow pass cannot begin.
    */
    Result render (const GpuDevice::Ptr& device, GpuFrame& frame, const SceneDrawList& drawList);

private:
    bool prepareTarget (const GpuDevice::Ptr& device);

    GpuPipeline::Ptr pipeline;
    GpuTarget::Ptr target;
    GpuTexture::Ptr depthTexture;
    std::optional<yup::Result> preparation;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShadowRenderer)
};

} // namespace yup
