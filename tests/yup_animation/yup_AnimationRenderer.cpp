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

constexpr const char* kShapeLayerJson = R"json({
    "v": "5.5.2",
    "nm": "ShapeTest",
    "ip": 0,
    "op": 30,
    "fr": 25.0,
    "w": 100,
    "h": 100,
    "ddd": 0,
    "assets": [],
    "layers": [
        {
            "ty": 4,
            "nm": "MyShapeLayer",
            "ind": 1,
            "ip": 0,
            "op": 30,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [50, 50] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 100 }
            },
            "shapes": [
                {
                    "ty": "gr",
                    "nm": "Group",
                    "it": [
                        {
                            "ty": "rc",
                            "nm": "Rect",
                            "p": { "a": 0, "k": [0, 0] },
                            "s": { "a": 0, "k": [40, 40] },
                            "r": { "a": 0, "k": 0 }
                        },
                        {
                            "ty": "fl",
                            "nm": "Fill",
                            "c": { "a": 0, "k": [1, 0, 0, 1] },
                            "o": { "a": 0, "k": 100 },
                            "r": 1
                        }
                    ]
                }
            ]
        }
    ]
})json";

constexpr const char* kSolidLayerJson = R"json({
    "v": "5.5.2",
    "nm": "SolidTest",
    "ip": 0,
    "op": 30,
    "fr": 25.0,
    "w": 100,
    "h": 100,
    "ddd": 0,
    "assets": [],
    "layers": [
        {
            "ty": 1,
            "nm": "MySolidLayer",
            "ind": 1,
            "ip": 0,
            "op": 30,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "sc": "#ff0000",
            "sw": 100,
            "sh": 100,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [50, 50] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 100 }
            }
        }
    ]
})json";

constexpr const char* kNullLayerJson = R"json({
    "v": "5.5.2",
    "nm": "NullTest",
    "ip": 0,
    "op": 30,
    "fr": 25.0,
    "w": 100,
    "h": 100,
    "ddd": 0,
    "assets": [],
    "layers": [
        {
            "ty": 3,
            "nm": "MyNullLayer",
            "ind": 1,
            "ip": 0,
            "op": 30,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [50, 50] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 0 }
            }
        }
    ]
})json";

constexpr const char* kHiddenLayerJson = R"json({
    "v": "5.5.2",
    "nm": "HiddenTest",
    "ip": 0,
    "op": 30,
    "fr": 25.0,
    "w": 100,
    "h": 100,
    "ddd": 0,
    "assets": [],
    "layers": [
        {
            "ty": 4,
            "nm": "HiddenLayer",
            "ind": 1,
            "ip": 0,
            "op": 30,
            "st": 0,
            "sr": 1,
            "hd": true,
            "bm": 0,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [50, 50] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 100 }
            },
            "shapes": []
        }
    ]
})json";

constexpr const char* kMultiLayerJson = R"json({
    "v": "5.5.2",
    "nm": "MultiLayerTest",
    "ip": 0,
    "op": 60,
    "fr": 30.0,
    "w": 200,
    "h": 200,
    "ddd": 0,
    "assets": [],
    "layers": [
        {
            "ty": 4,
            "nm": "TopShape",
            "ind": 1,
            "ip": 0,
            "op": 60,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [100, 100] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 80 }
            },
            "shapes": []
        },
        {
            "ty": 3,
            "nm": "ControlNull",
            "ind": 2,
            "ip": 0,
            "op": 60,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [0, 0] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 0 }
            }
        }
    ]
})json";

} // namespace

class AnimationRendererTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        context = GraphicsContext::createContext (GraphicsContext::Headless, {});
        ASSERT_NE (context, nullptr);
    }

    std::unique_ptr<GraphicsContext> context;
};

TEST_F (AnimationRendererTests, RenderEmptyCompositionDoesNotCrash)
{
    auto comp = AnimationComposition::create ({ 100.0f, 100.0f }, 25.0f);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

TEST_F (AnimationRendererTests, RenderShapeLayerCompositionDoesNotCrash)
{
    auto comp = LottieReader::parseData (kShapeLayerJson);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

TEST_F (AnimationRendererTests, RenderSolidLayerCompositionDoesNotCrash)
{
    auto comp = LottieReader::parseData (kSolidLayerJson);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

TEST_F (AnimationRendererTests, RenderNullLayerCompositionDoesNotCrash)
{
    auto comp = LottieReader::parseData (kNullLayerJson);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

TEST_F (AnimationRendererTests, RenderHiddenLayerCompositionDoesNotCrash)
{
    auto comp = LottieReader::parseData (kHiddenLayerJson);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 1u);
    EXPECT_TRUE (comp->layers[0]->hidden);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

TEST_F (AnimationRendererTests, RenderMultiLayerCompositionDoesNotCrash)
{
    auto comp = LottieReader::parseData (kMultiLayerJson);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 2u);

    auto renderer = context->makeRenderer (200, 200);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 200, 200));
    });
}

TEST_F (AnimationRendererTests, RenderAtVariousFrameNumbersDoesNotCrash)
{
    auto comp = LottieReader::parseData (kShapeLayerJson);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    const Rectangle<float> bounds (0, 0, 100, 100);

    for (const float frame : { 0.0f, 1.0f, 15.0f, 29.0f, 30.0f, 60.0f })
    {
        EXPECT_NO_THROW ({
            AnimationRenderer::renderComposition (g, *comp, frame, bounds);
        });
    }
}

TEST_F (AnimationRendererTests, RenderWithKeepAspectRatioTrueAndFalseDoesNotCrash)
{
    auto comp = LottieReader::parseData (kShapeLayerJson);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (200, 100);
    Graphics g (*context, *renderer);

    const Rectangle<float> bounds (0, 0, 200, 100);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, bounds, true);
    });

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, bounds, false);
    });
}

TEST_F (AnimationRendererTests, RenderIntoSmallBoundsDoesNotCrash)
{
    auto comp = LottieReader::parseData (kShapeLayerJson);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (1, 1);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 1, 1));
    });
}

TEST_F (AnimationRendererTests, RenderIntoLargeBoundsDoesNotCrash)
{
    auto comp = LottieReader::parseData (kShapeLayerJson);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (1000, 1000);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 1000, 1000));
    });
}

TEST_F (AnimationRendererTests, RenderProgrammaticallyBuiltCompositionDoesNotCrash)
{
    auto comp = AnimationComposition::create ({ 200.0f, 200.0f }, 30.0f);
    ASSERT_NE (comp, nullptr);

    comp->addNullLayer ("NullLayer");
    comp->addSolidLayer ("SolidLayer", Color (0xffff0000), { 200.0f, 200.0f });
    comp->addShapeLayer ("ShapeLayer");

    EXPECT_EQ (comp->layers.size(), 3u);

    auto renderer = context->makeRenderer (200, 200);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 200, 200));
    });
}
