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
/** The position, rotation and scale of an entity relative to its parent.

    An entity without a TransformNode has the identity transform and follows its parent, so
    only the entities that need to be placed carry one.

    @code
    yup::Entity lamp = scene->createEntity ("lamp");
    lamp.attach<yup::TransformNode> (yup::Vector3<float> { 0.0f, 2.0f, 0.0f });
    lamp.attach<yup::LightNode> (yup::LightNode::Type::point);

    lamp.get<yup::TransformNode>()->setScale ({ 2.0f, 2.0f, 2.0f });
    @endcode

    Changing the transform, attaching it or detaching it marks the world matrices of the
    subtree, which are computed again when asked for, see Entity::getWorldMatrix().

    @see Entity
*/
class YUP_API TransformNode final : public Node
{
public:
    //==============================================================================
    /** Creates the identity transform. */
    TransformNode() = default;

    /** Creates a transform.

        @param position  The translation relative to the parent.
        @param rotation  The rotation relative to the parent.
        @param scale     The scale relative to the parent.
    */
    explicit TransformNode (const Vector3<float>& position, const Quaternion& rotation = {}, const Vector3<float>& scale = { 1.0f, 1.0f, 1.0f });

    //==============================================================================
    /** Returns the translation relative to the parent. */
    const Vector3<float>& getPosition() const noexcept { return position; }

    /** Changes the translation relative to the parent. */
    void setPosition (const Vector3<float>& newPosition) noexcept;

    /** Returns the rotation relative to the parent. */
    const Quaternion& getRotation() const noexcept { return rotation; }

    /** Changes the rotation relative to the parent. */
    void setRotation (const Quaternion& newRotation) noexcept;

    /** Turns the entity so that its -Z axis points at a target and its +Y axis is as close to @p up as possible.

        Cameras and lights look along -Z, so this aims them. Nothing changes if the target is at the
        position. See Quaternion::fromLookDirection().

        @param target The point to look at, relative to the parent like the position.
        @param up     The direction that should appear upwards, relative to the parent.
    */
    void lookAt (const Vector3<float>& target, const Vector3<float>& up = { 0.0f, 1.0f, 0.0f }) noexcept;

    /** Returns the scale relative to the parent. */
    const Vector3<float>& getScale() const noexcept { return scale; }

    /** Changes the scale relative to the parent. */
    void setScale (const Vector3<float>& newScale) noexcept;

    /** Returns the local transform: scale, then rotation, then translation. The matrix is
        cached and only computed again after the transform changes.
    */
    const Matrix4& getLocalMatrix() const noexcept;

    //==============================================================================
    /** @internal */
    void attachedToEntity() override;
    /** @internal */
    void detachedFromEntity() override;

private:
    void transformChanged() noexcept;

    Vector3<float> position;
    Quaternion rotation;
    Vector3<float> scale { 1.0f, 1.0f, 1.0f };

    mutable Matrix4 localMatrix;
    mutable bool localDirty = true;
};

} // namespace yup
