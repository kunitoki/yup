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
/** Renders the shadow maps of a frame and writes the shadow records the shading passes read.

    The first directional light with LightNode::castsShadows that is not dark (GpuSceneData::isDark())
    is the sun. Its cascades (ShadowCascades, RenderCapabilities::getCascadeCount(), one for an
    orthographic camera) are fitted to the receivers (opaque, masked and blended items) and the
    casters (opaque and masked items), and rendered depth only, with slope-scaled depth bias, into
    the layers of one depth32float array texture of RenderCapabilities::getShadowResolution() texels.
    Masked casters discard the texels their material cuts out.

    The renderer owns the cascade texture: it is created when a sun casts, kept while it fits, and
    released when no sun casts. Its maps are rendered again only when the casters, the shadow
    casting lights, the shadow settings or the cascade fit change (Inputs::signature and the
    cascade matrices); invalidate() forgets them, for frames the GPU did not execute.

    @internal
*/
class YUP_API ShadowRenderer
{
public:
    //==============================================================================
    /** What the shadows of a frame depend on. */
    struct Inputs
    {
        const SceneDrawList& drawList;          ///< The items and lights of the frame.
        const ViewData& view;                   ///< The camera the cascades are fitted to.
        const RenderCapabilities& capabilities; ///< The tier: its cascade count and temporal anti-aliasing.
        const RenderSettings& settings;         ///< The shadow settings.
        uint64 signature = 0;                   ///< A hash of what the maps depend on besides the cascade fit: the casters, the shadow casting lights and the settings.
        bool enabled = true;                    ///< False when the shadow comparison sampler is missing: nothing is shadowed.
    };

    //==============================================================================
    /** Creates a renderer. GPU objects are created by prepare() and render(). */
    ShadowRenderer();

    /** Destructor. */
    ~ShadowRenderer();

    //==============================================================================
    /** Compiles the opaque and masked shadow pipelines once, remembering a failure instead of retrying every frame. */
    Result prepare (const GpuDevice::Ptr& device);

    /** Renders every shadow map the frame needs into @p frame, unless the maps it holds still fit,
        and writes the shadow records of the frame into @p out.

        @returns A failure when the cascade texture cannot be created or a shadow pass cannot be
                 recorded; the frame must then be abandoned.
    */
    Result render (const GpuDevice::Ptr& device, GpuFrame& frame, const Inputs& inputs, GpuShadowData& out);

    /** The sun cascades of the last render(): a depth32float array texture, or a one layer
        placeholder when no sun casts. */
    GpuTexture::Ptr getCascadeTexture() const noexcept;

    /** For each light of the last render()'s draw list, its first shadow record, -1 when unshadowed. */
    Span<const int> getShadowRecordIndices() const noexcept { return recordIndices; }

    /** Forgets the rendered maps, so the next render() draws them again: for frames the GPU failed
        to execute, or that were abandoned after their shadow passes were recorded. */
    void invalidate() noexcept;

    /** @internal The number of render() calls that drew shadow maps, for tests. */
    int getNumRenderedFrames() const noexcept { return numRenderedFrames; }

private:
    struct RenderedState;

    Result preparePlaceholder (const GpuDevice::Ptr& device, GpuFrame& frame);
    bool prepareCascades (const GpuDevice::Ptr& device, int resolution, int numCascades);
    Result renderCascade (const GpuDevice::Ptr& device, GpuFrame& frame, const SceneDrawList& drawList, int layer, const Matrix4& viewProjection);

    GeometryBindings geometry;
    GpuPipeline::Ptr opaquePipeline;
    GpuPipeline::Ptr maskedPipeline;
    std::optional<Result> preparation;
    GpuTexture::Ptr cascades;
    std::vector<GpuTarget::Ptr> layerTargets;
    GpuTexture::Ptr placeholder;
    bool placeholderCleared = false;
    std::unique_ptr<RenderedState> rendered;
    std::vector<int> recordIndices;
    int numRenderedFrames = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShadowRenderer)
};

} // namespace yup
