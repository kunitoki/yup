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

constexpr const char* kSimpleJson = R"json({
    "v": "5.5.2",
    "nm": "PlayerTest",
    "ip": 0,
    "op": 50,
    "fr": 25.0,
    "w": 100,
    "h": 100,
    "ddd": 0,
    "assets": [],
    "layers": []
})json";

constexpr const char* kNonZeroStartJson = R"json({
    "v": "5.5.2",
    "nm": "NonZeroStart",
    "ip": 10,
    "op": 60,
    "fr": 25.0,
    "w": 100,
    "h": 100,
    "ddd": 0,
    "assets": [],
    "layers": []
})json";

} // namespace

class AnimationPlayerTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        anim = Animation::loadFromData (kSimpleJson);
    }

    Animation anim;
};

TEST_F (AnimationPlayerTests, DefaultPlayerIsNotPlaying)
{
    AnimationPlayer player (anim);
    EXPECT_FALSE (player.isPlaying());
    EXPECT_NEAR (player.currentFrame(), 0.0f, 0.001f);
}

TEST_F (AnimationPlayerTests, PlayAndAdvance)
{
    AnimationPlayer player (anim);
    player.play();
    EXPECT_TRUE (player.isPlaying());

    // Advance by 1 second = 25 frames at 25 fps
    bool changed = player.advanceTime (1.0f);
    EXPECT_TRUE (changed);
    EXPECT_NEAR (player.currentFrame(), 25.0f, 1.0f);
}

TEST_F (AnimationPlayerTests, PauseStopsAdvancing)
{
    AnimationPlayer player (anim);
    player.play();
    player.pause();
    EXPECT_FALSE (player.isPlaying());

    const float frameBefore = player.currentFrame();
    player.advanceTime (1.0f);
    EXPECT_NEAR (player.currentFrame(), frameBefore, 0.001f);
}

TEST_F (AnimationPlayerTests, StopResetsToStart)
{
    AnimationPlayer player (anim);
    player.play();
    player.advanceTime (0.5f);
    player.stop();
    EXPECT_FALSE (player.isPlaying());
    EXPECT_NEAR (player.currentFrame(), 0.0f, 0.001f);
}

TEST_F (AnimationPlayerTests, LoopingWrapsAround)
{
    AnimationPlayer player (anim);
    player.setLooping (true);
    player.play();

    int loops = 0;
    player.onLoopCompleted = [&]
    {
        ++loops;
    };

    // Advance past the total (50 frames at 25fps = 2 seconds)
    player.advanceTime (2.5f);
    EXPECT_GT (loops, 0);
    EXPECT_TRUE (player.isPlaying()); // still playing because looping
}

TEST_F (AnimationPlayerTests, LoopingDoesNotSkipLastFrame)
{
    AnimationPlayer player (anim);
    player.setLooping (true);
    player.seekToFrame (48.0f);
    player.play();

    int loops = 0;
    player.onLoopCompleted = [&]
    {
        ++loops;
    };

    player.advanceTime (1.0f / 25.0f);
    EXPECT_NEAR (player.currentFrame(), 49.0f, 0.001f);
    EXPECT_EQ (loops, 0);

    player.advanceTime (1.0f / 25.0f);
    EXPECT_NEAR (player.currentFrame(), 0.0f, 0.001f);
    EXPECT_EQ (loops, 1);
}

TEST_F (AnimationPlayerTests, NoLoopingStopsAtEnd)
{
    AnimationPlayer player (anim);
    player.setLooping (false);
    player.play();

    bool ended = false;
    player.onPlaybackEnded = [&]
    {
        ended = true;
    };

    player.advanceTime (10.0f); // way past end
    EXPECT_TRUE (ended);
    EXPECT_FALSE (player.isPlaying());
}

TEST_F (AnimationPlayerTests, SpeedMultiplierAffectsAdvance)
{
    AnimationPlayer playerNormal (anim);
    AnimationPlayer playerFast (anim);
    playerFast.setSpeed (2.0f);

    playerNormal.play();
    playerFast.play();

    playerNormal.advanceTime (0.5f);
    playerFast.advanceTime (0.5f);

    EXPECT_NEAR (playerFast.currentFrame(), playerNormal.currentFrame() * 2.0f, 1.0f);
}

TEST_F (AnimationPlayerTests, SeekToProgress)
{
    AnimationPlayer player (anim);
    player.seekToProgress (0.5f);
    EXPECT_NEAR (player.currentProgress(), 0.5f, 0.01f);
}

TEST_F (AnimationPlayerTests, ReverseDirection)
{
    AnimationPlayer player (anim);
    player.setDirection (AnimationPlayer::Direction::Reverse);
    player.seekToProgress (1.0f);
    player.play();
    player.advanceTime (0.5f);
    EXPECT_LT (player.currentFrame(), player.getAnimation().totalFrames());
}

TEST_F (AnimationPlayerTests, UsesCompositionStartFrameAsInitialFrame)
{
    auto nonZeroStartAnim = Animation::loadFromData (kNonZeroStartJson);
    ASSERT_TRUE (nonZeroStartAnim.isValid());

    AnimationPlayer player (nonZeroStartAnim);
    EXPECT_NEAR (player.currentFrame(), 10.0f, 0.001f);

    player.seekToProgress (1.0f);
    EXPECT_NEAR (player.currentFrame(), 59.0f, 0.001f);

    player.stop();
    EXPECT_NEAR (player.currentFrame(), 10.0f, 0.001f);
}

TEST (AnimationTrimTests, FullRangeRemainsFullWhenOffset)
{
    AnimationTrim trim;
    trim.start = FloatProperty::staticValue (0.0f);
    trim.end = FloatProperty::staticValue (100.0f);
    trim.offset = FloatProperty::staticValue (90.0f);

    const auto segment = trim.getSegment (0.0f);

    EXPECT_NEAR (segment.start, 0.0f, 1.0e-6f);
    EXPECT_NEAR (segment.end, 1.0f, 1.0e-6f);
}

TEST (AnimationTrimTests, NegativeOffsetWrapsIntoNormalizedRange)
{
    AnimationTrim trim;
    trim.start = FloatProperty::staticValue (25.0f);
    trim.end = FloatProperty::staticValue (75.0f);
    trim.offset = FloatProperty::staticValue (-180.0f);

    const auto segment = trim.getSegment (0.0f);

    EXPECT_NEAR (segment.start, 0.75f, 1.0e-6f);
    EXPECT_NEAR (segment.end, 0.25f, 1.0e-6f);
}

TEST (AnimationTrimTests, ReversedStartAndEndWithoutOffsetDoesNotWrap)
{
    AnimationTrim trim;
    trim.start = FloatProperty::staticValue (75.0f);
    trim.end = FloatProperty::staticValue (25.0f);
    trim.offset = FloatProperty::staticValue (0.0f);

    const auto segment = trim.getSegment (0.0f);

    EXPECT_NEAR (segment.start, 0.25f, 1.0e-6f);
    EXPECT_NEAR (segment.end, 0.75f, 1.0e-6f);
}

TEST (AnimationLayerTests, TimeStretchDividesLocalFrame)
{
    NullLayer layer;
    layer.startFrame = 10.0f;
    layer.timeStretch = 2.0f;

    EXPECT_NEAR (layer.localFrame (30.0f), 10.0f, 1.0e-6f);
}

TEST (AnimationLayerTests, TimeRemapSecondsConvertToFrames)
{
    NullLayer layer;
    auto builder = FloatProperty::animated();
    builder.keyframe (0.0f, 0.0f, 1.0f, AnimationEasing::linear());
    builder.keyframe (30.0f, 1.0f, AnimationEasing::linear());
    layer.timeRemap = builder.build();

    EXPECT_NEAR (layer.localFrame (30.0f, 30.0f), 30.0f, 1.0e-6f);
}

TEST (AnimationLayerTests, TimeRemapLoopOutCycleWrapsAfterLastKeyframe)
{
    NullLayer layer;
    auto builder = FloatProperty::animated();
    builder.keyframe (0.0f, 0.0f, 1.0f, AnimationEasing::linear());
    builder.keyframe (30.0f, 1.0f, AnimationEasing::linear());
    layer.timeRemap = builder.build();
    layer.timeRemapLoopOutCycle = true;

    EXPECT_NEAR (layer.localFrame (45.0f, 30.0f), 15.0f, 1.0e-6f);
}
