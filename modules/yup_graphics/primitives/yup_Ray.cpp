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

namespace
{

constexpr float rayEpsilon = 1.0e-7f;

} // namespace

//==============================================================================

Ray Ray::fromViewportPoint (Point<float> viewportPoint, Rectangle<float> viewport, const Matrix4& inverseViewProjection) noexcept
{
    const auto ndcX = (viewportPoint.getX() - viewport.getX()) / viewport.getWidth() * 2.0f - 1.0f;
    const auto ndcY = 1.0f - (viewportPoint.getY() - viewport.getY()) / viewport.getHeight() * 2.0f;

    // NDC z = -1 lies at or before the near plane in both depth conventions, z = 1 is the far plane
    const auto nearPoint = inverseViewProjection.transformPoint ({ ndcX, ndcY, -1.0f });
    const auto farPoint = inverseViewProjection.transformPoint ({ ndcX, ndcY, 1.0f });

    return { nearPoint, (farPoint - nearPoint).normalized() };
}

//==============================================================================

std::optional<float> Ray::intersectPlane (const Vector3<float>& pointOnPlane, const Vector3<float>& normal) const noexcept
{
    const auto denominator = normal.dotProduct (direction);
    if (std::abs (denominator) < rayEpsilon)
        return std::nullopt;

    const auto distance = normal.dotProduct (pointOnPlane - origin) / denominator;
    if (distance < 0.0f)
        return std::nullopt;

    return distance;
}

std::optional<RayTriangleHit> Ray::intersectTriangle (const Vector3<float>& a, const Vector3<float>& b, const Vector3<float>& c, bool cullBackFaces) const noexcept
{
    const auto edge1 = b - a;
    const auto edge2 = c - a;
    const auto p = direction.crossProduct (edge2);
    const auto determinant = edge1.dotProduct (p);

    // A positive determinant means the triangle faces the ray origin
    if (cullBackFaces ? determinant < rayEpsilon : std::abs (determinant) < rayEpsilon)
        return std::nullopt;

    const auto inverseDeterminant = 1.0f / determinant;
    const auto toOrigin = origin - a;

    const auto u = toOrigin.dotProduct (p) * inverseDeterminant;
    if (u < 0.0f || u > 1.0f)
        return std::nullopt;

    const auto q = toOrigin.crossProduct (edge1);

    const auto v = direction.dotProduct (q) * inverseDeterminant;
    if (v < 0.0f || u + v > 1.0f)
        return std::nullopt;

    const auto distance = edge2.dotProduct (q) * inverseDeterminant;
    if (distance < 0.0f)
        return std::nullopt;

    return RayTriangleHit { distance, u, v };
}

std::optional<float> Ray::intersectBox (const BoundingBox& box, float minDistance, float maxDistance) const noexcept
{
    if (box.isEmpty())
        return std::nullopt;

    const float starts[] { origin.getX(), origin.getY(), origin.getZ() };
    const float steps[] { direction.getX(), direction.getY(), direction.getZ() };
    const float lows[] { box.getMin().getX(), box.getMin().getY(), box.getMin().getZ() };
    const float highs[] { box.getMax().getX(), box.getMax().getY(), box.getMax().getZ() };

    auto enter = minDistance;
    auto leave = maxDistance;

    // The ray is inside the box where it is between all three pairs of planes
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

} // namespace yup
