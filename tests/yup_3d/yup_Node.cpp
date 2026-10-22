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

class NodeTests : public ::testing::Test
{
protected:
    struct FirstNode : Node
    {
    };

    struct SecondNode : Node
    {
    };

    Scene scene;
};

//==============================================================================

TEST_F (NodeTests, DetachedNodeHasANullUuid)
{
    const FirstNode node;

    EXPECT_TRUE (node.getUuid().isNull());
}

TEST_F (NodeTests, UuidIsAVersion8UuidThatStaysTheSame)
{
    const auto& node = scene.createEntity().attach<FirstNode>();

    const auto uuid = node.getUuid();

    EXPECT_FALSE (uuid.isNull());
    EXPECT_EQ (uuid.getTimeHighAndVersion() >> 12, 8);
    EXPECT_EQ (uuid.getClockSeqAndReserved() >> 6, 2);
    EXPECT_EQ (node.getUuid(), uuid);
}

TEST_F (NodeTests, UuidDiffersFromTheEntityAndBetweenTypes)
{
    auto entity = scene.createEntity();
    const auto& first = entity.attach<FirstNode>();
    const auto& second = entity.attach<SecondNode>();

    EXPECT_NE (first.getUuid(), entity.getUuid());
    EXPECT_NE (first.getUuid(), second.getUuid());
}

TEST_F (NodeTests, UuidDiffersBetweenEntities)
{
    const auto& first = scene.createEntity().attach<FirstNode>();
    const auto& second = scene.createEntity().attach<FirstNode>();

    EXPECT_NE (first.getUuid(), second.getUuid());
}

TEST_F (NodeTests, UuidFollowsTheEntityUuid)
{
    auto entity = scene.createEntity();
    const auto& node = entity.attach<FirstNode>();
    const auto before = node.getUuid();

    entity.setUuid (Uuid ("8b1f6a8e-2c4d-4e5f-9a0b-1c2d3e4f5a6b"));

    EXPECT_NE (node.getUuid(), before);
}

TEST_F (NodeTests, UuidIsReconstructedFromTheSavedEntityUuid)
{
    auto original = scene.createEntity();
    const auto entityUuid = original.getUuid();
    const auto nodeUuid = original.attach<FirstNode>().getUuid();

    Scene loaded;
    auto restored = loaded.createEntity ({}, {}, entityUuid);

    EXPECT_EQ (restored.attach<FirstNode>().getUuid(), nodeUuid);
}

TEST_F (NodeTests, ReattachingTheSameTypeGivesTheSameUuid)
{
    auto entity = scene.createEntity();
    const auto uuid = entity.attach<FirstNode>().getUuid();

    entity.detach<FirstNode>();

    EXPECT_EQ (entity.attach<FirstNode>().getUuid(), uuid);
}
