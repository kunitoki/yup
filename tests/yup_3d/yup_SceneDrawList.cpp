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
