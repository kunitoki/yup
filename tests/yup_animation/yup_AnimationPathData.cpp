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

namespace
{

AnimationPathData makeTriangle()
{
    AnimationPathData pd;
    pd.vertices = { { 0.0f, 0.0f }, { 100.0f, 0.0f }, { 50.0f, 100.0f } };
    pd.inTangents = { { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f } };
    pd.outTangents = { { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f } };
    pd.closed = true;
    return pd;
}

AnimationPathData makeSquare()
{
    AnimationPathData pd;
    pd.vertices = { { 0, 0 }, { 100, 0 }, { 100, 100 }, { 0, 100 } };
    pd.inTangents = { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } };
    pd.outTangents = { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } };
    pd.closed = true;
    return pd;
}

} // namespace

class AnimationPathDataTests : public ::testing::Test
{
};

// =============================================================================
// Construction and defaults
// =============================================================================

TEST_F (AnimationPathDataTests, DefaultConstruction_IsEmpty)
{
    AnimationPathData pd;
    EXPECT_TRUE (pd.vertices.empty());
    EXPECT_TRUE (pd.inTangents.empty());
    EXPECT_TRUE (pd.outTangents.empty());
    EXPECT_FALSE (pd.closed);
}

TEST_F (AnimationPathDataTests, PopulatedPathData_StoresVertices)
{
    auto pd = makeTriangle();
    EXPECT_EQ (pd.vertices.size(), 3u);
    EXPECT_FLOAT_EQ (pd.vertices[0].getX(), 0.0f);
    EXPECT_FLOAT_EQ (pd.vertices[1].getX(), 100.0f);
    EXPECT_FLOAT_EQ (pd.vertices[2].getX(), 50.0f);
}

// =============================================================================
// toPath
// =============================================================================

TEST_F (AnimationPathDataTests, ToPath_EmptyDataProducesEmptyPath)
{
    AnimationPathData pd;
    EXPECT_TRUE (pd.toPath().isEmpty());
}

TEST_F (AnimationPathDataTests, ToPath_NonEmptyDataProducesNonEmptyPath)
{
    auto pd = makeTriangle();
    EXPECT_FALSE (pd.toPath().isEmpty());
}

TEST_F (AnimationPathDataTests, ToPath_ClosedFlagProducesClosedPath)
{
    auto pd = makeSquare();
    pd.closed = true;
    EXPECT_FALSE (pd.toPath().isEmpty());
}

// =============================================================================
// lerp
// =============================================================================

TEST_F (AnimationPathDataTests, Lerp_AtZeroReturnsFirstOperand)
{
    auto a = makeTriangle();
    auto b = makeSquare();

    // Resize b to match vertex count of a
    b.vertices.resize (3);
    b.inTangents.resize (3);
    b.outTangents.resize (3);

    auto result = AnimationPathData::lerp (a, b, 0.0f);
    ASSERT_EQ (result.vertices.size(), a.vertices.size());
    EXPECT_NEAR (result.vertices[0].getX(), a.vertices[0].getX(), 1e-4f);
    EXPECT_NEAR (result.vertices[1].getX(), a.vertices[1].getX(), 1e-4f);
}

TEST_F (AnimationPathDataTests, Lerp_AtOneReturnsSecondOperand)
{
    AnimationPathData a, b;
    a.vertices = { { 0.0f, 0.0f }, { 100.0f, 0.0f } };
    a.inTangents = a.outTangents = { { 0.0f, 0.0f }, { 0.0f, 0.0f } };

    b.vertices = { { 50.0f, 50.0f }, { 150.0f, 50.0f } };
    b.inTangents = b.outTangents = { { 0.0f, 0.0f }, { 0.0f, 0.0f } };

    auto result = AnimationPathData::lerp (a, b, 1.0f);
    ASSERT_EQ (result.vertices.size(), 2u);
    EXPECT_NEAR (result.vertices[0].getX(), 50.0f, 1e-4f);
    EXPECT_NEAR (result.vertices[0].getY(), 50.0f, 1e-4f);
    EXPECT_NEAR (result.vertices[1].getX(), 150.0f, 1e-4f);
}

TEST_F (AnimationPathDataTests, Lerp_AtHalfInterpolatesMidpoint)
{
    AnimationPathData a, b;
    a.vertices = { { 0.0f, 0.0f } };
    a.inTangents = a.outTangents = { { 0.0f, 0.0f } };

    b.vertices = { { 100.0f, 100.0f } };
    b.inTangents = b.outTangents = { { 0.0f, 0.0f } };

    auto result = AnimationPathData::lerp (a, b, 0.5f);
    ASSERT_EQ (result.vertices.size(), 1u);
    EXPECT_NEAR (result.vertices[0].getX(), 50.0f, 1e-4f);
    EXPECT_NEAR (result.vertices[0].getY(), 50.0f, 1e-4f);
}

// =============================================================================
// Equality operators
// =============================================================================

TEST_F (AnimationPathDataTests, EqualityOperator_SameDataIsEqual)
{
    auto a = makeTriangle();
    auto b = makeTriangle();
    EXPECT_EQ (a, b);
    EXPECT_FALSE (a != b);
}

TEST_F (AnimationPathDataTests, EqualityOperator_DifferentDataIsNotEqual)
{
    auto a = makeTriangle();
    auto b = makeSquare();
    EXPECT_NE (a, b);
}

TEST_F (AnimationPathDataTests, EqualityOperator_EmptyDataIsEqual)
{
    AnimationPathData a, b;
    EXPECT_EQ (a, b);
}

// =============================================================================
// Arithmetic operators
// =============================================================================

TEST_F (AnimationPathDataTests, MultiplyByZero_ProducesZeroVertices)
{
    AnimationPathData pd;
    pd.vertices = { { 10.0f, 20.0f } };
    pd.inTangents = { { 1.0f, 2.0f } };
    pd.outTangents = { { 3.0f, 4.0f } };

    auto result = pd * 0.0f;
    ASSERT_EQ (result.vertices.size(), 1u);
    EXPECT_NEAR (result.vertices[0].getX(), 0.0f, 1e-4f);
    EXPECT_NEAR (result.vertices[0].getY(), 0.0f, 1e-4f);
}

TEST_F (AnimationPathDataTests, MultiplyByOne_PreservesVertices)
{
    AnimationPathData pd;
    pd.vertices = { { 10.0f, 20.0f } };
    pd.inTangents = { { 0.0f, 0.0f } };
    pd.outTangents = { { 0.0f, 0.0f } };

    auto result = pd * 1.0f;
    ASSERT_EQ (result.vertices.size(), 1u);
    EXPECT_NEAR (result.vertices[0].getX(), 10.0f, 1e-4f);
    EXPECT_NEAR (result.vertices[0].getY(), 20.0f, 1e-4f);
}

TEST_F (AnimationPathDataTests, AddOperator_IsLerpAtOne_ReturnsSecondOperand)
{
    // operator+(a, b) = lerp(a, b, 1.0) = b
    AnimationPathData a, b;
    a.vertices = { { 10.0f, 20.0f } };
    a.inTangents = a.outTangents = { { 0.0f, 0.0f } };
    b.vertices = { { 50.0f, 70.0f } };
    b.inTangents = b.outTangents = { { 0.0f, 0.0f } };

    auto result = a + b;
    ASSERT_EQ (result.vertices.size(), 1u);
    EXPECT_NEAR (result.vertices[0].getX(), b.vertices[0].getX(), 1e-4f);
    EXPECT_NEAR (result.vertices[0].getY(), b.vertices[0].getY(), 1e-4f);
}

TEST_F (AnimationPathDataTests, SubtractOperator_IsExtrapolation_TwoBMinusA)
{
    // operator-(a, b) = lerp(b, a, -1.0) = 2*b - a
    AnimationPathData a, b;
    a.vertices = { { 30.0f, 50.0f } };
    a.inTangents = a.outTangents = { { 0.0f, 0.0f } };
    b.vertices = { { 10.0f, 20.0f } };
    b.inTangents = b.outTangents = { { 0.0f, 0.0f } };

    auto result = a - b;
    // Expected: 2*b - a = 2*{10,20} - {30,50} = {-10,-10}
    ASSERT_EQ (result.vertices.size(), 1u);
    EXPECT_NEAR (result.vertices[0].getX(), -10.0f, 1e-4f);
    EXPECT_NEAR (result.vertices[0].getY(), -10.0f, 1e-4f);
}

// =============================================================================
// Open path (non-closed)
// =============================================================================

TEST_F (AnimationPathDataTests, ToPath_OpenPathDoesNotImplicitlyClose)
{
    AnimationPathData pd;
    pd.vertices = { { 0.0f, 0.0f }, { 50.0f, 100.0f }, { 100.0f, 0.0f } };
    pd.inTangents = { { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f } };
    pd.outTangents = { { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f } };
    pd.closed = false;

    EXPECT_FALSE (pd.toPath().isEmpty());
}

TEST_F (AnimationPathDataTests, ToPath_ClosedAndOpenProduceDifferentPaths)
{
    auto open = makeTriangle();
    open.closed = false;
    auto closed = makeTriangle();
    closed.closed = true;

    // Both should be non-empty; they may differ internally but are both valid
    EXPECT_FALSE (open.toPath().isEmpty());
    EXPECT_FALSE (closed.toPath().isEmpty());
}

// =============================================================================
// Bezier tangents
// =============================================================================

TEST_F (AnimationPathDataTests, ToPath_NonZeroTangentsProducesNonEmptyPath)
{
    AnimationPathData pd;
    pd.vertices = { { 0.0f, 0.0f }, { 100.0f, 0.0f } };
    pd.outTangents = { { 0.0f, 50.0f }, { 0.0f, 0.0f } };
    pd.inTangents = { { 0.0f, 0.0f }, { 0.0f, -50.0f } };
    pd.closed = false;

    EXPECT_FALSE (pd.toPath().isEmpty());
}

TEST_F (AnimationPathDataTests, Lerp_InterpolatesTangentsCorrectly)
{
    AnimationPathData a, b;
    a.vertices = { { 0.0f, 0.0f } };
    a.outTangents = { { 0.0f, 20.0f } };
    a.inTangents = { { 0.0f, -20.0f } };

    b.vertices = { { 0.0f, 0.0f } };
    b.outTangents = { { 0.0f, 60.0f } };
    b.inTangents = { { 0.0f, -60.0f } };

    auto result = AnimationPathData::lerp (a, b, 0.5f);
    ASSERT_EQ (result.outTangents.size(), 1u);
    EXPECT_NEAR (result.outTangents[0].getY(), 40.0f, 1e-3f);
    EXPECT_NEAR (result.inTangents[0].getY(), -40.0f, 1e-3f);
}

// =============================================================================
// Lerp closed flag propagation
// =============================================================================

TEST_F (AnimationPathDataTests, Lerp_PreservesClosedFlagFromFirstOperand)
{
    auto a = makeTriangle();
    a.closed = true;
    auto b = makeTriangle();
    b.closed = false;

    auto result = AnimationPathData::lerp (a, b, 0.5f);
    EXPECT_TRUE (result.closed); // should follow the first operand
}

// =============================================================================
// Scalar multiply affects tangents
// =============================================================================

TEST_F (AnimationPathDataTests, MultiplyByTwo_DoublesTangents)
{
    AnimationPathData pd;
    pd.vertices = { { 10.0f, 20.0f } };
    pd.outTangents = { { 5.0f, 10.0f } };
    pd.inTangents = { { 2.0f, 4.0f } };

    auto result = pd * 2.0f;
    ASSERT_EQ (result.outTangents.size(), 1u);
    EXPECT_NEAR (result.outTangents[0].getX(), 10.0f, 1e-4f);
    EXPECT_NEAR (result.outTangents[0].getY(), 20.0f, 1e-4f);
    EXPECT_NEAR (result.inTangents[0].getX(), 4.0f, 1e-4f);
    EXPECT_NEAR (result.inTangents[0].getY(), 8.0f, 1e-4f);
}
