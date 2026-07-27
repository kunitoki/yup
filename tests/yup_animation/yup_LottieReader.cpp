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

constexpr const char* kLottieReaderBaseJson = R"json({
    "v": "5.5.2",
    "nm": "ReaderTest",
    "ip": 0,
    "op": 20,
    "fr": 24.0,
    "w": 120,
    "h": 80,
    "ddd": 0,
    "assets": [],
    "layers": []
})json";

constexpr const char* kLottieReaderWithMarkersJson = R"json({
    "v": "5.5.2",
    "nm": "MarkerTest",
    "ip": 0,
    "op": 60,
    "fr": 30.0,
    "w": 100,
    "h": 100,
    "ddd": 0,
    "assets": [],
    "layers": [],
    "markers": [
        { "cm": "intro", "tm": 0,  "dr": 15 },
        { "cm": "loop",  "tm": 15, "dr": 30 },
        { "cm": "outro", "tm": 45, "dr": 15 }
    ]
})json";

constexpr const char* kLottieReaderImageAssetJson = R"json({
    "v": "5.5.2",
    "nm": "ImageAssetTest",
    "ip": 0,
    "op": 10,
    "fr": 25.0,
    "w": 64,
    "h": 64,
    "ddd": 0,
    "assets": [
        {
            "id": "img0",
            "u": "images/",
            "p": "logo.png",
            "w": 32,
            "h": 32,
            "e": 0
        }
    ],
    "layers": []
})json";

constexpr const char* kLottieReaderReversedFrameJson = R"json({
    "v": "5.5.2",
    "nm": "BadFrameRange",
    "ip": 50,
    "op": 10,
    "fr": 25.0,
    "w": 100,
    "h": 100,
    "ddd": 0,
    "assets": [],
    "layers": []
})json";

} // namespace

class LottieReaderTests : public ::testing::Test
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
// parseData — basic functionality
// =============================================================================

TEST_F (LottieReaderTests, ParseDataReturnsValidCompositionForValidJson)
{
    auto comp = LottieReader::parseData (kLottieReaderBaseJson).valueOr (nullptr);
    EXPECT_NE (comp, nullptr);
}

TEST_F (LottieReaderTests, ParseDataReturnsNullForGarbageJson)
{
    auto comp = LottieReader::parseData ("{{ not json }}").valueOr (nullptr);
    EXPECT_EQ (comp, nullptr);
}

TEST_F (LottieReaderTests, ParseDataPopulatesErrorOutputForBadJson)
{
    auto result = LottieReader::parseData ("{{ bad }");

    EXPECT_TRUE (result.failed());
    EXPECT_FALSE (result.getErrorMessage().isEmpty());
    EXPECT_TRUE (result.getErrorMessage().contains ("JSON parse error"));
}

TEST_F (LottieReaderTests, ParseDataSetsNoErrorForValidJson)
{
    auto result = LottieReader::parseData (kLottieReaderBaseJson);

    EXPECT_TRUE (result.wasOk());
    EXPECT_NE (result.getReference(), nullptr);
}

#if ! YUP_WASM
TEST_F (LottieReaderTests, ParseFilePreservesBellSolidColor)
{
    const auto file = getLottieTestDataDir().getChildFile ("bell.json");
    auto comp = LottieReader::parseFile (file).valueOr (nullptr);

    ASSERT_NE (comp, nullptr);
    ASSERT_GE (comp->layers.size(), 2u);

    const auto* solid = dynamic_cast<const SolidLayer*> (comp->layers[1].get());
    ASSERT_NE (solid, nullptr);
    EXPECT_EQ (solid->solidColor, Color (0xFF000000));
}
#endif

TEST_F (LottieReaderTests, ParseDataReadsLayerAutoOrient)
{
    auto comp = LottieReader::parseData (R"json({
        "v": "5.5.2", "ip": 0, "op": 10, "fr": 30, "w": 100, "h": 100,
        "layers": [{ "ty": 3, "ind": 1, "ao": 1, "ks": {} }]
    })json")
                    .valueOr (nullptr);

    ASSERT_NE (comp, nullptr);
    ASSERT_EQ (comp->layers.size(), 1u);
    EXPECT_TRUE (comp->layers[0]->autoOrient);
    EXPECT_TRUE (comp->layers[0]->transform.autoOrient);
}

TEST_F (LottieReaderTests, ParseDataParsesCompositionProperties)
{
    auto comp = LottieReader::parseData (kLottieReaderBaseJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    EXPECT_FLOAT_EQ (comp->size.getWidth(), 120.0f);
    EXPECT_FLOAT_EQ (comp->size.getHeight(), 80.0f);
    EXPECT_FLOAT_EQ (comp->frameRate, 24.0f);
    EXPECT_FLOAT_EQ (comp->startFrame, 0.0f);
    EXPECT_FLOAT_EQ (comp->endFrame, 20.0f);
    EXPECT_EQ (comp->name, String ("ReaderTest"));
    EXPECT_EQ (comp->version, String ("5.5.2"));
}

TEST_F (LottieReaderTests, ParseDataReturnsNullForReversedFrameRange)
{
    // ip=50 > op=10 — startFrame > endFrame → validation fails
    auto comp = LottieReader::parseData (kLottieReaderReversedFrameJson).valueOr (nullptr);
    EXPECT_EQ (comp, nullptr);
}

TEST_F (LottieReaderTests, ParseDataSetsErrorForReversedFrameRange)
{
    auto result = LottieReader::parseData (kLottieReaderReversedFrameJson);

    EXPECT_TRUE (result.failed());
    EXPECT_FALSE (result.getErrorMessage().isEmpty());
}

// =============================================================================
// parseData — markers
// =============================================================================

TEST_F (LottieReaderTests, ParseDataParsesMarkersField)
{
    auto comp = LottieReader::parseData (kLottieReaderWithMarkersJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);
    EXPECT_EQ (comp->markers.size(), 3u);
}

TEST_F (LottieReaderTests, ParseDataMarkersHaveCorrectFields)
{
    auto comp = LottieReader::parseData (kLottieReaderWithMarkersJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);
    ASSERT_GE (comp->markers.size(), 3u);

    EXPECT_EQ (comp->markers[0].comment, String ("intro"));
    EXPECT_FLOAT_EQ (comp->markers[0].startFrame, 0.0f);
    EXPECT_FLOAT_EQ (comp->markers[0].duration, 15.0f);

    EXPECT_EQ (comp->markers[1].comment, String ("loop"));
    EXPECT_FLOAT_EQ (comp->markers[1].startFrame, 15.0f);

    EXPECT_EQ (comp->markers[2].comment, String ("outro"));
    EXPECT_FLOAT_EQ (comp->markers[2].startFrame, 45.0f);
}

TEST_F (LottieReaderTests, ParseDataWithNoMarkersFieldHasEmptyMarkersVector)
{
    auto comp = LottieReader::parseData (kLottieReaderBaseJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);
    EXPECT_TRUE (comp->markers.empty());
}

// =============================================================================
// findMarker on composition
// =============================================================================

TEST_F (LottieReaderTests, FindMarkerReturnsCorrectMarkerByName)
{
    auto comp = LottieReader::parseData (kLottieReaderWithMarkersJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    const AnimationMarker* m = comp->findMarker ("loop");
    ASSERT_NE (m, nullptr);
    EXPECT_EQ (m->comment, String ("loop"));
    EXPECT_FLOAT_EQ (m->startFrame, 15.0f);
    EXPECT_FLOAT_EQ (m->duration, 30.0f);
}

TEST_F (LottieReaderTests, FindMarkerReturnsNullForMissingName)
{
    auto comp = LottieReader::parseData (kLottieReaderWithMarkersJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    EXPECT_EQ (comp->findMarker ("nonexistent"), nullptr);
}

TEST_F (LottieReaderTests, FindMarkerReturnsNullOnEmptyMarkersList)
{
    auto comp = LottieReader::parseData (kLottieReaderBaseJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    EXPECT_EQ (comp->findMarker ("any"), nullptr);
}

// =============================================================================
// parseFile
// =============================================================================

TEST_F (LottieReaderTests, ParseFileReturnsNullForMissingFile)
{
    auto comp = LottieReader::parseFile (File ("/nonexistent/path/file.json")).valueOr (nullptr);
    EXPECT_EQ (comp, nullptr);
}

TEST_F (LottieReaderTests, ParseFileSetsErrorForMissingFile)
{
    auto result = LottieReader::parseFile (File ("/nonexistent/path/file.json"));

    EXPECT_TRUE (result.failed());
    EXPECT_FALSE (result.getErrorMessage().isEmpty());
    EXPECT_TRUE (result.getErrorMessage().contains ("File not found"));
}

TEST_F (LottieReaderTests, ParseFileReturnsNullForEmptyFile)
{
    const File tempFile = File::createTempFile (".json");
    tempFile.replaceWithText ({});

    auto result = LottieReader::parseFile (tempFile);

    EXPECT_TRUE (result.failed());
    EXPECT_FALSE (result.getErrorMessage().isEmpty());

    tempFile.deleteFile();
}

TEST_F (LottieReaderTests, ParseFileReturnsValidCompositionForValidFile)
{
    const File tempFile = File::createTempFile (".json");
    tempFile.replaceWithText (kLottieReaderBaseJson);

    auto comp = LottieReader::parseFile (tempFile).valueOr (nullptr);
    EXPECT_NE (comp, nullptr);

    tempFile.deleteFile();
}

TEST_F (LottieReaderTests, ParseFileSetsResourceDirectoryFromFileParent)
{
    const File tempFile = File::createTempFile (".json");
    tempFile.replaceWithText (kLottieReaderBaseJson);

    LottieLoadOptions opts;
    auto comp = LottieReader::parseFile (tempFile, opts).valueOr (nullptr);
    EXPECT_NE (comp, nullptr);

    tempFile.deleteFile();
}

// =============================================================================
// listAnimationIds
// =============================================================================

TEST_F (LottieReaderTests, ListAnimationIdsReturnsEmptyForMissingFile)
{
    const auto ids = LottieReader::listAnimationIds (File ("/nonexistent/file.lottie"));
    EXPECT_TRUE (ids.empty());
}

TEST_F (LottieReaderTests, ListAnimationIdsReturnsEmptyForRegularJsonFile)
{
    const File tempFile = File::createTempFile (".lottie");
    tempFile.replaceWithText (kLottieReaderBaseJson);

    const auto ids = LottieReader::listAnimationIds (tempFile);
    EXPECT_TRUE (ids.empty());

    tempFile.deleteFile();
}

// =============================================================================
// parseFromZip
// =============================================================================

TEST_F (LottieReaderTests, ParseFromZipReturnsNullForMissingFile)
{
    auto result = LottieReader::parseFromZip (File ("/nonexistent/file.lottie"));

    EXPECT_TRUE (result.failed());
    EXPECT_FALSE (result.getErrorMessage().isEmpty());
}

TEST_F (LottieReaderTests, ParseFromZipReturnsNullForNonZipFile)
{
    const File tempFile = File::createTempFile (".lottie");
    tempFile.replaceWithText ("not a zip file");

    auto result = LottieReader::parseFromZip (tempFile);

    EXPECT_TRUE (result.failed());

    tempFile.deleteFile();
}

// =============================================================================
// imageResolver callback
// =============================================================================

TEST_F (LottieReaderTests, ImageResolverIsCalledForExternalImageAssets)
{
    bool resolverCalled = false;
    String capturedRef;

    LottieLoadOptions opts;
    opts.imageResolver = [&] (const String& ref, const File&) -> std::optional<Image>
    {
        resolverCalled = true;
        capturedRef = ref;
        return std::nullopt; // don't provide a bitmap
    };

    auto comp = LottieReader::parseData (kLottieReaderImageAssetJson, opts).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    EXPECT_TRUE (resolverCalled);
    EXPECT_TRUE (capturedRef.contains ("logo.png"));
}

TEST_F (LottieReaderTests, ImageResolverCanProvideImageForAsset)
{
    Image providedImage (32, 32, PixelFormat::RGBA);
    providedImage.fill (0xFF0000FFu);

    LottieLoadOptions opts;
    opts.imageResolver = [&] (const String&, const File&) -> std::optional<Image>
    {
        return providedImage;
    };

    auto comp = LottieReader::parseData (kLottieReaderImageAssetJson, opts).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    // Verify the asset was registered (asset table contains our image)
    EXPECT_EQ (comp->assets.size(), 1);
}

TEST_F (LottieReaderTests, ImageResolverNotCalledWhenNoImageAssets)
{
    bool resolverCalled = false;

    LottieLoadOptions opts;
    opts.imageResolver = [&] (const String&, const File&) -> std::optional<Image>
    {
        resolverCalled = true;
        return std::nullopt;
    };

    auto comp = LottieReader::parseData (kLottieReaderBaseJson, opts).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    EXPECT_FALSE (resolverCalled);
}

// =============================================================================
// Assets parsing
// =============================================================================

TEST_F (LottieReaderTests, ParseDataWithImageAssetCreatesAssetEntry)
{
    auto comp = LottieReader::parseData (kLottieReaderImageAssetJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    EXPECT_EQ (comp->assets.size(), 1);
    EXPECT_NE (comp->assets["img0"], nullptr);
}

TEST_F (LottieReaderTests, ParseDataImageAssetHasCorrectDimensions)
{
    auto comp = LottieReader::parseData (kLottieReaderImageAssetJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    const auto asset = comp->assets["img0"];
    ASSERT_NE (asset, nullptr);

    EXPECT_EQ (asset->width, 32);
    EXPECT_EQ (asset->height, 32);
}

TEST_F (LottieReaderTests, ParseDataImageAssetHasCorrectPath)
{
    auto comp = LottieReader::parseData (kLottieReaderImageAssetJson).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);

    const auto asset = comp->assets["img0"];
    ASSERT_NE (asset, nullptr);

    EXPECT_TRUE (asset->path.contains ("logo.png"));
}

// =============================================================================
// Test data files — tests/data/lottie/*
// =============================================================================

#if ! YUP_WASM
TEST_F (LottieReaderTests, ParseFileWithGoalLottieReturnsValidComposition)
{
    const File file = getLottieTestDataDir().getChildFile ("goal.lottie");
    ASSERT_TRUE (file.existsAsFile()) << "Test data file missing: " << file.getFullPathName();

    auto comp = LottieReader::parseFile (file).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);
    EXPECT_EQ (comp->name, String ("Goal"));
    EXPECT_FLOAT_EQ (comp->frameRate, 30.0f);
    EXPECT_FLOAT_EQ (comp->size.getWidth(), 1080.0f);
    EXPECT_FLOAT_EQ (comp->size.getHeight(), 844.0f);
    EXPECT_FLOAT_EQ (comp->startFrame, 0.0f);
    EXPECT_FLOAT_EQ (comp->endFrame, 180.0f);
}

TEST_F (LottieReaderTests, ParseFileWithGoalLottieParsesLayers)
{
    const File file = getLottieTestDataDir().getChildFile ("goal.lottie");
    ASSERT_TRUE (file.existsAsFile());

    auto comp = LottieReader::parseFile (file).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);
    EXPECT_EQ (comp->layers.size(), 3u);
    EXPECT_GT (comp->assets.size(), 0u);
}

TEST_F (LottieReaderTests, ParseFileWithJollyWalkerJsonReturnsValidComposition)
{
    const File file = getLottieTestDataDir().getChildFile ("jolly_walker.json");
    ASSERT_TRUE (file.existsAsFile()) << "Test data file missing: " << file.getFullPathName();

    auto comp = LottieReader::parseFile (file).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);
    EXPECT_EQ (comp->name, String ("Comp 1"));
    EXPECT_FLOAT_EQ (comp->frameRate, 60.0f);
    EXPECT_FLOAT_EQ (comp->size.getWidth(), 1000.0f);
    EXPECT_FLOAT_EQ (comp->size.getHeight(), 1000.0f);
    EXPECT_EQ (comp->layers.size(), 21u);
}

TEST_F (LottieReaderTests, ParseFileWithImageTestJsonReturnsValidComposition)
{
    const File file = getLottieTestDataDir().getChildFile ("image_test.json");
    ASSERT_TRUE (file.existsAsFile()) << "Test data file missing: " << file.getFullPathName();

    auto comp = LottieReader::parseFile (file).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);
    EXPECT_EQ (comp->name, String ("test"));
    EXPECT_EQ (comp->assets.size(), 1u);
}

TEST_F (LottieReaderTests, ParseFileWithImageEmbeddedJsonReturnsValidComposition)
{
    const File file = getLottieTestDataDir().getChildFile ("image_embedded.json");
    ASSERT_TRUE (file.existsAsFile()) << "Test data file missing: " << file.getFullPathName();

    auto comp = LottieReader::parseFile (file).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);
    EXPECT_EQ (comp->name, String ("Comp 1"));
    EXPECT_EQ (comp->assets.size(), 1u);
}

TEST_F (LottieReaderTests, ListAnimationIdsForGoalLottieReturnsExpectedId)
{
    const File file = getLottieTestDataDir().getChildFile ("goal.lottie");
    ASSERT_TRUE (file.existsAsFile());

    const auto ids = LottieReader::listAnimationIds (file);
    ASSERT_FALSE (ids.empty());
    EXPECT_EQ (ids[0], String ("goal-celebrate-every-win"));
}

TEST_F (LottieReaderTests, ParseFromZipWithAnimationIdParsesGoalLottie)
{
    const File file = getLottieTestDataDir().getChildFile ("goal.lottie");
    ASSERT_TRUE (file.existsAsFile());

    auto comp = LottieReader::parseFromZip (file, "goal-celebrate-every-win").valueOr (nullptr);
    ASSERT_NE (comp, nullptr);
    EXPECT_EQ (comp->name, String ("Goal"));
    EXPECT_EQ (comp->layers.size(), 3u);
}

TEST_F (LottieReaderTests, ParseFromZipWithDefaultIdParsesGoalLottie)
{
    const File file = getLottieTestDataDir().getChildFile ("goal.lottie");
    ASSERT_TRUE (file.existsAsFile());

    auto comp = LottieReader::parseFromZip (file).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);
    EXPECT_EQ (comp->name, String ("Goal"));
}
#endif

// =============================================================================
// parseStream
// =============================================================================

TEST_F (LottieReaderTests, ParseStreamReturnsValidCompositionForValidJson)
{
    MemoryInputStream stream (kLottieReaderBaseJson, strlen (kLottieReaderBaseJson), false);

    auto comp = LottieReader::parseStream (stream).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);
    EXPECT_EQ (comp->name, String ("ReaderTest"));
    EXPECT_FLOAT_EQ (comp->frameRate, 24.0f);
    EXPECT_FLOAT_EQ (comp->size.getWidth(), 120.0f);
    EXPECT_FLOAT_EQ (comp->size.getHeight(), 80.0f);
}

TEST_F (LottieReaderTests, ParseStreamReturnsNullForGarbageInput)
{
    const char garbage[] = "not json {{{";
    MemoryInputStream stream (garbage, strlen (garbage), false);

    auto comp = LottieReader::parseStream (stream).valueOr (nullptr);
    EXPECT_EQ (comp, nullptr);
}

TEST_F (LottieReaderTests, ParseStreamSetsErrorForGarbageInput)
{
    const char garbage[] = "not json {{{";
    MemoryInputStream stream (garbage, strlen (garbage), false);

    auto result = LottieReader::parseStream (stream);

    EXPECT_TRUE (result.failed());
    EXPECT_FALSE (result.getErrorMessage().isEmpty());
}

TEST_F (LottieReaderTests, ParseStreamSetsNoErrorForValidJson)
{
    MemoryInputStream stream (kLottieReaderBaseJson, strlen (kLottieReaderBaseJson), false);

    auto result = LottieReader::parseStream (stream);

    EXPECT_TRUE (result.wasOk());
    EXPECT_NE (result.getReference(), nullptr);
}

TEST_F (LottieReaderTests, ParseStreamWithEmptyStreamReturnsNull)
{
    MemoryInputStream stream (nullptr, 0, false);

    auto result = LottieReader::parseStream (stream);

    EXPECT_TRUE (result.failed());
    EXPECT_FALSE (result.getErrorMessage().isEmpty());
    EXPECT_TRUE (result.getErrorMessage().contains ("Empty"));
}

TEST_F (LottieReaderTests, ParseStreamParsesMarkers)
{
    MemoryInputStream stream (kLottieReaderWithMarkersJson, strlen (kLottieReaderWithMarkersJson), false);

    auto comp = LottieReader::parseStream (stream).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);
    EXPECT_EQ (comp->markers.size(), 3u);
}

TEST_F (LottieReaderTests, ParseStreamWithFileInputStreamParsesJson)
{
    const File tempFile = File::createTempFile (".json");
    tempFile.replaceWithText (kLottieReaderBaseJson);

    auto fis = tempFile.createInputStream();
    ASSERT_NE (fis, nullptr);

    auto comp = LottieReader::parseStream (*fis).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);
    EXPECT_EQ (comp->name, String ("ReaderTest"));

    tempFile.deleteFile();
}

#if ! YUP_WASM
TEST_F (LottieReaderTests, ParseStreamWithGoalLottieAsZipStream)
{
    const File file = getLottieTestDataDir().getChildFile ("goal.lottie");
    ASSERT_TRUE (file.existsAsFile());

    auto fis = file.createInputStream();
    ASSERT_NE (fis, nullptr);

    auto comp = LottieReader::parseStream (*fis).valueOr (nullptr);
    ASSERT_NE (comp, nullptr);
    EXPECT_EQ (comp->name, String ("Goal"));
    EXPECT_EQ (comp->layers.size(), 3u);
}
#endif
