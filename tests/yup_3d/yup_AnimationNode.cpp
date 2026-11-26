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

class AnimationNodeTests : public ::testing::Test
{
protected:
    using Path = AnimationClip::Path;
    using Interpolation = AnimationClip::Interpolation;

    static AnimationClip::Track makeTrack (int targetIndex, Path path, std::vector<float> times, std::vector<float> values, Interpolation interpolation = Interpolation::linear)
    {
        AnimationClip::Track track;
        track.target = targetIndex;
        track.path = path;
        track.interpolation = interpolation;
        track.times = std::move (times);
        track.values = std::move (values);
        return track;
    }

    static AnimationClip::Ptr makeClip (const String& name, std::vector<AnimationClip::Track> tracks)
    {
        auto clip = AnimationClip::Ptr (new AnimationClip (name));

        for (auto& track : tracks)
            EXPECT_TRUE (clip->addTrack (std::move (track)).wasOk());

        return clip;
    }

    /** Moves a target along x, from 0 to 10 in 2 seconds. */
    static AnimationClip::Track makeSlideTrack (int targetIndex)
    {
        return makeTrack (targetIndex, Path::translation, { 0.0f, 2.0f }, { 0.0f, 0.0f, 0.0f, 10.0f, 0.0f, 0.0f });
    }

    static AnimationClip::Ptr makeSlide()
    {
        return makeClip ("slide", { makeSlideTrack (0) });
    }

    /** Moves target 0 along y, from 3 to 5 in 4 seconds. */
    static AnimationClip::Ptr makeLift()
    {
        return makeClip ("lift", { makeTrack (0, Path::translation, { 0.0f, 4.0f }, { 0.0f, 3.0f, 0.0f, 0.0f, 5.0f, 0.0f }) });
    }

    static void expectNear (const Vector3<float>& actual, const Vector3<float>& expected, float tolerance = 1.0e-5f)
    {
        EXPECT_NEAR (actual.getX(), expected.getX(), tolerance);
        EXPECT_NEAR (actual.getY(), expected.getY(), tolerance);
        EXPECT_NEAR (actual.getZ(), expected.getZ(), tolerance);
    }

    void SetUp() override
    {
        target = scene.createEntity ("target");
        transform = &target.attach<TransformNode>();
    }

    /** Attaches a player of the clips to a new entity of the scene. */
    AnimationNode& attachPlayer (std::vector<AnimationClip::Ptr> clips, std::vector<Entity> targets)
    {
        return scene.createEntity ("model").attach<AnimationNode> (std::move (clips), std::move (targets));
    }

    Scene scene;
    Entity target;
    TransformNode* transform = nullptr;
};

//==============================================================================

TEST_F (AnimationNodeTests, StartsPausedOnItsFirstClip)
{
    const auto slide = makeSlide();
    const auto lift = makeLift();
    const auto& player = attachPlayer ({ slide, lift }, { target });

    EXPECT_EQ (player.getNumClips(), 2);
    EXPECT_EQ (player.getClip (0), slide);
    EXPECT_EQ (player.getClip (1), lift);
    EXPECT_EQ (player.getClip (2), nullptr);
    EXPECT_EQ (player.getClip (-1), nullptr);

    EXPECT_EQ (player.findClip ("lift"), 1);
    EXPECT_EQ (player.findClip ("missing"), -1);

    EXPECT_EQ (player.getCurrentClip(), 0);
    EXPECT_DOUBLE_EQ (player.getDuration(), 2.0);
    EXPECT_DOUBLE_EQ (player.getTime(), 0.0);
    EXPECT_FLOAT_EQ (player.getSpeed(), 1.0f);
    EXPECT_FALSE (player.isLooping());
    EXPECT_FALSE (player.isPlaying());
}

TEST_F (AnimationNodeTests, NodeWithoutClipsPlaysNothing)
{
    auto& player = attachPlayer ({}, { target });
    int ended = 0;
    const auto connection = player.onClipEnded().connect ([&] { ++ended; });

    EXPECT_EQ (player.getNumClips(), 0);
    EXPECT_EQ (player.getCurrentClip(), -1);
    EXPECT_DOUBLE_EQ (player.getDuration(), 0.0);

    player.setPlaying (true);
    EXPECT_FALSE (player.isPlaying());

    player.setTime (1.0);
    scene.update (1.0);

    EXPECT_DOUBLE_EQ (player.getTime(), 0.0);
    EXPECT_FALSE (player.isPlaying());
    EXPECT_EQ (ended, 0);
}

TEST_F (AnimationNodeTests, SelectingNoClipStopsPlaying)
{
    auto& player = attachPlayer ({ makeSlide() }, { target });

    player.setPlaying (true);
    ASSERT_TRUE (player.isPlaying());

    player.setCurrentClip (-1);
    EXPECT_FALSE (player.isPlaying());

    // Nothing plays until a clip is selected and asked to play
    player.setPlaying (true);
    EXPECT_FALSE (player.isPlaying());

    player.setCurrentClip (0);
    EXPECT_FALSE (player.isPlaying());

    player.setPlaying (true);
    EXPECT_TRUE (player.isPlaying());
}

TEST_F (AnimationNodeTests, AnIndexWithoutAClipIsNeverSelected)
{
    // The first index has no clip, as the one of an animation of a model that can't be played
    const auto slide = makeSlide();
    auto& player = attachPlayer ({ AnimationClip::Ptr(), slide }, { target });

    EXPECT_EQ (player.getNumClips(), 2);
    EXPECT_EQ (player.getClip (0), nullptr);
    EXPECT_EQ (player.getClip (1), slide);
    EXPECT_EQ (player.findClip ("slide"), 1);

    // The node starts on the first clip there is
    EXPECT_EQ (player.getCurrentClip(), 1);
    EXPECT_DOUBLE_EQ (player.getDuration(), 2.0);

    player.setCurrentClip (0);
    EXPECT_EQ (player.getCurrentClip(), -1);

    player.setPlaying (true);
    EXPECT_FALSE (player.isPlaying());
}

TEST_F (AnimationNodeTests, TimesAndSpeedsThatAreNotFiniteAreIgnored)
{
#if YUP_ASSERTIONS_ENABLED_OR_LOGGED
    GTEST_SKIP() << "A time, a speed or a delta that is not finite triggers an assertion when assertions are enabled or logged";
#endif

    auto& player = attachPlayer ({ makeSlide() }, { target });

    player.setTime (0.5);
    player.setTime (std::numeric_limits<double>::quiet_NaN());
    EXPECT_DOUBLE_EQ (player.getTime(), 0.5);

    player.setSpeed (std::numeric_limits<float>::infinity());
    player.setSpeed (std::numeric_limits<float>::quiet_NaN());
    EXPECT_FLOAT_EQ (player.getSpeed(), 1.0f);

    // A looping clip has no end to stop at: such a delta would leave it without a time for good
    player.setLooping (true);
    player.setPlaying (true);
    scene.update (std::numeric_limits<double>::infinity());
    scene.update (std::numeric_limits<double>::quiet_NaN());

    EXPECT_DOUBLE_EQ (player.getTime(), 0.5);
    expectNear (transform->getPosition(), { 2.5f, 0.0f, 0.0f });

    // It plays on from where it is
    scene.update (0.5);
    EXPECT_DOUBLE_EQ (player.getTime(), 1.0);
    EXPECT_TRUE (player.isPlaying());
}

//==============================================================================

TEST_F (AnimationNodeTests, PlayingAdvancesTheTimeAndPosesTheTargets)
{
    auto& player = attachPlayer ({ makeSlide() }, { target });

    player.setPlaying (true);
    scene.update (0.5);

    EXPECT_DOUBLE_EQ (player.getTime(), 0.5);
    expectNear (transform->getPosition(), { 2.5f, 0.0f, 0.0f });

    scene.update (1.0);

    EXPECT_DOUBLE_EQ (player.getTime(), 1.5);
    expectNear (transform->getPosition(), { 7.5f, 0.0f, 0.0f });
    EXPECT_TRUE (player.isPlaying());
}

TEST_F (AnimationNodeTests, PausedNodeLeavesTheTargetsAlone)
{
    auto& player = attachPlayer ({ makeSlide() }, { target });
    transform->setPosition ({ 1.0f, 2.0f, 3.0f });

    // Nothing plays until asked
    scene.update (0.5);
    EXPECT_DOUBLE_EQ (player.getTime(), 0.0);
    expectNear (transform->getPosition(), { 1.0f, 2.0f, 3.0f });

    player.setPlaying (true);
    scene.update (0.5);
    player.setPlaying (false);
    scene.update (0.5);

    EXPECT_DOUBLE_EQ (player.getTime(), 0.5);
    expectNear (transform->getPosition(), { 2.5f, 0.0f, 0.0f });
}

TEST_F (AnimationNodeTests, PlayingPosesTheTargetsWhereTheClipStartsAtOnce)
{
    auto& player = attachPlayer ({ makeSlide() }, { target });
    transform->setPosition ({ 1.0f, 2.0f, 3.0f });

    player.setPlaying (true);
    expectNear (transform->getPosition(), { 0.0f, 0.0f, 0.0f });
}

TEST_F (AnimationNodeTests, AnUpdateInWhichNoTimeGoesByLeavesTheTargetsAlone)
{
    auto& player = attachPlayer ({ makeSlide() }, { target });

    player.setPlaying (true);
    scene.update (0.5);

    // What is written to a target after the clip posed it stays until the clip moves on
    transform->setPosition ({ 1.0f, 2.0f, 3.0f });
    scene.update (0.0);
    expectNear (transform->getPosition(), { 1.0f, 2.0f, 3.0f });

    player.setSpeed (0.0f);
    scene.update (0.5);
    expectNear (transform->getPosition(), { 1.0f, 2.0f, 3.0f });
    EXPECT_DOUBLE_EQ (player.getTime(), 0.5);

    player.setSpeed (1.0f);
    scene.update (0.5);
    expectNear (transform->getPosition(), { 5.0f, 0.0f, 0.0f });
}

TEST_F (AnimationNodeTests, SettingTheTimePosesTheTargetsAtOnce)
{
    auto& player = attachPlayer ({ makeSlide() }, { target });

    player.setTime (1.0);
    EXPECT_DOUBLE_EQ (player.getTime(), 1.0);
    expectNear (transform->getPosition(), { 5.0f, 0.0f, 0.0f });
    EXPECT_FALSE (player.isPlaying());

    // Limited to the clip
    player.setTime (10.0);
    EXPECT_DOUBLE_EQ (player.getTime(), 2.0);
    expectNear (transform->getPosition(), { 10.0f, 0.0f, 0.0f });

    player.setTime (-1.0);
    EXPECT_DOUBLE_EQ (player.getTime(), 0.0);
    expectNear (transform->getPosition(), { 0.0f, 0.0f, 0.0f });
}

TEST_F (AnimationNodeTests, SelectingAClipPosesItsStartAtOnce)
{
    auto& player = attachPlayer ({ makeSlide(), makeLift() }, { target });
    player.setTime (1.0);

    player.setCurrentClip (1);

    EXPECT_EQ (player.getCurrentClip(), 1);
    EXPECT_DOUBLE_EQ (player.getDuration(), 4.0);
    EXPECT_DOUBLE_EQ (player.getTime(), 0.0);
    expectNear (transform->getPosition(), { 0.0f, 3.0f, 0.0f });

    // Out of range no clip is selected: the pose stays, and there is nothing to play
    player.setTime (2.0);
    player.setCurrentClip (2);
    player.setPlaying (true);
    scene.update (1.0);

    EXPECT_EQ (player.getCurrentClip(), -1);
    EXPECT_DOUBLE_EQ (player.getDuration(), 0.0);
    EXPECT_DOUBLE_EQ (player.getTime(), 0.0);
    expectNear (transform->getPosition(), { 0.0f, 4.0f, 0.0f });
}

TEST_F (AnimationNodeTests, SpeedScalesTheTimeAndPlaysBackwards)
{
    auto& player = attachPlayer ({ makeSlide() }, { target });
    player.setPlaying (true);

    player.setSpeed (2.0f);
    scene.update (0.5);
    EXPECT_DOUBLE_EQ (player.getTime(), 1.0);
    expectNear (transform->getPosition(), { 5.0f, 0.0f, 0.0f });

    player.setSpeed (-1.0f);
    scene.update (0.25);
    EXPECT_DOUBLE_EQ (player.getTime(), 0.75);
    expectNear (transform->getPosition(), { 3.75f, 0.0f, 0.0f });
    EXPECT_TRUE (player.isPlaying());
}

TEST_F (AnimationNodeTests, LoopingCarriesOnFromTheOtherEnd)
{
    auto& player = attachPlayer ({ makeSlide() }, { target });
    int ended = 0;
    const auto connection = player.onClipEnded().connect ([&] { ++ended; });

    player.setLooping (true);
    player.setPlaying (true);

    scene.update (1.5);
    scene.update (1.0);
    EXPECT_DOUBLE_EQ (player.getTime(), 0.5);
    expectNear (transform->getPosition(), { 2.5f, 0.0f, 0.0f });

    player.setSpeed (-1.0f);
    scene.update (1.0);
    EXPECT_DOUBLE_EQ (player.getTime(), 1.5);
    expectNear (transform->getPosition(), { 7.5f, 0.0f, 0.0f });

    EXPECT_TRUE (player.isPlaying());
    EXPECT_EQ (ended, 0);
}

//==============================================================================

TEST_F (AnimationNodeTests, ClipStopsAtItsEndAndTellsOnce)
{
    auto& player = attachPlayer ({ makeSlide() }, { target });
    int ended = 0;
    const auto connection = player.onClipEnded().connect ([&]
    {
        // The targets are posed at the end and the node is stopped already
        EXPECT_FALSE (player.isPlaying());
        expectNear (transform->getPosition(), { 10.0f, 0.0f, 0.0f });
        ++ended;
    });

    player.setPlaying (true);
    scene.update (1.5);
    EXPECT_EQ (ended, 0);

    scene.update (1.0);
    EXPECT_EQ (ended, 1);
    EXPECT_FALSE (player.isPlaying());
    EXPECT_DOUBLE_EQ (player.getTime(), 2.0);

    scene.update (1.0);
    scene.update (1.0);
    EXPECT_EQ (ended, 1);
    EXPECT_DOUBLE_EQ (player.getTime(), 2.0);
}

TEST_F (AnimationNodeTests, ClipPlayedBackwardsStopsAtItsStartAndTellsOnce)
{
    auto& player = attachPlayer ({ makeSlide() }, { target });
    int ended = 0;
    const auto connection = player.onClipEnded().connect ([&] { ++ended; });

    player.setTime (1.0);
    player.setSpeed (-1.0f);
    player.setPlaying (true);

    scene.update (0.5);
    EXPECT_EQ (ended, 0);

    scene.update (3.0);
    scene.update (1.0);

    EXPECT_EQ (ended, 1);
    EXPECT_FALSE (player.isPlaying());
    EXPECT_DOUBLE_EQ (player.getTime(), 0.0);
    expectNear (transform->getPosition(), { 0.0f, 0.0f, 0.0f });
}

TEST_F (AnimationNodeTests, EndedCallbackCanPlayAnotherClip)
{
    auto& player = attachPlayer ({ makeSlide(), makeLift() }, { target });
    const auto connection = player.onClipEnded().connect ([&]
    {
        player.setCurrentClip (1);
        player.setPlaying (true);
    });

    player.setPlaying (true);
    scene.update (5.0);

    EXPECT_TRUE (player.isPlaying());
    EXPECT_EQ (player.getCurrentClip(), 1);
    EXPECT_DOUBLE_EQ (player.getTime(), 0.0);

    scene.update (2.0);
    expectNear (transform->getPosition(), { 0.0f, 4.0f, 0.0f });
}

//==============================================================================

TEST_F (AnimationNodeTests, PlayingAClipThatEndedStartsItOver)
{
    auto& player = attachPlayer ({ makeSlide() }, { target });
    int ended = 0;
    const auto connection = player.onClipEnded().connect ([&] { ++ended; });

    player.setPlaying (true);
    scene.update (3.0);
    ASSERT_EQ (ended, 1);
    ASSERT_FALSE (player.isPlaying());

    // Back at its start, where the targets are posed before any update
    player.setPlaying (true);
    EXPECT_TRUE (player.isPlaying());
    EXPECT_DOUBLE_EQ (player.getTime(), 0.0);
    expectNear (transform->getPosition(), { 0.0f, 0.0f, 0.0f });

    scene.update (0.5);
    EXPECT_DOUBLE_EQ (player.getTime(), 0.5);
    expectNear (transform->getPosition(), { 2.5f, 0.0f, 0.0f });
    EXPECT_EQ (ended, 1);

    scene.update (2.0);
    EXPECT_EQ (ended, 2);
    EXPECT_FALSE (player.isPlaying());
    EXPECT_DOUBLE_EQ (player.getTime(), 2.0);
}

TEST_F (AnimationNodeTests, PlayingAClipThatEndedBackwardsStartsItOverFromItsEnd)
{
    auto& player = attachPlayer ({ makeSlide() }, { target });
    int ended = 0;
    const auto connection = player.onClipEnded().connect ([&] { ++ended; });

    player.setTime (1.0);
    player.setSpeed (-1.0f);
    player.setPlaying (true);
    scene.update (3.0);
    ASSERT_EQ (ended, 1);
    ASSERT_FALSE (player.isPlaying());

    player.setPlaying (true);
    EXPECT_TRUE (player.isPlaying());
    EXPECT_DOUBLE_EQ (player.getTime(), 2.0);
    expectNear (transform->getPosition(), { 10.0f, 0.0f, 0.0f });

    scene.update (0.5);
    EXPECT_DOUBLE_EQ (player.getTime(), 1.5);
    expectNear (transform->getPosition(), { 7.5f, 0.0f, 0.0f });
    EXPECT_EQ (ended, 1);

    scene.update (2.0);
    EXPECT_EQ (ended, 2);
    EXPECT_FALSE (player.isPlaying());
    EXPECT_DOUBLE_EQ (player.getTime(), 0.0);
}

TEST_F (AnimationNodeTests, PlayingAPausedClipCarriesOnWhereItIs)
{
    auto& player = attachPlayer ({ makeSlide() }, { target });

    player.setPlaying (true);
    scene.update (0.5);
    player.setPlaying (false);

    player.setPlaying (true);
    EXPECT_DOUBLE_EQ (player.getTime(), 0.5);
    expectNear (transform->getPosition(), { 2.5f, 0.0f, 0.0f });

    scene.update (0.5);
    EXPECT_DOUBLE_EQ (player.getTime(), 1.0);
    expectNear (transform->getPosition(), { 5.0f, 0.0f, 0.0f });
}

TEST_F (AnimationNodeTests, PlayingAClipScrubbedToItsEndStartsItOver)
{
    auto& player = attachPlayer ({ makeSlide() }, { target });

    // Scrubbing neither plays nor starts anything over
    player.setTime (2.0);
    EXPECT_FALSE (player.isPlaying());
    EXPECT_DOUBLE_EQ (player.getTime(), 2.0);
    expectNear (transform->getPosition(), { 10.0f, 0.0f, 0.0f });

    player.setPlaying (true);
    EXPECT_DOUBLE_EQ (player.getTime(), 0.0);
    expectNear (transform->getPosition(), { 0.0f, 0.0f, 0.0f });

    scene.update (0.5);
    EXPECT_TRUE (player.isPlaying());
    EXPECT_DOUBLE_EQ (player.getTime(), 0.5);
}

TEST_F (AnimationNodeTests, TheSpeedSetWhenPlayingStartsTellsWhichEndIsAhead)
{
    // Turned backwards once it plays, a clip at its start stops there
    auto& late = attachPlayer ({ makeSlide() }, { target });
    late.setPlaying (true);
    late.setSpeed (-1.0f);
    scene.update (0.5);

    EXPECT_FALSE (late.isPlaying());
    EXPECT_DOUBLE_EQ (late.getTime(), 0.0);

    // Turned backwards before it plays, it starts from its end
    auto& early = attachPlayer ({ makeSlide() }, { target });
    early.setSpeed (-1.0f);
    early.setPlaying (true);
    EXPECT_DOUBLE_EQ (early.getTime(), 2.0);

    scene.update (0.5);

    EXPECT_TRUE (early.isPlaying());
    EXPECT_DOUBLE_EQ (early.getTime(), 1.5);
}

TEST_F (AnimationNodeTests, PlayingStartsOverOnlyAClipStoppedAtTheEndItRunsInto)
{
    auto& player = attachPlayer ({ makeSlide() }, { target });
    player.setTime (2.0);

    // A speed of 0 runs into no end
    player.setSpeed (0.0f);
    player.setPlaying (true);
    EXPECT_DOUBLE_EQ (player.getTime(), 2.0);

    // Backwards the end is behind: the clip plays from there
    player.setPlaying (false);
    player.setSpeed (-1.0f);
    player.setPlaying (true);
    EXPECT_DOUBLE_EQ (player.getTime(), 2.0);

    // Forwards the end is ahead again, but the clip is playing already
    player.setSpeed (1.0f);
    player.setPlaying (true);
    EXPECT_DOUBLE_EQ (player.getTime(), 2.0);

    // A looping clip carries on from the other end by itself
    player.setPlaying (false);
    player.setLooping (true);
    player.setPlaying (true);
    EXPECT_DOUBLE_EQ (player.getTime(), 2.0);

    scene.update (0.5);
    EXPECT_DOUBLE_EQ (player.getTime(), 0.5);
}

//==============================================================================

TEST_F (AnimationNodeTests, PathsWithoutATrackKeepTheirValue)
{
    const auto rotation = Quaternion::fromAxisAngle ({ 0.0f, 0.0f, 1.0f }, 0.5f);
    transform->setRotation (rotation);
    transform->setScale ({ 2.0f, 3.0f, 4.0f });

    auto& player = attachPlayer ({ makeSlide() }, { target });
    player.setTime (1.0);

    expectNear (transform->getPosition(), { 5.0f, 0.0f, 0.0f });
    expectNear (transform->getScale(), { 2.0f, 3.0f, 4.0f });
    EXPECT_EQ (transform->getRotation(), rotation);
}

TEST_F (AnimationNodeTests, RotationAndScaleTracksPoseTheTransform)
{
    const auto quarterTurn = Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, MathConstants<float>::halfPi);
    const auto turning = makeTrack (0, Path::rotation, { 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f, quarterTurn.getX(), quarterTurn.getY(), quarterTurn.getZ(), quarterTurn.getW() });
    const auto growing = makeTrack (0, Path::scale, { 0.0f, 0.25f }, { 1.0f, 1.0f, 1.0f, 2.0f, 3.0f, 4.0f }, Interpolation::step);

    transform->setPosition ({ 1.0f, 2.0f, 3.0f });

    auto& player = attachPlayer ({ makeClip ("turn", { turning, growing }) }, { target });
    player.setTime (0.5);

    EXPECT_TRUE (transform->getRotation().approximatelyEqualTo (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, MathConstants<float>::halfPi * 0.5f)));
    expectNear (transform->getScale(), { 2.0f, 3.0f, 4.0f });
    expectNear (transform->getPosition(), { 1.0f, 2.0f, 3.0f });
}

TEST_F (AnimationNodeTests, TargetsWithoutATransformGetOne)
{
    auto bare = scene.createEntity ("bare");
    auto& player = attachPlayer ({ makeSlide() }, { bare });

    player.setPlaying (true);
    scene.update (1.0);

    ASSERT_NE (bare.get<TransformNode>(), nullptr);
    expectNear (bare.get<TransformNode>()->getPosition(), { 5.0f, 0.0f, 0.0f });
}

TEST_F (AnimationNodeTests, TargetsThatAreGoneAreSkipped)
{
    auto doomed = scene.createEntity ("doomed");

    // Tracks for a target that gets destroyed, for one that never was valid and for one out of range, then for the one left
    const auto clip = makeClip ("all", { makeSlideTrack (0), makeSlideTrack (1), makeSlideTrack (7), makeSlideTrack (2) });

    auto& player = attachPlayer ({ clip }, { doomed, Entity(), target });
    const auto numEntities = scene.getNumEntities();

    doomed.destroy();
    player.setPlaying (true);
    scene.update (1.0);

    EXPECT_FALSE (doomed.isValid());
    EXPECT_EQ (scene.getNumEntities(), numEntities - 1);
    expectNear (transform->getPosition(), { 5.0f, 0.0f, 0.0f });
}

TEST_F (AnimationNodeTests, WeightsAreKeptForTheirTarget)
{
    auto morphed = scene.createEntity ("morphed");
    const auto clip = makeClip ("morph", { makeTrack (1, Path::weights, { 0.0f, 1.0f }, { 0.0f, 1.0f, 1.0f, 0.0f }) });
    auto& player = attachPlayer ({ clip }, { target, morphed });

    EXPECT_TRUE (player.getWeights (1).empty());

    player.setTime (0.25);

    const auto weights = player.getWeights (1);
    ASSERT_EQ (weights.size(), 2u);
    EXPECT_NEAR (weights[0], 0.25f, 1.0e-6f);
    EXPECT_NEAR (weights[1], 0.75f, 1.0e-6f);

    // Weights are for the meshes: they move nothing
    EXPECT_TRUE (player.getWeights (0).empty());
    EXPECT_TRUE (player.getWeights (5).empty());
    EXPECT_FALSE (morphed.has<TransformNode>());
}
