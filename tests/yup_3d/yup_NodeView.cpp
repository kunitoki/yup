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

class NodeViewTests : public ::testing::Test
{
protected:
    struct ValueNode : Node
    {
        explicit ValueNode (int initialValue = 0)
            : value (initialValue)
        {
        }

        int value = 0;
    };

    struct TagNode : Node
    {
    };

    Entity makeEntity (int value, bool tagged)
    {
        auto entity = scene.createEntity();
        entity.attach<ValueNode> (value);

        if (tagged)
            entity.attach<TagNode>();

        return entity;
    }

    Scene scene;
};

TEST_F (NodeViewTests, EmptyViewHasNothing)
{
    auto view = scene.view<ValueNode>();

    EXPECT_TRUE (view.isEmpty());
    EXPECT_EQ (view.size(), 0);
    EXPECT_FALSE (view.first().isValid());
}

TEST_F (NodeViewTests, SingleTypeVisitsEveryNode)
{
    makeEntity (1, false);
    makeEntity (2, true);

    int sum = 0;
    for (auto [entity, value] : scene.view<ValueNode>())
    {
        EXPECT_TRUE (entity.isValid());
        sum += value.value;
    }

    EXPECT_EQ (sum, 3);
    EXPECT_EQ (scene.view<ValueNode>().size(), 2);
}

TEST_F (NodeViewTests, SeveralTypesVisitOnlyEntitiesHavingAll)
{
    makeEntity (1, false);
    const auto tagged = makeEntity (2, true);
    makeEntity (3, false);

    std::vector<int> values;
    scene.view<ValueNode, TagNode>().forEach ([&] (Entity entity, ValueNode& value, TagNode&)
    {
        EXPECT_EQ (entity, tagged);
        values.push_back (value.value);
    });

    EXPECT_EQ (values, std::vector<int> { 2 });
    EXPECT_EQ ((scene.view<TagNode, ValueNode>().first()), tagged);
}

TEST_F (NodeViewTests, WhereFiltersAndChains)
{
    for (int i = 0; i < 10; ++i)
        makeEntity (i, false);

    auto view = scene.view<ValueNode>()
                    .where ([] (const ValueNode& node) { return node.value % 2 == 0; })
                    .where ([] (const ValueNode& node) { return node.value > 4; });

    EXPECT_EQ (view.size(), 2); // 6, 8
}

TEST_F (NodeViewTests, StoredViewsCanBeFilteredWhenMoved)
{
    for (int i = 0; i < 4; ++i)
        makeEntity (i, false);

    auto view = scene.view<ValueNode>();
    EXPECT_EQ (view.size(), 4);

    auto filtered = std::move (view).where ([] (const ValueNode& node) { return node.value < 3; });
    EXPECT_EQ (filtered.size(), 3);
}

TEST_F (NodeViewTests, SizeSkipsDetachedNodes)
{
    auto first = makeEntity (1, false);
    makeEntity (2, false);

    scene.view<ValueNode>().forEach ([&] (Entity, ValueNode&)
    {
        first.detach<ValueNode>();
        EXPECT_EQ (scene.view<ValueNode>().size(), 1);
    });

    EXPECT_EQ (scene.view<ValueNode>().size(), 1);
}

TEST_F (NodeViewTests, NodesCanBeModifiedThroughTheView)
{
    makeEntity (1, false);

    for (auto [entity, value] : scene.view<ValueNode>())
        value.value = 42;

    EXPECT_EQ (scene.view<ValueNode>().first().get<ValueNode>()->value, 42);
}

TEST_F (NodeViewTests, NodesDetachedWhileIteratingAreSkipped)
{
    auto first = makeEntity (1, false);
    auto second = makeEntity (2, false);

    std::vector<int> visited;
    scene.view<ValueNode>().forEach ([&] (Entity entity, ValueNode& value)
    {
        visited.push_back (value.value);

        if (entity == first)
            second.detach<ValueNode>();
    });

    EXPECT_EQ (visited, std::vector<int> { 1 });
    EXPECT_FALSE (second.has<ValueNode>());
}

TEST_F (NodeViewTests, EntitiesDestroyedWhileIteratingAreGoneAfterwards)
{
    for (int i = 0; i < 4; ++i)
        makeEntity (i, false);

    scene.view<ValueNode>().forEach ([] (Entity entity, ValueNode& value)
    {
        if (value.value % 2 == 0)
            entity.destroy();
    });

    EXPECT_EQ (scene.view<ValueNode>().size(), 2);
}
