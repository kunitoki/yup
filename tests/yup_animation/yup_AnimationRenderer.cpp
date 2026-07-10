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

constexpr const char* kPartialOpacityShapeJson = R"json({
    "v": "5.5.2",
    "nm": "OpacityTest",
    "ip": 0,
    "op": 10,
    "fr": 25.0,
    "w": 100,
    "h": 100,
    "ddd": 0,
    "assets": [],
    "layers": [
        {
            "ty": 4,
            "nm": "SemiTransparent",
            "ind": 1,
            "ip": 0,
            "op": 10,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [50, 50] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 50 }
            },
            "shapes": [
                {
                    "ty": "gr",
                    "nm": "Group",
                    "it": [
                        { "ty": "rc", "nm": "Rect", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [40, 40] }, "r": { "a": 0, "k": 0 } },
                        { "ty": "fl", "nm": "Fill", "c": { "a": 0, "k": [0, 1, 0, 1] }, "o": { "a": 0, "k": 100 }, "r": 1 }
                    ]
                }
            ]
        }
    ]
})json";

constexpr const char* kDropShadowShapeJson = R"json({
    "v": "5.5.2",
    "nm": "DropShadowTest",
    "ip": 0,
    "op": 10,
    "fr": 25.0,
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
            "op": 10,
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
            "ef": [
                {
                    "ty": 25,
                    "nm": "Drop Shadow",
                    "mn": "ADBE Drop Shadow",
                    "en": 1,
                    "ef": [
                        { "ty": 2, "nm": "Shadow Color", "mn": "ADBE Drop Shadow-0001", "v": { "a": 0, "k": [0, 0, 0, 1] } },
                        { "ty": 0, "nm": "Opacity",      "mn": "ADBE Drop Shadow-0002", "v": { "a": 0, "k": 51 } },
                        { "ty": 0, "nm": "Direction",    "mn": "ADBE Drop Shadow-0003", "v": { "a": 0, "k": 135 } },
                        { "ty": 0, "nm": "Distance",     "mn": "ADBE Drop Shadow-0004", "v": { "a": 0, "k": 6 } },
                        { "ty": 0, "nm": "Softness",     "mn": "ADBE Drop Shadow-0005", "v": { "a": 0, "k": 0 } },
                        { "ty": 7, "nm": "Shadow Only",  "mn": "ADBE Drop Shadow-0006", "v": { "a": 0, "k": 0 } }
                    ]
                }
            ],
            "shapes": [
                {
                    "ty": "gr",
                    "nm": "Group",
                    "it": [
                        { "ty": "rc", "nm": "Rect", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [40, 40] }, "r": { "a": 0, "k": 0 } },
                        { "ty": "fl", "nm": "Fill", "c": { "a": 0, "k": [0, 0, 1, 1] }, "o": { "a": 0, "k": 100 }, "r": 1 }
                    ]
                }
            ]
        }
    ]
})json";

constexpr const char* kMaskAddJson = R"json({
    "v": "5.5.2",
    "nm": "MaskTest",
    "ip": 0,
    "op": 10,
    "fr": 25.0,
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
            "masksProperties": [
                {
                    "inv": false,
                    "mode": "a",
                    "pt": { "a": 0, "k": { "i": [[0,0],[0,0],[0,0],[0,0]], "o": [[0,0],[0,0],[0,0],[0,0]], "v": [[10,10],[90,10],[90,90],[10,90]], "c": true } },
                    "o": { "a": 0, "k": 100 }
                }
            ],
            "shapes": [
                {
                    "ty": "gr",
                    "nm": "Group",
                    "it": [
                        { "ty": "rc", "nm": "Rect", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [80, 80] }, "r": { "a": 0, "k": 0 } },
                        { "ty": "fl", "nm": "Fill", "c": { "a": 0, "k": [1, 0, 0, 1] }, "o": { "a": 0, "k": 100 }, "r": 1 }
                    ]
                }
            ]
        }
    ]
})json";

constexpr const char* kMaskSubtractJson = R"json({
    "v": "5.5.2",
    "nm": "MaskSubtractTest",
    "ip": 0,
    "op": 10,
    "fr": 25.0,
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
            "masksProperties": [
                {
                    "inv": false,
                    "mode": "s",
                    "pt": { "a": 0, "k": { "i": [[0,0],[0,0],[0,0],[0,0]], "o": [[0,0],[0,0],[0,0],[0,0]], "v": [[20,20],[80,20],[80,80],[20,80]], "c": true } },
                    "o": { "a": 0, "k": 100 }
                }
            ],
            "shapes": [
                {
                    "ty": "gr",
                    "nm": "Group",
                    "it": [
                        { "ty": "rc", "nm": "Rect", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [80, 80] }, "r": { "a": 0, "k": 0 } },
                        { "ty": "fl", "nm": "Fill", "c": { "a": 0, "k": [0, 0, 1, 1] }, "o": { "a": 0, "k": 100 }, "r": 1 }
                    ]
                }
            ]
        }
    ]
})json";

constexpr const char* kMaskIntersectJson = R"json({
    "v": "5.5.2",
    "nm": "MaskIntersectTest",
    "ip": 0,
    "op": 10,
    "fr": 25.0,
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
            "masksProperties": [
                {
                    "inv": false,
                    "mode": "i",
                    "pt": { "a": 0, "k": { "i": [[0,0],[0,0],[0,0],[0,0]], "o": [[0,0],[0,0],[0,0],[0,0]], "v": [[10,10],[90,10],[90,90],[10,90]], "c": true } },
                    "o": { "a": 0, "k": 100 }
                }
            ],
            "shapes": [
                {
                    "ty": "gr",
                    "nm": "Group",
                    "it": [
                        { "ty": "rc", "nm": "Rect", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [80, 80] }, "r": { "a": 0, "k": 0 } },
                        { "ty": "fl", "nm": "Fill", "c": { "a": 0, "k": [0, 1, 0, 1] }, "o": { "a": 0, "k": 100 }, "r": 1 }
                    ]
                }
            ]
        }
    ]
})json";

constexpr const char* kMaskInvertedJson = R"json({
    "v": "5.5.2",
    "nm": "InvertedMaskTest",
    "ip": 0,
    "op": 10,
    "fr": 25.0,
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
            "masksProperties": [
                {
                    "inv": true,
                    "mode": "a",
                    "pt": { "a": 0, "k": { "i": [[0,0],[0,0],[0,0],[0,0]], "o": [[0,0],[0,0],[0,0],[0,0]], "v": [[30,30],[70,30],[70,70],[30,70]], "c": true } },
                    "o": { "a": 0, "k": 100 }
                }
            ],
            "shapes": [
                {
                    "ty": "gr",
                    "nm": "Group",
                    "it": [
                        { "ty": "rc", "nm": "Rect", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [80, 80] }, "r": { "a": 0, "k": 0 } },
                        { "ty": "fl", "nm": "Fill", "c": { "a": 0, "k": [1, 0.5, 0, 1] }, "o": { "a": 0, "k": 100 }, "r": 1 }
                    ]
                }
            ]
        }
    ]
})json";

constexpr const char* kTrimPathsSimultaneousJson = R"json({
    "v": "5.5.2",
    "nm": "TrimSimultaneousTest",
    "ip": 0,
    "op": 10,
    "fr": 25.0,
    "w": 100,
    "h": 100,
    "ddd": 0,
    "assets": [],
    "layers": [
        {
            "ty": 4,
            "nm": "TrimLayer",
            "ind": 1,
            "ip": 0,
            "op": 10,
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
                        { "ty": "rc", "nm": "Rect", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [40, 40] }, "r": { "a": 0, "k": 0 } },
                        { "ty": "st", "nm": "Stroke", "c": { "a": 0, "k": [1, 0, 0, 1] }, "o": { "a": 0, "k": 100 }, "w": { "a": 0, "k": 2 } },
                        { "ty": "tm", "nm": "Trim Paths", "s": { "a": 0, "k": 0 }, "e": { "a": 0, "k": 50 }, "o": { "a": 0, "k": 0 }, "m": 1 }
                    ]
                }
            ]
        }
    ]
})json";

constexpr const char* kTrimPathsIndividuallyJson = R"json({
    "v": "5.5.2",
    "nm": "TrimIndividuallyTest",
    "ip": 0,
    "op": 10,
    "fr": 25.0,
    "w": 100,
    "h": 100,
    "ddd": 0,
    "assets": [],
    "layers": [
        {
            "ty": 4,
            "nm": "TrimLayer",
            "ind": 1,
            "ip": 0,
            "op": 10,
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
                        { "ty": "rc", "nm": "Rect", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [40, 40] }, "r": { "a": 0, "k": 0 } },
                        { "ty": "st", "nm": "Stroke", "c": { "a": 0, "k": [0, 0, 1, 1] }, "o": { "a": 0, "k": 100 }, "w": { "a": 0, "k": 2 } },
                        { "ty": "tm", "nm": "Trim Paths", "s": { "a": 0, "k": 10 }, "e": { "a": 0, "k": 80 }, "o": { "a": 0, "k": 0 }, "m": 2 }
                    ]
                }
            ]
        }
    ]
})json";

constexpr const char* kRepeaterJson = R"json({
    "v": "5.5.2",
    "nm": "RepeaterTest",
    "ip": 0,
    "op": 10,
    "fr": 25.0,
    "w": 200,
    "h": 100,
    "ddd": 0,
    "assets": [],
    "layers": [
        {
            "ty": 4,
            "nm": "RepeaterLayer",
            "ind": 1,
            "ip": 0,
            "op": 10,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [20, 50] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 100 }
            },
            "shapes": [
                {
                    "ty": "gr",
                    "nm": "Group",
                    "it": [
                        { "ty": "rc", "nm": "Rect", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [15, 15] }, "r": { "a": 0, "k": 0 } },
                        { "ty": "fl", "nm": "Fill", "c": { "a": 0, "k": [1, 0, 0, 1] }, "o": { "a": 0, "k": 100 }, "r": 1 },
                        {
                            "ty": "rp",
                            "nm": "Repeater",
                            "c": { "a": 0, "k": 3 },
                            "o": { "a": 0, "k": 0 },
                            "tr": {
                                "p": { "a": 0, "k": [40, 0] },
                                "a": { "a": 0, "k": [0, 0] },
                                "s": { "a": 0, "k": [100, 100] },
                                "r": { "a": 0, "k": 0 },
                                "so": { "a": 0, "k": 100 },
                                "eo": { "a": 0, "k": 100 }
                            }
                        }
                    ]
                }
            ]
        }
    ]
})json";

constexpr const char* kAlphaMatteJson = R"json({
    "v": "5.5.2",
    "nm": "MatteTest",
    "ip": 0,
    "op": 10,
    "fr": 25.0,
    "w": 100,
    "h": 100,
    "ddd": 0,
    "assets": [],
    "layers": [
        {
            "ty": 4,
            "nm": "MatteSource",
            "ind": 1,
            "ip": 0,
            "op": 10,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "td": 1,
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
                        { "ty": "el", "nm": "Ellipse", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [60, 60] } },
                        { "ty": "fl", "nm": "Fill", "c": { "a": 0, "k": [1, 1, 1, 1] }, "o": { "a": 0, "k": 100 }, "r": 1 }
                    ]
                }
            ]
        },
        {
            "ty": 4,
            "nm": "MatteTarget",
            "ind": 2,
            "ip": 0,
            "op": 10,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "tt": 1,
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
                        { "ty": "rc", "nm": "Rect", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [80, 80] }, "r": { "a": 0, "k": 0 } },
                        { "ty": "fl", "nm": "Fill", "c": { "a": 0, "k": [1, 0, 0, 1] }, "o": { "a": 0, "k": 100 }, "r": 1 }
                    ]
                }
            ]
        }
    ]
})json";

constexpr const char* kParentChainJson = R"json({
    "v": "5.5.2",
    "nm": "ParentChainTest",
    "ip": 0,
    "op": 10,
    "fr": 25.0,
    "w": 200,
    "h": 200,
    "ddd": 0,
    "assets": [],
    "layers": [
        {
            "ty": 4,
            "nm": "ChildLayer",
            "ind": 1,
            "parent": 2,
            "ip": 0,
            "op": 10,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [20, 0] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 100 }
            },
            "shapes": [
                {
                    "ty": "gr",
                    "nm": "Group",
                    "it": [
                        { "ty": "rc", "nm": "Rect", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [20, 20] }, "r": { "a": 0, "k": 0 } },
                        { "ty": "fl", "nm": "Fill", "c": { "a": 0, "k": [1, 0, 0, 1] }, "o": { "a": 0, "k": 100 }, "r": 1 }
                    ]
                }
            ]
        },
        {
            "ty": 3,
            "nm": "ParentNull",
            "ind": 2,
            "ip": 0,
            "op": 10,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [50, 50] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 30 },
                "o": { "a": 0, "k": 0 }
            }
        }
    ]
})json";

constexpr const char* kDashStrokeJson = R"json({
    "v": "5.5.2",
    "nm": "DashStrokeTest",
    "ip": 0,
    "op": 10,
    "fr": 25.0,
    "w": 100,
    "h": 100,
    "ddd": 0,
    "assets": [],
    "layers": [
        {
            "ty": 4,
            "nm": "DashLayer",
            "ind": 1,
            "ip": 0,
            "op": 10,
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
                        { "ty": "rc", "nm": "Rect", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [60, 60] }, "r": { "a": 0, "k": 0 } },
                        {
                            "ty": "st",
                            "nm": "Stroke",
                            "c": { "a": 0, "k": [0, 0, 0, 1] },
                            "o": { "a": 0, "k": 100 },
                            "w": { "a": 0, "k": 3 },
                            "d": [
                                { "n": "d", "nm": "dash", "v": { "a": 0, "k": 10 } },
                                { "n": "g", "nm": "gap", "v": { "a": 0, "k": 5 } }
                            ]
                        }
                    ]
                }
            ]
        }
    ]
})json";

constexpr const char* kLayerOutOfRangeJson = R"json({
    "v": "5.5.2",
    "nm": "OutOfRangeTest",
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
            "nm": "LateLayer",
            "ind": 1,
            "ip": 20,
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
                        { "ty": "rc", "nm": "Rect", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [40, 40] }, "r": { "a": 0, "k": 0 } },
                        { "ty": "fl", "nm": "Fill", "c": { "a": 0, "k": [1, 0, 0, 1] }, "o": { "a": 0, "k": 100 }, "r": 1 }
                    ]
                }
            ]
        }
    ]
})json";

constexpr const char* kGradientFillJson = R"json({
    "v": "5.5.2",
    "nm": "GradientFillTest",
    "ip": 0,
    "op": 10,
    "fr": 25.0,
    "w": 100,
    "h": 100,
    "ddd": 0,
    "assets": [],
    "layers": [
        {
            "ty": 4,
            "nm": "GradientLayer",
            "ind": 1,
            "ip": 0,
            "op": 10,
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
                        { "ty": "rc", "nm": "Rect", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [80, 80] }, "r": { "a": 0, "k": 0 } },
                        {
                            "ty": "gf",
                            "nm": "Gradient Fill",
                            "o": { "a": 0, "k": 100 },
                            "r": 1,
                            "s": { "a": 0, "k": [-40, 0] },
                            "e": { "a": 0, "k": [40, 0] },
                            "t": 1,
                            "g": { "p": 2, "k": { "a": 0, "k": [0, 1, 0, 0, 1, 0, 1, 0] } }
                        }
                    ]
                }
            ]
        }
    ]
})json";

constexpr const char* kGradientStrokeJson = R"json({
    "v": "5.5.2",
    "nm": "GradientStrokeTest",
    "ip": 0,
    "op": 10,
    "fr": 25.0,
    "w": 100,
    "h": 100,
    "ddd": 0,
    "assets": [],
    "layers": [
        {
            "ty": 4,
            "nm": "GradientStrokeLayer",
            "ind": 1,
            "ip": 0,
            "op": 10,
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
                        { "ty": "rc", "nm": "Rect", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [60, 60] }, "r": { "a": 0, "k": 0 } },
                        {
                            "ty": "gs",
                            "nm": "Gradient Stroke",
                            "o": { "a": 0, "k": 100 },
                            "w": { "a": 0, "k": 4 },
                            "s": { "a": 0, "k": [-30, 0] },
                            "e": { "a": 0, "k": [30, 0] },
                            "t": 1,
                            "g": { "p": 2, "k": { "a": 0, "k": [0, 0, 0, 1, 1, 1, 0, 0] } }
                        }
                    ]
                }
            ]
        }
    ]
})json";

constexpr const char* kEllipseAndPolystarJson = R"json({
    "v": "5.5.2",
    "nm": "EllipsePolystarTest",
    "ip": 0,
    "op": 10,
    "fr": 25.0,
    "w": 200,
    "h": 100,
    "ddd": 0,
    "assets": [],
    "layers": [
        {
            "ty": 4,
            "nm": "ShapesLayer",
            "ind": 1,
            "ip": 0,
            "op": 10,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [100, 50] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 100 }
            },
            "shapes": [
                {
                    "ty": "gr",
                    "nm": "EllipseGroup",
                    "it": [
                        { "ty": "el", "nm": "Ellipse", "p": { "a": 0, "k": [-50, 0] }, "s": { "a": 0, "k": [40, 30] } },
                        { "ty": "fl", "nm": "Fill", "c": { "a": 0, "k": [0, 0.5, 1, 1] }, "o": { "a": 0, "k": 100 }, "r": 1 }
                    ]
                },
                {
                    "ty": "gr",
                    "nm": "PolystarGroup",
                    "it": [
                        { "ty": "sr", "nm": "Star", "sy": 1, "p": { "a": 0, "k": [50, 0] }, "r": { "a": 0, "k": 0 }, "pt": { "a": 0, "k": 5 }, "ir": { "a": 0, "k": 10 }, "is": { "a": 0, "k": 0 }, "or": { "a": 0, "k": 20 }, "os": { "a": 0, "k": 0 } },
                        { "ty": "fl", "nm": "Fill", "c": { "a": 0, "k": [1, 1, 0, 1] }, "o": { "a": 0, "k": 100 }, "r": 1 }
                    ]
                }
            ]
        }
    ]
})json";

constexpr const char* kFillEffectJson = R"json({
    "v": "5.5.2",
    "nm": "FillEffectTest",
    "ip": 0,
    "op": 10,
    "fr": 25.0,
    "w": 100,
    "h": 100,
    "ddd": 0,
    "assets": [],
    "layers": [
        {
            "ty": 4,
            "nm": "FillEffectLayer",
            "ind": 1,
            "ip": 0,
            "op": 10,
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
            "ef": [
                {
                    "ty": 21,
                    "nm": "Fill",
                    "mn": "ADBE Fill",
                    "en": 1,
                    "ef": [
                        { "ty": 2, "nm": "Color", "mn": "ADBE Fill-0001", "v": { "a": 0, "k": [1, 0, 0, 1] } },
                        { "ty": 0, "nm": "Opacity", "mn": "ADBE Fill-0002", "v": { "a": 0, "k": 100 } }
                    ]
                }
            ],
            "shapes": [
                {
                    "ty": "gr",
                    "nm": "Group",
                    "it": [
                        { "ty": "rc", "nm": "Rect", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [60, 60] }, "r": { "a": 0, "k": 0 } },
                        { "ty": "fl", "nm": "Fill", "c": { "a": 0, "k": [0, 1, 0, 1] }, "o": { "a": 0, "k": 100 }, "r": 1 }
                    ]
                }
            ]
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

TEST_F (AnimationRendererTests, RenderWithScaleToFitAndFillDoesNotCrash)
{
    auto comp = LottieReader::parseData (kShapeLayerJson);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (200, 100);
    Graphics g (*context, *renderer);

    const Rectangle<float> bounds (0, 0, 200, 100);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, bounds, Fitting::scaleToFit);
    });

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, bounds, Fitting::fill);
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

// =============================================================================
// Partial opacity — exercises renderLayerIsolated (transparency layer path)
// =============================================================================

TEST_F (AnimationRendererTests, RenderShapeLayerWithPartialOpacityDoesNotCrash)
{
    auto comp = LottieReader::parseData (kPartialOpacityShapeJson);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 1u);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

// =============================================================================
// Drop shadow effect
// =============================================================================

TEST_F (AnimationRendererTests, RenderShapeLayerWithDropShadowDoesNotCrash)
{
    auto comp = LottieReader::parseData (kDropShadowShapeJson);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

// =============================================================================
// Fill effect
// =============================================================================

TEST_F (AnimationRendererTests, RenderShapeLayerWithFillEffectDoesNotCrash)
{
    auto comp = LottieReader::parseData (kFillEffectJson);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

// =============================================================================
// Masks — add, subtract, intersect, inverted
// =============================================================================

TEST_F (AnimationRendererTests, RenderShapeLayerWithAddMaskDoesNotCrash)
{
    auto comp = LottieReader::parseData (kMaskAddJson);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

TEST_F (AnimationRendererTests, RenderShapeLayerWithSubtractMaskDoesNotCrash)
{
    auto comp = LottieReader::parseData (kMaskSubtractJson);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

TEST_F (AnimationRendererTests, RenderShapeLayerWithIntersectMaskDoesNotCrash)
{
    auto comp = LottieReader::parseData (kMaskIntersectJson);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

TEST_F (AnimationRendererTests, RenderShapeLayerWithInvertedMaskDoesNotCrash)
{
    auto comp = LottieReader::parseData (kMaskInvertedJson);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

// =============================================================================
// Trim paths — simultaneous (m=1) and individual (m=2)
// =============================================================================

TEST_F (AnimationRendererTests, RenderShapeLayerWithTrimPathsSimultaneousDoesNotCrash)
{
    auto comp = LottieReader::parseData (kTrimPathsSimultaneousJson);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

TEST_F (AnimationRendererTests, RenderShapeLayerWithTrimPathsIndividuallyDoesNotCrash)
{
    auto comp = LottieReader::parseData (kTrimPathsIndividuallyJson);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

// =============================================================================
// Repeater
// =============================================================================

TEST_F (AnimationRendererTests, RenderShapeLayerWithRepeaterDoesNotCrash)
{
    auto comp = LottieReader::parseData (kRepeaterJson);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (200, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 200, 100));
    });
}

// =============================================================================
// Matte — alpha matte pair
// =============================================================================

TEST_F (AnimationRendererTests, RenderLayerWithAlphaMatteDoesNotCrash)
{
    auto comp = LottieReader::parseData (kAlphaMatteJson);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 2u);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

// =============================================================================
// Parent chain — child offset by rotated parent null
// =============================================================================

TEST_F (AnimationRendererTests, RenderLayerWithParentChainDoesNotCrash)
{
    auto comp = LottieReader::parseData (kParentChainJson);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 2u);

    auto renderer = context->makeRenderer (200, 200);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 200, 200));
    });
}

// =============================================================================
// Dash stroke
// =============================================================================

TEST_F (AnimationRendererTests, RenderShapeLayerWithDashStrokeDoesNotCrash)
{
    auto comp = LottieReader::parseData (kDashStrokeJson);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

// =============================================================================
// Layer in/out range — layer not visible at frame 0 should be skipped
// =============================================================================

TEST_F (AnimationRendererTests, RenderLayerNotYetVisibleAtFrameZeroDoesNotCrash)
{
    auto comp = LottieReader::parseData (kLayerOutOfRangeJson);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 1u);
    EXPECT_FLOAT_EQ (comp->layers[0]->inFrame, 20.0f);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 25.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

// =============================================================================
// Gradient fill and gradient stroke rendering
// =============================================================================

TEST_F (AnimationRendererTests, RenderShapeLayerWithGradientFillDoesNotCrash)
{
    auto comp = LottieReader::parseData (kGradientFillJson);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

TEST_F (AnimationRendererTests, RenderShapeLayerWithGradientStrokeDoesNotCrash)
{
    auto comp = LottieReader::parseData (kGradientStrokeJson);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

// =============================================================================
// Ellipse and polystar shapes
// =============================================================================

TEST_F (AnimationRendererTests, RenderShapeLayerWithEllipseAndPolystarDoesNotCrash)
{
    auto comp = LottieReader::parseData (kEllipseAndPolystarJson);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (200, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 200, 100));
    });
}
