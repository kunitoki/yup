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
/** Represents a 3D vector (or point) with coordinates of a generic type.

    This is the three dimensional counterpart of Point, used together with Matrix4 and Ray
    to place geometry in 3D space, for example to present a Component on a 3D surface.

    @tparam ValueType The type of the coordinates, can be any numeric type (int, float, double, etc.).

    @see Matrix4, Ray, MeshSurfaceMapper
*/
template <class ValueType>
class YUP_API Vector3
{
public:
    //==============================================================================
    /** Value type of the vector. */
    using Type = ValueType;

    //==============================================================================
    /** Constructs a vector at the origin (0, 0, 0). */
    constexpr Vector3() noexcept = default;

    /** Constructs a vector with the given coordinates.

        @param newX The x coordinate.
        @param newY The y coordinate.
        @param newZ The z coordinate.
    */
    constexpr Vector3 (ValueType newX, ValueType newY, ValueType newZ) noexcept
        : x (newX)
        , y (newY)
        , z (newZ)
    {
    }

    //==============================================================================
    /** Copy and move constructors and assignment operators. */
    constexpr Vector3 (const Vector3& other) noexcept = default;
    constexpr Vector3 (Vector3&& other) noexcept = default;
    constexpr Vector3& operator= (const Vector3& other) noexcept = default;
    constexpr Vector3& operator= (Vector3&& other) noexcept = default;

    //==============================================================================
    /** Returns the x coordinate. */
    [[nodiscard]] constexpr ValueType getX() const noexcept { return x; }

    /** Returns the y coordinate. */
    [[nodiscard]] constexpr ValueType getY() const noexcept { return y; }

    /** Returns the z coordinate. */
    [[nodiscard]] constexpr ValueType getZ() const noexcept { return z; }

    /** Returns a copy of this vector with a different x coordinate. */
    [[nodiscard]] constexpr Vector3 withX (ValueType newX) const noexcept { return { newX, y, z }; }

    /** Returns a copy of this vector with a different y coordinate. */
    [[nodiscard]] constexpr Vector3 withY (ValueType newY) const noexcept { return { x, newY, z }; }

    /** Returns a copy of this vector with a different z coordinate. */
    [[nodiscard]] constexpr Vector3 withZ (ValueType newZ) const noexcept { return { x, y, newZ }; }

    //==============================================================================
    /** Returns the dot product of this vector and another one.

        @param other The other vector.

        @return The sum of the products of the matching coordinates.
    */
    [[nodiscard]] constexpr ValueType dotProduct (const Vector3& other) const noexcept
    {
        return x * other.x + y * other.y + z * other.z;
    }

    /** Returns the cross product of this vector and another one.

        The result is perpendicular to both vectors, oriented with the right-hand rule, and its
        length is the area of the parallelogram they span.

        @param other The other vector.

        @return The cross product (this x other).
    */
    [[nodiscard]] constexpr Vector3 crossProduct (const Vector3& other) const noexcept
    {
        return { y * other.z - z * other.y,
                 z * other.x - x * other.z,
                 x * other.y - y * other.x };
    }

    /** Returns the squared length of this vector, which avoids a square root. */
    [[nodiscard]] constexpr ValueType lengthSquared() const noexcept
    {
        return dotProduct (*this);
    }

    /** Returns the length (magnitude) of this vector. */
    [[nodiscard]] float length() const noexcept
    {
        return std::sqrt (static_cast<float> (lengthSquared()));
    }

    /** Returns a vector with the same direction and a length of one.

        A zero length vector is returned unchanged.
    */
    [[nodiscard]] Vector3 normalized() const noexcept
    {
        const auto len = length();
        if (len == 0.0f)
            return *this;

        return { static_cast<ValueType> (x / len),
                 static_cast<ValueType> (y / len),
                 static_cast<ValueType> (z / len) };
    }

    //==============================================================================
    /** Converts the coordinates of this vector to another numeric type.

        @tparam T The target numeric type.

        @return A new Vector3<T> with the coordinates cast to T.
    */
    template <class T>
    [[nodiscard]] constexpr Vector3<T> to() const noexcept
    {
        return { static_cast<T> (x), static_cast<T> (y), static_cast<T> (z) };
    }

    //==============================================================================
    /** Returns the sum of this vector and another one. */
    [[nodiscard]] constexpr Vector3 operator+ (const Vector3& other) const noexcept { return { x + other.x, y + other.y, z + other.z }; }

    /** Returns the difference between this vector and another one. */
    [[nodiscard]] constexpr Vector3 operator- (const Vector3& other) const noexcept { return { x - other.x, y - other.y, z - other.z }; }

    /** Returns this vector scaled by a factor. */
    [[nodiscard]] constexpr Vector3 operator* (ValueType factor) const noexcept { return { x * factor, y * factor, z * factor }; }

    /** Returns this vector divided by a factor. */
    [[nodiscard]] constexpr Vector3 operator/ (ValueType factor) const noexcept { return { x / factor, y / factor, z / factor }; }

    /** Returns the vector pointing in the opposite direction. */
    [[nodiscard]] constexpr Vector3 operator-() const noexcept { return { -x, -y, -z }; }

    /** Adds another vector to this one. */
    constexpr Vector3& operator+= (const Vector3& other) noexcept
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    /** Subtracts another vector from this one. */
    constexpr Vector3& operator-= (const Vector3& other) noexcept
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    /** Scales this vector by a factor. */
    constexpr Vector3& operator*= (ValueType factor) noexcept
    {
        x *= factor;
        y *= factor;
        z *= factor;
        return *this;
    }

    //==============================================================================
    /** Returns true if the coordinates of both vectors are exactly equal. */
    constexpr bool operator== (const Vector3& other) const noexcept
    {
        return x == other.x && y == other.y && z == other.z;
    }

    /** Returns true if any coordinate differs. */
    constexpr bool operator!= (const Vector3& other) const noexcept
    {
        return ! (*this == other);
    }

    /** Returns true if the two vectors are approximately equal. */
    constexpr bool approximatelyEqualTo (const Vector3& other) const noexcept
    {
        if constexpr (std::is_floating_point_v<ValueType>)
        {
            return approximatelyEqual (x, other.x)
                && approximatelyEqual (y, other.y)
                && approximatelyEqual (z, other.z);
        }
        else
        {
            return *this == other;
        }
    }

    //==============================================================================
    /** Returns the coordinates as a comma separated string. */
    String toString() const
    {
        String result;
        result << x << ", " << y << ", " << z;
        return result;
    }

private:
    ValueType x = 0;
    ValueType y = 0;
    ValueType z = 0;
};

/** Returns a vector scaled by a factor. */
template <class ValueType>
[[nodiscard]] constexpr Vector3<ValueType> operator* (ValueType factor, const Vector3<ValueType>& v) noexcept
{
    return v * factor;
}

} // namespace yup
