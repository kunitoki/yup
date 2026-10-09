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

class SceneDrawListTests : public ::testing::Test
{
protected:
    static EntityNode::Ptr makeEntity (const String& name = {})
    {
        return EntityNode::Ptr (new EntityNode (name));
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

    EntityNode::Ptr root = makeEntity ("root");
    SceneDrawList drawList;
};

TEST_F (SceneDrawListTests, EmptyTreeHasNothingToDraw)
{
    drawList.build (*root, {});

    EXPECT_TRUE (drawList.getOpaqueItems().empty());
    EXPECT_TRUE (drawList.getBlendItems().empty());
    EXPECT_TRUE (drawList.getLights().empty());
}

TEST_F (SceneDrawListTests, WorldMatricesAccumulate)
{
    auto child = makeEntity();
    auto grandChild = makeEntity();
    root->addChild (child);
    child->addChild (grandChild);

    root->setPosition ({ 1.0f, 0.0f, 0.0f });
    child->setScale ({ 2.0f, 2.0f, 2.0f });
    grandChild->setPosition ({ 0.0f, 1.0f, 0.0f });
    grandChild->attach<MeshNode> (makeTriangle());

    drawList.build (*root, {});

    ASSERT_EQ (drawList.getOpaqueItems().size(), 1u);
    expectNear (drawList.getOpaqueItems()[0].world.transformPoint ({}), { 1.0f, 2.0f, 0.0f });
    EXPECT_TRUE (drawList.getOpaqueItems()[0].world.approximatelyEqualTo (grandChild->getWorldMatrix()));
}

TEST_F (SceneDrawListTests, HiddenSubtreesAreSkipped)
{
    auto hidden = makeEntity();
    auto hiddenChild = makeEntity();
    auto visible = makeEntity();
    root->addChild (hidden);
    hidden->addChild (hiddenChild);
    root->addChild (visible);

    hiddenChild->attach<MeshNode> (makeTriangle());
    hiddenChild->attach<LightNode>();
    visible->attach<MeshNode> (makeTriangle());
    hidden->setVisible (false);

    drawList.build (*root, {});

    EXPECT_EQ (drawList.getOpaqueItems().size(), 1u);
    EXPECT_TRUE (drawList.getLights().empty());
}

TEST_F (SceneDrawListTests, EveryMeshSlotAndPrimitiveIsDrawn)
{
    auto mesh = makeTriangle();
    std::vector<Mesh::Vertex> vertices (3);
    vertices[1].position = { 1.0f, 0.0f, 0.0f };
    vertices[2].position = { 0.0f, 0.0f, 1.0f };
    mesh->addPrimitive (vertices, {});

    root->attach<MeshNode> (mesh);
    root->attach<MeshNode, 3> (makeTriangle());
    root->attach<MeshNode, 4>();

    drawList.build (*root, {});

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
    root->addChild (withMaterial);
    root->addChild (withoutMaterial);
    withMaterial->attach<MeshNode> (makeTriangle (own));
    withoutMaterial->attach<MeshNode> (makeTriangle());

    drawList.build (*root, {});

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
    auto subGroup = makeEntity();
    auto leafA = makeEntity ("a");
    auto leafB = makeEntity ("b");
    root->addChild (group);
    group->addChild (leafA);
    group->addChild (subGroup);
    subGroup->addChild (leafB);

    group->attach<MaterialNode> (outer);
    subGroup->attach<MaterialNode> (inner);
    leafA->attach<MeshNode> (makeTriangle (own));
    leafB->attach<MeshNode> (makeTriangle (own));

    drawList.build (*root, {});

    ASSERT_EQ (drawList.getOpaqueItems().size(), 2u);

    for (const auto& item : drawList.getOpaqueItems())
    {
        if (item.world.approximatelyEqualTo (leafA->getWorldMatrix()) && item.mesh == leafA->getNode<MeshNode>()->mesh.get())
            EXPECT_EQ (item.material, outer.get());
        else
            EXPECT_EQ (item.material, inner.get());
    }
}

TEST_F (SceneDrawListTests, OnlyTheLowestMaterialSlotOverrides)
{
    auto lowest = makeMaterial ("lowest");
    auto spare = makeMaterial ("spare");

    root->attach<MaterialNode, 5> (spare);
    root->attach<MaterialNode, 2> (lowest);
    root->attach<MeshNode> (makeTriangle());

    drawList.build (*root, {});

    ASSERT_EQ (drawList.getOpaqueItems().size(), 1u);
    EXPECT_EQ (drawList.getOpaqueItems()[0].material, lowest.get());
}

TEST_F (SceneDrawListTests, AnOverrideWithoutMaterialInheritsTheParentOne)
{
    auto outer = makeMaterial ("outer");
    auto child = makeEntity();
    root->addChild (child);
    root->attach<MaterialNode> (outer);
    child->attach<MaterialNode>();
    child->attach<MeshNode> (makeTriangle (makeMaterial ("own")));

    drawList.build (*root, {});

    ASSERT_EQ (drawList.getOpaqueItems().size(), 1u);
    EXPECT_EQ (drawList.getOpaqueItems()[0].material, outer.get());
}

TEST_F (SceneDrawListTests, BlendItemsAreSortedBackToFront)
{
    auto glass = makeMaterial ("glass", Material::AlphaMode::blend);
    const float depths[] = { -5.0f, -20.0f, -10.0f };

    for (auto z : depths)
    {
        auto entity = makeEntity (String (z));
        entity->setPosition ({ 0.0f, 0.0f, z });
        entity->attach<MeshNode> (makeTriangle (glass));
        root->addChild (entity);
    }

    root->attach<MeshNode> (makeTriangle (makeMaterial ("solid")));

    // Camera at the origin looking down -Z
    drawList.build (*root, Matrix4());

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
        entity->attach<MeshNode> (makeTriangle (material));
        root->addChild (entity);
    }

    drawList.build (*root, {});

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

TEST_F (SceneDrawListTests, LightsAreGatheredInWorldSpace)
{
    auto lamp = makeEntity();
    root->addChild (lamp);
    lamp->setPosition ({ 1.0f, 2.0f, 3.0f });
    lamp->setRotation (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, MathConstants<float>::halfPi));
    lamp->attach<LightNode> (LightNode::Type::point);
    lamp->attach<LightNode, 1> (LightNode::Type::spot);

    drawList.build (*root, {});

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
    auto doubleMirrored = makeEntity();
    root->addChild (mirrored);
    mirrored->addChild (doubleMirrored);

    mirrored->setScale ({ -1.0f, 1.0f, 1.0f });
    doubleMirrored->setScale ({ 1.0f, -2.0f, 1.0f });
    root->attach<MeshNode> (makeTriangle());
    mirrored->attach<MeshNode> (makeTriangle());
    doubleMirrored->attach<MeshNode> (makeTriangle());

    drawList.build (*root, {});

    ASSERT_EQ (drawList.getOpaqueItems().size(), 3u);

    int mirroredCount = 0;
    for (const auto& item : drawList.getOpaqueItems())
    {
        if (item.mirrored)
        {
            ++mirroredCount;
            EXPECT_TRUE (item.world.approximatelyEqualTo (mirrored->getWorldMatrix()));
        }
    }

    EXPECT_EQ (mirroredCount, 1);
}

TEST_F (SceneDrawListTests, RebuildingReplacesThePreviousItems)
{
    root->attach<MeshNode> (makeTriangle());
    drawList.build (*root, {});
    drawList.build (*root, {});

    EXPECT_EQ (drawList.getOpaqueItems().size(), 1u);

    drawList.clear();
    EXPECT_TRUE (drawList.getOpaqueItems().empty());
}

TEST_F (SceneDrawListTests, ItemsPointToTheirMeshNode)
{
    auto first = makeEntity();
    auto second = makeEntity();
    root->addChild (first);
    root->addChild (second);

    auto& firstNode = first->attach<MeshNode> (makeTriangle());
    auto& transparentNode = second->attach<MeshNode> (makeTriangle (makeMaterial ("glass", Material::AlphaMode::blend)));

    drawList.build (*root, {});

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
    entity->attach<MeshNode> (makeTriangle());
    root->addChild (entity);

    drawList.build (*root, {});
    EXPECT_TRUE (drawList.getLights().empty());

    drawList.build (*root, {}, { true });
    ASSERT_EQ (drawList.getLights().size(), 1u);
    EXPECT_EQ (drawList.getLights()[0].node->getEntity(), nullptr);
    EXPECT_EQ (drawList.getLights()[0].node->type, LightNode::Type::directional);
    EXPECT_EQ (drawList.getLights()[0].node->color, (std::array<float, 3> { 1.0f, 1.0f, 1.0f }));
    EXPECT_FLOAT_EQ (drawList.getLights()[0].node->intensity, 3.0f);
    EXPECT_FALSE (drawList.getLights()[0].node->castsShadows);
    expectNear (drawList.getLights()[0].direction, Vector3<float> (-0.35f, -0.75f, -0.55f).normalized());

    auto lamp = makeEntity();
    lamp->attach<LightNode> (LightNode::Type::point);
    root->addChild (lamp);

    drawList.build (*root, {}, { true });
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

TEST_F (SceneDrawListTests, DefaultLightIsSelectedForEveryDraw)
{
    auto entity = makeEntity();
    entity->attach<MeshNode> (makeTriangle());
    root->addChild (entity);

    drawList.build (*root, {}, { true, true });

    ASSERT_EQ (drawList.getOpaqueItems().size(), 1u);
    EXPECT_EQ (drawList.getOpaqueItems()[0].numLights, 1);
    EXPECT_EQ (drawList.getOpaqueItems()[0].lightIndices[0], 0);
}

TEST_F (SceneDrawListTests, BlendedItemsSelectLightsToo)
{
    auto entity = makeEntity();
    entity->attach<MeshNode> (makeTriangle (makeMaterial ("glass", Material::AlphaMode::blend)));
    entity->attach<LightNode> (LightNode::Type::point);
    root->addChild (entity);

    drawList.build (*root, {}, { false, true });

    ASSERT_EQ (drawList.getBlendItems().size(), 1u);
    EXPECT_EQ (drawList.getBlendItems()[0].numLights, 1);
    EXPECT_EQ (drawList.getBlendItems()[0].lightIndices[0], 0);
}

TEST_F (SceneDrawListTests, PerDrawSelectionIsSkippedUnlessAsked)
{
    auto entity = makeEntity();
    entity->attach<MeshNode> (makeTriangle());
    entity->attach<LightNode> (LightNode::Type::point);
    root->addChild (entity);

    drawList.build (*root, {}, { false, false });
    EXPECT_EQ (drawList.getOpaqueItems()[0].numLights, 0);
}
