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

#include <yup_animation/yup_animation.h>

using namespace yup;

class AnimationGroupTests : public ::testing::Test
{
};

// =============================================================================
// Default state
// =============================================================================

TEST_F (AnimationGroupTests, DefaultConstruction_IsEmpty)
{
    AnimationGroup group;
    EXPECT_TRUE (group.children.empty());
    EXPECT_FALSE (group.hidden);
    EXPECT_TRUE (group.name.isEmpty());
}

// =============================================================================
// Shape children
// =============================================================================

TEST_F (AnimationGroupTests, AddEllipseShape_AppendsShapeChild)
{
    AnimationGroup group;
    auto* shape = group.addShape<EllipseShape>();

    ASSERT_NE (shape, nullptr);
    ASSERT_EQ (group.children.size(), 1u);
    EXPECT_EQ (group.children[0].kind, AnimationGroup::ChildKind::Shape);
    EXPECT_EQ (shape->getKind(), AnimationShape::Kind::Ellipse);
    EXPECT_NE (group.children[0].shape.get(), nullptr);
}

TEST_F (AnimationGroupTests, AddRectShape_AppendsShapeChild)
{
    AnimationGroup group;
    auto* shape = group.addShape<RectShape>();

    ASSERT_NE (shape, nullptr);
    ASSERT_EQ (group.children.size(), 1u);
    EXPECT_EQ (shape->getKind(), AnimationShape::Kind::Rect);
}

TEST_F (AnimationGroupTests, AddBezierPathShape_AppendsShapeChild)
{
    AnimationGroup group;
    auto* shape = group.addShape<BezierPathShape>();

    ASSERT_NE (shape, nullptr);
    EXPECT_EQ (shape->getKind(), AnimationShape::Kind::BezierPath);
}

TEST_F (AnimationGroupTests, AddPolystarShape_AppendsShapeChild)
{
    AnimationGroup group;
    auto* shape = group.addShape<PolystarShape>();

    ASSERT_NE (shape, nullptr);
    EXPECT_EQ (shape->getKind(), AnimationShape::Kind::Polystar);
}

// =============================================================================
// Non-shape children
// =============================================================================

TEST_F (AnimationGroupTests, AddGroup_AppendsGroupChild)
{
    AnimationGroup group;
    auto* nested = group.addGroup();

    ASSERT_NE (nested, nullptr);
    ASSERT_EQ (group.children.size(), 1u);
    EXPECT_EQ (group.children[0].kind, AnimationGroup::ChildKind::Group);
    EXPECT_NE (group.children[0].group.get(), nullptr);
}

TEST_F (AnimationGroupTests, AddFill_AppendsFillChild)
{
    AnimationGroup group;
    auto* fill = group.addFill();

    ASSERT_NE (fill, nullptr);
    ASSERT_EQ (group.children.size(), 1u);
    EXPECT_EQ (group.children[0].kind, AnimationGroup::ChildKind::Fill);
}

TEST_F (AnimationGroupTests, AddStroke_AppendsStrokeChild)
{
    AnimationGroup group;
    auto* stroke = group.addStroke();

    ASSERT_NE (stroke, nullptr);
    ASSERT_EQ (group.children.size(), 1u);
    EXPECT_EQ (group.children[0].kind, AnimationGroup::ChildKind::Stroke);
}

TEST_F (AnimationGroupTests, AddTrim_AppendsTrimChild)
{
    AnimationGroup group;
    auto* trim = group.addTrim();

    ASSERT_NE (trim, nullptr);
    ASSERT_EQ (group.children.size(), 1u);
    EXPECT_EQ (group.children[0].kind, AnimationGroup::ChildKind::Trim);
}

TEST_F (AnimationGroupTests, AddRepeater_AppendsRepeaterChild)
{
    AnimationGroup group;
    auto* rep = group.addRepeater();

    ASSERT_NE (rep, nullptr);
    ASSERT_EQ (group.children.size(), 1u);
    EXPECT_EQ (group.children[0].kind, AnimationGroup::ChildKind::Repeater);
}

TEST_F (AnimationGroupTests, AddRoundedCorner_AppendsRoundedCornerChild)
{
    AnimationGroup group;
    auto* rc = group.addRoundedCorner();

    ASSERT_NE (rc, nullptr);
    ASSERT_EQ (group.children.size(), 1u);
    EXPECT_EQ (group.children[0].kind, AnimationGroup::ChildKind::RoundedCorner);
}

// =============================================================================
// Multiple children
// =============================================================================

TEST_F (AnimationGroupTests, MultipleChildren_AccumulateInOrder)
{
    AnimationGroup group;
    group.addShape<EllipseShape>();
    group.addFill();
    group.addStroke();
    group.addGroup();

    ASSERT_EQ (group.children.size(), 4u);
    EXPECT_EQ (group.children[0].kind, AnimationGroup::ChildKind::Shape);
    EXPECT_EQ (group.children[1].kind, AnimationGroup::ChildKind::Fill);
    EXPECT_EQ (group.children[2].kind, AnimationGroup::ChildKind::Stroke);
    EXPECT_EQ (group.children[3].kind, AnimationGroup::ChildKind::Group);
}

// =============================================================================
// ShapeLayer
// =============================================================================

class ShapeLayerTests : public ::testing::Test
{
};

TEST_F (ShapeLayerTests, GetType_ReturnsShape)
{
    ShapeLayer layer;
    EXPECT_EQ (layer.getType(), AnimationLayer::Type::Shape);
}

TEST_F (ShapeLayerTests, DefaultConstruction_NoGroups)
{
    ShapeLayer layer;
    EXPECT_EQ (layer.getNumGroups(), 0);
    EXPECT_TRUE (layer.groups.empty());
}

TEST_F (ShapeLayerTests, AddGroup_AppendsGroup)
{
    ShapeLayer layer;
    auto* group = layer.addGroup ("MyGroup");

    ASSERT_NE (group, nullptr);
    EXPECT_EQ (layer.getNumGroups(), 1);
    EXPECT_EQ (layer.groups.size(), 1u);
    EXPECT_EQ (group->name, String ("MyGroup"));
}

TEST_F (ShapeLayerTests, GetGroup_ReturnsCorrectPointer)
{
    ShapeLayer layer;
    auto* a = layer.addGroup ("A");
    auto* b = layer.addGroup ("B");

    EXPECT_EQ (layer.getGroup (0), a);
    EXPECT_EQ (layer.getGroup (1), b);
}

TEST_F (ShapeLayerTests, AddMultipleGroups_CountsCorrectly)
{
    ShapeLayer layer;
    layer.addGroup ("G1");
    layer.addGroup ("G2");
    layer.addGroup ("G3");

    EXPECT_EQ (layer.getNumGroups(), 3);
}

// =============================================================================
// AnimationLayer subclass types
// =============================================================================

class AnimationLayerSubclassTests : public ::testing::Test
{
};

TEST_F (AnimationLayerSubclassTests, NullLayer_GetTypeReturnsNull)
{
    NullLayer layer;
    EXPECT_EQ (layer.getType(), AnimationLayer::Type::Null);
}

TEST_F (AnimationLayerSubclassTests, SolidLayer_GetTypeReturnsSolid)
{
    SolidLayer layer;
    EXPECT_EQ (layer.getType(), AnimationLayer::Type::Solid);
}

TEST_F (AnimationLayerSubclassTests, ImageLayer_GetTypeReturnsImage)
{
    ImageLayer layer;
    EXPECT_EQ (layer.getType(), AnimationLayer::Type::Image);
}

TEST_F (AnimationLayerSubclassTests, PrecompLayer_GetTypeReturnsPrecomp)
{
    PrecompLayer layer;
    EXPECT_EQ (layer.getType(), AnimationLayer::Type::Precomp);
}

TEST_F (AnimationLayerSubclassTests, NullLayer_DefaultFields)
{
    NullLayer layer;
    EXPECT_EQ (layer.id, -1);
    EXPECT_EQ (layer.parentId, -1);
    EXPECT_FALSE (layer.hidden);
    EXPECT_FLOAT_EQ (layer.inFrame, 0.0f);
    EXPECT_FLOAT_EQ (layer.outFrame, 0.0f);
}

TEST_F (AnimationLayerSubclassTests, IsVisibleAt_TrueWhenInRange)
{
    NullLayer layer;
    layer.inFrame = 5.0f;
    layer.outFrame = 25.0f;

    EXPECT_TRUE (layer.isVisibleAt (5.0f));
    EXPECT_TRUE (layer.isVisibleAt (15.0f));
    EXPECT_TRUE (layer.isVisibleAt (24.9f));
    EXPECT_FALSE (layer.isVisibleAt (25.0f)); // outFrame is exclusive
    EXPECT_FALSE (layer.isVisibleAt (4.9f));
    EXPECT_FALSE (layer.isVisibleAt (30.0f));
}

TEST_F (AnimationLayerSubclassTests, IsVisibleAt_HiddenLayerAlwaysFalse)
{
    NullLayer layer;
    layer.inFrame = 0.0f;
    layer.outFrame = 100.0f;
    layer.hidden = true;

    EXPECT_FALSE (layer.isVisibleAt (0.0f));
    EXPECT_FALSE (layer.isVisibleAt (50.0f));
}

TEST_F (AnimationLayerSubclassTests, LocalFrame_NoTimeRemapUsesStartFrameOffset)
{
    NullLayer layer;
    layer.startFrame = 10.0f;
    layer.timeStretch = 1.0f;

    // localFrame(compFrame) = (compFrame - startFrame) / timeStretch
    EXPECT_NEAR (layer.localFrame (10.0f), 0.0f, 1e-4f);
    EXPECT_NEAR (layer.localFrame (20.0f), 10.0f, 1e-4f);
    EXPECT_NEAR (layer.localFrame (5.0f), -5.0f, 1e-4f);
}

TEST_F (AnimationLayerSubclassTests, LocalFrame_TimeStretchScalesResult)
{
    NullLayer layer;
    layer.startFrame = 0.0f;
    layer.timeStretch = 2.0f;

    EXPECT_NEAR (layer.localFrame (20.0f), 10.0f, 1e-4f);
    EXPECT_NEAR (layer.localFrame (40.0f), 20.0f, 1e-4f);
}

TEST_F (AnimationLayerSubclassTests, SolidLayer_StoresSolidColor)
{
    SolidLayer layer;
    layer.solidColor = Color (0xff00ff00);
    layer.layerSize = { 100.0f, 100.0f };

    EXPECT_EQ (layer.solidColor, Color (0xff00ff00));
    EXPECT_FLOAT_EQ (layer.layerSize.getWidth(), 100.0f);
    EXPECT_FLOAT_EQ (layer.layerSize.getHeight(), 100.0f);
}
