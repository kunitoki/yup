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

constexpr const char* kMinimalJson = R"json({
    "v": "5.5.2",
    "nm": "MinimalComp",
    "ip": 0,
    "op": 50,
    "fr": 25.0,
    "w": 200,
    "h": 150,
    "ddd": 0,
    "assets": [],
    "layers": []
})json";

constexpr const char* kShapeAndNullJson = R"json({
    "v": "5.5.2",
    "nm": "TwoLayerComp",
    "ip": 0,
    "op": 60,
    "fr": 30.0,
    "w": 400,
    "h": 300,
    "ddd": 0,
    "assets": [],
    "layers": [
        {
            "ty": 4,
            "nm": "ShapeOne",
            "ind": 1,
            "ip": 0,
            "op": 60,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [200, 150] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 100 }
            },
            "shapes": []
        },
        {
            "ty": 3,
            "nm": "NullControl",
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

constexpr const char* kMarkerJson = R"json({
    "v": "5.5.2",
    "nm": "MarkerComp",
    "ip": 0,
    "op": 90,
    "fr": 30.0,
    "w": 100,
    "h": 100,
    "ddd": 0,
    "assets": [],
    "markers": [
        { "cm": "intro", "tm": 0, "dr": 30 },
        { "cm": "outro", "tm": 60, "dr": 30 }
    ],
    "layers": []
})json";

constexpr const char* kSolidLayerExampleJson = R"json({
    "v": "5.5.2",
    "nm": "SolidComp",
    "ip": 0,
    "op": 30,
    "fr": 25.0,
    "w": 200,
    "h": 150,
    "ddd": 0,
    "assets": [],
    "layers": [
        {
            "ty": 1,
            "nm": "MySolid",
            "ind": 1,
            "ip": 0,
            "op": 30,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "sc": "#ff0000",
            "sw": 200,
            "sh": 150,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [100, 75] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 100 }
            }
        }
    ]
})json";

constexpr const char* kShapeWithFillJson = R"json({
    "v": "5.5.2",
    "nm": "ShapeFillComp",
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
            "nm": "FillLayer",
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
                    "nm": "Group1",
                    "it": [
                        { "ty": "rc", "nm": "Rect", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [40, 40] }, "r": { "a": 0, "k": 0 } },
                        { "ty": "fl", "nm": "RedFill", "c": { "a": 0, "k": [1, 0, 0, 1] }, "o": { "a": 0, "k": 100 }, "r": 1 }
                    ]
                }
            ]
        }
    ]
})json";

constexpr const char* kShapeWithStrokeJson = R"json({
    "v": "5.5.2",
    "nm": "ShapeStrokeComp",
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
            "nm": "StrokeLayer",
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
                    "nm": "Group1",
                    "it": [
                        { "ty": "el", "nm": "Ellipse", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [40, 30] } },
                        { "ty": "st", "nm": "BlueStroke", "c": { "a": 0, "k": [0, 0, 1, 1] }, "o": { "a": 0, "k": 100 }, "w": { "a": 0, "k": 3 } }
                    ]
                }
            ]
        }
    ]
})json";

constexpr const char* kShapeWithEllipseJson = R"json({
    "v": "5.5.2",
    "nm": "EllipseComp",
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
            "nm": "EllipseLayer",
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
                    "nm": "EllipseGroup",
                    "it": [
                        { "ty": "el", "nm": "MyEllipse", "p": { "a": 0, "k": [10, 20] }, "s": { "a": 0, "k": [50, 30] } },
                        { "ty": "fl", "nm": "Fill", "c": { "a": 0, "k": [0, 1, 0, 1] }, "o": { "a": 0, "k": 100 }, "r": 1 }
                    ]
                }
            ]
        }
    ]
})json";

constexpr const char* kShapeWithMaskJson = R"json({
    "v": "5.5.2",
    "nm": "MaskComp",
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
                    "mode": "a",
                    "pt": { "a": 0, "k": { "i": [[0,0],[0,0],[0,0],[0,0]], "o": [[0,0],[0,0],[0,0],[0,0]], "v": [[10,10],[90,10],[90,90],[10,90]], "c": true } },
                    "o": { "a": 0, "k": 100 }
                }
            ],
            "shapes": []
        }
    ]
})json";

constexpr const char* kHiddenShapeLayerJson = R"json({
    "v": "5.5.2",
    "nm": "HiddenLayerComp",
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
            "nm": "HiddenShape",
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

constexpr const char* kLayerWithInOutJson = R"json({
    "v": "5.5.2",
    "nm": "InOutComp",
    "ip": 0,
    "op": 90,
    "fr": 30.0,
    "w": 100,
    "h": 100,
    "ddd": 0,
    "assets": [],
    "layers": [
        {
            "ty": 3,
            "nm": "TimedNull",
            "ind": 1,
            "ip": 10,
            "op": 80,
            "st": 5,
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

} // namespace

class LottieWriterTests : public ::testing::Test
{
protected:
    static File getLottieTestDataDir()
    {
        return File (__FILE__)
            .getParentDirectory()
            .getParentDirectory()
            .getChildFile ("data")
            .getChildFile ("lottie");
    }
};

// =============================================================================
// Basic serialization
// =============================================================================

TEST_F (LottieWriterTests, ToJson_ProducesNonEmptyString)
{
    auto comp = AnimationComposition::create ({ 100.0f, 100.0f }, 25.0f);
    ASSERT_NE (comp, nullptr);

    const String json = LottieWriter::toJson (*comp);
    EXPECT_FALSE (json.isEmpty());
}

TEST_F (LottieWriterTests, ToJson_PrettyPrintContainsNewlines)
{
    auto comp = AnimationComposition::create ({ 100.0f, 100.0f }, 25.0f);
    ASSERT_NE (comp, nullptr);

    const String pretty = LottieWriter::toJson (*comp, true);
    EXPECT_TRUE (pretty.containsChar ('\n'));
}

TEST_F (LottieWriterTests, ToJson_CompactHasNoNewlines)
{
    auto comp = AnimationComposition::create ({ 100.0f, 100.0f }, 25.0f);
    ASSERT_NE (comp, nullptr);

    const String compact = LottieWriter::toJson (*comp, false);
    EXPECT_FALSE (compact.containsChar ('\n'));
}

TEST_F (LottieWriterTests, ToJson_CompactIsShorterThanPrettyPrint)
{
    auto comp = AnimationComposition::create ({ 200.0f, 150.0f }, 30.0f);
    ASSERT_NE (comp, nullptr);
    comp->addShapeLayer ("Layer1");
    comp->addNullLayer ("Layer2");

    const String pretty = LottieWriter::toJson (*comp, true);
    const String compact = LottieWriter::toJson (*comp, false);

    EXPECT_LT (compact.length(), pretty.length());
}

// =============================================================================
// Field presence
// =============================================================================

TEST_F (LottieWriterTests, ToJson_ContainsRequiredTopLevelFields)
{
    auto comp = AnimationComposition::create ({ 100.0f, 100.0f }, 25.0f);
    ASSERT_NE (comp, nullptr);

    const std::string json = LottieWriter::toJson (*comp).toStdString();

    EXPECT_NE (json.find ("\"v\""), std::string::npos);
    EXPECT_NE (json.find ("\"nm\""), std::string::npos);
    EXPECT_NE (json.find ("\"ip\""), std::string::npos);
    EXPECT_NE (json.find ("\"op\""), std::string::npos);
    EXPECT_NE (json.find ("\"fr\""), std::string::npos);
    EXPECT_NE (json.find ("\"w\""), std::string::npos);
    EXPECT_NE (json.find ("\"h\""), std::string::npos);
    EXPECT_NE (json.find ("\"layers\""), std::string::npos);
}

// =============================================================================
// Composition metadata roundtrip
// =============================================================================

TEST_F (LottieWriterTests, Roundtrip_PreservesCompositionName)
{
    auto comp = LottieReader::parseData (kMinimalJson);
    ASSERT_NE (comp, nullptr);

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    EXPECT_EQ (readback->name, "MinimalComp");
}

TEST_F (LottieWriterTests, Roundtrip_PreservesFrameRate)
{
    auto comp = LottieReader::parseData (kMinimalJson);
    ASSERT_NE (comp, nullptr);

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    EXPECT_FLOAT_EQ (readback->frameRate, 25.0f);
}

TEST_F (LottieWriterTests, Roundtrip_PreservesSize)
{
    auto comp = LottieReader::parseData (kMinimalJson);
    ASSERT_NE (comp, nullptr);

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    EXPECT_FLOAT_EQ (readback->size.getWidth(), 200.0f);
    EXPECT_FLOAT_EQ (readback->size.getHeight(), 150.0f);
}

TEST_F (LottieWriterTests, Roundtrip_PreservesStartAndEndFrames)
{
    auto comp = LottieReader::parseData (kMinimalJson);
    ASSERT_NE (comp, nullptr);

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    EXPECT_FLOAT_EQ (readback->startFrame, 0.0f);
    EXPECT_FLOAT_EQ (readback->endFrame, 50.0f);
}

TEST_F (LottieWriterTests, Roundtrip_EmptyLayerList)
{
    auto comp = LottieReader::parseData (kMinimalJson);
    ASSERT_NE (comp, nullptr);
    EXPECT_EQ (comp->layers.size(), 0u);

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    EXPECT_EQ (readback->layers.size(), 0u);
}

TEST_F (LottieWriterTests, Roundtrip_PreservesLayerCount)
{
    auto comp = LottieReader::parseData (kShapeAndNullJson);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 2u);

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    EXPECT_EQ (readback->layers.size(), 2u);
}

TEST_F (LottieWriterTests, Roundtrip_PreservesLayerAutoOrient)
{
    auto comp = LottieReader::parseData (kShapeAndNullJson);
    ASSERT_NE (comp, nullptr);
    ASSERT_FALSE (comp->layers.empty());
    comp->layers[0]->autoOrient = true;
    comp->layers[0]->transform.autoOrient = true;

    auto readback = LottieReader::parseData (LottieWriter::toJson (*comp));

    ASSERT_NE (readback, nullptr);
    ASSERT_FALSE (readback->layers.empty());
    EXPECT_TRUE (readback->layers[0]->autoOrient);
    EXPECT_TRUE (readback->layers[0]->transform.autoOrient);
}

TEST_F (LottieWriterTests, Roundtrip_PreservesLayerNames)
{
    auto comp = LottieReader::parseData (kShapeAndNullJson);
    ASSERT_NE (comp, nullptr);

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    ASSERT_EQ (readback->layers.size(), 2u);
    EXPECT_EQ (readback->layers[0]->name, "ShapeOne");
    EXPECT_EQ (readback->layers[1]->name, "NullControl");
}

TEST_F (LottieWriterTests, Roundtrip_PreservesLayerTypes)
{
    auto comp = LottieReader::parseData (kShapeAndNullJson);
    ASSERT_NE (comp, nullptr);

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    ASSERT_EQ (readback->layers.size(), 2u);
    EXPECT_EQ (readback->layers[0]->getType(), AnimationLayer::Type::Shape);
    EXPECT_EQ (readback->layers[1]->getType(), AnimationLayer::Type::Null);
}

// =============================================================================
// Programmatically-built compositions
// =============================================================================

TEST_F (LottieWriterTests, Roundtrip_ProgrammaticCompositionMetadata)
{
    auto comp = AnimationComposition::create ({ 320.0f, 240.0f }, 60.0f);
    ASSERT_NE (comp, nullptr);
    comp->name = "MyAnimation";
    comp->startFrame = 5.0f;
    comp->endFrame = 65.0f;

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    EXPECT_EQ (readback->name, String ("MyAnimation"));
    EXPECT_FLOAT_EQ (readback->frameRate, 60.0f);
    EXPECT_FLOAT_EQ (readback->size.getWidth(), 320.0f);
    EXPECT_FLOAT_EQ (readback->size.getHeight(), 240.0f);
    EXPECT_FLOAT_EQ (readback->startFrame, 5.0f);
    EXPECT_FLOAT_EQ (readback->endFrame, 65.0f);
}

TEST_F (LottieWriterTests, Roundtrip_ProgrammaticLayers)
{
    auto comp = AnimationComposition::create ({ 100.0f, 100.0f }, 25.0f);
    ASSERT_NE (comp, nullptr);
    comp->addShapeLayer ("ShapeA");
    comp->addNullLayer ("NullB");

    ASSERT_EQ (comp->layers.size(), 2u);

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    ASSERT_EQ (readback->layers.size(), 2u);
    EXPECT_EQ (readback->layers[0]->getType(), AnimationLayer::Type::Shape);
    EXPECT_EQ (readback->layers[1]->getType(), AnimationLayer::Type::Null);
    EXPECT_EQ (readback->layers[0]->name, String ("ShapeA"));
    EXPECT_EQ (readback->layers[1]->name, String ("NullB"));
}

// =============================================================================
// Markers roundtrip
// =============================================================================

TEST_F (LottieWriterTests, Roundtrip_PreservesMarkers)
{
    auto comp = LottieReader::parseData (kMarkerJson);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->markers.size(), 2u);

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    ASSERT_EQ (readback->markers.size(), 2u);
    EXPECT_EQ (readback->markers[0].comment, String ("intro"));
    EXPECT_EQ (readback->markers[1].comment, String ("outro"));
}

TEST_F (LottieWriterTests, Roundtrip_PreservesMarkerTiming)
{
    auto comp = LottieReader::parseData (kMarkerJson);
    ASSERT_NE (comp, nullptr);

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    ASSERT_EQ (readback->markers.size(), 2u);
    EXPECT_FLOAT_EQ (readback->markers[0].startFrame, 0.0f);
    EXPECT_FLOAT_EQ (readback->markers[0].duration, 30.0f);
    EXPECT_FLOAT_EQ (readback->markers[1].startFrame, 60.0f);
    EXPECT_FLOAT_EQ (readback->markers[1].duration, 30.0f);
}

// =============================================================================
// toFile
// =============================================================================

TEST_F (LottieWriterTests, ToFile_WritesToTemporaryFileAndCanBeReadBack)
{
    auto comp = AnimationComposition::create ({ 100.0f, 100.0f }, 25.0f);
    ASSERT_NE (comp, nullptr);
    comp->name = "FileRoundtripTest";
    comp->addShapeLayer ("Layer1");

    const File tempFile = File::createTempFile ("lottie_writer_test.json");

    const Result writeResult = LottieWriter::toFile (*comp, tempFile);
    EXPECT_TRUE (writeResult.wasOk());
    EXPECT_TRUE (tempFile.exists());

    String outError;
    auto readback = LottieReader::parseFile (tempFile, {}, &outError);

    ASSERT_NE (readback, nullptr) << outError;
    EXPECT_EQ (readback->name, String ("FileRoundtripTest"));
    EXPECT_EQ (readback->layers.size(), 1u);
    EXPECT_FLOAT_EQ (readback->frameRate, 25.0f);

    tempFile.deleteFile();
}

// =============================================================================
// Composition stats after roundtrip
// =============================================================================

TEST_F (LottieWriterTests, Roundtrip_StatsAreConsistentAfterWriteAndRead)
{
    auto comp = LottieReader::parseData (kShapeAndNullJson);
    ASSERT_NE (comp, nullptr);

    const auto originalStats = comp->computeStats();

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);
    ASSERT_NE (readback, nullptr);

    const auto readbackStats = readback->computeStats();

    EXPECT_EQ (readbackStats.shapeLayerCount, originalStats.shapeLayerCount);
    EXPECT_EQ (readbackStats.nullLayerCount, originalStats.nullLayerCount);
    EXPECT_EQ (readbackStats.totalLayerCount(), originalStats.totalLayerCount());
}

// =============================================================================
// Solid layer roundtrip
// =============================================================================

TEST_F (LottieWriterTests, Roundtrip_SolidLayerPreservesType)
{
    auto comp = LottieReader::parseData (kSolidLayerExampleJson);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 1u);
    EXPECT_EQ (comp->layers[0]->getType(), AnimationLayer::Type::Solid);

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    ASSERT_EQ (readback->layers.size(), 1u);
    EXPECT_EQ (readback->layers[0]->getType(), AnimationLayer::Type::Solid);
}

TEST_F (LottieWriterTests, Roundtrip_SolidLayerPreservesSize)
{
    auto comp = LottieReader::parseData (kSolidLayerExampleJson);
    ASSERT_NE (comp, nullptr);

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);
    ASSERT_NE (readback, nullptr);
    ASSERT_EQ (readback->layers.size(), 1u);

    const auto* solid = dynamic_cast<const SolidLayer*> (readback->layers[0].get());
    ASSERT_NE (solid, nullptr);
    EXPECT_FLOAT_EQ (solid->layerSize.getWidth(), 200.0f);
    EXPECT_FLOAT_EQ (solid->layerSize.getHeight(), 150.0f);
}

TEST_F (LottieWriterTests, ToJson_SolidLayerContainsSolidColorField)
{
    auto comp = LottieReader::parseData (kSolidLayerExampleJson);
    ASSERT_NE (comp, nullptr);

    const std::string json = LottieWriter::toJson (*comp).toStdString();
    EXPECT_NE (json.find ("\"sc\""), std::string::npos);
    EXPECT_NE (json.find ("\"sw\""), std::string::npos);
    EXPECT_NE (json.find ("\"sh\""), std::string::npos);
}

// =============================================================================
// Shape layer with fill roundtrip
// =============================================================================

TEST_F (LottieWriterTests, Roundtrip_ShapeLayerWithFillPreservesLayerType)
{
    auto comp = LottieReader::parseData (kShapeWithFillJson);
    ASSERT_NE (comp, nullptr);

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    ASSERT_EQ (readback->layers.size(), 1u);
    EXPECT_EQ (readback->layers[0]->getType(), AnimationLayer::Type::Shape);
}

TEST_F (LottieWriterTests, ToJson_ShapeLayerWithFillContainsShapesField)
{
    auto comp = LottieReader::parseData (kShapeWithFillJson);
    ASSERT_NE (comp, nullptr);

    const std::string json = LottieWriter::toJson (*comp).toStdString();
    EXPECT_NE (json.find ("\"shapes\""), std::string::npos);
    EXPECT_NE (json.find ("\"fl\""), std::string::npos);
}

TEST_F (LottieWriterTests, Roundtrip_ShapeLayerWithFillPreservesGroupCount)
{
    auto comp = LottieReader::parseData (kShapeWithFillJson);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 1u);
    const auto* sl = dynamic_cast<const ShapeLayer*> (comp->layers[0].get());
    ASSERT_NE (sl, nullptr);
    const auto originalGroupCount = sl->groups.size();

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    ASSERT_EQ (readback->layers.size(), 1u);
    const auto* readbackSl = dynamic_cast<const ShapeLayer*> (readback->layers[0].get());
    ASSERT_NE (readbackSl, nullptr);
    EXPECT_EQ (readbackSl->groups.size(), originalGroupCount);
}

// =============================================================================
// Shape layer with stroke roundtrip
// =============================================================================

TEST_F (LottieWriterTests, Roundtrip_ShapeLayerWithStrokePreservesType)
{
    auto comp = LottieReader::parseData (kShapeWithStrokeJson);
    ASSERT_NE (comp, nullptr);

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    ASSERT_EQ (readback->layers.size(), 1u);
    EXPECT_EQ (readback->layers[0]->getType(), AnimationLayer::Type::Shape);
}

TEST_F (LottieWriterTests, ToJson_ShapeLayerWithStrokeContainsStrokeType)
{
    auto comp = LottieReader::parseData (kShapeWithStrokeJson);
    ASSERT_NE (comp, nullptr);

    const std::string json = LottieWriter::toJson (*comp).toStdString();
    EXPECT_NE (json.find ("\"st\""), std::string::npos);
}

// =============================================================================
// Ellipse shape roundtrip
// =============================================================================

TEST_F (LottieWriterTests, Roundtrip_ShapeLayerWithEllipsePreservesType)
{
    auto comp = LottieReader::parseData (kShapeWithEllipseJson);
    ASSERT_NE (comp, nullptr);

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    ASSERT_EQ (readback->layers.size(), 1u);
    EXPECT_EQ (readback->layers[0]->getType(), AnimationLayer::Type::Shape);
}

TEST_F (LottieWriterTests, ToJson_ShapeLayerWithEllipseContainsEllipseType)
{
    auto comp = LottieReader::parseData (kShapeWithEllipseJson);
    ASSERT_NE (comp, nullptr);

    const std::string json = LottieWriter::toJson (*comp).toStdString();
    EXPECT_NE (json.find ("\"el\""), std::string::npos);
}

// =============================================================================
// Layer mask roundtrip
// =============================================================================

TEST_F (LottieWriterTests, Roundtrip_LayerWithMaskPreservesMaskCount)
{
    auto comp = LottieReader::parseData (kShapeWithMaskJson);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 1u);
    EXPECT_EQ (comp->layers[0]->masks.size(), 1u);

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    ASSERT_EQ (readback->layers.size(), 1u);
    EXPECT_EQ (readback->layers[0]->masks.size(), 1u);
}

TEST_F (LottieWriterTests, ToJson_LayerWithMaskContainsMasksPropertiesField)
{
    auto comp = LottieReader::parseData (kShapeWithMaskJson);
    ASSERT_NE (comp, nullptr);

    const std::string json = LottieWriter::toJson (*comp).toStdString();
    EXPECT_NE (json.find ("\"masksProperties\""), std::string::npos);
}

// =============================================================================
// Hidden layer roundtrip
// =============================================================================

TEST_F (LottieWriterTests, Roundtrip_HiddenLayerPreservesHiddenState)
{
    auto comp = LottieReader::parseData (kHiddenShapeLayerJson);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 1u);
    EXPECT_TRUE (comp->layers[0]->hidden);

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    ASSERT_EQ (readback->layers.size(), 1u);
    EXPECT_TRUE (readback->layers[0]->hidden);
}

// =============================================================================
// Layer in/out frames roundtrip
// =============================================================================

TEST_F (LottieWriterTests, Roundtrip_LayerInOutFramesArePreserved)
{
    auto comp = LottieReader::parseData (kLayerWithInOutJson);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 1u);
    EXPECT_FLOAT_EQ (comp->layers[0]->inFrame, 10.0f);
    EXPECT_FLOAT_EQ (comp->layers[0]->outFrame, 80.0f);

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    ASSERT_EQ (readback->layers.size(), 1u);
    EXPECT_FLOAT_EQ (readback->layers[0]->inFrame, 10.0f);
    EXPECT_FLOAT_EQ (readback->layers[0]->outFrame, 80.0f);
}

TEST_F (LottieWriterTests, Roundtrip_LayerStartFrameIsPreserved)
{
    auto comp = LottieReader::parseData (kLayerWithInOutJson);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 1u);
    EXPECT_FLOAT_EQ (comp->layers[0]->startFrame, 5.0f);

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    ASSERT_EQ (readback->layers.size(), 1u);
    EXPECT_FLOAT_EQ (readback->layers[0]->startFrame, 5.0f);
}

// =============================================================================
// Programmatic solid / shape layer
// =============================================================================

TEST_F (LottieWriterTests, Roundtrip_ProgrammaticSolidLayer)
{
    auto comp = AnimationComposition::create ({ 100.0f, 100.0f }, 25.0f);
    ASSERT_NE (comp, nullptr);
    comp->addSolidLayer ("RedSolid", Color (0xFFFF0000), { 80.0f, 60.0f });

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    ASSERT_EQ (readback->layers.size(), 1u);
    EXPECT_EQ (readback->layers[0]->getType(), AnimationLayer::Type::Solid);
    EXPECT_EQ (readback->layers[0]->name, String ("RedSolid"));

    const auto* solid = dynamic_cast<const SolidLayer*> (readback->layers[0].get());
    ASSERT_NE (solid, nullptr);
    EXPECT_FLOAT_EQ (solid->layerSize.getWidth(), 80.0f);
    EXPECT_FLOAT_EQ (solid->layerSize.getHeight(), 60.0f);
}

// =============================================================================
// Transform serialization — ks fields
// =============================================================================

TEST_F (LottieWriterTests, ToJson_LayerContainsTransformField)
{
    auto comp = LottieReader::parseData (kShapeWithFillJson);
    ASSERT_NE (comp, nullptr);

    const std::string json = LottieWriter::toJson (*comp).toStdString();
    EXPECT_NE (json.find ("\"ks\""), std::string::npos);
}

// =============================================================================
// Roundtrip with test data files — tests/data/lottie/*
// =============================================================================

#if ! YUP_WASM

TEST_F (LottieWriterTests, Roundtrip_GoalLottiePreservesName)
{
    const File file = getLottieTestDataDir().getChildFile ("goal.lottie");
    ASSERT_TRUE (file.existsAsFile());

    auto comp = LottieReader::parseFile (file);
    ASSERT_NE (comp, nullptr);

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    EXPECT_EQ (readback->name, String ("Goal"));
}

TEST_F (LottieWriterTests, Roundtrip_GoalLottiePreservesFrameRate)
{
    const File file = getLottieTestDataDir().getChildFile ("goal.lottie");
    ASSERT_TRUE (file.existsAsFile());

    auto comp = LottieReader::parseFile (file);
    ASSERT_NE (comp, nullptr);

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    EXPECT_FLOAT_EQ (readback->frameRate, 30.0f);
}

TEST_F (LottieWriterTests, Roundtrip_GoalLottiePreservesSize)
{
    const File file = getLottieTestDataDir().getChildFile ("goal.lottie");
    ASSERT_TRUE (file.existsAsFile());

    auto comp = LottieReader::parseFile (file);
    ASSERT_NE (comp, nullptr);

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    EXPECT_FLOAT_EQ (readback->size.getWidth(), 1080.0f);
    EXPECT_FLOAT_EQ (readback->size.getHeight(), 844.0f);
}

TEST_F (LottieWriterTests, Roundtrip_GoalLottiePreservesLayerCount)
{
    const File file = getLottieTestDataDir().getChildFile ("goal.lottie");
    ASSERT_TRUE (file.existsAsFile());

    auto comp = LottieReader::parseFile (file);
    ASSERT_NE (comp, nullptr);

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    EXPECT_EQ (readback->layers.size(), 3u);
}

TEST_F (LottieWriterTests, Roundtrip_GoalLottiePreservesMarkers)
{
    const File file = getLottieTestDataDir().getChildFile ("goal.lottie");
    ASSERT_TRUE (file.existsAsFile());

    auto comp = LottieReader::parseFile (file);
    ASSERT_NE (comp, nullptr);

    const auto originalMarkerCount = comp->markers.size();

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    EXPECT_EQ (readback->markers.size(), originalMarkerCount);
}

TEST_F (LottieWriterTests, Roundtrip_JollyWalkerPreservesLayerCount)
{
    const File file = getLottieTestDataDir().getChildFile ("jolly_walker.json");
    ASSERT_TRUE (file.existsAsFile());

    auto comp = LottieReader::parseFile (file);
    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 21u);

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    EXPECT_EQ (readback->layers.size(), 21u);
}

TEST_F (LottieWriterTests, Roundtrip_JollyWalkerPreservesName)
{
    const File file = getLottieTestDataDir().getChildFile ("jolly_walker.json");
    ASSERT_TRUE (file.existsAsFile());

    auto comp = LottieReader::parseFile (file);
    ASSERT_NE (comp, nullptr);

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    EXPECT_EQ (readback->name, String ("Comp 1"));
}

TEST_F (LottieWriterTests, Roundtrip_JollyWalkerPreservesFrameRate)
{
    const File file = getLottieTestDataDir().getChildFile ("jolly_walker.json");
    ASSERT_TRUE (file.existsAsFile());

    auto comp = LottieReader::parseFile (file);
    ASSERT_NE (comp, nullptr);

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    EXPECT_FLOAT_EQ (readback->frameRate, 60.0f);
}

TEST_F (LottieWriterTests, Roundtrip_ImageTestPreservesAssetCount)
{
    const File file = getLottieTestDataDir().getChildFile ("image_test.json");
    ASSERT_TRUE (file.existsAsFile());

    auto comp = LottieReader::parseFile (file);
    ASSERT_NE (comp, nullptr);

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    EXPECT_EQ (readback->assets.size(), 1u);
}

TEST_F (LottieWriterTests, Roundtrip_ImageEmbeddedPreservesAssetCount)
{
    const File file = getLottieTestDataDir().getChildFile ("image_embedded.json");
    ASSERT_TRUE (file.existsAsFile());

    auto comp = LottieReader::parseFile (file);
    ASSERT_NE (comp, nullptr);

    const String json = LottieWriter::toJson (*comp);
    auto readback = LottieReader::parseData (json);

    ASSERT_NE (readback, nullptr);
    EXPECT_EQ (readback->assets.size(), 1u);
}

TEST_F (LottieWriterTests, ToFile_GoalLottieWriteAndReadBack)
{
    const File inputFile = getLottieTestDataDir().getChildFile ("goal.lottie");
    ASSERT_TRUE (inputFile.existsAsFile());

    auto comp = LottieReader::parseFile (inputFile);
    ASSERT_NE (comp, nullptr);

    const File tempFile = File::createTempFile ("lottie_writer_goal.json");
    const Result writeResult = LottieWriter::toFile (*comp, tempFile);
    EXPECT_TRUE (writeResult.wasOk());
    EXPECT_TRUE (tempFile.exists());

    String outError;
    auto readback = LottieReader::parseFile (tempFile, {}, &outError);

    ASSERT_NE (readback, nullptr) << outError;
    EXPECT_EQ (readback->name, String ("Goal"));
    EXPECT_FLOAT_EQ (readback->frameRate, 30.0f);
    EXPECT_FLOAT_EQ (readback->size.getWidth(), 1080.0f);
    EXPECT_FLOAT_EQ (readback->size.getHeight(), 844.0f);
    EXPECT_EQ (readback->layers.size(), 3u);

    tempFile.deleteFile();
}

TEST_F (LottieWriterTests, ToFile_JollyWalkerWriteAndReadBack)
{
    const File inputFile = getLottieTestDataDir().getChildFile ("jolly_walker.json");
    ASSERT_TRUE (inputFile.existsAsFile());

    auto comp = LottieReader::parseFile (inputFile);
    ASSERT_NE (comp, nullptr);

    const File tempFile = File::createTempFile ("lottie_writer_jolly.json");
    const Result writeResult = LottieWriter::toFile (*comp, tempFile);
    EXPECT_TRUE (writeResult.wasOk());

    auto readback = LottieReader::parseFile (tempFile);
    ASSERT_NE (readback, nullptr);
    EXPECT_EQ (readback->layers.size(), 21u);

    tempFile.deleteFile();
}

#endif