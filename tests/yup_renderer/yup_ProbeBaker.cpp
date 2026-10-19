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

#include <yup_renderer/yup_renderer.h>

#include <gtest/gtest.h>

using namespace yup;

class ProbeBakerTests : public ::testing::Test
{
protected:
    /** Adds a spherical probe at (1, 2, 3), the entity "probe", the first time, then builds the list. */
    void build()
    {
        if (! scene.getRoot().findChild ("probe").isValid())
            addProbe();

        list.build (scene.getRoot(), Matrix4());
    }

    void addProbe()
    {
        auto entity = scene.createEntity ("probe");
        entity.attach<ReflectionProbeNode>();
        entity.attach<TransformNode> (Vector3<float> (1.0f, 2.0f, 3.0f));
    }

    ReflectionProbeNode& node() { return *scene.getRoot().findChild ("probe").get<ReflectionProbeNode>(); }

    /** Adds a counter-clockwise quad facing @p normal, centered on @p normal times @p extent. */
    static void addQuad (std::vector<Mesh::Vertex>& vertices, Vector3<float> normal, Vector3<float> u, Vector3<float> v, float extent)
    {
        if (u.crossProduct (v).dotProduct (normal) < 0.0f)
            std::swap (u, v);

        const auto center = normal * extent;
        const Vector3<float> corners[] = { center - (u + v) * extent, center + (u - v) * extent, center + (u + v) * extent, center - (u - v) * extent };
        for (const int corner : { 0, 1, 2, 0, 2, 3 })
            vertices.push_back ({ corners[corner] });
    }

    /** A cube from -1 to 1 with its faces pointing outwards. */
    static Mesh::Ptr makeCube()
    {
        const Vector3<float> x { 1.0f, 0.0f, 0.0f }, y { 0.0f, 1.0f, 0.0f }, z { 0.0f, 0.0f, 1.0f };
        std::vector<Mesh::Vertex> vertices;
        addQuad (vertices, x, y, z, 1.0f);
        addQuad (vertices, -x, y, z, 1.0f);
        addQuad (vertices, y, z, x, 1.0f);
        addQuad (vertices, -y, z, x, 1.0f);
        addQuad (vertices, z, x, y, 1.0f);
        addQuad (vertices, -z, x, y, 1.0f);

        auto mesh = Mesh::Ptr (new Mesh ("cube"));
        mesh->addPrimitive (std::move (vertices), {});
        return mesh;
    }

    static SceneDrawList::DrawItem makeItem (const Mesh& mesh, const Material& material, const Matrix4& world = {}, bool mirrored = false)
    {
        SceneDrawList::DrawItem item;
        item.primitive = &mesh.getPrimitive (0);
        item.material = &material;
        item.world = world;
        item.mirrored = mirrored;
        return item;
    }

    Scene scene;
    SceneDrawList list;
};

TEST_F (ProbeBakerTests, SignatureFollowsEveryFieldAndTheWorldMatrix)
{
    build();
    ASSERT_EQ (list.getReflectionProbes().size(), 1u);
    const auto original = ProbeBaker::getSignature (list.getReflectionProbes()[0]);

    // The bake id is the key of a bake, not part of its signature
    node().requestBake();
    build();
    EXPECT_EQ (ProbeBaker::getSignature (list.getReflectionProbes()[0]), original);

    const std::pair<const char*, std::function<void()>> changes[] = {
        { "shape", [this] { node().shape = ReflectionProbeNode::Shape::box; } },
        { "influenceRadius", [this] { node().influenceRadius = 6.0f; } },
        { "influenceExtents", [this] { node().influenceExtents = { 5.0f, 6.0f, 5.0f }; } },
        { "falloff", [this] { node().falloff = 1.0f; } },
        { "parallaxCorrection", [this] { node().parallaxCorrection = false; } },
        { "intensity", [this] { node().intensity = 2.0f; } },
        { "clipNear", [this] { node().clipNear = 0.5f; } },
        { "position", [this] { scene.getRoot().findChild ("probe").get<TransformNode>()->setPosition ({ 1.0f, 2.0f, 4.0f }); } },
    };

    auto previous = original;
    for (const auto& [name, change] : changes)
    {
        change();
        build();
        const auto signature = ProbeBaker::getSignature (list.getReflectionProbes()[0]);
        EXPECT_NE (signature, previous) << name;
        previous = signature;
    }
}

TEST_F (ProbeBakerTests, UnbakedProbesArePendingAndWeighNothing)
{
    build();

    ProbeRenderer probes (nullptr);
    ProbeBaker baker (probes);
    EXPECT_EQ (baker.getNumPending (list), 1);
    EXPECT_EQ (baker.getReflectionAtlas(), nullptr);
    EXPECT_EQ (baker.getNumBakedProbes(), 0);
    EXPECT_EQ (baker.getCaptureShadowRenderer(), nullptr);

    // Packed in list order for raster, all zero: an empty influence that weighs nothing
    const auto packed = baker.packReflectionProbes (list);
    ASSERT_EQ (packed.size(), 1u);
    const GpuReflectionProbe empty {};
    EXPECT_EQ (std::memcmp (&packed[0], &empty, sizeof (empty)), 0);

    // The full tier packs baked probes only
    EXPECT_TRUE (baker.packReflectionProbesByPriority (list).empty());
}

TEST_F (ProbeBakerTests, VolumeSignatureFollowsEveryFieldAndTheWorldMatrix)
{
    auto entity = scene.createEntity ("volume");
    auto& volume = entity.attach<IrradianceVolumeNode>();
    auto& transform = entity.attach<TransformNode> (Vector3<float> (1.0f, 2.0f, 3.0f));

    const auto signature = [this]
    {
        list.build (scene.getRoot(), Matrix4());
        return list.getIrradianceVolumes().size() == 1 ? ProbeBaker::getSignature (list.getIrradianceVolumes()[0]) : 0;
    };

    const auto original = signature();
    ASSERT_NE (original, 0u);

    // The bake id is the key of a bake, not part of its signature
    volume.requestBake();
    EXPECT_EQ (signature(), original);

    const std::pair<const char*, std::function<void()>> changes[] = {
        { "extents", [&] { volume.extents = { 5.0f, 3.0f, 5.0f }; } },
        { "resolutionX", [&] { volume.resolutionX = 5; } },
        { "resolutionY", [&] { volume.resolutionY = 3; } },
        { "resolutionZ", [&] { volume.resolutionZ = 6; } },
        { "falloff", [&] { volume.falloff = 1.0f; } },
        { "intensity", [&] { volume.intensity = 2.0f; } },
        { "normalBias", [&] { volume.normalBias = 0.2f; } },
        { "position", [&] { transform.setPosition ({ 1.0f, 2.0f, 4.0f }); } },
    };

    auto previous = original;
    for (const auto& [name, change] : changes)
    {
        change();
        const auto changed = signature();
        EXPECT_NE (changed, previous) << name;
        previous = changed;
    }
}

TEST_F (ProbeBakerTests, UnbakedVolumesArePendingAndWeighNothing)
{
    scene.getRoot().attach<IrradianceVolumeNode>();
    list.build (scene.getRoot(), Matrix4());
    ASSERT_EQ (list.getIrradianceVolumes().size(), 1u);

    ProbeRenderer probes (nullptr);
    ProbeBaker baker (probes);
    EXPECT_EQ (baker.getNumPending (list), 1);
    EXPECT_EQ (baker.getIrradianceAtlas(), nullptr);
    EXPECT_EQ (baker.getVisibilityAtlas(), nullptr);
    EXPECT_EQ (baker.getNumBakedVolumes(), 0);

    // Packed in list order for raster, all zero: a volume that weighs nothing
    const auto packed = baker.packIrradianceVolumes (list);
    ASSERT_EQ (packed.size(), 1u);
    const GpuIrradianceVolume empty {};
    EXPECT_EQ (std::memcmp (&packed[0], &empty, sizeof (empty)), 0);

    // The full tier packs baked volumes only
    EXPECT_TRUE (baker.packIrradianceVolumesByPriority (list).empty());
}

TEST_F (ProbeBakerTests, ProbesInsideSingleSidedGeometryAreInvalid)
{
    const auto cube = makeCube();
    Material material;
    const SceneDrawList::DrawItem items[] = { makeItem (*cube, material, Matrix4::translation ({ 10.0f, 0.0f, 0.0f })) };

    EXPECT_EQ (ProbeBaker::getVolumeProbeValidity (items, { 10.0f, 0.2f, -0.3f }, 100.0f), 0.0f);
    EXPECT_EQ (ProbeBaker::getVolumeProbeValidity (items, { 7.0f, 0.0f, 0.0f }, 100.0f), 1.0f);
    EXPECT_EQ (ProbeBaker::getVolumeProbeValidity (items, {}, 100.0f), 1.0f);
}

TEST_F (ProbeBakerTests, ProbeValidityFollowsMirroredItems)
{
    const auto cube = makeCube();
    Material material;
    const SceneDrawList::DrawItem items[] = { makeItem (*cube, material, Matrix4::scaling ({ -2.0f, 1.0f, 1.0f }), true) };

    EXPECT_EQ (ProbeBaker::getVolumeProbeValidity (items, { 1.5f, 0.0f, 0.0f }, 100.0f), 0.0f);
    EXPECT_EQ (ProbeBaker::getVolumeProbeValidity (items, { 3.0f, 0.0f, 0.0f }, 100.0f), 1.0f);
}

TEST_F (ProbeBakerTests, DoubleSidedGeometryKeepsProbesValid)
{
    const auto cube = makeCube();
    Material material;
    material.doubleSided = true;
    const SceneDrawList::DrawItem items[] = { makeItem (*cube, material) };

    EXPECT_EQ (ProbeBaker::getVolumeProbeValidity (items, {}, 100.0f), 1.0f);
}

TEST_F (ProbeBakerTests, ProbesBelowAFloorAreInvalidAndOutOfReachOnesValid)
{
    std::vector<Mesh::Vertex> vertices;
    addQuad (vertices, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, 1.0f);
    for (auto& vertex : vertices)
        vertex.position = { vertex.position.getX() * 100.0f, 0.0f, vertex.position.getZ() * 100.0f };

    auto floor = Mesh::Ptr (new Mesh ("floor"));
    floor->addPrimitive (std::move (vertices), {});
    Material material;
    const SceneDrawList::DrawItem items[] = { makeItem (*floor, material) };

    EXPECT_EQ (ProbeBaker::getVolumeProbeValidity (items, { 0.0f, -0.5f, 0.0f }, 50.0f), 0.0f);
    EXPECT_EQ (ProbeBaker::getVolumeProbeValidity (items, { 0.0f, 0.5f, 0.0f }, 50.0f), 1.0f);
    EXPECT_EQ (ProbeBaker::getVolumeProbeValidity (items, { 0.0f, -0.5f, 0.0f }, 0.25f), 1.0f);
    EXPECT_EQ (ProbeBaker::getVolumeProbeValidity ({}, {}, 50.0f), 1.0f);
}
