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
/** One light as the shaders read it: 64 bytes, laid out the same in std140 and std430.

    @internal
*/
struct GpuLight
{
    float positionType[4];     // xyz world position, w type (0 directional, 1 point, 2 spot)
    float directionRange[4];   // xyz unit direction the light shines towards, w range: 0 unbounded (directional), > 0 the node range, < 0 minus the effective range
    float colorIntensity[4];   // rgb linear color, w intensity
    float spotRadiusShadow[4]; // x cone scale, y cone offset, z radius, w first shadow record (GpuShadowData::records) or -1
};

static_assert (sizeof (GpuLight) == 64);

//==============================================================================
/** The material parameters beyond the core metallic-roughness model, packed for the GPU.

    It is the tail of the forward MaterialBlock (std140) and a material table entry (std430):
    both layouts are the same for two vec4 rows. Filled by GpuSceneData::packMaterial().

    @internal
*/
struct YUP_API GpuMaterial
{
    float specular[4]; ///< rgb specularColorFactor, w specularFactor (KHR_materials_specular).
    float extended[4]; ///< x ior (KHR_materials_ior), y emissiveStrength (KHR_materials_emissive_strength), zw reserved (0).
};

static_assert (sizeof (GpuMaterial) == 32);
static_assert (offsetof (GpuMaterial, extended) == 16);

//==============================================================================
/** One reflection probe as the shaders read it: 128 bytes, laid out the same in std140 and std430.
    Filled by GpuSceneData::packReflectionProbe().

    @internal
*/
struct YUP_API GpuReflectionProbe
{
    float worldToLocal[16]; ///< The inverse of the entity world matrix, column-major.
    float positionShape[4]; ///< xyz capture position in world space, w shape (0 sphere, 1 box).
    float influence[4];     ///< xyz half extents in entity space (sphere: the radius in x, yz 0), w falloff in entity space (0 or more).
    float atlasRect[4];     ///< UV of the level 0 tile origin (xy) and size (zw) in the reflection atlas.
    float params[4];        ///< x intensity, y parallax correction (0 or 1), z number of mips, w unused (0).
};

static_assert (sizeof (GpuReflectionProbe) == 128);
static_assert (offsetof (GpuReflectionProbe, positionShape) == 64);
static_assert (offsetof (GpuReflectionProbe, influence) == 80);
static_assert (offsetof (GpuReflectionProbe, atlasRect) == 96);
static_assert (offsetof (GpuReflectionProbe, params) == 112);

//==============================================================================
/** One shadow map view as the shaders read it: a sun cascade, a spot light or one face of a point
    light. std140, 112 bytes, mirrored by GpuShadowRecord in lib/shadows.glsl.

    A light with contact shadows but no shadow map still takes one record, with atlasRect size 0.

    @internal
*/
struct YUP_API GpuShadowRecord
{
    float viewProjection[16]; ///< World to shadow clip space, depth 0 to 1, column-major.
    float atlasRect[4];       ///< u, v, size in UV units, 1 / atlas size (cascades: layer, 0, 1, 1 / resolution); size 0 means no map.
    float params[4];          ///< Depth bias, normal offset in world units (point and spot lights: per unit of depth from the light), near, far.
    float contact[4];         ///< (contact shadow march distance (0 off), contact shadow thickness in world units, number of views of the light (point and spot lights: 1 or 6), 0). Writers of the contact shadow distance and thickness write contact[0] and contact[1] only: contact[2] selects the faces of a point light.
};

static_assert (sizeof (GpuShadowRecord) == 112);

//==============================================================================
/** The shadow data of a frame, read by the shading passes as ShadowBlock (set 3, binding 4).
    std140, mirrored by GpuShadowData in lib/shadows.glsl.

    Cascades occupy the first records, then each shadowed point light takes 6 consecutive records
    (+X, -X, +Y, -Y, +Z, -Z) and each spot light 1, or 6 like a point light when its cone is too
    wide for one view (see ShadowAtlas::makeRequest()).

    @internal
*/
struct YUP_API GpuShadowData
{
    /** The records the block holds. */
    static constexpr int maxRecords = 64;

    float cascadeSplits[4];              ///< The view-space far distance of each cascade.
    float cascadeTexel[4];               ///< The world size of one texel of each cascade.
    float sunInfo[4];                    ///< Cascade count, unused (0), cascade blend band fraction, 1 when receivers lie beyond the last cascade (it then fades out).
    float filterInfo[4];                 ///< Soft shadows (1 when on and a light with a shadow map has a radius, else 0), PCF taps, blocker search taps, flags (1 contact shadows, 2 temporal noise).
    GpuShadowRecord records[maxRecords]; ///< The shadow map views, cascades first.
};

static_assert (sizeof (GpuShadowData) == 64 + sizeof (GpuShadowRecord) * GpuShadowData::maxRecords);
static_assert (sizeof (GpuShadowData) <= 16384, "ShadowBlock must fit the WebGL2 minimum uniform block size");

//==============================================================================
/** CPU mirrors of the scene data the shaders read: the octahedral unit vector encoding in
    lib/octahedral.glsl, the packed lights and the packed materials.

    @internal
*/
class YUP_API GpuSceneData
{
public:
    //==============================================================================
    /** Octahedral encoding of a unit vector into [-1, 1]^2 (Cigolle et al. 2014), mirroring lib/octahedral.glsl. */
    static Point<float> encodeOctahedral (const Vector3<float>& unitVector) noexcept;

    /** Inverse of encodeOctahedral, returning a unit vector. */
    static Vector3<float> decodeOctahedral (Point<float> encoded) noexcept;

    //==============================================================================
    /** Intensity a light contributes below which it is culled, in the same unit as the lit color. */
    static constexpr float lightCutoff = 0.005f;

    /** True when a light of any type contributes nothing: an intensity of 0 or less, or no color
        channel above 0. Dark lights are culled everywhere and never take a light slot.
    */
    static bool isDark (const LightNode& light) noexcept;

    /** The distance beyond which a point or spot light is culled: 0 for a dark light (see isDark()),
        its range when set, otherwise where intensity * the brightest color channel / d^2 drops
        under lightCutoff. Directional lights return 0 (unbounded).
    */
    static float getEffectiveRange (const LightNode& light) noexcept;

    /** True when a point or spot light can reach a box: the box must meet the sphere of @p range
        around the light and, for a spot light, the sphere bounding its cone (Wronski, "Cull that
        cone", 2017). A range of 0 reaches nothing. Box, position and direction share one space.

        @param box        The box to test.
        @param position   The position of the light.
        @param direction  The unit direction a spot light shines towards.
        @param range      The effective range of the light.
        @param isSpot     True for a spot light.
        @param cosOuter   The cosine of the outer cone angle of a spot light.
    */
    static bool lightReachesBox (const BoundingBox& box, const Vector3<float>& position, const Vector3<float>& direction, float range, bool isSpot, float cosOuter) noexcept;

    /** Packs a light. @p shadowIndex is its first shadow record in GpuShadowData::records, or -1.

        directionRange.w is 0 for a directional light and for a dark one (see isDark()), the node
        range when it is set, and minus the effective range otherwise: the shaders cull at abs (w)
        and window the falloff only where w > 0, as for a light with a range.
    */
    static GpuLight packLight (const SceneDrawList::Light& light, int shadowIndex) noexcept;

    /** Packs every light: directional lights first, in list order, then the others in list order.

        @param lights             The lights to pack.
        @param firstShadowRecord  For each of @p lights, in the same order, its first shadow record or
                                  -1 (ShadowRenderer::getShadowRecordIndices()); empty for none.
        @param order              Receives, for each packed slot, the index into @p lights it came from.
    */
    static std::vector<GpuLight> packLights (Span<const SceneDrawList::Light> lights, Span<const int> firstShadowRecord, std::vector<uint32>& order);

    /** Packs the specular, index of refraction and emissive strength of a material. */
    static GpuMaterial packMaterial (const Material& material) noexcept;

    /** Packs a reflection probe.

        @param probe      The probe, with its node.
        @param atlasRect  The UV of the level 0 tile origin and size of the probe in the reflection atlas.
        @param numMips    The number of mips of the tile.
    */
    static GpuReflectionProbe packReflectionProbe (const SceneDrawList::ReflectionProbe& probe, const Rectangle<float>& atlasRect, int numMips) noexcept;
};

} // namespace yup
