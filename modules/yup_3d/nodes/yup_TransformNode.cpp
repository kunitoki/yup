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

TransformNode::TransformNode (const Vector3<float>& newPosition, const Quaternion& newRotation, const Vector3<float>& newScale)
    : position (newPosition)
    , rotation (newRotation)
    , scale (newScale)
{
}

//==============================================================================

void TransformNode::setPosition (const Vector3<float>& newPosition) noexcept
{
    position = newPosition;
    transformChanged();
}

void TransformNode::setRotation (const Quaternion& newRotation) noexcept
{
    rotation = newRotation;
    transformChanged();
}

void TransformNode::lookAt (const Vector3<float>& target, const Vector3<float>& up) noexcept
{
    const auto direction = target - position;
    if (direction.lengthSquared() > 0.0f)
        setRotation (Quaternion::fromLookDirection (direction, up));
}

void TransformNode::setScale (const Vector3<float>& newScale) noexcept
{
    scale = newScale;
    transformChanged();
}

const Matrix4& TransformNode::getLocalMatrix() const noexcept
{
    if (localDirty)
    {
        localMatrix = Matrix4::compose (position, rotation, scale);

        localDirty = false;
    }

    return localMatrix;
}

//==============================================================================

void TransformNode::attachedToEntity()
{
    getEntity().setTransform (this);
}

void TransformNode::detachedFromEntity()
{
    getEntity().setTransform (nullptr);
}

void TransformNode::transformChanged() noexcept
{
    localDirty = true;

    getEntity().invalidateWorldMatrix();
}

} // namespace yup
