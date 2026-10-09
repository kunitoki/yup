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
/** A rotation in 3D space, stored as a quaternion (x, y, z, w).

    The vector part is (x, y, z) and the scalar part is w, the same order glTF uses. Rotations
    follow the right-handed convention of Matrix4: a positive angle turns counter-clockwise when
    looking down the axis towards the origin.

    Quaternions compose with the Hamilton product: (a * b) rotates by b first and then by a, so
    (a * b).toMatrix4() equals b.toMatrix4().followedBy (a.toMatrix4()).

    Only unit quaternions describe rotations. The factories return unit quaternions; use
    normalized() after accumulating many products.

    @see Matrix4, Vector3
*/
class YUP_API Quaternion
{
public:
    //==============================================================================
    /** Constructs the identity rotation. */
    constexpr Quaternion() noexcept = default;

    /** Constructs a quaternion from its components.

        @param newX The x component of the vector part.
        @param newY The y component of the vector part.
        @param newZ The z component of the vector part.
        @param newW The scalar part.
    */
    constexpr Quaternion (float newX, float newY, float newZ, float newW) noexcept
        : x (newX)
        , y (newY)
        , z (newZ)
        , w (newW)
    {
    }

    constexpr Quaternion (const Quaternion& other) noexcept = default;
    constexpr Quaternion (Quaternion&& other) noexcept = default;
    constexpr Quaternion& operator= (const Quaternion& other) noexcept = default;
    constexpr Quaternion& operator= (Quaternion&& other) noexcept = default;

    //==============================================================================
    /** Returns the identity rotation. */
    [[nodiscard]] static constexpr Quaternion identity() noexcept { return {}; }

    /** Returns a rotation around an arbitrary axis.

        @param axis            The rotation axis. It doesn't need to be normalized; a zero axis gives the identity.
        @param angleInRadians  The rotation angle, counter-clockwise when looking down the axis towards the origin.
    */
    [[nodiscard]] static Quaternion fromAxisAngle (const Vector3<float>& axis, float angleInRadians) noexcept;

    /** Returns the rotation held by the upper 3x3 part of a matrix.

        The translation is ignored. The 3x3 part must be a pure rotation: remove any scale from
        its columns first.

        @param matrix The rotation matrix.
    */
    [[nodiscard]] static Quaternion fromRotationMatrix (const Matrix4& matrix) noexcept;

    //==============================================================================
    /** Returns the x component of the vector part. */
    [[nodiscard]] constexpr float getX() const noexcept { return x; }

    /** Returns the y component of the vector part. */
    [[nodiscard]] constexpr float getY() const noexcept { return y; }

    /** Returns the z component of the vector part. */
    [[nodiscard]] constexpr float getZ() const noexcept { return z; }

    /** Returns the scalar part. */
    [[nodiscard]] constexpr float getW() const noexcept { return w; }

    //==============================================================================
    /** Returns the Hamilton product: the rotation by @a other followed by this rotation. */
    [[nodiscard]] constexpr Quaternion operator* (const Quaternion& other) const noexcept
    {
        return { w * other.x + x * other.w + y * other.z - z * other.y,
                 w * other.y - x * other.z + y * other.w + z * other.x,
                 w * other.z + x * other.y - y * other.x + z * other.w,
                 w * other.w - x * other.x - y * other.y - z * other.z };
    }

    /** Returns the conjugate, which is the inverse rotation for a unit quaternion. */
    [[nodiscard]] constexpr Quaternion conjugate() const noexcept { return { -x, -y, -z, w }; }

    /** Returns the length of the quaternion. */
    [[nodiscard]] float length() const noexcept;

    /** Returns this quaternion scaled to unit length, or the identity if its length is zero. */
    [[nodiscard]] Quaternion normalized() const noexcept;

    //==============================================================================
    /** Rotates a vector.

        @param vector The vector to rotate.

        @return The rotated vector.
    */
    [[nodiscard]] Vector3<float> rotate (const Vector3<float>& vector) const noexcept;

    /** Returns the rotation as a matrix. */
    [[nodiscard]] Matrix4 toMatrix4() const noexcept;

    //==============================================================================
    constexpr bool operator== (const Quaternion& other) const noexcept
    {
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }

    constexpr bool operator!= (const Quaternion& other) const noexcept { return ! (*this == other); }

    /** Returns true if every component is within @a tolerance of the other quaternion. */
    bool approximatelyEqualTo (const Quaternion& other, float tolerance = 1.0e-5f) const noexcept;

private:
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;
};

} // namespace yup
