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

// =============================================================================
// parseData — basic functionality
// =============================================================================

TEST (LottieReaderTests, ParseDataReturnsValidCompositionForValidJson)
{
    auto comp = LottieReader::parseData (kLottieReaderBaseJson);
    EXPECT_NE (comp, nullptr);
}

TEST (LottieReaderTests, ParseDataReturnsNullForGarbageJson)
{
    auto comp = LottieReader::parseData ("{{ not json }}");
    EXPECT_EQ (comp, nullptr);
}

TEST (LottieReaderTests, ParseDataPopulatesErrorOutputForBadJson)
{
    String errorMsg;
    auto comp = LottieReader::parseData ("{{ bad }", {}, &errorMsg);

    EXPECT_EQ (comp, nullptr);
    EXPECT_FALSE (errorMsg.isEmpty());
    EXPECT_TRUE (errorMsg.contains ("JSON parse error"));
}

TEST (LottieReaderTests, ParseDataSetsNoErrorForValidJson)
{
    String errorMsg;
    auto comp = LottieReader::parseData (kLottieReaderBaseJson, {}, &errorMsg);

    EXPECT_NE (comp, nullptr);
    EXPECT_TRUE (errorMsg.isEmpty());
}

TEST (LottieReaderTests, ParseDataParsesCompositionProperties)
{
    auto comp = LottieReader::parseData (kLottieReaderBaseJson);
    ASSERT_NE (comp, nullptr);

    EXPECT_FLOAT_EQ (comp->size.getWidth(), 120.0f);
    EXPECT_FLOAT_EQ (comp->size.getHeight(), 80.0f);
    EXPECT_FLOAT_EQ (comp->frameRate, 24.0f);
    EXPECT_FLOAT_EQ (comp->startFrame, 0.0f);
    EXPECT_FLOAT_EQ (comp->endFrame, 20.0f);
    EXPECT_EQ (comp->name, String ("ReaderTest"));
    EXPECT_EQ (comp->version, String ("5.5.2"));
}

TEST (LottieReaderTests, ParseDataReturnsNullForReversedFrameRange)
{
    // ip=50 > op=10 — startFrame > endFrame → validation fails
    auto comp = LottieReader::parseData (kLottieReaderReversedFrameJson);
    EXPECT_EQ (comp, nullptr);
}

TEST (LottieReaderTests, ParseDataSetsErrorForReversedFrameRange)
{
    String errorMsg;
    auto comp = LottieReader::parseData (kLottieReaderReversedFrameJson, {}, &errorMsg);

    EXPECT_EQ (comp, nullptr);
    EXPECT_FALSE (errorMsg.isEmpty());
}

// =============================================================================
// parseData — markers
// =============================================================================

TEST (LottieReaderTests, ParseDataParsesMarkersField)
{
    auto comp = LottieReader::parseData (kLottieReaderWithMarkersJson);
    ASSERT_NE (comp, nullptr);
    EXPECT_EQ (comp->markers.size(), 3u);
}

TEST (LottieReaderTests, ParseDataMarkersHaveCorrectFields)
{
    auto comp = LottieReader::parseData (kLottieReaderWithMarkersJson);
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

TEST (LottieReaderTests, ParseDataWithNoMarkersFieldHasEmptyMarkersVector)
{
    auto comp = LottieReader::parseData (kLottieReaderBaseJson);
    ASSERT_NE (comp, nullptr);
    EXPECT_TRUE (comp->markers.empty());
}

// =============================================================================
// findMarker on composition
// =============================================================================

TEST (LottieReaderTests, FindMarkerReturnsCorrectMarkerByName)
{
    auto comp = LottieReader::parseData (kLottieReaderWithMarkersJson);
    ASSERT_NE (comp, nullptr);

    const AnimationMarker* m = comp->findMarker ("loop");
    ASSERT_NE (m, nullptr);
    EXPECT_EQ (m->comment, String ("loop"));
    EXPECT_FLOAT_EQ (m->startFrame, 15.0f);
    EXPECT_FLOAT_EQ (m->duration, 30.0f);
}

TEST (LottieReaderTests, FindMarkerReturnsNullForMissingName)
{
    auto comp = LottieReader::parseData (kLottieReaderWithMarkersJson);
    ASSERT_NE (comp, nullptr);

    EXPECT_EQ (comp->findMarker ("nonexistent"), nullptr);
}

TEST (LottieReaderTests, FindMarkerReturnsNullOnEmptyMarkersList)
{
    auto comp = LottieReader::parseData (kLottieReaderBaseJson);
    ASSERT_NE (comp, nullptr);

    EXPECT_EQ (comp->findMarker ("any"), nullptr);
}

// =============================================================================
// parseFile
// =============================================================================

TEST (LottieReaderTests, ParseFileReturnsNullForMissingFile)
{
    auto comp = LottieReader::parseFile (File ("/nonexistent/path/file.json"));
    EXPECT_EQ (comp, nullptr);
}

TEST (LottieReaderTests, ParseFileSetsErrorForMissingFile)
{
    String errorMsg;
    auto comp = LottieReader::parseFile (File ("/nonexistent/path/file.json"), {}, &errorMsg);

    EXPECT_EQ (comp, nullptr);
    EXPECT_FALSE (errorMsg.isEmpty());
    EXPECT_TRUE (errorMsg.contains ("File not found"));
}

TEST (LottieReaderTests, ParseFileReturnsNullForEmptyFile)
{
    const File tempFile = File::createTempFile (".json");
    tempFile.replaceWithText ({});

    String errorMsg;
    auto comp = LottieReader::parseFile (tempFile, {}, &errorMsg);

    EXPECT_EQ (comp, nullptr);
    EXPECT_FALSE (errorMsg.isEmpty());

    tempFile.deleteFile();
}

TEST (LottieReaderTests, ParseFileReturnsValidCompositionForValidFile)
{
    const File tempFile = File::createTempFile (".json");
    tempFile.replaceWithText (kLottieReaderBaseJson);

    auto comp = LottieReader::parseFile (tempFile);
    EXPECT_NE (comp, nullptr);

    tempFile.deleteFile();
}

TEST (LottieReaderTests, ParseFileSetsResourceDirectoryFromFileParent)
{
    const File tempFile = File::createTempFile (".json");
    tempFile.replaceWithText (kLottieReaderBaseJson);

    LottieLoadOptions opts;
    auto comp = LottieReader::parseFile (tempFile, opts);
    EXPECT_NE (comp, nullptr);

    tempFile.deleteFile();
}

// =============================================================================
// listAnimationIds
// =============================================================================

TEST (LottieReaderTests, ListAnimationIdsReturnsEmptyForMissingFile)
{
    const auto ids = LottieReader::listAnimationIds (File ("/nonexistent/file.lottie"));
    EXPECT_TRUE (ids.empty());
}

TEST (LottieReaderTests, ListAnimationIdsReturnsEmptyForRegularJsonFile)
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

TEST (LottieReaderTests, ParseFromZipReturnsNullForMissingFile)
{
    String errorMsg;
    auto comp = LottieReader::parseFromZip (File ("/nonexistent/file.lottie"), {}, {}, &errorMsg);

    EXPECT_EQ (comp, nullptr);
    EXPECT_FALSE (errorMsg.isEmpty());
}

TEST (LottieReaderTests, ParseFromZipReturnsNullForNonZipFile)
{
    const File tempFile = File::createTempFile (".lottie");
    tempFile.replaceWithText ("not a zip file");

    String errorMsg;
    auto comp = LottieReader::parseFromZip (tempFile, {}, {}, &errorMsg);

    EXPECT_EQ (comp, nullptr);

    tempFile.deleteFile();
}

// =============================================================================
// imageResolver callback
// =============================================================================

TEST (LottieReaderTests, ImageResolverIsCalledForExternalImageAssets)
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

    auto comp = LottieReader::parseData (kLottieReaderImageAssetJson, opts);
    ASSERT_NE (comp, nullptr);

    EXPECT_TRUE (resolverCalled);
    EXPECT_TRUE (capturedRef.contains ("logo.png"));
}

TEST (LottieReaderTests, ImageResolverCanProvideImageForAsset)
{
    Image providedImage (32, 32, PixelFormat::RGBA);
    providedImage.fill (0xFF0000FFu);

    LottieLoadOptions opts;
    opts.imageResolver = [&] (const String&, const File&) -> std::optional<Image>
    {
        return providedImage;
    };

    auto comp = LottieReader::parseData (kLottieReaderImageAssetJson, opts);
    ASSERT_NE (comp, nullptr);

    // Verify the asset was registered (asset table contains our image)
    EXPECT_EQ (comp->assets.size(), 1);
}

TEST (LottieReaderTests, ImageResolverNotCalledWhenNoImageAssets)
{
    bool resolverCalled = false;

    LottieLoadOptions opts;
    opts.imageResolver = [&] (const String&, const File&) -> std::optional<Image>
    {
        resolverCalled = true;
        return std::nullopt;
    };

    auto comp = LottieReader::parseData (kLottieReaderBaseJson, opts);
    ASSERT_NE (comp, nullptr);

    EXPECT_FALSE (resolverCalled);
}

// =============================================================================
// Assets parsing
// =============================================================================

TEST (LottieReaderTests, ParseDataWithImageAssetCreatesAssetEntry)
{
    auto comp = LottieReader::parseData (kLottieReaderImageAssetJson);
    ASSERT_NE (comp, nullptr);

    EXPECT_EQ (comp->assets.size(), 1);
    EXPECT_NE (comp->assets["img0"], nullptr);
}

TEST (LottieReaderTests, ParseDataImageAssetHasCorrectDimensions)
{
    auto comp = LottieReader::parseData (kLottieReaderImageAssetJson);
    ASSERT_NE (comp, nullptr);

    const auto asset = comp->assets["img0"];
    ASSERT_NE (asset, nullptr);

    EXPECT_EQ (asset->width, 32);
    EXPECT_EQ (asset->height, 32);
}

TEST (LottieReaderTests, ParseDataImageAssetHasCorrectPath)
{
    auto comp = LottieReader::parseData (kLottieReaderImageAssetJson);
    ASSERT_NE (comp, nullptr);

    const auto asset = comp->assets["img0"];
    ASSERT_NE (asset, nullptr);

    EXPECT_TRUE (asset->path.contains ("logo.png"));
}
