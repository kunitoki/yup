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
/** A part that views the scene from the position and orientation of its entity.

    Cameras look down the -Z axis of their entity with +Y up, like glTF cameras. Choose which
    camera renders a scene with Scene::setActiveCamera().

    @see Scene, Entity
*/
class YUP_API CameraNode : public Node
{
public:
    /** The kind of projection. */
    enum class Projection
    {
        perspective, ///< Objects get smaller with distance.
        orthographic ///< Objects keep their size at any distance.
    };

    /** Creates a perspective camera. */
    CameraNode() = default;

    //==============================================================================
    /** Returns the projection matrix for a viewport.

        @param aspectRatio     The viewport width divided by its height.
        @param depthZeroToOne  True for a [0, 1] clip depth range, as every backend expects from shaders
                               compiled by YUP's shader transpiler, OpenGL included. False for [-1, 1],
                               only for shaders that reach OpenGL without the transpiler.
    */
    Matrix4 getProjectionMatrix (float aspectRatio, bool depthZeroToOne = true) const noexcept;

    /** Returns the view matrix: the inverse of the world matrix of the entity, or identity if unattached. */
    Matrix4 getViewMatrix() const noexcept;

    /** Returns the signed circle of confusion diameter, in pixels, of a point at a view distance.

        Uses the thin lens model with the focal length implied by yFov and sensorHeight. Negative in
        front of the focus distance, positive behind it, and 0 for orthographic cameras, for fStop <= 0,
        for yFov <= 0, for distances <= 0 and for a viewportHeight <= 0. Tends to a finite value as the
        distance grows. A focusDistance at or below the focal length is treated as just past it.

        @param distance        View-space distance along -Z, in world units (read as meters, like sensorHeight).
        @param viewportHeight  The height of the rendered image in pixels, 0 or less gives 0.
    */
    float getCircleOfConfusion (float distance, float viewportHeight) const noexcept;

    //==============================================================================
    Projection projection = Projection::perspective; ///< The kind of projection.
    float yFov = 0.8f;                               ///< Perspective vertical field of view in radians.
    float zNear = 0.1f;                              ///< Distance of the near clipping plane.
    float zFar = 1000.0f;                            ///< Distance of the far clipping plane, 0 for infinite (perspective only).
    float xMag = 1.0f;                               ///< Orthographic half width.
    float yMag = 1.0f;                               ///< Orthographic half height.

    /** The height of the virtual sensor in world units, read as meters: 24 mm, a full-frame sensor held
        in landscape. Together with yFov it defines the focal length the depth of field uses.
    */
    static constexpr float sensorHeight = 0.024f;

    float fStop = 2.8f;          ///< Aperture f-number for depth of field; smaller blurs more, 0 or less disables it.
    float focusDistance = 10.0f; ///< Distance along -Z, in world units read as meters, that stays perfectly sharp.
    int apertureBlades = 0;      ///< Diaphragm blades shaping the bokeh: below 3 is round, otherwise a regular polygon.
};

} // namespace yup
