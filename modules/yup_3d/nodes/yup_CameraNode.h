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

    @see Scene, EntityNode
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
        @param depthZeroToOne  True for a [0, 1] depth range, false for [-1, 1].
    */
    Matrix4 getProjectionMatrix (float aspectRatio, bool depthZeroToOne = true) const noexcept;

    /** Returns the view matrix: the inverse of the world matrix of the entity, or identity if unattached. */
    Matrix4 getViewMatrix() const noexcept;

    //==============================================================================
    Projection projection = Projection::perspective; ///< The kind of projection.
    float yFov = 0.8f;                               ///< Perspective vertical field of view in radians.
    float zNear = 0.1f;                              ///< Distance of the near clipping plane.
    float zFar = 1000.0f;                            ///< Distance of the far clipping plane, 0 for infinite (perspective only).
    float xMag = 1.0f;                               ///< Orthographic half width.
    float yMag = 1.0f;                               ///< Orthographic half height.
};

} // namespace yup
