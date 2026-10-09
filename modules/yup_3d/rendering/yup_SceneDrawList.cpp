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

static bool isMirroringTransform (const Matrix4& m) noexcept
{
    const auto determinant = m (0, 0) * (m (1, 1) * m (2, 2) - m (2, 1) * m (1, 2))
                           - m (0, 1) * (m (1, 0) * m (2, 2) - m (2, 0) * m (1, 2))
                           + m (0, 2) * (m (1, 0) * m (2, 1) - m (2, 0) * m (1, 1));

    return determinant < 0.0f;
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

void SceneDrawList::build (const EntityNode& root, const Matrix4& view)
{
    build (root, view, BuildOptions {});
}

void SceneDrawList::build (const EntityNode& root, const Matrix4& view, const BuildOptions& options)
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
                item.numLights = selectLights (item.primitive->bounds.transformedBy (item.world), lights, lightRankings, item.lightIndices);
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
    return selectLights (worldBounds, lights, rankings, indices);
}

int SceneDrawList::selectLights (const BoundingBox& worldBounds, Span<const Light> lights, Span<const LightRanking> rankings, std::array<uint16, maxLightsPerDraw>& indices)
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
        return std::pair { false, ranking.power / jmax (distanceSquared, node.radius * node.radius + 1.0e-4f) };
    };

    std::array<std::pair<bool, float>, maxLightsPerDraw> ranks {};
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
            indices[i] = indices[i - 1];
        }

        ranks[slot] = *rank;
        indices[slot] = static_cast<uint16> (index);
    }

    return static_cast<int> (count);
}

void SceneDrawList::clear()
{
    opaqueItems.clear();
    blendItems.clear();
    lights.clear();
}

void SceneDrawList::collect (const EntityNode& entity, const Matrix4& world, const Material* overrideMaterial, const Matrix4& view)
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

    const auto mirrored = isMirroringTransform (world);

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

    for (const auto& child : entity.getChildren())
        collect (*child, child->getLocalMatrix().followedBy (world), overrideMaterial, view);
}

} // namespace yup
