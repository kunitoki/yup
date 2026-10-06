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

void writePoints (rive::RenderBuffer& buffer, const std::vector<Point<float>>& points)
{
    auto* destination = static_cast<float*> (buffer.map());
    if (destination != nullptr)
    {
        for (const auto& point : points)
        {
            *destination++ = point.getX();
            *destination++ = point.getY();
        }
    }

    buffer.unmap();
}

void writeIndices (rive::RenderBuffer& buffer, const std::vector<uint16>& indices)
{
    if (auto* destination = buffer.map())
        std::memcpy (destination, indices.data(), indices.size() * sizeof (uint16));

    buffer.unmap();
}

} // namespace

//==============================================================================
ImageMesh::ImageMesh (std::vector<Point<float>> vertices, std::vector<Point<float>> textureCoordinates, std::vector<uint16> indices)
    : vertices (std::move (vertices))
    , textureCoordinates (std::move (textureCoordinates))
    , indices (std::move (indices))
{
}

ImageMesh::ImageMesh (const ImageMesh& other)
    : vertices (other.vertices)
    , textureCoordinates (other.textureCoordinates)
    , indices (other.indices)
{
}

ImageMesh& ImageMesh::operator= (const ImageMesh& other)
{
    if (this == std::addressof (other))
        return *this;

    releaseGpuBuffers();
    vertices = other.vertices;
    textureCoordinates = other.textureCoordinates;
    indices = other.indices;
    return *this;
}

ImageMesh::ImageMesh (ImageMesh&& other) noexcept
    : vertices (std::move (other.vertices))
    , textureCoordinates (std::move (other.textureCoordinates))
    , indices (std::move (other.indices))
    , gpuBuffers (std::exchange (other.gpuBuffers, {}))
{
}

ImageMesh& ImageMesh::operator= (ImageMesh&& other) noexcept
{
    if (this == std::addressof (other))
        return *this;

    releaseGpuBuffers();
    vertices = std::move (other.vertices);
    textureCoordinates = std::move (other.textureCoordinates);
    indices = std::move (other.indices);
    gpuBuffers = std::exchange (other.gpuBuffers, {});
    return *this;
}

ImageMesh::~ImageMesh()
{
    releaseGpuBuffers();
}

//==============================================================================
ImageMesh ImageMesh::createGrid (Rectangle<float> area, int columns, int rows)
{
    // 256 x 256 vertices is as many as 16-bit indices can address
    columns = jlimit (1, 255, columns);
    rows = jlimit (1, 255, rows);

    std::vector<Point<float>> gridVertices;
    std::vector<Point<float>> gridTextureCoordinates;
    std::vector<uint16> gridIndices;

    gridVertices.reserve (static_cast<std::size_t> ((columns + 1) * (rows + 1)));
    gridTextureCoordinates.reserve (gridVertices.capacity());
    gridIndices.reserve (static_cast<std::size_t> (columns * rows * 6));

    for (int row = 0; row <= rows; ++row)
    {
        for (int column = 0; column <= columns; ++column)
        {
            const Point<float> uv (static_cast<float> (column) / static_cast<float> (columns),
                                   static_cast<float> (row) / static_cast<float> (rows));

            gridTextureCoordinates.push_back (uv);
            gridVertices.push_back ({ area.getX() + uv.getX() * area.getWidth(), area.getY() + uv.getY() * area.getHeight() });
        }
    }

    for (int row = 0; row < rows; ++row)
    {
        for (int column = 0; column < columns; ++column)
        {
            const auto topLeft = static_cast<uint16> (row * (columns + 1) + column);
            const auto bottomLeft = static_cast<uint16> (topLeft + columns + 1);

            for (auto index : { topLeft, static_cast<uint16> (topLeft + 1), bottomLeft,
                                static_cast<uint16> (topLeft + 1), static_cast<uint16> (bottomLeft + 1), bottomLeft })
                gridIndices.push_back (index);
        }
    }

    return { std::move (gridVertices), std::move (gridTextureCoordinates), std::move (gridIndices) };
}

//==============================================================================
Span<const Point<float>> ImageMesh::getVertices() const noexcept
{
    return { vertices.data(), vertices.size() };
}

Span<const Point<float>> ImageMesh::getTextureCoordinates() const noexcept
{
    return { textureCoordinates.data(), textureCoordinates.size() };
}

Span<const uint16> ImageMesh::getIndices() const noexcept
{
    return { indices.data(), indices.size() };
}

//==============================================================================
void ImageMesh::setVertex (int index, Point<float> position)
{
    if (! isPositiveAndBelow (index, static_cast<int> (vertices.size())))
        return;

    vertices[static_cast<std::size_t> (index)] = position;
    gpuBuffers.verticesChanged = true;
}

bool ImageMesh::setVertices (Span<const Point<float>> newVertices)
{
    if (newVertices.size() != vertices.size())
        return false;

    if (newVertices.data() == vertices.data())
        return true;

    vertices.assign (newVertices.begin(), newVertices.end());
    gpuBuffers.verticesChanged = true;
    return true;
}

//==============================================================================
bool ImageMesh::isValid() const noexcept
{
    constexpr std::size_t maxVertices = 65536;

    if (indices.empty() || indices.size() % 3 != 0)
        return false;

    if (vertices.empty() || vertices.size() > maxVertices || textureCoordinates.size() != vertices.size())
        return false;

    return std::all_of (indices.begin(), indices.end(), [this] (uint16 index)
    {
        return index < vertices.size();
    });
}

//==============================================================================
const ImageMesh::GpuBuffers* ImageMesh::updateGpuBuffers (rive::Factory& factory, GpuDevice::Ptr device) const
{
    if (! isValid())
        return nullptr;

    if (gpuBuffers.factory != std::addressof (factory))
    {
        const auto pointsSize = vertices.size() * 2 * sizeof (float);

        releaseGpuBuffers();
        gpuBuffers.device = std::move (device);
        gpuBuffers.factory = std::addressof (factory);
        gpuBuffers.vertices = factory.makeRenderBuffer (rive::RenderBufferType::vertex, rive::RenderBufferFlags::none, pointsSize);
        gpuBuffers.textureCoordinates = factory.makeRenderBuffer (rive::RenderBufferType::vertex, rive::RenderBufferFlags::mappedOnceAtInitialization, pointsSize);
        gpuBuffers.indices = factory.makeRenderBuffer (rive::RenderBufferType::index, rive::RenderBufferFlags::mappedOnceAtInitialization, indices.size() * sizeof (uint16));

        if (gpuBuffers.vertices == nullptr || gpuBuffers.textureCoordinates == nullptr || gpuBuffers.indices == nullptr)
        {
            releaseGpuBuffers();
            return nullptr;
        }

        writePoints (*gpuBuffers.textureCoordinates, textureCoordinates);
        writeIndices (*gpuBuffers.indices, indices);
    }

    if (std::exchange (gpuBuffers.verticesChanged, false))
        writePoints (*gpuBuffers.vertices, vertices);

    return std::addressof (gpuBuffers);
}

void ImageMesh::releaseGpuBuffers() const
{
    auto released = std::exchange (gpuBuffers, {});

    // On OpenGL the buffers must be deleted with the context current, which the message
    // thread (where meshes are often dropped) does not have
    if (released.device != nullptr)
    {
        released.device->runOnGraphicsContext ([&released]
        {
            released.vertices = nullptr;
            released.textureCoordinates = nullptr;
            released.indices = nullptr;
        });
    }
}

} // namespace yup
