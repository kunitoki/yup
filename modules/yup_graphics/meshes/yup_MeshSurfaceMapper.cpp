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

constexpr float uvContainmentTolerance = 1.0e-5f;

/** Returns the barycentric weights (u, v) of @a point, lying on the plane of the triangle, for
    the second and third vertex. The weights fall outside [0, 1] for points outside the triangle.
*/
std::optional<Point<float>> getPlaneBarycentrics (const Vector3<float>& point, const Vector3<float>& a, const Vector3<float>& b, const Vector3<float>& c)
{
    const auto edge1 = b - a;
    const auto edge2 = c - a;
    const auto toPoint = point - a;

    const auto d11 = edge1.dotProduct (edge1);
    const auto d12 = edge1.dotProduct (edge2);
    const auto d22 = edge2.dotProduct (edge2);
    const auto dp1 = toPoint.dotProduct (edge1);
    const auto dp2 = toPoint.dotProduct (edge2);

    const auto denominator = d11 * d22 - d12 * d12;
    if (denominator == 0.0f)
        return std::nullopt;

    return Point<float> { (d22 * dp1 - d12 * dp2) / denominator,
                          (d11 * dp2 - d12 * dp1) / denominator };
}

template <class T>
T interpolateBarycentric (const T& a, const T& b, const T& c, float u, float v)
{
    return a * (1.0f - u - v) + b * u + c * v;
}

} // namespace

//==============================================================================

void MeshSurfaceMapper::setMesh (Span<const Vector3<float>> positions, Span<const Point<float>> uvs, Span<const uint32> indices)
{
    jassert (positions.size() == uvs.size());
    jassert (indices.size() % 3 == 0);

    triangles.clear();
    triangles.reserve (indices.size() / 3);

    for (std::size_t i = 0; i + 2 < indices.size(); i += 3)
    {
        const auto ia = indices[i];
        const auto ib = indices[i + 1];
        const auto ic = indices[i + 2];

        if (jmax (ia, ib, ic) >= jmin (positions.size(), uvs.size()))
        {
            jassertfalse; // Index out of range
            continue;
        }

        triangles.push_back ({ positions[ia], positions[ib], positions[ic], uvs[ia], uvs[ib], uvs[ic] });
    }
}

void MeshSurfaceMapper::setModelViewProjection (const Matrix4& newModelViewProjection, Rectangle<float> newViewport)
{
    modelViewProjection = newModelViewProjection;
    inverseModelViewProjection = newModelViewProjection.inverted();
    viewport = newViewport;
}

void MeshSurfaceMapper::setBackFaceCulling (bool shouldCullBackFaces)
{
    cullBackFaces = shouldCullBackFaces;
}

bool MeshSurfaceMapper::isBackFaceCulling() const
{
    return cullBackFaces;
}

//==============================================================================

std::optional<MeshSurfaceMapper::Hit> MeshSurfaceMapper::hitTest (Point<float> viewportPoint) const
{
    if (viewport.isEmpty())
        return std::nullopt;

    const auto ray = getPickingRay (viewportPoint);

    std::optional<Hit> nearest;
    float nearestDistance = std::numeric_limits<float>::max();

    for (std::size_t index = 0; index < triangles.size(); ++index)
    {
        const auto& triangle = triangles[index];

        const auto hit = ray.intersectTriangle (triangle.a, triangle.b, triangle.c, cullBackFaces);
        if (! hit || hit->distance >= nearestDistance)
            continue;

        const auto point = ray.getPointAt (hit->distance);
        const auto clip = modelViewProjection.transformPoint4 (point.getX(), point.getY(), point.getZ(), 1.0f);

        nearestDistance = hit->distance;
        nearest = Hit { interpolateBarycentric (triangle.uvA, triangle.uvB, triangle.uvC, hit->u, hit->v),
                        clip[3] != 0.0f ? clip[2] / clip[3] : clip[2],
                        static_cast<int> (index) };
    }

    return nearest;
}

std::optional<Point<float>> MeshSurfaceMapper::viewportToUV (Point<float> viewportPoint) const
{
    if (auto hit = hitTest (viewportPoint))
        return hit->uv;

    if (viewport.isEmpty())
        return std::nullopt;

    const auto ray = getPickingRay (viewportPoint);

    const Triangle* closest = nullptr;
    float closestDistance = std::numeric_limits<float>::max();

    for (const auto& triangle : triangles)
    {
        const auto normal = (triangle.b - triangle.a).crossProduct (triangle.c - triangle.a);
        if (cullBackFaces && normal.dotProduct (ray.getDirection()) >= 0.0f)
            continue;

        const auto center = clipToViewport ((triangle.a + triangle.b + triangle.c) / 3.0f);
        if (! center)
            continue;

        if (const auto distance = center->distanceToSquared (viewportPoint); distance < closestDistance)
        {
            closestDistance = distance;
            closest = &triangle;
        }
    }

    if (closest == nullptr)
        return std::nullopt;

    const auto normal = (closest->b - closest->a).crossProduct (closest->c - closest->a);

    const auto distance = ray.intersectPlane (closest->a, normal);
    if (! distance)
        return std::nullopt;

    const auto weights = getPlaneBarycentrics (ray.getPointAt (*distance), closest->a, closest->b, closest->c);
    if (! weights)
        return std::nullopt;

    return interpolateBarycentric (closest->uvA, closest->uvB, closest->uvC, weights->getX(), weights->getY());
}

std::optional<Point<float>> MeshSurfaceMapper::uvToViewport (Point<float> uv) const
{
    if (viewport.isEmpty())
        return std::nullopt;

    for (const auto& triangle : triangles)
    {
        const auto edge1 = triangle.uvB - triangle.uvA;
        const auto edge2 = triangle.uvC - triangle.uvA;
        const auto toPoint = uv - triangle.uvA;

        const auto denominator = edge1.getX() * edge2.getY() - edge2.getX() * edge1.getY();
        if (denominator == 0.0f)
            continue;

        const auto u = (toPoint.getX() * edge2.getY() - edge2.getX() * toPoint.getY()) / denominator;
        const auto v = (edge1.getX() * toPoint.getY() - toPoint.getX() * edge1.getY()) / denominator;

        if (u < -uvContainmentTolerance || v < -uvContainmentTolerance || u + v > 1.0f + uvContainmentTolerance)
            continue;

        return clipToViewport (interpolateBarycentric (triangle.a, triangle.b, triangle.c, u, v));
    }

    return std::nullopt;
}

//==============================================================================

Ray MeshSurfaceMapper::getPickingRay (Point<float> viewportPoint) const
{
    return Ray::fromViewportPoint (viewportPoint, viewport, inverseModelViewProjection);
}

std::optional<Point<float>> MeshSurfaceMapper::clipToViewport (const Vector3<float>& modelPoint) const
{
    const auto clip = modelViewProjection.transformPoint4 (modelPoint.getX(), modelPoint.getY(), modelPoint.getZ(), 1.0f);
    if (clip[3] <= 0.0f)
        return std::nullopt;

    const auto ndcX = clip[0] / clip[3];
    const auto ndcY = clip[1] / clip[3];

    return Point<float> { viewport.getX() + (ndcX + 1.0f) * 0.5f * viewport.getWidth(),
                          viewport.getY() + (1.0f - ndcY) * 0.5f * viewport.getHeight() };
}

} // namespace yup
