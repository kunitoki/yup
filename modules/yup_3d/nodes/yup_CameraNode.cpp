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
    if (const auto entity = getEntity())
        return entity.getWorldMatrix().inverted();

    return {};
}

float CameraNode::getCircleOfConfusion (float distance, float viewportHeight) const noexcept
{
    if (projection != Projection::perspective || fStop <= 0.0f || yFov <= 0.0f || distance <= 0.0f || viewportHeight <= 0.0f)
        return 0.0f;

    // Thin lens (Jimenez 2014): the focal length follows from the field of view of the sensor
    const auto focalLength = sensorHeight / (2.0f * std::tan (yFov * 0.5f));
    const auto focus = jmax (focusDistance, focalLength * 1.001f);
    const auto aperture = focalLength / fStop;

    // (distance - focus) / distance as 1 - focus / distance, so an infinite distance stays finite
    const auto onSensor = aperture * focalLength * (1.0f - focus / distance) / (focus - focalLength);
    return onSensor / sensorHeight * viewportHeight;
}

} // namespace yup
