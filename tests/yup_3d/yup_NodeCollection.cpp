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

class NodeCollectionTests : public ::testing::Test
{
protected:
    struct ValueNode : Node
    {
    };

    struct TagNode : Node
    {
    };

    Scene scene;
};

TEST_F (NodeCollectionTests, CollectReturnsTheSameCollection)
{
    auto& first = scene.collect<ValueNode>();
    auto& second = scene.collect<ValueNode>();

    EXPECT_EQ (&first, &second);
}

TEST_F (NodeCollectionTests, IncludesNodesAttachedBeforeAndAfter)
{
    auto before = scene.createEntity();
    before.attach<ValueNode>();

    auto& collection = scene.collect<ValueNode>();

    auto after = scene.createEntity();
    after.attach<ValueNode>();

    EXPECT_EQ (collection.size(), 2);
    EXPECT_TRUE (collection.contains (before));
    EXPECT_TRUE (collection.contains (after));
}

TEST_F (NodeCollectionTests, StaysInSyncWithDetachAndDestroy)
{
    auto& collection = scene.collect<ValueNode>();

    auto detached = scene.createEntity();
    detached.attach<ValueNode>();
    auto destroyed = scene.createEntity();
    destroyed.attach<ValueNode>();
    auto kept = scene.createEntity();
    kept.attach<ValueNode>();

    detached.detach<ValueNode>();
    destroyed.destroy();

    EXPECT_EQ (collection.size(), 1);
    EXPECT_EQ (collection.first(), kept);
    EXPECT_FALSE (collection.contains (detached));
}

TEST_F (NodeCollectionTests, SeveralTypesNeedEveryNode)
{
    auto& collection = scene.collect<ValueNode, TagNode>();
    auto entity = scene.createEntity();

    entity.attach<ValueNode>();
    EXPECT_TRUE (collection.isEmpty());

    entity.attach<TagNode>();
    EXPECT_TRUE (collection.contains (entity));

    entity.detach<ValueNode>();
    EXPECT_TRUE (collection.isEmpty());
}

TEST_F (NodeCollectionTests, ForEachToleratesRemovalWhileIterating)
{
    auto& collection = scene.collect<ValueNode>();
    std::vector<Entity> entities;

    for (int i = 0; i < 4; ++i)
    {
        entities.push_back (scene.createEntity());
        entities.back().attach<ValueNode>();
    }

    int visited = 0;
    collection.forEach ([&] (Entity entity, ValueNode&)
    {
        ++visited;

        if (entity == entities[0])
            entities[1].detach<ValueNode>();
    });

    EXPECT_EQ (visited, 3);
    EXPECT_EQ (collection.size(), 3);
}
