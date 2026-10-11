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
    and at most 4096. A tile is at most half the atlas side while every light still gets at least
    the tile it would get with tiles of at most a quarter of it, and at most a quarter otherwise, so
    a few lights get the sharpest shadows and more lights still fit. The layout follows the lights
    and their size on screen, with no hysteresis: a light coming or going, or growing on screen past
    a quarter of the atlas, can move and resize the other lights' views, which are drawn again.
    6 perspective views for a point light (and for a spot light too wide for one view), 1 for a
    spot light, each spanning a guard band
    (ShadowAtlas::getGuardAngle()) beyond its field of view, from max (0.01, LightNode::radius) to
    the effective range of the light. Lights take tiles from the most to the least important, as the
    atlas serves them, while their views fit the shadow records left after the cascades, at most 16
    on raster: a light whose views do not all fit is left unshadowed. The tiles drawn in a frame
    share one depth-only pass. The scene fit takes one record where the cascades take the tier's
    cascade count, so switching between them changes how many records are left.

    When the tier enables contact shadows (RenderCapabilities::Feature::contactShadows, filterInfo
    flag 1), each light with LightNode::contactShadows that is not dark carries its march distance
    and thickness in contact[0] and contact[1] of its first record. A light without a shadow map
    takes one record without a map (atlasRect size 0) for them, after the atlas views and within the
    same records, while any are left, from the most important light as the atlas serves them:
    directional lights first, then larger tiles (ShadowAtlas::getLightTileSize() with tiles of at
    most a quarter of the atlas, whichever layout the frame uses), then lower light indices.

    The renderer owns the cascade texture and the atlas: each is created when a light needs it, kept
    while it fits, and released when no light needs it, a 1x1 placeholder standing in for it. The
    maps are rendered again only when what they hold changes: a cascade when the shadow casting
    directional lights or the shadow settings (Inputs::globalSignature), its matrix, or the casters
    reaching it and their signatures (Inputs::casterSignatures) change; an atlas view when its tile,
    its matrix, or the casters within its light's reach (GpuSceneData::lightReachesBox()) and its
    frustum and their signatures change. Each view draws only those casters, and a view drawn
    again while other tiles are kept is first cleared alone (a quad at the far plane, scissored to
    its tile). invalidate() forgets every map, for frames the GPU did not execute.

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
        uint64 globalSignature = 0;             ///< What every cascade depends on besides its fit and casters: the shadow casting directional lights and the settings.
        Span<const uint64> casterSignatures;    ///< One hash per opaque item of drawList, in order: what it draws into a shadow map. Without one per item every map is drawn every frame.
        bool enabled = true;                    ///< False when the shadow comparison sampler is missing: nothing is shadowed.
    };

    //==============================================================================
    /** Creates a renderer. GPU objects are created by prepare() and render(). */
    ShadowRenderer();

    /** Destructor. */
    ~ShadowRenderer();

    //==============================================================================
    /** Compiles the shadow pipelines once, remembering a failure instead of retrying every frame.
        After a failure, logged once, render() writes no record, as without the comparison sampler
        (Inputs::enabled): the frames render without shadow maps and without contact shadows.
    */
    Result prepare (const GpuDevice::Ptr& device);

    /** Renders every shadow map the frame needs into @p frame, unless the maps it holds still fit,
        and writes the shadow records of the frame, which getShadowData() returns.

        A cascade texture or an atlas the device cannot create leaves the lights that needed it
        without a shadow map, logged once: their records are -1, or contact shadow only records, so
        their contact shadows stay. The same size (for the cascades, the same resolution, the scene
        fit's single layer included) is not asked for again until the settings change it, and the
        other map keeps what it holds.

        @returns A failure when a placeholder cannot be created or a shadow pass cannot be
                 recorded; the frame must then be abandoned.
    */
    Result render (const GpuDevice::Ptr& device, GpuFrame& frame, const Inputs& inputs);

    /** The shadow records and sun parameters the last render() wrote (the ShadowBlock). */
    const GpuShadowData& getShadowData() const noexcept { return shadowData; }

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

    /** @internal The number of casters drawn into shadow maps since the renderer was created, for tests and benchmarks. */
    int getNumCasterDraws() const noexcept { return numCasterDraws; }

    /** @internal Test hook: from the next prepare() on, the pipelines fail to compile. */
    void failPipelinesForTesting() noexcept { pipelinesFailForTesting = true; }

    /** @internal Test hook: the cascade texture and the atlas the next render() creates fail to create. */
    void failTexturesForTesting() noexcept { texturesFailForTesting = true; }

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

    Result preparePipelines (const GpuDevice::Ptr& device);
    Result preparePlaceholder (const GpuDevice::Ptr& device, GpuFrame& frame, Placeholder& placeholder, bool array);
    bool prepareCascades (const GpuDevice::Ptr& device, int resolution, int numCascades);
    bool prepareAtlas (const GpuDevice::Ptr& device, int size);
    int planLocalShadows (const Inputs& inputs, bool withAtlas, int firstRecord, GpuShadowData& out, std::vector<LocalView>& views);
    void drawCasters (GpuRenderPass& pass, const GpuDevice::Ptr& device, const SceneDrawList& drawList, const Matrix4& viewProjection, Span<const std::pair<size_t, uint64>> casters);
    bool clearTile (GpuRenderPass& pass, const GpuDevice::Ptr& device);
    Result renderCascade (const GpuDevice::Ptr& device, GpuFrame& frame, const SceneDrawList& drawList, int layer, const CascadeState& cascade);
    Result renderAtlas (const GpuDevice::Ptr& device, GpuFrame& frame, const SceneDrawList& drawList, Span<const LocalView> views, const std::vector<bool>& redraw, bool keepOtherTiles);

    GeometryBindings geometry;
    GpuPipeline::Ptr opaquePipeline;
    GpuPipeline::Ptr maskedPipeline;
    GpuPipeline::Ptr tileClearPipeline; // draws tileClearQuad at depth 1 over a tile, for redrawing it alone
    Mesh::Ptr tileClearQuad;
    Material::Ptr tileClearMaterial;
    std::optional<Result> preparation;
    GpuTexture::Ptr cascades;
    std::vector<GpuTarget::Ptr> layerTargets;
    Placeholder cascadePlaceholder;
    std::unique_ptr<ShadowAtlas> halfTileLayout;    // tiles up to half the atlas side
    std::unique_ptr<ShadowAtlas> quarterTileLayout; // tiles up to a quarter of it, which fit more lights
    const ShadowAtlas* atlasLayout = nullptr;       // the one of the last render(), see planLocalShadows()
    GpuTexture::Ptr atlas;
    GpuTarget::Ptr atlasTarget;
    Placeholder atlasPlaceholder;
    std::unique_ptr<RenderedState> rendered;
    std::unique_ptr<RenderedState> pending; // the state render() builds, kept so its lists keep their storage
    GpuShadowData shadowData {};
    std::vector<int> recordIndices;
    std::vector<BoundingBox> casterBounds; // the world bounds of each opaque item of the last render()
    std::vector<BoundingBox> receivers;    // reused by planLocalShadows(), as the lists below
    std::vector<ShadowAtlas::Request> requests;
    std::vector<std::pair<int, size_t>> contactLights; // the tile size ranking a light, its index
    std::vector<bool> redrawViews;         // for each atlas view of the frame, whether it is drawn again
    int failedCascadeResolution = 0;       // the cascade resolution the device could not create
    int failedAtlasSize = 0;               // the atlas size the device could not create
    bool sceneFitted = false;              // the last render() fitted the sun's map to the scene, not the view
    int numRenderedFrames = 0;
    std::array<int, ShadowCascades::maxCascades> numCascadeRenders {};
    int numCasterDraws = 0;
    int numRecords = 0;
    int firstContactOnlyRecord = GpuShadowData::maxRecords; // the records from here on have no map
    bool shadowFilters = false;            // see needsShadowFilters()
    bool pipelinesFailForTesting = false;
    bool texturesFailForTesting = false;
    bool failTextures = false;             // texturesFailForTesting, for the current render()

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShadowRenderer)
};

} // namespace yup
