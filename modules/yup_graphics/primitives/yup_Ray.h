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
/** The result of a successful Ray::intersectTriangle() test. */
struct YUP_API RayTriangleHit
{
    /** The ray parameter of the hit: the hit point is Ray::getPointAt (distance). */
    float distance = 0.0f;

    /** The barycentric weight of the triangle's second vertex at the hit point. */
    float u = 0.0f;

    /** The barycentric weight of the triangle's third vertex at the hit point.
        The first vertex weighs 1 - u - v.
    */
    float v = 0.0f;
};

//==============================================================================
/** A half line in 3D space, starting at an origin and extending along a direction.

    Rays are used for picking: Ray::fromViewportPoint() turns a point on screen into the ray
    of all the 3D points displayed under it, which can then be intersected with planes or
    triangles.

    @see Vector3, Matrix4, MeshSurfaceMapper
*/
class YUP_API Ray
{
public:
    //==============================================================================
    /** Constructs a ray at the origin with no direction. */
    constexpr Ray() noexcept = default;

    /** Constructs a ray.

        @param newOrigin     The start of the ray.
        @param newDirection  The direction of the ray. Distances along the ray are measured in
                             multiples of its length, so pass a normalized vector to get
                             distances in world units.
    */
    constexpr Ray (const Vector3<float>& newOrigin, const Vector3<float>& newDirection) noexcept
        : origin (newOrigin)
        , direction (newDirection)
    {
    }

    //==============================================================================
    /** Returns the ray of the 3D points displayed at a point of a viewport.

        The point is unprojected at both ends of the depth range, which lie on the ray for
        both the [0, 1] and the [-1, 1] depth conventions, so the result doesn't depend on
        the graphics backend. The origin lies at or before the near plane and the direction
        is normalized.

        @param viewportPoint          The point, in the same coordinates as @a viewport (y pointing down).
        @param viewport               The area the scene is displayed in.
        @param inverseViewProjection  The inverse of the matrix that maps world space to clip space.

        @return The picking ray, in world space.
    */
    [[nodiscard]] static Ray fromViewportPoint (Point<float> viewportPoint, Rectangle<float> viewport, const Matrix4& inverseViewProjection) noexcept;

    //==============================================================================
    /** Returns the start of the ray. */
    [[nodiscard]] constexpr Vector3<float> getOrigin() const noexcept { return origin; }

    /** Returns the direction of the ray. */
    [[nodiscard]] constexpr Vector3<float> getDirection() const noexcept { return direction; }

    /** Returns the point at a given distance along the ray.

        @param distance The ray parameter, in multiples of the direction length.
    */
    [[nodiscard]] constexpr Vector3<float> getPointAt (float distance) const noexcept
    {
        return origin + direction * distance;
    }

    //==============================================================================
    /** Intersects the ray with a plane.

        @param pointOnPlane  Any point of the plane.
        @param normal        The plane normal, it doesn't need to be normalized.

        @return The distance of the intersection, or std::nullopt if the ray is parallel to the
                plane or the plane lies behind the origin.
    */
    [[nodiscard]] std::optional<float> intersectPlane (const Vector3<float>& pointOnPlane, const Vector3<float>& normal) const noexcept;

    /** Intersects the ray with a triangle (Moller-Trumbore).

        A triangle is front facing when its vertices appear counter-clockwise from the ray
        origin, which matches the counter-clockwise front face of the GPU pipelines.

        @param a               The first vertex.
        @param b               The second vertex.
        @param c               The third vertex.
        @param cullBackFaces   True to ignore triangles that face away from the ray origin.

        @return The hit distance and barycentric coordinates, or std::nullopt if the ray misses.
    */
    [[nodiscard]] std::optional<RayTriangleHit> intersectTriangle (const Vector3<float>& a, const Vector3<float>& b, const Vector3<float>& c, bool cullBackFaces) const noexcept;

private:
    Vector3<float> origin;
    Vector3<float> direction;
};

} // namespace yup
