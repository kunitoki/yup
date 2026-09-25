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

Matrix4::Matrix4 (const rive::Mat4& mat4) noexcept
{
    std::copy (mat4.values(), mat4.values() + 16, m.begin());
}

rive::Mat4 Matrix4::toMat4() const noexcept
{
    rive::Mat4 result;
    std::copy (m.begin(), m.end(), result.values());
    return result;
}

//==============================================================================

Matrix4 Matrix4::translation (const Vector3<float>& offset) noexcept
{
    return Matrix4 (std::array<float, 16> { 1.0f, 0.0f, 0.0f, 0.0f,
                                            0.0f, 1.0f, 0.0f, 0.0f,
                                            0.0f, 0.0f, 1.0f, 0.0f,
                                            offset.getX(), offset.getY(), offset.getZ(), 1.0f });
}

Matrix4 Matrix4::scaling (const Vector3<float>& factors) noexcept
{
    return Matrix4 (std::array<float, 16> { factors.getX(), 0.0f, 0.0f, 0.0f,
                                            0.0f, factors.getY(), 0.0f, 0.0f,
                                            0.0f, 0.0f, factors.getZ(), 0.0f,
                                            0.0f, 0.0f, 0.0f, 1.0f });
}

Matrix4 Matrix4::scaling (float factor) noexcept
{
    return scaling ({ factor, factor, factor });
}

Matrix4 Matrix4::rotationX (float angleInRadians) noexcept
{
    const auto c = std::cos (angleInRadians);
    const auto s = std::sin (angleInRadians);

    return Matrix4 (std::array<float, 16> { 1.0f, 0.0f, 0.0f, 0.0f,
                                            0.0f, c, s, 0.0f,
                                            0.0f, -s, c, 0.0f,
                                            0.0f, 0.0f, 0.0f, 1.0f });
}

Matrix4 Matrix4::rotationY (float angleInRadians) noexcept
{
    const auto c = std::cos (angleInRadians);
    const auto s = std::sin (angleInRadians);

    return Matrix4 (std::array<float, 16> { c, 0.0f, -s, 0.0f,
                                            0.0f, 1.0f, 0.0f, 0.0f,
                                            s, 0.0f, c, 0.0f,
                                            0.0f, 0.0f, 0.0f, 1.0f });
}

Matrix4 Matrix4::rotationZ (float angleInRadians) noexcept
{
    const auto c = std::cos (angleInRadians);
    const auto s = std::sin (angleInRadians);

    return Matrix4 (std::array<float, 16> { c, s, 0.0f, 0.0f,
                                            -s, c, 0.0f, 0.0f,
                                            0.0f, 0.0f, 1.0f, 0.0f,
                                            0.0f, 0.0f, 0.0f, 1.0f });
}

Matrix4 Matrix4::rotation (const Vector3<float>& axis, float angleInRadians) noexcept
{
    if (axis.lengthSquared() == 0.0f)
        return identity();

    const auto n = axis.normalized();
    const auto x = n.getX();
    const auto y = n.getY();
    const auto z = n.getZ();
    const auto c = std::cos (angleInRadians);
    const auto s = std::sin (angleInRadians);
    const auto t = 1.0f - c;

    // Rodrigues' rotation formula, written column by column
    return Matrix4 (std::array<float, 16> { t * x * x + c, t * x * y + s * z, t * x * z - s * y, 0.0f,
                                            t * x * y - s * z, t * y * y + c, t * y * z + s * x, 0.0f,
                                            t * x * z + s * y, t * y * z - s * x, t * z * z + c, 0.0f,
                                            0.0f, 0.0f, 0.0f, 1.0f });
}

Matrix4 Matrix4::perspective (float fovYInRadians, float aspectRatio, float nearPlane, float farPlane, bool depthZeroToOne) noexcept
{
    jassert (nearPlane > 0.0f && farPlane > nearPlane && aspectRatio > 0.0f);

    const auto f = 1.0f / std::tan (fovYInRadians * 0.5f);
    const auto rangeInverse = 1.0f / (nearPlane - farPlane);

    const auto zScale = depthZeroToOne ? farPlane * rangeInverse : (farPlane + nearPlane) * rangeInverse;
    const auto zOffset = depthZeroToOne ? farPlane * nearPlane * rangeInverse : 2.0f * farPlane * nearPlane * rangeInverse;

    return Matrix4 (std::array<float, 16> { f / aspectRatio, 0.0f, 0.0f, 0.0f,
                                            0.0f, f, 0.0f, 0.0f,
                                            0.0f, 0.0f, zScale, -1.0f,
                                            0.0f, 0.0f, zOffset, 0.0f });
}

Matrix4 Matrix4::orthographic (float left, float right, float bottom, float top, float nearPlane, float farPlane, bool depthZeroToOne) noexcept
{
    const auto depth = farPlane - nearPlane;

    const auto zScale = depthZeroToOne ? -1.0f / depth : -2.0f / depth;
    const auto zOffset = depthZeroToOne ? -nearPlane / depth : -(farPlane + nearPlane) / depth;

    return Matrix4 (std::array<float, 16> { 2.0f / (right - left), 0.0f, 0.0f, 0.0f,
                                            0.0f, 2.0f / (top - bottom), 0.0f, 0.0f,
                                            0.0f, 0.0f, zScale, 0.0f,
                                            -(right + left) / (right - left), -(top + bottom) / (top - bottom), zOffset, 1.0f });
}

Matrix4 Matrix4::lookAt (const Vector3<float>& eye, const Vector3<float>& target, const Vector3<float>& up) noexcept
{
    const auto forward = (target - eye).normalized();
    const auto side = forward.crossProduct (up).normalized();
    const auto trueUp = side.crossProduct (forward);

    return Matrix4 (std::array<float, 16> { side.getX(), trueUp.getX(), -forward.getX(), 0.0f,
                                            side.getY(), trueUp.getY(), -forward.getY(), 0.0f,
                                            side.getZ(), trueUp.getZ(), -forward.getZ(), 0.0f,
                                            -side.dotProduct (eye), -trueUp.dotProduct (eye), forward.dotProduct (eye), 1.0f });
}

Matrix4 Matrix4::fromAffineTransform (const AffineTransform& transform) noexcept
{
    return Matrix4 (std::array<float, 16> { transform.getScaleX(), transform.getShearY(), 0.0f, 0.0f,
                                            transform.getShearX(), transform.getScaleY(), 0.0f, 0.0f,
                                            0.0f, 0.0f, 1.0f, 0.0f,
                                            transform.getTranslateX(), transform.getTranslateY(), 0.0f, 1.0f });
}

//==============================================================================

Matrix4 Matrix4::followedBy (const Matrix4& other) const noexcept
{
    // With column vectors, applying this first and then other is other * this
    std::array<float, 16> result {};

    for (std::size_t column = 0; column < 4; ++column)
    {
        for (std::size_t row = 0; row < 4; ++row)
        {
            float sum = 0.0f;

            for (std::size_t k = 0; k < 4; ++k)
                sum += other.m[k * 4 + row] * m[column * 4 + k];

            result[column * 4 + row] = sum;
        }
    }

    return Matrix4 (result);
}

Matrix4 Matrix4::inverted() const noexcept
{
    // Laplace expansion over 2x2 sub-determinants. Inverting commutes with transposing, so the
    // textbook row-major formula can be applied as it is to the column-major storage.
    const auto s0 = m[0] * m[5] - m[4] * m[1];
    const auto s1 = m[0] * m[6] - m[4] * m[2];
    const auto s2 = m[0] * m[7] - m[4] * m[3];
    const auto s3 = m[1] * m[6] - m[5] * m[2];
    const auto s4 = m[1] * m[7] - m[5] * m[3];
    const auto s5 = m[2] * m[7] - m[6] * m[3];

    const auto c0 = m[8] * m[13] - m[12] * m[9];
    const auto c1 = m[8] * m[14] - m[12] * m[10];
    const auto c2 = m[8] * m[15] - m[12] * m[11];
    const auto c3 = m[9] * m[14] - m[13] * m[10];
    const auto c4 = m[9] * m[15] - m[13] * m[11];
    const auto c5 = m[10] * m[15] - m[14] * m[11];

    const auto determinant = s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0;
    if (determinant == 0.0f)
        return identity();

    const auto d = 1.0f / determinant;

    return Matrix4 (std::array<float, 16> {
        (m[5] * c5 - m[6] * c4 + m[7] * c3) * d,
        (-m[1] * c5 + m[2] * c4 - m[3] * c3) * d,
        (m[13] * s5 - m[14] * s4 + m[15] * s3) * d,
        (-m[9] * s5 + m[10] * s4 - m[11] * s3) * d,

        (-m[4] * c5 + m[6] * c2 - m[7] * c1) * d,
        (m[0] * c5 - m[2] * c2 + m[3] * c1) * d,
        (-m[12] * s5 + m[14] * s2 - m[15] * s1) * d,
        (m[8] * s5 - m[10] * s2 + m[11] * s1) * d,

        (m[4] * c4 - m[5] * c2 + m[7] * c0) * d,
        (-m[0] * c4 + m[1] * c2 - m[3] * c0) * d,
        (m[12] * s4 - m[13] * s2 + m[15] * s0) * d,
        (-m[8] * s4 + m[9] * s2 - m[11] * s0) * d,

        (-m[4] * c3 + m[5] * c1 - m[6] * c0) * d,
        (m[0] * c3 - m[1] * c1 + m[2] * c0) * d,
        (-m[12] * s3 + m[13] * s1 - m[14] * s0) * d,
        (m[8] * s3 - m[9] * s1 + m[10] * s0) * d });
}

Matrix4 Matrix4::transposed() const noexcept
{
    std::array<float, 16> result {};

    for (std::size_t row = 0; row < 4; ++row)
        for (std::size_t column = 0; column < 4; ++column)
            result[row * 4 + column] = m[column * 4 + row];

    return Matrix4 (result);
}

//==============================================================================

Vector3<float> Matrix4::transformPoint (const Vector3<float>& point) const noexcept
{
    const auto [x, y, z, w] = transformPoint4 (point.getX(), point.getY(), point.getZ(), 1.0f);

    if (w == 0.0f)
        return { x, y, z };

    return { x / w, y / w, z / w };
}

Vector3<float> Matrix4::transformVector (const Vector3<float>& vector) const noexcept
{
    const auto result = transformPoint4 (vector.getX(), vector.getY(), vector.getZ(), 0.0f);

    return { result[0], result[1], result[2] };
}

std::array<float, 4> Matrix4::transformPoint4 (float x, float y, float z, float w) const noexcept
{
    return { m[0] * x + m[4] * y + m[8] * z + m[12] * w,
             m[1] * x + m[5] * y + m[9] * z + m[13] * w,
             m[2] * x + m[6] * y + m[10] * z + m[14] * w,
             m[3] * x + m[7] * y + m[11] * z + m[15] * w };
}

//==============================================================================

bool Matrix4::approximatelyEqualTo (const Matrix4& other, float tolerance) const noexcept
{
    for (std::size_t i = 0; i < m.size(); ++i)
    {
        if (std::abs (m[i] - other.m[i]) > tolerance)
            return false;
    }

    return true;
}

} // namespace yup
