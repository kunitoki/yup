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
{
    defaultMaterial->name = "default";
    defaultMaterial->metallicFactor = 0.0f;
    defaultMaterial->roughnessFactor = 0.5f;
}

SceneDrawList::~SceneDrawList() = default;

//==============================================================================

void SceneDrawList::build (const EntityNode& root, const Matrix4& view)
{
    clear();

    collect (root, root.getWorldMatrix(), nullptr, view);

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
