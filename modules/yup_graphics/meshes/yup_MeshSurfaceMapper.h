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
/** Maps between points of a viewport and texture coordinates of a 3D triangle mesh.

    This is the picking half of presenting a 2D surface, typically a Component rendered with
    Component::renderToTexture(), on arbitrary 3D geometry: flat quads, cylinders, spheres or
    cube faces. Give it the same vertex positions, texture coordinates and indices uploaded to
    the GPU, and the same model-view-projection matrix used to draw them, and it tells which
    texture coordinate is displayed under a point of the viewport, and the other way around.

    Texture coordinates must follow the convention the fragment shader uses to sample the
    texture: if the shader flips v, the coordinates given here must be flipped as well.

    This class is not thread safe: when it's updated from paint() and queried from input
    handlers on another thread, guard it with a lock.

    @see Ray, Matrix4, Component::getChildPointFromLocal
*/
class YUP_API MeshSurfaceMapper
{
public:
    //==============================================================================
    /** A point of the mesh found under a viewport point. */
    struct Hit
    {
        /** The texture coordinate at the hit point. */
        Point<float> uv;

        /** The normalized device depth of the hit point, lower is closer to the camera.

            Compare it between surfaces drawn with the same projection to resolve occlusion:
            the hit with the lowest depth wins.
        */
        float depth = 0.0f;

        /** The index of the triangle that was hit. */
        int triangleIndex = -1;
    };

    //==============================================================================
    /** Constructs an empty mapper. */
    MeshSurfaceMapper() = default;

    //==============================================================================
    /** Sets the triangle mesh, which is copied.

        @param positions  The vertex positions, in model space.
        @param uvs        The texture coordinates, one per vertex.
        @param indices    Three vertex indices per triangle.
    */
    void setMesh (Span<const Vector3<float>> positions, Span<const Point<float>> uvs, Span<const uint32> indices);

    /** Sets the matrix and the viewport used to display the mesh.

        @param modelViewProjection  The matrix mapping model space to clip space.
        @param viewport             The area the mesh is displayed in, with y pointing down.
    */
    void setModelViewProjection (const Matrix4& modelViewProjection, Rectangle<float> viewport);

    /** Enables or disables ignoring triangles that face away from the camera.

        This should match the cull mode of the pipeline drawing the mesh. It is enabled by default.
    */
    void setBackFaceCulling (bool shouldCullBackFaces);

    /** Returns true if back facing triangles are ignored. */
    bool isBackFaceCulling() const;

    //==============================================================================
    /** Finds the nearest point of the mesh displayed under a viewport point.

        @param viewportPoint The point, in viewport coordinates.

        @return The nearest hit, or std::nullopt if the point doesn't lie over the mesh.
    */
    std::optional<Hit> hitTest (Point<float> viewportPoint) const;

    /** Returns the texture coordinate displayed under a viewport point.

        Over the mesh this is the coordinate of the nearest hit. Outside of it, the point is
        extrapolated on the plane of the front facing triangle whose projected center is the
        closest, which gives coordinates outside the range of the mesh: that's what keeps a
        captured drag tracking when the pointer leaves the surface.

        @param viewportPoint The point, in viewport coordinates.

        @return The texture coordinate, or std::nullopt if there is no front facing triangle
                in front of the camera or the picking ray is parallel to its plane.
    */
    std::optional<Point<float>> viewportToUV (Point<float> viewportPoint) const;

    /** Returns the viewport point where a texture coordinate is displayed.

        @param uv The texture coordinate.

        @return The viewport point, or std::nullopt if no triangle covers the coordinate or it
                lies behind the camera.
    */
    std::optional<Point<float>> uvToViewport (Point<float> uv) const;

private:
    struct Triangle
    {
        Vector3<float> a, b, c;
        Point<float> uvA, uvB, uvC;
    };

    Ray getPickingRay (Point<float> viewportPoint) const;
    std::optional<Point<float>> clipToViewport (const Vector3<float>& modelPoint) const;

    std::vector<Triangle> triangles;
    Matrix4 modelViewProjection;
    Matrix4 inverseModelViewProjection;
    Rectangle<float> viewport;
    bool cullBackFaces = true;

    YUP_LEAK_DETECTOR (MeshSurfaceMapper)
};

} // namespace yup
