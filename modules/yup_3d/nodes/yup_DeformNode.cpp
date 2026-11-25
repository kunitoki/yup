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

/** True if the vertices of a mesh would not stay where they are, as far as it shows, were a joint
    matrix replaced by another: if the two take a vertex to places that are further apart, along an
    axis, than a ten thousandth of the size of the mesh the joints make.

    @param reach  How far the vertices get from the origin of the mesh, before the joints move them.
    @param size   The radius of the bounds of the same vertices, where the joints put them. This is
                  another space than the one of @p reach, of another scale when the skin has one:
                  a mesh of integer positions that its inverse bind matrices bring back to size.
*/
bool movesVertices (const Matrix4& jointMatrix, const Matrix4& other, float reach, float size) noexcept
{
    const auto* a = jointMatrix.getData();
    const auto* b = other.getData();

    for (size_t row = 0; row < 3; ++row)
    {
        // The most a vertex within reach moves along the axis, where it lands: by the linear part, then
        // by the translation
        const auto turned = std::abs (a[row] - b[row]) + std::abs (a[4 + row] - b[4 + row]) + std::abs (a[8 + row] - b[8 + row]);
        const auto shifted = std::abs (a[12 + row] - b[12 + row]);

        if (turned * reach + shifted > 1.0e-4f * size)
            return true;
    }

    return false;
}

} // namespace

//==============================================================================

DeformNode::DeformNode (Mesh::Ptr meshToDeform, Skin::Ptr skinToFollow, std::vector<Entity> jointEntities)
    : source (std::move (meshToDeform))
    , skin (std::move (skinToFollow))
    , joints (std::move (jointEntities))
{
    if (source == nullptr)
    {
        jassertfalse; // There is no mesh to deform
        source = new Mesh();
    }

    // Only a skin that has every joint the mesh refers to can move it
    const auto numMeshJoints = source->getNumJoints();
    jassert (skin == nullptr || skin->getNumJoints() >= numMeshJoints);

    if (skin != nullptr && (numMeshJoints == 0 || skin->getNumJoints() < numMeshJoints))
        skin = nullptr;

    const auto numJoints = static_cast<size_t> (skin != nullptr ? skin->getNumJoints() : 0);
    joints.resize (numJoints);
    jointWorlds.resize (numJoints);
    jointMatrices.resize (numJoints);
    nextJointMatrices.resize (numJoints);

    weights.resize (static_cast<size_t> (source->getNumMorphTargets()));
    setWeights (source->getMorphWeights());

    // The mesh completed its primitives when it took them, so the copy takes them as they are, vertex
    // for vertex: what comes with the vertices of the mesh goes with the ones of the copy as well
    deformed = new Mesh (source->getName());

    for (const auto& primitive : source->getPrimitives())
        deformed->addPrimitive (primitive.vertices, primitive.indices, primitive.material);

    sourceChangeCount = source->getChangeCount();
}

//==============================================================================

void DeformNode::setWeights (Span<const float> newWeights)
{
    const auto count = jmin (newWeights.size(), weights.size());

    if (std::equal (newWeights.begin(), newWeights.begin() + count, weights.begin()))
        return;

    std::copy_n (newWeights.begin(), count, weights.begin());
    stale = true;
}

//==============================================================================

void DeformNode::update ([[maybe_unused]] double deltaSeconds)
{
    deform();
}

void DeformNode::attachedToEntity()
{
    auto entity = getEntity();

    // A pose is written by the animations before it is read here, whichever of the two the scene met
    // first. Asking again, for every node, changes nothing
    entity.getScene()->addSystemRule<AnimationNode, DeformNode>();

    entity.getOrAttach<MeshNode>().mesh = deformed;

    // At once, so an entity that is never updated shows where its joints are and not the bind pose
    deform();
}

void DeformNode::detachedFromEntity()
{
    // The entity draws the shared mesh again
    if (auto* meshNode = getEntity().get<MeshNode>(); meshNode != nullptr && meshNode->mesh == deformed)
        meshNode->mesh = source;
}

//==============================================================================

void DeformNode::deform()
{
    // Every entity drawing a mesh shows the vertices the mesh is given: the copy takes them again
    const auto sourceChanges = source->getChangeCount();
    const auto sourceChanged = std::exchange (sourceChangeCount, sourceChanges) != sourceChanges;

    // With new weights or new vertices, and the first time, the mesh is deformed whatever its joints did
    const auto deformsAnyway = std::exchange (stale, false) || sourceChanged;
    const auto jointsMoved = skin != nullptr && updateJointMatrices (deformsAnyway);

    if (! deformsAnyway && ! jointsMoved)
        return;

    // What the joints move is measured for the next time they do, see updateJointMatrices(): how far these
    // vertices get from the origin of the mesh, and the bounds of where they land
    auto reachSquared = 0.0f;
    BoundingBox landed;

    const auto numPrimitives = jmin (source->getNumPrimitives(), deformed->getNumPrimitives());

    for (int index = 0; index < numPrimitives; ++index)
    {
        const auto& primitive = source->getPrimitive (index);
        const auto skinned = skin != nullptr && ! primitive.influences.empty();

        // A primitive that nothing deforms only changes with the mesh
        if (! skinned && primitive.morphTargets.empty())
        {
            if (sourceChanged)
                deformed->setVertices (index, primitive.vertices);

            continue;
        }

        vertices.assign (primitive.vertices.begin(), primitive.vertices.end());
        morphVertices (primitive);

        for (size_t v = 0; v < vertices.size(); ++v)
        {
            auto& vertex = vertices[v];
            Vector3<float> tangent (vertex.tangent[0], vertex.tangent[1], vertex.tangent[2]);

            if (skinned)
            {
                const auto distanceSquared = vertex.position.lengthSquared();

                if (skinVertex (primitive.influences[v], vertex, tangent))
                {
                    reachSquared = jmax (reachSquared, distanceSquared);
                    landed.expand (vertex.position);
                }
            }

            // Sums of deltas and blends of joints don't keep the length of a direction
            vertex.normal = vertex.normal.normalized();
            tangent = tangent.normalized();
            vertex.tangent = { tangent.getX(), tangent.getY(), tangent.getZ(), vertex.tangent[3] };
        }

        deformed->setVertices (index, vertices);
    }

    skinnedReach = std::sqrt (reachSquared);
    skinnedSize = landed.getRadius();
}

bool DeformNode::updateJointMatrices (bool deformsAnyway)
{
    auto moved = false;

    const auto follow = [&moved] (Matrix4& last, const Matrix4& current)
    {
        if (last == current)
            return;

        last = current;
        moved = true;
    };

    follow (entityWorld, getEntity().getWorldMatrix());

    // A joint that was destroyed keeps the matrix it last had, one that never existed the identity
    for (size_t i = 0; i < joints.size(); ++i)
    {
        if (joints[i].isValid())
            follow (jointWorlds[i], joints[i].getWorldMatrix());
    }

    // Nothing moved in the scene: this is all a mesh that stands still costs
    if (! moved && ! deformsAnyway)
        return false;

    // glTF takes a vertex of the mesh to the scene with globalJoint * inverseBind, written for column
    // vectors: the rightmost matrix applies first. The renderer then places the mesh with the world
    // matrix of this entity, which glTF leaves out for a skinned mesh, so its inverse comes last to
    // cancel it: inverse (globalMesh) * globalJoint * inverseBind. Matrix4 composes the other way
    // round: a.followedBy (b), which a * b also means there, applies a first and then b
    const auto toEntity = entityWorld.inverted();
    const auto inverseBinds = skin->getInverseBindMatrices();

    // The entity and its joints can move together, as an instance turning as a whole does: the joint
    // matrices then are what they were, but for the rounding of the products, and the mesh stays as
    // it is. A change is passed over only when it would move no vertex by more than a ten thousandth
    // of the size of the deformed mesh: both are lengths where the vertices land, whatever scale the
    // skin has. The matrices are compared with the ones the mesh has, so what moves slowly still adds
    // up, and a mesh that is deformed anyway, as it is the first time, takes them as they are
    auto changed = deformsAnyway;

    for (size_t i = 0; i < jointMatrices.size(); ++i)
    {
        nextJointMatrices[i] = inverseBinds[i].followedBy (jointWorlds[i]).followedBy (toEntity);
        changed = changed || movesVertices (jointMatrices[i], nextJointMatrices[i], skinnedReach, skinnedSize);
    }

    if (! changed)
        return false;

    jointMatrices.swap (nextJointMatrices);
    return true;
}

void DeformNode::morphVertices (const Mesh::Primitive& primitive)
{
    const auto numTargets = jmin (primitive.morphTargets.size(), weights.size());

    for (size_t index = 0; index < numTargets; ++index)
    {
        const auto weight = weights[index];
        if (weight == 0.0f)
            continue;

        // Each list of a target is empty or holds a delta for every vertex
        const auto& target = primitive.morphTargets[index];

        for (size_t v = 0; v < target.positions.size(); ++v)
            vertices[v].position += target.positions[v] * weight;

        for (size_t v = 0; v < target.normals.size(); ++v)
            vertices[v].normal += target.normals[v] * weight;

        for (size_t v = 0; v < target.tangents.size(); ++v)
        {
            const auto delta = target.tangents[v] * weight;
            auto& tangent = vertices[v].tangent;

            tangent[0] += delta.getX();
            tangent[1] += delta.getY();
            tangent[2] += delta.getZ();
        }
    }
}

bool DeformNode::skinVertex (const Mesh::Influences& influences, Mesh::Vertex& vertex, Vector3<float>& tangent) const noexcept
{
    // The matrix of the vertex is the one of each of its joints, for its weight. All of them are affine:
    // the three axes of the linear part and the translation are the whole of it
    Vector3<float> x, y, z, origin;
    auto hasWeights = false;

    for (size_t i = 0; i < influences.joints.size(); ++i)
    {
        const auto weight = influences.weights[i];
        if (weight == 0.0f)
            continue;

        const auto* joint = jointMatrices[influences.joints[i]].getData();

        x += Vector3<float> (joint[0], joint[1], joint[2]) * weight;
        y += Vector3<float> (joint[4], joint[5], joint[6]) * weight;
        z += Vector3<float> (joint[8], joint[9], joint[10]) * weight;
        origin += Vector3<float> (joint[12], joint[13], joint[14]) * weight;
        hasWeights = true;
    }

    // glTF: no transformation applies to a vertex without weights. It stays where the bind pose has it
    if (! hasWeights)
        return false;

    const auto position = vertex.position;
    const auto normal = vertex.normal;

    vertex.position = x * position.getX() + y * position.getY() + z * position.getZ() + origin;

    // A normal turns with the cofactors of the linear part, which is its inverse transpose up to a factor:
    // it stays perpendicular to the surface when a joint is scaled along one axis only
    vertex.normal = y.crossProduct (z) * normal.getX() + z.crossProduct (x) * normal.getY() + x.crossProduct (y) * normal.getZ();

    tangent = x * tangent.getX() + y * tangent.getY() + z * tangent.getZ();
    return true;
}

} // namespace yup
