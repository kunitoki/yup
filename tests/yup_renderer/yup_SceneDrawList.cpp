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

#include <yup_renderer/yup_renderer.h>

using namespace yup;

class SceneDrawListTests : public ::testing::Test
{
protected:
    Entity makeEntity (const String& name = {})
    {
        return scene.createEntity (name);
    }

    static Material::Ptr makeMaterial (const String& name, Material::AlphaMode alphaMode = Material::AlphaMode::opaque, bool doubleSided = false)
    {
        auto material = Material::Ptr (new Material());
        material->name = name;
        material->alphaMode = alphaMode;
        material->doubleSided = doubleSided;
        return material;
    }

    static Mesh::Ptr makeTriangle (Material::Ptr material = nullptr)
    {
        auto mesh = Mesh::Ptr (new Mesh());

        std::vector<Mesh::Vertex> vertices (3);
        vertices[0].position = { 0.0f, 0.0f, 0.0f };
        vertices[1].position = { 1.0f, 0.0f, 0.0f };
        vertices[2].position = { 0.0f, 1.0f, 0.0f };
        mesh->addPrimitive (vertices, {}, std::move (material));

        return mesh;
    }

    static void expectNear (const Vector3<float>& actual, const Vector3<float>& expected, float tolerance = 1.0e-5f)
    {
        EXPECT_NEAR (actual.getX(), expected.getX(), tolerance);
        EXPECT_NEAR (actual.getY(), expected.getY(), tolerance);
        EXPECT_NEAR (actual.getZ(), expected.getZ(), tolerance);
    }

    Scene scene;
    Entity root = scene.getRoot();
    SceneDrawList drawList;
};

TEST_F (SceneDrawListTests, EmptyTreeHasNothingToDraw)
{
    drawList.build (root, {});

    EXPECT_TRUE (drawList.getOpaqueItems().empty());
    EXPECT_TRUE (drawList.getBlendItems().empty());
    EXPECT_TRUE (drawList.getLights().empty());
}

TEST_F (SceneDrawListTests, WorldMatricesAccumulate)
{
    auto child = makeEntity();
    auto grandChild = child.createChild();

    root.getOrAttach<TransformNode>().setPosition ({ 1.0f, 0.0f, 0.0f });
    child.getOrAttach<TransformNode>().setScale ({ 2.0f, 2.0f, 2.0f });
    grandChild.getOrAttach<TransformNode>().setPosition ({ 0.0f, 1.0f, 0.0f });
    grandChild.attach<MeshNode> (makeTriangle());

    drawList.build (root, {});

    ASSERT_EQ (drawList.getOpaqueItems().size(), 1u);
    expectNear (drawList.getOpaqueItems()[0].world.transformPoint ({}), { 1.0f, 2.0f, 0.0f });
    EXPECT_TRUE (drawList.getOpaqueItems()[0].world.approximatelyEqualTo (grandChild.getWorldMatrix()));
}

TEST_F (SceneDrawListTests, HiddenSubtreesAreSkipped)
{
    auto hidden = makeEntity();
    auto hiddenChild = hidden.createChild();
    auto visible = makeEntity();

    hiddenChild.attach<MeshNode> (makeTriangle());
    hiddenChild.attach<LightNode>();
    visible.attach<MeshNode> (makeTriangle());
    hidden.setVisible (false);

    drawList.build (root, {});

    EXPECT_EQ (drawList.getOpaqueItems().size(), 1u);
    EXPECT_TRUE (drawList.getLights().empty());
}

TEST_F (SceneDrawListTests, EveryMeshNodeAndPrimitiveIsDrawn)
{
    auto mesh = makeTriangle();
    std::vector<Mesh::Vertex> vertices (3);
    vertices[1].position = { 1.0f, 0.0f, 0.0f };
    vertices[2].position = { 0.0f, 0.0f, 1.0f };
    mesh->addPrimitive (vertices, {});

    root.attach<MeshNode> (mesh);
    root.createChild().attach<MeshNode> (makeTriangle());
    root.createChild().attach<MeshNode>();

    drawList.build (root, {});

    ASSERT_EQ (drawList.getOpaqueItems().size(), 3u);

    int secondPrimitiveCount = 0;
    for (const auto& item : drawList.getOpaqueItems())
    {
        EXPECT_EQ (item.primitive, &item.mesh->getPrimitive (item.primitiveIndex));

        if (item.primitiveIndex == 1)
            ++secondPrimitiveCount;
    }

    EXPECT_EQ (secondPrimitiveCount, 1);
}

TEST_F (SceneDrawListTests, MaterialFallsBackToTheDefault)
{
    auto own = makeMaterial ("own");
    auto withMaterial = makeEntity();
    auto withoutMaterial = makeEntity();
    withMaterial.attach<MeshNode> (makeTriangle (own));
    withoutMaterial.attach<MeshNode> (makeTriangle());

    drawList.build (root, {});

    ASSERT_EQ (drawList.getOpaqueItems().size(), 2u);

    std::vector<const Material*> materials;
    for (const auto& item : drawList.getOpaqueItems())
        materials.push_back (item.material);

    EXPECT_NE (std::find (materials.begin(), materials.end(), own.get()), materials.end());
    EXPECT_NE (std::find (materials.begin(), materials.end(), &drawList.getDefaultMaterial()), materials.end());
}

TEST_F (SceneDrawListTests, NearestMaterialOverrideWins)
{
    auto outer = makeMaterial ("outer");
    auto inner = makeMaterial ("inner");
    auto own = makeMaterial ("own");

    auto group = makeEntity();
    auto leafA = group.createChild ("a");
    auto subGroup = group.createChild();
    auto leafB = subGroup.createChild ("b");

    group.attach<MaterialNode> (outer);
    subGroup.attach<MaterialNode> (inner);
    leafA.attach<MeshNode> (makeTriangle (own));
    leafB.attach<MeshNode> (makeTriangle (own));

    drawList.build (root, {});

    ASSERT_EQ (drawList.getOpaqueItems().size(), 2u);

    for (const auto& item : drawList.getOpaqueItems())
    {
        if (item.world.approximatelyEqualTo (leafA.getWorldMatrix()) && item.mesh == leafA.get<MeshNode>()->mesh.get())
            EXPECT_EQ (item.material, outer.get());
        else
            EXPECT_EQ (item.material, inner.get());
    }
}

TEST_F (SceneDrawListTests, AnOverrideWithoutMaterialInheritsTheParentOne)
{
    auto outer = makeMaterial ("outer");
    auto child = makeEntity();
    root.attach<MaterialNode> (outer);
    child.attach<MaterialNode>();
    child.attach<MeshNode> (makeTriangle (makeMaterial ("own")));

    drawList.build (root, {});

    ASSERT_EQ (drawList.getOpaqueItems().size(), 1u);
    EXPECT_EQ (drawList.getOpaqueItems()[0].material, outer.get());
}

TEST_F (SceneDrawListTests, OverridesAndVisibilityAreReadAgainOnEachBuild)
{
    auto first = makeMaterial ("first");
    auto second = makeMaterial ("second");

    auto group = makeEntity();
    auto leaf = group.createChild();
    auto& materialNode = group.attach<MaterialNode> (first);
    leaf.attach<MeshNode> (makeTriangle (makeMaterial ("own")));

    drawList.build (root, {});
    ASSERT_EQ (drawList.getOpaqueItems().size(), 1u);
    EXPECT_EQ (drawList.getOpaqueItems()[0].material, first.get());

    materialNode.material = second;
    drawList.build (root, {});
    ASSERT_EQ (drawList.getOpaqueItems().size(), 1u);
    EXPECT_EQ (drawList.getOpaqueItems()[0].material, second.get());

    group.setVisible (false);
    drawList.build (root, {});
    EXPECT_TRUE (drawList.getOpaqueItems().empty());
}

TEST_F (SceneDrawListTests, OnlyTheSubtreeOfTheBuildRootIsDrawn)
{
    auto outer = makeMaterial ("outer");
    auto group = makeEntity();
    auto leaf = group.createChild();
    auto sibling = makeEntity();

    root.attach<MaterialNode> (outer);
    leaf.attach<MeshNode> (makeTriangle());
    sibling.attach<MeshNode> (makeTriangle());

    drawList.build (group, {});

    ASSERT_EQ (drawList.getOpaqueItems().size(), 1u);
    EXPECT_EQ (drawList.getOpaqueItems()[0].node, leaf.get<MeshNode>());
    EXPECT_NE (drawList.getOpaqueItems()[0].material, outer.get());
}

TEST_F (SceneDrawListTests, BlendItemsAreSortedBackToFront)
{
    auto glass = makeMaterial ("glass", Material::AlphaMode::blend);
    const float depths[] = { -5.0f, -20.0f, -10.0f };

    for (auto z : depths)
    {
        auto entity = makeEntity (String (z));
        entity.getOrAttach<TransformNode>().setPosition ({ 0.0f, 0.0f, z });
        entity.attach<MeshNode> (makeTriangle (glass));
    }

    root.attach<MeshNode> (makeTriangle (makeMaterial ("solid")));

    // Camera at the origin looking down -Z
    drawList.build (root, Matrix4());

    EXPECT_EQ (drawList.getOpaqueItems().size(), 1u);
    ASSERT_EQ (drawList.getBlendItems().size(), 3u);

    EXPECT_NEAR (drawList.getBlendItems()[0].world.transformPoint ({}).getZ(), -20.0f, 1.0e-5f);
    EXPECT_NEAR (drawList.getBlendItems()[1].world.transformPoint ({}).getZ(), -10.0f, 1.0e-5f);
    EXPECT_NEAR (drawList.getBlendItems()[2].world.transformPoint ({}).getZ(), -5.0f, 1.0e-5f);
    EXPECT_GT (drawList.getBlendItems()[0].viewDistance, drawList.getBlendItems()[2].viewDistance);
}

TEST_F (SceneDrawListTests, OpaqueItemsAreGroupedByPipelineThenMaterial)
{
    auto singleA = makeMaterial ("singleA");
    auto singleB = makeMaterial ("singleB", Material::AlphaMode::mask);
    auto doubleA = makeMaterial ("doubleA", Material::AlphaMode::opaque, true);

    const Material::Ptr order[] = { doubleA, singleA, singleB, doubleA, singleA, singleB };
    for (const auto& material : order)
    {
        auto entity = makeEntity();
        entity.attach<MeshNode> (makeTriangle (material));
    }

    drawList.build (root, {});

    const auto items = drawList.getOpaqueItems();
    ASSERT_EQ (items.size(), 6u);

    // Single-sided first, double-sided last, materials contiguous within each group
    for (size_t i = 0; i < 4; ++i)
        EXPECT_FALSE (items[i].material->doubleSided);

    EXPECT_TRUE (items[4].material->doubleSided);
    EXPECT_TRUE (items[5].material->doubleSided);
    EXPECT_EQ (items[0].material, items[1].material);
    EXPECT_EQ (items[2].material, items[3].material);
    EXPECT_NE (items[1].material, items[2].material);
}

TEST_F (SceneDrawListTests, OpaqueItemsOfTheSameMaterialKeepTheOrderTheyWereCollectedIn)
{
    const Material::Ptr materials[] = { makeMaterial ("a"), makeMaterial ("b"), makeMaterial ("c", Material::AlphaMode::opaque, true) };

    // More of them than a sort orders by insertion, their materials interleaved, at various depths
    std::vector<Entity> entities;
    for (int i = 0; i < 300; ++i)
    {
        auto entity = entities.emplace_back (makeEntity());
        entity.getOrAttach<TransformNode>().setPosition ({ static_cast<float> (i % 7), 0.0f, static_cast<float> (i % 5) });
        entity.attach<MeshNode> (makeTriangle (materials[i % 3]));
    }

    // Whatever the camera sees them from: the shadow maps tell their casters by their place in the list
    for (const auto& view : { Matrix4(), Matrix4::lookAt ({ 3.0f, 2.0f, 1.0f }, {}, { 0.0f, 1.0f, 0.0f }) })
    {
        drawList.build (root, view);

        const auto items = drawList.getOpaqueItems();
        ASSERT_EQ (items.size(), entities.size());

        for (size_t material = 0; material < 3; ++material)
        {
            std::vector<const MeshNode*> drawn, attached;

            for (const auto& item : items)
                if (item.material == materials[material].get())
                    drawn.push_back (item.node);

            for (size_t i = material; i < entities.size(); i += 3)
                attached.push_back (entities[i].get<MeshNode>());

            EXPECT_EQ (drawn, attached) << material;
        }
    }
}

TEST_F (SceneDrawListTests, LightsAreGatheredInWorldSpace)
{
    auto lamp = makeEntity();
    lamp.getOrAttach<TransformNode>().setPosition ({ 1.0f, 2.0f, 3.0f });
    lamp.getOrAttach<TransformNode>().setRotation (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, MathConstants<float>::halfPi));
    lamp.attach<LightNode> (LightNode::Type::point);
    lamp.createChild().attach<LightNode> (LightNode::Type::spot);

    drawList.build (root, {});

    ASSERT_EQ (drawList.getLights().size(), 2u);
    expectNear (drawList.getLights()[0].position, { 1.0f, 2.0f, 3.0f });

    // -Z turned 90 degrees around +Y points down -X
    expectNear (drawList.getLights()[0].direction, { -1.0f, 0.0f, 0.0f });
    EXPECT_EQ (drawList.getLights()[0].node->type, LightNode::Type::point);
    EXPECT_EQ (drawList.getLights()[1].node->type, LightNode::Type::spot);
}

TEST_F (SceneDrawListTests, MirroringTransformsAreFlagged)
{
    auto mirrored = makeEntity();
    auto doubleMirrored = mirrored.createChild();

    mirrored.getOrAttach<TransformNode>().setScale ({ -1.0f, 1.0f, 1.0f });
    doubleMirrored.getOrAttach<TransformNode>().setScale ({ 1.0f, -2.0f, 1.0f });
    root.attach<MeshNode> (makeTriangle());
    mirrored.attach<MeshNode> (makeTriangle());
    doubleMirrored.attach<MeshNode> (makeTriangle());

    drawList.build (root, {});

    ASSERT_EQ (drawList.getOpaqueItems().size(), 3u);

    int mirroredCount = 0;
    for (const auto& item : drawList.getOpaqueItems())
    {
        if (item.mirrored)
        {
            ++mirroredCount;
            EXPECT_TRUE (item.world.approximatelyEqualTo (mirrored.getWorldMatrix()));
        }
    }

    EXPECT_EQ (mirroredCount, 1);
}

TEST_F (SceneDrawListTests, RebuildingReplacesThePreviousItems)
{
    root.attach<MeshNode> (makeTriangle());
    drawList.build (root, {});
    drawList.build (root, {});

    EXPECT_EQ (drawList.getOpaqueItems().size(), 1u);

    drawList.clear();
    EXPECT_TRUE (drawList.getOpaqueItems().empty());
}

TEST_F (SceneDrawListTests, ItemsPointToTheirMeshNode)
{
    auto first = makeEntity();
    auto second = makeEntity();

    auto& firstNode = first.attach<MeshNode> (makeTriangle());
    auto& transparentNode = second.attach<MeshNode> (makeTriangle (makeMaterial ("glass", Material::AlphaMode::blend)));

    drawList.build (root, {});

    ASSERT_EQ (drawList.getOpaqueItems().size(), 1u);
    ASSERT_EQ (drawList.getBlendItems().size(), 1u);
    EXPECT_EQ (drawList.getOpaqueItems()[0].node, &firstNode);
    EXPECT_EQ (drawList.getBlendItems()[0].node, &transparentNode);
}

TEST_F (SceneDrawListTests, NewMeshNodesHaveNoPreviousStamp)
{
    const MeshNode node (makeTriangle());

    EXPECT_EQ (node.previousStamp, 0u);
    EXPECT_TRUE (node.previousWorld.approximatelyEqualTo (Matrix4()));
}

TEST_F (SceneDrawListTests, DefaultLightIsAddedOnlyWhenAskedAndTreeHasNone)
{
    auto entity = makeEntity();
    entity.attach<MeshNode> (makeTriangle());

    drawList.build (root, {});
    EXPECT_TRUE (drawList.getLights().empty());

    drawList.build (root, {}, { true });
    ASSERT_EQ (drawList.getLights().size(), 1u);
    EXPECT_FALSE (drawList.getLights()[0].node->getEntity().isValid());
    EXPECT_EQ (drawList.getLights()[0].node->type, LightNode::Type::directional);
    EXPECT_EQ (drawList.getLights()[0].node->color, (std::array<float, 3> { 1.0f, 1.0f, 1.0f }));
    EXPECT_FLOAT_EQ (drawList.getLights()[0].node->intensity, 3.0f);
    EXPECT_FALSE (drawList.getLights()[0].node->castsShadows);
    expectNear (drawList.getLights()[0].direction, Vector3<float> (-0.35f, -0.75f, -0.55f).normalized());

    auto lamp = makeEntity();
    lamp.attach<LightNode> (LightNode::Type::point);

    drawList.build (root, {}, { true });
    ASSERT_EQ (drawList.getLights().size(), 1u);
    EXPECT_EQ (drawList.getLights()[0].node->type, LightNode::Type::point);
}

class SceneDrawListSelectionTests : public SceneDrawListTests
{
protected:
    static SceneDrawList::Light makeLight (const LightNode& node, Vector3<float> position)
    {
        return { &node, position, { 0.0f, 0.0f, -1.0f } };
    }

    static BoundingBox unitBoxAt (Vector3<float> center)
    {
        BoundingBox box;
        box.expand (center - Vector3<float> (0.5f, 0.5f, 0.5f));
        box.expand (center + Vector3<float> (0.5f, 0.5f, 0.5f));
        return box;
    }

    /** Adds a point light @p distance away from the triangle of addTriangle(), and returns its entity. */
    Entity addPointLight (float distance, float range = 0.0f)
    {
        auto entity = makeEntity();
        entity.attach<LightNode> (LightNode::Type::point).range = range;
        entity.getOrAttach<TransformNode>().setPosition ({ 0.5f, 0.5f, distance });
        return entity;
    }

    /** Adds the triangle of makeTriangle(), in the z = 0 plane. */
    void addTriangle()
    {
        auto entity = makeEntity();
        entity.attach<MeshNode> (makeTriangle());
    }

    /** Builds the list and returns the weight of every light on its only item, 0 for a light it does not select. */
    std::vector<float> buildEffectiveWeights()
    {
        drawList.build (root, {}, { false, true });

        std::vector<float> weights (drawList.getLights().size(), 0.0f);
        const auto& item = drawList.getOpaqueItems()[0];
        for (size_t i = 0; i < static_cast<size_t> (item.numLights); ++i)
            weights[item.lightIndices[i]] = item.lightWeights[i];

        return weights;
    }

    /** Sweeps a ninth light from 6 to 1.5 units towards the triangle, past 8 lights from 2 to 3.4
        units away, and returns the largest change of the weight of any light between two steps
        of 0.01.
    */
    float sweepNinthLight (float range)
    {
        addTriangle();
        for (int i = 0; i < 8; ++i)
            addPointLight (2.0f + 0.2f * static_cast<float> (i));

        auto sweeping = addPointLight (6.0f, range);

        std::vector<float> previous;
        auto largestStep = 0.0f;
        for (int step = 0; step <= 450; ++step)
        {
            sweeping.getOrAttach<TransformNode>().setPosition ({ 0.5f, 0.5f, 6.0f - 0.01f * static_cast<float> (step) });

            const auto weights = buildEffectiveWeights();
            for (size_t i = 0; i < previous.size(); ++i)
                largestStep = jmax (largestStep, std::abs (weights[i] - previous[i]));

            previous = weights;
        }

        return largestStep;
    }
};

TEST_F (SceneDrawListSelectionTests, PicksTheEightMostInfluentialLights)
{
    std::vector<std::unique_ptr<LightNode>> nodes;
    std::vector<SceneDrawList::Light> lights;

    for (int i = 0; i < 12; ++i)
    {
        auto& node = *nodes.emplace_back (std::make_unique<LightNode> (LightNode::Type::point));
        node.intensity = 1.0f;
        lights.push_back (makeLight (node, { 1.0f + static_cast<float> (i), 0.0f, 0.0f })); // farther and farther
    }

    std::array<uint16, SceneDrawList::maxLightsPerDraw> indices {};
    const auto count = SceneDrawList::selectLights (unitBoxAt ({}), lights, indices);

    ASSERT_EQ (count, 8);
    for (int i = 0; i < 8; ++i)
        EXPECT_EQ (indices[static_cast<size_t> (i)], i);
}

TEST_F (SceneDrawListSelectionTests, PicksTheEightMostInfluentialLightsListedLast)
{
    std::vector<std::unique_ptr<LightNode>> nodes;
    std::vector<SceneDrawList::Light> lights;

    // Nearer and nearer: every light from the ninth on evicts the least influential slot
    for (int i = 0; i < 12; ++i)
    {
        auto& node = *nodes.emplace_back (std::make_unique<LightNode> (LightNode::Type::point));
        lights.push_back (makeLight (node, { 12.0f - static_cast<float> (i), 0.0f, 0.0f }));
    }

    std::array<uint16, SceneDrawList::maxLightsPerDraw> indices {};
    const auto count = SceneDrawList::selectLights (unitBoxAt ({}), lights, indices);

    ASSERT_EQ (count, 8);
    for (int i = 0; i < 8; ++i)
        EXPECT_EQ (indices[static_cast<size_t> (i)], 11 - i);
}

TEST_F (SceneDrawListSelectionTests, DirectionalLightsComeFirst)
{
    LightNode nearLight (LightNode::Type::point), sun;
    nearLight.intensity = 1000.0f;
    sun.intensity = 0.1f;

    const SceneDrawList::Light lights[] = { makeLight (nearLight, { 0.6f, 0.0f, 0.0f }), makeLight (sun, {}) };

    std::array<uint16, SceneDrawList::maxLightsPerDraw> indices {};
    ASSERT_EQ (SceneDrawList::selectLights (unitBoxAt ({}), lights, indices), 2);
    EXPECT_EQ (indices[0], 1);
}

TEST_F (SceneDrawListSelectionTests, LightsOutOfRangeAreSkipped)
{
    LightNode point (LightNode::Type::point);
    point.range = 1.0f;

    const SceneDrawList::Light lights[] = { makeLight (point, { 3.0f, 0.0f, 0.0f }) };

    std::array<uint16, SceneDrawList::maxLightsPerDraw> indices {};
    EXPECT_EQ (SceneDrawList::selectLights (unitBoxAt ({}), lights, indices), 0);
}

TEST_F (SceneDrawListSelectionTests, SpotConeMissingTheBoundsIsSkipped)
{
    LightNode spot (LightNode::Type::spot);
    spot.range = 10.0f;
    spot.outerConeAngle = 0.2f;

    // At +X 3 units, pointing further +X: the box at the origin is behind it
    const SceneDrawList::Light lights[] = { { &spot, { 3.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } } };

    std::array<uint16, SceneDrawList::maxLightsPerDraw> indices {};
    EXPECT_EQ (SceneDrawList::selectLights (unitBoxAt ({}), lights, indices), 0);
}

TEST_F (SceneDrawListSelectionTests, SpotConeReachingTheBoundsIsSelected)
{
    LightNode spot (LightNode::Type::spot);
    spot.range = 10.0f;
    spot.outerConeAngle = 0.2f;

    // At +X 3 units, pointing back at the box at the origin
    const SceneDrawList::Light lights[] = { { &spot, { 3.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f } } };

    std::array<uint16, SceneDrawList::maxLightsPerDraw> indices {};
    ASSERT_EQ (SceneDrawList::selectLights (unitBoxAt ({}), lights, indices), 1);
    EXPECT_EQ (indices[0], 0);
}

TEST_F (SceneDrawListSelectionTests, TiesKeepTheLowerIndex)
{
    LightNode a (LightNode::Type::point), b (LightNode::Type::point);
    const SceneDrawList::Light lights[] = { makeLight (a, { 2.0f, 0.0f, 0.0f }), makeLight (b, { -2.0f, 0.0f, 0.0f }) };

    std::array<uint16, SceneDrawList::maxLightsPerDraw> indices {};
    ASSERT_EQ (SceneDrawList::selectLights (unitBoxAt ({}), lights, indices), 2);
    EXPECT_EQ (indices[0], 0);
    EXPECT_EQ (indices[1], 1);
}

TEST_F (SceneDrawListSelectionTests, DarkPointAndSpotLightsAreSkippedEvenInsideTheBounds)
{
    // Both pack an effective range of 0: a distance of 0 to the bounds must not keep them
    LightNode dark (LightNode::Type::point), black (LightNode::Type::spot), lit (LightNode::Type::point);
    dark.intensity = 0.0f;
    black.color = { 0.0f, 0.0f, 0.0f };

    const SceneDrawList::Light lights[] = { makeLight (dark, {}), makeLight (black, {}), makeLight (lit, {}) };

    std::array<uint16, SceneDrawList::maxLightsPerDraw> indices {};
    ASSERT_EQ (SceneDrawList::selectLights (unitBoxAt ({}), lights, indices), 1);
    EXPECT_EQ (indices[0], 2);
}

TEST_F (SceneDrawListSelectionTests, DarkLightsOfEveryTypeTakeNoSlot)
{
    // Dark directional lights would rank first and fill every slot; dark lights with a range would
    // reach the bounds. None of them may keep the lit point light out
    std::vector<std::unique_ptr<LightNode>> nodes;
    std::vector<SceneDrawList::Light> lights;

    for (int i = 0; i < 8; ++i)
    {
        auto& sun = *nodes.emplace_back (std::make_unique<LightNode>());
        if (i % 2 == 0)
            sun.intensity = 0.0f;
        else
            sun.color = { 0.0f, 0.0f, 0.0f };

        lights.push_back (makeLight (sun, {}));
    }

    auto& ranged = *nodes.emplace_back (std::make_unique<LightNode> (LightNode::Type::point));
    ranged.range = 5.0f;
    ranged.intensity = 0.0f;
    lights.push_back (makeLight (ranged, {}));

    auto& lit = *nodes.emplace_back (std::make_unique<LightNode> (LightNode::Type::point));
    lights.push_back (makeLight (lit, { 1.0f, 0.0f, 0.0f }));

    std::array<uint16, SceneDrawList::maxLightsPerDraw> indices {};
    ASSERT_EQ (SceneDrawList::selectLights (unitBoxAt ({}), lights, indices), 1);
    EXPECT_EQ (indices[0], 9);
}

TEST_F (SceneDrawListSelectionTests, EightLightsOrFewerKeepTheirFullIntensity)
{
    addTriangle();
    for (int i = 0; i < 8; ++i)
        addPointLight (2.0f + 0.01f * static_cast<float> (i));

    drawList.build (root, {}, { false, true });

    const auto& item = drawList.getOpaqueItems()[0];
    ASSERT_EQ (item.numLights, 8);
    for (const auto weight : item.lightWeights)
        EXPECT_EQ (weight, 1.0f);
}

TEST_F (SceneDrawListSelectionTests, LightsFadeOutTowardsTheFirstLightLeftOut)
{
    // Seven near lights, then two tied: the eighth slot ranks the same as the light left out
    addTriangle();
    for (int i = 0; i < 7; ++i)
        addPointLight (1.0f);

    addPointLight (5.0f);
    addPointLight (5.0f);

    const auto weights = buildEffectiveWeights();
    for (int i = 0; i < 7; ++i)
        EXPECT_EQ (weights[static_cast<size_t> (i)], 1.0f);

    EXPECT_EQ (weights[7], 0.0f);
    EXPECT_EQ (weights[8], 0.0f);
}

TEST_F (SceneDrawListSelectionTests, LightCrossingTheCutoffChangesWeightsInSmallSteps)
{
    EXPECT_LT (sweepNinthLight (0.0f), 0.05f);
}

TEST_F (SceneDrawListSelectionTests, LightEnteringItsRangeChangesWeightsInSmallSteps)
{
    // The light reaches the triangle from 4 units: it must rank 0 there, not 1 / 16
    EXPECT_LT (sweepNinthLight (4.0f), 0.05f);
}

TEST_F (SceneDrawListTests, DefaultLightIsSelectedForEveryDraw)
{
    auto entity = makeEntity();
    entity.attach<MeshNode> (makeTriangle());

    drawList.build (root, {}, { true, true });

    ASSERT_EQ (drawList.getOpaqueItems().size(), 1u);
    EXPECT_EQ (drawList.getOpaqueItems()[0].numLights, 1);
    EXPECT_EQ (drawList.getOpaqueItems()[0].lightIndices[0], 0);
}

TEST_F (SceneDrawListTests, BlendedItemsSelectLightsToo)
{
    auto entity = makeEntity();
    entity.attach<MeshNode> (makeTriangle (makeMaterial ("glass", Material::AlphaMode::blend)));
    entity.attach<LightNode> (LightNode::Type::point);

    drawList.build (root, {}, { false, true });

    ASSERT_EQ (drawList.getBlendItems().size(), 1u);
    EXPECT_EQ (drawList.getBlendItems()[0].numLights, 1);
    EXPECT_EQ (drawList.getBlendItems()[0].lightIndices[0], 0);
}

TEST_F (SceneDrawListTests, PerDrawSelectionIsSkippedUnlessAsked)
{
    auto entity = makeEntity();
    entity.attach<MeshNode> (makeTriangle());
    entity.attach<LightNode> (LightNode::Type::point);

    drawList.build (root, {}, { false, false });
    EXPECT_EQ (drawList.getOpaqueItems()[0].numLights, 0);
}

TEST_F (SceneDrawListTests, CollectsReflectionProbes)
{
    auto probeEntity = makeEntity ("probe");
    probeEntity.getOrAttach<TransformNode>().setPosition ({ 2.0f, 0.0f, 0.0f });
    auto& probe = probeEntity.attach<ReflectionProbeNode>();
    probe.influenceRadius = 3.0f;

    drawList.build (root, {});

    ASSERT_EQ (drawList.getReflectionProbes().size(), 1u);
    const auto& collected = drawList.getReflectionProbes()[0];
    EXPECT_EQ (collected.node, &probe);
    EXPECT_TRUE (collected.world.approximatelyEqualTo (probeEntity.getWorldMatrix()));
    expectNear (collected.position, { 2.0f, 0.0f, 0.0f });
    expectNear (collected.bounds.getMin(), { -1.0f, -3.0f, -3.0f });
    expectNear (collected.bounds.getMax(), { 5.0f, 3.0f, 3.0f });
}

TEST_F (SceneDrawListTests, BoxProbeBoundsFollowTheEntityTransform)
{
    auto probeEntity = makeEntity();
    probeEntity.getOrAttach<TransformNode>().setPosition ({ 1.0f, 0.0f, 0.0f });
    probeEntity.getOrAttach<TransformNode>().setRotation (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, MathConstants<float>::halfPi));
    auto& probe = probeEntity.attach<ReflectionProbeNode>();
    probe.shape = ReflectionProbeNode::Shape::box;
    probe.influenceExtents = { 2.0f, 1.0f, 0.5f };

    drawList.build (root, {});

    // Turned 90 degrees around +Y, the X extent lies along Z and the Z extent along X
    ASSERT_EQ (drawList.getReflectionProbes().size(), 1u);
    expectNear (drawList.getReflectionProbes()[0].bounds.getMin(), { 0.5f, -1.0f, -2.0f });
    expectNear (drawList.getReflectionProbes()[0].bounds.getMax(), { 1.5f, 1.0f, 2.0f });
}

TEST_F (SceneDrawListTests, SphereProbeBoundsIgnoreRotation)
{
    auto rotated = makeEntity();
    rotated.getOrAttach<TransformNode>().setRotation (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, MathConstants<float>::pi * 0.25f));
    rotated.attach<ReflectionProbeNode>().influenceRadius = 2.0f;

    auto scaled = makeEntity();
    scaled.getOrAttach<TransformNode>().setPosition ({ 10.0f, 0.0f, 0.0f });
    scaled.getOrAttach<TransformNode>().setScale ({ 2.0f, 1.0f, 1.0f });
    scaled.attach<ReflectionProbeNode>().influenceRadius = 2.0f;

    drawList.build (root, {});

    ASSERT_EQ (drawList.getReflectionProbes().size(), 2u);
    expectNear (drawList.getReflectionProbes()[0].bounds.getMin(), { -2.0f, -2.0f, -2.0f });
    expectNear (drawList.getReflectionProbes()[0].bounds.getMax(), { 2.0f, 2.0f, 2.0f });
    expectNear (drawList.getReflectionProbes()[1].bounds.getMin(), { 6.0f, -2.0f, -2.0f });
    expectNear (drawList.getReflectionProbes()[1].bounds.getMax(), { 14.0f, 2.0f, 2.0f });
}

TEST_F (SceneDrawListTests, ZeroScaleProbesAreSkipped)
{
    auto flat = makeEntity();
    flat.getOrAttach<TransformNode>().setRotation (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 0.3f));
    flat.getOrAttach<TransformNode>().setScale ({ 0.0f, 1.0f, 1.0f });
    flat.attach<ReflectionProbeNode>();

    auto collapsed = makeEntity();
    collapsed.getOrAttach<TransformNode>().setScale ({ 0.0f, 0.0f, 0.0f });
    collapsed.attach<ReflectionProbeNode>().shape = ReflectionProbeNode::Shape::box;

    drawList.build (root, {});
    EXPECT_TRUE (drawList.getReflectionProbes().empty());
}

TEST_F (SceneDrawListTests, ProbesWithoutInfluenceAreSkipped)
{
    auto sphere = makeEntity();
    sphere.attach<ReflectionProbeNode>().influenceRadius = 0.0f;

    auto box = makeEntity();
    auto& boxProbe = box.attach<ReflectionProbeNode>();
    boxProbe.shape = ReflectionProbeNode::Shape::box;
    boxProbe.influenceExtents = { 1.0f, -1.0f, 1.0f };

    drawList.build (root, {});
    EXPECT_TRUE (drawList.getReflectionProbes().empty());
}

TEST_F (SceneDrawListTests, RebuildingReplacesThePreviousProbes)
{
    root.attach<ReflectionProbeNode>();
    drawList.build (root, {});
    drawList.build (root, {});

    EXPECT_EQ (drawList.getReflectionProbes().size(), 1u);

    drawList.clear();
    EXPECT_TRUE (drawList.getReflectionProbes().empty());
}

TEST_F (SceneDrawListTests, SelectsTheTwoNearestOverlappingProbes)
{
    std::vector<SceneDrawList::ReflectionProbe> probes (4);
    const float xs[] = { 10.0f, 0.5f, 3.0f, -1.0f };
    for (size_t i = 0; i < probes.size(); ++i)
    {
        probes[i].position = { xs[i], 0.0f, 0.0f };
        probes[i].bounds = BoundingBox ({ xs[i] - 2.0f, -2.0f, -2.0f }, { xs[i] + 2.0f, 2.0f, 2.0f });
    }

    const auto picked = SceneDrawList::selectReflectionProbes (probes, BoundingBox ({ -0.5f, -0.5f, -0.5f }, { 0.5f, 0.5f, 0.5f }));
    EXPECT_EQ (picked[0], 1); // 0.5 away
    EXPECT_EQ (picked[1], 3); // 1.0 away; probe 2 at 3.0 is farther, probe 0 does not overlap
}

TEST_F (SceneDrawListTests, NoOverlappingProbeSelectsNone)
{
    std::vector<SceneDrawList::ReflectionProbe> probes (1);
    probes[0].position = { 10.0f, 0.0f, 0.0f };
    probes[0].bounds = BoundingBox ({ 9.0f, -1.0f, -1.0f }, { 11.0f, 1.0f, 1.0f });

    const auto picked = SceneDrawList::selectReflectionProbes (probes, BoundingBox ({ -1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f }));
    EXPECT_EQ (picked[0], -1);
    EXPECT_EQ (picked[1], -1);
}

TEST_F (SceneDrawListTests, SmallerProbesRankFirst)
{
    std::vector<SceneDrawList::ReflectionProbe> probes (2);
    probes[0].position = { 3.0f, 0.0f, 0.0f };
    probes[0].bounds = BoundingBox ({ -7.0f, -10.0f, -10.0f }, { 13.0f, 10.0f, 10.0f });
    probes[1].position = { 1.0f, 0.0f, 0.0f };
    probes[1].bounds = BoundingBox ({ 0.0f, -1.0f, -1.0f }, { 2.0f, 1.0f, 1.0f });

    const auto picked = SceneDrawList::selectReflectionProbes (probes, BoundingBox ({ -0.5f, -0.5f, -0.5f }, { 0.5f, 0.5f, 0.5f }));
    EXPECT_EQ (picked[0], 1);
    EXPECT_EQ (picked[1], 0);
}

TEST_F (SceneDrawListTests, NestedSmallProbeIsPickedFirst)
{
    auto large = makeEntity ("large");
    large.attach<ReflectionProbeNode>().influenceRadius = 10.0f;

    auto small = makeEntity ("small");
    small.attach<ReflectionProbeNode>().influenceRadius = 1.0f;

    auto entity = makeEntity();
    entity.getOrAttach<TransformNode>().setPosition ({ 0.5f, 0.0f, 0.0f });
    entity.getOrAttach<TransformNode>().setScale ({ 0.1f, 0.1f, 0.1f });
    entity.attach<MeshNode> (makeTriangle());

    drawList.build (root, {}, { false, true });

    // Both probes capture from the origin and the draw lies inside both influences
    ASSERT_EQ (drawList.getReflectionProbes().size(), 2u);
    ASSERT_EQ (drawList.getOpaqueItems().size(), 1u);
    EXPECT_EQ (drawList.getOpaqueItems()[0].reflectionProbes[0], 1);
    EXPECT_EQ (drawList.getOpaqueItems()[0].reflectionProbes[1], 0);
}

TEST_F (SceneDrawListTests, ProbeTiesFollowTheBakeId)
{
    ReflectionProbeNode first;
    ReflectionProbeNode second;
    first.requestBake(); // now newer than second

    std::vector<SceneDrawList::ReflectionProbe> probes (2);
    probes[0].node = &first;
    probes[1].node = &second;
    for (auto& probe : probes)
    {
        probe.position = { 1.0f, 0.0f, 0.0f };
        probe.bounds = BoundingBox ({ -1.0f, -2.0f, -2.0f }, { 3.0f, 2.0f, 2.0f });
    }

    const auto picked = SceneDrawList::selectReflectionProbes (probes, BoundingBox ({ -0.5f, -0.5f, -0.5f }, { 0.5f, 0.5f, 0.5f }));
    EXPECT_EQ (picked[0], 1);
    EXPECT_EQ (picked[1], 0);
}

TEST_F (SceneDrawListTests, ProbeTiesKeepTheLowerIndex)
{
    std::vector<SceneDrawList::ReflectionProbe> probes (3);
    for (auto& probe : probes)
    {
        probe.position = { 1.0f, 0.0f, 0.0f };
        probe.bounds = BoundingBox ({ -1.0f, -2.0f, -2.0f }, { 3.0f, 2.0f, 2.0f });
    }

    const auto picked = SceneDrawList::selectReflectionProbes (probes, BoundingBox ({ -0.5f, -0.5f, -0.5f }, { 0.5f, 0.5f, 0.5f }));
    EXPECT_EQ (picked[0], 0);
    EXPECT_EQ (picked[1], 1);
}

TEST_F (SceneDrawListTests, EmptyItemBoundsSelectNoProbe)
{
    std::vector<SceneDrawList::ReflectionProbe> probes (1);
    probes[0].bounds = BoundingBox ({ -1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f });

    const auto picked = SceneDrawList::selectReflectionProbes (probes, BoundingBox());
    EXPECT_EQ (picked[0], -1);
    EXPECT_EQ (picked[1], -1);
}

TEST_F (SceneDrawListTests, PerDrawSelectionPicksReflectionProbes)
{
    auto entity = makeEntity();
    entity.attach<MeshNode> (makeTriangle());

    auto probeEntity = makeEntity();
    probeEntity.getOrAttach<TransformNode>().setPosition ({ 0.5f, 0.0f, 0.0f });
    probeEntity.attach<ReflectionProbeNode>().influenceRadius = 2.0f;

    drawList.build (root, {}, { false, true });

    ASSERT_EQ (drawList.getOpaqueItems().size(), 1u);
    EXPECT_EQ (drawList.getOpaqueItems()[0].reflectionProbes[0], 0);
    EXPECT_EQ (drawList.getOpaqueItems()[0].reflectionProbes[1], -1);
}

TEST_F (SceneDrawListTests, ReflectionProbeSelectionIsSkippedUnlessAsked)
{
    auto entity = makeEntity();
    entity.attach<MeshNode> (makeTriangle());
    entity.attach<ReflectionProbeNode>();

    drawList.build (root, {}, { false, false });

    ASSERT_EQ (drawList.getReflectionProbes().size(), 1u);
    EXPECT_EQ (drawList.getOpaqueItems()[0].reflectionProbes[0], -1);
    EXPECT_EQ (drawList.getOpaqueItems()[0].reflectionProbes[1], -1);
}

TEST_F (SceneDrawListTests, CollectsIrradianceVolumes)
{
    auto volumeEntity = makeEntity ("volume");
    volumeEntity.getOrAttach<TransformNode>().setPosition ({ 2.0f, 0.0f, 0.0f });
    volumeEntity.getOrAttach<TransformNode>().setRotation (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, MathConstants<float>::halfPi));
    auto& volume = volumeEntity.attach<IrradianceVolumeNode>();
    volume.extents = { 2.0f, 1.0f, 0.5f };

    drawList.build (root, {});

    // Turned 90 degrees around +Y, the X extent lies along Z and the Z extent along X
    ASSERT_EQ (drawList.getIrradianceVolumes().size(), 1u);
    const auto& collected = drawList.getIrradianceVolumes()[0];
    EXPECT_EQ (collected.node, &volume);
    EXPECT_TRUE (collected.world.approximatelyEqualTo (volumeEntity.getWorldMatrix()));
    expectNear (collected.bounds.getMin(), { 1.5f, -1.0f, -2.0f });
    expectNear (collected.bounds.getMax(), { 2.5f, 1.0f, 2.0f });

    drawList.clear();
    EXPECT_TRUE (drawList.getIrradianceVolumes().empty());
}

TEST_F (SceneDrawListTests, EmptyOrFlattenedVolumesAreSkipped)
{
    auto empty = makeEntity();
    empty.attach<IrradianceVolumeNode>().extents = { 1.0f, 0.0f, 1.0f };

    auto flat = makeEntity();
    flat.getOrAttach<TransformNode>().setScale ({ 1.0f, 0.0f, 1.0f });
    flat.attach<IrradianceVolumeNode>();

    drawList.build (root, {});
    EXPECT_TRUE (drawList.getIrradianceVolumes().empty());
}

TEST_F (SceneDrawListTests, SelectsTheSmallestVolumeContainingTheItem)
{
    std::vector<SceneDrawList::IrradianceVolume> volumes (2);
    volumes[0].bounds = BoundingBox ({ -10.0f, -10.0f, -10.0f }, { 10.0f, 10.0f, 10.0f });
    volumes[1].bounds = BoundingBox ({ -1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f });

    EXPECT_EQ (SceneDrawList::selectIrradianceVolume (volumes, BoundingBox ({ 0, 0, 0 }, { 0.2f, 0.2f, 0.2f })), 1);
    EXPECT_EQ (SceneDrawList::selectIrradianceVolume (volumes, BoundingBox ({ 5, 5, 5 }, { 5.2f, 5.2f, 5.2f })), 0);
    EXPECT_EQ (SceneDrawList::selectIrradianceVolume (volumes, BoundingBox ({ 50, 0, 0 }, { 51, 1, 1 })), -1);
}

TEST_F (SceneDrawListTests, VolumeSelectionTakesTheItemCenterAndKeepsTheLowerIndexOnTies)
{
    std::vector<SceneDrawList::IrradianceVolume> volumes (2);
    volumes[0].bounds = BoundingBox ({ -1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f });
    volumes[1].bounds = volumes[0].bounds;

    // Overlapping the volumes but centered outside them
    EXPECT_EQ (SceneDrawList::selectIrradianceVolume (volumes, BoundingBox ({ 0.5f, 0.0f, 0.0f }, { 4.5f, 0.1f, 0.1f })), -1);
    EXPECT_EQ (SceneDrawList::selectIrradianceVolume (volumes, BoundingBox ({ -0.5f, 0.0f, 0.0f }, { 0.5f, 0.1f, 0.1f })), 0);
    EXPECT_EQ (SceneDrawList::selectIrradianceVolume (volumes, BoundingBox()), -1);
}

TEST_F (SceneDrawListTests, PerDrawSelectionPicksTheIrradianceVolume)
{
    auto entity = makeEntity();
    entity.attach<MeshNode> (makeTriangle());

    auto volumeEntity = makeEntity();
    volumeEntity.attach<IrradianceVolumeNode>().extents = { 2.0f, 2.0f, 2.0f };

    drawList.build (root, {}, { false, false });
    ASSERT_EQ (drawList.getOpaqueItems().size(), 1u);
    EXPECT_EQ (drawList.getOpaqueItems()[0].irradianceVolume, -1);

    drawList.build (root, {}, { false, true });
    ASSERT_EQ (drawList.getOpaqueItems().size(), 1u);
    EXPECT_EQ (drawList.getOpaqueItems()[0].irradianceVolume, 0);
}

TEST_F (SceneDrawListTests, VolumeResolutionIsClampedToTwo)
{
    auto volumeEntity = makeEntity();
    auto& volume = volumeEntity.attach<IrradianceVolumeNode>();
    volume.resolutionX = 0;
    volume.resolutionY = -3;
    volume.resolutionZ = 5;

    drawList.build (root, {});
    ASSERT_EQ (drawList.getIrradianceVolumes().size(), 1u);

    const auto packed = GpuSceneData::packIrradianceVolume (drawList.getIrradianceVolumes()[0], {}, {}, 16);
    EXPECT_EQ (packed.resolution[0], 2.0f);
    EXPECT_EQ (packed.resolution[1], 2.0f);
    EXPECT_EQ (packed.resolution[2], 5.0f);
}

TEST_F (SceneDrawListTests, VolumeResolutionIsClampedToTheLargestThatFits)
{
    // Every volume fits both atlases, and no block arithmetic overflows: each axis is clamped to maxResolution
    auto volumeEntity = makeEntity();
    auto& volume = volumeEntity.attach<IrradianceVolumeNode>();
    volume.resolutionX = 100000;
    volume.resolutionY = IrradianceVolumeNode::maxResolution + 1;
    volume.resolutionZ = IrradianceVolumeNode::maxResolution;

    drawList.build (root, {});
    ASSERT_EQ (drawList.getIrradianceVolumes().size(), 1u);

    const auto [nx, ny, nz] = GpuSceneData::getVolumeResolution (volume);
    EXPECT_EQ (nx, IrradianceVolumeNode::maxResolution);
    EXPECT_EQ (ny, IrradianceVolumeNode::maxResolution);
    EXPECT_EQ (nz, IrradianceVolumeNode::maxResolution);

    // The largest volume fits the visibility atlas (18 texels per probe, 1024 wide) and the irradiance atlas (256 wide)
    const auto x = static_cast<int64> (nx), y = static_cast<int64> (ny), z = static_cast<int64> (nz);
    EXPECT_LE (x * z * (ProbeBaker::visibilityTileSize + 2), ProbeBaker::visibilityAtlasWidth);
    EXPECT_LE (y * (ProbeBaker::visibilityTileSize + 2), ProbeBaker::maxVisibilityAtlasHeight);
    EXPECT_LE (x * z, ProbeBaker::irradianceAtlasWidth);
    EXPECT_LE (y * 4, ProbeBaker::maxIrradianceAtlasHeight);
}

//==============================================================================
class SceneDrawListParityTests : public SceneDrawListTests
{
protected:
    using ItemKey = std::tuple<const MeshNode*, int, const Material*>;

    struct Collected
    {
        std::vector<ItemKey> items;
        std::vector<const LightNode*> lights;

        void sort()
        {
            std::sort (items.begin(), items.end());
            std::sort (lights.begin(), lights.end());
        }
    };

    /** What a depth-first walk of the visible tree under @p entity draws, the reference the draw list must match. */
    void walk (const Entity& entity, const Material* overrideMaterial, Collected& collected) const
    {
        if (! entity.isVisible())
            return;

        if (const auto* node = entity.get<MaterialNode>(); node != nullptr && node->material != nullptr)
            overrideMaterial = node->material.get();

        if (const auto* node = entity.get<MeshNode>(); node != nullptr && node->mesh != nullptr)
        {
            for (int i = 0; i < node->mesh->getNumPrimitives(); ++i)
            {
                const auto& primitive = node->mesh->getPrimitive (i);
                const auto* material = overrideMaterial != nullptr     ? overrideMaterial
                                     : primitive.material != nullptr ? primitive.material.get()
                                                                     : &drawList.getDefaultMaterial();
                collected.items.emplace_back (node, i, material);
            }
        }

        if (const auto* node = entity.get<LightNode>())
            collected.lights.push_back (node);

        for (const auto child : entity.getChildren())
            walk (child, overrideMaterial, collected);
    }

    Collected getReference() const
    {
        Collected collected;
        walk (root, nullptr, collected);
        collected.sort();
        return collected;
    }

    Collected getDrawn() const
    {
        Collected collected;

        for (const auto items : { drawList.getOpaqueItems(), drawList.getBlendItems() })
            for (const auto& item : items)
                collected.items.emplace_back (item.node, item.primitiveIndex, item.material);

        for (const auto& light : drawList.getLights())
            collected.lights.push_back (light.node);

        collected.sort();
        return collected;
    }

    static bool isSameOrBelow (Entity entity, const Entity& ancestor)
    {
        for (; entity.isValid(); entity = entity.getParent())
            if (entity == ancestor)
                return true;

        return false;
    }

    /** Builds a random tree of @p count entities with meshes, lights, material overrides and hidden branches. */
    std::vector<Entity> buildRandomTree (Random& random, int count)
    {
        const std::array<Material::Ptr, 3> materials { makeMaterial ("a"),
                                                       makeMaterial ("b", Material::AlphaMode::blend),
                                                       nullptr };
        std::vector<Entity> entities { root };

        for (int i = 0; i < count; ++i)
        {
            auto entity = entities[(size_t) random.nextInt ((int) entities.size())].createChild();
            entities.push_back (entity);

            if (random.nextInt (3) != 0)
                entity.attach<MeshNode> (makeTriangle (materials[(size_t) random.nextInt (3)]));

            if (random.nextInt (5) == 0)
                entity.attach<LightNode>();

            if (random.nextInt (6) == 0)
                entity.attach<MaterialNode> (materials[(size_t) random.nextInt (3)]);

            if (random.nextInt (8) == 0)
                entity.setVisible (false);
        }

        return entities;
    }
};

TEST_F (SceneDrawListParityTests, MatchesAHierarchyWalkThroughStructuralChanges)
{
    Random random (1234);
    auto entities = buildRandomTree (random, 200);

    for (int round = 0; round < 20; ++round)
    {
        for (int change = 0; change < 10; ++change)
        {
            auto& entity = entities[(size_t) (1 + random.nextInt ((int) entities.size() - 1))];
            if (! entity.isValid())
                continue;

            switch (random.nextInt (6))
            {
                case 0:
                {
                    auto& parent = entities[(size_t) random.nextInt ((int) entities.size())];
                    if (parent.isValid() && ! isSameOrBelow (parent, entity))
                        parent.addChild (entity);
                    break;
                }
                case 1: entity.setVisible (! entity.isVisible()); break;
                case 2: entity.removeFromParent(); break;
                case 3: entity.destroy(); break;
                case 4:
                    if (entity.has<MeshNode>())
                        entity.detach<MeshNode>();
                    else
                        entity.attach<MeshNode> (makeTriangle());
                    break;
                default:
                    if (entity.has<MaterialNode>())
                        entity.detach<MaterialNode>();
                    else
                        entity.attach<MaterialNode> (makeMaterial ("override"));
                    break;
            }
        }

        drawList.build (root, {});

        const auto reference = getReference();
        const auto drawn = getDrawn();
        ASSERT_EQ (drawn.items, reference.items) << "round " << round;
        ASSERT_EQ (drawn.lights, reference.lights) << "round " << round;
    }
}

TEST_F (SceneDrawListParityTests, RecordsCollectionAndChurnTimes)
{
    constexpr int numEntities = 10000;
    constexpr int numFrames = 100;

    Random random (5678);
    auto entities = buildRandomTree (random, numEntities);

    auto start = Time::getMillisecondCounterHiRes();
    for (int frame = 0; frame < numFrames; ++frame)
        drawList.build (root, {});

    RecordProperty ("buildMillisecondsPerFrame", String ((Time::getMillisecondCounterHiRes() - start) / numFrames, 4).toStdString());

    start = Time::getMillisecondCounterHiRes();
    for (int i = 1; i < (int) entities.size(); ++i)
    {
        auto& entity = entities[(size_t) i];
        if (entity.has<MeshNode>())
            entity.detach<MeshNode>();
        else
            entity.attach<MeshNode> (makeTriangle());
    }

    RecordProperty ("attachOrDetachMicroseconds", String ((Time::getMillisecondCounterHiRes() - start) * 1000.0 / numEntities, 4).toStdString());

    drawList.build (root, {});
    EXPECT_EQ (getDrawn().items, getReference().items);
}

//==============================================================================
/** The meshes under a MaterialShaderNode, built as SceneRenderer builds its list on a tier. */
class SceneDrawListMaterialShaderTests : public SceneDrawListTests
{
protected:
    /** Builds the list of the raster tier, which selects the lights of each draw, or of the full
        one, coloring the meshes of every material shader but @p refused. */
    void buildForTier (bool raster)
    {
        drawList.build (root, {}, { false, raster, [this] (const MaterialShader::Ptr& asked)
        {
            ++numShadersAsked;
            return asked != refused;
        } });
    }

    /** The item of @p items drawing the mesh of @p entity, or one drawing nothing. */
    static const SceneDrawList::DrawItem& findItem (Span<const SceneDrawList::DrawItem> items, const Entity& entity)
    {
        static const SceneDrawList::DrawItem none;

        const auto* item = std::find_if (items.begin(), items.end(), [&entity] (const SceneDrawList::DrawItem& candidate)
        {
            return candidate.node == entity.get<MeshNode>();
        });

        return item != items.end() ? *item : none;
    }

    MaterialShader::Ptr shader = MaterialShader::createDefault();
    MaterialShader::Ptr otherShader = MaterialShader::createDefault();
    MaterialShader::Ptr refused;
    int numShadersAsked = 0;
};

TEST_F (SceneDrawListMaterialShaderTests, ItemsUnderAMaterialShaderNodeAreShadedForwardOnBothTiers)
{
    auto group = makeEntity ("group");
    auto solid = group.createChild ("solid");
    auto glass = group.createChild ("glass");
    auto outside = makeEntity ("outside");

    auto& node = group.attach<MaterialShaderNode> (shader);
    solid.attach<MeshNode> (makeTriangle (makeMaterial ("solid")));
    glass.attach<MeshNode> (makeTriangle (makeMaterial ("glass", Material::AlphaMode::blend)));
    outside.attach<MeshNode> (makeTriangle (makeMaterial ("solid")));

    for (const auto raster : { true, false })
    {
        buildForTier (raster);

        // Every opaque item is still listed, for the shadows and the depth prepass: the one of the shader last
        ASSERT_EQ (drawList.getOpaqueItems().size(), 2u);
        ASSERT_EQ (drawList.getDeferredItems().size(), 1u);
        ASSERT_EQ (drawList.getForwardOpaqueItems().size(), 1u);
        ASSERT_EQ (drawList.getBlendItems().size(), 1u);
        EXPECT_EQ (drawList.getDeferredItems().data(), drawList.getOpaqueItems().data());
        EXPECT_EQ (drawList.getForwardOpaqueItems().data(), drawList.getOpaqueItems().data() + 1);

        // Out of the deferred items, into the forward ones
        EXPECT_EQ (findItem (drawList.getDeferredItems(), solid).node, nullptr);
        EXPECT_EQ (drawList.getDeferredItems()[0].node, outside.get<MeshNode>());
        EXPECT_EQ (drawList.getDeferredItems()[0].materialShader, nullptr);
        EXPECT_EQ (drawList.getForwardOpaqueItems()[0].node, solid.get<MeshNode>());
        EXPECT_EQ (drawList.getForwardOpaqueItems()[0].materialShader, &node);
        EXPECT_EQ (drawList.getBlendItems()[0].node, glass.get<MeshNode>());
        EXPECT_EQ (drawList.getBlendItems()[0].materialShader, &node);
    }
}

TEST_F (SceneDrawListMaterialShaderTests, MaterialOfTheMeshIsResolvedAsWithoutTheShader)
{
    auto overriding = makeMaterial ("override", Material::AlphaMode::mask);
    auto own = makeMaterial ("own");

    auto overridden = makeEntity ("overridden");
    auto plain = makeEntity ("plain");
    auto bare = makeEntity ("bare");

    root.attach<MaterialShaderNode> (shader);
    overridden.attach<MaterialNode> (overriding);
    overridden.attach<MeshNode> (makeTriangle (own));
    plain.attach<MeshNode> (makeTriangle (own));
    bare.attach<MeshNode> (makeTriangle());

    buildForTier (false);

    ASSERT_EQ (drawList.getForwardOpaqueItems().size(), 3u);
    EXPECT_TRUE (drawList.getDeferredItems().empty());
    EXPECT_EQ (findItem (drawList.getForwardOpaqueItems(), overridden).material, overriding.get());
    EXPECT_EQ (findItem (drawList.getForwardOpaqueItems(), plain).material, own.get());
    EXPECT_EQ (findItem (drawList.getForwardOpaqueItems(), bare).material, &drawList.getDefaultMaterial());
}

TEST_F (SceneDrawListMaterialShaderTests, NearestMaterialShaderNodeWins)
{
    auto group = makeEntity ("group");
    auto leafA = group.createChild ("a");
    auto subGroup = group.createChild ("inner");
    auto leafB = subGroup.createChild ("b");
    auto emptyGroup = group.createChild ("empty");
    auto leafC = emptyGroup.createChild ("c");

    auto& outer = group.attach<MaterialShaderNode> (shader);
    auto& inner = subGroup.attach<MaterialShaderNode> (otherShader);
    emptyGroup.attach<MaterialShaderNode>();

    for (auto leaf : { leafA, leafB, leafC })
        leaf.attach<MeshNode> (makeTriangle());

    buildForTier (true);

    // A node without a shader changes nothing: the one above it still applies
    ASSERT_EQ (drawList.getForwardOpaqueItems().size(), 3u);
    EXPECT_EQ (findItem (drawList.getForwardOpaqueItems(), leafA).materialShader, &outer);
    EXPECT_EQ (findItem (drawList.getForwardOpaqueItems(), leafB).materialShader, &inner);
    EXPECT_EQ (findItem (drawList.getForwardOpaqueItems(), leafC).materialShader, &outer);

    // Each node with a shader is asked about once
    EXPECT_EQ (numShadersAsked, 2);
}

TEST_F (SceneDrawListMaterialShaderTests, MaterialShaderNodesAreIgnoredUnlessAsked)
{
    auto entity = makeEntity();
    entity.attach<MaterialShaderNode> (shader);
    entity.attach<MeshNode> (makeTriangle());
    entity.createChild().attach<MeshNode> (makeTriangle (makeMaterial ("glass", Material::AlphaMode::blend)));

    // As the captures of reflection probes and irradiance volumes build their list
    for (const auto& options : { SceneDrawList::BuildOptions {}, SceneDrawList::BuildOptions { true, true } })
    {
        drawList.build (root, {}, options);

        ASSERT_EQ (drawList.getOpaqueItems().size(), 1u);
        ASSERT_EQ (drawList.getBlendItems().size(), 1u);
        EXPECT_EQ (drawList.getDeferredItems().size(), 1u);
        EXPECT_TRUE (drawList.getForwardOpaqueItems().empty());
        EXPECT_EQ (drawList.getOpaqueItems()[0].materialShader, nullptr);
        EXPECT_EQ (drawList.getBlendItems()[0].materialShader, nullptr);
    }
}

TEST_F (SceneDrawListMaterialShaderTests, RefusedShaderLeavesItsMeshesToTheirMaterials)
{
    auto group = makeEntity ("group");
    auto leafA = group.createChild ("a");
    auto subGroup = group.createChild ("inner");
    auto leafB = subGroup.createChild ("b");

    auto& outer = group.attach<MaterialShaderNode> (shader);
    subGroup.attach<MaterialShaderNode> (otherShader);
    leafA.attach<MeshNode> (makeTriangle());
    leafB.attach<MeshNode> (makeTriangle());

    // As on a tier the shader has no variant for: not the shader of the node above either
    refused = otherShader;
    buildForTier (false);

    ASSERT_EQ (drawList.getDeferredItems().size(), 1u);
    ASSERT_EQ (drawList.getForwardOpaqueItems().size(), 1u);
    EXPECT_EQ (drawList.getDeferredItems()[0].node, leafB.get<MeshNode>());
    EXPECT_EQ (drawList.getDeferredItems()[0].materialShader, nullptr);
    EXPECT_EQ (drawList.getForwardOpaqueItems()[0].node, leafA.get<MeshNode>());
    EXPECT_EQ (drawList.getForwardOpaqueItems()[0].materialShader, &outer);
}

TEST_F (SceneDrawListMaterialShaderTests, RemovingTheNodeOrItsShaderRestoresTheStandardPath)
{
    auto group = makeEntity ("group");
    auto leaf = group.createChild ("leaf");
    auto& node = group.attach<MaterialShaderNode> (shader);
    leaf.attach<MeshNode> (makeTriangle());

    buildForTier (false);
    ASSERT_EQ (drawList.getForwardOpaqueItems().size(), 1u);
    EXPECT_TRUE (drawList.getDeferredItems().empty());

    node.setShader (nullptr);
    buildForTier (false);
    ASSERT_EQ (drawList.getDeferredItems().size(), 1u);
    EXPECT_TRUE (drawList.getForwardOpaqueItems().empty());
    EXPECT_EQ (drawList.getDeferredItems()[0].materialShader, nullptr);

    node.setShader (shader);
    buildForTier (false);
    ASSERT_EQ (drawList.getForwardOpaqueItems().size(), 1u);

    group.detach<MaterialShaderNode>();
    buildForTier (false);
    ASSERT_EQ (drawList.getDeferredItems().size(), 1u);
    EXPECT_TRUE (drawList.getForwardOpaqueItems().empty());
    EXPECT_EQ (drawList.getDeferredItems()[0].materialShader, nullptr);

    // An empty list has no item of either kind
    drawList.clear();
    EXPECT_TRUE (drawList.getDeferredItems().empty());
    EXPECT_TRUE (drawList.getForwardOpaqueItems().empty());
}

TEST_F (SceneDrawListMaterialShaderTests, OpaqueItemsOfMaterialShadersComeLastOneShaderAfterTheOther)
{
    const MaterialShader::Ptr order[] = { shader, nullptr, otherShader, nullptr, shader, otherShader };

    for (const auto& entityShader : order)
    {
        auto entity = makeEntity();
        entity.attach<MeshNode> (makeTriangle (makeMaterial ("solid")));

        if (entityShader != nullptr)
            entity.attach<MaterialShaderNode> (entityShader);
    }

    buildForTier (true);

    const auto items = drawList.getOpaqueItems();
    ASSERT_EQ (items.size(), 6u);
    ASSERT_EQ (drawList.getDeferredItems().size(), 2u);

    const auto shaderOf = [&items] (size_t index)
    {
        return items[index].materialShader != nullptr ? items[index].materialShader->getShader().get() : nullptr;
    };

    EXPECT_EQ (shaderOf (0), nullptr);
    EXPECT_EQ (shaderOf (1), nullptr);
    EXPECT_NE (shaderOf (2), nullptr);
    EXPECT_EQ (shaderOf (2), shaderOf (3));
    EXPECT_NE (shaderOf (3), shaderOf (4));
    EXPECT_EQ (shaderOf (4), shaderOf (5));
}

TEST_F (SceneDrawListMaterialShaderTests, LightsAreSelectedForTheItemsTheirMaterialsShadeOnly)
{
    auto colored = makeEntity ("colored");
    auto lit = makeEntity ("lit");

    colored.attach<MaterialShaderNode> (shader);
    colored.attach<MeshNode> (makeTriangle());
    lit.attach<MeshNode> (makeTriangle());
    lit.attach<LightNode> (LightNode::Type::point);

    buildForTier (true);

    ASSERT_EQ (drawList.getDeferredItems().size(), 1u);
    ASSERT_EQ (drawList.getForwardOpaqueItems().size(), 1u);
    EXPECT_EQ (drawList.getDeferredItems()[0].numLights, 1);
    EXPECT_EQ (drawList.getForwardOpaqueItems()[0].numLights, 0);
}

//==============================================================================
/** The items a camera sees, told by a list built with its projection. The camera of the fixture
    stands at the origin and looks down -Z through a square frustum of 90 degrees from 0.1 to 100:
    at a depth d it reaches d to the left, to the right, up and down.
*/
class SceneDrawListCullingTests : public SceneDrawListTests
{
protected:
    /** Adds the triangle of makeTriangle() scaled by @p size, its right angle at @p corner: its
        bounds reach @p size along +X and +Y from there, at the depth of the corner.
    */
    Entity addTriangle (Vector3<float> corner, float size = 1.0f, Material::Ptr material = nullptr)
    {
        auto entity = makeEntity();
        entity.getOrAttach<TransformNode>().setPosition (corner);
        entity.getOrAttach<TransformNode>().setScale ({ size, size, size });
        entity.attach<MeshNode> (makeTriangle (std::move (material)));
        return entity;
    }

    /** Builds the list of what a camera sees through @p cameraProjection, at the origin looking down -Z unless @p view tells otherwise. */
    void buildForCamera (const Matrix4& cameraProjection, const Matrix4& view = {})
    {
        SceneDrawList::BuildOptions options;
        options.projection = cameraProjection;
        drawList.build (root, view, options);
    }

    /** The item drawing the mesh of @p entity, or one drawing nothing. */
    const SceneDrawList::DrawItem& getItem (const Entity& entity) const
    {
        for (const auto items : { drawList.getOpaqueItems(), drawList.getBlendItems() })
            for (const auto& item : items)
                if (item.node == entity.get<MeshNode>())
                    return item;

        ADD_FAILURE() << "The list has no item of the entity";
        return noItem;
    }

    const Matrix4 projection = Matrix4::perspective (MathConstants<float>::halfPi, 1.0f, 0.1f, 100.0f);
    const SceneDrawList::DrawItem noItem {};
};

TEST_F (SceneDrawListCullingTests, WithoutAProjectionEveryItemIsInView)
{
    const auto ahead = addTriangle ({ -0.5f, -0.5f, -5.0f });
    const auto behind = addTriangle ({ -0.5f, -0.5f, 5.0f });
    const auto aside = addTriangle ({ 50.0f, 0.0f, -5.0f }, 1.0f, makeMaterial ("glass", Material::AlphaMode::blend));

    buildForCamera (projection);
    EXPECT_FALSE (getItem (behind).inView);
    EXPECT_FALSE (getItem (aside).inView);

    // Whatever the list before told, and when selecting the lights of each draw, which computes its bounds as culling does
    for (const auto selectLights : { false, true })
    {
        drawList.build (root, {}, { false, selectLights });

        for (const auto& entity : { ahead, behind, aside })
            EXPECT_TRUE (getItem (entity).inView) << selectLights;
    }
}

TEST_F (SceneDrawListCullingTests, ItemsInsideTheFrustumOrAcrossItsPlanesAreInView)
{
    // At a depth of 5 the frustum reaches 5 to each side: from -5.5 to -4.5 is across the left
    // plane, across the bottom one, across both
    const Entity entitiesInView[] = {
        addTriangle ({ -0.5f, -0.5f, -5.0f }),
        addTriangle ({ 1.0f, 1.0f, -20.0f }, 1.0f, makeMaterial ("glass", Material::AlphaMode::blend)),
        addTriangle ({ -5.5f, 0.0f, -5.0f }),
        addTriangle ({ 0.0f, -5.5f, -5.0f }),
        addTriangle ({ -5.5f, -5.5f, -5.0f }),
        addTriangle ({ -500.0f, -500.0f, -5.0f }, 1000.0f), // around the frustum, every corner of its bounds out of it
    };

    buildForCamera (projection);

    for (const auto& entity : entitiesInView)
        EXPECT_TRUE (getItem (entity).inView) << (&entity - entitiesInView);
}

TEST_F (SceneDrawListCullingTests, ItemsBeyondAPlaneOfTheFrustumAreOutOfView)
{
    // At a depth of 5 the frustum reaches 5 to each side, and it spans the depths from 0.1 to 100
    const Entity entitiesOutOfView[] = {
        addTriangle ({ -8.0f, 0.0f, -5.0f }),
        addTriangle ({ 7.0f, 0.0f, -5.0f }),
        addTriangle ({ 0.0f, -8.0f, -5.0f }),
        addTriangle ({ 0.0f, 7.0f, -5.0f }),
        addTriangle ({ -0.5f, -0.5f, -150.0f }),
        addTriangle ({ -0.005f, -0.005f, -0.01f }, 0.01f),
        addTriangle ({ 7.0f, 0.0f, -5.0f }, 1.0f, makeMaterial ("glass", Material::AlphaMode::blend)),
        addTriangle ({ -0.5f, -0.5f, 5.0f }),           // behind the camera
        addTriangle ({ -50.0f, -50.0f, 5.0f }, 100.0f), // and there wide enough to reach across the planes of the sides, which meet again behind it
    };

    buildForCamera (projection);

    for (const auto& entity : entitiesOutOfView)
        EXPECT_FALSE (getItem (entity).inView) << (&entity - entitiesOutOfView);
}

TEST_F (SceneDrawListCullingTests, FrustumFollowsTheViewMatrix)
{
    const auto alongZ = addTriangle ({ -0.5f, -0.5f, -5.0f });
    const auto alongX = addTriangle ({ 5.0f, -0.5f, 0.0f });

    buildForCamera (projection);
    EXPECT_TRUE (getItem (alongZ).inView);
    EXPECT_FALSE (getItem (alongX).inView);

    // The camera turned to look down +X
    buildForCamera (projection, Matrix4::lookAt ({}, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }));
    EXPECT_FALSE (getItem (alongZ).inView);
    EXPECT_TRUE (getItem (alongX).inView);
}

TEST_F (SceneDrawListCullingTests, FrustumFollowsAProjectionMovedAsideAsTheJitterMovesIt)
{
    // At a depth of 5, a view whose image moved right by a quarter of its width shows from -7.5 to 2.5
    const auto shown = addTriangle ({ -7.0f, 0.0f, -5.0f });
    const auto hidden = addTriangle ({ 3.5f, 0.0f, -5.0f });

    buildForCamera (projection);
    EXPECT_FALSE (getItem (shown).inView);
    EXPECT_TRUE (getItem (hidden).inView);

    // As ViewData jitters its projection, by far more than its part of a pixel
    buildForCamera (projection.followedBy (Matrix4::translation ({ 0.5f, 0.0f, 0.0f })));
    EXPECT_TRUE (getItem (shown).inView);
    EXPECT_FALSE (getItem (hidden).inView);
}

TEST_F (SceneDrawListCullingTests, EveryDepthRangeKeepsWhatIsAheadOfTheCamera)
{
    const auto nearby = addTriangle ({ -0.05f, -0.05f, -0.15f }, 0.1f);
    const auto remote = addTriangle ({ -0.5f, -0.5f, -90.0f });
    const auto distant = addTriangle ({ -0.5f, -0.5f, -100000.0f });
    const auto behind = addTriangle ({ -50.0f, -50.0f, 5.0f }, 100.0f);

    CameraNode camera;
    camera.yFov = MathConstants<float>::halfPi;
    camera.zFar = 0.0f;

    // The same frustum from 0.1, to 100 or without a far plane
    const struct
    {
        Matrix4 matrix;
        bool endless;
    } depthRanges[] = {
        { projection, false },                                                                      // 0 at the near plane to 1 at the far one
        { Matrix4::perspective (MathConstants<float>::halfPi, 1.0f, 0.1f, 100.0f, false), false }, // -1 to 1
        { Matrix4 (std::array<float, 16> { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.1f / 99.9f, -1.0f, 0.0f, 0.0f, 10.0f / 99.9f, 0.0f }), false }, // 1 to 0
        { Matrix4 (std::array<float, 16> { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.1f, 0.0f }), true },                   // 1 to 0
        { camera.getProjectionMatrix (1.0f), true },                                                // 0 to 1
    };

    for (const auto& depthRange : depthRanges)
    {
        buildForCamera (depthRange.matrix);

        const auto index = &depthRange - depthRanges;
        EXPECT_TRUE (getItem (nearby).inView) << index;
        EXPECT_TRUE (getItem (remote).inView) << index;
        EXPECT_FALSE (getItem (behind).inView) << index;

        // Without a far plane, however far
        if (depthRange.endless)
            EXPECT_TRUE (getItem (distant).inView) << index;
    }
}

TEST_F (SceneDrawListCullingTests, ItemsOutOfViewKeepTheirPlaceInTheList)
{
    const auto solid = makeMaterial ("solid");
    const auto glass = makeMaterial ("glass", Material::AlphaMode::blend);

    // Every third one behind the camera
    for (int i = 0; i < 12; ++i)
        addTriangle ({ -0.5f, -0.5f, i % 3 == 0 ? 5.0f : -5.0f - static_cast<float> (i) }, 1.0f, i % 2 == 0 ? solid : glass);

    const auto getNodes = [this]
    {
        std::vector<const MeshNode*> nodes;
        for (const auto items : { drawList.getOpaqueItems(), drawList.getBlendItems() })
            for (const auto& item : items)
                nodes.push_back (item.node);

        return nodes;
    };

    drawList.build (root, {});
    const auto everyItem = getNodes();
    ASSERT_EQ (everyItem.size(), 12u);

    buildForCamera (projection);
    EXPECT_EQ (getNodes(), everyItem);

    auto numOutOfView = 0;
    for (const auto items : { drawList.getOpaqueItems(), drawList.getBlendItems() })
        for (const auto& item : items)
            numOutOfView += item.inView ? 0 : 1;

    EXPECT_EQ (numOutOfView, 4);
}

TEST_F (SceneDrawListCullingTests, LightsAreSelectedForTheItemsInViewOnly)
{
    const auto seen = addTriangle ({ -0.5f, -0.5f, -5.0f });
    const auto unseen = addTriangle ({ -0.5f, -0.5f, 5.0f });
    makeEntity().attach<LightNode>();

    SceneDrawList::BuildOptions options;
    options.selectLightsPerDraw = true;
    options.projection = projection;
    drawList.build (root, {}, options);

    EXPECT_EQ (getItem (seen).numLights, 1);
    EXPECT_EQ (getItem (unseen).numLights, 0);
}
