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

Matrix4 CameraNode::getProjectionMatrix (float aspectRatio, bool depthZeroToOne) const noexcept
{
    const auto aspect = aspectRatio > 0.0f ? aspectRatio : 1.0f;

    if (projection == Projection::orthographic)
        return Matrix4::orthographic (-xMag, xMag, -yMag, yMag, zNear, zFar, depthZeroToOne);

    if (zFar > zNear)
        return Matrix4::perspective (yFov, aspect, zNear, zFar, depthZeroToOne);

    // Infinite far plane: the limit of Matrix4::perspective as the far plane goes to infinity
    const auto f = 1.0f / std::tan (yFov * 0.5f);
    const auto zOffset = depthZeroToOne ? -zNear : -2.0f * zNear;

    return Matrix4 (std::array<float, 16> { f / aspect, 0.0f, 0.0f, 0.0f,
                                            0.0f, f, 0.0f, 0.0f,
                                            0.0f, 0.0f, -1.0f, -1.0f,
                                            0.0f, 0.0f, zOffset, 0.0f });
}

Matrix4 CameraNode::getViewMatrix() const noexcept
{
    if (auto* entity = getEntity())
        return entity->getWorldMatrix().inverted();

    return {};
}

} // namespace yup
