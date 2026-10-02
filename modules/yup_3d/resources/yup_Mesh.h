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
/** Triangle geometry made of one or more primitives, each with its own material.

    Meshes are shared: the same mesh can be attached to any number of entities through a
    MeshNode. The vertex data on the CPU is the source of truth; GPU buffers are created the
    first time a device renders the mesh, and again when another device does.

    @code
    auto mesh = yup::Mesh::Ptr (new yup::Mesh ("triangle"));
    mesh->addPrimitive ({ { { 0.0f, 1.0f, 0.0f } }, { { -1.0f, -1.0f, 0.0f } }, { { 1.0f, -1.0f, 0.0f } } }, {});
    @endcode

    @see MeshNode, Material
*/
class YUP_API Mesh : public ReferenceCountedObject
{
public:
    //==============================================================================
    using Ptr = ReferenceCountedObjectPtr<Mesh>;

    /** A vertex, laid out as the renderer uploads it: 48 bytes, tightly packed. */
    struct Vertex
    {
        Vector3<float> position;                         ///< Position in model space.
        Vector3<float> normal;                           ///< Unit normal, or zero to have one generated.
        Point<float> uv;                                 ///< Texture coordinates, v pointing down.
        std::array<float, 4> color { 1.0f, 1.0f, 1.0f, 1.0f }; ///< Linear RGBA multiplied with the base color.
    };

    /** A set of triangles drawn with one material. */
    struct Primitive
    {
        std::vector<Vertex> vertices;                    ///< The vertices.
        std::vector<uint32> indices;                     ///< Three indices per triangle, counter-clockwise when front facing.
        Material::Ptr material;                          ///< The material, or nullptr for the default one.
        BoundingBox bounds;                              ///< The bounds of the vertices in model space.
    };

    //==============================================================================
    /** Creates an empty mesh.

        @param meshName The name of the mesh.
    */
    explicit Mesh (const String& meshName = {});

    /** Destructor. */
    ~Mesh() override;

    //==============================================================================
    /** Returns the name of the mesh. */
    const String& getName() const noexcept { return name; }

    //==============================================================================
    /** Adds a primitive.

        When @a indices is empty the vertices are drawn in order, three per triangle. When every
        normal is zero, flat normals are generated: each triangle gets its own three vertices,
        facing the side its counter-clockwise winding faces.

        @param vertices  The vertices, at least three.
        @param indices   The triangle indices, a multiple of three, or empty.
        @param material  The material, or nullptr for the default one.
    */
    void addPrimitive (std::vector<Vertex> vertices, std::vector<uint32> indices, Material::Ptr material = nullptr);

    /** Returns the number of primitives. */
    int getNumPrimitives() const noexcept { return static_cast<int> (primitives.size()); }

    /** Returns a primitive. */
    const Primitive& getPrimitive (int index) const noexcept;

    /** Returns every primitive. */
    Span<const Primitive> getPrimitives() const noexcept { return primitives; }

    /** Returns the union of the bounds of every primitive, in model space. */
    const BoundingBox& getBounds() const noexcept { return bounds; }

    //==============================================================================
    /** Returns the vertex buffer of a primitive for a device, uploading every primitive if needed.

        @param device          The device that renders the mesh.
        @param primitiveIndex  The primitive.
    */
    GpuBuffer::Ptr getVertexBuffer (const GpuDevice::Ptr& device, int primitiveIndex);

    /** Returns the 32-bit index buffer of a primitive for a device, uploading every primitive if needed.

        @param device          The device that renders the mesh.
        @param primitiveIndex  The primitive.
    */
    GpuBuffer::Ptr getIndexBuffer (const GpuDevice::Ptr& device, int primitiveIndex);

private:
    struct GpuBuffers
    {
        GpuBuffer::Ptr vertices;
        GpuBuffer::Ptr indices;
    };

    void prepareGpuBuffers (const GpuDevice::Ptr& device);

    String name;
    std::vector<Primitive> primitives;
    BoundingBox bounds;

    GpuDevice::Ptr cachedDevice;
    std::vector<GpuBuffers> gpuBuffers;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Mesh)
};

} // namespace yup
