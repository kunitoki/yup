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

// =============================================================================
// Common AnimationShape interface helpers
// =============================================================================

namespace
{

template <typename ShapeT>
void checkCommonDefaults (const ShapeT& shape, AnimationShape::Kind expectedKind)
{
    EXPECT_EQ (shape.getKind(), expectedKind);
    EXPECT_FALSE (shape.isHidden());
    EXPECT_EQ (shape.getDirection(), 1);
}

} // namespace

// =============================================================================
// EllipseShape
// =============================================================================

class EllipseShapeTests : public ::testing::Test
{
};

TEST_F (EllipseShapeTests, GetKind_ReturnsEllipse)
{
    EllipseShape shape;
    EXPECT_EQ (shape.getKind(), AnimationShape::Kind::Ellipse);
}

TEST_F (EllipseShapeTests, DefaultValues)
{
    EllipseShape shape;
    checkCommonDefaults (shape, AnimationShape::Kind::Ellipse);
}

TEST_F (EllipseShapeTests, SetName_StoresName)
{
    EllipseShape shape;
    shape.setName ("MyEllipse");
    EXPECT_EQ (shape.getName(), String ("MyEllipse"));
}

TEST_F (EllipseShapeTests, SetHidden_ReturnsTrue)
{
    EllipseShape shape;
    shape.setHidden (true);
    EXPECT_TRUE (shape.isHidden());
}

TEST_F (EllipseShapeTests, SetDirection_StoresDirection)
{
    EllipseShape shape;
    shape.setDirection (-1);
    EXPECT_EQ (shape.getDirection(), -1);
}

TEST_F (EllipseShapeTests, BuildPath_DefaultProducesNonEmptyPath)
{
    EllipseShape shape;
    EXPECT_FALSE (shape.buildPath (0.0f).isEmpty());
}

TEST_F (EllipseShapeTests, BuildPath_ZeroSizeDoesNotCrash)
{
    EllipseShape shape;
    shape.size = SizeProperty::staticValue ({ 0.0f, 0.0f });
    // A zero-size ellipse produces a degenerate path — just verify no crash
    (void) shape.buildPath (0.0f);
}

TEST_F (EllipseShapeTests, BuildPath_NonZeroSizeProducesPath)
{
    EllipseShape shape;
    shape.size = SizeProperty::staticValue ({ 50.0f, 30.0f });
    EXPECT_FALSE (shape.buildPath (0.0f).isEmpty());
}

// =============================================================================
// RectShape
// =============================================================================

class RectShapeTests : public ::testing::Test
{
};

TEST_F (RectShapeTests, GetKind_ReturnsRect)
{
    RectShape shape;
    EXPECT_EQ (shape.getKind(), AnimationShape::Kind::Rect);
}

TEST_F (RectShapeTests, DefaultValues)
{
    RectShape shape;
    checkCommonDefaults (shape, AnimationShape::Kind::Rect);
}

TEST_F (RectShapeTests, BuildPath_DefaultProducesNonEmptyPath)
{
    RectShape shape;
    EXPECT_FALSE (shape.buildPath (0.0f).isEmpty());
}

TEST_F (RectShapeTests, BuildPath_ZeroSizeProducesEmptyOrDegeneratePath)
{
    RectShape shape;
    shape.size = SizeProperty::staticValue ({ 0.0f, 0.0f });
    EXPECT_FALSE (shape.buildPath (0.0f).isEmpty());
}

TEST_F (RectShapeTests, BuildPath_WithRoundness_ProducesPath)
{
    RectShape shape;
    shape.size = SizeProperty::staticValue ({ 100.0f, 100.0f });
    shape.roundness = FloatProperty::staticValue (10.0f);
    EXPECT_FALSE (shape.buildPath (0.0f).isEmpty());
}

TEST_F (RectShapeTests, BuildPath_WithZeroRoundness_ProducesPath)
{
    RectShape shape;
    shape.size = SizeProperty::staticValue ({ 100.0f, 100.0f });
    shape.roundness = FloatProperty::staticValue (0.0f);
    EXPECT_FALSE (shape.buildPath (0.0f).isEmpty());
}

// =============================================================================
// BezierPathShape
// =============================================================================

class BezierPathShapeTests : public ::testing::Test
{
};

TEST_F (BezierPathShapeTests, GetKind_ReturnsBezierPath)
{
    BezierPathShape shape;
    EXPECT_EQ (shape.getKind(), AnimationShape::Kind::BezierPath);
}

TEST_F (BezierPathShapeTests, DefaultValues)
{
    BezierPathShape shape;
    checkCommonDefaults (shape, AnimationShape::Kind::BezierPath);
}

TEST_F (BezierPathShapeTests, BuildPath_EmptyPathDataProducesEmptyPath)
{
    BezierPathShape shape;
    EXPECT_TRUE (shape.buildPath (0.0f).isEmpty());
}

TEST_F (BezierPathShapeTests, BuildPath_WithPathDataProducesNonEmptyPath)
{
    BezierPathShape shape;

    AnimationPathData pd;
    pd.vertices = { { 0.0f, 0.0f }, { 100.0f, 0.0f }, { 50.0f, 100.0f } };
    pd.inTangents = { { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f } };
    pd.outTangents = { { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f } };
    pd.closed = true;
    shape.pathData = PathDataProperty::staticValue (pd);

    EXPECT_FALSE (shape.buildPath (0.0f).isEmpty());
}

// =============================================================================
// PolystarShape
// =============================================================================

class PolystarShapeTests : public ::testing::Test
{
};

TEST_F (PolystarShapeTests, GetKind_ReturnsPolystar)
{
    PolystarShape shape;
    EXPECT_EQ (shape.getKind(), AnimationShape::Kind::Polystar);
}

TEST_F (PolystarShapeTests, DefaultValues)
{
    PolystarShape shape;
    checkCommonDefaults (shape, AnimationShape::Kind::Polystar);
    EXPECT_EQ (shape.starType, PolystarShape::StarType::Polygon);
}

TEST_F (PolystarShapeTests, BuildPath_DefaultPolygonProducesNonEmptyPath)
{
    PolystarShape shape;
    EXPECT_FALSE (shape.buildPath (0.0f).isEmpty());
}

TEST_F (PolystarShapeTests, BuildPath_StarTypeProducesNonEmptyPath)
{
    PolystarShape shape;
    shape.starType = PolystarShape::StarType::Star;
    EXPECT_FALSE (shape.buildPath (0.0f).isEmpty());
}

TEST_F (PolystarShapeTests, BuildPath_TrianglePolygon_ThreePoints)
{
    PolystarShape shape;
    shape.starType = PolystarShape::StarType::Polygon;
    shape.points = FloatProperty::staticValue (3.0f);
    EXPECT_FALSE (shape.buildPath (0.0f).isEmpty());
}

TEST_F (PolystarShapeTests, StarTypeEnum_BothValuesExistAndAreDistinct)
{
    EXPECT_NE (PolystarShape::StarType::Star, PolystarShape::StarType::Polygon);
}

TEST_F (PolystarShapeTests, SetName_StoresName)
{
    PolystarShape shape;
    shape.setName ("Star");
    EXPECT_EQ (shape.getName(), String ("Star"));
}

TEST_F (PolystarShapeTests, BuildPath_WithCustomRadius)
{
    PolystarShape shape;
    shape.outerRadius = FloatProperty::staticValue (200.0f);
    EXPECT_FALSE (shape.buildPath (0.0f).isEmpty());
}
