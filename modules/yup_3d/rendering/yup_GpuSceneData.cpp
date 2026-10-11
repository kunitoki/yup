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

namespace yup
{

//==============================================================================
namespace
{

float octahedralSignNotZero (float value) noexcept
{
    return value >= 0.0f ? 1.0f : -1.0f;
}

} // namespace

Point<float> GpuSceneData::encodeOctahedral (const Vector3<float>& unitVector) noexcept
{
    const auto x = unitVector.getX();
    const auto y = unitVector.getY();
    const auto z = unitVector.getZ();
    const auto l1Norm = std::abs (x) + std::abs (y) + std::abs (z);

    const auto px = x / l1Norm;
    const auto py = y / l1Norm;
    if (z >= 0.0f)
        return { px, py };

    return { (1.0f - std::abs (py)) * octahedralSignNotZero (px),
             (1.0f - std::abs (px)) * octahedralSignNotZero (py) };
}

Vector3<float> GpuSceneData::decodeOctahedral (Point<float> encoded) noexcept
{
    const auto ex = encoded.getX();
    const auto ey = encoded.getY();
    const auto z = 1.0f - std::abs (ex) - std::abs (ey);

    if (z >= 0.0f)
        return Vector3<float> (ex, ey, z).normalized();

    return Vector3<float> ((1.0f - std::abs (ey)) * octahedralSignNotZero (ex),
                           (1.0f - std::abs (ex)) * octahedralSignNotZero (ey),
                           z)
        .normalized();
}

//==============================================================================
bool GpuSceneData::isDark (const LightNode& light) noexcept
{
    return ! (light.intensity > 0.0f && jmax (light.color[0], light.color[1], light.color[2]) > 0.0f);
}

float GpuSceneData::getEffectiveRange (const LightNode& light) noexcept
{
    if (light.type == LightNode::Type::directional || isDark (light))
        return 0.0f;

    if (light.range > 0.0f)
        return light.range;

    const auto brightest = jmax (light.color[0], light.color[1], light.color[2]);
    return std::sqrt (light.intensity * brightest / lightCutoff);
}

bool GpuSceneData::lightReachesBox (const BoundingBox& box, const Vector3<float>& position, const Vector3<float>& direction, float range, bool isSpot, float cosOuter) noexcept
{
    if (! (range > 0.0f) || box.getDistanceSquared (position) > range * range)
        return false;

    // A narrow cone is bounded by the sphere through its apex and rim, a wide one by the sphere
    // around its rim; past 90 degrees only the range sphere bounds it
    if (! isSpot || cosOuter <= 0.0f)
        return true;

    const auto narrow = cosOuter >= 0.70710678f;
    const auto radius = narrow ? range * 0.5f / cosOuter : range * std::sqrt (jmax (0.0f, 1.0f - cosOuter * cosOuter));
    const auto offset = narrow ? radius : range * cosOuter;

    return box.getDistanceSquared (position + direction * offset) <= radius * radius;
}

GpuLight GpuSceneData::packLight (const SceneDrawList::Light& light, int shadowIndex) noexcept
{
    const auto& node = *light.node;
    const auto effectiveRange = getEffectiveRange (node);
    const auto range = node.type == LightNode::Type::directional ? 0.0f
                     : node.range > 0.0f                         ? effectiveRange
                                                                 : -effectiveRange;

    // KHR_lights_punctual cone falloff: clamp (cos * scale + offset)
    const auto cosOuter = std::cos (node.outerConeAngle);
    const auto cosInner = std::cos (node.innerConeAngle);
    const auto scale = 1.0f / jmax (0.001f, cosInner - cosOuter);

    return {
        { light.position.getX(), light.position.getY(), light.position.getZ(), static_cast<float> (node.type) },
        { light.direction.getX(), light.direction.getY(), light.direction.getZ(), range },
        { node.color[0], node.color[1], node.color[2], node.intensity },
        { scale, -cosOuter * scale, node.radius, static_cast<float> (shadowIndex) }
    };
}

std::vector<GpuLight> GpuSceneData::packLights (Span<const SceneDrawList::Light> lights, Span<const int> firstShadowRecord, std::vector<uint32>& order)
{
    jassert (firstShadowRecord.empty() || firstShadowRecord.size() == lights.size());

    order.resize (lights.size());
    std::iota (order.begin(), order.end(), 0u);
    std::stable_partition (order.begin(), order.end(), [&lights] (uint32 index)
    {
        return lights[index].node->type == LightNode::Type::directional;
    });

    std::vector<GpuLight> packed;
    packed.reserve (order.size());

    for (const auto index : order)
        packed.push_back (packLight (lights[index], index < firstShadowRecord.size() ? firstShadowRecord[index] : -1));

    return packed;
}

GpuMaterial GpuSceneData::packMaterial (const Material& material) noexcept
{
    const auto& color = material.specularColorFactor;

    return { { color[0], color[1], color[2], material.specularFactor },
             { material.ior, material.emissiveStrength, 0.0f, 0.0f } };
}

GpuReflectionProbe GpuSceneData::packReflectionProbe (const SceneDrawList::ReflectionProbe& probe, const Rectangle<float>& atlasRect, int numMips) noexcept
{
    const auto& node = *probe.node;
    const auto isBox = node.shape == ReflectionProbeNode::Shape::box;
    const auto& extents = node.influenceExtents;

    GpuReflectionProbe packed {
        {},
        { probe.position.getX(), probe.position.getY(), probe.position.getZ(), isBox ? 1.0f : 0.0f },
        { isBox ? extents.getX() : node.influenceRadius, isBox ? extents.getY() : 0.0f, isBox ? extents.getZ() : 0.0f, jmax (0.0f, node.falloff) },
        { atlasRect.getX(), atlasRect.getY(), atlasRect.getWidth(), atlasRect.getHeight() },
        { node.intensity, node.parallaxCorrection ? 1.0f : 0.0f, static_cast<float> (numMips), 0.0f }
    };

    const auto worldToLocal = probe.world.inverted();
    std::copy_n (worldToLocal.getData(), 16, packed.worldToLocal);

    return packed;
}

} // namespace yup
