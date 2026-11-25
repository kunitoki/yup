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
namespace
{

struct SceneCameraComponentIds
{
    static inline const Identifier fov { "fov" };
    static inline const Identifier nearPlane { "near" };
    static inline const Identifier farPlane { "far" };
    static inline const Identifier projection { "projection" };
    static inline const Identifier orthographicSize { "orthographicSize" };
    static inline const Identifier active { "active" };
};

} // namespace

//==============================================================================

SceneCameraComponent::SceneCameraComponent (StringRef componentID)
    : SceneNodeComponent (componentID)
{
}

//==============================================================================

template <class Value>
void SceneCameraComponent::setValue (Value& value, const Value& newValue, const Identifier& property)
{
    if (value == newValue)
        return;

    value = newValue;

    updateCamera();
    notifyPropertyChanged (property);
}

void SceneCameraComponent::setProjection (CameraNode::Projection newProjection)
{
    setValue (settings.projection, newProjection, SceneCameraComponentIds::projection);
}

void SceneCameraComponent::setFieldOfView (float newFieldOfView)
{
    setValue (settings.yFov, newFieldOfView, SceneCameraComponentIds::fov);
}

void SceneCameraComponent::setNearPlane (float newDistance)
{
    setValue (settings.zNear, newDistance, SceneCameraComponentIds::nearPlane);
}

void SceneCameraComponent::setFarPlane (float newDistance)
{
    setValue (settings.zFar, newDistance, SceneCameraComponentIds::farPlane);
}

void SceneCameraComponent::setOrthographicSize (float newSize)
{
    setValue (settings.yMag, newSize, SceneCameraComponentIds::orthographicSize);
}

void SceneCameraComponent::setActive (bool shouldBeActive)
{
    if (active == shouldBeActive)
        return;

    active = shouldBeActive;

    updateCamera();
    updateActiveCamera();
    notifyPropertyChanged (SceneCameraComponentIds::active);
}

//==============================================================================

void SceneCameraComponent::updateCamera()
{
    auto entity = getEntity();

    if (! entity.isValid())
        return;

    if (active)
    {
        auto& camera = entity.getOrAttach<CameraNode>();

        camera.projection = settings.projection;
        camera.yFov = settings.yFov;
        camera.zNear = settings.zNear;
        camera.zFar = settings.zFar;
        camera.yMag = settings.yMag;

        // An orthographic view is as wide as the shape of the scene, which the node has, makes it
        camera.xMag = getHeight() > 0.0f ? settings.yMag * getWidth() / getHeight() : settings.yMag;
    }
    else
    {
        // Without its part the scene can't fall back on a camera that was turned off
        entity.detach<CameraNode>();
    }

    repaintScene();
}

void SceneCameraComponent::updateActiveCamera()
{
    if (auto* view = SceneComponent::findFor (*this))
        view->updateActiveCamera();
}

void SceneCameraComponent::entityChanged()
{
    updateCamera();
    updateActiveCamera();
}

void SceneCameraComponent::parentHierarchyChanged()
{
    SceneNodeComponent::parentHierarchyChanged();

    // Its place among the components may be another one
    updateActiveCamera();
}

void SceneCameraComponent::resized()
{
    SceneNodeComponent::resized();
    updateCamera();
}

//==============================================================================

const ComponentTypeInfo& SceneCameraComponent::getClassTypeInfo()
{
    static const auto info = []
    {
        const CameraNode defaults {};

        return ComponentTypeBuilder<SceneCameraComponent> (Identifier ("Camera3D"), SceneNodeComponent::getClassTypeInfo())
            .styleableProperty (SceneCameraComponentIds::fov, &SceneCameraComponent::getFieldOfView, &SceneCameraComponent::setFieldOfView, defaults.yFov, "The vertical field of view of a perspective camera, in radians.")
            .styleableProperty (SceneCameraComponentIds::nearPlane, &SceneCameraComponent::getNearPlane, &SceneCameraComponent::setNearPlane, defaults.zNear, "The distance of the near clipping plane.")
            .styleableProperty (SceneCameraComponentIds::farPlane, &SceneCameraComponent::getFarPlane, &SceneCameraComponent::setFarPlane, defaults.zFar, "The distance of the far clipping plane, 0 for a perspective camera to see infinitely far.")
            .enumeration<CameraNode::Projection> (
                SceneCameraComponentIds::projection,
                { { "perspective", CameraNode::Projection::perspective },
                  { "orthographic", CameraNode::Projection::orthographic } },
                &SceneCameraComponent::getProjection,
                &SceneCameraComponent::setProjection,
                "Whether what is far looks smaller (perspective) or keeps its size (orthographic).")
            .styleableProperty (SceneCameraComponentIds::orthographicSize, &SceneCameraComponent::getOrthographicSize, &SceneCameraComponent::setOrthographicSize, defaults.yMag, "Half the height of what an orthographic camera sees, in world units. The width follows the shape of the scene.")
            .styleableProperty (SceneCameraComponentIds::active, &SceneCameraComponent::isActive, &SceneCameraComponent::setActive, true, "Whether the camera may be the one of the scene, which is the first active camera written in it.")
            .build();
    }();

    return info;
}

} // namespace yup
