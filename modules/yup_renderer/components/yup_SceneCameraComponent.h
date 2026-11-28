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

#pragma once

namespace yup
{

//==============================================================================
/** A node that views the scene: the `Camera3D` of a `.yui` document.

    The camera looks down the -z axis of its entity with +y up, so it is aimed with the
    `position`, the `rotation` and the `lookAt` of its node:

    @code
    Scene3D {
        Camera3D { position: [0, 0.5, 4]; lookAt: [0, 0, 0]; fov: 45deg; }
    }
    @endcode

    The first camera of the scene that is `active`, in the order the components are written,
    is the camera of the scene (see Scene::setActiveCamera). It is chosen again each time a
    camera comes, goes, moves among the components or changes `active`, so turning one off
    hands the view over to the next. A camera that is not active has no CameraNode. Without
    any active camera the scene falls back on a camera of a model, or on its default one.

    The depth of field of CameraNode has no property: set it on the CameraNode of getEntity(),
    which keeps it for as long as the camera is active and keeps its entity, so also when it
    moves to another parent of its scene.

    A value a camera can't have is reported in the diagnostics of the document, and the camera
    keeps the one it had: a field of view that is not above 0 and below 180deg, a near plane or
    an orthographic size that is not above 0, a far plane that is negative, and for any of them
    a value that is not a finite number. So is an orthographic camera whose far plane is not
    beyond its near one, as `far: 0` leaves it: it has no infinite far plane.

    @see SceneNodeComponent, CameraNode
*/
class YUP_API SceneCameraComponent : public SceneNodeComponent
{
public:
    //==============================================================================
    /** Creates an active perspective camera.

        @param componentID The ID of the component.
    */
    explicit SceneCameraComponent (StringRef componentID = {});

    //==============================================================================
    /** Changes the kind of projection, the `projection` of a `Camera3D`. */
    void setProjection (CameraNode::Projection newProjection);

    /** Returns the kind of projection. */
    CameraNode::Projection getProjection() const noexcept { return settings.projection; }

    /** Changes the vertical field of view of a perspective camera, the `fov` of a `Camera3D`.

        @param newFieldOfView The angle in radians.
    */
    void setFieldOfView (float newFieldOfView);

    /** Returns the vertical field of view of a perspective camera, in radians. */
    float getFieldOfView() const noexcept { return settings.yFov; }

    /** Changes the distance of the near clipping plane, the `near` of a `Camera3D`. */
    void setNearPlane (float newDistance);

    /** Returns the distance of the near clipping plane. */
    float getNearPlane() const noexcept { return settings.zNear; }

    /** Changes the distance of the far clipping plane, the `far` of a `Camera3D`.

        @param newDistance The distance, or 0 for a perspective camera to see infinitely far.
    */
    void setFarPlane (float newDistance);

    /** Returns the distance of the far clipping plane, 0 for infinite. */
    float getFarPlane() const noexcept { return settings.zFar; }

    /** Changes how much an orthographic camera sees, the `orthographicSize` of a `Camera3D`.

        @param newSize Half the height of the view, in world units. Its width follows the shape
                       of the SceneComponent the camera is in.
    */
    void setOrthographicSize (float newSize);

    /** Returns half the height of the view of an orthographic camera, in world units. */
    float getOrthographicSize() const noexcept { return settings.yMag; }

    //==============================================================================
    /** Chooses whether the camera may be the one of the scene, the `active` of a `Camera3D`.

        @param shouldBeActive False to leave the view to the next active camera of the scene.
    */
    void setActive (bool shouldBeActive);

    /** Returns true if the camera may be the one of the scene. */
    bool isActive() const noexcept { return active; }

    //==============================================================================
    /** Returns the static type info describing the Camera3D type. */
    static const ComponentTypeInfo& getClassTypeInfo();

    /** @internal */
    const ComponentTypeInfo& getDefaultTypeInfo() const override { return getClassTypeInfo(); }

    //==============================================================================
    /** @internal */
    void parentHierarchyChanged() override;
    /** @internal */
    void resized() override;

protected:
    //==============================================================================
    /** @internal */
    void entityChanged() override;

private:
    //==============================================================================
    template <class Value>
    void setValue (Value& value, const Value& newValue, const Identifier& property);

    void updateCamera();
    void updatePlanesProblem();
    void updateActiveCamera();

    CameraNode settings; // The values the CameraNode of the entity is given, which it only has while the camera is active
    bool active = true;
    bool hasPlanesProblem = false; // An orthographic camera whose far plane is not beyond its near one, as it was last told

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SceneCameraComponent)
};

} // namespace yup
