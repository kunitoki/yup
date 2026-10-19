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

std::optional<float> MeshNode::intersectRay (const Ray& ray, float minDistance, float maxDistance) const noexcept
{
    const auto entity = getEntity();
    if (mesh == nullptr || ! entity)
        return std::nullopt;

    const auto world = entity.getWorldMatrix();
    const auto determinant = world (0, 0) * (world (1, 1) * world (2, 2) - world (2, 1) * world (1, 2))
                           - world (0, 1) * (world (1, 0) * world (2, 2) - world (2, 0) * world (1, 2))
                           + world (0, 2) * (world (1, 0) * world (2, 1) - world (2, 0) * world (1, 1));

    if (std::abs (determinant) <= 1.0e-12f)
        return std::nullopt;

    return ray.transformedBy (world.inverted()).intersectBox (mesh->getBounds(), minDistance, maxDistance);
}

} // namespace yup
