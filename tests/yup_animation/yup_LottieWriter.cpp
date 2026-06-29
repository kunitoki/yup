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

} // namespace

class LottieWriterTests : public ::testing::Test
{
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
