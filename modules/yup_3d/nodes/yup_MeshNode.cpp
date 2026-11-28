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

/* A world space ray in the space of a mesh an entity places, or nothing for one flattened by a zero scale. */
std::optional<Ray> getMeshSpaceRay (const Ray& ray, const Entity& entity) noexcept
{
    const auto world = entity.getWorldMatrix();
    const auto determinant = world (0, 0) * (world (1, 1) * world (2, 2) - world (2, 1) * world (1, 2))
                           - world (0, 1) * (world (1, 0) * world (2, 2) - world (2, 0) * world (1, 2))
                           + world (0, 2) * (world (1, 0) * world (2, 1) - world (2, 0) * world (1, 1));

    // Only what can't be inverted, as Matrix4::inverted() takes it: the determinant is the cube of a scale, so any
    // threshold would leave out the small meshes that are drawn
    if (determinant == 0.0f)
        return std::nullopt;

    return ray.transformedBy (world.inverted());
}

} // namespace

//==============================================================================

std::optional<float> MeshNode::intersectRay (const Ray& ray, float minDistance, float maxDistance) const noexcept
{
    const auto entity = getEntity();
    if (mesh == nullptr || ! entity)
        return std::nullopt;

    const auto local = getMeshSpaceRay (ray, entity);
    if (! local.has_value())
        return std::nullopt;

    return local->intersectBox (mesh->getBounds(), minDistance, maxDistance);
}

std::optional<float> MeshNode::intersectTriangles (const Ray& ray, float minDistance, float maxDistance) const noexcept
{
    const auto entity = getEntity();
    if (mesh == nullptr || ! entity)
        return std::nullopt;

    const auto local = getMeshSpaceRay (ray, entity);
    if (! local.has_value())
        return std::nullopt;

    // As it is drawn: with the material of the nearest MaterialNode that has one, from its entity up
    const Material* overrideMaterial = nullptr;

    for (auto owner = entity; owner.isValid() && overrideMaterial == nullptr; owner = owner.getParent())
    {
        if (const auto* node = owner.get<MaterialNode>())
            overrideMaterial = node->material.get();
    }

    return mesh->intersectTriangles (*local, minDistance, maxDistance, overrideMaterial);
}

} // namespace yup
