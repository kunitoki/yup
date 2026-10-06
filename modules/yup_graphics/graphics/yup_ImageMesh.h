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
/** A triangle mesh that maps an image onto arbitrary geometry.

    Each vertex has a position, in the coordinates of the Graphics drawing it, and a
    texture coordinate, from (0, 0) at the image's top-left corner to (1, 1) at its
    bottom-right. Triangles are listed as triples of vertex indices.

    Draw it with Graphics::drawImageMesh(), or many copies at once with
    Graphics::drawImageMeshInstanced(). The GPU buffers are created the first time the
    mesh is drawn and kept; moving vertices with setVertex() or setVertices() only
    uploads the positions again, so a mesh can be warped every frame cheaply.

    The GPU reads the positions when the frame is rendered, so a mesh shows one set of
    positions per frame: to draw differently warped copies in the same frame, use one
    mesh for each. Meshes that fold over themselves may show artifacts on GPUs without
    raster ordering.

    @see Graphics::drawImageMesh, ImageMeshInstance
*/
class YUP_API ImageMesh
{
public:
    //==============================================================================
    /** Creates an empty mesh, which draws nothing. */
    ImageMesh() = default;

    /** Creates a mesh from its vertices and triangles.

        @param vertices           The vertex positions.
        @param textureCoordinates The texture coordinate of each vertex, in 0 to 1 image units.
        @param indices            The vertex indices of the triangles, three per triangle.
    */
    ImageMesh (std::vector<Point<float>> vertices, std::vector<Point<float>> textureCoordinates, std::vector<uint16> indices);

    /** Copies the mesh data. The copy creates GPU buffers of its own when drawn. */
    ImageMesh (const ImageMesh& other);

    /** Copies the mesh data. The copy creates GPU buffers of its own when drawn. */
    ImageMesh& operator= (const ImageMesh& other);

    /** Move constructor and assignment operator. */
    ImageMesh (ImageMesh&& other) noexcept;
    ImageMesh& operator= (ImageMesh&& other) noexcept;

    /** Destructor. The GPU buffers are released with the graphics context current. */
    ~ImageMesh();

    //==============================================================================
    /** Creates a grid of columns x rows cells covering an area, with the whole image across it.

        Vertices are ordered row by row from the top-left corner, (columns + 1) per row.
        Columns and rows are kept between 1 and 255, so the grid stays within the
        65536 vertices a mesh can have.

        @param area    The area the grid covers.
        @param columns The number of cells across.
        @param rows    The number of cells down.
    */
    static ImageMesh createGrid (Rectangle<float> area, int columns, int rows);

    //==============================================================================
    /** Returns the vertex positions. */
    Span<const Point<float>> getVertices() const noexcept;

    /** Returns the texture coordinates, one per vertex. */
    Span<const Point<float>> getTextureCoordinates() const noexcept;

    /** Returns the vertex indices of the triangles, three per triangle. */
    Span<const uint16> getIndices() const noexcept;

    //==============================================================================
    /** Moves one vertex. Out of range indices are ignored. */
    void setVertex (int index, Point<float> position);

    /** Replaces every vertex position.

        @param newVertices The new positions, as many as the mesh has vertices.
        @return True if the count matched and the positions were replaced.
    */
    bool setVertices (Span<const Point<float>> newVertices);

    //==============================================================================
    /** Returns true if the mesh can be drawn.

        That needs at least one triangle, a texture coordinate per vertex, at most 65536
        vertices and every index referring to an existing vertex.
    */
    bool isValid() const noexcept;

private:
    friend class Graphics;

    struct GpuBuffers
    {
        GpuDevice::Ptr device;
        rive::Factory* factory = nullptr;
        rive::rcp<rive::RenderBuffer> vertices;
        rive::rcp<rive::RenderBuffer> textureCoordinates;
        rive::rcp<rive::RenderBuffer> indices;
        bool verticesChanged = true;
    };

    const GpuBuffers* updateGpuBuffers (rive::Factory& factory, GpuDevice::Ptr device) const;
    void releaseGpuBuffers() const;

    std::vector<Point<float>> vertices;
    std::vector<Point<float>> textureCoordinates;
    std::vector<uint16> indices;
    mutable GpuBuffers gpuBuffers;
};

//==============================================================================
/** One copy of a mesh drawn by Graphics::drawImageMeshInstanced().

    @see ImageMesh
*/
struct YUP_API ImageMeshInstance
{
    /** Places this copy, applied before the Graphics' own transform. */
    AffineTransform transform;

    /** Added to the scaled texture coordinates, e.g. to pick a cell of a sprite sheet. */
    Point<float> textureOffset;

    /** Multiplies the texture coordinates before the offset is added. */
    Point<float> textureScale { 1.0f, 1.0f };

    /** The opacity of this copy, multiplied with the Graphics' opacity. */
    float opacity = 1.0f;

    /** How much this copy adds to what is below, from 0 (normal) to 1 (fully additive). */
    float additiveAmount = 0.0f;
};

} // namespace yup
