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

    /** Turns a target a quarter turn about y in 1 second. */
    static AnimationClip::Track makeTurnTrack (int targetIndex)
    {
        const auto quarterTurn = Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, MathConstants<float>::halfPi);
        return makeTrack (targetIndex, Path::rotation, { 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f, quarterTurn.getX(), quarterTurn.getY(), quarterTurn.getZ(), quarterTurn.getW() });
    }

    /** Takes the two weights of a target from 0 and 1 to 1 and 0 in 1 second. */
    static AnimationClip::Track makeMorphTrack (int targetIndex)
    {
        return makeTrack (targetIndex, Path::weights, { 0.0f, 1.0f }, { 0.0f, 1.0f, 1.0f, 0.0f });
    }

    /** A triangle with two morph targets, whose weights start at 0.5 and 0.25 wherever it is drawn deformed. */
    static Mesh::Ptr makeMorphedTriangle()
    {
        std::vector<Mesh::Vertex> vertices (3);
        vertices[1].position = { 1.0f, 0.0f, 0.0f };
        vertices[2].position = { 0.0f, 1.0f, 0.0f };

        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        std::vector<Mesh::MorphTarget> targets (2);
        targets[0].positions = { {}, {}, { 0.0f, 1.0f, 0.0f } };
        targets[1].positions = { {}, { 2.0f, 0.0f, 0.0f }, {} };

        auto mesh = Mesh::Ptr (new Mesh ("triangle"));
        EXPECT_TRUE (mesh->addPrimitive (std::move (vertices), { 0, 1, 2 }, Material::Ptr (new Material()), {}, std::move (targets)).wasOk());
        mesh->setMorphWeights ({ 0.5f, 0.25f });
        return mesh;
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

//==============================================================================

TEST_F (AnimationNodeTests, SelectingAClipPutsBackWhatTheClipBeforeItAnimated)
{
    // The rest pose is the one the target has when the node is given it
    const auto rest = Quaternion::fromAxisAngle ({ 0.0f, 0.0f, 1.0f }, 0.5f);
    transform->setPosition ({ 1.0f, 2.0f, 3.0f });
    transform->setRotation (rest);
    transform->setScale ({ 2.0f, 3.0f, 4.0f });

    const auto turn = makeClip ("turn", { makeTurnTrack (0), makeTrack (0, Path::scale, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 3.0f, 3.0f, 3.0f }) });
    auto& player = attachPlayer ({ turn, makeSlide() }, { target });

    player.setPlaying (true);
    scene.update (0.5);

    EXPECT_TRUE (transform->getRotation().approximatelyEqualTo (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, MathConstants<float>::halfPi * 0.5f)));
    expectNear (transform->getScale(), { 2.0f, 2.0f, 2.0f });
    expectNear (transform->getPosition(), { 1.0f, 2.0f, 3.0f });

    // The slide only moves the target: its rotation and its scale are the ones it had before any clip
    player.setCurrentClip (1);

    expectNear (transform->getPosition(), { 0.0f, 0.0f, 0.0f });
    expectNear (transform->getScale(), { 2.0f, 3.0f, 4.0f });
    EXPECT_EQ (transform->getRotation(), rest);

    // And the turn doesn't move it: its position is the one it had before any clip
    scene.update (1.0);
    expectNear (transform->getPosition(), { 5.0f, 0.0f, 0.0f });

    player.setCurrentClip (0);

    expectNear (transform->getPosition(), { 1.0f, 2.0f, 3.0f });
    expectNear (transform->getScale(), { 1.0f, 1.0f, 1.0f });
    EXPECT_TRUE (transform->getRotation().approximatelyEqualTo (Quaternion()));
}

TEST_F (AnimationNodeTests, SelectingAClipPutsBackTheWeightsTheClipBeforeItAnimated)
{
    auto morphed = scene.createEntity ("morphed");
    const auto& deform = morphed.attach<DeformNode> (makeMorphedTriangle());

    auto& player = attachPlayer ({ makeClip ("morph", { makeMorphTrack (1) }), makeSlide() }, { target, morphed });
    player.setTime (0.25);

    ASSERT_EQ (deform.getWeights().size(), 2u);
    EXPECT_NEAR (deform.getWeights()[0], 0.25f, 1.0e-6f);
    EXPECT_NEAR (deform.getWeights()[1], 0.75f, 1.0e-6f);

    // The slide animates no weight: the mesh has the ones it had before any clip, and the node keeps none for it
    player.setCurrentClip (1);

    EXPECT_FLOAT_EQ (deform.getWeights()[0], 0.5f);
    EXPECT_FLOAT_EQ (deform.getWeights()[1], 0.25f);
    EXPECT_TRUE (player.getWeights (1).empty());
}

TEST_F (AnimationNodeTests, AClipSelectedAfterNoClipPutsBackWhatTheLastOneAnimated)
{
    const auto rest = Quaternion::fromAxisAngle ({ 0.0f, 0.0f, 1.0f }, 0.5f);
    transform->setRotation (rest);

    // The turn also has a track for a target the node doesn't have, which has nothing to put back
    auto& player = attachPlayer ({ makeClip ("turn", { makeTurnTrack (0), makeSlideTrack (7) }), makeSlide() }, { target });
    player.setTime (1.0);

    const auto turned = transform->getRotation();
    ASSERT_FALSE (turned.approximatelyEqualTo (rest));

    // Without a clip the pose stays as the turn left it
    player.setCurrentClip (-1);
    EXPECT_EQ (transform->getRotation(), turned);

    player.setCurrentClip (1);
    EXPECT_EQ (transform->getRotation(), rest);
}

TEST_F (AnimationNodeTests, SelectingAClipLeavesWhatNoClipPosedAlone)
{
    auto& player = attachPlayer ({ makeSlide(), makeClip ("turn", { makeTurnTrack (0) }) }, { target });

    // The slide is selected and posed nothing yet: where code put the target is not what a clip left
    transform->setPosition ({ 1.0f, 2.0f, 3.0f });
    transform->setScale ({ 2.0f, 3.0f, 4.0f });

    player.setCurrentClip (1);
    expectNear (transform->getPosition(), { 1.0f, 2.0f, 3.0f });

    // Neither is its scale, which no clip animates
    player.setCurrentClip (0);
    player.setCurrentClip (1);

    expectNear (transform->getScale(), { 2.0f, 3.0f, 4.0f });
}

TEST_F (AnimationNodeTests, ResetPosePutsEveryTargetBackAsTheNodeWasGivenIt)
{
    transform->setPosition ({ 1.0f, 2.0f, 3.0f });

    auto bare = scene.createEntity ("bare");
    auto morphed = scene.createEntity ("morphed");
    auto doomed = scene.createEntity ("doomed");
    const auto& deform = morphed.attach<DeformNode> (makeMorphedTriangle());

    const auto clip = makeClip ("all", { makeSlideTrack (0), makeSlideTrack (1), makeMorphTrack (2), makeSlideTrack (3) });
    auto& player = attachPlayer ({ clip }, { target, bare, morphed, doomed, Entity() });
    const auto numEntities = scene.getNumEntities();

    player.setTime (1.0);

    const auto* bareTransform = bare.get<TransformNode>();
    ASSERT_NE (bareTransform, nullptr);
    expectNear (bareTransform->getPosition(), { 5.0f, 0.0f, 0.0f });
    EXPECT_FLOAT_EQ (deform.getWeights()[0], 1.0f);

    // What code wrote since goes too, where no clip animates
    transform->setScale ({ 2.0f, 3.0f, 4.0f });
    doomed.destroy();

    player.resetPose();

    expectNear (transform->getPosition(), { 1.0f, 2.0f, 3.0f });
    expectNear (transform->getScale(), { 1.0f, 1.0f, 1.0f });

    // A target that had no transform is where it was without one
    expectNear (bareTransform->getPosition(), { 0.0f, 0.0f, 0.0f });

    EXPECT_FLOAT_EQ (deform.getWeights()[0], 0.5f);
    EXPECT_FLOAT_EQ (deform.getWeights()[1], 0.25f);
    EXPECT_TRUE (player.getWeights (2).empty());

    // Targets that are gone are skipped
    EXPECT_FALSE (doomed.isValid());
    EXPECT_EQ (scene.getNumEntities(), numEntities - 1);
}

TEST_F (AnimationNodeTests, ResetPoseLeavesTheClipWhereItIsToPoseAgain)
{
    transform->setPosition ({ 1.0f, 2.0f, 3.0f });

    auto& player = attachPlayer ({ makeSlide() }, { target });
    player.setPlaying (true);
    scene.update (0.5);

    player.resetPose();

    expectNear (transform->getPosition(), { 1.0f, 2.0f, 3.0f });
    EXPECT_TRUE (player.isPlaying());
    EXPECT_EQ (player.getCurrentClip(), 0);
    EXPECT_DOUBLE_EQ (player.getTime(), 0.5);

    // The clip poses what it animates again once it moves on
    scene.update (0.5);
    expectNear (transform->getPosition(), { 5.0f, 0.0f, 0.0f });
}

TEST_F (AnimationNodeTests, ASecondNodeGivenTheTargetsPlaysAnotherClipBesideTheFirst)
{
    auto other = scene.createEntity ("other");
    const auto turn = makeClip ("turn", { makeTurnTrack (1) });
    const auto grow = makeClip ("grow", { makeTrack (1, Path::scale, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 3.0f, 3.0f, 3.0f }) });

    auto& player = attachPlayer ({ makeSlide(), turn }, { target, other });

    const auto targets = player.getTargets();
    ASSERT_EQ (targets.size(), 2u);
    EXPECT_EQ (targets[0], target);
    EXPECT_EQ (targets[1], other);

    // An entity holds one AnimationNode, which plays one clip: the second one is on another entity
    auto& second = attachPlayer ({ turn, grow }, std::vector<Entity> (targets.begin(), targets.end()));

    player.setPlaying (true);
    second.setPlaying (true);
    scene.update (0.5);

    const auto* turned = other.get<TransformNode>();
    ASSERT_NE (turned, nullptr);
    expectNear (transform->getPosition(), { 2.5f, 0.0f, 0.0f });
    EXPECT_TRUE (turned->getRotation().approximatelyEqualTo (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, MathConstants<float>::halfPi * 0.5f)));

    // Each node puts back what its own clips animated: the slide of the first one stays where it is
    second.setCurrentClip (1);

    EXPECT_EQ (turned->getRotation(), Quaternion());
    expectNear (transform->getPosition(), { 2.5f, 0.0f, 0.0f });
}
