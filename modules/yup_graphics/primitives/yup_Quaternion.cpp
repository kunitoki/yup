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

Quaternion Quaternion::fromAxisAngle (const Vector3<float>& axis, float angleInRadians) noexcept
{
    if (axis.lengthSquared() == 0.0f)
        return identity();

    const auto n = axis.normalized();
    const auto s = std::sin (angleInRadians * 0.5f);

    return { n.getX() * s, n.getY() * s, n.getZ() * s, std::cos (angleInRadians * 0.5f) };
}

Quaternion Quaternion::fromRotationMatrix (const Matrix4& matrix) noexcept
{
    const auto r00 = matrix (0, 0);
    const auto r11 = matrix (1, 1);
    const auto r22 = matrix (2, 2);
    const auto trace = r00 + r11 + r22;

    // Shepperd's method: divide by the largest of the four candidates for stability
    if (trace > 0.0f)
    {
        const auto s = std::sqrt (trace + 1.0f) * 2.0f;
        return Quaternion ((matrix (2, 1) - matrix (1, 2)) / s,
                           (matrix (0, 2) - matrix (2, 0)) / s,
                           (matrix (1, 0) - matrix (0, 1)) / s,
                           0.25f * s)
            .normalized();
    }

    if (r00 > r11 && r00 > r22)
    {
        const auto s = std::sqrt (1.0f + r00 - r11 - r22) * 2.0f;
        return Quaternion (0.25f * s,
                           (matrix (0, 1) + matrix (1, 0)) / s,
                           (matrix (0, 2) + matrix (2, 0)) / s,
                           (matrix (2, 1) - matrix (1, 2)) / s)
            .normalized();
    }

    if (r11 > r22)
    {
        const auto s = std::sqrt (1.0f + r11 - r00 - r22) * 2.0f;
        return Quaternion ((matrix (0, 1) + matrix (1, 0)) / s,
                           0.25f * s,
                           (matrix (1, 2) + matrix (2, 1)) / s,
                           (matrix (0, 2) - matrix (2, 0)) / s)
            .normalized();
    }

    const auto s = std::sqrt (1.0f + r22 - r00 - r11) * 2.0f;
    return Quaternion ((matrix (0, 2) + matrix (2, 0)) / s,
                       (matrix (1, 2) + matrix (2, 1)) / s,
                       0.25f * s,
                       (matrix (1, 0) - matrix (0, 1)) / s)
        .normalized();
}

//==============================================================================

float Quaternion::length() const noexcept
{
    return std::sqrt (x * x + y * y + z * z + w * w);
}

Quaternion Quaternion::normalized() const noexcept
{
    const auto len = length();
    if (len == 0.0f)
        return identity();

    return { x / len, y / len, z / len, w / len };
}

//==============================================================================

Vector3<float> Quaternion::rotate (const Vector3<float>& vector) const noexcept
{
    const Vector3<float> axis { x, y, z };
    const auto t = axis.crossProduct (vector) * 2.0f;

    return vector + t * w + axis.crossProduct (t);
}

Matrix4 Quaternion::toMatrix4() const noexcept
{
    const auto xx = x * x;
    const auto yy = y * y;
    const auto zz = z * z;
    const auto xy = x * y;
    const auto xz = x * z;
    const auto yz = y * z;
    const auto wx = w * x;
    const auto wy = w * y;
    const auto wz = w * z;

    return Matrix4 (std::array<float, 16> { 1.0f - 2.0f * (yy + zz), 2.0f * (xy + wz), 2.0f * (xz - wy), 0.0f,
                                            2.0f * (xy - wz), 1.0f - 2.0f * (xx + zz), 2.0f * (yz + wx), 0.0f,
                                            2.0f * (xz + wy), 2.0f * (yz - wx), 1.0f - 2.0f * (xx + yy), 0.0f,
                                            0.0f, 0.0f, 0.0f, 1.0f });
}

//==============================================================================

bool Quaternion::approximatelyEqualTo (const Quaternion& other, float tolerance) const noexcept
{
    return std::abs (x - other.x) <= tolerance
        && std::abs (y - other.y) <= tolerance
        && std::abs (z - other.z) <= tolerance
        && std::abs (w - other.w) <= tolerance;
}

} // namespace yup
