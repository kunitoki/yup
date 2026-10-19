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

static float getLinearDeterminant (const Matrix4& m) noexcept
{
    return m (0, 0) * (m (1, 1) * m (2, 2) - m (2, 1) * m (1, 2))
         - m (0, 1) * (m (1, 0) * m (2, 2) - m (2, 0) * m (1, 2))
         + m (0, 2) * (m (1, 0) * m (2, 1) - m (2, 0) * m (1, 1));
}

//==============================================================================

SceneDrawList::SceneDrawList()
    : defaultMaterial (new Material())
    , defaultLight (std::make_unique<LightNode>())
{
    defaultMaterial->name = "default";
    defaultMaterial->metallicFactor = 0.0f;
    defaultMaterial->roughnessFactor = 0.5f;

    defaultLight->intensity = 3.0f;
}

SceneDrawList::~SceneDrawList() = default;

//==============================================================================

void SceneDrawList::build (const Entity& root, const Matrix4& view)
{
    build (root, view, BuildOptions {});
}

void SceneDrawList::build (const Entity& root, const Matrix4& view, const BuildOptions& options)
{
    clear();

    collect (root, root.getWorldMatrix(), nullptr, view);

    if (options.addDefaultLight && lights.empty())
        lights.push_back ({ defaultLight.get(), {}, Vector3<float> (-0.35f, -0.75f, -0.55f).normalized() });

    if (options.selectLightsPerDraw)
    {
        rankLights (lights, lightRankings);

        for (auto* items : { &opaqueItems, &blendItems })
        {
            for (auto& item : *items)
            {
                const auto worldBounds = item.primitive->bounds.transformedBy (item.world);
                item.numLights = selectLights (worldBounds, lights, lightRankings, item.lightIndices, item.lightWeights);
                item.reflectionProbes = selectReflectionProbes (reflectionProbes, worldBounds);
                item.irradianceVolume = selectIrradianceVolume (irradianceVolumes, worldBounds);
            }
        }
    }

    const auto less = std::less<const Material*>();

    std::stable_sort (opaqueItems.begin(), opaqueItems.end(), [&less] (const DrawItem& a, const DrawItem& b)
    {
        if (a.material->doubleSided != b.material->doubleSided)
            return b.material->doubleSided;

        if (a.mirrored != b.mirrored)
            return b.mirrored;

        return less (a.material, b.material);
    });

    std::stable_sort (blendItems.begin(), blendItems.end(), [] (const DrawItem& a, const DrawItem& b)
    {
        return a.viewDistance > b.viewDistance;
    });
}

void SceneDrawList::rankLights (Span<const Light> lights, std::vector<LightRanking>& rankings)
{
    rankings.clear();

    for (const auto& light : lights)
    {
        const auto& node = *light.node;
        rankings.push_back ({ node.intensity * (0.2126f * node.color[0] + 0.7152f * node.color[1] + 0.0722f * node.color[2]),
                              GpuSceneData::getEffectiveRange (node),
                              std::cos (node.outerConeAngle),
                              GpuSceneData::isDark (node) });
    }
}

int SceneDrawList::selectLights (const BoundingBox& worldBounds, Span<const Light> lights, std::array<uint16, maxLightsPerDraw>& indices)
{
    std::vector<LightRanking> rankings;
    rankLights (lights, rankings);

    std::array<float, maxLightsPerDraw> weights;
    return selectLights (worldBounds, lights, rankings, indices, weights);
}

int SceneDrawList::selectLights (const BoundingBox& worldBounds, Span<const Light> lights, Span<const LightRanking> rankings,
                                 std::array<uint16, maxLightsPerDraw>& indices, std::array<float, maxLightsPerDraw>& weights)
{
    // How much a light matters to the object, directional lights first, or nothing if it cannot reach it
    const auto getLightRank = [&worldBounds] (const Light& light, const LightRanking& ranking) -> std::optional<std::pair<bool, float>>
    {
        if (ranking.dark)
            return std::nullopt;

        const auto& node = *light.node;
        if (node.type == LightNode::Type::directional)
            return std::pair { true, ranking.power };

        if (! GpuSceneData::lightReachesBox (worldBounds, light.position, light.direction, ranking.range, node.type == LightNode::Type::spot, ranking.cosOuter))
            return std::nullopt;

        const auto distanceSquared = worldBounds.getDistanceSquared (light.position);

        // The range window of the shaders: a light with a range ranks 0 at its edge, so it enters and leaves the selection smoothly
        auto window = 1.0f;
        if (node.range > 0.0f)
        {
            const auto ratioSquared = distanceSquared / (ranking.range * ranking.range);
            window = 1.0f - ratioSquared * ratioSquared;

            if (window <= 0.0f)
                return std::nullopt;
        }

        return std::pair { false, window * ranking.power / jmax (distanceSquared, node.radius * node.radius + 1.0e-4f) };
    };

    // One slot more than kept: the first light left out is where the kept ones fade out
    std::array<std::pair<bool, float>, maxLightsPerDraw + 1> ranks {};
    std::array<uint16, maxLightsPerDraw + 1> slots {};
    size_t count = 0;

    const auto numLights = jmin (lights.size(), static_cast<size_t> (std::numeric_limits<uint16>::max()) + 1);

    // An insertion sort into the slots: only a greater rank moves ahead, so ties keep the lower index
    for (size_t index = 0; index < numLights; ++index)
    {
        const auto rank = getLightRank (lights[index], rankings[index]);
        if (! rank.has_value())
            continue;

        auto slot = count;
        while (slot > 0 && ranks[slot - 1] < *rank)
            --slot;

        if (slot == ranks.size())
            continue;

        count = jmin (count + 1, ranks.size());

        for (auto i = count - 1; i > slot; --i)
        {
            ranks[i] = ranks[i - 1];
            slots[i] = slots[i - 1];
        }

        ranks[slot] = *rank;
        slots[slot] = static_cast<uint16> (index);
    }

    // A kept light fades out as its rank nears the one left out, so it never switches off within a frame
    constexpr auto fadeRatio = 2.0f;
    const auto& cutoff = ranks[maxLightsPerDraw];
    const auto numKept = jmin (count, static_cast<size_t> (maxLightsPerDraw));

    for (size_t i = 0; i < numKept; ++i)
    {
        indices[i] = slots[i];
        weights[i] = 1.0f;

        if (count > numKept && ranks[i].first == cutoff.first)
        {
            const auto t = jlimit (0.0f, 1.0f, (ranks[i].second - cutoff.second) / ((fadeRatio - 1.0f) * cutoff.second));
            weights[i] = t * t * (3.0f - 2.0f * t);
        }
    }

    return static_cast<int> (numKept);
}

std::array<int, 2> SceneDrawList::selectReflectionProbes (Span<const ReflectionProbe> probes, const BoundingBox& itemBounds) noexcept
{
    std::array<int, 2> picked { -1, -1 };

    if (itemBounds.isEmpty())
        return picked;

    const auto overlaps = [&itemBounds] (const BoundingBox& bounds)
    {
        const auto& a = itemBounds;
        return ! bounds.isEmpty()
            && bounds.getMin().getX() <= a.getMax().getX() && a.getMin().getX() <= bounds.getMax().getX()
            && bounds.getMin().getY() <= a.getMax().getY() && a.getMin().getY() <= bounds.getMax().getY()
            && bounds.getMin().getZ() <= a.getMax().getZ() && a.getMin().getZ() <= bounds.getMax().getZ();
    };

    // Smaller influences first, then nearer probes, then older bake ids
    struct Rank
    {
        float volume = 0.0f;
        float distance = 0.0f;
        uint64 bakeId = 0;

        bool operator< (const Rank& other) const noexcept
        {
            if (volume != other.volume)
                return volume < other.volume;

            if (distance != other.distance)
                return distance < other.distance;

            return bakeId < other.bakeId;
        }
    };

    std::array<Rank, 2> ranks {};
    const auto itemCenter = itemBounds.getCenter();

    for (size_t index = 0; index < probes.size(); ++index)
    {
        const auto& probe = probes[index];
        if (! overlaps (probe.bounds))
            continue;

        const auto size = probe.bounds.getSize();
        const Rank rank { size.getX() * size.getY() * size.getZ(),
                          (probe.position - itemCenter).length(),
                          probe.node != nullptr ? probe.node->getBakeId() : 0 };

        // Only a strictly better probe moves ahead, so full ties keep the lower index
        if (picked[0] < 0 || rank < ranks[0])
        {
            picked[1] = picked[0];
            ranks[1] = ranks[0];
            picked[0] = static_cast<int> (index);
            ranks[0] = rank;
        }
        else if (picked[1] < 0 || rank < ranks[1])
        {
            picked[1] = static_cast<int> (index);
            ranks[1] = rank;
        }
    }

    return picked;
}

int SceneDrawList::selectIrradianceVolume (Span<const IrradianceVolume> volumes, const BoundingBox& itemBounds) noexcept
{
    if (itemBounds.isEmpty())
        return -1;

    const auto center = itemBounds.getCenter();
    auto picked = -1;
    auto pickedVolume = 0.0f;

    for (size_t index = 0; index < volumes.size(); ++index)
    {
        const auto& bounds = volumes[index].bounds;
        if (bounds.isEmpty()
            || center.getX() < bounds.getMin().getX() || center.getX() > bounds.getMax().getX()
            || center.getY() < bounds.getMin().getY() || center.getY() > bounds.getMax().getY()
            || center.getZ() < bounds.getMin().getZ() || center.getZ() > bounds.getMax().getZ())
            continue;

        // Only a strictly smaller volume moves ahead, so ties keep the lower index
        const auto size = bounds.getSize();
        const auto volume = size.getX() * size.getY() * size.getZ();
        if (picked < 0 || volume < pickedVolume)
        {
            picked = static_cast<int> (index);
            pickedVolume = volume;
        }
    }

    return picked;
}

void SceneDrawList::clear()
{
    opaqueItems.clear();
    blendItems.clear();
    lights.clear();
    reflectionProbes.clear();
    irradianceVolumes.clear();
}

void SceneDrawList::collect (const Entity& entity, const Matrix4& world, const Material* overrideMaterial, const Matrix4& view)
{
    if (! entity.isVisible())
        return;

    // Only the lowest MaterialNode slot is the subtree override
    bool overrideChecked = false;
    entity.forEachNode<MaterialNode> ([&] (MaterialNode& node, int)
    {
        if (! std::exchange (overrideChecked, true) && node.material != nullptr)
            overrideMaterial = node.material.get();
    });

    const auto determinant = getLinearDeterminant (world);
    const auto mirrored = determinant < 0.0f;

    entity.forEachNode<MeshNode> ([&] (MeshNode& node, int)
    {
        if (node.mesh == nullptr)
            return;

        for (int i = 0; i < node.mesh->getNumPrimitives(); ++i)
        {
            const auto& primitive = node.mesh->getPrimitive (i);

            DrawItem item;
            item.world = world;
            item.mesh = node.mesh.get();
            item.node = &node;
            item.primitiveIndex = i;
            item.primitive = &primitive;
            item.material = overrideMaterial != nullptr     ? overrideMaterial
                          : primitive.material != nullptr ? primitive.material.get()
                                                          : defaultMaterial.get();
            item.viewDistance = -view.transformPoint (world.transformPoint (primitive.bounds.getCenter())).getZ();
            item.mirrored = mirrored;

            if (item.material->alphaMode == Material::AlphaMode::blend)
                blendItems.push_back (item);
            else
                opaqueItems.push_back (item);
        }
    });

    entity.forEachNode<LightNode> ([&] (LightNode& node, int)
    {
        lights.push_back ({ &node,
                            world.transformPoint ({}),
                            world.transformVector ({ 0.0f, 0.0f, -1.0f }).normalized() });
    });

    entity.forEachNode<ReflectionProbeNode> ([&] (ReflectionProbeNode& node, int)
    {
        const auto isBox = node.shape == ReflectionProbeNode::Shape::box;
        const auto& extents = node.influenceExtents;
        const auto hasInfluence = isBox ? jmin (extents.getX(), extents.getY(), extents.getZ()) > 0.0f
                                        : node.influenceRadius > 0.0f;

        // A probe without influence, or flattened by a zero scale, reaches nothing
        if (! hasInfluence || std::abs (determinant) <= 1.0e-12f)
            return;

        const auto position = world.transformPoint ({});

        // A sphere is bounded exactly: along each world axis, the radius times the length of that row of the linear part
        const auto rowLength = [&world] (int row)
        {
            return Vector3<float> (world (row, 0), world (row, 1), world (row, 2)).length();
        };

        const auto sphereExtents = Vector3<float> (rowLength (0), rowLength (1), rowLength (2)) * node.influenceRadius;

        reflectionProbes.push_back ({ &node,
                                      world,
                                      position,
                                      isBox ? BoundingBox (-extents, extents).transformedBy (world)
                                            : BoundingBox (position - sphereExtents, position + sphereExtents) });
    });

    entity.forEachNode<IrradianceVolumeNode> ([&] (IrradianceVolumeNode& node, int)
    {
        const auto& extents = node.extents;

        // A volume without extent, or flattened by a zero scale, lights nothing
        if (jmin (extents.getX(), extents.getY(), extents.getZ()) <= 0.0f || std::abs (determinant) <= 1.0e-12f)
            return;

        irradianceVolumes.push_back ({ &node, world, BoundingBox (-extents, extents).transformedBy (world) });
    });

    for (const auto& child : entity.getChildren())
        collect (*child, child->getLocalMatrix().followedBy (world), overrideMaterial, view);
}

} // namespace yup
