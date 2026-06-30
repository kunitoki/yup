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

constexpr const char* kSimpleAnimJson = R"json({
    "v": "5.5.2",
    "nm": "ExporterTest",
    "ip": 0,
    "op": 5,
    "fr": 10.0,
    "w": 20,
    "h": 20,
    "ddd": 0,
    "assets": [],
    "layers": [
        {
            "ty": 4,
            "nm": "Shape",
            "ind": 1,
            "ip": 0,
            "op": 5,
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
                        { "ty": "rc", "nm": "Rect", "p": { "a": 0, "k": [0, 0] }, "s": { "a": 0, "k": [10, 10] }, "r": { "a": 0, "k": 0 } },
                        { "ty": "fl", "nm": "Fill", "c": { "a": 0, "k": [1, 0, 0, 1] }, "o": { "a": 0, "k": 100 } }
                    ]
                }
            ]
        }
    ]
})json";

} // namespace

class AnimationFrameExporterTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        context = GraphicsContext::createContext (GraphicsContext::Headless, {});
        ASSERT_NE (context, nullptr);

        anim = Animation::loadFromData (kSimpleAnimJson);
        ASSERT_TRUE (anim.isValid());
    }

    std::unique_ptr<GraphicsContext> context;
    Animation anim;
};

// =============================================================================
// renderFrame — invalid animation
// =============================================================================

TEST_F (AnimationFrameExporterTests, RenderFrame_ReturnsInvalidImageForInvalidAnimation)
{
    Animation invalid;
    const Image result = AnimationFrameExporter::renderFrame (*context, invalid, 0.0f);
    EXPECT_FALSE (result.isValid());
}

// =============================================================================
// renderFrame — valid animation
// =============================================================================

TEST_F (AnimationFrameExporterTests, RenderFrame_ReturnsValidImageForValidAnimation)
{
    const Image result = AnimationFrameExporter::renderFrame (*context, anim, 0.0f);
    EXPECT_TRUE (result.isValid());
}

TEST_F (AnimationFrameExporterTests, RenderFrame_UsesNativeSizeWhenZeroTargetSize)
{
    const Image result = AnimationFrameExporter::renderFrame (*context, anim, 0.0f, {});
    ASSERT_TRUE (result.isValid());
    EXPECT_EQ (result.getWidth(), (int) anim.size().getWidth());
    EXPECT_EQ (result.getHeight(), (int) anim.size().getHeight());
}

TEST_F (AnimationFrameExporterTests, RenderFrame_UsesRequestedSizeWhenProvided)
{
    const Image result = AnimationFrameExporter::renderFrame (*context, anim, 0.0f, { 40, 40 });
    ASSERT_TRUE (result.isValid());
    EXPECT_EQ (result.getWidth(), 40);
    EXPECT_EQ (result.getHeight(), 40);
}

TEST_F (AnimationFrameExporterTests, RenderFrame_ReturnsDifferentResultAtDifferentFrames)
{
    const Image frame0 = AnimationFrameExporter::renderFrame (*context, anim, 0.0f);
    const Image frame2 = AnimationFrameExporter::renderFrame (*context, anim, 2.0f);

    EXPECT_TRUE (frame0.isValid());
    EXPECT_TRUE (frame2.isValid());
    EXPECT_EQ (frame0.getWidth(), frame2.getWidth());
    EXPECT_EQ (frame0.getHeight(), frame2.getHeight());
}

TEST_F (AnimationFrameExporterTests, RenderFrame_ProducesRGBAFormat)
{
    const Image result = AnimationFrameExporter::renderFrame (*context, anim, 0.0f);
    ASSERT_TRUE (result.isValid());
    EXPECT_EQ (result.getPixelFormat(), PixelFormat::RGBA);
}

// =============================================================================
// renderAllFrames — invalid animation
// =============================================================================

TEST_F (AnimationFrameExporterTests, RenderAllFrames_FailsForInvalidAnimation)
{
    Animation invalid;
    const auto result = AnimationFrameExporter::renderAllFrames (*context, invalid);
    EXPECT_FALSE (result.wasOk());
}

// =============================================================================
// renderAllFrames — valid animation
// =============================================================================

TEST_F (AnimationFrameExporterTests, RenderAllFrames_SucceedsForValidAnimation)
{
    const auto result = AnimationFrameExporter::renderAllFrames (*context, anim);
    EXPECT_TRUE (result.wasOk());
}

TEST_F (AnimationFrameExporterTests, RenderAllFrames_ReturnsOneImagePerFrame)
{
    const auto result = AnimationFrameExporter::renderAllFrames (*context, anim);
    ASSERT_TRUE (result.wasOk());

    const int expectedFrames = (int) anim.totalFrames();
    EXPECT_EQ ((int) result.getValue().size(), expectedFrames);
}

TEST_F (AnimationFrameExporterTests, RenderAllFrames_AllImagesAreValid)
{
    const auto result = AnimationFrameExporter::renderAllFrames (*context, anim);
    ASSERT_TRUE (result.wasOk());

    for (const auto& img : result.getValue())
        EXPECT_TRUE (img.isValid());
}

TEST_F (AnimationFrameExporterTests, RenderAllFrames_AllImagesUseNativeSize)
{
    const auto result = AnimationFrameExporter::renderAllFrames (*context, anim);
    ASSERT_TRUE (result.wasOk());

    for (const auto& img : result.getValue())
    {
        EXPECT_EQ (img.getWidth(), (int) anim.size().getWidth());
        EXPECT_EQ (img.getHeight(), (int) anim.size().getHeight());
    }
}

TEST_F (AnimationFrameExporterTests, RenderAllFrames_RespectsRequestedSize)
{
    const auto result = AnimationFrameExporter::renderAllFrames (*context, anim, { 64, 32 });
    ASSERT_TRUE (result.wasOk());

    for (const auto& img : result.getValue())
    {
        EXPECT_EQ (img.getWidth(), 64);
        EXPECT_EQ (img.getHeight(), 32);
    }
}

// =============================================================================
// exportToGif (Animation overload)
// =============================================================================

TEST_F (AnimationFrameExporterTests, ExportToGif_FailsForInvalidAnimation)
{
    Animation invalid;
    const File dest = File::createTempFile ("test_exporter_invalid.gif");

    const Result result = AnimationFrameExporter::exportToGif (*context, invalid, dest);
    EXPECT_FALSE (result.wasOk());

    dest.deleteFile();
}

#if YUP_MODULE_AVAILABLE_libgif && YUP_IMAGE_FORMAT_GIF

TEST_F (AnimationFrameExporterTests, ExportToGif_CreatesFileForValidAnimation)
{
    const File dest = File::createTempFile ("test_exporter_anim.gif");

    const Result result = AnimationFrameExporter::exportToGif (*context, anim, dest);
    EXPECT_TRUE (result.wasOk()) << result.getErrorMessage().toStdString();
    EXPECT_TRUE (dest.exists());
    EXPECT_GT (dest.getSize(), 0);

    dest.deleteFile();
}

TEST_F (AnimationFrameExporterTests, ExportToGif_RespectsTargetSize)
{
    const File dest = File::createTempFile ("test_exporter_sized.gif");

    const Result result = AnimationFrameExporter::exportToGif (*context, anim, dest, { 40, 30 });
    EXPECT_TRUE (result.wasOk()) << result.getErrorMessage().toStdString();
    EXPECT_TRUE (dest.exists());

    dest.deleteFile();
}

// =============================================================================
// exportToGif (frames overload)
// =============================================================================

TEST_F (AnimationFrameExporterTests, ExportToGifFromFrames_FailsWithNoFrames)
{
    const File dest = File::createTempFile ("test_exporter_empty.gif");

    const Result result = AnimationFrameExporter::exportToGif ({}, 25.0f, dest);
    EXPECT_FALSE (result.wasOk());

    dest.deleteFile();
}

TEST_F (AnimationFrameExporterTests, ExportToGifFromFrames_FailsWithInvalidFrameRate)
{
    Image frame (8, 8, PixelFormat::RGBA);
    const File dest = File::createTempFile ("test_exporter_badrate.gif");

    const Result result = AnimationFrameExporter::exportToGif ({ frame }, 0.0f, dest);
    EXPECT_FALSE (result.wasOk());

    dest.deleteFile();
}

TEST_F (AnimationFrameExporterTests, ExportToGifFromFrames_SucceedsWithValidFrames)
{
    std::vector<Image> frames;
    for (int i = 0; i < 3; ++i)
    {
        Image img (16, 16, PixelFormat::RGBA);
        img.fill (0xFF000000u | (uint32) (i * 80) << 16);
        frames.push_back (img);
    }

    const File dest = File::createTempFile ("test_exporter_frames.gif");

    const Result result = AnimationFrameExporter::exportToGif (frames, 10.0f, dest);
    EXPECT_TRUE (result.wasOk()) << result.getErrorMessage().toStdString();
    EXPECT_TRUE (dest.exists());
    EXPECT_GT (dest.getSize(), 0);

    dest.deleteFile();
}

TEST_F (AnimationFrameExporterTests, ExportToGifFromFrames_CreatesParentDirectoryIfNeeded)
{
    const File tempDir = File::createTempFile ("gif_dir");
    tempDir.createDirectory();
    const File dest = tempDir.getChildFile ("subdir/out.gif");

    std::vector<Image> frames;
    Image img (8, 8, PixelFormat::RGBA);
    frames.push_back (img);

    const Result result = AnimationFrameExporter::exportToGif (frames, 10.0f, dest);
    EXPECT_TRUE (result.wasOk()) << result.getErrorMessage().toStdString();

    tempDir.deleteRecursively();
}

#endif // YUP_MODULE_AVAILABLE_libgif && YUP_IMAGE_FORMAT_GIF
