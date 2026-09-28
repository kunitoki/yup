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

} // namespace yup
