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
/** Draws one fullscreen triangle with a pipeline compiled from a shader bundle.

    The bundle pairs shaders/yup_Fullscreen.vert with a fragment shader of the pass. The vertex
    shader outputs `uv` at location 0, running from (0, 0) at the top-left texel to (1, 1) at the
    bottom-right one on every backend. The pipeline has no vertex buffer, culls nothing, has no
    depth, and writes one color target, or several (bind attaches the extra ones with
    GpuRenderPass::setColorAttachment).

    The render target of each texture and view drawn into is kept for the next draws, until a
    trimUnusedTargets() finds it was not drawn into since the previous one: call it once per frame.

    @code
    FullscreenPass toneMapping;
    toneMapping.prepare (device, bundleData, bundleSize, GpuTextureFormat::rgba8unorm);
    toneMapping.draw (frame, destination, {}, [&] (GpuRenderPass& pass) { pass.setTexture (1, 1, source); });
    @endcode

    @internal
*/
class YUP_API FullscreenPass
{
public:
    //==============================================================================
    /** How the fragment's output combines with the target. */
    enum class Blend
    {
        replace,          ///< The output replaces the target.
        additive,         ///< The output is added to the target: (one, one).
        premultipliedOver ///< A premultiplied output covers the target by its alpha: (one, oneMinusSrcAlpha), alpha included.
    };

    //==============================================================================
    /** Creates an unprepared pass. */
    FullscreenPass();

    /** Destructor. */
    ~FullscreenPass();

    //==============================================================================
    /** Compiles the bundle (yup_Fullscreen.vert + a fragment) for one output format.

        @param device         The device to compile for.
        @param bundleData     The serialized shader bundle.
        @param bundleSize     The size of the bundle in bytes.
        @param outputFormat   The format of the textures draw() renders into.
        @param blend          How the output combines with the target.
        @param colorTargets   The number of color targets the fragment writes, all of outputFormat.
    */
    Result prepare (const GpuDevice::Ptr& device, const uint8* bundleData, size_t bundleSize,
                    GpuTextureFormat outputFormat, Blend blend = Blend::replace, int colorTargets = 1);

    /** Compiles the bundle (yup_Fullscreen.vert + a fragment) for color targets of different formats.

        @param device         The device to compile for.
        @param bundleData     The serialized shader bundle.
        @param bundleSize     The size of the bundle in bytes.
        @param outputFormats  The format of each color target the fragment writes, the first one
                              the texture draw() renders into, at least one.
        @param blend          How the outputs combine with the targets.
    */
    Result prepare (const GpuDevice::Ptr& device, const uint8* bundleData, size_t bundleSize,
                    Span<const GpuTextureFormat> outputFormats, Blend blend = Blend::replace);

    /** Draws one fullscreen triangle into @p target (one mip / layer of it), binding through @p bind.

        @param frame   The frame to record into.
        @param target  A render-target texture of the first prepared format.
        @param view    The mip level and layer of @p target to render into.
        @param bind    Sets the textures, samplers and uniforms of the draw, and attaches the
                       color targets after the first one.
        @param clear   The color to clear to first, or empty to keep the contents.

        @returns False if the pass is not prepared or the draw could not be recorded.
    */
    bool draw (GpuFrame& frame, const GpuTexture::Ptr& target, const GpuTextureViewDesc& view,
               const std::function<void (GpuRenderPass&)>& bind, std::optional<GpuColor> clear = {});

    /** Returns true once prepare() succeeded. */
    bool isPrepared() const noexcept;

    /** Releases the render targets not drawn into since the previous call. Call it at the end of
        every frame the pass may draw in, so targets of textures gone from the frame don't pile up.
    */
    void trimUnusedTargets();

    /** Releases every render target kept for the next draws, for a pass skipped this frame. */
    void releaseTargets() noexcept { targets.clear(); }

    /** @internal For tests: the number of render targets kept for the next draws. */
    size_t getNumCachedTargets() const noexcept { return targets.size(); }

private:
    struct CachedTarget
    {
        GpuTexture::Ptr texture;
        GpuTextureViewDesc view;
        GpuTarget::Ptr target;
        bool used = true; // Drawn into since the last trimUnusedTargets()
    };

    GpuDevice::Ptr device;
    GpuPipeline::Ptr pipeline;
    std::vector<CachedTarget> targets;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FullscreenPass)
};

} // namespace yup
