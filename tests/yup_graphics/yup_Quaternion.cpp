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

using namespace yup;

class QuaternionTests : public ::testing::Test
{
protected:
    static void expectNear (const Vector3<float>& actual, const Vector3<float>& expected, float tolerance = 1.0e-5f)
    {
        EXPECT_NEAR (actual.getX(), expected.getX(), tolerance);
        EXPECT_NEAR (actual.getY(), expected.getY(), tolerance);
        EXPECT_NEAR (actual.getZ(), expected.getZ(), tolerance);
    }

    static void expectSameRotation (const Quaternion& a, const Quaternion& b, float tolerance = 1.0e-5f)
    {
        // q and -q describe the same rotation
        const auto sign = (a.getX() * b.getX() + a.getY() * b.getY() + a.getZ() * b.getZ() + a.getW() * b.getW()) < 0.0f ? -1.0f : 1.0f;

        EXPECT_NEAR (a.getX(), sign * b.getX(), tolerance);
        EXPECT_NEAR (a.getY(), sign * b.getY(), tolerance);
        EXPECT_NEAR (a.getZ(), sign * b.getZ(), tolerance);
        EXPECT_NEAR (a.getW(), sign * b.getW(), tolerance);
    }

    static constexpr float halfPi = MathConstants<float>::halfPi;
    static constexpr float pi = MathConstants<float>::pi;
};

TEST_F (QuaternionTests, DefaultIsIdentity)
{
    const Quaternion q;

    EXPECT_FLOAT_EQ (q.getX(), 0.0f);
    EXPECT_FLOAT_EQ (q.getY(), 0.0f);
    EXPECT_FLOAT_EQ (q.getZ(), 0.0f);
    EXPECT_FLOAT_EQ (q.getW(), 1.0f);
    EXPECT_EQ (q, Quaternion::identity());
    EXPECT_TRUE (q.toMatrix4().isIdentity());
    expectNear (q.rotate ({ 1.0f, 2.0f, 3.0f }), { 1.0f, 2.0f, 3.0f });
}

TEST_F (QuaternionTests, AxisAngleRotatesCounterClockwise)
{
    expectNear (Quaternion::fromAxisAngle ({ 0.0f, 0.0f, 1.0f }, halfPi).rotate ({ 1.0f, 0.0f, 0.0f }), { 0.0f, 1.0f, 0.0f });
    expectNear (Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, halfPi).rotate ({ 0.0f, 1.0f, 0.0f }), { 0.0f, 0.0f, 1.0f });
    expectNear (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, halfPi).rotate ({ 0.0f, 0.0f, 1.0f }), { 1.0f, 0.0f, 0.0f });
}

TEST_F (QuaternionTests, AxisAngleNormalizesAxisAndHandlesZeroAxis)
{
    expectNear (Quaternion::fromAxisAngle ({ 0.0f, 0.0f, 5.0f }, halfPi).rotate ({ 1.0f, 0.0f, 0.0f }), { 0.0f, 1.0f, 0.0f });
    EXPECT_EQ (Quaternion::fromAxisAngle ({}, 1.0f), Quaternion::identity());
}

TEST_F (QuaternionTests, ToMatrix4MatchesMatrixRotation)
{
    const Vector3<float> axis { 1.0f, -2.0f, 0.5f };
    const auto angle = 0.73f;

    EXPECT_TRUE (Quaternion::fromAxisAngle (axis, angle).toMatrix4().approximatelyEqualTo (Matrix4::rotation (axis, angle)));
    EXPECT_TRUE (Quaternion::fromAxisAngle ({ 0.0f, 0.0f, 1.0f }, 1.2f).toMatrix4().approximatelyEqualTo (Matrix4::rotationZ (1.2f)));
}

TEST_F (QuaternionTests, RotateMatchesMatrix)
{
    const auto q = Quaternion::fromAxisAngle ({ 0.3f, 0.9f, -0.2f }, 2.1f);
    const Vector3<float> v { 0.4f, -1.5f, 2.25f };

    expectNear (q.rotate (v), q.toMatrix4().transformPoint (v));
}

TEST_F (QuaternionTests, FromRotationMatrixRoundTrips)
{
    const Quaternion rotations[] = {
        Quaternion::identity(),
        Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, pi),
        Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, pi),
        Quaternion::fromAxisAngle ({ 0.0f, 0.0f, 1.0f }, pi),
        Quaternion::fromAxisAngle ({ 1.0f, 1.0f, 1.0f }, 2.5f),
        Quaternion::fromAxisAngle ({ -0.2f, 0.7f, 0.1f }, -0.4f),
    };

    for (const auto& q : rotations)
        expectSameRotation (Quaternion::fromRotationMatrix (q.toMatrix4()), q);
}

TEST_F (QuaternionTests, FromRotationMatrixIgnoresTranslation)
{
    const auto q = Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 0.8f);
    const auto m = q.toMatrix4().followedBy (Matrix4::translation ({ 4.0f, 5.0f, 6.0f }));

    expectSameRotation (Quaternion::fromRotationMatrix (m), q);
}

TEST_F (QuaternionTests, CompositionAppliesRightOperandFirst)
{
    const auto a = Quaternion::fromAxisAngle ({ 0.0f, 0.0f, 1.0f }, halfPi);
    const auto b = Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, halfPi);
    const Vector3<float> v { 0.0f, 1.0f, 0.0f };

    // b first: (0,1,0) -> (0,0,1), then a leaves it on z
    expectNear ((a * b).rotate (v), a.rotate (b.rotate (v)));
    expectNear ((a * b).rotate (v), { 0.0f, 0.0f, 1.0f });

    EXPECT_TRUE ((a * b).toMatrix4().approximatelyEqualTo (b.toMatrix4().followedBy (a.toMatrix4())));
}

TEST_F (QuaternionTests, ConjugateIsInverseForUnitQuaternions)
{
    const auto q = Quaternion::fromAxisAngle ({ 0.5f, 0.5f, -1.0f }, 1.1f);

    expectSameRotation (q * q.conjugate(), Quaternion::identity());
    expectNear (q.conjugate().rotate (q.rotate ({ 1.0f, 2.0f, 3.0f })), { 1.0f, 2.0f, 3.0f });
}

TEST_F (QuaternionTests, NormalizedHasUnitLength)
{
    const auto q = Quaternion (1.0f, 2.0f, 3.0f, 4.0f).normalized();

    EXPECT_NEAR (q.length(), 1.0f, 1.0e-6f);
    EXPECT_EQ (Quaternion (0.0f, 0.0f, 0.0f, 0.0f).normalized(), Quaternion::identity());
}

TEST_F (QuaternionTests, FromLookDirectionTurnsMinusZToTheDirection)
{
    for (const auto direction : { Vector3<float> { 1.0f, 0.0f, 0.0f }, Vector3<float> { 0.0f, 0.0f, 3.0f }, Vector3<float> { 1.0f, 1.0f, -1.0f } })
    {
        const auto q = Quaternion::fromLookDirection (direction);

        expectNear (q.rotate ({ 0.0f, 0.0f, -1.0f }), direction.normalized());
        EXPECT_GT (q.rotate ({ 0.0f, 1.0f, 0.0f }).getY(), 0.0f);
        EXPECT_NEAR (q.rotate ({ 1.0f, 0.0f, 0.0f }).getY(), 0.0f, 1.0e-5f);
    }
}

TEST_F (QuaternionTests, FromLookDirectionHandlesAnUpAlongTheDirection)
{
    expectNear (Quaternion::fromLookDirection ({ 0.0f, 2.0f, 0.0f }).rotate ({ 0.0f, 0.0f, -1.0f }), { 0.0f, 1.0f, 0.0f });
    expectNear (Quaternion::fromLookDirection ({ 0.0f, -1.0f, 0.0f }).rotate ({ 0.0f, 0.0f, -1.0f }), { 0.0f, -1.0f, 0.0f });
    EXPECT_EQ (Quaternion::fromLookDirection ({}), Quaternion::identity());
}

TEST_F (QuaternionTests, SlerpReturnsTheEndsAndHalvesTheAngle)
{
    const auto a = Quaternion::identity();
    const auto b = Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, halfPi);

    expectSameRotation (a.slerp (b, 0.0f), a);
    expectSameRotation (a.slerp (b, 1.0f), b);
    expectSameRotation (a.slerp (b, 0.5f), Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, halfPi * 0.5f));
    expectSameRotation (a.slerp (b, 0.25f), Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, halfPi * 0.25f));
}

TEST_F (QuaternionTests, SlerpAndNlerpTakeTheShorterWay)
{
    const auto a = Quaternion::identity();
    const auto b = Quaternion::fromAxisAngle ({ 0.0f, 0.0f, 1.0f }, halfPi);
    const Quaternion negated (-b.getX(), -b.getY(), -b.getZ(), -b.getW());

    expectSameRotation (a.slerp (negated, 0.5f), a.slerp (b, 0.5f));
    expectSameRotation (a.nlerp (negated, 0.5f), a.nlerp (b, 0.5f));
}

TEST_F (QuaternionTests, NlerpIsNormalizedAndMatchesSlerpAtTheEndsAndHalfway)
{
    const auto a = Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, 0.3f);
    const auto b = Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 1.2f);

    for (const auto t : { 0.0f, 0.5f, 1.0f })
        expectSameRotation (a.nlerp (b, t), a.slerp (b, t));

    EXPECT_NEAR (a.nlerp (b, 0.3f).length(), 1.0f, 1.0e-5f);
}

TEST_F (QuaternionTests, SlerpOfNearlyEqualRotationsStaysNormalized)
{
    const auto a = Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 0.5f);
    const auto b = Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 0.5001f);

    EXPECT_NEAR (a.slerp (b, 0.5f).length(), 1.0f, 1.0e-5f);
    expectSameRotation (a.slerp (b, 0.5f), Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 0.50005f));
}
