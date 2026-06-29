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

constexpr const char* kLayerPositionExpressionJson = R"json({
    "v": "5.5.2",
    "nm": "PositionExpression",
    "ip": 0,
    "op": 30,
    "fr": 30.0,
    "w": 200,
    "h": 200,
    "ddd": 0,
    "assets": [],
    "layers": [
        {
            "ty": 4,
            "nm": "TargetLayer",
            "ind": 1,
            "ip": 0,
            "op": 30,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": {
                    "a": 0,
                    "k": [50, 60],
                    "x": "var $bm_rt;\n$bm_rt = thisComp.layer('Null 1').transform.position;"
                },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 100 }
            },
            "shapes": []
        },
        {
            "ty": 3,
            "nm": "Null 1",
            "ind": 2,
            "ip": 0,
            "op": 30,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": {
                    "a": 1,
                    "k": [
                        { "t": 0, "s": [10, 20], "e": [30, 40], "i": { "x": 0.833, "y": 0.833 }, "o": { "x": 0.167, "y": 0.167 } },
                        { "t": 10 }
                    ]
                },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 0 }
            }
        }
    ]
})json";

// Tests two layers sharing the same position expression (night_own.json pattern).
constexpr const char* kMultiLayerPositionExpressionJson = R"json({
    "v": "5.5.2",
    "nm": "MultiLayerPositionExpression",
    "ip": 0,
    "op": 30,
    "fr": 30.0,
    "w": 200,
    "h": 200,
    "ddd": 0,
    "assets": [],
    "layers": [
        {
            "ty": 4,
            "nm": "LayerA",
            "ind": 1,
            "ip": 0, "op": 30, "st": 0, "sr": 1, "hd": false, "bm": 0,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [50, 60], "x": "var $bm_rt;\n$bm_rt = thisComp.layer('Null 1').transform.position;" },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 100 }
            },
            "shapes": []
        },
        {
            "ty": 4,
            "nm": "LayerB",
            "ind": 2,
            "ip": 0, "op": 30, "st": 0, "sr": 1, "hd": false, "bm": 0,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [99, 99], "x": "var $bm_rt;\n$bm_rt = thisComp.layer('Null 1').transform.position;" },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 100 }
            },
            "shapes": []
        },
        {
            "ty": 3,
            "nm": "Null 1",
            "ind": 3,
            "ip": 0, "op": 30, "st": 0, "sr": 1, "hd": false, "bm": 0,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": {
                    "a": 1,
                    "k": [
                        { "t": 0, "s": [10, 20], "e": [30, 40], "i": { "x": 0.833, "y": 0.833 }, "o": { "x": 0.167, "y": 0.167 } },
                        { "t": 10 }
                    ]
                },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 0 }
            }
        }
    ]
})json";

constexpr const char* kPrecompAssetLayerExpressionJson = R"json({
    "v": "5.5.2",
    "nm": "PrecompAssetLayerExpression",
    "ip": 0,
    "op": 30,
    "fr": 30.0,
    "w": 200,
    "h": 200,
    "ddd": 0,
    "assets": [
        {
            "id": "pre_0",
            "layers": [
                {
                    "ty": 4,
                    "nm": "AssetLayer",
                    "ind": 1,
                    "ip": 0, "op": 30, "st": 0, "sr": 1, "hd": false, "bm": 0,
                    "ks": {
                        "a": { "a": 0, "k": [0, 0] },
                        "p": { "a": 0, "k": [50, 60], "x": "var $bm_rt;\n$bm_rt = thisComp.layer('Asset Null').transform.position;" },
                        "s": { "a": 0, "k": [100, 100] },
                        "r": { "a": 0, "k": 0 },
                        "o": { "a": 0, "k": 100 }
                    },
                    "shapes": []
                },
                {
                    "ty": 3,
                    "nm": "Asset Null",
                    "ind": 2,
                    "ip": 0, "op": 30, "st": 0, "sr": 1, "hd": false, "bm": 0,
                    "ks": {
                        "a": { "a": 0, "k": [0, 0] },
                        "p": { "a": 0, "k": [12, 34] },
                        "s": { "a": 0, "k": [100, 100] },
                        "r": { "a": 0, "k": 0 },
                        "o": { "a": 0, "k": 0 }
                    }
                }
            ]
        }
    ],
    "layers": [
        {
            "ty": 0,
            "nm": "Precomp",
            "ind": 1,
            "refId": "pre_0",
            "w": 200,
            "h": 200,
            "ip": 0, "op": 30, "st": 0, "sr": 1, "hd": false, "bm": 0,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [0, 0] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 100 }
            }
        }
    ]
})json";

constexpr const char* kShapeContentExpressionJson = R"json({
    "v": "5.5.2",
    "nm": "ShapeContentExpression",
    "ip": 0,
    "op": 30,
    "fr": 30.0,
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
                "p": { "a": 0, "k": [0, 0] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 100 }
            },
            "shapes": [
                {
                    "ty": "gr",
                    "nm": "Shape 2",
                    "it": [
                        {
                            "ty": "sh",
                            "nm": "Path 1",
                            "ks": {
                                "a": 0,
                                "k": { "i": [[0, 0], [0, 0]], "o": [[0, 0], [0, 0]], "v": [[0, 0], [1, 1]], "c": false },
                                "x": "var $bm_rt;\n$bm_rt = content('Shape 1').content('Path 1').path;"
                            }
                        },
                        { "ty": "st", "nm": "Stroke 1", "c": { "a": 0, "k": [0, 0, 0, 1] }, "o": { "a": 0, "k": 100 }, "w": { "a": 0, "k": 10 } },
                        {
                            "ty": "tr",
                            "p": { "a": 0, "k": [0, 0] },
                            "a": { "a": 0, "k": [0, 0] },
                            "s": { "a": 0, "k": [100, 100] },
                            "r": { "a": 0, "k": 0, "x": "var $bm_rt;\n$bm_rt = content('Shape 1').transform.rotation;" },
                            "o": { "a": 0, "k": 100 }
                        }
                    ]
                },
                {
                    "ty": "gr",
                    "nm": "Shape 1",
                    "it": [
                        {
                            "ty": "sh",
                            "nm": "Path 1",
                            "ks": {
                                "a": 1,
                                "k": [
                                    {
                                        "t": 0,
                                        "s": [{ "i": [[0, 0], [0, 0]], "o": [[0, 0], [0, 0]], "v": [[0, 0], [10, 0]], "c": false }],
                                        "e": [{ "i": [[0, 0], [0, 0]], "o": [[0, 0], [0, 0]], "v": [[0, 0], [20, 0]], "c": false }],
                                        "i": { "x": 0.833, "y": 0.833 },
                                        "o": { "x": 0.167, "y": 0.167 }
                                    },
                                    { "t": 10 }
                                ]
                            }
                        },
                        { "ty": "st", "nm": "Stroke 1", "c": { "a": 0, "k": [1, 0, 0, 1] }, "o": { "a": 0, "k": 100 }, "w": { "a": 0, "k": 10 } },
                        {
                            "ty": "tr",
                            "p": { "a": 0, "k": [0, 0] },
                            "a": { "a": 0, "k": [0, 0] },
                            "s": { "a": 0, "k": [100, 100] },
                            "r": {
                                "a": 1,
                                "k": [
                                    { "t": 0, "s": [0], "e": [90], "i": { "x": 0.833, "y": 0.833 }, "o": { "x": 0.167, "y": 0.167 } },
                                    { "t": 10 }
                                ]
                            },
                            "o": { "a": 0, "k": 100 }
                        }
                    ]
                }
            ]
        }
    ]
})json";

constexpr const char* kDropShadowJson = R"json({
    "v": "5.5.2",
    "nm": "DropShadow",
    "ip": 0,
    "op": 30,
    "fr": 30.0,
    "w": 100,
    "h": 100,
    "ddd": 0,
    "assets": [],
    "layers": [
        {
            "ty": 4,
            "nm": "ShadowLayer",
            "ind": 1,
            "ip": 0,
            "op": 30,
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
            "ef": [
                {
                    "ty": 25,
                    "nm": "Drop Shadow",
                    "mn": "ADBE Drop Shadow",
                    "en": 1,
                    "ef": [
                        { "ty": 2, "nm": "Shadow Color", "mn": "ADBE Drop Shadow-0001", "v": { "a": 0, "k": [0, 0, 0, 1] } },
                        { "ty": 0, "nm": "Opacity", "mn": "ADBE Drop Shadow-0002", "v": { "a": 0, "k": 51 } },
                        { "ty": 0, "nm": "Direction", "mn": "ADBE Drop Shadow-0003", "v": { "a": 0, "k": 135 } },
                        { "ty": 0, "nm": "Distance", "mn": "ADBE Drop Shadow-0004", "v": { "a": 0, "k": 6 } },
                        { "ty": 0, "nm": "Softness", "mn": "ADBE Drop Shadow-0005", "v": { "a": 0, "k": 0 } },
                        { "ty": 7, "nm": "Shadow Only", "mn": "ADBE Drop Shadow-0006", "v": { "a": 0, "k": 0 } }
                    ]
                }
            ],
            "shapes": []
        }
    ]
})json";

constexpr const char* kFillEffectJson = R"json({
    "v": "5.5.2",
    "nm": "FillEffect",
    "ip": 0,
    "op": 30,
    "fr": 30.0,
    "w": 100,
    "h": 100,
    "ddd": 0,
    "assets": [],
    "layers": [
        {
            "ty": 4,
            "nm": "FilledLayer",
            "ind": 1,
            "ip": 0,
            "op": 30,
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
            "ef": [
                {
                    "ty": 21,
                    "nm": "Fill",
                    "mn": "ADBE Fill",
                    "en": 1,
                    "ef": [
                        { "ty": 2, "nm": "Color", "mn": "ADBE Fill-0002", "v": { "a": 0, "k": [0.25, 0.5, 0.75, 1] } },
                        { "ty": 0, "nm": "Opacity", "mn": "ADBE Fill-0005", "v": { "a": 0, "k": 1 } }
                    ]
                }
            ],
            "shapes": []
        }
    ]
})json";

constexpr const char* kNoneMaskJson = R"json({
    "v": "5.5.2",
    "nm": "NoneMask",
    "ip": 0,
    "op": 30,
    "fr": 30.0,
    "w": 100,
    "h": 100,
    "ddd": 0,
    "assets": [],
    "layers": [
        {
            "ty": 4,
            "nm": "MaskedLayer",
            "ind": 1,
            "ip": 0,
            "op": 30,
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
            "masksProperties": [
                {
                    "inv": false,
                    "mode": "n",
                    "pt": {
                        "a": 0,
                        "k": {
                            "i": [[0, 0], [0, 0], [0, 0], [0, 0]],
                            "o": [[0, 0], [0, 0], [0, 0], [0, 0]],
                            "v": [[0, 0], [100, 0], [100, 100], [0, 100]],
                            "c": true
                        }
                    },
                    "o": { "a": 0, "k": 100 },
                    "x": { "a": 0, "k": 0 },
                    "nm": "Mask 1"
                }
            ],
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

constexpr const char* kTerminalKeyframeJson = R"json({
    "v": "5.5.2",
    "nm": "TerminalKeyframe",
    "ip": 0,
    "op": 20,
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
            "op": 20,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [0, 0] },
                "s": {
                    "a": 1,
                    "k": [
                        { "t": 0, "s": [100, 100, 100], "e": [130, 80, 100] },
                        { "t": 10, "s": [130, 80, 100], "e": [100, 100, 100] },
                        { "t": 17 }
                    ]
                },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 100 }
            },
            "shapes": []
        }
    ]
})json";

constexpr const char* kNestedPrecompImageJson = R"json({
    "v": "5.5.2",
    "nm": "NestedPrecompImage",
    "ip": 0,
    "op": 30,
    "fr": 30.0,
    "w": 1,
    "h": 1,
    "ddd": 0,
    "assets": [
        {
            "id": "image_0",
            "w": 1,
            "h": 1,
            "u": "",
            "p": "nested.png",
            "e": 0
        },
        {
            "id": "comp_b",
            "layers": [
                {
                    "ty": 2,
                    "nm": "NestedImage",
                    "ind": 1,
                    "refId": "image_0",
                    "ip": 0,
                    "op": 30,
                    "st": 0,
                    "sr": 1,
                    "ks": {
                        "a": { "a": 0, "k": [0, 0] },
                        "p": { "a": 0, "k": [0, 0] },
                        "s": { "a": 0, "k": [100, 100] },
                        "r": { "a": 0, "k": 0 },
                        "o": { "a": 0, "k": 100 }
                    }
                }
            ]
        },
        {
            "id": "comp_a",
            "layers": [
                {
                    "ty": 0,
                    "nm": "NestedPrecomp",
                    "ind": 1,
                    "refId": "comp_b",
                    "w": 1,
                    "h": 1,
                    "ip": 0,
                    "op": 30,
                    "st": 0,
                    "sr": 1,
                    "ks": {
                        "a": { "a": 0, "k": [0, 0] },
                        "p": { "a": 0, "k": [0, 0] },
                        "s": { "a": 0, "k": [100, 100] },
                        "r": { "a": 0, "k": 0 },
                        "o": { "a": 0, "k": 100 }
                    }
                }
            ]
        }
    ],
    "layers": [
        {
            "ty": 0,
            "nm": "RootPrecomp",
            "ind": 1,
            "refId": "comp_a",
            "w": 1,
            "h": 1,
            "ip": 0,
            "op": 30,
            "st": 0,
            "sr": 1,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [0, 0] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 100 }
            }
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

TEST_F (LottieRoundtripTests, ResolvesLayerPositionExpressionReferences)
{
    auto comp = LottieReader::parseData (kLayerPositionExpressionJson);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 2u);

    const auto* targetLayer = comp->layers[0].get();
    ASSERT_NE (targetLayer, nullptr);

    const Point<float> start = targetLayer->transform.positionAt (0.0f);
    EXPECT_NEAR (start.getX(), 10.0f, 0.001f);
    EXPECT_NEAR (start.getY(), 20.0f, 0.001f);

    const Point<float> end = targetLayer->transform.positionAt (10.0f);
    EXPECT_NEAR (end.getX(), 30.0f, 0.001f);
    EXPECT_NEAR (end.getY(), 40.0f, 0.001f);
}

TEST_F (LottieRoundtripTests, ResolvesMultipleLayersWithSamePositionExpression)
{
    auto comp = LottieReader::parseData (kMultiLayerPositionExpressionJson);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 3u);

    // Both LayerA and LayerB must follow Null 1 — not stay at their default positions.
    for (int li = 0; li < 2; ++li)
    {
        const auto* layer = comp->layers[li].get();
        ASSERT_NE (layer, nullptr);

        const Point<float> start = layer->transform.positionAt (0.0f);
        EXPECT_NEAR (start.getX(), 10.0f, 0.001f) << "layer " << li << " start X";
        EXPECT_NEAR (start.getY(), 20.0f, 0.001f) << "layer " << li << " start Y";

        const Point<float> end = layer->transform.positionAt (10.0f);
        EXPECT_NEAR (end.getX(), 30.0f, 0.001f) << "layer " << li << " end X";
        EXPECT_NEAR (end.getY(), 40.0f, 0.001f) << "layer " << li << " end Y";
    }
}

TEST_F (LottieRoundtripTests, ResolvesPrecompAssetLayerPositionExpressionReferences)
{
    auto comp = LottieReader::parseData (kPrecompAssetLayerExpressionJson);
    ASSERT_NE (comp, nullptr);

    auto asset = comp->assets["pre_0"];
    ASSERT_NE (asset, nullptr);
    ASSERT_EQ (asset->layers.size(), 2u);

    const auto* targetLayer = asset->layers[0].get();
    ASSERT_NE (targetLayer, nullptr);

    const Point<float> position = targetLayer->transform.positionAt (0.0f);
    EXPECT_NEAR (position.getX(), 12.0f, 0.001f);
    EXPECT_NEAR (position.getY(), 34.0f, 0.001f);
}

TEST_F (LottieRoundtripTests, ResolvesShapeContentExpressionReferences)
{
    auto comp = LottieReader::parseData (kShapeContentExpressionJson);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 1u);

    auto* layer = static_cast<ShapeLayer*> (comp->layers[0].get());
    ASSERT_NE (layer, nullptr);
    ASSERT_EQ (layer->groups.size(), 2u);

    auto* targetPath = static_cast<BezierPathShape*> (layer->groups[0]->children[0].shape.get());
    ASSERT_NE (targetPath, nullptr);

    const auto startPath = targetPath->pathData.getValueAt (0.0f);
    const auto endPath = targetPath->pathData.getValueAt (10.0f);

    ASSERT_EQ (startPath.vertices.size(), 2u);
    ASSERT_EQ (endPath.vertices.size(), 2u);
    EXPECT_NEAR (startPath.vertices[1].getX(), 10.0f, 0.001f);
    EXPECT_NEAR (endPath.vertices[1].getX(), 20.0f, 0.001f);
    EXPECT_NEAR (layer->groups[0]->transform.rotation.getValueAt (10.0f), 90.0f, 0.001f);
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

TEST_F (LottieRoundtripTests, ParsesDropShadowEffect)
{
    auto comp = LottieReader::parseData (kDropShadowJson);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 1u);

    const auto* layer = comp->layers[0].get();
    ASSERT_NE (layer, nullptr);
    ASSERT_TRUE (layer->dropShadow.has_value());

    const auto& shadow = *layer->dropShadow;
    EXPECT_TRUE (shadow.enabled);
    EXPECT_FALSE (shadow.shadowOnly);
    EXPECT_NEAR (shadow.opacityAt (0.0f), 0.51f, 1.0e-6f);
    EXPECT_NEAR (shadow.distance.getValueAt (0.0f), 6.0f, 1.0e-6f);
    EXPECT_NEAR (shadow.direction.getValueAt (0.0f), 135.0f, 1.0e-6f);
}

TEST_F (LottieRoundtripTests, ParsesFillEffect)
{
    auto comp = LottieReader::parseData (kFillEffectJson);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 1u);

    const auto* layer = comp->layers[0].get();
    ASSERT_NE (layer, nullptr);
    ASSERT_TRUE (layer->fillEffect.has_value());

    const auto& fill = *layer->fillEffect;
    EXPECT_TRUE (fill.enabled);
    EXPECT_NEAR (fill.opacityAt (0.0f), 1.0f, 1.0e-6f);

    const auto color = fill.colorAt (0.0f);
    EXPECT_NEAR (color.getRed() / 255.0f, 0.25f, 0.01f);
    EXPECT_NEAR (color.getGreen() / 255.0f, 0.5f, 0.01f);
    EXPECT_NEAR (color.getBlue() / 255.0f, 0.75f, 0.01f);
}

TEST_F (LottieRoundtripTests, ParsesNoneMaskMode)
{
    auto comp = LottieReader::parseData (kNoneMaskJson);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 1u);

    const auto* layer = comp->layers[0].get();
    ASSERT_NE (layer, nullptr);
    ASSERT_EQ (layer->masks.size(), 1u);
    ASSERT_NE (layer->masks[0], nullptr);
    EXPECT_EQ (layer->masks[0]->mode, AnimationMask::Mode::None);
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

TEST_F (LottieRoundtripTests, TerminalKeyframeUsesPreviousEndValue)
{
    auto comp = LottieReader::parseData (kTerminalKeyframeJson);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 1u);

    const auto* layer = static_cast<const ShapeLayer*> (comp->layers[0].get());
    ASSERT_NE (layer, nullptr);

    const auto scaleAtEnd = layer->transform.scale.getValueAt (17.0f);
    EXPECT_NEAR (scaleAtEnd.getWidth(), 100.0f, 0.01f);
    EXPECT_NEAR (scaleAtEnd.getHeight(), 100.0f, 0.01f);
}

TEST_F (LottieRoundtripTests, ResolvesImageLayersInsideNestedPrecomps)
{
    LottieLoadOptions options;
    options.imageResolver = [] (const String& ref, const File&) -> std::optional<Image>
    {
        if (ref == "nested.png")
            return Image (1, 1, PixelFormat::RGBA);

        return std::nullopt;
    };

    auto comp = LottieReader::parseData (kNestedPrecompImageJson, options);
    ASSERT_NE (comp, nullptr);

    auto compB = comp->assets["comp_b"];
    ASSERT_NE (compB, nullptr);
    ASSERT_EQ (compB->layers.size(), 1u);

    const auto* layer = compB->layers[0].get();
    ASSERT_NE (layer, nullptr);
    ASSERT_EQ (layer->getType(), AnimationLayer::Type::Image);

    const auto* imageLayer = static_cast<const ImageLayer*> (layer);
    ASSERT_NE (imageLayer, nullptr);
    EXPECT_TRUE (imageLayer->image.has_value());
}
