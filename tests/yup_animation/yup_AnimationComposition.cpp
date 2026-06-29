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

class AnimationCompositionTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        comp = AnimationComposition::create ({ 200.0f, 150.0f }, 30.0f);
        ASSERT_NE (comp, nullptr);
    }

    AnimationComposition::Ptr comp;
};

// =============================================================================
// Factory and default values
// =============================================================================

TEST_F (AnimationCompositionTests, Create_SetsSize)
{
    EXPECT_FLOAT_EQ (comp->size.getWidth(), 200.0f);
    EXPECT_FLOAT_EQ (comp->size.getHeight(), 150.0f);
}

TEST_F (AnimationCompositionTests, Create_SetsFrameRate)
{
    EXPECT_FLOAT_EQ (comp->frameRate, 30.0f);
}

TEST_F (AnimationCompositionTests, Create_DefaultFrameRange)
{
    EXPECT_FLOAT_EQ (comp->startFrame, 0.0f);
    EXPECT_FLOAT_EQ (comp->endFrame, 60.0f);
}

TEST_F (AnimationCompositionTests, Create_DefaultLayersEmpty)
{
    EXPECT_TRUE (comp->layers.empty());
}

// =============================================================================
// Timing helpers
// =============================================================================

TEST_F (AnimationCompositionTests, TotalFrames_IsEndMinusStart)
{
    comp->startFrame = 5.0f;
    comp->endFrame = 65.0f;
    EXPECT_FLOAT_EQ (comp->totalFrames(), 60.0f);
}

TEST_F (AnimationCompositionTests, Duration_IsTotalFramesDividedByFrameRate)
{
    EXPECT_FLOAT_EQ (comp->duration(), 60.0f / 30.0f);
}

TEST_F (AnimationCompositionTests, FrameAtProgress_ZeroMapsToStartFrame)
{
    comp->startFrame = 5.0f;
    comp->endFrame = 65.0f;
    EXPECT_FLOAT_EQ (comp->frameAtProgress (0.0f), 5.0f);
}

TEST_F (AnimationCompositionTests, FrameAtProgress_OneMapsToEndFrame)
{
    comp->startFrame = 5.0f;
    comp->endFrame = 65.0f;
    EXPECT_FLOAT_EQ (comp->frameAtProgress (1.0f), 65.0f);
}

TEST_F (AnimationCompositionTests, FrameAtProgress_HalfMapsToMidpoint)
{
    comp->startFrame = 0.0f;
    comp->endFrame = 60.0f;
    EXPECT_FLOAT_EQ (comp->frameAtProgress (0.5f), 30.0f);
}

TEST_F (AnimationCompositionTests, FrameAtProgress_ClampsBelowZero)
{
    EXPECT_FLOAT_EQ (comp->frameAtProgress (-1.0f), comp->startFrame);
}

TEST_F (AnimationCompositionTests, FrameAtProgress_ClampsAboveOne)
{
    EXPECT_FLOAT_EQ (comp->frameAtProgress (2.0f), comp->endFrame);
}

TEST_F (AnimationCompositionTests, FrameAtTime_ZeroSecondsIsStartFrame)
{
    EXPECT_FLOAT_EQ (comp->frameAtTime (0.0f), 0.0f);
}

TEST_F (AnimationCompositionTests, FrameAtTime_ConvertsSecondsToFrames)
{
    // 1 second at 30fps = 30 frames from startFrame
    EXPECT_FLOAT_EQ (comp->frameAtTime (1.0f), 30.0f);
    EXPECT_FLOAT_EQ (comp->frameAtTime (2.0f), 60.0f);
}

// =============================================================================
// Layer builders
// =============================================================================

TEST_F (AnimationCompositionTests, AddShapeLayer_AppendsShapeLayerAndReturnsPtr)
{
    auto* layer = comp->addShapeLayer ("MyShape");

    ASSERT_NE (layer, nullptr);
    EXPECT_EQ (comp->layers.size(), 1u);
    EXPECT_EQ (layer->getType(), AnimationLayer::Type::Shape);
    EXPECT_EQ (layer->name, String ("MyShape"));
}

TEST_F (AnimationCompositionTests, AddNullLayer_AppendsNullLayer)
{
    auto* layer = comp->addNullLayer ("Control");

    ASSERT_NE (layer, nullptr);
    EXPECT_EQ (comp->layers.size(), 1u);
    EXPECT_EQ (layer->getType(), AnimationLayer::Type::Null);
    EXPECT_EQ (layer->name, String ("Control"));
}

TEST_F (AnimationCompositionTests, AddSolidLayer_AppendsSolidLayer)
{
    const Color red (0xffff0000);
    auto* layer = comp->addSolidLayer ("Background", red, { 200.0f, 150.0f });

    ASSERT_NE (layer, nullptr);
    EXPECT_EQ (comp->layers.size(), 1u);
    EXPECT_EQ (layer->getType(), AnimationLayer::Type::Solid);
    EXPECT_EQ (layer->solidColor, red);
}

TEST_F (AnimationCompositionTests, AddMultipleLayers_CountsCorrectly)
{
    comp->addShapeLayer ("Shape");
    comp->addNullLayer ("Null");
    comp->addSolidLayer ("Solid", Color (0xff000000), { 200.0f, 150.0f });

    EXPECT_EQ (comp->layers.size(), 3u);
}

TEST_F (AnimationCompositionTests, AddShapeLayer_AssignsIncrementalIds)
{
    auto* a = comp->addShapeLayer ("A");
    auto* b = comp->addNullLayer ("B");

    EXPECT_NE (a->id, b->id);
    EXPECT_EQ (b->id, a->id + 1);
}

TEST_F (AnimationCompositionTests, AddShapeLayer_SetsFrameRange)
{
    comp->startFrame = 5.0f;
    comp->endFrame = 55.0f;
    auto* layer = comp->addShapeLayer ("Layer");

    EXPECT_FLOAT_EQ (layer->inFrame, 5.0f);
    EXPECT_FLOAT_EQ (layer->outFrame, 55.0f);
}

// =============================================================================
// findLayerById
// =============================================================================

TEST_F (AnimationCompositionTests, FindLayerById_ReturnsCorrectLayer)
{
    auto* a = comp->addShapeLayer ("A");
    auto* b = comp->addNullLayer ("B");

    EXPECT_EQ (comp->findLayerById (a->id), a);
    EXPECT_EQ (comp->findLayerById (b->id), b);
}

TEST_F (AnimationCompositionTests, FindLayerById_ReturnsNullForMissingId)
{
    comp->addShapeLayer ("Layer");
    EXPECT_EQ (comp->findLayerById (9999), nullptr);
}

// =============================================================================
// findMarker
// =============================================================================

TEST_F (AnimationCompositionTests, FindMarker_ReturnsCorrectMarker)
{
    comp->markers.push_back ({ "intro", 0.0f, 30.0f });
    comp->markers.push_back ({ "outro", 60.0f, 30.0f });

    auto* m = comp->findMarker ("intro");
    ASSERT_NE (m, nullptr);
    EXPECT_EQ (m->comment, String ("intro"));
    EXPECT_FLOAT_EQ (m->startFrame, 0.0f);
    EXPECT_FLOAT_EQ (m->duration, 30.0f);
}

TEST_F (AnimationCompositionTests, FindMarker_ReturnsNullForMissingName)
{
    comp->markers.push_back ({ "intro", 0.0f, 30.0f });
    EXPECT_EQ (comp->findMarker ("nonexistent"), nullptr);
}

// =============================================================================
// computeStats
// =============================================================================

TEST_F (AnimationCompositionTests, ComputeStats_EmptyComposition)
{
    const auto stats = comp->computeStats();
    EXPECT_EQ (stats.totalLayerCount(), 0);
}

TEST_F (AnimationCompositionTests, ComputeStats_CountsEachLayerType)
{
    comp->addShapeLayer ("Shape");
    comp->addNullLayer ("Null");
    comp->addSolidLayer ("Solid", Color (0xff000000), { 100.0f, 100.0f });

    const auto stats = comp->computeStats();
    EXPECT_EQ (stats.shapeLayerCount, 1);
    EXPECT_EQ (stats.nullLayerCount, 1);
    EXPECT_EQ (stats.solidLayerCount, 1);
    EXPECT_EQ (stats.totalLayerCount(), 3);
}

TEST_F (AnimationCompositionTests, ComputeStats_MultipleShapeLayers)
{
    comp->addShapeLayer ("A");
    comp->addShapeLayer ("B");
    comp->addShapeLayer ("C");

    const auto stats = comp->computeStats();
    EXPECT_EQ (stats.shapeLayerCount, 3);
    EXPECT_EQ (stats.totalLayerCount(), 3);
}

// =============================================================================
// Property overrides
// =============================================================================

TEST_F (AnimationCompositionTests, PropertyOverride_MissingKeyPathReturnsNull)
{
    EXPECT_EQ (comp->getPropertyOverride ("nonexistent"), nullptr);
}

TEST_F (AnimationCompositionTests, PropertyOverride_SetAndGetFindable)
{
    comp->setPropertyOverride<Color> ("Layer.Fill",
                                      AnimationPropertyID::FillColor,
                                      [] (float) -> std::optional<Color>
    {
        return Color (0xffff0000);
    });

    auto* set = comp->getPropertyOverride ("Layer.Fill");
    ASSERT_NE (set, nullptr);
    EXPECT_TRUE (set->hasOverride (AnimationPropertyID::FillColor));
}

TEST_F (AnimationCompositionTests, PropertyOverride_EvaluatesFloatCallback)
{
    comp->setPropertyOverride<float> ("Layer.Stroke",
                                      AnimationPropertyID::StrokeWidth,
                                      [] (float) -> std::optional<float>
    {
        return 5.0f;
    });

    auto* set = comp->getPropertyOverride ("Layer.Stroke");
    ASSERT_NE (set, nullptr);
    EXPECT_FLOAT_EQ (set->evaluateFloat (AnimationPropertyID::StrokeWidth, 0.0f, 1.0f), 5.0f);
}

TEST_F (AnimationCompositionTests, PropertyOverride_EvaluatesColorCallback)
{
    const Color override (0xffaabbcc);

    comp->setPropertyOverride<Color> ("Layer.Fill",
                                      AnimationPropertyID::FillColor,
                                      [override] (float) -> std::optional<Color>
    {
        return override;
    });

    auto* set = comp->getPropertyOverride ("Layer.Fill");
    ASSERT_NE (set, nullptr);
    EXPECT_EQ (set->evaluateColor (AnimationPropertyID::FillColor, 0.0f, Color()), override);
}

TEST_F (AnimationCompositionTests, PropertyOverride_NulloptCallbackReturnsFallback)
{
    comp->setPropertyOverride<float> ("Layer.Fill",
                                      AnimationPropertyID::FillOpacity,
                                      [] (float) -> std::optional<float>
    {
        return std::nullopt;
    });

    auto* set = comp->getPropertyOverride ("Layer.Fill");
    ASSERT_NE (set, nullptr);
    EXPECT_FLOAT_EQ (set->evaluateFloat (AnimationPropertyID::FillOpacity, 0.0f, 0.75f), 0.75f);
}

TEST_F (AnimationCompositionTests, PropertyOverride_NoRegisteredOverrideReturnsFallback)
{
    comp->setPropertyOverride<Color> ("Layer.Fill",
                                      AnimationPropertyID::FillColor,
                                      [] (float) -> std::optional<Color>
    {
        return std::nullopt;
    });

    auto* set = comp->getPropertyOverride ("Layer.Fill");
    ASSERT_NE (set, nullptr);
    EXPECT_FALSE (set->hasOverride (AnimationPropertyID::StrokeColor));
    EXPECT_FLOAT_EQ (set->evaluateFloat (AnimationPropertyID::StrokeWidth, 0.0f, 3.0f), 3.0f);
}
