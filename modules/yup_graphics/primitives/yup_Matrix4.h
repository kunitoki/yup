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

namespace rive
{
class Mat4;
} // namespace rive

namespace yup
{

//==============================================================================
/** A 4x4 single precision matrix for 3D transformations and projections.

    The storage is column-major, the layout GPU shaders expect, so getData() can be uploaded
    directly as a uniform. Points are treated as column vectors (M * p).

    Matrices compose like AffineTransform: a.followedBy (b) applies a first and then b, so a
    model-view-projection matrix is built as model.followedBy (view).followedBy (projection).

    The projection factories follow the right-handed convention of the GPU backends (camera
    looking down -z), with a choice of [0, 1] or [-1, 1] depth range.

    @see Vector3, Ray, MeshSurfaceMapper, AffineTransform
*/
class YUP_API Matrix4
{
public:
    //==============================================================================
    /** Constructs an identity matrix. */
    constexpr Matrix4() noexcept
        : m { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f }
    {
    }

    /** Constructs a matrix from 16 values in column-major order.

        @param columnMajorValues The values, column after column.
    */
    explicit constexpr Matrix4 (const std::array<float, 16>& columnMajorValues) noexcept
        : m (columnMajorValues)
    {
    }

    /** Copy and move constructors and assignment operators. */
    constexpr Matrix4 (const Matrix4& other) noexcept = default;
    constexpr Matrix4 (Matrix4&& other) noexcept = default;
    constexpr Matrix4& operator= (const Matrix4& other) noexcept = default;
    constexpr Matrix4& operator= (Matrix4&& other) noexcept = default;

    //==============================================================================
    /** Returns the identity matrix. */
    [[nodiscard]] static constexpr Matrix4 identity() noexcept { return {}; }

    /** Returns true if this is the identity matrix. */
    [[nodiscard]] constexpr bool isIdentity() const noexcept { return *this == identity(); }

    /** Returns a translation matrix.

        @param offset The translation to apply.
    */
    [[nodiscard]] static Matrix4 translation (const Vector3<float>& offset) noexcept;

    /** Returns a scaling matrix.

        @param factors The scale factor along each axis.
    */
    [[nodiscard]] static Matrix4 scaling (const Vector3<float>& factors) noexcept;

    /** Returns a uniform scaling matrix.

        @param factor The scale factor applied to all axes.
    */
    [[nodiscard]] static Matrix4 scaling (float factor) noexcept;

    /** Returns a rotation around the x axis.

        @param angleInRadians The rotation angle, counter-clockwise when looking down the axis towards the origin.
    */
    [[nodiscard]] static Matrix4 rotationX (float angleInRadians) noexcept;

    /** Returns a rotation around the y axis.

        @param angleInRadians The rotation angle, counter-clockwise when looking down the axis towards the origin.
    */
    [[nodiscard]] static Matrix4 rotationY (float angleInRadians) noexcept;

    /** Returns a rotation around the z axis.

        @param angleInRadians The rotation angle, counter-clockwise when looking down the axis towards the origin.
    */
    [[nodiscard]] static Matrix4 rotationZ (float angleInRadians) noexcept;

    /** Returns a rotation around an arbitrary axis.

        @param axis            The rotation axis. It doesn't need to be normalized; a zero axis gives the identity.
        @param angleInRadians  The rotation angle, counter-clockwise when looking down the axis towards the origin.
    */
    [[nodiscard]] static Matrix4 rotation (const Vector3<float>& axis, float angleInRadians) noexcept;

    /** Returns a right-handed perspective projection.

        View space z between -nearPlane and -farPlane maps to the depth range of the backend.

        @param fovYInRadians   The vertical field of view.
        @param aspectRatio     The viewport width divided by its height.
        @param nearPlane       The distance of the near clipping plane, must be positive.
        @param farPlane        The distance of the far clipping plane, must be larger than nearPlane.
        @param depthZeroToOne  True for a [0, 1] depth range (Metal, D3D, WebGPU), false for [-1, 1] (OpenGL).
    */
    [[nodiscard]] static Matrix4 perspective (float fovYInRadians, float aspectRatio, float nearPlane, float farPlane, bool depthZeroToOne = true) noexcept;

    /** Returns a right-handed orthographic projection.

        @param left            The left edge of the view volume.
        @param right           The right edge of the view volume.
        @param bottom          The bottom edge of the view volume.
        @param top             The top edge of the view volume.
        @param nearPlane       The distance of the near clipping plane.
        @param farPlane        The distance of the far clipping plane.
        @param depthZeroToOne  True for a [0, 1] depth range, false for [-1, 1].
    */
    [[nodiscard]] static Matrix4 orthographic (float left, float right, float bottom, float top, float nearPlane, float farPlane, bool depthZeroToOne = true) noexcept;

    /** Returns a right-handed view matrix for a camera at @a eye looking at @a target.

        @param eye     The camera position.
        @param target  The point the camera looks at.
        @param up      The up direction of the camera, it doesn't need to be normalized.
    */
    [[nodiscard]] static Matrix4 lookAt (const Vector3<float>& eye, const Vector3<float>& target, const Vector3<float>& up) noexcept;

    /** Returns the 3D embedding of a 2D affine transform, acting on x and y and leaving z untouched.

        @param transform The 2D transform to embed.
    */
    [[nodiscard]] static Matrix4 fromAffineTransform (const AffineTransform& transform) noexcept;

    //==============================================================================
    /** Returns a matrix that applies this one first and then @a other.

        @param other The transformation applied after this one.
    */
    [[nodiscard]] Matrix4 followedBy (const Matrix4& other) const noexcept;

    /** Same as followedBy(). */
    [[nodiscard]] Matrix4 operator* (const Matrix4& other) const noexcept { return followedBy (other); }

    /** Returns the inverse of this matrix, or the identity if the matrix is singular. */
    [[nodiscard]] Matrix4 inverted() const noexcept;

    /** Returns the transposed matrix. */
    [[nodiscard]] Matrix4 transposed() const noexcept;

    //==============================================================================
    /** Transforms a point, including the perspective divide.

        When the resulting w is zero the divide is skipped.

        @param point The point to transform.

        @return The transformed point.
    */
    [[nodiscard]] Vector3<float> transformPoint (const Vector3<float>& point) const noexcept;

    /** Transforms a direction, ignoring the translation and the perspective row.

        @param vector The direction to transform.

        @return The transformed direction.
    */
    [[nodiscard]] Vector3<float> transformVector (const Vector3<float>& vector) const noexcept;

    /** Transforms a homogeneous point without the perspective divide.

        This returns clip space coordinates for a projection matrix: a w lower or equal to zero
        means the point lies behind the camera.

        @return The transformed x, y, z and w.
    */
    [[nodiscard]] std::array<float, 4> transformPoint4 (float x, float y, float z, float w) const noexcept;

    //==============================================================================
    /** Returns the 16 values in column-major order, ready to be uploaded to the GPU. */
    [[nodiscard]] const float* getData() const noexcept { return m.data(); }

    /** Returns the value at the given row and column. */
    [[nodiscard]] constexpr float operator() (int row, int column) const noexcept
    {
        jassert (isPositiveAndBelow (row, 4) && isPositiveAndBelow (column, 4));
        return m[static_cast<std::size_t> (column * 4 + row)];
    }

    //==============================================================================
    /** Returns true if all the values are exactly equal. */
    constexpr bool operator== (const Matrix4& other) const noexcept { return m == other.m; }

    /** Returns true if any value differs. */
    constexpr bool operator!= (const Matrix4& other) const noexcept { return ! (*this == other); }

    /** Returns true if all the values are equal within @a tolerance. */
    bool approximatelyEqualTo (const Matrix4& other, float tolerance = 1.0e-5f) const noexcept;

    //==============================================================================
    /** @internal Conversion from the Rive Mat4 class, which uses the same column-major layout. */
    explicit Matrix4 (const rive::Mat4& mat4) noexcept;

    /** @internal Conversion to the Rive Mat4 class. */
    rive::Mat4 toMat4() const noexcept;

private:
    std::array<float, 16> m;
};

} // namespace yup
