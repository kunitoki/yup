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
    float spotRadiusShadow[4]; // x cone scale, y cone offset, z radius, w shadow index or -1
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

    /** The distance beyond which a point or spot light is culled: its range when set, otherwise
        where intensity * the brightest color channel / d^2 drops under lightCutoff. Directional
        lights return 0 (unbounded).
    */
    static float getEffectiveRange (const LightNode& light) noexcept;

    /** Packs a light. @p shadowIndex is the shadow map slot or -1.

        directionRange.w is 0 for a directional light, the node range when it is set, and minus
        the effective range otherwise: the shaders cull at abs (w) and window the falloff only
        where w > 0, as for a light with a range.
    */
    static GpuLight packLight (const SceneDrawList::Light& light, int shadowIndex) noexcept;

    /** Packs every light: directional lights first, in list order, then the others in list order.

        @param lights         The lights to pack.
        @param shadowedLight  The index into @p lights of the light using shadow slot 0, or -1.
        @param order          Receives, for each packed slot, the index into @p lights it came from.
    */
    static std::vector<GpuLight> packLights (Span<const SceneDrawList::Light> lights, int shadowedLight, std::vector<uint32>& order);

    /** Packs the specular, index of refraction and emissive strength of a material. */
    static GpuMaterial packMaterial (const Material& material) noexcept;
};

} // namespace yup
