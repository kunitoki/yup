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

#include <gtest/gtest.h>

#include <yup_3d/yup_3d.h>

using namespace yup;

class DeformNodeTests : public ::testing::Test
{
protected:
    /** An entity drawing a deformed strip, and the two joints the strip follows. */
    struct Rig
    {
        Entity entity;
        Entity root;
        Entity tip;
        DeformNode* deform = nullptr;

        /** The vertices the entity draws, which stay in place when they are deformed again. */
        const std::vector<Mesh::Vertex>& getVertices() const
        {
            return entity.get<MeshNode>()->mesh->getPrimitive (0).vertices;
        }

        /** Where the scene sees a vertex: placed by the entity that draws it. */
        Vector3<float> getWorldPosition (size_t index) const
        {
            return entity.getWorldMatrix().transformPoint (getVertices()[index].position);
        }
    };

    /** A strip standing in the plane that faces +z, 1 wide and 2 high: two vertices at its foot, then
        two at its top, with normals along z and tangents along y.
    */
    static std::vector<Mesh::Vertex> makeStripVertices()
    {
        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { 0.0f, 0.0f, 0.0f };
        vertices[1].position = { 1.0f, 0.0f, 0.0f };
        vertices[2].position = { 0.0f, 2.0f, 0.0f };
        vertices[3].position = { 1.0f, 2.0f, 0.0f };

        for (auto& vertex : vertices)
        {
            vertex.normal = { 0.0f, 0.0f, 1.0f };
            vertex.tangent = { 0.0f, 1.0f, 0.0f, 1.0f };
        }

        return vertices;
    }

    /** The foot of the strip follows joint 0 and its top joint 1. */
    static std::vector<Mesh::Influences> makeStripInfluences()
    {
        std::vector<Mesh::Influences> influences (4);
        influences[0] = influences[1] = { { 0, 0, 0, 0 }, { 1.0f, 0.0f, 0.0f, 0.0f } };
        influences[2] = influences[3] = { { 1, 0, 0, 0 }, { 1.0f, 0.0f, 0.0f, 0.0f } };
        return influences;
    }

    /** Two morph targets of the strip. The first lifts its top by 1, leaning the normals of the top
        from z to y and its tangents from y to z. The second moves every vertex by 0.5 along x.
    */
    static std::vector<Mesh::MorphTarget> makeStripTargets()
    {
        std::vector<Mesh::MorphTarget> targets (2);
        targets[0].positions = { {}, {}, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } };
        targets[0].normals = { {}, {}, { 0.0f, 1.0f, -1.0f }, { 0.0f, 1.0f, -1.0f } };
        targets[0].tangents = { {}, {}, { 0.0f, -1.0f, 1.0f }, { 0.0f, -1.0f, 1.0f } };
        targets[1].positions = std::vector<Vector3<float>> (4, Vector3<float> (0.5f, 0.0f, 0.0f));
        return targets;
    }

    static Mesh::Ptr makeStrip (std::vector<Mesh::Vertex> vertices = makeStripVertices(),
                                std::vector<Mesh::Influences> influences = makeStripInfluences(),
                                std::vector<Mesh::MorphTarget> targets = {})
    {
        auto mesh = Mesh::Ptr (new Mesh ("strip"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 1, 3, 2 }, Material::Ptr (new Material()), std::move (influences), std::move (targets));
        return mesh;
    }

    /** The strip with its morph targets, and no joints. */
    static Mesh::Ptr makeMorphedStrip()
    {
        return makeStrip (makeStripVertices(), {}, makeStripTargets());
    }

    /** Joint 0 is bound where the strip stands. Joint 1 is bound 1 up, halfway up the strip, where
        the strip bends: its inverse bind matrix brings that point back to the origin.
    */
    static Skin::Ptr makeSkin()
    {
        return new Skin ("bend", { 0, 1 }, { Matrix4(), Matrix4::translation ({ 0.0f, -1.0f, 0.0f }) });
    }

    /** How much larger the positions of makeScaledStrip() are than the strip they make: the range of
        14-bit integers, as in files whose positions are quantized to save room.
    */
    static constexpr float positionScale = 16384.0f;

    /** The strip with positions that are positionScale times as large. */
    static Mesh::Ptr makeScaledStrip()
    {
        auto vertices = makeStripVertices();

        for (auto& vertex : vertices)
            vertex.position *= positionScale;

        return makeStrip (vertices);
    }

    /** The skin of makeSkin() for the scaled strip: its inverse bind matrices also bring the positions
        back to size, as the ones of a mesh of integer positions do.
    */
    static Skin::Ptr makeScalingSkin()
    {
        const auto toSize = Matrix4::scaling (1.0f / positionScale);
        return new Skin ("bend", { 0, 1 }, { toSize, toSize.followedBy (Matrix4::translation ({ 0.0f, -1.0f, 0.0f })) });
    }

    /** A quarter turn about x: a point (x, y, z) goes to (x, -z, y). */
    static Quaternion quarterTurnAboutX()
    {
        return Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, MathConstants<float>::halfPi);
    }

    /** A clip of one track animating the first target of its player over the given keys. */
    static AnimationClip::Ptr makeClip (AnimationClip::Path path, std::vector<float> times, std::vector<float> values)
    {
        AnimationClip::Track track;
        track.target = 0;
        track.path = path;
        track.times = std::move (times);
        track.values = std::move (values);

        auto clip = AnimationClip::Ptr (new AnimationClip ("clip"));
        EXPECT_TRUE (clip->addTrack (std::move (track)).wasOk());
        return clip;
    }

    static void expectNear (const Vector3<float>& actual, const Vector3<float>& expected, float tolerance = 1.0e-5f)
    {
        EXPECT_NEAR (actual.getX(), expected.getX(), tolerance);
        EXPECT_NEAR (actual.getY(), expected.getY(), tolerance);
        EXPECT_NEAR (actual.getZ(), expected.getZ(), tolerance);
    }

    static void expectNear (const std::array<float, 4>& actual, const std::array<float, 4>& expected, float tolerance = 1.0e-5f)
    {
        for (size_t i = 0; i < actual.size(); ++i)
            EXPECT_NEAR (actual[i], expected[i], tolerance) << i;
    }

    /** The strip drawn by an entity of the scene, following a root joint standing at the origin and
        its child, a tip joint 1 up: where the skin of makeSkin() was bound. The entity and the root
        joint are children of the given entity, or of the root of the scene.
    */
    Rig makeRig (const Mesh::Ptr& mesh, const Skin::Ptr& skin = makeSkin(), Entity parent = {})
    {
        Rig rig;
        rig.root = scene.createEntity ("root", parent);
        rig.root.attach<TransformNode>();
        rig.tip = rig.root.createChild ("tip");
        rig.tip.attach<TransformNode> (Vector3<float> { 0.0f, 1.0f, 0.0f });

        rig.entity = scene.createEntity ("strip", parent);
        rig.entity.attach<MeshNode> (mesh);
        rig.deform = &rig.entity.attach<DeformNode> (mesh, skin, std::vector<Entity> { rig.root, rig.tip });
        return rig;
    }

    Scene scene;
};

//==============================================================================

TEST_F (DeformNodeTests, DrawsACopyOfTheMeshInThePoseOfItsJoints)
{
    const auto mesh = makeStrip();
    const auto rig = makeRig (mesh);

    // The entity draws a mesh of its own, with the indices and the material of the one it was given
    const auto drawn = rig.entity.get<MeshNode>()->mesh;
    ASSERT_NE (drawn, nullptr);
    EXPECT_NE (drawn, mesh);
    EXPECT_EQ (drawn, rig.deform->getDeformedMesh());
    EXPECT_EQ (rig.deform->getSourceMesh(), mesh);
    EXPECT_NE (rig.deform->getSkin(), nullptr);
    EXPECT_EQ (drawn->getName(), "strip");
    ASSERT_EQ (drawn->getNumPrimitives(), 1);
    EXPECT_EQ (drawn->getPrimitive (0).indices, mesh->getPrimitive (0).indices);
    EXPECT_EQ (drawn->getPrimitive (0).material, mesh->getPrimitive (0).material);

    // The joints stand where the strip was bound to them, so it is in its bind pose
    const auto& vertices = rig.getVertices();
    ASSERT_EQ (vertices.size(), 4u);
    expectNear (vertices[0].position, { 0.0f, 0.0f, 0.0f });
    expectNear (vertices[1].position, { 1.0f, 0.0f, 0.0f });
    expectNear (vertices[2].position, { 0.0f, 2.0f, 0.0f });
    expectNear (vertices[3].position, { 1.0f, 2.0f, 0.0f });
    expectNear (vertices[3].normal, { 0.0f, 0.0f, 1.0f });
    expectNear (vertices[3].tangent, { 0.0f, 1.0f, 0.0f, 1.0f });
}

TEST_F (DeformNodeTests, AttachingGivesTheEntityAMeshNodeIfItHasNone)
{
    const auto mesh = makeStrip();
    auto entity = scene.createEntity ("strip");
    const auto& deform = entity.attach<DeformNode> (mesh);

    ASSERT_NE (entity.get<MeshNode>(), nullptr);
    EXPECT_EQ (entity.get<MeshNode>()->mesh, deform.getDeformedMesh());

    // Without a skin the joints of the mesh move nothing
    EXPECT_EQ (deform.getSkin(), nullptr);
    expectNear (deform.getDeformedMesh()->getPrimitive (0).vertices[2].position, { 0.0f, 2.0f, 0.0f });
}

TEST_F (DeformNodeTests, TurningAJointMovesItsVerticesAndLeavesTheOthers)
{
    const auto mesh = makeStrip();
    const auto rig = makeRig (mesh);
    const auto& vertices = rig.getVertices();

    rig.tip.get<TransformNode>()->setRotation (quarterTurnAboutX());
    scene.update (0.0);

    // The foot follows the root, which stays
    expectNear (vertices[0].position, { 0.0f, 0.0f, 0.0f });
    expectNear (vertices[1].position, { 1.0f, 0.0f, 0.0f });
    expectNear (vertices[0].normal, { 0.0f, 0.0f, 1.0f });
    expectNear (vertices[0].tangent, { 0.0f, 1.0f, 0.0f, 1.0f });

    // The top turns about the tip: (0, 1, 0) and (1, 1, 0) from it, they go to (0, 0, 1) and (1, 0, 1)
    expectNear (vertices[2].position, { 0.0f, 1.0f, 1.0f });
    expectNear (vertices[3].position, { 1.0f, 1.0f, 1.0f });

    // Normals and tangents turn with their joint, the handedness of a tangent staying
    expectNear (vertices[2].normal, { 0.0f, -1.0f, 0.0f });
    expectNear (vertices[3].normal, { 0.0f, -1.0f, 0.0f });
    expectNear (vertices[2].tangent, { 0.0f, 0.0f, 1.0f, 1.0f });

    // The bounds, in the space of the entity like the vertices, follow them
    expectNear (rig.deform->getDeformedMesh()->getBounds().getMin(), { 0.0f, 0.0f, 0.0f });
    expectNear (rig.deform->getDeformedMesh()->getBounds().getMax(), { 1.0f, 1.0f, 1.0f });
}

TEST_F (DeformNodeTests, AVertexSharedByTwoJointsLandsBetweenThem)
{
    auto influences = makeStripInfluences();
    influences[2] = { { 0, 1, 0, 0 }, { 0.5f, 0.5f, 0.0f, 0.0f } };

    const auto mesh = makeStrip (makeStripVertices(), influences);
    const auto rig = makeRig (mesh);
    const auto& vertices = rig.getVertices();

    rig.tip.get<TransformNode>()->setRotation (quarterTurnAboutX());
    scene.update (0.0);

    // Halfway between (0, 2, 0), where the root leaves it, and (0, 1, 1), where the tip takes it
    expectNear (vertices[2].position, { 0.0f, 1.5f, 0.5f });

    // Its normal, halfway between z and -y, and its tangent, halfway between y and z, are unit vectors
    expectNear (vertices[2].normal, { 0.0f, -0.70710678f, 0.70710678f });
    expectNear (vertices[2].tangent, { 0.0f, 0.70710678f, 0.70710678f, 1.0f });

    // The other vertex of the top follows the tip alone
    expectNear (vertices[3].position, { 1.0f, 1.0f, 1.0f });
}

TEST_F (DeformNodeTests, VerticesWithoutWeightsStayInTheBindPose)
{
    auto influences = makeStripInfluences();
    influences[3] = {};

    const auto mesh = makeStrip (makeStripVertices(), influences);
    const auto rig = makeRig (mesh);
    const auto& vertices = rig.getVertices();

    rig.tip.get<TransformNode>()->setRotation (quarterTurnAboutX());
    scene.update (0.0);

    expectNear (vertices[2].position, { 0.0f, 1.0f, 1.0f });
    expectNear (vertices[3].position, { 1.0f, 2.0f, 0.0f });
    expectNear (vertices[3].normal, { 0.0f, 0.0f, 1.0f });
}

TEST_F (DeformNodeTests, NormalsStayPerpendicularToASurfaceAJointStretches)
{
    // The top leans: its normal is (0, 0.6, 0.8), and its tangent (0, 0.8, -0.6) lies in the surface
    auto stripVertices = makeStripVertices();
    stripVertices[2].normal = { 0.0f, 0.6f, 0.8f };
    stripVertices[2].tangent = { 0.0f, 0.8f, -0.6f, -1.0f };

    const auto mesh = makeStrip (stripVertices);
    const auto rig = makeRig (mesh);
    const auto& vertices = rig.getVertices();

    rig.tip.get<TransformNode>()->setScale ({ 1.0f, 3.0f, 1.0f });
    scene.update (0.0);

    // 1 above the tip, the vertex is taken 3 above it
    expectNear (vertices[2].position, { 0.0f, 4.0f, 0.0f });

    // The tangent stretches with the surface, to (0, 2.4, -0.6), and the normal turns the other way to
    // stay perpendicular to it, to (0, 0.6, 2.4): both of length 2.4738634 before they are unit vectors
    expectNear (vertices[2].tangent, { 0.0f, 0.9701425f, -0.2425356f, -1.0f });
    expectNear (vertices[2].normal, { 0.0f, 0.2425356f, 0.9701425f });
}

TEST_F (DeformNodeTests, MovingTheEntityOfTheMeshLeavesItWhereItsJointsPutIt)
{
    const auto mesh = makeStrip();
    auto rig = makeRig (mesh);
    const auto& vertices = rig.getVertices();

    // The entity moves to (5, 0, 0) and doubles in size: a point p of its space is at 2 p + (5, 0, 0)
    rig.entity.attach<TransformNode> (Vector3<float> { 5.0f, 0.0f, 0.0f }, Quaternion(), Vector3<float> { 2.0f, 2.0f, 2.0f });
    scene.update (0.0);

    // The vertices are given the places that this brings back to the bind pose
    expectNear (vertices[0].position, { -2.5f, 0.0f, 0.0f });
    expectNear (vertices[1].position, { -2.0f, 0.0f, 0.0f });
    expectNear (vertices[2].position, { -2.5f, 1.0f, 0.0f });
    expectNear (vertices[3].position, { -2.0f, 1.0f, 0.0f });
    expectNear (vertices[3].normal, { 0.0f, 0.0f, 1.0f });

    expectNear (rig.getWorldPosition (0), { 0.0f, 0.0f, 0.0f });
    expectNear (rig.getWorldPosition (3), { 1.0f, 2.0f, 0.0f });

    // A joint still takes its vertices where it would without the move
    rig.tip.get<TransformNode>()->setRotation (quarterTurnAboutX());
    scene.update (0.0);

    expectNear (rig.getWorldPosition (1), { 1.0f, 0.0f, 0.0f });
    expectNear (rig.getWorldPosition (2), { 0.0f, 1.0f, 1.0f });
    expectNear (rig.getWorldPosition (3), { 1.0f, 1.0f, 1.0f });
}

TEST_F (DeformNodeTests, InstancesDeformApartAndTheSharedMeshKeepsItsBindPose)
{
    const auto mesh = makeStrip();
    const auto first = makeRig (mesh);
    const auto second = makeRig (mesh);

    first.tip.get<TransformNode>()->setRotation (quarterTurnAboutX());
    scene.update (0.0);

    // Each entity draws its own copy, and only the one whose joint turned bends
    EXPECT_NE (first.deform->getDeformedMesh(), second.deform->getDeformedMesh());
    expectNear (first.getVertices()[2].position, { 0.0f, 1.0f, 1.0f });
    expectNear (second.getVertices()[2].position, { 0.0f, 2.0f, 0.0f });

    // The mesh they share was never written, and its material is the one both copies draw with
    expectNear (mesh->getPrimitive (0).vertices[2].position, { 0.0f, 2.0f, 0.0f });
    EXPECT_EQ (mesh->getChangeCount(), 0u);
    EXPECT_EQ (first.deform->getDeformedMesh()->getPrimitive (0).material, mesh->getPrimitive (0).material);
    EXPECT_EQ (second.deform->getDeformedMesh()->getPrimitive (0).material, mesh->getPrimitive (0).material);
}

TEST_F (DeformNodeTests, NothingIsDoneWhileNothingChanges)
{
    const auto mesh = makeStrip();
    const auto rig = makeRig (mesh);
    const auto deformed = rig.deform->getDeformedMesh();
    const auto count = deformed->getChangeCount();

    scene.update (1.0);
    scene.update (1.0);
    EXPECT_EQ (deformed->getChangeCount(), count);

    // A transform set to what it is gives the same world matrix
    rig.tip.get<TransformNode>()->setPosition ({ 0.0f, 1.0f, 0.0f });
    scene.update (1.0);
    EXPECT_EQ (deformed->getChangeCount(), count);

    // A joint that moves has the mesh deformed, once
    rig.tip.get<TransformNode>()->setRotation (quarterTurnAboutX());
    scene.update (1.0);
    EXPECT_EQ (deformed->getChangeCount(), count + 1);

    scene.update (1.0);
    rig.deform->update (0.0);
    EXPECT_EQ (deformed->getChangeCount(), count + 1);
}

TEST_F (DeformNodeTests, MovingAnInstanceAsAWholeLeavesItsMeshAsItIs)
{
    // The strip and its joints stand under one entity, as the nodes of a model do under their instance
    auto instance = scene.createEntity ("instance");
    auto& transform = instance.attach<TransformNode>();

    const auto mesh = makeStrip();
    const auto rig = makeRig (mesh, makeSkin(), instance);
    const auto deformed = rig.deform->getDeformedMesh();
    const auto count = deformed->getChangeCount();

    // Moved and turned, frame after frame: every world matrix changes, and the joints stay where they
    // are for the strip
    for (int frame = 1; frame <= 5; ++frame)
    {
        transform.setPosition ({ 0.25f * static_cast<float> (frame), 0.5f, -1.0f });
        transform.setRotation (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 0.3f * static_cast<float> (frame)));
        scene.update (0.0);
    }

    EXPECT_EQ (deformed->getChangeCount(), count);

    // The same goes for the root of the scene
    auto& rootTransform = scene.getRoot().getOrAttach<TransformNode>();
    rootTransform.setPosition ({ 0.0f, 0.5f, 0.25f });
    scene.update (0.0);
    rootTransform.setRotation (quarterTurnAboutX());
    scene.update (0.0);

    EXPECT_EQ (deformed->getChangeCount(), count);
    expectNear (rig.getVertices()[3].position, { 1.0f, 2.0f, 0.0f });

    // A joint that moves on its own still takes its vertices along: the top turns about the tip
    rig.tip.get<TransformNode>()->setRotation (quarterTurnAboutX());
    scene.update (0.0);

    EXPECT_EQ (deformed->getChangeCount(), count + 1);
    expectNear (rig.getVertices()[2].position, { 0.0f, 1.0f, 1.0f }, 1.0e-4f);
    expectNear (rig.getVertices()[0].position, { 0.0f, 0.0f, 0.0f }, 1.0e-4f);
}

TEST_F (DeformNodeTests, AMeshItsSkinBringsBackToSizeFollowsItsJointsLikeAnyOther)
{
    // The same strip twice: as it is, and with positions 16 384 times as large, which its inverse bind
    // matrices bring back to size
    const auto plain = makeRig (makeStrip());
    const auto scaled = makeRig (makeScaledStrip(), makeScalingSkin());
    const auto deformed = scaled.deform->getDeformedMesh();

    // From the start it is drawn where its joints put it, the bind pose of the strip, and not where the
    // mesh has its vertices
    expectNear (scaled.getVertices()[1].position, { 1.0f, 0.0f, 0.0f });
    expectNear (scaled.getVertices()[2].position, { 0.0f, 2.0f, 0.0f });
    expectNear (scaled.getVertices()[3].position, { 1.0f, 2.0f, 0.0f });
    expectNear (deformed->getBounds().getMax(), { 1.0f, 2.0f, 0.0f });

    // Moved as a whole, here by the root of the scene, it stays as it is
    const auto count = deformed->getChangeCount();
    auto& rootTransform = scene.getRoot().getOrAttach<TransformNode>();

    for (int frame = 1; frame <= 5; ++frame)
    {
        rootTransform.setPosition ({ 0.25f * static_cast<float> (frame), 0.5f, -1.0f });
        rootTransform.setRotation (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 0.3f * static_cast<float> (frame)));
        scene.update (0.0);
    }

    EXPECT_EQ (deformed->getChangeCount(), count);

    // One degree of a joint shows on a strip that is 2 high, however large the positions it is made
    // from: the top, at (0, 1, 0) from the tip, turns to (0, cos, sin) from it
    const auto oneDegree = Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, MathConstants<float>::pi / 180.0f);
    plain.tip.get<TransformNode>()->setRotation (oneDegree);
    scaled.tip.get<TransformNode>()->setRotation (oneDegree);
    scene.update (0.0);

    EXPECT_EQ (deformed->getChangeCount(), count + 1);
    expectNear (scaled.getVertices()[2].position, { 0.0f, 1.9998477f, 0.0174524f });

    // It lands every vertex where the strip that has that size goes
    for (size_t i = 0; i < 4; ++i)
    {
        expectNear (scaled.getVertices()[i].position, plain.getVertices()[i].position);
        expectNear (scaled.getVertices()[i].normal, plain.getVertices()[i].normal);
    }
}

TEST_F (DeformNodeTests, UpdatingTheNodeAloneDeformsTheMesh)
{
    const auto mesh = makeStrip();
    const auto rig = makeRig (mesh);

    // Nothing follows a joint until the node is updated, by the scene or by hand
    rig.tip.get<TransformNode>()->setRotation (quarterTurnAboutX());
    expectNear (rig.getVertices()[2].position, { 0.0f, 2.0f, 0.0f });

    rig.deform->update (0.0);
    expectNear (rig.getVertices()[2].position, { 0.0f, 1.0f, 1.0f });
}

TEST_F (DeformNodeTests, FollowsTheVerticesItsMeshIsGiven)
{
    // The strip, which its joints deform, and a triangle 2 behind it, which nothing does
    std::vector<Mesh::Vertex> triangle (3);
    triangle[0].position = { 0.0f, 0.0f, -2.0f };
    triangle[1].position = { 1.0f, 0.0f, -2.0f };
    triangle[2].position = { 0.0f, 1.0f, -2.0f };

    for (auto& vertex : triangle)
        vertex.normal = { 0.0f, 0.0f, 1.0f };

    const auto mesh = makeStrip();
    mesh->addPrimitive (triangle, {});

    const auto rig = makeRig (mesh);
    const auto deformed = rig.deform->getDeformedMesh();
    const auto count = deformed->getChangeCount();
    ASSERT_EQ (deformed->getNumPrimitives(), 2);

    // The mesh is given vertices that are 1 further along z, for both primitives
    auto strip = makeStripVertices();

    for (auto& vertex : strip)
        vertex.position += Vector3<float> (0.0f, 0.0f, 1.0f);

    for (auto& vertex : triangle)
        vertex.position += Vector3<float> (0.0f, 0.0f, 1.0f);

    mesh->setVertices (0, strip);
    mesh->setVertices (1, triangle);

    // The copy the entity draws takes them at the next update, once
    expectNear (deformed->getPrimitive (0).vertices[2].position, { 0.0f, 2.0f, 0.0f });
    scene.update (0.0);

    expectNear (deformed->getPrimitive (0).vertices[2].position, { 0.0f, 2.0f, 1.0f });
    expectNear (deformed->getPrimitive (1).vertices[2].position, { 0.0f, 1.0f, -1.0f });
    EXPECT_EQ (deformed->getChangeCount(), count + 2);

    scene.update (0.0);
    EXPECT_EQ (deformed->getChangeCount(), count + 2);

    // The joints deform the vertices the mesh has now: the top of the strip, at (0, 1, 1) from the tip,
    // turns to (0, -1, 1) from it. The triangle is not written again
    rig.tip.get<TransformNode>()->setRotation (quarterTurnAboutX());
    scene.update (0.0);

    expectNear (deformed->getPrimitive (0).vertices[2].position, { 0.0f, 0.0f, 1.0f });
    expectNear (deformed->getPrimitive (1).vertices[2].position, { 0.0f, 1.0f, -1.0f });
    EXPECT_EQ (deformed->getChangeCount(), count + 3);
}

TEST_F (DeformNodeTests, ADestroyedJointStaysWhereItLastWas)
{
    const auto mesh = makeStrip();
    auto rig = makeRig (mesh);
    const auto& vertices = rig.getVertices();

    rig.tip.get<TransformNode>()->setRotation (quarterTurnAboutX());
    scene.update (0.0);

    const auto count = rig.deform->getDeformedMesh()->getChangeCount();
    rig.tip.destroy();
    scene.update (0.0);

    // Losing a joint changes nothing by itself
    EXPECT_EQ (rig.deform->getDeformedMesh()->getChangeCount(), count);
    expectNear (vertices[2].position, { 0.0f, 1.0f, 1.0f });

    // The root carries the foot along, while the top stays with the place the tip last had
    rig.root.get<TransformNode>()->setPosition ({ 0.0f, 0.0f, 3.0f });
    scene.update (0.0);

    expectNear (vertices[0].position, { 0.0f, 0.0f, 3.0f });
    expectNear (vertices[1].position, { 1.0f, 0.0f, 3.0f });
    expectNear (vertices[2].position, { 0.0f, 1.0f, 1.0f });
    expectNear (vertices[3].position, { 1.0f, 1.0f, 1.0f });
}

TEST_F (DeformNodeTests, AJointThatNeverExistedStandsAtTheOrigin)
{
    const auto mesh = makeStrip();
    auto entity = scene.createEntity ("strip");

    // One entity for two joints: the tip has none
    const auto& deform = entity.attach<DeformNode> (mesh, makeSkin(), std::vector<Entity> { scene.createEntity ("root") });
    const auto& vertices = deform.getDeformedMesh()->getPrimitive (0).vertices;

    // Bound 1 up and standing at the origin, the tip takes its vertices 1 down
    EXPECT_NE (deform.getSkin(), nullptr);
    expectNear (vertices[0].position, { 0.0f, 0.0f, 0.0f });
    expectNear (vertices[2].position, { 0.0f, 1.0f, 0.0f });
    expectNear (vertices[3].position, { 1.0f, 1.0f, 0.0f });

    scene.update (0.0);
    expectNear (vertices[3].position, { 1.0f, 1.0f, 0.0f });
}

TEST_F (DeformNodeTests, FollowsAnAnimationInTheUpdateThatPosesIt)
{
    // The scene meets the DeformNode first: without a rule its nodes would update before the animations
    const auto mesh = makeStrip();
    const auto rig = makeRig (mesh);

    // The tip makes a quarter turn about x in a second
    const auto clip = makeClip (AnimationClip::Path::rotation, { 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f, 0.70710678f, 0.0f, 0.0f, 0.70710678f });
    auto& player = scene.createEntity ("player").attach<AnimationNode> (std::vector<AnimationClip::Ptr> { clip }, std::vector<Entity> { rig.tip });

    player.setPlaying (true);
    scene.update (1.0);

    expectNear (rig.getVertices()[2].position, { 0.0f, 1.0f, 1.0f });
    expectNear (rig.getVertices()[3].position, { 1.0f, 1.0f, 1.0f });

    // Every DeformNode asks for that order when it is attached: asking again is fine
    makeRig (mesh);
    EXPECT_TRUE ((scene.addSystemRule<AnimationNode, DeformNode>()));
}

TEST_F (DeformNodeTests, DetachingGivesTheEntityTheSharedMeshBack)
{
    const auto mesh = makeStrip();
    auto rig = makeRig (mesh);
    ASSERT_NE (rig.entity.get<MeshNode>()->mesh, mesh);

    EXPECT_TRUE (rig.entity.detach<DeformNode>());
    EXPECT_EQ (rig.entity.get<MeshNode>()->mesh, mesh);
}

TEST_F (DeformNodeTests, ASkinWithoutEveryJointOfTheMeshIsNotUsed)
{
#if YUP_ASSERTIONS_ENABLED_OR_LOGGED
    GTEST_SKIP() << "A skin with fewer joints than the mesh refers to triggers an assertion when assertions are enabled or logged";
#endif

    // The strip follows joints 0 and 1, the skin only has one
    const auto mesh = makeStrip();
    const auto rig = makeRig (mesh, new Skin ("short", { 0 }));

    EXPECT_EQ (rig.deform->getSkin(), nullptr);

    rig.root.get<TransformNode>()->setPosition ({ 0.0f, 0.0f, 3.0f });
    scene.update (0.0);
    expectNear (rig.getVertices()[0].position, { 0.0f, 0.0f, 0.0f });
}

//==============================================================================

TEST_F (DeformNodeTests, ATargetMovesTheVerticesByItsWeight)
{
    const auto mesh = makeMorphedStrip();
    auto entity = scene.createEntity ("strip");
    auto& deform = entity.attach<DeformNode> (mesh);
    const auto& vertices = deform.getDeformedMesh()->getPrimitive (0).vertices;

    // Every weight starts at 0, which leaves the bind pose
    ASSERT_EQ (deform.getWeights().size(), 2u);
    EXPECT_FLOAT_EQ (deform.getWeights()[0], 0.0f);
    EXPECT_FLOAT_EQ (deform.getWeights()[1], 0.0f);
    expectNear (vertices[2].position, { 0.0f, 2.0f, 0.0f });

    // Half of the first target: the top is lifted by 0.5
    deform.setWeights (std::vector<float> { 0.5f });
    scene.update (0.0);

    expectNear (vertices[0].position, { 0.0f, 0.0f, 0.0f });
    expectNear (vertices[2].position, { 0.0f, 2.5f, 0.0f });
    expectNear (vertices[3].position, { 1.0f, 2.5f, 0.0f });

    // (0, 0, 1) + 0.5 (0, 1, -1) and (0, 1, 0) + 0.5 (0, -1, 1) are both (0, 0.5, 0.5), made unit vectors
    expectNear (vertices[2].normal, { 0.0f, 0.70710678f, 0.70710678f });
    expectNear (vertices[2].tangent, { 0.0f, 0.70710678f, 0.70710678f, 1.0f });
    expectNear (vertices[0].normal, { 0.0f, 0.0f, 1.0f });

    // All of it
    deform.setWeights (std::vector<float> { 1.0f });
    scene.update (0.0);

    expectNear (vertices[2].position, { 0.0f, 3.0f, 0.0f });
    expectNear (vertices[2].normal, { 0.0f, 1.0f, 0.0f });
    expectNear (vertices[2].tangent, { 0.0f, 0.0f, 1.0f, 1.0f });

    // The bounds follow, and the mesh the targets belong to stays as it is
    expectNear (deform.getDeformedMesh()->getBounds().getMax(), { 1.0f, 3.0f, 0.0f });
    expectNear (mesh->getPrimitive (0).vertices[2].position, { 0.0f, 2.0f, 0.0f });
}

TEST_F (DeformNodeTests, TargetsAddUp)
{
    const auto mesh = makeMorphedStrip();
    auto entity = scene.createEntity ("strip");
    auto& deform = entity.attach<DeformNode> (mesh);
    const auto& vertices = deform.getDeformedMesh()->getPrimitive (0).vertices;

    deform.setWeights (std::vector<float> { 1.0f, 1.0f });
    scene.update (0.0);

    // The second target moves every vertex by 0.5 along x, the first lifts the top by 1
    expectNear (vertices[0].position, { 0.5f, 0.0f, 0.0f });
    expectNear (vertices[3].position, { 1.5f, 3.0f, 0.0f });

    // Twice the second target and a quarter of the first
    deform.setWeights (std::vector<float> { 0.25f, 2.0f });
    scene.update (0.0);

    expectNear (vertices[0].position, { 1.0f, 0.0f, 0.0f });
    expectNear (vertices[3].position, { 2.0f, 2.25f, 0.0f });
}

TEST_F (DeformNodeTests, TargetsApplyBeforeTheSkin)
{
    const auto mesh = makeStrip (makeStripVertices(), makeStripInfluences(), makeStripTargets());
    const auto rig = makeRig (mesh);
    const auto& vertices = rig.getVertices();

    rig.deform->setWeights (std::vector<float> { 1.0f });
    rig.tip.get<TransformNode>()->setRotation (quarterTurnAboutX());
    scene.update (0.0);

    // The top is lifted to (0, 3, 0) and (1, 3, 0), 2 above the tip, which then turns it: the other
    // way round, it would be turned to (0, 1, 1) and then lifted to (0, 2, 1)
    expectNear (vertices[2].position, { 0.0f, 1.0f, 2.0f });
    expectNear (vertices[3].position, { 1.0f, 1.0f, 2.0f });

    // The target leans the normal to y and the tangent to z, which the tip turns to z and -y
    expectNear (vertices[2].normal, { 0.0f, 0.0f, 1.0f });
    expectNear (vertices[2].tangent, { 0.0f, -1.0f, 0.0f, 1.0f });

    expectNear (vertices[0].position, { 0.0f, 0.0f, 0.0f });
}

TEST_F (DeformNodeTests, SettingTheSameWeightsAgainDoesNoWork)
{
    const auto mesh = makeMorphedStrip();
    auto entity = scene.createEntity ("strip");
    auto& deform = entity.attach<DeformNode> (mesh);

    deform.setWeights (std::vector<float> { 0.5f, 0.25f });
    scene.update (0.0);
    const auto count = deform.getDeformedMesh()->getChangeCount();

    deform.setWeights (std::vector<float> { 0.5f, 0.25f });
    scene.update (0.0);
    EXPECT_EQ (deform.getDeformedMesh()->getChangeCount(), count);

    // Moving the entity of a mesh without a skin changes nothing either
    entity.attach<TransformNode> (Vector3<float> { 5.0f, 0.0f, 0.0f });
    scene.update (0.0);
    EXPECT_EQ (deform.getDeformedMesh()->getChangeCount(), count);

    deform.setWeights (std::vector<float> { 0.5f, 0.5f });
    scene.update (0.0);
    EXPECT_EQ (deform.getDeformedMesh()->getChangeCount(), count + 1);
}

TEST_F (DeformNodeTests, MoreWeightsThanTargetsAreIgnoredAndFewerLeaveTheRest)
{
    const auto mesh = makeMorphedStrip();
    auto& deform = scene.createEntity ("strip").attach<DeformNode> (mesh);

    deform.setWeights (std::vector<float> { 0.25f, 0.5f, 9.0f });
    ASSERT_EQ (deform.getWeights().size(), 2u);
    EXPECT_FLOAT_EQ (deform.getWeights()[0], 0.25f);
    EXPECT_FLOAT_EQ (deform.getWeights()[1], 0.5f);

    deform.setWeights (std::vector<float> { 1.0f });
    EXPECT_FLOAT_EQ (deform.getWeights()[0], 1.0f);
    EXPECT_FLOAT_EQ (deform.getWeights()[1], 0.5f);

    deform.setWeights (std::vector<float>());
    EXPECT_FLOAT_EQ (deform.getWeights()[0], 1.0f);
    EXPECT_FLOAT_EQ (deform.getWeights()[1], 0.5f);
}

TEST_F (DeformNodeTests, WeightsStartAsTheMeshHasThemAndShowAtOnce)
{
    const auto mesh = makeMorphedStrip();
    mesh->setMorphWeights ({ 0.5f });

    auto entity = scene.createEntity ("strip");
    const auto& deform = entity.attach<DeformNode> (mesh);

    // A target the mesh gives no weight starts at 0
    ASSERT_EQ (deform.getWeights().size(), 2u);
    EXPECT_FLOAT_EQ (deform.getWeights()[0], 0.5f);
    EXPECT_FLOAT_EQ (deform.getWeights()[1], 0.0f);

    // Attached, the node has deformed the mesh: no update is needed to see it
    expectNear (deform.getDeformedMesh()->getPrimitive (0).vertices[2].position, { 0.0f, 2.5f, 0.0f });
}

TEST_F (DeformNodeTests, AWeightsTrackSetsTheWeightsWhenItPosesTheEntity)
{
    const auto mesh = makeMorphedStrip();
    auto entity = scene.createEntity ("strip");
    auto& deform = entity.attach<DeformNode> (mesh);
    const auto& vertices = deform.getDeformedMesh()->getPrimitive (0).vertices;

    // In 2 seconds the first weight goes from 0 to 1 and the second from 0 to 0.5
    const auto clip = makeClip (AnimationClip::Path::weights, { 0.0f, 2.0f }, { 0.0f, 0.0f, 1.0f, 0.5f });
    auto& player = scene.createEntity ("player").attach<AnimationNode> (std::vector<AnimationClip::Ptr> { clip }, std::vector<Entity> { entity });

    // Halfway, with weights of 0.5 and 0.25: the top is lifted by 0.5 and moved by 0.125 along x
    player.setPlaying (true);
    scene.update (1.0);

    EXPECT_FLOAT_EQ (deform.getWeights()[0], 0.5f);
    EXPECT_FLOAT_EQ (deform.getWeights()[1], 0.25f);
    expectNear (vertices[2].position, { 0.125f, 2.5f, 0.0f });

    // Paused, the clip poses nothing: the weights are the ones code sets
    player.setPlaying (false);
    deform.setWeights (std::vector<float> { 0.0f, 0.0f });
    scene.update (1.0);

    EXPECT_FLOAT_EQ (deform.getWeights()[0], 0.0f);
    expectNear (vertices[2].position, { 0.0f, 2.0f, 0.0f });

    // Scrubbed to its end, it sets them at once, and the mesh follows at the next update
    player.setTime (2.0);
    EXPECT_FLOAT_EQ (deform.getWeights()[0], 1.0f);
    EXPECT_FLOAT_EQ (deform.getWeights()[1], 0.5f);

    scene.update (0.0);
    expectNear (vertices[2].position, { 0.25f, 3.0f, 0.0f });
}
