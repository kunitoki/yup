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

class AnimationClipTests : public ::testing::Test
{
protected:
    using Path = AnimationClip::Path;
    using Interpolation = AnimationClip::Interpolation;

    static AnimationClip::Track makeTrack (Path path, Interpolation interpolation, std::vector<float> times, std::vector<float> values)
    {
        AnimationClip::Track track;
        track.path = path;
        track.interpolation = interpolation;
        track.times = std::move (times);
        track.values = std::move (values);
        return track;
    }

    /** The value of a track at a time, written over values that no track of these tests holds. */
    static std::vector<float> sample (const AnimationClip::Track& track, float time)
    {
        std::vector<float> values (static_cast<size_t> (track.getNumComponents()), -1000.0f);
        track.sample (time, values);
        return values;
    }

    static std::vector<float> toValues (const Quaternion& rotation)
    {
        return { rotation.getX(), rotation.getY(), rotation.getZ(), rotation.getW() };
    }

    static void expectNear (const std::vector<float>& actual, const std::vector<float>& expected, float tolerance = 1.0e-5f)
    {
        ASSERT_EQ (actual.size(), expected.size());

        for (size_t i = 0; i < expected.size(); ++i)
            EXPECT_NEAR (actual[i], expected[i], tolerance) << "component " << i;
    }
};

//==============================================================================

TEST_F (AnimationClipTests, NewClipHasANameAndNoTracks)
{
    const AnimationClip clip ("walk");

    EXPECT_EQ (clip.getName(), "walk");
    EXPECT_TRUE (clip.getTracks().empty());
    EXPECT_FLOAT_EQ (clip.getDuration(), 0.0f);
}

TEST_F (AnimationClipTests, DurationIsTheLatestKeyOfTheTracks)
{
    AnimationClip clip;

    EXPECT_TRUE (clip.addTrack (makeTrack (Path::scale, Interpolation::step, { 0.5f, 2.0f }, { 1.0f, 1.0f, 1.0f, 2.0f, 2.0f, 2.0f })).wasOk());
    EXPECT_FLOAT_EQ (clip.getDuration(), 2.0f);

    EXPECT_TRUE (clip.addTrack (makeTrack (Path::translation, Interpolation::linear, { 5.0f }, { 0.0f, 0.0f, 0.0f })).wasOk());
    EXPECT_TRUE (clip.addTrack (makeTrack (Path::rotation, Interpolation::linear, { 3.0f }, { 0.0f, 0.0f, 0.0f, 1.0f })).wasOk());
    EXPECT_FLOAT_EQ (clip.getDuration(), 5.0f);

    ASSERT_EQ (clip.getTracks().size(), 3u);
    EXPECT_EQ (clip.getTracks()[0].path, Path::scale);
    EXPECT_EQ (clip.getTracks()[1].path, Path::translation);
    EXPECT_EQ (clip.getTracks()[2].path, Path::rotation);
}

TEST_F (AnimationClipTests, TracksThatCannotBePlayedAreRejected)
{
    AnimationClip clip;
    const std::vector<float> oneVector { 0.0f, 0.0f, 0.0f };
    const std::vector<float> twoVectors { 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f };

    auto negativeTarget = makeTrack (Path::translation, Interpolation::linear, { 0.0f }, oneVector);
    negativeTarget.target = -1;
    EXPECT_TRUE (clip.addTrack (negativeTarget).failed());

    // No keys, then times going back, starting before 0 and not finite
    EXPECT_TRUE (clip.addTrack (makeTrack (Path::translation, Interpolation::linear, {}, {})).failed());
    EXPECT_TRUE (clip.addTrack (makeTrack (Path::translation, Interpolation::linear, { 1.0f, 0.5f }, twoVectors)).failed());
    EXPECT_TRUE (clip.addTrack (makeTrack (Path::translation, Interpolation::linear, { -1.0f, 0.5f }, twoVectors)).failed());
    EXPECT_TRUE (clip.addTrack (makeTrack (Path::translation, Interpolation::linear, { 0.0f, std::numeric_limits<float>::quiet_NaN() }, twoVectors)).failed());

    // Values that are not what the keys take: a vector for two keys, a vector for a rotation,
    // values without their tangents, three weights for two keys and no weights at all
    EXPECT_TRUE (clip.addTrack (makeTrack (Path::translation, Interpolation::linear, { 0.0f, 1.0f }, oneVector)).failed());
    EXPECT_TRUE (clip.addTrack (makeTrack (Path::rotation, Interpolation::linear, { 0.0f }, oneVector)).failed());
    EXPECT_TRUE (clip.addTrack (makeTrack (Path::translation, Interpolation::cubicSpline, { 0.0f, 1.0f }, twoVectors)).failed());
    EXPECT_TRUE (clip.addTrack (makeTrack (Path::weights, Interpolation::linear, { 0.0f, 1.0f }, oneVector)).failed());
    EXPECT_TRUE (clip.addTrack (makeTrack (Path::weights, Interpolation::linear, { 0.0f, 1.0f }, {})).failed());

    EXPECT_TRUE (clip.getTracks().empty());
    EXPECT_FLOAT_EQ (clip.getDuration(), 0.0f);
}

//==============================================================================

TEST_F (AnimationClipTests, StepHoldsEachKeyUntilTheNextOne)
{
    const auto track = makeTrack (Path::translation, Interpolation::step, { 1.0f, 2.0f, 4.0f }, { 5.0f, 6.0f, 7.0f, 1.0f, 2.0f, 3.0f, 10.0f, 20.0f, 30.0f });

    expectNear (sample (track, 1.0f), { 5.0f, 6.0f, 7.0f });
    expectNear (sample (track, 1.999f), { 5.0f, 6.0f, 7.0f });
    expectNear (sample (track, 2.0f), { 1.0f, 2.0f, 3.0f });
    expectNear (sample (track, 3.5f), { 1.0f, 2.0f, 3.0f });
    expectNear (sample (track, 4.0f), { 10.0f, 20.0f, 30.0f });
}

TEST_F (AnimationClipTests, LinearBlendsTheKeysAroundTheTime)
{
    const auto track = makeTrack (Path::translation, Interpolation::linear, { 0.0f, 1.0f, 2.0f, 4.0f }, { 0.0f, 10.0f, -2.0f, 10.0f, 10.0f, 2.0f, 20.0f, 30.0f, 2.0f, 40.0f, 30.0f, -6.0f });

    expectNear (sample (track, 0.5f), { 5.0f, 10.0f, 0.0f });
    expectNear (sample (track, 1.25f), { 12.5f, 15.0f, 2.0f });
    expectNear (sample (track, 3.0f), { 30.0f, 30.0f, -2.0f });

    // At the time of a key its value comes out as it is
    expectNear (sample (track, 1.0f), { 10.0f, 10.0f, 2.0f }, 0.0f);
    expectNear (sample (track, 2.0f), { 20.0f, 30.0f, 2.0f }, 0.0f);
}

TEST_F (AnimationClipTests, CubicSplineFollowsTheTangentsScaledByTheTimeBetweenKeys)
{
    // Each key is its in-tangent, its value and its out-tangent. The 9s and the 7s are the in-tangent
    // of the first key and the out-tangent of the last one, which nothing uses
    const auto track = makeTrack (Path::translation, Interpolation::cubicSpline, { 1.0f, 3.0f }, { 9.0f, 9.0f, 9.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, -2.0f, 3.0f, 0.0f, 0.0f, 2.0f, 1.0f, 4.0f, 7.0f, 7.0f, 7.0f });

    // Halfway the values weigh 1/2 each and the tangents 1/8 and -1/8, times the 2 seconds between the keys:
    // x = 2/8 * 1 + 1/2 * 2 - 2/8 * 3, z = 2/8 * -2 + 1/2 * 4
    expectNear (sample (track, 2.0f), { 0.5f, 1.0f, 1.5f });

    // A quarter of the way the values weigh 27/32 and 5/32, the tangents 9/64 and -3/64
    expectNear (sample (track, 1.5f), { 0.3125f, 1.0f, 0.0625f });

    expectNear (sample (track, 0.0f), { 0.0f, 1.0f, 0.0f }, 0.0f);
    expectNear (sample (track, 1.0f), { 0.0f, 1.0f, 0.0f }, 0.0f);
    expectNear (sample (track, 3.0f), { 2.0f, 1.0f, 4.0f }, 0.0f);
    expectNear (sample (track, 8.0f), { 2.0f, 1.0f, 4.0f }, 0.0f);
}

TEST_F (AnimationClipTests, TimesOutsideTheKeysGiveTheNearestKey)
{
    for (const auto interpolation : { Interpolation::step, Interpolation::linear })
    {
        const auto track = makeTrack (Path::scale, interpolation, { 1.0f, 3.0f }, { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f });

        expectNear (sample (track, -5.0f), { 1.0f, 2.0f, 3.0f });
        expectNear (sample (track, 0.999f), { 1.0f, 2.0f, 3.0f });
        expectNear (sample (track, 3.0f), { 4.0f, 5.0f, 6.0f });
        expectNear (sample (track, 100.0f), { 4.0f, 5.0f, 6.0f });
    }
}

TEST_F (AnimationClipTests, SingleKeyIsConstant)
{
    const auto linear = makeTrack (Path::scale, Interpolation::linear, { 2.0f }, { 1.0f, 2.0f, 3.0f });
    const auto cubic = makeTrack (Path::scale, Interpolation::cubicSpline, { 2.0f }, { 9.0f, 9.0f, 9.0f, 1.0f, 2.0f, 3.0f, 7.0f, 7.0f, 7.0f });

    for (const auto time : { 0.0f, 2.0f, 5.0f })
    {
        expectNear (sample (linear, time), { 1.0f, 2.0f, 3.0f });
        expectNear (sample (cubic, time), { 1.0f, 2.0f, 3.0f });
    }
}

//==============================================================================

TEST_F (AnimationClipTests, LinearRotationsTurnAtAConstantSpeedTheShorterWayRound)
{
    const Vector3<float> axis { 0.0f, 1.0f, 0.0f };
    const auto quarterTurn = Quaternion::fromAxisAngle (axis, MathConstants<float>::halfPi);

    // From no rotation to a quarter turn with every sign flipped, which is the same rotation:
    // blending the values as they are would turn the other way, through three quarters of a turn
    const auto track = makeTrack (Path::rotation, Interpolation::linear, { 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f, -quarterTurn.getX(), -quarterTurn.getY(), -quarterTurn.getZ(), -quarterTurn.getW() });

    expectNear (sample (track, 0.25f), toValues (Quaternion::fromAxisAngle (axis, MathConstants<float>::halfPi * 0.25f)));
    expectNear (sample (track, 0.5f), toValues (Quaternion::fromAxisAngle (axis, MathConstants<float>::halfPi * 0.5f)));
    expectNear (sample (track, 0.75f), toValues (Quaternion::fromAxisAngle (axis, MathConstants<float>::halfPi * 0.75f)));
}

TEST_F (AnimationClipTests, RotationsComeOutAsUnitQuaternions)
{
    // No rotation, then half a turn around Y, both twice as long as a unit quaternion
    const std::vector<float> keys { 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 2.0f, 0.0f, 0.0f };
    const auto quarterTurn = toValues (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, MathConstants<float>::halfPi));

    const auto step = makeTrack (Path::rotation, Interpolation::step, { 0.0f, 1.0f }, keys);
    expectNear (sample (step, 0.5f), { 0.0f, 0.0f, 0.0f, 1.0f });
    expectNear (sample (step, 1.0f), { 0.0f, 1.0f, 0.0f, 0.0f });

    const auto linear = makeTrack (Path::rotation, Interpolation::linear, { 0.0f, 1.0f }, keys);
    expectNear (sample (linear, 0.5f), quarterTurn);
    expectNear (sample (linear, 2.0f), { 0.0f, 1.0f, 0.0f, 0.0f });

    // Halfway along a cubic spline without tangents the values of the keys are averaged
    const auto cubic = makeTrack (Path::rotation, Interpolation::cubicSpline, { 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f });
    expectNear (sample (cubic, 0.5f), quarterTurn);
    expectNear (sample (cubic, 0.0f), { 0.0f, 0.0f, 0.0f, 1.0f });
}

//==============================================================================

TEST_F (AnimationClipTests, ComponentsFollowThePath)
{
    EXPECT_EQ (makeTrack (Path::translation, Interpolation::linear, { 0.0f }, { 0.0f, 0.0f, 0.0f }).getNumComponents(), 3);
    EXPECT_EQ (makeTrack (Path::rotation, Interpolation::linear, { 0.0f }, { 0.0f, 0.0f, 0.0f, 1.0f }).getNumComponents(), 4);
    EXPECT_EQ (makeTrack (Path::scale, Interpolation::linear, { 0.0f }, { 1.0f, 1.0f, 1.0f }).getNumComponents(), 3);
}

TEST_F (AnimationClipTests, WeightsHaveOneComponentPerMorphTarget)
{
    const auto linear = makeTrack (Path::weights, Interpolation::linear, { 0.0f, 1.0f }, { 0.0f, 0.5f, 1.0f, 1.0f, 0.5f, 0.0f });

    EXPECT_EQ (linear.getNumComponents(), 3);
    expectNear (sample (linear, 0.25f), { 0.25f, 0.5f, 0.75f });
    expectNear (sample (linear, 1.0f), { 1.0f, 0.5f, 0.0f });

    // Two weights: each key holds their two in-tangents, their two values and their two out-tangents
    const auto cubic = makeTrack (Path::weights, Interpolation::cubicSpline, { 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f });

    EXPECT_EQ (cubic.getNumComponents(), 2);
    expectNear (sample (cubic, 0.0f), { 0.0f, 1.0f });
    expectNear (sample (cubic, 0.5f), { 0.5f, 0.5f });
}
