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
    ShadowRenderer, the area light tables it owns for its device and the shared samplers of its
    FrameResources, and the full tier adds the storage buffers and the cluster grid
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
    };

    //==============================================================================
    /** Fills the inputs of a frame: the ambient light and the environment of @p scene (uploaded
        the first time @p device draws it), and @p lights packed by GpuSceneData::packLights() with
        their first shadow records, @p firstShadowRecord (ShadowRenderer::getShadowRecordIndices()).
    */
    static LightingInputs create (const GpuDevice::Ptr& device, const Scene& scene,
                                  Span<const SceneDrawList::Light> lights, Span<const int> firstShadowRecord);

    /** True when a light of @p lights has a radius: the frame then shades with the area light
        variants of ForwardPass and LightingPass, the only ones reading the area light tables.
    */
    static bool hasAreaLights (Span<const SceneDrawList::Light> lights) noexcept;

    /** True when the environment shows behind the scene. */
    bool isEnvironmentVisible() const noexcept { return block.ambient[3] > 0.5f; }

    /** The packed light of a SceneDrawList::getLights() index. */
    const GpuLight& getPackedLight (size_t listIndex) const noexcept { return packedLights[packedSlots[listIndex]]; }

    //==============================================================================
    Block block {};                      ///< The LightingBlock at set 1 binding 0.
    GpuTexture::Ptr environment;         ///< The environment (set 0 binding 6), or nullptr without one.
    GpuSampler::Ptr environmentSampler;  ///< Its interim sampler (set 0 binding 14), or nullptr without an environment.
    GpuTexture::Ptr shadowCascades;      ///< The sun cascades (set 3 binding 0), ShadowRenderer::getCascadeTexture().
    GpuTexture::Ptr shadowAtlas;         ///< The point and spot light shadow atlas (set 3 binding 2), ShadowRenderer::getAtlasTexture().
    GpuShadowData shadows {};            ///< The ShadowBlock (set 3 binding 4): the shadow records ShadowRenderer::render() wrote.
    GpuTexture::Ptr ltcMatrix;           ///< The area light inverse matrices (set 0 binding 7), from LtcTables::createMatrixTexture().
    GpuTexture::Ptr ltcAmplitude;        ///< The area light amplitudes (set 0 binding 11), from LtcTables::createAmplitudeTexture().
    FrameResources::Samplers samplers;   ///< The shared set 0 samplers, from FrameResources::getSamplers().
    bool areaLights = false;             ///< hasAreaLights() of the lights: the passes shade with their area light variant, and bind ltcMatrix and ltcAmplitude.
    std::vector<GpuLight> packedLights;  ///< Every light, directional ones first.
    std::vector<uint32> packedSlots;     ///< For each SceneDrawList::getLights() index, its slot in packedLights.
    GpuBuffer::Ptr lights;               ///< Full tier: packedLights in a storage buffer (set 1 binding 6).
    GpuBuffer::Ptr clusterLights;        ///< Full tier: the light lists of the clusters (set 1 binding 7).
    GpuBuffer::Ptr materials;            ///< Full tier: the frame's MaterialTable (set 1 binding 8), read by LightingPass.
};

static_assert (sizeof (LightingInputs::Block) == 208);

} // namespace yup
