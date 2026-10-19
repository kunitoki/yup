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
/** A half line in 3D space: an origin and a direction, for picking and hit tests.

    A point of the ray is origin + direction * distance. The direction doesn't need to be
    normalized: distances are counted in lengths of the direction, which transformedBy() keeps,
    so a hit found in the local space of a mesh is at the same distance in world space.

    @see CameraNode::getRay, Entity::intersectRay, Scene::raycast
*/
class YUP_API Ray
{
public:
    //==============================================================================
    /** Constructs a ray from the origin along -Z. */
    constexpr Ray() noexcept = default;

    /** Constructs a ray.

        @param rayOrigin    The point the ray starts from.
        @param rayDirection The direction of the ray, and the length of one unit of distance.
    */
    constexpr Ray (const Vector3<float>& rayOrigin, const Vector3<float>& rayDirection) noexcept
        : origin (rayOrigin)
        , direction (rayDirection)
    {
    }

    //==============================================================================
    /** Returns the point the ray starts from. */
    constexpr const Vector3<float>& getOrigin() const noexcept { return origin; }

    /** Returns the direction of the ray. */
    constexpr const Vector3<float>& getDirection() const noexcept { return direction; }

    /** Returns the point at a distance along the ray, in lengths of the direction. */
    Vector3<float> getPointAt (float distance) const noexcept { return origin + direction * distance; }

    //==============================================================================
    /** Returns the ray transformed by a matrix, with the same distances to the same points. */
    Ray transformedBy (const Matrix4& matrix) const noexcept
    {
        return { matrix.transformPoint (origin), matrix.transformVector (direction) };
    }

    /** Returns the nearest distance in [minDistance, maxDistance] where the ray is inside a box.

        That is where the ray enters the box, or minDistance when it is already inside then.

        @param box          The box to test, an empty box is never hit.
        @param minDistance  The smallest distance to consider, the near plane of a camera for example.
        @param maxDistance  The largest distance to consider.

        @return The distance of the hit, or nothing if the ray misses the box within the range.
    */
    std::optional<float> intersect (const BoundingBox& box,
                                    float minDistance = 0.0f,
                                    float maxDistance = std::numeric_limits<float>::max()) const noexcept
    {
        if (box.isEmpty())
            return std::nullopt;

        const float starts[] { origin.getX(), origin.getY(), origin.getZ() };
        const float steps[] { direction.getX(), direction.getY(), direction.getZ() };
        const float lows[] { box.getMin().getX(), box.getMin().getY(), box.getMin().getZ() };
        const float highs[] { box.getMax().getX(), box.getMax().getY(), box.getMax().getZ() };

        auto enter = minDistance;
        auto leave = maxDistance;

        // The slab method: the ray is inside the box where it is inside all three pairs of planes
        for (int axis = 0; axis < 3; ++axis)
        {
            if (steps[axis] == 0.0f)
            {
                if (starts[axis] < lows[axis] || starts[axis] > highs[axis])
                    return std::nullopt;

                continue;
            }

            const auto t0 = (lows[axis] - starts[axis]) / steps[axis];
            const auto t1 = (highs[axis] - starts[axis]) / steps[axis];

            enter = jmax (enter, jmin (t0, t1));
            leave = jmin (leave, jmax (t0, t1));

            if (enter > leave)
                return std::nullopt;
        }

        return enter;
    }

private:
    Vector3<float> origin;
    Vector3<float> direction { 0.0f, 0.0f, -1.0f };
};

} // namespace yup
