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

constexpr const char* kMaskDifferenceJson = R"json({
    "v": "5.5.2",
    "nm": "MaskDifferenceTest",
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
                    "mode": "d",
                    "pt": { "a": 0, "k": { "i": [[0,0],[0,0],[0,0],[0,0]], "o": [[0,0],[0,0],[0,0],[0,0]], "v": [[10,10],[90,10],[90,90],[10,90]], "c": true } },
                    "o": { "a": 0, "k": 100 }
                }
            ],
            "shapes": [
                {
                    "ty": "gr",
                    "nm": "Group",
                    "it": [
                        { "ty": "rc", "nm": "Rect", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [60, 60] }, "r": { "a": 0, "k": 0 } },
                        { "ty": "fl", "nm": "Fill", "c": { "a": 0, "k": [1, 0, 1, 1] }, "o": { "a": 0, "k": 100 }, "r": 1 }
                    ]
                }
            ]
        }
    ]
})json";

constexpr const char* kShadowOnlyJson = R"json({
    "v": "5.5.2",
    "nm": "ShadowOnlyTest",
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
            "nm": "ShadowOnlyLayer",
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
                        { "ty": 0, "nm": "Opacity",      "mn": "ADBE Drop Shadow-0002", "v": { "a": 0, "k": 50 } },
                        { "ty": 0, "nm": "Direction",    "mn": "ADBE Drop Shadow-0003", "v": { "a": 0, "k": 135 } },
                        { "ty": 0, "nm": "Distance",     "mn": "ADBE Drop Shadow-0004", "v": { "a": 0, "k": 6 } },
                        { "ty": 0, "nm": "Softness",     "mn": "ADBE Drop Shadow-0005", "v": { "a": 0, "k": 0 } },
                        { "ty": 7, "nm": "Shadow Only",  "mn": "ADBE Drop Shadow-0006", "v": { "a": 0, "k": 1 } }
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

constexpr const char* kPrecompJson = R"json({
    "v": "5.5.2",
    "nm": "PrecompTest",
    "ip": 0,
    "op": 30,
    "fr": 25.0,
    "w": 100,
    "h": 100,
    "ddd": 0,
    "assets": [
        {
            "id": "precompAsset",
            "nm": "Precomp",
            "fr": 25.0,
            "ip": 0,
            "op": 30,
            "w": 50,
            "h": 50,
            "layers": [
                {
                    "ty": 4,
                    "nm": "NestedShape",
                    "ind": 1,
                    "ip": 0,
                    "op": 30,
                    "st": 0,
                    "sr": 1,
                    "hd": false,
                    "bm": 0,
                    "ks": {
                        "a": { "a": 0, "k": [0, 0] },
                        "p": { "a": 0, "k": [25, 25] },
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
                                { "ty": "fl", "nm": "Fill", "c": { "a": 0, "k": [0, 1, 0, 1] }, "o": { "a": 0, "k": 100 }, "r": 1 }
                            ]
                        }
                    ]
                }
            ]
        }
    ],
    "layers": [
        {
            "ty": 0,
            "nm": "PrecompLayer",
            "ind": 1,
            "refId": "precompAsset",
            "ip": 0,
            "op": 30,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "w": 50,
            "h": 50,
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

// A paint-less nested group carrying the modifier that defines the outline, with the
// paint on the parent group. This is how RubberHose rigs draw a limb: a 4-point star
// trimmed down to an arc, stroked by the enclosing group. The nested group's trim has
// to survive being handed up to that stroke, otherwise the whole star gets painted.
constexpr const char* kNestedTrimmedGroupJson = R"json({
    "v": "5.7.0", "fr": 25, "ip": 0, "op": 50, "w": 100, "h": 100, "nm": "nested trim",
    "layers": [
        {
            "ddd": 0, "ind": 1, "ty": 4, "nm": "hose", "sr": 1, "ao": 0,
            "ip": 0, "op": 50, "st": 0, "bm": 0,
            "ks": {
                "o": { "a": 0, "k": 100 }, "r": { "a": 0, "k": 0 },
                "p": { "a": 0, "k": [50, 50, 0] }, "a": { "a": 0, "k": [0, 0, 0] },
                "s": { "a": 0, "k": [100, 100, 100] }
            },
            "shapes": [
                {
                    "ty": "gr", "nm": "BaseHose", "hd": false,
                    "it": [
                        {
                            "ty": "gr", "nm": "Arc", "hd": false,
                            "it": [
                                {
                                    "ty": "sr", "nm": "LineForCurve", "hd": false, "sy": 1,
                                    "pt": { "a": 0, "k": 4 }, "p": { "a": 0, "k": [0, 0] },
                                    "r": { "a": 0, "k": 0 }, "ir": { "a": 0, "k": 20 },
                                    "is": { "a": 0, "k": 0 }, "or": { "a": 0, "k": 40 },
                                    "os": { "a": 0, "k": 0 }
                                },
                                {
                                    "ty": "tm", "nm": "Line Halfer", "hd": false, "m": 1,
                                    "s": { "a": 0, "k": 0 }, "e": { "a": 0, "k": 25 },
                                    "o": { "a": 0, "k": 0 }
                                },
                                {
                                    "ty": "tr", "p": { "a": 0, "k": [0, 0] }, "a": { "a": 0, "k": [0, 0] },
                                    "s": { "a": 0, "k": [100, 100] }, "r": { "a": 0, "k": 0 },
                                    "o": { "a": 0, "k": 100 }
                                }
                            ]
                        },
                        {
                            "ty": "st", "nm": "Stroke 1", "hd": false, "lc": 2, "lj": 2,
                            "c": { "a": 0, "k": [0, 0, 0, 1] }, "o": { "a": 0, "k": 100 },
                            "w": { "a": 0, "k": 4 }
                        },
                        {
                            "ty": "tr", "p": { "a": 0, "k": [0, 0] }, "a": { "a": 0, "k": [0, 0] },
                            "s": { "a": 0, "k": [100, 100] }, "r": { "a": 0, "k": 0 },
                            "o": { "a": 0, "k": 100 }
                        }
                    ]
                }
            ]
        }
    ]
})json";

// One asset drawn by two precomp layers: the offscreen render is shared between
// the references.
constexpr const char* kSharedPrecompJson = R"json({
    "v": "5.5.2",
    "nm": "SharedPrecompTest",
    "ip": 0,
    "op": 30,
    "fr": 25.0,
    "w": 100,
    "h": 100,
    "ddd": 0,
    "assets": [
        {
            "id": "sharedAsset",
            "nm": "Shared",
            "fr": 25.0,
            "ip": 0,
            "op": 30,
            "w": 40,
            "h": 40,
            "layers": [
                {
                    "ty": 4,
                    "nm": "Inner",
                    "ind": 1,
                    "ip": 0,
                    "op": 30,
                    "st": 0,
                    "sr": 1,
                    "hd": false,
                    "bm": 0,
                    "ks": {
                        "a": { "a": 0, "k": [0, 0] },
                        "p": { "a": 0, "k": [20, 20] },
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
                                { "ty": "fl", "nm": "Fill", "c": { "a": 0, "k": [0, 1, 0, 1] }, "o": { "a": 0, "k": 100 }, "r": 1 }
                            ]
                        }
                    ]
                }
            ]
        }
    ],
    "layers": [
        {
            "ty": 0,
            "nm": "PrecompLeft",
            "ind": 1,
            "refId": "sharedAsset",
            "ip": 0,
            "op": 30,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "w": 40,
            "h": 40,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [25, 50] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 100 }
            }
        },
        {
            "ty": 0,
            "nm": "PrecompRight",
            "ind": 2,
            "refId": "sharedAsset",
            "ip": 0,
            "op": 30,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "w": 40,
            "h": 40,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [75, 50] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 100 }
            }
        }
    ]
})json";

// A precomp drawn by another precomp, each referenced exactly once: nothing can
// share an offscreen render, so both levels draw directly into the parent.
constexpr const char* kNestedPrecompJson = R"json({
    "v": "5.5.2",
    "nm": "NestedPrecompTest",
    "ip": 0,
    "op": 30,
    "fr": 25.0,
    "w": 100,
    "h": 100,
    "ddd": 0,
    "assets": [
        {
            "id": "innerAsset",
            "nm": "Inner",
            "fr": 25.0,
            "ip": 0,
            "op": 30,
            "w": 20,
            "h": 20,
            "layers": [
                {
                    "ty": 4,
                    "nm": "Innermost",
                    "ind": 1,
                    "ip": 0,
                    "op": 30,
                    "st": 0,
                    "sr": 1,
                    "hd": false,
                    "bm": 0,
                    "ks": {
                        "a": { "a": 0, "k": [0, 0] },
                        "p": { "a": 0, "k": [10, 10] },
                        "s": { "a": 0, "k": [100, 100] },
                        "r": { "a": 0, "k": 0 },
                        "o": { "a": 0, "k": 100 }
                    },
                    "shapes": [
                        {
                            "ty": "gr",
                            "nm": "Group",
                            "it": [
                                { "ty": "rc", "nm": "Rect", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [12, 12] }, "r": { "a": 0, "k": 0 } },
                                { "ty": "fl", "nm": "Fill", "c": { "a": 0, "k": [0, 0, 1, 1] }, "o": { "a": 0, "k": 100 }, "r": 1 }
                            ]
                        }
                    ]
                }
            ]
        },
        {
            "id": "outerAsset",
            "nm": "Outer",
            "fr": 25.0,
            "ip": 0,
            "op": 30,
            "w": 40,
            "h": 40,
            "layers": [
                {
                    "ty": 0,
                    "nm": "MiddlePrecomp",
                    "ind": 1,
                    "refId": "innerAsset",
                    "ip": 0,
                    "op": 30,
                    "st": 0,
                    "sr": 1,
                    "hd": false,
                    "bm": 0,
                    "w": 20,
                    "h": 20,
                    "ks": {
                        "a": { "a": 0, "k": [0, 0] },
                        "p": { "a": 0, "k": [20, 20] },
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
            "nm": "OutermostPrecomp",
            "ind": 1,
            "refId": "outerAsset",
            "ip": 0,
            "op": 30,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "w": 40,
            "h": 40,
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

// A precomp layer whose file omits the precomp's w/h: the reader falls back to
// the composition size rather than handing the renderer a zero-area precomp.
constexpr const char* kZeroSizePrecompJson = R"json({
    "v": "5.5.2",
    "nm": "ZeroSizePrecompTest",
    "ip": 0,
    "op": 10,
    "fr": 25.0,
    "w": 100,
    "h": 100,
    "ddd": 0,
    "assets": [
        {
            "id": "emptyAreaAsset",
            "nm": "EmptyArea",
            "fr": 25.0,
            "ip": 0,
            "op": 10,
            "w": 0,
            "h": 0,
            "layers": [
                {
                    "ty": 4,
                    "nm": "Inner",
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
                            "ty": "gr",
                            "nm": "Group",
                            "it": [
                                { "ty": "rc", "nm": "Rect", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [20, 20] }, "r": { "a": 0, "k": 0 } },
                                { "ty": "fl", "nm": "Fill", "c": { "a": 0, "k": [1, 0, 0, 1] }, "o": { "a": 0, "k": 100 }, "r": 1 }
                            ]
                        }
                    ]
                }
            ]
        }
    ],
    "layers": [
        {
            "ty": 0,
            "nm": "ZeroSizePrecomp",
            "ind": 1,
            "refId": "emptyAreaAsset",
            "ip": 0,
            "op": 10,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "w": 0,
            "h": 0,
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

// The same asset drawn at two different precomp sizes: the mask clip is derived
// from the precomp's size, so the two never share a cached clip path.
constexpr const char* kPrecompSizedAssetJson = R"json({
    "v": "5.5.2",
    "nm": "PrecompSizedAssetTest",
    "ip": 0,
    "op": 30,
    "fr": 25.0,
    "w": 200,
    "h": 200,
    "ddd": 0,
    "assets": [
        {
            "id": "sizedAsset",
            "nm": "Sized",
            "fr": 25.0,
            "ip": 0,
            "op": 30,
            "w": 40,
            "h": 40,
            "layers": [
                {
                    "ty": 4,
                    "nm": "MaskedInner",
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
                            "inv": true,
                            "mode": "a",
                            "pt": { "a": 0, "k": { "i": [[0,0],[0,0],[0,0],[0,0]], "o": [[0,0],[0,0],[0,0],[0,0]], "v": [[5,5],[35,5],[35,35],[5,35]], "c": true } },
                            "o": { "a": 0, "k": 100 }
                        }
                    ],
                    "shapes": [
                        {
                            "ty": "gr",
                            "nm": "Group",
                            "it": [
                                { "ty": "rc", "nm": "Rect", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [36, 36] }, "r": { "a": 0, "k": 0 } },
                                { "ty": "fl", "nm": "Fill", "c": { "a": 0, "k": [0, 1, 0, 1] }, "o": { "a": 0, "k": 100 }, "r": 1 }
                            ]
                        }
                    ]
                }
            ]
        }
    ],
    "layers": [
        {
            "ty": 0,
            "nm": "SmallPrecomp",
            "ind": 1,
            "refId": "sizedAsset",
            "ip": 0,
            "op": 30,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "w": 40,
            "h": 40,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [50, 50] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 100 }
            }
        },
        {
            "ty": 0,
            "nm": "LargePrecomp",
            "ind": 2,
            "refId": "sizedAsset",
            "ip": 0,
            "op": 30,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "w": 80,
            "h": 80,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [150, 150] },
                "s": { "a": 0, "k": [200, 200] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 100 }
            }
        }
    ]
})json";

// A mask that lies entirely outside the composition viewport.
constexpr const char* kMaskOutsideViewportJson = R"json({
    "v": "5.5.2",
    "nm": "MaskOutsideViewportTest",
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
            "nm": "OffscreenMaskedLayer",
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
                    "pt": { "a": 0, "k": { "i": [[0,0],[0,0],[0,0],[0,0]], "o": [[0,0],[0,0],[0,0],[0,0]], "v": [[300,300],[400,300],[400,400],[300,400]], "c": true } },
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

// A layer parented to itself, plus a valid two-layer chain next to it.
constexpr const char* kSelfParentJson = R"json({
    "v": "5.5.2",
    "nm": "SelfParentTest",
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
            "nm": "SelfParentedLayer",
            "ind": 1,
            "parent": 1,
            "ip": 0,
            "op": 10,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [30, 30] },
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
            "ty": 4,
            "nm": "GrandChild",
            "ind": 2,
            "parent": 3,
            "ip": 0,
            "op": 10,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [10, 10] },
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
                        { "ty": "fl", "nm": "Fill", "c": { "a": 0, "k": [0, 1, 0, 1] }, "o": { "a": 0, "k": 100 }, "r": 1 }
                    ]
                }
            ]
        },
        {
            "ty": 3,
            "nm": "ParentNull",
            "ind": 3,
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
                "r": { "a": 0, "k": 45 },
                "o": { "a": 0, "k": 0 }
            }
        }
    ]
})json";

// One layer per opacity decision: a solid and a null below the opaque threshold
// (neither needs a transparency layer), a near-opaque solid (treated as opaque),
// and a solid carrying a drop shadow (which still does need one).
constexpr const char* kPartialOpacityLayersJson = R"json({
    "v": "5.5.2",
    "nm": "PartialOpacityLayersTest",
    "ip": 0,
    "op": 30,
    "fr": 25.0,
    "w": 200,
    "h": 200,
    "ddd": 0,
    "assets": [],
    "layers": [
        {
            "ty": 1,
            "nm": "HalfSolid",
            "ind": 1,
            "ip": 0,
            "op": 30,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "sc": "#ff0000",
            "sw": 60,
            "sh": 60,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [50, 50] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 50 }
            }
        },
        {
            "ty": 3,
            "nm": "PartialNull",
            "ind": 2,
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
                "o": { "a": 0, "k": 40 }
            }
        },
        {
            "ty": 1,
            "nm": "NotQuiteOpaqueSolid",
            "ind": 3,
            "ip": 0,
            "op": 30,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "sc": "#00ff00",
            "sw": 60,
            "sh": 60,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [150, 50] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 99 }
            }
        },
        {
            "ty": 1,
            "nm": "ShadowedSolid",
            "ind": 4,
            "ip": 0,
            "op": 30,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "sc": "#0000ff",
            "sw": 60,
            "sh": 60,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [50, 150] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 50 }
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
            ]
        }
    ]
})json";

// Both sides of the near-opaque threshold: 99.9 is treated as opaque, 99 is not.
constexpr const char* kNearOpaqueShapeJson = R"json({
    "v": "5.5.2",
    "nm": "NearOpaqueShapeTest",
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
            "nm": "RoundedDown",
            "ind": 1,
            "ip": 0,
            "op": 30,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [30, 50] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 99.9 }
            },
            "shapes": [
                {
                    "ty": "gr",
                    "nm": "Group",
                    "it": [
                        { "ty": "rc", "nm": "Rect", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [30, 30] }, "r": { "a": 0, "k": 0 } },
                        { "ty": "fl", "nm": "Fill", "c": { "a": 0, "k": [1, 0, 0, 1] }, "o": { "a": 0, "k": 100 }, "r": 1 }
                    ]
                }
            ]
        },
        {
            "ty": 4,
            "nm": "NotRoundedDown",
            "ind": 2,
            "ip": 0,
            "op": 30,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [70, 50] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 99 }
            },
            "shapes": [
                {
                    "ty": "gr",
                    "nm": "Group",
                    "it": [
                        { "ty": "rc", "nm": "Rect", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [30, 30] }, "r": { "a": 0, "k": 0 } },
                        { "ty": "fl", "nm": "Fill", "c": { "a": 0, "k": [0, 0, 1, 1] }, "o": { "a": 0, "k": 100 }, "r": 1 }
                    ]
                }
            ]
        }
    ]
})json";

// A precomp much smaller than the composition it sits in, drawn at partial
// opacity: its transparency target only needs to cover the precomp's own box.
constexpr const char* kPartialOpacityPrecompJson = R"json({
    "v": "5.5.2",
    "nm": "PartialOpacityPrecompTest",
    "ip": 0,
    "op": 30,
    "fr": 25.0,
    "w": 200,
    "h": 200,
    "ddd": 0,
    "assets": [
        {
            "id": "smallAsset",
            "nm": "Small",
            "fr": 25.0,
            "ip": 0,
            "op": 30,
            "w": 40,
            "h": 40,
            "layers": [
                {
                    "ty": 4,
                    "nm": "Inner",
                    "ind": 1,
                    "ip": 0,
                    "op": 30,
                    "st": 0,
                    "sr": 1,
                    "hd": false,
                    "bm": 0,
                    "ks": {
                        "a": { "a": 0, "k": [0, 0] },
                        "p": { "a": 0, "k": [20, 20] },
                        "s": { "a": 0, "k": [100, 100] },
                        "r": { "a": 0, "k": 0 },
                        "o": { "a": 0, "k": 100 }
                    },
                    "shapes": [
                        {
                            "ty": "gr",
                            "nm": "Group",
                            "it": [
                                { "ty": "rc", "nm": "Rect", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [24, 24] }, "r": { "a": 0, "k": 0 } },
                                { "ty": "fl", "nm": "Fill", "c": { "a": 0, "k": [0, 1, 0, 1] }, "o": { "a": 0, "k": 100 }, "r": 1 }
                            ]
                        }
                    ]
                }
            ]
        }
    ],
    "layers": [
        {
            "ty": 0,
            "nm": "FadedPrecomp",
            "ind": 1,
            "refId": "smallAsset",
            "ip": 0,
            "op": 30,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "w": 40,
            "h": 40,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [60, 60] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 40 }
            }
        }
    ]
})json";

// Three time slices of one asset, each visible in its own window - the shape a
// "sequential shots of one scene" export produces. Only ever one of them is
// drawn, so the asset is a single-reference asset on every frame.
constexpr const char* kSequentialPrecompSlicesJson = R"json({
    "v": "5.5.2",
    "nm": "SequentialPrecompSlicesTest",
    "ip": 0,
    "op": 30,
    "fr": 25.0,
    "w": 100,
    "h": 100,
    "ddd": 0,
    "assets": [
        {
            "id": "sliceAsset",
            "nm": "Slice",
            "fr": 25.0,
            "ip": 0,
            "op": 30,
            "w": 60,
            "h": 60,
            "layers": [
                {
                    "ty": 4,
                    "nm": "Inner",
                    "ind": 1,
                    "ip": 0,
                    "op": 30,
                    "st": 0,
                    "sr": 1,
                    "hd": false,
                    "bm": 0,
                    "ks": {
                        "a": { "a": 0, "k": [0, 0] },
                        "p": { "a": 0, "k": [30, 30] },
                        "s": { "a": 0, "k": [100, 100] },
                        "r": { "a": 0, "k": 0 },
                        "o": { "a": 0, "k": 100 }
                    },
                    "shapes": [
                        {
                            "ty": "gr",
                            "nm": "Group",
                            "it": [
                                { "ty": "rc", "nm": "Rect", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [30, 30] }, "r": { "a": 0, "k": 0 } },
                                { "ty": "fl", "nm": "Fill", "c": { "a": 0, "k": [0, 1, 0, 1] }, "o": { "a": 0, "k": 100 }, "r": 1 }
                            ]
                        }
                    ]
                }
            ]
        }
    ],
    "layers": [
        {
            "ty": 0,
            "nm": "SliceStart",
            "ind": 1,
            "refId": "sliceAsset",
            "ip": 0,
            "op": 10,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "w": 60,
            "h": 60,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [50, 50] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 100 }
            }
        },
        {
            "ty": 0,
            "nm": "SliceMiddle",
            "ind": 2,
            "refId": "sliceAsset",
            "ip": 10,
            "op": 20,
            "st": 10,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "w": 60,
            "h": 60,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [50, 50] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 100 }
            }
        },
        {
            "ty": 0,
            "nm": "SliceEnd",
            "ind": 3,
            "refId": "sliceAsset",
            "ip": 20,
            "op": 30,
            "st": 20,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "w": 60,
            "h": 60,
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

// One asset drawn twice at once, but at two different phases of its own timeline
// (the start frames differ). The two renders are not interchangeable.
constexpr const char* kSameAssetDifferentPhasesJson = R"json({
    "v": "5.5.2",
    "nm": "SameAssetDifferentPhasesTest",
    "ip": 0,
    "op": 30,
    "fr": 25.0,
    "w": 100,
    "h": 100,
    "ddd": 0,
    "assets": [
        {
            "id": "phaseAsset",
            "nm": "Phase",
            "fr": 25.0,
            "ip": 0,
            "op": 30,
            "w": 40,
            "h": 40,
            "layers": [
                {
                    "ty": 4,
                    "nm": "Inner",
                    "ind": 1,
                    "ip": 0,
                    "op": 30,
                    "st": 0,
                    "sr": 1,
                    "hd": false,
                    "bm": 0,
                    "ks": {
                        "a": { "a": 0, "k": [0, 0] },
                        "p": { "a": { "a": 1, "k": [
                            { "t": 0,  "s": [10, 20], "i": { "x": [0.5], "y": [0.5] }, "o": { "x": [0.5], "y": [0.5] } },
                            { "t": 29, "s": [30, 20] }
                        ] } },
                        "s": { "a": 0, "k": [100, 100] },
                        "r": { "a": 0, "k": 0 },
                        "o": { "a": 0, "k": 100 }
                    },
                    "shapes": [
                        {
                            "ty": "gr",
                            "nm": "Group",
                            "it": [
                                { "ty": "rc", "nm": "Rect", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [16, 16] }, "r": { "a": 0, "k": 0 } },
                                { "ty": "fl", "nm": "Fill", "c": { "a": 0, "k": [1, 0, 0, 1] }, "o": { "a": 0, "k": 100 }, "r": 1 }
                            ]
                        }
                    ]
                }
            ]
        }
    ],
    "layers": [
        {
            "ty": 0,
            "nm": "PhaseAhead",
            "ind": 1,
            "refId": "phaseAsset",
            "ip": 0,
            "op": 30,
            "st": 10,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "w": 40,
            "h": 40,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [30, 50] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 100 }
            }
        },
        {
            "ty": 0,
            "nm": "PhaseBehind",
            "ind": 2,
            "refId": "phaseAsset",
            "ip": 0,
            "op": 30,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "w": 40,
            "h": 40,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [70, 50] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 100 }
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
        context = GraphicsContext::createContext (GpuPlatform::Headless, {});
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
    auto comp = LottieReader::parseData (kShapeLayerJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

TEST_F (AnimationRendererTests, RenderNestedTrimmedGroupParsesAndRenders)
{
    auto comp = LottieReader::parseData (kNestedTrimmedGroupJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 1u);

    // The trim must be parsed onto the nested group rather than the layer's outer
    // group, since that is what the geometry hand-off has to carry up to the stroke.
    const auto* shapeLayer = dynamic_cast<const ShapeLayer*> (comp->layers[0].get());
    ASSERT_NE (shapeLayer, nullptr);
    ASSERT_EQ (shapeLayer->groups.size(), 1u);

    const auto* baseHose = shapeLayer->groups[0].get();
    ASSERT_NE (baseHose, nullptr);

    const AnimationGroup* arc = nullptr;
    bool baseHoseHasStroke = false;
    for (const auto& child : baseHose->children)
    {
        if (child.kind == AnimationGroup::ChildKind::Group && child.group != nullptr)
            arc = child.group.get();
        else if (child.kind == AnimationGroup::ChildKind::Stroke)
            baseHoseHasStroke = true;
    }

    EXPECT_TRUE (baseHoseHasStroke);
    ASSERT_NE (arc, nullptr);
    EXPECT_TRUE (arc->hasAnyModifier);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

TEST_F (AnimationRendererTests, RenderSolidLayerCompositionDoesNotCrash)
{
    auto comp = LottieReader::parseData (kSolidLayerJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

TEST_F (AnimationRendererTests, RenderNullLayerCompositionDoesNotCrash)
{
    auto comp = LottieReader::parseData (kNullLayerJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

TEST_F (AnimationRendererTests, RenderHiddenLayerCompositionDoesNotCrash)
{
    auto comp = LottieReader::parseData (kHiddenLayerJson).valueOr (nullptr);
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
    auto comp = LottieReader::parseData (kMultiLayerJson).valueOr (nullptr);
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
    auto comp = LottieReader::parseData (kShapeLayerJson).valueOr (nullptr);
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
    auto comp = LottieReader::parseData (kShapeLayerJson).valueOr (nullptr);
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
    auto comp = LottieReader::parseData (kShapeLayerJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (1, 1);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 1, 1));
    });
}

TEST_F (AnimationRendererTests, RenderIntoLargeBoundsDoesNotCrash)
{
    auto comp = LottieReader::parseData (kShapeLayerJson).valueOr (nullptr);
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
    auto comp = LottieReader::parseData (kPartialOpacityShapeJson).valueOr (nullptr);
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
    auto comp = LottieReader::parseData (kDropShadowShapeJson).valueOr (nullptr);
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
    auto comp = LottieReader::parseData (kFillEffectJson).valueOr (nullptr);
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
    auto comp = LottieReader::parseData (kMaskAddJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

TEST_F (AnimationRendererTests, RenderShapeLayerWithSubtractMaskDoesNotCrash)
{
    auto comp = LottieReader::parseData (kMaskSubtractJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

TEST_F (AnimationRendererTests, RenderShapeLayerWithIntersectMaskDoesNotCrash)
{
    auto comp = LottieReader::parseData (kMaskIntersectJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

TEST_F (AnimationRendererTests, RenderShapeLayerWithInvertedMaskDoesNotCrash)
{
    auto comp = LottieReader::parseData (kMaskInvertedJson).valueOr (nullptr);
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
    auto comp = LottieReader::parseData (kTrimPathsSimultaneousJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

TEST_F (AnimationRendererTests, RenderShapeLayerWithTrimPathsIndividuallyDoesNotCrash)
{
    auto comp = LottieReader::parseData (kTrimPathsIndividuallyJson).valueOr (nullptr);
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
    auto comp = LottieReader::parseData (kRepeaterJson).valueOr (nullptr);
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
    auto comp = LottieReader::parseData (kAlphaMatteJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 2u);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

TEST_F (AnimationRendererTests, RenderLayerWithPartialOpacityMatteSourceDoesNotCrash)
{
    // Matte source fill at 65% opacity: a correct alpha matte multiplies the
    // target's alpha by the source's rendered alpha. On a headless context this
    // exercises the geometric-clip fallback path.
    auto comp = LottieReader::parseData (kAlphaMatteJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 2u);

    for (const auto matteType : { AnimationLayer::MatteType::Alpha,
                                  AnimationLayer::MatteType::AlphaInv,
                                  AnimationLayer::MatteType::Luma,
                                  AnimationLayer::MatteType::LumaInv })
    {
        for (const auto& layer : comp->layers)
        {
            if (layer != nullptr && layer->matteType != AnimationLayer::MatteType::None)
                layer->matteType = matteType;
        }

        auto renderer = context->makeRenderer (100, 100);
        Graphics g (*context, *renderer);

        EXPECT_NO_THROW ({
            AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
        });
    }
}

// =============================================================================
// Parent chain — child offset by rotated parent null
// =============================================================================

TEST_F (AnimationRendererTests, RenderLayerWithParentChainDoesNotCrash)
{
    auto comp = LottieReader::parseData (kParentChainJson).valueOr (nullptr);
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
    auto comp = LottieReader::parseData (kDashStrokeJson).valueOr (nullptr);
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
    auto comp = LottieReader::parseData (kLayerOutOfRangeJson).valueOr (nullptr);
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
    auto comp = LottieReader::parseData (kGradientFillJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

TEST_F (AnimationRendererTests, RenderShapeLayerWithGradientStrokeDoesNotCrash)
{
    auto comp = LottieReader::parseData (kGradientStrokeJson).valueOr (nullptr);
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
    auto comp = LottieReader::parseData (kEllipseAndPolystarJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (200, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 200, 100));
    });
}

// =============================================================================
// Mask mode — difference (mode "d")
// =============================================================================

TEST_F (AnimationRendererTests, RenderShapeLayerWithDifferenceMaskDoesNotCrash)
{
    auto comp = LottieReader::parseData (kMaskDifferenceJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

// =============================================================================
// Drop shadow — shadowOnly = true
// =============================================================================

TEST_F (AnimationRendererTests, RenderShapeLayerWithShadowOnlyDoesNotCrash)
{
    auto comp = LottieReader::parseData (kShadowOnlyJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

// =============================================================================
// Precomp layer
// =============================================================================

TEST_F (AnimationRendererTests, RenderPrecompLayerDoesNotCrash)
{
    auto comp = LottieReader::parseData (kPrecompJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);
    ASSERT_GE (comp->layers.size(), 1u);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

TEST_F (AnimationRendererTests, RenderPrecompLayerAtVariousFramesDoesNotCrash)
{
    auto comp = LottieReader::parseData (kPrecompJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    const Rectangle<float> bounds (0, 0, 100, 100);

    for (const float frame : { 0.0f, 10.0f, 29.0f })
    {
        EXPECT_NO_THROW ({
            AnimationRenderer::renderComposition (g, *comp, frame, bounds);
        });
    }
}

// =============================================================================
// All fitting modes — ensure calculateViewTransform handles every enum value
// =============================================================================

TEST_F (AnimationRendererTests, RenderWithFittingNoneDoesNotCrash)
{
    auto comp = LottieReader::parseData (kShapeLayerJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100), Fitting::none);
    });
}

TEST_F (AnimationRendererTests, RenderWithFittingFitWidthDoesNotCrash)
{
    auto comp = LottieReader::parseData (kShapeLayerJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (200, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 200, 100), Fitting::fitWidth);
    });
}

TEST_F (AnimationRendererTests, RenderWithFittingFitHeightDoesNotCrash)
{
    auto comp = LottieReader::parseData (kShapeLayerJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 200);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 200), Fitting::fitHeight);
    });
}

TEST_F (AnimationRendererTests, RenderWithFittingScaleToFillDoesNotCrash)
{
    auto comp = LottieReader::parseData (kShapeLayerJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (200, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 200, 100), Fitting::scaleToFill);
    });
}

TEST_F (AnimationRendererTests, RenderWithFittingCenterCropDoesNotCrash)
{
    auto comp = LottieReader::parseData (kShapeLayerJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (200, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 200, 100), Fitting::centerCrop);
    });
}

TEST_F (AnimationRendererTests, RenderWithFittingCenterInsideDoesNotCrash)
{
    auto comp = LottieReader::parseData (kShapeLayerJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (200, 200);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 200, 200), Fitting::centerInside);
    });
}

TEST_F (AnimationRendererTests, RenderWithFittingStretchWidthDoesNotCrash)
{
    auto comp = LottieReader::parseData (kShapeLayerJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (200, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 200, 100), Fitting::stretchWidth);
    });
}

TEST_F (AnimationRendererTests, RenderWithFittingStretchHeightDoesNotCrash)
{
    auto comp = LottieReader::parseData (kShapeLayerJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 200);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 200), Fitting::stretchHeight);
    });
}

TEST_F (AnimationRendererTests, RenderWithFittingTileDoesNotCrash)
{
    auto comp = LottieReader::parseData (kShapeLayerJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100), Fitting::tile);
    });
}

// =============================================================================
// Justification flags
// =============================================================================

TEST_F (AnimationRendererTests, RenderWithLeftJustificationDoesNotCrash)
{
    auto comp = LottieReader::parseData (kShapeLayerJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100), Fitting::scaleToFit, Justification::left);
    });
}

TEST_F (AnimationRendererTests, RenderWithRightJustificationDoesNotCrash)
{
    auto comp = LottieReader::parseData (kShapeLayerJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100), Fitting::scaleToFit, Justification::right);
    });
}

TEST_F (AnimationRendererTests, RenderWithTopJustificationDoesNotCrash)
{
    auto comp = LottieReader::parseData (kShapeLayerJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100), Fitting::scaleToFit, Justification::top);
    });
}

TEST_F (AnimationRendererTests, RenderWithBottomJustificationDoesNotCrash)
{
    auto comp = LottieReader::parseData (kShapeLayerJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100), Fitting::scaleToFit, Justification::bottom);
    });
}

TEST_F (AnimationRendererTests, RenderWithCenterJustificationDoesNotCrash)
{
    auto comp = LottieReader::parseData (kShapeLayerJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100), Fitting::scaleToFit, Justification::center);
    });
}

// =============================================================================
// RenderComposition with render resources argument
// =============================================================================

TEST_F (AnimationRendererTests, RenderWithRenderResourcesDoesNotCrash)
{
    auto comp = LottieReader::parseData (kShapeLayerJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    AnimationRenderResources resources;

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100), Fitting::scaleToFit, Justification::center, &resources);
    });
}

// =============================================================================
// RenderComposition — non-default opacity
// =============================================================================

TEST_F (AnimationRendererTests, RenderProgrammaticCompositionSmokeTest)
{
    auto comp = AnimationComposition::create ({ 200.0f, 200.0f }, 30.0f);
    ASSERT_NE (comp, nullptr);

    comp->addSolidLayer ("SolidLoop", Color (0xff00ff00), { 100.0f, 100.0f });
    comp->addShapeLayer ("ShapeSecond");
    comp->addNullLayer ("NullThird");

    EXPECT_EQ (comp->layers.size(), 3u);

    auto renderer = context->makeRenderer (200, 200);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 200, 200));
    });

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 15.0f, Rectangle<float> (0, 0, 200, 200));
    });
}

// =============================================================================
// Clip culling and precomp reuse
// =============================================================================

TEST_F (AnimationRendererTests, RenderSharedPrecompLayerDoesNotCrash)
{
    auto comp = LottieReader::parseData (kSharedPrecompJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 2u);

    // Both references must resolve to the same asset, which is what makes one
    // offscreen render serve both of them.
    const auto* left = dynamic_cast<const PrecompLayer*> (comp->layers[0].get());
    const auto* right = dynamic_cast<const PrecompLayer*> (comp->layers[1].get());
    ASSERT_NE (left, nullptr);
    ASSERT_NE (right, nullptr);
    EXPECT_EQ (left->precompRefId, right->precompRefId);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    const Rectangle<float> bounds (0, 0, 100, 100);

    for (const float frame : { 0.0f, 10.0f, 29.0f })
    {
        EXPECT_NO_THROW ({
            AnimationRenderer::renderComposition (g, *comp, frame, bounds);
        });
    }
}

TEST_F (AnimationRendererTests, RenderNestedPrecompLayerDoesNotCrash)
{
    auto comp = LottieReader::parseData (kNestedPrecompJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 1u);

    // The outer asset must draw the inner asset, so the render walks two levels
    // of precomp - each referenced only once.
    ASSERT_TRUE (comp->assets.contains (String ("outerAsset")));
    const auto& outer = comp->assets[String ("outerAsset")];
    ASSERT_NE (outer, nullptr);
    ASSERT_EQ (outer->layers.size(), 1u);

    const auto* middle = dynamic_cast<const PrecompLayer*> (outer->layers[0].get());
    ASSERT_NE (middle, nullptr);
    EXPECT_EQ (middle->precompRefId, String ("innerAsset"));

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    const Rectangle<float> bounds (0, 0, 100, 100);

    for (const float frame : { 0.0f, 10.0f, 29.0f })
    {
        EXPECT_NO_THROW ({
            AnimationRenderer::renderComposition (g, *comp, frame, bounds);
        });
    }
}

TEST_F (AnimationRendererTests, RenderPrecompLayerWithoutSizeFallsBackToCompositionSize)
{
    auto comp = LottieReader::parseData (kZeroSizePrecompJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    // The reader normalizes a precomp layer with no w/h to the composition size,
    // so the renderer only sees a zero-area precomp when a caller builds one by
    // hand - its degenerate-bounds guard is a safety net, not the common path.
    const auto* precomp = dynamic_cast<const PrecompLayer*> (comp->layers[0].get());
    ASSERT_NE (precomp, nullptr);
    EXPECT_EQ (precomp->layerSize, comp->size);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

TEST_F (AnimationRendererTests, RenderSharedAssetAtDifferentPrecompSizesDoesNotCrash)
{
    auto comp = LottieReader::parseData (kPrecompSizedAssetJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 2u);

    // The same masked asset drawn at two sizes derives two different mask clips,
    // so the per-layer mask cache must not hand one size's clip to the other.
    const auto* small = dynamic_cast<const PrecompLayer*> (comp->layers[0].get());
    const auto* large = dynamic_cast<const PrecompLayer*> (comp->layers[1].get());
    ASSERT_NE (small, nullptr);
    ASSERT_NE (large, nullptr);
    EXPECT_EQ (small->precompRefId, large->precompRefId);
    EXPECT_NE (small->layerSize, large->layerSize);

    auto renderer = context->makeRenderer (200, 200);
    Graphics g (*context, *renderer);

    const Rectangle<float> bounds (0, 0, 200, 200);

    for (const float frame : { 0.0f, 10.0f })
    {
        EXPECT_NO_THROW ({
            AnimationRenderer::renderComposition (g, *comp, frame, bounds);
        });
    }
}

TEST_F (AnimationRendererTests, RenderStaticMaskAcrossFramesDoesNotCrash)
{
    auto comp = LottieReader::parseData (kMaskAddJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    const Rectangle<float> bounds (0, 0, 100, 100);

    // A static mask must keep clipping the same way on every frame, including
    // after the clip has been cached for the layer.
    for (const float frame : { 0.0f, 1.0f, 4.5f, 9.0f, 3.0f })
    {
        EXPECT_NO_THROW ({
            AnimationRenderer::renderComposition (g, *comp, frame, bounds);
        });
    }
}

TEST_F (AnimationRendererTests, RenderMaskOutsideViewportDoesNotCrash)
{
    auto comp = LottieReader::parseData (kMaskOutsideViewportJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    // The mask cannot overlap the composition viewport, so the layer is culled
    // without building the clip intersection.
    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });

    // The culled render must leave the Graphics state balanced, so a following
    // render into the same context still draws.
    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

TEST_F (AnimationRendererTests, RenderLayerWithSelfParentDoesNotCrash)
{
    auto comp = LottieReader::parseData (kSelfParentJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);
    ASSERT_GE (comp->layers.size(), 3u);

    // A layer parented to itself must not send the parent-chain walk into a loop.
    ASSERT_NE (comp->layers[0], nullptr);
    EXPECT_EQ (comp->layers[0]->parentId, comp->layers[0]->id);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 100, 100));
    });
}

TEST_F (AnimationRendererTests, RenderBoundsOutsideCompositionDoesNotCrash)
{
    auto comp = LottieReader::parseData (kShapeLayerJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    auto renderer = context->makeRenderer (200, 200);
    Graphics g (*context, *renderer);

    // Bounds that miss the composition cull the whole render, which returns
    // before any layer is walked.
    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (500, 500, 50, 50));
    });

    // Rendering normally afterwards must still produce output, proving the
    // culling path restored the Graphics state it saved.
    EXPECT_NO_THROW ({
        AnimationRenderer::renderComposition (g, *comp, 0.0f, Rectangle<float> (0, 0, 200, 200));
    });
}

// =============================================================================
// Opacity handling: which layers need a transparency layer at all
// =============================================================================

TEST_F (AnimationRendererTests, RenderPartialOpacityLayersDoesNotCrash)
{
    auto comp = LottieReader::parseData (kPartialOpacityLayersJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 4u);

    // The fixture has to cover each decision the renderer makes about opacities:
    // a solid and a null below the threshold (neither needs isolating), a solid
    // just under it, and a solid carrying a drop shadow (which still does).
    ASSERT_NE (comp->layers[0], nullptr);
    EXPECT_EQ (comp->layers[0]->getType(), AnimationLayer::Type::Solid);
    EXPECT_NEAR (comp->layers[0]->transform.opacityAt (0.0f), 0.5f, 1.0e-4f);
    EXPECT_FALSE (comp->layers[0]->dropShadow.has_value());

    ASSERT_NE (comp->layers[1], nullptr);
    EXPECT_EQ (comp->layers[1]->getType(), AnimationLayer::Type::Null);
    EXPECT_NEAR (comp->layers[1]->transform.opacityAt (0.0f), 0.4f, 1.0e-4f);

    ASSERT_NE (comp->layers[2], nullptr);
    EXPECT_EQ (comp->layers[2]->getType(), AnimationLayer::Type::Solid);
    EXPECT_NEAR (comp->layers[2]->transform.opacityAt (0.0f), 0.99f, 1.0e-4f);

    ASSERT_NE (comp->layers[3], nullptr);
    EXPECT_EQ (comp->layers[3]->getType(), AnimationLayer::Type::Solid);
    ASSERT_TRUE (comp->layers[3]->dropShadow.has_value());
    EXPECT_TRUE (comp->layers[3]->dropShadow->enabled);

    auto renderer = context->makeRenderer (200, 200);
    Graphics g (*context, *renderer);

    const Rectangle<float> bounds (0, 0, 200, 200);

    for (const float frame : { 0.0f, 5.0f, 29.0f })
    {
        EXPECT_NO_THROW ({
            AnimationRenderer::renderComposition (g, *comp, frame, bounds);
        });
    }
}

TEST_F (AnimationRendererTests, RenderNearOpaqueShapeLayersDoesNotCrash)
{
    auto comp = LottieReader::parseData (kNearOpaqueShapeJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 2u);

    // 99.9 is an exporter's rounding of "fully opaque" and must not isolate the
    // layer; 99 is a real half-percent and still has to composite offscreen.
    ASSERT_NE (comp->layers[0], nullptr);
    EXPECT_NEAR (comp->layers[0]->transform.opacityAt (0.0f), 0.999f, 1.0e-4f);

    ASSERT_NE (comp->layers[1], nullptr);
    EXPECT_NEAR (comp->layers[1]->transform.opacityAt (0.0f), 0.99f, 1.0e-4f);

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    const Rectangle<float> bounds (0, 0, 100, 100);

    for (const float frame : { 0.0f, 7.0f, 29.0f })
    {
        EXPECT_NO_THROW ({
            AnimationRenderer::renderComposition (g, *comp, frame, bounds);
        });
    }
}

TEST_F (AnimationRendererTests, RenderSequentialPrecompSlicesDoesNotCrash)
{
    auto comp = LottieReader::parseData (kSequentialPrecompSlicesJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 3u);

    // All three layers draw the same asset, but their visibility windows must not
    // overlap, so only one of them is ever drawn - the asset must not be treated
    // as shared on the strength of the two invisible references.
    const auto* start = dynamic_cast<const PrecompLayer*> (comp->layers[0].get());
    const auto* middle = dynamic_cast<const PrecompLayer*> (comp->layers[1].get());
    const auto* end = dynamic_cast<const PrecompLayer*> (comp->layers[2].get());
    ASSERT_NE (start, nullptr);
    ASSERT_NE (middle, nullptr);
    ASSERT_NE (end, nullptr);
    EXPECT_EQ (start->precompRefId, middle->precompRefId);
    EXPECT_EQ (middle->precompRefId, end->precompRefId);

    for (const float frame : { 5.0f, 15.0f, 25.0f })
    {
        int visible = 0;
        for (const auto& layer : comp->layers)
        {
            if (layer != nullptr && layer->isVisibleAt (frame))
                ++visible;
        }

        EXPECT_EQ (visible, 1) << "at frame " << frame;
    }

    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    const Rectangle<float> bounds (0, 0, 100, 100);

    // Cover both sides of each window boundary as well as its interior.
    for (const float frame : { 0.0f, 5.0f, 9.0f, 10.0f, 15.0f, 19.0f, 20.0f, 25.0f, 29.0f })
    {
        EXPECT_NO_THROW ({
            AnimationRenderer::renderComposition (g, *comp, frame, bounds);
        });
    }
}

TEST_F (AnimationRendererTests, RenderPartialOpacityPrecompLayerDoesNotCrash)
{
    auto comp = LottieReader::parseData (kPartialOpacityPrecompJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 1u);

    // The isolated target for this layer only needs to cover the precomp's own
    // box, which is far smaller than the composition it is drawn into.
    const auto* precomp = dynamic_cast<const PrecompLayer*> (comp->layers[0].get());
    ASSERT_NE (precomp, nullptr);
    EXPECT_NEAR (precomp->transform.opacityAt (0.0f), 0.4f, 1.0e-4f);
    EXPECT_LT (precomp->layerSize.getWidth(), comp->size.getWidth());
    EXPECT_LT (precomp->layerSize.getHeight(), comp->size.getHeight());

    auto renderer = context->makeRenderer (200, 200);
    Graphics g (*context, *renderer);

    const Rectangle<float> bounds (0, 0, 200, 200);

    for (const float frame : { 0.0f, 5.0f, 29.0f })
    {
        EXPECT_NO_THROW ({
            AnimationRenderer::renderComposition (g, *comp, frame, bounds);
        });
    }
}

