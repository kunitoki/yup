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

constexpr const char* kAnimTestJson = R"json({
    "v": "5.5.2",
    "nm": "AnimTest",
    "ip": 0,
    "op": 10,
    "fr": 25.0,
    "w": 100,
    "h": 80,
    "ddd": 0,
    "assets": [],
    "layers": [
        {
            "ty": 4,
            "nm": "Rect",
            "ind": 1,
            "ip": 0,
            "op": 10,
            "st": 0,
            "sr": 1,
            "hd": false,
            "bm": 0,
            "ks": {
                "a": { "a": 0, "k": [0, 0] },
                "p": { "a": 0, "k": [50, 40] },
                "s": { "a": 0, "k": [100, 100] },
                "r": { "a": 0, "k": 0 },
                "o": { "a": 0, "k": 100 }
            },
            "shapes": [
                {
                    "ty": "gr",
                    "nm": "Group",
                    "it": [
                        { "ty": "rc", "nm": "Rect", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [40, 30] }, "r": { "a": 0, "k": 0 } },
                        { "ty": "fl", "nm": "Fill", "c": { "a": 0, "k": [0, 0, 1, 1] }, "o": { "a": 0, "k": 100 } }
                    ]
                }
            ]
        }
    ]
})json";

} // namespace

// =============================================================================
// Default / invalid animation
// =============================================================================

TEST (AnimationTests, DefaultConstructedAnimationIsInvalid)
{
    Animation anim;
    EXPECT_FALSE (anim.isValid());
}

TEST (AnimationTests, DefaultConstructedCompositionIsNull)
{
    Animation anim;
    EXPECT_EQ (anim.getComposition(), nullptr);
}

TEST (AnimationTests, DefaultConstructedTotalFramesIsZero)
{
    Animation anim;
    EXPECT_EQ (anim.totalFrames(), 0.0f);
}

TEST (AnimationTests, DefaultConstructedFrameRateIsZero)
{
    Animation anim;
    EXPECT_EQ (anim.frameRate(), 0.0f);
}

TEST (AnimationTests, DefaultConstructedDurationIsZero)
{
    Animation anim;
    EXPECT_EQ (anim.duration(), 0.0f);
}

TEST (AnimationTests, DefaultConstructedSizeIsEmpty)
{
    Animation anim;
    const auto sz = anim.size();
    EXPECT_EQ (sz.getWidth(), 0.0f);
    EXPECT_EQ (sz.getHeight(), 0.0f);
}

TEST (AnimationTests, DefaultConstructedToJsonReturnsEmptyString)
{
    Animation anim;
    EXPECT_EQ (anim.toJson(), String());
}

TEST (AnimationTests, DefaultConstructedSaveToFileFailsWithMessage)
{
    Animation anim;
    const File dest = File::createTempFile ("anim_invalid.json");
    const Result result = anim.saveToFile (dest);
    EXPECT_FALSE (result.wasOk());
    dest.deleteFile();
}

// =============================================================================
// loadFromData
// =============================================================================

TEST (AnimationTests, LoadFromDataReturnsValidAnimationForValidJson)
{
    const auto anim = Animation::loadFromData (kAnimTestJson);
    EXPECT_TRUE (anim.isValid());
}

TEST (AnimationTests, LoadFromDataWithEmptyStringDoesNotCrash)
{
    // LottieReader is lenient: empty string produces a valid but empty composition.
    EXPECT_NO_THROW ({
        [[maybe_unused]] const auto anim = Animation::loadFromData ({});
    });
}

TEST (AnimationTests, LoadFromDataReturnsInvalidAnimationForGarbageInput)
{
    const auto anim = Animation::loadFromData ("not json at all {{{");
    EXPECT_FALSE (anim.isValid());
}

TEST (AnimationTests, LoadFromDataWithMinimalJsonDoesNotCrash)
{
    // LottieReader creates a default composition even for non-Lottie JSON.
    EXPECT_NO_THROW ({
        [[maybe_unused]] const auto anim = Animation::loadFromData (R"json({"key": "value"})json");
    });
}

// =============================================================================
// loadFromFile
// =============================================================================

TEST (AnimationTests, LoadFromFileReturnsInvalidAnimationForMissingFile)
{
    const auto anim = Animation::loadFromFile (File ("/nonexistent/path/anim_test_missing.json"));
    EXPECT_FALSE (anim.isValid());
}

TEST (AnimationTests, LoadFromFileReturnsValidAnimationForValidFile)
{
    const File tempFile = File::createTempFile (".json");
    tempFile.replaceWithText (kAnimTestJson);

    const auto anim = Animation::loadFromFile (tempFile);
    EXPECT_TRUE (anim.isValid());

    tempFile.deleteFile();
}

// =============================================================================
// fromComposition
// =============================================================================

TEST (AnimationTests, FromCompositionWithNullptrReturnsInvalid)
{
    const auto anim = Animation::fromComposition (nullptr);
    EXPECT_FALSE (anim.isValid());
}

// =============================================================================
// Properties on valid animation
// =============================================================================

class AnimationValidTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        anim = Animation::loadFromData (kAnimTestJson);
        ASSERT_TRUE (anim.isValid());
    }

    Animation anim;
};

TEST_F (AnimationValidTests, TotalFramesMatchesExpectedValue)
{
    // ip=0, op=10 → 10 total frames
    EXPECT_EQ (anim.totalFrames(), 10.0f);
}

TEST_F (AnimationValidTests, FrameRateMatchesExpectedValue)
{
    // fr=25.0
    EXPECT_EQ (anim.frameRate(), 25.0f);
}

TEST_F (AnimationValidTests, DurationMatchesExpectedValue)
{
    // duration = totalFrames / frameRate = 10 / 25 = 0.4s
    EXPECT_NEAR (anim.duration(), 0.4f, 1e-5f);
}

TEST_F (AnimationValidTests, SizeMatchesExpectedValue)
{
    const auto sz = anim.size();
    EXPECT_EQ (sz.getWidth(), 100.0f);
    EXPECT_EQ (sz.getHeight(), 80.0f);
}

TEST_F (AnimationValidTests, GetCompositionReturnsNonNull)
{
    EXPECT_NE (anim.getComposition(), nullptr);
}

// =============================================================================
// Render methods — null-safety on invalid animation
// =============================================================================

TEST (AnimationTests, RenderFrameOnInvalidAnimationDoesNotCrash)
{
    Animation invalid;
    auto context = GraphicsContext::createContext (GraphicsContext::Headless, {});
    ASSERT_NE (context, nullptr);

    Image canvas (32, 32, PixelFormat::RGBA);
    Graphics g (*context, canvas);

    EXPECT_NO_THROW (invalid.renderFrame (g, 0.0f, Rectangle<float> (0, 0, 32, 32)));
}

TEST (AnimationTests, RenderAtTimeOnInvalidAnimationDoesNotCrash)
{
    Animation invalid;
    auto context = GraphicsContext::createContext (GraphicsContext::Headless, {});
    ASSERT_NE (context, nullptr);

    Image canvas (32, 32, PixelFormat::RGBA);
    Graphics g (*context, canvas);

    EXPECT_NO_THROW (invalid.renderAtTime (g, 0.0f, Rectangle<float> (0, 0, 32, 32)));
}

TEST (AnimationTests, RenderAtProgressOnInvalidAnimationDoesNotCrash)
{
    Animation invalid;
    auto context = GraphicsContext::createContext (GraphicsContext::Headless, {});
    ASSERT_NE (context, nullptr);

    Image canvas (32, 32, PixelFormat::RGBA);
    Graphics g (*context, canvas);

    EXPECT_NO_THROW (invalid.renderAtProgress (g, 0.0f, Rectangle<float> (0, 0, 32, 32)));
}

// =============================================================================
// Render methods — valid animation
// =============================================================================

class AnimationRenderTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        context = GraphicsContext::createContext (GraphicsContext::Headless, {});
        ASSERT_NE (context, nullptr);

        anim = Animation::loadFromData (kAnimTestJson);
        ASSERT_TRUE (anim.isValid());
    }

    std::unique_ptr<GraphicsContext> context;
    Animation anim;
};

TEST_F (AnimationRenderTests, RenderFrameDoesNotCrash)
{
    Image canvas (100, 80, PixelFormat::RGBA);
    Graphics g (*context, canvas);

    EXPECT_NO_THROW (anim.renderFrame (g, 0.0f, Rectangle<float> (0, 0, 100, 80)));
}

TEST_F (AnimationRenderTests, RenderAtTimeDoesNotCrash)
{
    Image canvas (100, 80, PixelFormat::RGBA);
    Graphics g (*context, canvas);

    EXPECT_NO_THROW (anim.renderAtTime (g, 0.1f, Rectangle<float> (0, 0, 100, 80)));
}

TEST_F (AnimationRenderTests, RenderAtProgressDoesNotCrash)
{
    Image canvas (100, 80, PixelFormat::RGBA);
    Graphics g (*context, canvas);

    EXPECT_NO_THROW (anim.renderAtProgress (g, 0.5f, Rectangle<float> (0, 0, 100, 80)));
}

TEST_F (AnimationRenderTests, RenderAtProgressZeroMatchesRenderAtTimeZero)
{
    Image canvas1 (100, 80, PixelFormat::RGBA);
    Image canvas2 (100, 80, PixelFormat::RGBA);

    {
        Graphics g (*context, canvas1);
        anim.renderAtProgress (g, 0.0f, Rectangle<float> (0, 0, 100, 80));
    }
    {
        Graphics g (*context, canvas2);
        anim.renderAtTime (g, 0.0f, Rectangle<float> (0, 0, 100, 80));
    }

    for (int y = 0; y < 80; ++y)
        for (int x = 0; x < 100; ++x)
            EXPECT_EQ (canvas1.getPixel (x, y), canvas2.getPixel (x, y));
}

// =============================================================================
// Serialization
// =============================================================================

TEST_F (AnimationValidTests, ToJsonReturnsNonEmptyString)
{
    const String json = anim.toJson();
    EXPECT_FALSE (json.isEmpty());
}

TEST_F (AnimationValidTests, ToJsonContainsExpectedFields)
{
    const String json = anim.toJson();
    EXPECT_TRUE (json.contains ("\"v\""));
    EXPECT_TRUE (json.contains ("\"layers\""));
    EXPECT_TRUE (json.contains ("\"fr\""));
}

TEST_F (AnimationValidTests, SaveToFileCreatesFile)
{
    const File dest = File::createTempFile ("anim_save.json");

    const Result result = anim.saveToFile (dest);
    EXPECT_TRUE (result.wasOk()) << result.getErrorMessage().toStdString();
    EXPECT_TRUE (dest.existsAsFile());
    EXPECT_GT (dest.getSize(), 0);

    dest.deleteFile();
}

TEST_F (AnimationValidTests, SavedFileCanBeReloadedAsValidAnimation)
{
    const File dest = File::createTempFile ("anim_reload.json");
    ASSERT_TRUE (anim.saveToFile (dest).wasOk());

    const auto reloaded = Animation::loadFromFile (dest);
    EXPECT_TRUE (reloaded.isValid());

    dest.deleteFile();
}
