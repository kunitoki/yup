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

static_assert (sizeof (Mesh::Vertex) == 48, "Mesh::Vertex must stay tightly packed, the renderer uploads it as is");
static_assert (std::is_standard_layout_v<Mesh::Vertex>);

//==============================================================================

Mesh::Mesh (const String& meshName)
    : name (meshName)
{
}

Mesh::~Mesh() = default;

//==============================================================================

void Mesh::addPrimitive (std::vector<Vertex> vertices, std::vector<uint32> indices, Material::Ptr material)
{
    if (indices.empty())
    {
        indices.resize (vertices.size() - vertices.size() % 3);
        std::iota (indices.begin(), indices.end(), 0u);
    }

    const auto numVertices = vertices.size();
    const auto indicesValid = std::all_of (indices.begin(), indices.end(), [numVertices] (uint32 index)
    {
        return index < numVertices;
    });

    if (vertices.size() < 3 || indices.size() < 3 || indices.size() % 3 != 0 || ! indicesValid)
    {
        jassertfalse; // Primitives need at least one triangle, three indices per triangle, all in range
        return;
    }

    const auto hasNormals = std::any_of (vertices.begin(), vertices.end(), [] (const Vertex& vertex)
    {
        return vertex.normal.lengthSquared() > 0.0f;
    });

    if (! hasNormals)
    {
        // Flat shading needs its own vertices per triangle
        std::vector<Vertex> unshared;
        unshared.reserve (indices.size());

        for (size_t i = 0; i < indices.size(); i += 3)
        {
            auto a = vertices[indices[i]];
            auto b = vertices[indices[i + 1]];
            auto c = vertices[indices[i + 2]];

            const auto normal = (b.position - a.position).crossProduct (c.position - a.position).normalized();
            a.normal = b.normal = c.normal = normal;

            unshared.insert (unshared.end(), { a, b, c });
        }

        vertices = std::move (unshared);
        indices.resize (vertices.size());
        std::iota (indices.begin(), indices.end(), 0u);
    }

    Primitive primitive;

    for (const auto& vertex : vertices)
        primitive.bounds.expand (vertex.position);

    primitive.vertices = std::move (vertices);
    primitive.indices = std::move (indices);
    primitive.material = std::move (material);

    bounds.expand (primitive.bounds);
    primitives.push_back (std::move (primitive));

    cachedDevice = nullptr;
    gpuBuffers.clear();
}

const Mesh::Primitive& Mesh::getPrimitive (int index) const noexcept
{
    jassert (isPositiveAndBelow (index, getNumPrimitives()));
    return primitives[static_cast<size_t> (index)];
}

//==============================================================================

GpuBuffer::Ptr Mesh::getVertexBuffer (const GpuDevice::Ptr& device, int primitiveIndex)
{
    prepareGpuBuffers (device);

    return isPositiveAndBelow (primitiveIndex, static_cast<int> (gpuBuffers.size()))
             ? gpuBuffers[static_cast<size_t> (primitiveIndex)].vertices
             : nullptr;
}

GpuBuffer::Ptr Mesh::getIndexBuffer (const GpuDevice::Ptr& device, int primitiveIndex)
{
    prepareGpuBuffers (device);

    return isPositiveAndBelow (primitiveIndex, static_cast<int> (gpuBuffers.size()))
             ? gpuBuffers[static_cast<size_t> (primitiveIndex)].indices
             : nullptr;
}

void Mesh::prepareGpuBuffers (const GpuDevice::Ptr& device)
{
    if (device == cachedDevice && gpuBuffers.size() == primitives.size())
        return;

    cachedDevice = device;
    gpuBuffers.clear();

    if (device == nullptr)
        return;

    for (const auto& primitive : primitives)
    {
        gpuBuffers.push_back ({ GpuBuffer::create (device, GpuBufferType::vertex, primitive.vertices.data(), primitive.vertices.size() * sizeof (Vertex)),
                                GpuBuffer::create (device, GpuBufferType::index, primitive.indices.data(), primitive.indices.size() * sizeof (uint32)) });
    }
}

} // namespace yup
