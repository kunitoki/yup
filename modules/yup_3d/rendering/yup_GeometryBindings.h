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
/** What the passes drawing meshes (ForwardPass, GBufferPass) share: the Mesh::Vertex pipeline
    layout, the cull and winding state per draw, and the set 2 bindings of a draw.

    Set 2 holds the DrawBlock (binding 0, lib/geometry.glsl), the MaterialBlock (binding 1,
    lib/material.glsl), and the five material textures (bindings 2-6) with their samplers
    (bindings 7-11). Missing textures are replaced by white, or by a flat normal for the normal
    texture. The raster ForwardPass adds its LightListBlock at binding 12.

    The DrawBlock, the MaterialBlock and the LightListBlock are dynamic uniform buffers (see
    GpuPipelineOptions::dynamicUniformBuffers): a draw only moves their offsets, so set 2 is built
    again only when the textures or samplers change, or when the frame's uniform buffer fills up.
    Pipelines whose shaders lack some of these blocks, such as the shadow pass, ignore them.

    @internal
*/
class YUP_API GeometryBindings
{
public:
    //==============================================================================
    /** std140 mirror of DrawBlock in lib/geometry.glsl. */
    struct DrawBlock
    {
        float model[16];
        float normalMatrix[16];  ///< Inverse transpose of the model matrix.
        float previousModel[16]; ///< The model matrix of the previous frame.
        float ids[4];            ///< x material id in the frame's MaterialTable, yzw reserved.
    };

    /** std140 mirror of MaterialBlock in lib/material.glsl. */
    struct MaterialBlock
    {
        float baseColorFactor[4];
        float emissiveFactor[4]; ///< rgb emissive, w alpha cutoff.
        float params[4];         ///< Metallic, roughness, normal scale, occlusion strength.
        float flags[4];          ///< Alpha mode, base color sRGB, emissive sRGB, has normal texture.
        GpuMaterial extension;   ///< Specular, ior and emissive strength.
    };

    //==============================================================================
    /** The number of cull and winding states, see getRasterState(). */
    static constexpr size_t numRasterStates = 4;

    /** The cull and winding state of an item: doubleSided * 2 + mirrored. */
    static size_t getRasterState (const SceneDrawList::DrawItem& item) noexcept;

    /** Sets the Mesh::Vertex layout (attributes 0-4), indexed triangles, the culling and winding
        of a raster state from getRasterState(), and the set 2 blocks as dynamic uniform buffers.
    */
    static void setPipelineGeometry (GpuPipelineOptions& options, size_t rasterState);

    /** Packs the transforms of an item, and the material id the G-buffer stores for it. */
    static DrawBlock makeDrawBlock (const SceneDrawList::DrawItem& item, int materialId) noexcept;

    /** Packs a material. @p hasNormalTexture tells whether its normal texture could be uploaded. */
    static MaterialBlock makeMaterialBlock (const Material& material, bool hasNormalTexture) noexcept;

    /** Creates a 1x1 rgba8 texture of one color, 0xRRGGBBAA, or returns nullptr. */
    static GpuTexture::Ptr createSolidTexture (const GpuDevice::Ptr& device, uint32 rgba);

    //==============================================================================
    /** Creates the bindings. GPU objects are created by prepare(). */
    GeometryBindings();

    /** Destructor. */
    ~GeometryBindings();

    /** Creates the textures and the sampler standing in for missing material textures. */
    Result prepare (const GpuDevice::Ptr& device);

    /** Binds set 2 for an item and its vertex and index buffers, then draws it.

        Bindings the pass adds to set 2 (such as the ForwardPass light list) must be set before.

        @returns False if the mesh has no GPU buffers, and nothing was drawn.
    */
    bool draw (GpuRenderPass& pass, const GpuDevice::Ptr& device, const SceneDrawList::DrawItem& item, int materialId) const;

    /** The 1x1 opaque white stand-in texture, after prepare(). */
    const GpuTexture::Ptr& getWhiteTexture() const noexcept { return whiteTexture; }

private:
    GpuTexture::Ptr whiteTexture;
    GpuTexture::Ptr flatNormalTexture;
    GpuSampler::Ptr defaultSampler;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GeometryBindings)
};

} // namespace yup
