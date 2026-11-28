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
/** A part that deforms the mesh its entity draws: by morph targets, then by a skin.

    The node is given a mesh in its bind pose, the one every instance of a model shares, and
    draws a copy of it in its place: it makes the copy, with the same indices and materials, and
    hands it to the MeshNode of its entity. The copy is what it deforms, with
    Mesh::setVertices(), so instances deform apart and the shared mesh is never written. A
    MeshNode that is given the shared mesh again while the node is attached, by being attached
    anew for example, takes the copy back at the next update. Given another mesh it draws that
    one, and the node deforms a copy that nothing draws.

    Each vertex starts from its bind pose. The morph targets of its primitive come first, their
    deltas added for the weight each target has here (Mesh::MorphTarget, setWeights()). Then, if
    there is a skin, the vertex goes where its joints take it, blended by its weights
    (Mesh::Influences, Skin). A skinned mesh is placed by its joints alone, as in glTF: moving
    the entity that draws it doesn't move it.

    Scene::update() brings the copy up to date, and only when there is something to do: a weight
    that changed, a joint that is no longer where it was for the entity, or new vertices or
    another primitive given to the mesh the node was made with, see Mesh::setVertices() and
    Mesh::addPrimitive(). A mesh that stands still costs a comparison per joint, and one that
    moves along with its joints, as an instance turning as a whole does, stays as it is. Joints
    are plain entities, so an AnimationNode, a binding or code moving their TransformNode are
    all the same to the node, which only looks at where they are. A scene that has a DeformNode
    updates its AnimationNodes before them, so that a pose is drawn by the frame that made it.
    A custom node moving joints from its update() asks for the same with
    Scene::addSystemRule<MyNode, yup::DeformNode>().

    GltfModel attaches one to every entity whose mesh a skin or morph targets deform:

    @code
    yup::Entity instance = model.instantiate (*scene);
    yup::Entity face = instance.findChild ("Face");

    if (auto* deform = face.get<yup::DeformNode>())
        deform->setWeights (std::array<float, 2> { 1.0f, 0.25f }); // the first target fully, a quarter of the second

    scene->update (0.0); // deforms what changed: on every frame, or before repainting a view that doesn't
    @endcode

    The mesh is deformed on the CPU, with four joints per vertex at most. A change rewrites
    every vertex of the primitives it deforms and their vertex buffers, and counts as a change
    of the mesh for the renderer, see Mesh::getChangeCount(). Each node keeps its copy of the
    vertices and of the indices, and as many vertices again to work on.

    @see Skin, Mesh, MeshNode, AnimationNode, GltfModel
*/
class YUP_API DeformNode : public Node
{
public:
    //==============================================================================
    /** Creates a part deforming a mesh, whose copy the entity draws once the node is attached.

        Attaching it gives the entity a MeshNode if it has none, and deforms the copy at once,
        so the entity shows the pose its joints are in before any update.

        @param meshToDeform   The mesh in its bind pose, with the influences and the morph
                              targets of its primitives. It is shared, and stays as it is. The
                              vertices it is given later, with Mesh::setVertices(), and the
                              primitives it takes later show in the copy from the next update.
        @param skinToFollow   The skin the influences of the mesh refer to, or nullptr for a
                              mesh that only has morph targets. A skin with fewer joints than
                              the mesh refers to, see Mesh::getNumJoints(), asserts and is not
                              used.
        @param jointEntities  The entity of every joint of the skin, in the order of
                              Skin::getJoints(). A joint that is destroyed later stays where it
                              last was. One that is not valid from the start, or missing, counts
                              as standing at the origin of the scene.
    */
    explicit DeformNode (Mesh::Ptr meshToDeform, Skin::Ptr skinToFollow = nullptr, std::vector<Entity> jointEntities = {});

    //==============================================================================
    /** Returns the mesh in its bind pose, the one this node was given. */
    const Mesh::Ptr& getSourceMesh() const noexcept { return source; }

    /** Returns the deformed copy of the mesh, the one the MeshNode of the entity draws. */
    const Mesh::Ptr& getDeformedMesh() const noexcept { return deformed; }

    /** Returns the skin the mesh follows, or nullptr when only morph targets deform it. */
    const Skin::Ptr& getSkin() const noexcept { return skin; }

    //==============================================================================
    /** Changes the weights of the morph targets.

        The mesh follows at the next update, and only if a weight is not what it was. A weights
        track of an AnimationNode animating the entity sets them the same way each time it poses
        its targets, so while such a clip plays its weights replace these on every update in
        which time goes by.

        @param newWeights  The weight of each morph target, in the order of the targets: 0
                           leaves a target out, 1 applies it fully. Weights beyond the targets
                           of the mesh are ignored, and targets beyond the weights keep theirs.
    */
    void setWeights (Span<const float> newWeights);

    /** Returns the weight of every morph target of the mesh, as many as
        Mesh::getNumMorphTargets().

        They start as the mesh has them, see Mesh::getMorphWeights().
    */
    Span<const float> getWeights() const noexcept { return weights; }

    //==============================================================================
    /** Deforms the mesh if a weight, the place of a joint for the entity or the mesh the node was
        given changed since the last time, called by Scene::update().

        Call it to see a change without updating the whole scene: the time is not used.

        @param deltaSeconds The time elapsed since the previous update.
    */
    void update (double deltaSeconds) override;

    /** @internal */
    void attachedToEntity() override;
    /** @internal */
    void detachedFromEntity() override;

private:
    bool copyAddedPrimitives();
    void deform();
    bool updateJointMatrices (bool deformsAnyway);
    void morphVertices (const Mesh::Primitive& primitive);
    bool skinVertex (const Mesh::Influences& influences, Mesh::Vertex& vertex, Vector3<float>& tangent) const noexcept;

    Mesh::Ptr source;
    Mesh::Ptr deformed;
    Skin::Ptr skin;
    std::vector<Entity> joints;
    std::vector<float> weights;

    // What the mesh was last deformed with
    std::vector<Matrix4> jointMatrices; // Takes a vertex where its joint has it, in the space of the entity
    uint64 sourceChangeCount = 0;
    bool stale = true; // A weight changed since, or the mesh was never deformed

    // The vertices the joints moved then: how far they get from the origin of the mesh, and the radius
    // of their bounds where they landed
    float skinnedReach = 0.0f;
    float skinnedSize = 0.0f;

    // Where the entity and the joints last were in the scene, to tell at once that nothing moved
    Matrix4 entityWorld;
    std::vector<Matrix4> jointWorlds;

    // Kept to allocate once
    std::vector<Matrix4> nextJointMatrices;
    std::vector<Mesh::Vertex> vertices; // Of the primitive being deformed
};

} // namespace yup
