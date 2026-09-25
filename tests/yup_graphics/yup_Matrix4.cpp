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

#include <yup_graphics/yup_graphics.h>

#include <rive/math/mat4.hpp>

using namespace yup;

class Matrix4Tests : public ::testing::Test
{
protected:
    static void expectNear (const Vector3<float>& actual, const Vector3<float>& expected, float tolerance = 1.0e-5f)
    {
        EXPECT_NEAR (actual.getX(), expected.getX(), tolerance);
        EXPECT_NEAR (actual.getY(), expected.getY(), tolerance);
        EXPECT_NEAR (actual.getZ(), expected.getZ(), tolerance);
    }

    static constexpr float halfPi = MathConstants<float>::halfPi;
};

TEST_F (Matrix4Tests, DefaultIsIdentity)
{
    const Matrix4 m;
    EXPECT_TRUE (m.isIdentity());

    for (int row = 0; row < 4; ++row)
        for (int column = 0; column < 4; ++column)
            EXPECT_FLOAT_EQ (m (row, column), row == column ? 1.0f : 0.0f);
}

TEST_F (Matrix4Tests, StorageIsColumnMajor)
{
    const auto m = Matrix4::translation ({ 1.0f, 2.0f, 3.0f });

    EXPECT_FLOAT_EQ (m.getData()[12], 1.0f);
    EXPECT_FLOAT_EQ (m.getData()[13], 2.0f);
    EXPECT_FLOAT_EQ (m.getData()[14], 3.0f);
    EXPECT_FLOAT_EQ (m (0, 3), 1.0f);
    EXPECT_FLOAT_EQ (m (1, 3), 2.0f);
    EXPECT_FLOAT_EQ (m (2, 3), 3.0f);
}

TEST_F (Matrix4Tests, TranslationAndScaling)
{
    expectNear (Matrix4::translation ({ 1.0f, 2.0f, 3.0f }).transformPoint ({ 1.0f, 1.0f, 1.0f }), { 2.0f, 3.0f, 4.0f });
    expectNear (Matrix4::scaling ({ 2.0f, 3.0f, 4.0f }).transformPoint ({ 1.0f, 1.0f, 1.0f }), { 2.0f, 3.0f, 4.0f });
    expectNear (Matrix4::scaling (2.0f).transformPoint ({ 1.0f, -1.0f, 0.5f }), { 2.0f, -2.0f, 1.0f });
}

TEST_F (Matrix4Tests, TransformVectorIgnoresTranslation)
{
    const auto m = Matrix4::translation ({ 10.0f, 20.0f, 30.0f }).followedBy (Matrix4::scaling (2.0f));
    expectNear (m.transformVector ({ 1.0f, 0.0f, 0.0f }), { 2.0f, 0.0f, 0.0f });
}

TEST_F (Matrix4Tests, AxisRotationsAreCounterClockwise)
{
    expectNear (Matrix4::rotationZ (halfPi).transformPoint ({ 1.0f, 0.0f, 0.0f }), { 0.0f, 1.0f, 0.0f });
    expectNear (Matrix4::rotationX (halfPi).transformPoint ({ 0.0f, 1.0f, 0.0f }), { 0.0f, 0.0f, 1.0f });
    expectNear (Matrix4::rotationY (halfPi).transformPoint ({ 0.0f, 0.0f, 1.0f }), { 1.0f, 0.0f, 0.0f });
}

TEST_F (Matrix4Tests, ArbitraryAxisRotationMatchesAxisRotations)
{
    EXPECT_TRUE (Matrix4::rotation ({ 0.0f, 0.0f, 2.0f }, 0.7f).approximatelyEqualTo (Matrix4::rotationZ (0.7f)));
    EXPECT_TRUE (Matrix4::rotation ({ 1.0f, 0.0f, 0.0f }, -1.1f).approximatelyEqualTo (Matrix4::rotationX (-1.1f)));
    EXPECT_TRUE (Matrix4::rotation ({ 0.0f, 3.0f, 0.0f }, 2.3f).approximatelyEqualTo (Matrix4::rotationY (2.3f)));
}

TEST_F (Matrix4Tests, RotationAroundZeroAxisIsIdentity)
{
    EXPECT_TRUE (Matrix4::rotation ({}, 1.0f).isIdentity());
}

TEST_F (Matrix4Tests, FollowedByAppliesThisFirst)
{
    const auto translate = Matrix4::translation ({ 1.0f, 0.0f, 0.0f });
    const auto scale = Matrix4::scaling (2.0f);

    expectNear (translate.followedBy (scale).transformPoint ({}), { 2.0f, 0.0f, 0.0f });
    expectNear (scale.followedBy (translate).transformPoint ({}), { 1.0f, 0.0f, 0.0f });
    EXPECT_EQ (translate * scale, translate.followedBy (scale));
}

TEST_F (Matrix4Tests, AgreesWithAffineTransformComposition)
{
    const auto a = AffineTransform::rotation (0.4f).scaled (2.0f, 0.5f).translated (3.0f, -7.0f);
    const auto b = AffineTransform::shearing (0.2f, 0.1f).translated (-1.0f, 4.0f);

    const auto combined = Matrix4::fromAffineTransform (a).followedBy (Matrix4::fromAffineTransform (b));
    EXPECT_TRUE (combined.approximatelyEqualTo (Matrix4::fromAffineTransform (a.followedBy (b))));

    const auto point = Point<float> (5.0f, 6.0f).transformed (a.followedBy (b));
    expectNear (combined.transformPoint ({ 5.0f, 6.0f, 9.0f }), { point.getX(), point.getY(), 9.0f }, 1.0e-4f);
}

TEST_F (Matrix4Tests, InverseRoundTrips)
{
    const auto m = Matrix4::rotation ({ 1.0f, 2.0f, 3.0f }, 0.8f)
                       .followedBy (Matrix4::scaling ({ 2.0f, 3.0f, 0.5f }))
                       .followedBy (Matrix4::translation ({ 4.0f, -5.0f, 6.0f }));

    EXPECT_TRUE (m.followedBy (m.inverted()).approximatelyEqualTo (Matrix4::identity(), 1.0e-5f));

    const Vector3<float> p (1.5f, -2.0f, 3.25f);
    expectNear (m.inverted().transformPoint (m.transformPoint (p)), p, 1.0e-4f);
}

TEST_F (Matrix4Tests, SingularInverseIsIdentity)
{
    EXPECT_TRUE (Matrix4::scaling ({ 1.0f, 0.0f, 1.0f }).inverted().isIdentity());
}

TEST_F (Matrix4Tests, TransposedSwapsRowsAndColumns)
{
    const auto m = Matrix4::translation ({ 1.0f, 2.0f, 3.0f });
    const auto t = m.transposed();

    EXPECT_FLOAT_EQ (t (3, 0), 1.0f);
    EXPECT_FLOAT_EQ (t (3, 1), 2.0f);
    EXPECT_FLOAT_EQ (t (3, 2), 3.0f);
    EXPECT_EQ (t.transposed(), m);
}

TEST_F (Matrix4Tests, PerspectiveMapsNearAndFarPlanes)
{
    const auto zeroToOne = Matrix4::perspective (halfPi, 2.0f, 1.0f, 10.0f, true);
    EXPECT_NEAR (zeroToOne.transformPoint ({ 0.0f, 0.0f, -1.0f }).getZ(), 0.0f, 1.0e-5f);
    EXPECT_NEAR (zeroToOne.transformPoint ({ 0.0f, 0.0f, -10.0f }).getZ(), 1.0f, 1.0e-5f);

    const auto minusOneToOne = Matrix4::perspective (halfPi, 2.0f, 1.0f, 10.0f, false);
    EXPECT_NEAR (minusOneToOne.transformPoint ({ 0.0f, 0.0f, -1.0f }).getZ(), -1.0f, 1.0e-5f);
    EXPECT_NEAR (minusOneToOne.transformPoint ({ 0.0f, 0.0f, -10.0f }).getZ(), 1.0f, 1.0e-5f);

    // A 90 degree field of view puts the frustum edge at x = aspect * depth, y = depth
    const auto edge = zeroToOne.transformPoint ({ 4.0f, 2.0f, -2.0f });
    EXPECT_NEAR (edge.getX(), 1.0f, 1.0e-5f);
    EXPECT_NEAR (edge.getY(), 1.0f, 1.0e-5f);
}

TEST_F (Matrix4Tests, PointsBehindTheCameraHaveNegativeW)
{
    const auto projection = Matrix4::perspective (halfPi, 1.0f, 0.1f, 100.0f);

    EXPECT_GT (projection.transformPoint4 (0.0f, 0.0f, -5.0f, 1.0f)[3], 0.0f);
    EXPECT_LT (projection.transformPoint4 (0.0f, 0.0f, 5.0f, 1.0f)[3], 0.0f);
}

TEST_F (Matrix4Tests, OrthographicMapsTheViewVolumeToNdc)
{
    const auto ortho = Matrix4::orthographic (-2.0f, 2.0f, -1.0f, 1.0f, 1.0f, 3.0f);

    expectNear (ortho.transformPoint ({ -2.0f, -1.0f, -1.0f }), { -1.0f, -1.0f, 0.0f });
    expectNear (ortho.transformPoint ({ 2.0f, 1.0f, -3.0f }), { 1.0f, 1.0f, 1.0f });
}

TEST_F (Matrix4Tests, LookAtMovesTheEyeToTheOriginLookingDownNegativeZ)
{
    const auto view = Matrix4::lookAt ({ 0.0f, 0.0f, 5.0f }, {}, { 0.0f, 1.0f, 0.0f });

    expectNear (view.transformPoint ({ 0.0f, 0.0f, 5.0f }), {});
    expectNear (view.transformPoint ({}), { 0.0f, 0.0f, -5.0f });
    expectNear (view.transformPoint ({ 1.0f, 2.0f, 0.0f }), { 1.0f, 2.0f, -5.0f });

    const auto sideView = Matrix4::lookAt ({ 5.0f, 0.0f, 0.0f }, {}, { 0.0f, 1.0f, 0.0f });
    expectNear (sideView.transformPoint ({ 0.0f, 0.0f, -1.0f }), { 1.0f, 0.0f, -5.0f });
}

TEST_F (Matrix4Tests, ConvertsToAndFromRiveMat4)
{
    const auto m = Matrix4::rotationY (0.3f).followedBy (Matrix4::translation ({ 1.0f, 2.0f, 3.0f }));
    const auto mat4 = m.toMat4();

    for (std::size_t i = 0; i < 16; ++i)
        EXPECT_FLOAT_EQ (mat4[i], m.getData()[i]);

    EXPECT_EQ (Matrix4 (mat4), m);
}
