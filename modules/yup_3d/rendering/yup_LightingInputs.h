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
/** What the shading passes read besides the geometry, shared by ForwardPass and LightingPass so
    both shade from the same data: the LightingBlock, the set 0 lookups and shared samplers, the
    set 3 shadow maps with their ShadowBlock, and the packed lights.

    create() fills everything the scene decides, and packs the lights with the shadow records
    ShadowRenderer gave them. The renderer then sets the shadow maps and records of its
    ShadowRenderer, the lookup tables it owns for its device, the shared samplers of its
    FrameResources and the atlases, probes and volumes of its ProbeBaker, and the full tier adds
    the storage buffers and the cluster grid
    (LightClusters::packGrid into Block::clusterGrid and Block::clusterDepth) before the passes draw.

    @internal
*/
struct YUP_API LightingInputs
{
    //==============================================================================
    /** std140 mirror of LightingBlock in lib/lighting.glsl. */
    struct Block
    {
        float ambient[4];         ///< rgb linear ambient, w 1 when the environment shows behind the scene.
        float environmentInfo[4]; ///< x intensity (0 for none), y last level, z background blur level, w output mode (ForwardPass::Output).
        float irradiance[9][4];   ///< rgb spherical harmonics coefficients of the environment diffuse light.
        float clusterGrid[4];     ///< Full tier: tiles x, tiles y, depth slices, number of lights.
        float clusterDepth[4];    ///< Full tier: near, far, number of directional lights, 1 for an infinite far plane.
        float probeInfo[4];       ///< x 1 when the environment, probe and volume textures hold RGBM; y the number of reflection probes of the frame (reflectionProbes or reflectionProbeBuffer); z the number of irradiance volumes of the frame (irradianceVolumes or irradianceVolumeBuffer); w 1 when the frame has ambientOcclusion (both tiers; LightingPass reads it).
    };

    //==============================================================================
    /** The environment cube one renderer built on its device, and the state of that build. */
    struct EnvironmentCube
    {
        EnvironmentMap::Ptr environment;  ///< The environment the cube was built from, held so its address is never reused.
        GpuTexture::Ptr cube;             ///< The cube, nullptr until built or once given up.
        GpuTexture::Ptr image;            ///< EnvironmentMap::CubeBuild::image, kept with the cube: the frame does not retain its mip pipeline.
        GpuFrameStatus status;            ///< The pending build, until its outcome is known.
        int consecutiveFailures = 0;      ///< Builds the GPU failed to execute in a row.
        bool givenUp = false;             ///< Two failures in a row: the renderer shades without the environment.
    };

    /** Returns the environment cube of @p scene, or nullptr when the scene has no environment or a
        dark one, or the cube could not be built.

        The cube is kept in @p slot, one per renderer, and built with @p probes in a GpuFrame of its
        own (EnvironmentMap::buildCube()) when the scene's environment changes, so call this before
        beginning the frame that shades with it. The build is checked without blocking: a build the
        GPU failed to execute is built once more, and a second failure in a row leaves the renderer
        without the environment until it changes.
    */
    static GpuTexture::Ptr prepareEnvironment (ProbeRenderer& probes, const Scene& scene, EnvironmentCube& slot);

    /** Fills the inputs of a frame: the ambient light and the environment of @p scene, its cube
        being @p environmentCube (prepareEnvironment()), and @p lights packed by
        GpuSceneData::packLights() with their first shadow records, @p firstShadowRecord
        (ShadowRenderer::getShadowRecordIndices()). The renderer sets Block::probeInfo from the
        radiance format of its ProbeRenderer.
    */
    static LightingInputs create (const Scene& scene, const GpuTexture::Ptr& environmentCube,
                                  Span<const SceneDrawList::Light> lights, Span<const int> firstShadowRecord);

    /** Creates the 1x1 black cube the shading passes bind at set 0 binding 6 without an environment. */
    static GpuTexture::Ptr createBlackCube (const GpuDevice::Ptr& device);

    /** True when a light of @p lights has a radius: the frame then shades with the area light
        variants of ForwardPass and LightingPass, the only ones shading area lights.
    */
    static bool hasAreaLights (Span<const SceneDrawList::Light> lights) noexcept;

    /** True when an item of @p items is drawn with a material that has a clearcoat or a sheen
        (GpuSceneData::hasLayers()): the frame then shades with the variants compiling the layers.
    */
    static bool hasMaterialLayers (Span<const SceneDrawList::DrawItem> items) noexcept;

    /** True when the frame shades with reflection probes (Block::probeInfo y). */
    bool hasReflectionProbes() const noexcept { return block.probeInfo[1] > 0.5f; }

    /** True when the frame shades with irradiance volumes (Block::probeInfo z). */
    bool hasIrradianceVolumes() const noexcept { return block.probeInfo[2] > 0.5f; }

    /** True when the environment shows behind the scene. */
    bool isEnvironmentVisible() const noexcept { return block.ambient[3] > 0.5f; }

    /** The packed light of a SceneDrawList::getLights() index. */
    const GpuLight& getPackedLight (size_t listIndex) const noexcept { return packedLights[packedSlots[listIndex]]; }

    //==============================================================================
    Block block {};                      ///< The LightingBlock at set 1 binding 0.
    GpuTexture::Ptr environment;         ///< The environment cube (set 0 binding 6), or nullptr without one.
    GpuTexture::Ptr shadowCascades;      ///< The sun cascades (set 3 binding 0), ShadowRenderer::getCascadeTexture().
    GpuTexture::Ptr shadowAtlas;         ///< The point and spot light shadow atlas (set 3 binding 2), ShadowRenderer::getAtlasTexture().
    GpuShadowData shadows {};            ///< The ShadowBlock (set 3 binding 4): the shadow records ShadowRenderer::render() wrote.
    GpuTexture::Ptr lookupTables;        ///< The lookup array (set 0 binding 7), from LookupTables, or its black stand-in.
    FrameResources::Samplers samplers;   ///< The shared set 0 samplers, from FrameResources::getSamplers().
    bool areaLights = false;             ///< hasAreaLights() of the lights: the passes shade with their area light variant.
    bool shadowFilters = false;          ///< ShadowRenderer::needsShadowFilters(): the passes shade with their shadow filter variant (soft shadows, and contact shadows in LightingPass), and bind the raw depth maps.
    bool materialLayers = false;         ///< hasMaterialLayers() of the items the pass shades: the raster ForwardPass and LightingPass shade with their local indirect layers variant (YUP_3D_LAYERS), the only one compiling the clearcoat and sheen layers (the full ForwardPass always compiles them).
    std::vector<GpuLight> packedLights;  ///< Every light, directional ones first.
    std::vector<uint32> packedSlots;     ///< For each SceneDrawList::getLights() index, its slot in packedLights.
    GpuBuffer::Ptr lights;               ///< Full tier: packedLights in a storage buffer (set 1 binding 6).
    GpuBuffer::Ptr clusterLights;        ///< Full tier: the light lists of the clusters (set 1 binding 7).
    GpuBuffer::Ptr materials;            ///< Full tier: the frame's MaterialTable (set 1 binding 8), read by LightingPass.
    GpuTexture::Ptr ambientOcclusion;    ///< The AmbientOcclusionPass output (set 1 binding 13), or nullptr without one. With it the raster ForwardPass shades with its local indirect variant; LightingPass binds white without it.
    GpuTexture::Ptr reflections;         ///< Full: the ReflectionPass output (LightingPass set 1 binding 16), or nullptr without screen-space reflections: LightingPass binds transparent black, a confidence of 0.
    GpuTexture::Ptr reflectionAtlas;     ///< The reflection atlas (set 0 binding 8), ProbeBaker::getReflectionAtlas(), or nullptr without probes: the passes bind black.
    std::vector<GpuReflectionProbe> reflectionProbes; ///< Raster: every probe of the draw list packed in its order (ProbeBaker::packReflectionProbes()), which each draw picks by SceneDrawList::DrawItem::reflectionProbes into its ProbeListBlock. Empty without probes.
    GpuBuffer::Ptr reflectionProbeBuffer; ///< Full tier: the probes of the frame by priority (ProbeBaker::packReflectionProbesByPriority()), set 1 binding 14 of LightingPass and 8 of ForwardPass, sized for one probe at least.
    GpuTexture::Ptr irradianceAtlas;     ///< The irradiance atlas (set 0 binding 9), ProbeBaker::getIrradianceAtlas(), or nullptr without volumes: the passes bind black.
    GpuTexture::Ptr visibilityAtlas;     ///< The visibility atlas (set 0 binding 10), ProbeBaker::getVisibilityAtlas(), or nullptr without volumes or below the float floor: the passes bind black.
    std::vector<GpuIrradianceVolume> irradianceVolumes; ///< Raster: every volume of the draw list packed in its order (ProbeBaker::packIrradianceVolumes()), which each draw picks by SceneDrawList::DrawItem::irradianceVolume into its ProbeListBlock. Empty without volumes.
    GpuBuffer::Ptr irradianceVolumeBuffer; ///< Full tier: the volumes of the frame by priority (ProbeBaker::packIrradianceVolumesByPriority()), set 1 binding 15 of LightingPass and 9 of ForwardPass, sized for one volume at least.
};

static_assert (sizeof (LightingInputs::Block) == 224);

} // namespace yup
