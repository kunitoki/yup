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
    Masked casters discard the texels their material cuts out, and each cascade only draws the
    casters that reach it (ShadowCascades::castsInto()).

    A compact scene gets one map fitted to the whole scene instead (ShadowCascades::fitScene()),
    while its texels are no larger than those of the first cascade the view would get
    (ShadowCascades::prefersSceneFit()): it does not follow the camera, so orbiting, panning or
    zooming around such a scene draws no shadow map while that holds (a closer or lower view can make
    the first cascade finer and bring the cascades back), and it covers every receiver, so it never
    fades out.

    Point and spot lights with LightNode::castsShadows that are not dark and reach a receiver get
    tiles of one depth32float atlas (ShadowAtlas) of RenderCapabilities::getShadowResolution() texels
    on raster, twice that on full, rounded down to a power of two (so it never exceeds the setting)
    and at most 4096: 6 perspective views for a point light (and for a
    spot light too wide for one view), 1 for a spot light, each spanning a guard band
    (ShadowAtlas::getGuardAngle()) beyond its field of view, from max (0.01, LightNode::radius) to
    the effective range of the light. Lights take tiles from the most to the least important, as the
    atlas serves them, while their views fit the shadow records left after the cascades, at most 16
    on raster: a light whose views do not all fit is left unshadowed. Every tile is rendered in one
    depth-only pass, which clears the whole atlas once. The scene fit takes one record where the
    cascades take the tier's cascade count, so switching between them changes how many records are left.

    When the tier enables contact shadows (RenderCapabilities::Feature::contactShadows, filterInfo
    flag 1), each light with LightNode::contactShadows that is not dark carries its march distance
    and thickness in contact[0] and contact[1] of its first record. A light without a shadow map
    takes one record without a map (atlasRect size 0) for them, after the atlas views and within the
    same records, while any are left, from the most important light as the atlas serves them:
    directional lights first, then larger tiles (ShadowAtlas::getLightTileSize()), then lower
    light indices.

    The renderer owns the cascade texture and the atlas: each is created when a light needs it, kept
    while it fits, and released when no light needs it, a 1x1 placeholder standing in for it. The
    maps are rendered again only when what they hold changes: a cascade when the shadow casting
    directional lights or the shadow settings (Inputs::globalSignature), its matrix, or the casters
    reaching it and their signatures (Inputs::casterSignatures) change; the atlas when the casters,
    the lights, the settings (Inputs::signature), the tiles or the view matrices change. invalidate()
    forgets every map, for frames the GPU did not execute.

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
        uint64 globalSignature = 0;             ///< The part of signature every cascade depends on: the shadow casting directional lights and the settings.
        Span<const uint64> casterSignatures;    ///< One hash per opaque item of drawList, in order: what it draws into a shadow map. Without one per item every map is drawn every frame.
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

        @returns A failure when the cascade texture or the atlas cannot be created or a shadow pass
                 cannot be recorded; the frame must then be abandoned.
    */
    Result render (const GpuDevice::Ptr& device, GpuFrame& frame, const Inputs& inputs, GpuShadowData& out);

    /** The sun cascades of the last render(): a depth32float array texture, or a one layer
        placeholder when no sun casts. */
    GpuTexture::Ptr getCascadeTexture() const noexcept;

    /** The point and spot light atlas of the last render(): a depth32float texture, or a 1x1
        placeholder when no point or spot light is shadowed. */
    GpuTexture::Ptr getAtlasTexture() const noexcept;

    /** For each light of the last render()'s draw list, its first shadow record, -1 when the light
        has neither a shadow map nor contact shadows (a contact shadow only record has no map). */
    Span<const int> getShadowRecordIndices() const noexcept { return recordIndices; }

    /** True when the last render() asks the shading passes for their shadow filter variants
        (YUP_3D_SOFT_SHADOWS, LightingInputs::shadowFilters): when its records ask for soft
        filtering (GpuShadowData::filterInfo x), or when the tier marches contact shadows and a
        record holds a contact shadow distance (contact[0] > 0).
    */
    bool needsShadowFilters() const noexcept { return shadowFilters; }

    /** Forgets the rendered maps, so the next render() draws them again: for frames the GPU failed
        to execute, or that were abandoned after their shadow passes were recorded. */
    void invalidate() noexcept;

    /** @internal The number of render() calls that drew shadow maps, for tests. */
    int getNumRenderedFrames() const noexcept { return numRenderedFrames; }

    /** @internal The number of render() calls that drew sun cascade @p cascade, for tests. */
    int getNumCascadeRenders (int cascade) const noexcept;

    /** @internal The shadow records the last render() wrote, for tests. */
    int getNumRecords() const noexcept { return numRecords; }

    /** @internal The side in texels of the shadow atlas, 0 when no point or spot light needs one, for tests. */
    int getAtlasSize() const noexcept { return atlas != nullptr ? static_cast<int> (atlas->getWidth()) : 0; }

    /** @internal The side in texels of the atlas tiles of a light of the last render()'s draw list,
        0 when it has no record in the atlas, for tests. */
    int getTileSize (int lightIndex) const noexcept;

private:
    struct RenderedState;
    struct CascadeState;
    struct LocalView;

    /** A 1x1 depth texture standing in for a shadow map no light needs, cleared to 1 once. */
    struct Placeholder
    {
        GpuTexture::Ptr texture;
        bool cleared = false;
    };

    Result preparePlaceholder (const GpuDevice::Ptr& device, GpuFrame& frame, Placeholder& placeholder, bool array);
    bool prepareCascades (const GpuDevice::Ptr& device, int resolution, int numCascades);
    bool prepareAtlas (const GpuDevice::Ptr& device, int size);
    int planLocalShadows (const Inputs& inputs, int firstRecord, GpuShadowData& out, std::vector<LocalView>& views);
    void drawCasters (GpuRenderPass& pass, const GpuDevice::Ptr& device, const SceneDrawList& drawList, const Matrix4& viewProjection, const CascadeState* cascade);
    Result renderCascade (const GpuDevice::Ptr& device, GpuFrame& frame, const SceneDrawList& drawList, int layer, const CascadeState& cascade);
    Result renderAtlas (const GpuDevice::Ptr& device, GpuFrame& frame, const SceneDrawList& drawList, Span<const LocalView> views);

    GeometryBindings geometry;
    GpuPipeline::Ptr opaquePipeline;
    GpuPipeline::Ptr maskedPipeline;
    std::optional<Result> preparation;
    GpuTexture::Ptr cascades;
    std::vector<GpuTarget::Ptr> layerTargets;
    Placeholder cascadePlaceholder;
    std::unique_ptr<ShadowAtlas> atlasLayout;
    GpuTexture::Ptr atlas;
    GpuTarget::Ptr atlasTarget;
    Placeholder atlasPlaceholder;
    std::unique_ptr<RenderedState> rendered;
    std::unique_ptr<RenderedState> pending; // the state render() builds, kept so its lists keep their storage
    std::vector<int> recordIndices;
    std::vector<BoundingBox> casterBounds; // the world bounds of each opaque item of the last render()
    bool sceneFitted = false;              // the last render() fitted the sun's map to the scene, not the view
    int numRenderedFrames = 0;
    std::array<int, ShadowCascades::maxCascades> numCascadeRenders {};
    int numRecords = 0;
    int firstContactOnlyRecord = GpuShadowData::maxRecords; // the records from here on have no map
    bool shadowFilters = false;            // see needsShadowFilters()

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShadowRenderer)
};

} // namespace yup
