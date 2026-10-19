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
/** An axis-aligned bounding box in 3D space.

    A default constructed box is empty: expanding it by a point makes it contain just that
    point. Empty boxes are ignored when expanding by another box.

    @see Mesh, Entity::computeWorldBounds
*/
class YUP_API BoundingBox
{
public:
    //==============================================================================
    /** Constructs an empty box. */
    BoundingBox() noexcept = default;

    /** Constructs a box from its two corners.

        @param minCorner The corner with the smallest coordinates.
        @param maxCorner The corner with the largest coordinates.
    */
    BoundingBox (const Vector3<float>& minCorner, const Vector3<float>& maxCorner) noexcept
        : minimum (minCorner)
        , maximum (maxCorner)
    {
    }

    //==============================================================================
    /** Returns true if the box contains nothing, not even a single point. */
    bool isEmpty() const noexcept
    {
        return minimum.getX() > maximum.getX() || minimum.getY() > maximum.getY() || minimum.getZ() > maximum.getZ();
    }

    /** Returns the corner with the smallest coordinates. */
    const Vector3<float>& getMin() const noexcept { return minimum; }

    /** Returns the corner with the largest coordinates. */
    const Vector3<float>& getMax() const noexcept { return maximum; }

    /** Returns one of the eight corners.

        @param index  The corner, in [0, 8): bit 0 picks the maximum x, bit 1 the maximum y and bit 2 the maximum z.
    */
    Vector3<float> getCorner (int index) const noexcept
    {
        jassert (isPositiveAndBelow (index, 8));

        return { (index & 1) != 0 ? maximum.getX() : minimum.getX(),
                 (index & 2) != 0 ? maximum.getY() : minimum.getY(),
                 (index & 4) != 0 ? maximum.getZ() : minimum.getZ() };
    }

    //==============================================================================
    /** Grows the box to contain a point. */
    void expand (const Vector3<float>& point) noexcept
    {
        minimum = { jmin (minimum.getX(), point.getX()), jmin (minimum.getY(), point.getY()), jmin (minimum.getZ(), point.getZ()) };
        maximum = { jmax (maximum.getX(), point.getX()), jmax (maximum.getY(), point.getY()), jmax (maximum.getZ(), point.getZ()) };
    }

    /** Grows the box to contain another box. Empty boxes are ignored. */
    void expand (const BoundingBox& other) noexcept
    {
        if (other.isEmpty())
            return;

        expand (other.minimum);
        expand (other.maximum);
    }

    /** Returns the box containing this box after a transformation.

        The result contains all eight transformed corners, so rotations give a box that is larger
        than the transformed geometry but always contains it. Affine transforms compute it from
        the transformed center and the absolute linear part (Arvo's method) instead of
        transforming each corner.

        @param transform The transformation to apply.
    */
    BoundingBox transformedBy (const Matrix4& transform) const noexcept
    {
        if (isEmpty())
            return {};

        BoundingBox result;

        if (! transform.isAffine())
        {
            for (int corner = 0; corner < 8; ++corner)
                result.expand (transform.transformPoint (getCorner (corner)));

            return result;
        }

        const auto halfSize = (maximum - minimum) * 0.5f;
        const auto extent = [&] (int row)
        {
            return std::abs (transform (row, 0)) * halfSize.getX()
                 + std::abs (transform (row, 1)) * halfSize.getY()
                 + std::abs (transform (row, 2)) * halfSize.getZ();
        };

        const auto center = transform.transformPoint ((minimum + maximum) * 0.5f);
        const Vector3<float> extents (extent (0), extent (1), extent (2));
        return { center - extents, center + extents };
    }

    //==============================================================================
    /** Returns the center of the box, or the origin if it is empty. */
    Vector3<float> getCenter() const noexcept
    {
        return isEmpty() ? Vector3<float>() : (minimum + maximum) * 0.5f;
    }

    /** Returns the extent of the box along each axis, or zero if it is empty. */
    Vector3<float> getSize() const noexcept
    {
        return isEmpty() ? Vector3<float>() : maximum - minimum;
    }

    /** Returns the radius of the sphere centered on the box that touches its corners. */
    float getRadius() const noexcept
    {
        return getSize().length() * 0.5f;
    }

    /** Returns the squared distance from a point to the box, 0 when the point is inside it. */
    float getDistanceSquared (const Vector3<float>& point) const noexcept
    {
        const auto axis = [] (float p, float low, float high)
        {
            return jmax (low - p, 0.0f, p - high);
        };

        return Vector3<float> (axis (point.getX(), minimum.getX(), maximum.getX()),
                               axis (point.getY(), minimum.getY(), maximum.getY()),
                               axis (point.getZ(), minimum.getZ(), maximum.getZ()))
            .lengthSquared();
    }

private:
    Vector3<float> minimum { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
    Vector3<float> maximum { std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest() };
};

} // namespace yup
