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

constexpr const char* kMinimalLottieJson = R"json({
    "v": "5.5.2",
    "nm": "Test",
    "ip": 0,
    "op": 30,
    "fr": 25.0,
    "w": 200,
    "h": 200,
    "ddd": 0,
    "assets": [],
    "layers": [
        {
            "ty": 4,
            "nm": "ShapeLayer",
            "ind": 1,
            "ip": 0,
            "op": 30,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [100, 100] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 100 }
            },
            "shapes": []
        }
    ]
})json";

constexpr const char* kAnimatedPathJson = R"json({
    "v": "5.5.2",
    "nm": "AnimatedPath",
    "ip": 0,
    "op": 10,
    "fr": 10.0,
    "w": 100,
    "h": 100,
    "ddd": 0,
    "assets": [],
    "layers": [
        {
            "ty": 4,
            "nm": "ShapeLayer",
            "ind": 1,
            "ip": 0,
            "op": 10,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [0, 0] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 100 }
            },
            "shapes": [
                {
                    "ty": "sh",
                    "nm": "Path",
                    "ks": {
                        "a": 1,
                        "k": [
                            {
                                "t": 0,
                                "s": [
                                    {
                                        "i": [[0, 0], [0, 0], [0, 0]],
                                        "o": [[0, 0], [0, 0], [0, 0]],
                                        "v": [[0, 0], [10, 0], [10, 10]],
                                        "c": true
                                    }
                                ],
                                "e": [
                                    {
                                        "i": [[0, 0], [0, 0], [0, 0]],
                                        "o": [[0, 0], [0, 0], [0, 0]],
                                        "v": [[0, 0], [20, 0], [20, 20]],
                                        "c": true
                                    }
                                ]
                            },
                            {
                                "t": 10,
                                "s": [
                                    {
                                        "i": [[0, 0], [0, 0], [0, 0]],
                                        "o": [[0, 0], [0, 0], [0, 0]],
                                        "v": [[0, 0], [20, 0], [20, 20]],
                                        "c": true
                                    }
                                ]
                            }
                        ]
                    },
                    "d": 1
                }
            ]
        }
    ]
})json";

} // namespace

class LottieRoundtripTests : public ::testing::Test
{
};

TEST_F (LottieRoundtripTests, ParseMinimalJson)
{
    auto comp = LottieReader::parseData (kMinimalLottieJson);
    ASSERT_NE (comp, nullptr);

    EXPECT_EQ (comp->name, "Test");
    EXPECT_NEAR (comp->frameRate, 25.0f, 0.01f);
    EXPECT_NEAR (comp->startFrame, 0.0f, 0.01f);
    EXPECT_NEAR (comp->endFrame, 30.0f, 0.01f);
    EXPECT_NEAR (comp->size.getWidth(), 200.0f, 0.01f);
    EXPECT_NEAR (comp->size.getHeight(), 200.0f, 0.01f);
}

TEST_F (LottieRoundtripTests, ParsedLayerCountMatchesInput)
{
    auto comp = LottieReader::parseData (kMinimalLottieJson);
    ASSERT_NE (comp, nullptr);

    EXPECT_EQ (comp->layers.size(), 1u);
    EXPECT_NE (comp->layers[0], nullptr);
    EXPECT_EQ (comp->layers[0]->getType(), AnimationLayer::Type::Shape);
}

TEST_F (LottieRoundtripTests, WriteAndReparse)
{
    auto original = LottieReader::parseData (kMinimalLottieJson);
    ASSERT_NE (original, nullptr);

    const String json = LottieWriter::toJson (*original);
    EXPECT_FALSE (json.isEmpty());

    auto reparsed = LottieReader::parseData (json);
    ASSERT_NE (reparsed, nullptr);

    EXPECT_EQ (reparsed->name, original->name);
    EXPECT_NEAR (reparsed->frameRate, original->frameRate, 0.01f);
    EXPECT_EQ (reparsed->layers.size(), original->layers.size());
}

TEST_F (LottieRoundtripTests, AnimationLoadFromData)
{
    auto anim = Animation::loadFromData (kMinimalLottieJson);
    EXPECT_TRUE (anim.isValid());
    EXPECT_NEAR (anim.frameRate(), 25.0f, 0.01f);
    EXPECT_NEAR (anim.totalFrames(), 30.0f, 0.01f);
    EXPECT_NEAR (anim.duration(), 1.2f, 0.05f);
}

TEST_F (LottieRoundtripTests, AnimationInvalidData)
{
    auto anim = Animation::loadFromData ("not valid json");
    EXPECT_FALSE (anim.isValid());
}

TEST_F (LottieRoundtripTests, CompositionFactoryCreatesValidComp)
{
    auto comp = AnimationComposition::create ({ 400.0f, 300.0f }, 30.0f);
    ASSERT_NE (comp, nullptr);

    EXPECT_NEAR (comp->size.getWidth(), 400.0f, 0.01f);
    EXPECT_NEAR (comp->size.getHeight(), 300.0f, 0.01f);
    EXPECT_NEAR (comp->frameRate, 30.0f, 0.01f);
    EXPECT_GT (comp->totalFrames(), 0.0f);
}

TEST_F (LottieRoundtripTests, CompositionCanAddShapeLayer)
{
    auto comp = AnimationComposition::create ({ 400.0f, 300.0f }, 30.0f);
    comp->endFrame = 60.0f;

    ShapeLayer* layer = comp->addShapeLayer ("TestLayer");
    ASSERT_NE (layer, nullptr);
    EXPECT_EQ (layer->name, "TestLayer");
    EXPECT_EQ (comp->layers.size(), 1u);
}

TEST_F (LottieRoundtripTests, AnimatedPathKeyframesUnwrapShapeValues)
{
    auto comp = LottieReader::parseData (kAnimatedPathJson);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 1u);

    auto* layer = static_cast<ShapeLayer*> (comp->layers[0].get());
    ASSERT_NE (layer, nullptr);
    ASSERT_EQ (layer->groups.size(), 1u);
    ASSERT_EQ (layer->groups[0]->children.size(), 1u);

    auto* shape = static_cast<BezierPathShape*> (layer->groups[0]->children[0].shape.get());
    ASSERT_NE (shape, nullptr);

    const auto startPath = shape->pathData.getValueAt (0.0f);
    const auto endPath = shape->pathData.getValueAt (10.0f);

    ASSERT_EQ (startPath.vertices.size(), 3u);
    ASSERT_EQ (endPath.vertices.size(), 3u);
    EXPECT_NEAR (startPath.vertices[1].getX(), 10.0f, 0.01f);
    EXPECT_NEAR (endPath.vertices[1].getX(), 20.0f, 0.01f);
}
