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

static_assert (sizeof (Mesh::Vertex) == 64, "Mesh::Vertex must stay tightly packed, the renderer uploads it as is");
static_assert (std::is_standard_layout_v<Mesh::Vertex>);

//==============================================================================

namespace
{

/** Gives every index its own copy of what its vertex comes with, if there is any. */
template <class Type>
void unshareVertexData (std::vector<Type>& perVertex, const std::vector<uint32>& indices)
{
    if (perVertex.empty())
        return;

    std::vector<Type> unshared;
    unshared.reserve (indices.size());

    for (const auto index : indices)
        unshared.push_back (perVertex[index]);

    perVertex = std::move (unshared);
}

} // namespace

//==============================================================================

Mesh::Mesh (const String& meshName)
    : name (meshName)
{
}

Mesh::~Mesh() = default;

//==============================================================================

void Mesh::addPrimitive (std::vector<Vertex> vertices, std::vector<uint32> indices, Material::Ptr material, std::vector<Influences> influences, std::vector<MorphTarget> morphTargets)
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

    // What comes with the vertices holds one element for each of them, or none
    const auto fitsVertices = [numVertices] (const auto& perVertex)
    {
        return perVertex.empty() || perVertex.size() == numVertices;
    };

    const auto targetsValid = std::all_of (morphTargets.begin(), morphTargets.end(), [&fitsVertices] (const MorphTarget& target)
    {
        return fitsVertices (target.positions) && fitsVertices (target.normals) && fitsVertices (target.tangents);
    });

    if (vertices.size() < 3 || indices.size() < 3 || indices.size() % 3 != 0 || ! indicesValid || ! fitsVertices (influences) || ! targetsValid)
    {
        jassertfalse; // Primitives need at least one triangle, three indices per triangle, all in range, and influences and morph deltas for every vertex
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

            // Tangents authored for another frame can't be trusted with the generated one
            a.tangent = b.tangent = c.tangent = {};

            unshared.insert (unshared.end(), { a, b, c });
        }

        vertices = std::move (unshared);

        // What comes with the vertices follows them
        unshareVertexData (influences, indices);

        for (auto& target : morphTargets)
        {
            unshareVertexData (target.positions, indices);

            // glTF: a primitive without normals is flat in every morph target too. As for the vertices,
            // deltas authored for other normals and tangents can't be trusted with the generated ones
            target.normals.clear();
            target.tangents.clear();

            if (! target.positions.empty())
                target.normals.resize (vertices.size());

            for (size_t i = 0; i < target.normals.size(); i += 3)
            {
                const auto a = vertices[i].position + target.positions[i];
                const auto b = vertices[i + 1].position + target.positions[i + 1];
                const auto c = vertices[i + 2].position + target.positions[i + 2];

                const auto delta = (b - a).crossProduct (c - a).normalized() - vertices[i].normal;
                target.normals[i] = target.normals[i + 1] = target.normals[i + 2] = delta;
            }
        }

        indices.resize (vertices.size());
        std::iota (indices.begin(), indices.end(), 0u);
    }

    for (const auto& vertex : influences)
    {
        for (size_t i = 0; i < vertex.joints.size(); ++i)
        {
            if (vertex.weights[i] != 0.0f)
                numJoints = jmax (numJoints, vertex.joints[i] + 1);
        }
    }

    numMorphTargets = jmax (numMorphTargets, static_cast<int> (morphTargets.size()));

    Primitive primitive;
    primitive.vertices = std::move (vertices);
    primitive.indices = std::move (indices);
    primitive.material = std::move (material);
    primitive.influences = std::move (influences);
    primitive.morphTargets = std::move (morphTargets);

    primitives.push_back (std::move (primitive));
    updateBounds (primitives.back());

    cachedDevice = nullptr;
    gpuBuffers.clear();
}

const Mesh::Primitive& Mesh::getPrimitive (int index) const noexcept
{
    jassert (isPositiveAndBelow (index, getNumPrimitives()));
    return primitives[static_cast<size_t> (index)];
}

void Mesh::setVertices (int primitiveIndex, Span<const Vertex> vertices)
{
    const auto index = static_cast<size_t> (primitiveIndex);

    if (! isPositiveAndBelow (primitiveIndex, getNumPrimitives()) || vertices.size() != primitives[index].vertices.size())
    {
        jassertfalse; // The primitive must exist and keep its number of vertices, which its indices refer to
        return;
    }

    auto& primitive = primitives[index];
    std::copy (vertices.begin(), vertices.end(), primitive.vertices.begin());
    updateBounds (primitive);

    // Uploaded when the buffer is next asked for: before the first use there is none yet
    if (index < gpuBuffers.size())
        gpuBuffers[index].verticesStale = true;

    ++changeCount;
}

void Mesh::updateBounds (Primitive& changed)
{
    changed.bounds = {};

    for (const auto& vertex : changed.vertices)
        changed.bounds.expand (vertex.position);

    // The union of every primitive again: new vertices can shrink the one they replace
    bounds = {};

    for (const auto& primitive : primitives)
        bounds.expand (primitive.bounds);
}

std::optional<float> Mesh::intersectTriangles (const Ray& ray, float minDistance, float maxDistance, const Material* overrideMaterial) const noexcept
{
    const auto length = ray.getDirection().length();
    std::optional<float> nearest;

    for (const auto& primitive : primitives)
    {
        const auto size = primitive.bounds.getMax() - primitive.bounds.getMin();
        const auto reach = jmax (size.getX(), size.getY(), size.getZ());

        if (! (reach > 0.0f && length > 0.0f) || ! ray.intersectBox (primitive.bounds, minDistance, nearest.value_or (maxDistance)).has_value())
            continue;

        const auto* material = overrideMaterial != nullptr ? overrideMaterial : primitive.material.get();
        const auto cullBackFaces = material == nullptr || ! material->doubleSided;

        // Ray::intersectTriangle() takes a triangle as parallel to the ray under an absolute threshold, which the
        // small triangles of a detailed part would all be under whatever the ray: the ray is given the length
        // that takes them away from it, for the size of the primitive and the number of its triangles. A
        // distance along it is that many times shorter
        const auto scale = static_cast<float> (primitive.indices.size() / 3) / (reach * reach * length);
        const Ray scaled (ray.getOrigin(), ray.getDirection() * scale);

        // shortcut: every triangle of a primitive whose bounds the ray enters is tested, which gets slow with
        // hundreds of thousands of them. The upgrade is a bounding volume hierarchy for each primitive, built
        // when it is first asked for and again when setVertices() changes it
        for (size_t i = 0; i + 2 < primitive.indices.size(); i += 3)
        {
            const auto hit = scaled.intersectTriangle (primitive.vertices[primitive.indices[i]].position,
                                                       primitive.vertices[primitive.indices[i + 1]].position,
                                                       primitive.vertices[primitive.indices[i + 2]].position,
                                                       cullBackFaces);
            if (! hit.has_value())
                continue;

            if (const auto distance = hit->distance * scale; distance >= minDistance && distance <= nearest.value_or (maxDistance))
                nearest = distance;
        }
    }

    return nearest;
}

//==============================================================================

GpuBuffer::Ptr Mesh::getVertexBuffer (const GpuDevice::Ptr& device, int primitiveIndex)
{
    prepareGpuBuffers (device);

    if (! isPositiveAndBelow (primitiveIndex, static_cast<int> (gpuBuffers.size())))
        return nullptr;

    auto& buffers = gpuBuffers[static_cast<size_t> (primitiveIndex)];

    if (std::exchange (buffers.verticesStale, false))
    {
        // Rewritten in place, the size being the same. Frames in flight still draw the vertices
        // they bound: a buffer bound since its last update is written through another backing
        // (OpenGL orders the write after the draws instead)
        const auto& vertices = primitives[static_cast<size_t> (primitiveIndex)].vertices;
        device->updateBuffer (buffers.vertices, vertices.data(), vertices.size() * sizeof (Vertex));
    }

    return buffers.vertices;
}

GpuBuffer::Ptr Mesh::getIndexBuffer (const GpuDevice::Ptr& device, int primitiveIndex)
{
    prepareGpuBuffers (device);

    return isPositiveAndBelow (primitiveIndex, static_cast<int> (gpuBuffers.size()))
             ? gpuBuffers[static_cast<size_t> (primitiveIndex)].indices
             : nullptr;
}

GpuBuffer::Ptr Mesh::getEdgeIndexBuffer (const GpuDevice::Ptr& device, int primitiveIndex)
{
    prepareGpuBuffers (device);

    if (! isPositiveAndBelow (primitiveIndex, static_cast<int> (gpuBuffers.size())))
        return nullptr;

    auto& buffers = gpuBuffers[static_cast<size_t> (primitiveIndex)];
    if (buffers.edges == nullptr)
    {
        const auto edges = computeEdgeIndices (primitives[static_cast<size_t> (primitiveIndex)].indices);
        buffers.edges = GpuBuffer::create (device, GpuBufferType::index, edges.data(), edges.size() * sizeof (uint32));
    }

    return buffers.edges;
}

std::vector<uint32> Mesh::computeEdgeIndices (Span<const uint32> triangleIndices)
{
    const auto numTriangles = triangleIndices.size() / 3;

    std::vector<uint32> edges;
    edges.reserve (numTriangles * 6);

    std::unordered_set<uint64> seen;
    seen.reserve (numTriangles * 3);

    for (size_t i = 0; i < numTriangles * 3; i += 3)
    {
        for (size_t corner = 0; corner < 3; ++corner)
        {
            const auto a = triangleIndices[i + corner];
            const auto b = triangleIndices[i + (corner + 1) % 3];
            const auto key = (static_cast<uint64> (jmin (a, b)) << 32) | jmax (a, b);

            if (a != b && seen.insert (key).second)
                edges.insert (edges.end(), { a, b });
        }
    }

    return edges;
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
                                GpuBuffer::create (device, GpuBufferType::index, primitive.indices.data(), primitive.indices.size() * sizeof (uint32)),
                                nullptr });
    }
}

} // namespace yup
