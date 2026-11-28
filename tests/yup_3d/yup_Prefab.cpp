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

class PrefabTests : public ::testing::Test
{
protected:
    struct CounterNode : Node
    {
        CounterNode() = default;

        explicit CounterNode (int initialValue)
            : value (initialValue)
        {
        }

        int value = 0;
    };

    struct LinkNode : Node
    {
        explicit LinkNode (Entity entityToKeep)
            : linked (entityToKeep)
        {
        }

        Entity linked;
    };

    static Prefab::Ptr makeLamp()
    {
        auto prefab = Prefab::Ptr (new Prefab ("lamp"));
        prefab->getRoot()
            .attach<TransformNode> (Vector3<float> (0.0f, 2.0f, 0.0f))
            .attach<CounterNode> (7);

        auto& bulb = prefab->getRoot().addChild ("bulb");
        bulb.attachWith<CounterNode> ([] (CounterNode& node) { node.value = 42; });
        bulb.addChild ("filament");
        prefab->getRoot().addChild ("shade");

        return prefab;
    }

    Scene scene;
};

TEST_F (PrefabTests, InstantiateBuildsTheTreeUnderTheRoot)
{
    const auto lamp = makeLamp();
    const auto entity = lamp->instantiate (scene);

    ASSERT_TRUE (entity.isValid());
    EXPECT_EQ (entity.getParent(), scene.getRoot());
    EXPECT_EQ (entity.getName(), "lamp");
    ASSERT_EQ (entity.getNumChildren(), 2);
    EXPECT_EQ (entity.getChild (0).getName(), "bulb");
    EXPECT_EQ (entity.getChild (1).getName(), "shade");
    ASSERT_EQ (entity.getChild (0).getNumChildren(), 1);
    EXPECT_EQ (entity.getChild (0).getChild (0).getName(), "filament");
}

TEST_F (PrefabTests, NodesAreBuiltFromTheirArgumentsOrSetup)
{
    const auto entity = makeLamp()->instantiate (scene);

    ASSERT_NE (entity.get<TransformNode>(), nullptr);
    EXPECT_FLOAT_EQ (entity.get<TransformNode>()->getPosition().getY(), 2.0f);
    ASSERT_NE (entity.get<CounterNode>(), nullptr);
    EXPECT_EQ (entity.get<CounterNode>()->value, 7);
    ASSERT_NE (entity.getChild (0).get<CounterNode>(), nullptr);
    EXPECT_EQ (entity.getChild (0).get<CounterNode>()->value, 42);
    EXPECT_FALSE (entity.getChild (1).has<CounterNode>());
}

TEST_F (PrefabTests, EveryInstanceHasItsOwnNodes)
{
    const auto lamp = makeLamp();
    const auto first = lamp->instantiate (scene);
    const auto second = lamp->instantiate (scene, first);

    EXPECT_EQ (second.getParent(), first);
    EXPECT_NE (first.get<CounterNode>(), second.get<CounterNode>());

    first.get<CounterNode>()->value = 1;
    EXPECT_EQ (second.get<CounterNode>()->value, 7);
    EXPECT_NE (first.getUuid(), second.getUuid());
}

TEST_F (PrefabTests, InstantiatesIntoSeveralScenes)
{
    const auto lamp = makeLamp();
    Scene other;

    const auto here = lamp->instantiate (scene);
    const auto there = lamp->instantiate (other);

    EXPECT_EQ (here.getScene(), &scene);
    EXPECT_EQ (there.getScene(), &other);
    EXPECT_EQ (there.get<CounterNode>()->value, 7);
}

TEST_F (PrefabTests, SharedArgumentsAreSharedByInstances)
{
    auto mesh = Mesh::Ptr (new Mesh());
    auto prefab = Prefab::Ptr (new Prefab());
    prefab->getRoot().attach<MeshNode> (mesh);

    const auto first = prefab->instantiate (scene);
    const auto second = prefab->instantiate (scene);

    EXPECT_EQ (first.get<MeshNode>()->mesh, mesh);
    EXPECT_EQ (second.get<MeshNode>()->mesh, mesh);
}

TEST_F (PrefabTests, AttachSignalsSeeTheParentAlreadyBuilt)
{
    int checked = 0;
    auto connection = scene.onAttach<CounterNode>().connect ([&] (Entity entity, CounterNode&)
    {
        if (entity.getName() == "bulb")
        {
            EXPECT_EQ (entity.getParent().getName(), "lamp");
            ++checked;
        }
    });

    makeLamp()->instantiate (scene);
    EXPECT_EQ (checked, 1);
}

TEST_F (PrefabTests, DeepHierarchiesDoNotOverflowTheStack)
{
    auto prefab = Prefab::Ptr (new Prefab ("top"));
    auto* part = &prefab->getRoot();
    for (int i = 0; i < 2000; ++i)
        part = &part->addChild();

    auto entity = prefab->instantiate (scene);
    int depth = 0;
    while (entity.getNumChildren() > 0)
    {
        entity = entity.getChild (0);
        ++depth;
    }

    EXPECT_EQ (depth, 2000);
}

TEST_F (PrefabTests, PartsReportTheirNameAndChildren)
{
    const auto lamp = makeLamp();
    const auto& root = std::as_const (*lamp).getRoot();

    EXPECT_EQ (root.getName(), "lamp");
    ASSERT_EQ (root.getNumChildren(), 2);
    EXPECT_EQ (root.getChild (0).getName(), "bulb");
    EXPECT_EQ (root.getChild (0).getNumChildren(), 1);
}

TEST_F (PrefabTests, PartsAreIndexedInTheOrderTheyWereAdded)
{
    const auto lamp = makeLamp();
    auto& root = lamp->getRoot();

    EXPECT_EQ (root.getIndex(), 0);
    EXPECT_EQ (root.getChild (0).getIndex(), 1);
    EXPECT_EQ (root.getChild (0).getChild (0).getIndex(), 2);
    EXPECT_EQ (root.getChild (1).getIndex(), 3);

    // A part added later, anywhere in the tree, gets the next index and moves no other
    EXPECT_EQ (root.getChild (0).addChild ("socket").getIndex(), 4);
    EXPECT_EQ (root.getChild (0).getChild (0).getIndex(), 2);
    EXPECT_EQ (root.getChild (1).getIndex(), 3);
}

TEST_F (PrefabTests, LinksRunOncePerInstanceWithTheEntityOfEveryPart)
{
    const auto lamp = makeLamp();
    const auto bulb = static_cast<size_t> (lamp->getRoot().getChild (0).getIndex());
    const auto filament = static_cast<size_t> (lamp->getRoot().getChild (0).getChild (0).getIndex());
    int runs = 0;

    lamp->addLink ([&runs, bulb, filament] (Span<const Entity> entities)
    {
        ++runs;
        ASSERT_EQ (entities.size(), 4u);

        // The whole tree is built, and its nodes are attached
        EXPECT_EQ (entities.front().getName(), "lamp");
        EXPECT_EQ (entities[filament].getName(), "filament");
        EXPECT_EQ (entities[filament].getParent(), entities[bulb]);
        ASSERT_NE (entities[bulb].get<CounterNode>(), nullptr);
        EXPECT_EQ (entities[bulb].get<CounterNode>()->value, 42);

        auto root = entities.front();
        root.attach<LinkNode> (entities[bulb]);
    });

    const auto first = lamp->instantiate (scene);
    const auto second = lamp->instantiate (scene, first);

    EXPECT_EQ (runs, 2);
    ASSERT_NE (first.get<LinkNode>(), nullptr);
    ASSERT_NE (second.get<LinkNode>(), nullptr);

    // Every instance is linked to its own entities
    EXPECT_EQ (first.get<LinkNode>()->linked, first.getChild (0));
    EXPECT_EQ (second.get<LinkNode>()->linked, second.getChild (0));
    EXPECT_NE (first.get<LinkNode>()->linked, second.get<LinkNode>()->linked);
}

TEST_F (PrefabTests, PartsOfAnEntityDestroyedWhileBuildingAreLeftOut)
{
    auto prefab = Prefab::Ptr (new Prefab ("lamp"));
    auto& bulb = prefab->getRoot().addChild ("bulb");
    bulb.attach<CounterNode> (1).attach<TransformNode> (Vector3<float> (0.0f, 1.0f, 0.0f));
    bulb.addChild ("filament").addChild ("coil");
    prefab->getRoot().addChild ("shade");

    std::vector<Entity> linked;
    prefab->addLink ([&linked] (Span<const Entity> entities)
    {
        linked.assign (entities.begin(), entities.end());
    });

    // The bulb is destroyed as soon as its first node is attached
    const auto connection = scene.onAttach<CounterNode>().connect ([] (Entity entity, CounterNode&)
    {
        entity.destroy();
    });

    const auto numEntities = scene.getNumEntities();
    const auto lamp = prefab->instantiate (scene);

    // Nothing is made for what the bulb held, which would hang on the root of the scene
    EXPECT_EQ (scene.getNumEntities(), numEntities + 2);
    EXPECT_EQ (scene.getRoot().getNumChildren(), 1);
    ASSERT_EQ (lamp.getNumChildren(), 1);
    EXPECT_EQ (lamp.getChild (0).getName(), "shade");

    // The links get an invalid entity for each part that made none
    ASSERT_EQ (linked.size(), 5u);
    EXPECT_EQ (linked[0], lamp);
    EXPECT_FALSE (linked[1].isValid());
    EXPECT_FALSE (linked[2].isValid());
    EXPECT_FALSE (linked[3].isValid());
    EXPECT_EQ (linked[4], lamp.getChild (0));
}

TEST_F (PrefabTests, LinksRunInTheOrderTheyWereAdded)
{
    auto prefab = Prefab::Ptr (new Prefab());
    std::vector<int> order;

    prefab->addLink ([&order] (Span<const Entity>) { order.push_back (1); });
    prefab->addLink ([&order] (Span<const Entity>) { order.push_back (2); });

    prefab->instantiate (scene);
    EXPECT_EQ (order, (std::vector<int> { 1, 2 }));

    prefab->instantiate (scene);
    EXPECT_EQ (order, (std::vector<int> { 1, 2, 1, 2 }));
}
