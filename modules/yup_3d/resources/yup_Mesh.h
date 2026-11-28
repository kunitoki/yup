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
    first time a device renders the mesh, and again when another device does. The vertices of a
    primitive can be replaced with setVertices(), which its vertex buffer then follows.

    @code
    auto mesh = yup::Mesh::Ptr (new yup::Mesh ("triangle"));
    mesh->addPrimitive ({ { { 0.0f, 1.0f, 0.0f } }, { { -1.0f, -1.0f, 0.0f } }, { { 1.0f, -1.0f, 0.0f } } }, {});
    @endcode

    A primitive can also hold what deforms it: the joints of a Skin that move each of its
    vertices (Influences) and morph targets (MorphTarget). The mesh only keeps them, and stays
    in the pose it was built in, its bind pose: a DeformNode applies them to a copy of the mesh,
    one for each entity drawing it deformed.

    @see MeshNode, Material, DeformNode, Skin
*/
class YUP_API Mesh : public ReferenceCountedObject
{
public:
    //==============================================================================
    using Ptr = ReferenceCountedObjectPtr<Mesh>;

    /** A vertex, laid out as the renderer uploads it: 64 bytes, tightly packed. */
    struct Vertex
    {
        Vector3<float> position;                         ///< Position in model space.
        Vector3<float> normal;                           ///< Unit normal, or zero to have one generated.
        Point<float> uv;                                 ///< Texture coordinates, v pointing down.
        std::array<float, 4> color { 1.0f, 1.0f, 1.0f, 1.0f }; ///< Linear RGBA multiplied with the base color.

        /** The tangent frame for normal maps, as in glTF: xyz the unit tangent along increasing u, and
            w the handedness (+1 or -1), with the bitangent B = cross (N, T) * w. It is used only when w
            is not zero; all zero, the default, derives the frame from the texture coordinates.
        */
        std::array<float, 4> tangent { 0.0f, 0.0f, 0.0f, 0.0f };
    };

    /** The joints that move a vertex of a skinned primitive, and how much each one does.

        Up to four joints of a Skin share a vertex, as in glTF: a DeformNode puts the vertex where
        each of them takes it, blended by the weights. It is kept apart from Vertex, which holds
        what the renderer draws.
    */
    struct Influences
    {
        std::array<uint16, 4> joints {};                 ///< Indices among the joints of the Skin. A joint whose weight is 0 is not used.

        /** The share of each joint. addPrimitive() makes the weights of a vertex add up to 1, one that
            is negative counting for nothing. All zero, the vertex stays in its bind pose.
        */
        std::array<float, 4> weights {};
    };

    /** A morph target of a primitive: how far its vertices move when the weight of the target is 1.

        Each list holds one delta per vertex, or is empty when the target leaves that attribute
        alone. A DeformNode adds the deltas of every target to the vertices, scaled by the weight
        of the target.
    */
    struct MorphTarget
    {
        std::vector<Vector3<float>> positions;           ///< Added to the positions.
        std::vector<Vector3<float>> normals;             ///< Added to the normals, which are made unit vectors again afterwards.
        std::vector<Vector3<float>> tangents;            ///< Added to the directions of the tangents, whose handedness is kept.
    };

    /** A set of triangles drawn with one material. */
    struct Primitive
    {
        std::vector<Vertex> vertices;                    ///< The vertices.
        std::vector<uint32> indices;                     ///< Three indices per triangle, counter-clockwise when front facing.
        Material::Ptr material;                          ///< The material, or nullptr for the default one.
        BoundingBox bounds;                              ///< The bounds of the vertices in model space.
        std::vector<Influences> influences;              ///< The joints moving each vertex of a skinned primitive, else empty.
        std::vector<MorphTarget> morphTargets;           ///< The morph targets, each one moved by the weight at its index.
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
        facing the side its counter-clockwise winding faces, and their tangents are cleared.
        Influences and morph targets follow their vertices there. A morph target that moves
        positions then gets the normal deltas that keep it flat too, as glTF asks, in place of
        the ones it came with, and its tangent deltas go with the tangents.

        The weights of the influences of a vertex are made to add up to 1, see Influences.

        A primitive that is not made of triangles of its vertices, or whose influences or morph
        deltas are neither empty nor one per vertex, is not added.

        @param vertices      The vertices, at least three.
        @param indices       The triangle indices, a multiple of three, or empty.
        @param material      The material, or nullptr for the default one.
        @param influences    The joints moving each vertex, one per vertex, or empty for a
                             primitive that is not skinned.
        @param morphTargets  The morph targets, or empty for a primitive that has none.

        @return A failure telling what is wrong with the primitive, which is not added then.
    */
    Result addPrimitive (std::vector<Vertex> vertices, std::vector<uint32> indices, Material::Ptr material = nullptr, std::vector<Influences> influences = {}, std::vector<MorphTarget> morphTargets = {});

    /** Returns the number of primitives. */
    int getNumPrimitives() const noexcept { return static_cast<int> (primitives.size()); }

    /** Returns a primitive. */
    const Primitive& getPrimitive (int index) const noexcept;

    /** Returns every primitive. */
    Span<const Primitive> getPrimitives() const noexcept { return primitives; }

    /** Returns the union of the bounds of every primitive, in model space. */
    const BoundingBox& getBounds() const noexcept { return bounds; }

    /** Returns where a ray first hits a triangle of the mesh, in model space.

        The primitives whose bounds the ray enters are tested triangle by triangle, so the cost
        grows with their number of triangles. A triangle seen from behind, its vertices appearing
        clockwise from the origin of the ray, is hit only when the material it is drawn with is
        double sided: that is when a renderer draws it.

        @param ray               The ray, in the space of the mesh.
        @param minDistance       The smallest distance to consider.
        @param maxDistance       The largest distance to consider.
        @param overrideMaterial  The material every primitive is drawn with in place of its own,
                                 as a MaterialNode gives one, or nullptr.

        @return The distance of the nearest hit along the ray, or nothing when the ray hits no
                triangle between the two distances.

        @see MeshNode::intersectTriangles
    */
    std::optional<float> intersectTriangles (const Ray& ray,
                                             float minDistance = 0.0f,
                                             float maxDistance = std::numeric_limits<float>::max(),
                                             const Material* overrideMaterial = nullptr) const noexcept;

    //==============================================================================
    /** Returns the number of joints the mesh refers to: one more than the highest joint an
        influence of its primitives uses with a weight, or 0 when no primitive is skinned.

        A Skin needs at least as many joints to move the mesh.
    */
    int getNumJoints() const noexcept { return numJoints; }

    /** Returns the number of morph targets of the mesh: the most a primitive has. */
    int getNumMorphTargets() const noexcept { return numMorphTargets; }

    /** Sets the weights the morph targets start with wherever the mesh is drawn deformed.

        A DeformNode takes them when it is made, so they don't reach the ones that already exist.

        @param newWeights  The weight of each morph target, in their order. Targets beyond them
                           start at 0, weights beyond the targets are not used.
    */
    void setMorphWeights (std::vector<float> newWeights) { morphWeights = std::move (newWeights); }

    /** Returns the weights the morph targets start with, as they were set: none by default,
        every target then starting at 0.
    */
    Span<const float> getMorphWeights() const noexcept { return morphWeights; }

    /** Sets the names of the morph targets, which findMorphTarget() looks for.

        glTF has no place for them: GltfModel takes the ones exporters write to the targetNames
        of the extras of a mesh.

        @param newNames  The name of each morph target, in their order. Targets beyond them have
                         no name, names beyond the targets are not used.
    */
    void setMorphTargetNames (StringArray newNames) { morphTargetNames = std::move (newNames); }

    /** Returns the names of the morph targets, as they were set: none by default. */
    const StringArray& getMorphTargetNames() const noexcept { return morphTargetNames; }

    /** Returns the index of the first morph target with a name, or -1 if there is none.

        It is the index of the weight of the target, in setMorphWeights() and in
        DeformNode::setWeights().
    */
    int findMorphTarget (StringRef targetName) const noexcept;

    //==============================================================================
    /** Replaces the vertices of a primitive, keeping its indices and material: for meshes
        deformed on the CPU, by skinning or morph targets for example.

        The bounds of the primitive and of the mesh follow the new vertices, getChangeCount()
        counts the change, and the vertex buffer of the primitive takes them the next time a
        device asks for it. The vertices are taken as they are: no normal is generated.

        The number of vertices cannot change, since the indices refer to them: another number,
        or a primitive that does not exist, changes nothing.

        Every entity drawing the mesh shows the new vertices, so give each deformed instance a
        mesh of its own.

        @param primitiveIndex  The primitive.
        @param vertices        The new vertices, as many as the primitive has, copied into it.

        @return A failure telling why the vertices were not taken.
    */
    Result setVertices (int primitiveIndex, Span<const Vertex> vertices);

    /** Returns how many times setVertices() changed the vertices of a primitive.

        Renderers compare this count to notice the new vertices: a change draws the shadow maps
        the mesh casts into again and restarts the temporal accumulation of the full tier, so
        calling setVertices() on every frame keeps a view repainting. Only call it when the
        vertices actually changed.
    */
    uint64 getChangeCount() const noexcept { return changeCount; }

    //==============================================================================
    /** Returns the vertex buffer of a primitive for a device, uploading every primitive if needed.

        The vertices setVertices() changed since are written into the same buffer first. A device
        that could not take them is asked again the next time.

        @param device          The device that renders the mesh.
        @param primitiveIndex  The primitive.
    */
    GpuBuffer::Ptr getVertexBuffer (const GpuDevice::Ptr& device, int primitiveIndex);

    /** Returns the 32-bit index buffer of a primitive for a device, uploading every primitive if needed.

        @param device          The device that renders the mesh.
        @param primitiveIndex  The primitive.
    */
    GpuBuffer::Ptr getIndexBuffer (const GpuDevice::Ptr& device, int primitiveIndex);

    /** Returns the 32-bit line list index buffer of the edges of a primitive for a device, as
        computeEdgeIndices() lists them, for wireframes. It is created the first time it is asked
        for, so meshes never drawn as wireframes do not hold one.

        @param device          The device that renders the mesh.
        @param primitiveIndex  The primitive.
    */
    GpuBuffer::Ptr getEdgeIndexBuffer (const GpuDevice::Ptr& device, int primitiveIndex);

    //==============================================================================
    /** Returns the edges of triangles as a line list, two indices per edge. An edge shared by
        several triangles is listed once, in the order the edges first appear.

        @param triangleIndices  Three indices per triangle; a trailing incomplete triangle is ignored.
    */
    static std::vector<uint32> computeEdgeIndices (Span<const uint32> triangleIndices);

private:
    friend class DeformNode;

    struct GpuBuffers
    {
        GpuBuffer::Ptr vertices;
        GpuBuffer::Ptr indices;
        GpuBuffer::Ptr edges;
        bool verticesStale = false; // setVertices() changed the vertices since they were uploaded
    };

    /** Adds what is drawn of a primitive a mesh already took, as it is: its vertices, indices, material and
        bounds, with nothing checked or generated again. For the copy a DeformNode draws.
    */
    void addDrawnCopy (const Primitive& primitive);

    void updateBounds (Primitive& changed);
    void prepareGpuBuffers (const GpuDevice::Ptr& device);

    String name;
    std::vector<Primitive> primitives;
    BoundingBox bounds;
    uint64 changeCount = 0;
    int numJoints = 0;
    int numMorphTargets = 0;
    std::vector<float> morphWeights;
    StringArray morphTargetNames;

    GpuDevice::Ptr cachedDevice;
    std::vector<GpuBuffers> gpuBuffers;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Mesh)
};

} // namespace yup
