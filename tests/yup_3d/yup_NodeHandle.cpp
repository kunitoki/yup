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

class NodeHandleTests : public ::testing::Test
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

    Scene scene;
};

TEST_F (NodeHandleTests, DefaultHandleIsEmpty)
{
    const NodeHandle<ValueNode> handle;

    EXPECT_FALSE (handle);
    EXPECT_EQ (handle.get(), nullptr);
    EXPECT_FALSE (handle.getEntity().isValid());
}

TEST_F (NodeHandleTests, PointsAtTheAttachedNode)
{
    auto entity = scene.createEntity();
    auto& node = entity.attach<ValueNode> (3);

    const NodeHandle<ValueNode> handle (node);

    ASSERT_TRUE (handle);
    EXPECT_EQ (handle.get(), &node);
    EXPECT_EQ (handle->value, 3);
    EXPECT_EQ ((*handle).value, 3);
    EXPECT_EQ (handle.getEntity(), entity);
}

TEST_F (NodeHandleTests, BecomesEmptyWhenTheNodeIsDetached)
{
    auto entity = scene.createEntity();
    const NodeHandle<ValueNode> handle (entity.attach<ValueNode>());

    entity.detach<ValueNode>();

    EXPECT_FALSE (handle);
}

TEST_F (NodeHandleTests, BecomesEmptyWhenTheEntityIsDestroyed)
{
    auto entity = scene.createEntity();
    const NodeHandle<ValueNode> handle (entity.attach<ValueNode>());

    entity.destroy();
    scene.createEntity().attach<ValueNode>(); // may reuse the index

    EXPECT_FALSE (handle);
}

TEST_F (NodeHandleTests, FollowsAReplacedNode)
{
    auto entity = scene.createEntity();
    const NodeHandle<ValueNode> handle (entity.attach<ValueNode> (1));

    entity.attach<ValueNode> (2);

    ASSERT_TRUE (handle);
    EXPECT_EQ (handle->value, 2);
}

TEST_F (NodeHandleTests, EntityHandleSeesNodesAttachedLater)
{
    auto entity = scene.createEntity();
    const NodeHandle<ValueNode> handle (entity);

    EXPECT_FALSE (handle);

    entity.attach<ValueNode> (5);

    ASSERT_TRUE (handle);
    EXPECT_EQ (handle->value, 5);
}
