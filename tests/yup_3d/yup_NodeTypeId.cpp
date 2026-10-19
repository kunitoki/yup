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

class NodeTypeIdTests : public ::testing::Test
{
protected:
    struct PlainNode : Node
    {
    };

    struct OtherNode : Node
    {
    };

    struct PinnedNode : Node
    {
        static constexpr std::string_view nodeTypeName = "tests::Pinned";
    };
};

TEST_F (NodeTypeIdTests, IdsAreConstantExpressions)
{
    constexpr auto id = NodeTypeId::get<MeshNode>();
    static_assert (id == NodeTypeId::get<MeshNode>());

    EXPECT_EQ (id, NodeTypeId::get<MeshNode>());
}

TEST_F (NodeTypeIdTests, DistinctTypesHaveDistinctIds)
{
    EXPECT_NE (NodeTypeId::get<PlainNode>(), NodeTypeId::get<OtherNode>());
    EXPECT_NE (NodeTypeId::get<MeshNode>(), NodeTypeId::get<MaterialNode>());
}

TEST_F (NodeTypeIdTests, NamesAreQualifiedWithoutKeywords)
{
    EXPECT_EQ (NodeTypeId::getName<MeshNode>(), "yup::MeshNode");
    EXPECT_EQ (NodeTypeId::getName<TransformNode>(), "yup::TransformNode");
}

TEST_F (NodeTypeIdTests, NodeTypeNamePinsTheName)
{
    EXPECT_EQ (NodeTypeId::getName<PinnedNode>(), "tests::Pinned");
    EXPECT_EQ (NodeTypeId::get<PinnedNode>(), NodeTypeId::hash ("tests::Pinned"));
}

TEST_F (NodeTypeIdTests, IdIsTheHashOfTheName)
{
    EXPECT_EQ (NodeTypeId::get<MeshNode>(), NodeTypeId::hash ("yup::MeshNode"));
}

TEST_F (NodeTypeIdTests, HashIsFnv1a)
{
    EXPECT_EQ (NodeTypeId::hash (""), 14695981039346656037ull);
    EXPECT_EQ (NodeTypeId::hash ("a"), 0xaf63dc4c8601ec8cull);
}
