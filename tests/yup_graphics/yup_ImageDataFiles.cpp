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

#if ! YUP_WASM

#include "yup_ImageFormatTools.h"

using namespace yup;

namespace
{

constexpr uint32 kMagentaARGB = 0xFFFF00FFu;

auto imagesDir()
{
    return getTestDataImagesDirectory();
}

bool allPixelsMatch (const Image& image, uint32 expected, int tolerance = 0)
{
    for (int y = 0; y < image.getHeight(); ++y)
    {
        for (int x = 0; x < image.getWidth(); ++x)
        {
            const uint32 pixel = image.getPixel (x, y);

            if (std::abs (int ((pixel >> 16) & 0xFF) - int ((expected >> 16) & 0xFF)) > tolerance)
                return false;
            if (std::abs (int ((pixel >> 8) & 0xFF) - int ((expected >> 8) & 0xFF)) > tolerance)
                return false;
            if (std::abs (int ((pixel >> 0) & 0xFF) - int ((expected >> 0) & 0xFF)) > tolerance)
                return false;
            if (std::abs (int ((pixel >> 24) & 0xFF) - int ((expected >> 24) & 0xFF)) > tolerance)
                return false;
        }
    }

    return true;
}

} // namespace

// ======================================================================
// Magenta image tests — verify all magenta.* files contain magenta pixels
// ======================================================================

TEST (ImageDataFilesTests, MagentaBmp)
{
    const auto file = imagesDir().getChildFile ("magenta.bmp");
    ASSERT_TRUE (file.existsAsFile());

    auto image = readImageFromFile (file);
    ASSERT_TRUE (image.isValid());
    EXPECT_GT (image.getWidth(), 0);
    EXPECT_GT (image.getHeight(), 0);
    EXPECT_TRUE (allPixelsMatch (image, kMagentaARGB, 2));
}

TEST (ImageDataFilesTests, MagentaPpm)
{
    const auto file = imagesDir().getChildFile ("magenta.ppm");
    ASSERT_TRUE (file.existsAsFile());

    auto image = readImageFromFile (file);
    ASSERT_TRUE (image.isValid());
    EXPECT_GT (image.getWidth(), 0);
    EXPECT_GT (image.getHeight(), 0);
    EXPECT_TRUE (allPixelsMatch (image, kMagentaARGB, 2));
}

#if YUP_IMAGE_FORMAT_JPEG
TEST (ImageDataFilesTests, MagentaJpg)
{
    const auto file = imagesDir().getChildFile ("magenta.jpg");
    ASSERT_TRUE (file.existsAsFile());

    auto image = readImageFromFile (file);
    ASSERT_TRUE (image.isValid());
    EXPECT_GT (image.getWidth(), 0);
    EXPECT_GT (image.getHeight(), 0);
    // JPEG is lossy; allow per-channel tolerance
    EXPECT_TRUE (allPixelsMatch (image, kMagentaARGB, 20));
}
#endif

#if YUP_IMAGE_FORMAT_GIF
TEST (ImageDataFilesTests, MagentaGif)
{
    const auto file = imagesDir().getChildFile ("magenta.gif");
    ASSERT_TRUE (file.existsAsFile());

    auto image = readImageFromFile (file);
    ASSERT_TRUE (image.isValid());
    EXPECT_GT (image.getWidth(), 0);
    EXPECT_GT (image.getHeight(), 0);
    // GIF is palette-based; allow per-channel tolerance
    EXPECT_TRUE (allPixelsMatch (image, kMagentaARGB, 8));
}
#endif

#if YUP_IMAGE_FORMAT_WEBP
TEST (ImageDataFilesTests, MagentaWebP)
{
    const auto file = imagesDir().getChildFile ("magenta.webp");
    ASSERT_TRUE (file.existsAsFile());

    auto image = readImageFromFile (file);
    ASSERT_TRUE (image.isValid());
    EXPECT_GT (image.getWidth(), 0);
    EXPECT_GT (image.getHeight(), 0);
    EXPECT_TRUE (allPixelsMatch (image, kMagentaARGB, 5));
}
#endif

// ======================================================================
// Grayscale PNG tests
// ======================================================================

#if YUP_IMAGE_FORMAT_PNG
TEST (ImageDataFilesTests, Gray1Png)
{
    const auto file = imagesDir().getChildFile ("gray-1.png");
    ASSERT_TRUE (file.existsAsFile());

    auto image = readImageFromFile (file);
    ASSERT_TRUE (image.isValid());
    EXPECT_GT (image.getWidth(), 0);
    EXPECT_GT (image.getHeight(), 0);
    EXPECT_EQ (image.getPixelFormat(), PixelFormat::Grayscale);
}

TEST (ImageDataFilesTests, Gray2Png)
{
    const auto file = imagesDir().getChildFile ("gray-2.png");
    ASSERT_TRUE (file.existsAsFile());

    auto image = readImageFromFile (file);
    ASSERT_TRUE (image.isValid());
    EXPECT_GT (image.getWidth(), 0);
    EXPECT_GT (image.getHeight(), 0);
    EXPECT_EQ (image.getPixelFormat(), PixelFormat::Grayscale);
}

TEST (ImageDataFilesTests, Gray4Png)
{
    const auto file = imagesDir().getChildFile ("gray-4.png");
    ASSERT_TRUE (file.existsAsFile());

    auto image = readImageFromFile (file);
    ASSERT_TRUE (image.isValid());
    EXPECT_GT (image.getWidth(), 0);
    EXPECT_GT (image.getHeight(), 0);
    EXPECT_EQ (image.getPixelFormat(), PixelFormat::Grayscale);
}

TEST (ImageDataFilesTests, Gray8Png)
{
    const auto file = imagesDir().getChildFile ("gray-8.png");
    ASSERT_TRUE (file.existsAsFile());

    auto image = readImageFromFile (file);
    ASSERT_TRUE (image.isValid());
    EXPECT_GT (image.getWidth(), 0);
    EXPECT_GT (image.getHeight(), 0);
    EXPECT_EQ (image.getPixelFormat(), PixelFormat::Grayscale);
}

TEST (ImageDataFilesTests, Gray16Png)
{
    const auto file = imagesDir().getChildFile ("gray-16.png");
    ASSERT_TRUE (file.existsAsFile());

    auto image = readImageFromFile (file);
    ASSERT_TRUE (image.isValid());
    EXPECT_GT (image.getWidth(), 0);
    EXPECT_GT (image.getHeight(), 0);
    EXPECT_EQ (image.getPixelFormat(), PixelFormat::Grayscale);
}

// ======================================================================
// Grayscale + Alpha PNG tests (decode to RGBA since YUP has no grayscale-alpha format)
// ======================================================================

TEST (ImageDataFilesTests, GrayAlpha8Png)
{
    const auto file = imagesDir().getChildFile ("gray-alpha-8.png");
    ASSERT_TRUE (file.existsAsFile());

    auto image = readImageFromFile (file);
    ASSERT_TRUE (image.isValid());
    EXPECT_GT (image.getWidth(), 0);
    EXPECT_GT (image.getHeight(), 0);
    // Two channels (gray + alpha) expand to RGBA
    EXPECT_EQ (image.getPixelFormat(), PixelFormat::RGBA);
}

TEST (ImageDataFilesTests, GrayAlpha16Png)
{
    const auto file = imagesDir().getChildFile ("gray-alpha-16.png");
    ASSERT_TRUE (file.existsAsFile());

    auto image = readImageFromFile (file);
    ASSERT_TRUE (image.isValid());
    EXPECT_GT (image.getWidth(), 0);
    EXPECT_GT (image.getHeight(), 0);
    EXPECT_EQ (image.getPixelFormat(), PixelFormat::RGBA);
}

// ======================================================================
// RGB PNG tests
// ======================================================================

TEST (ImageDataFilesTests, Rgb8Png)
{
    const auto file = imagesDir().getChildFile ("rgb-8.png");
    ASSERT_TRUE (file.existsAsFile());

    auto image = readImageFromFile (file);
    ASSERT_TRUE (image.isValid());
    EXPECT_GT (image.getWidth(), 0);
    EXPECT_GT (image.getHeight(), 0);
    EXPECT_EQ (image.getPixelFormat(), PixelFormat::RGB);
}

TEST (ImageDataFilesTests, Rgb16Png)
{
    const auto file = imagesDir().getChildFile ("rgb-16.png");
    ASSERT_TRUE (file.existsAsFile());

    auto image = readImageFromFile (file);
    ASSERT_TRUE (image.isValid());
    EXPECT_GT (image.getWidth(), 0);
    EXPECT_GT (image.getHeight(), 0);
    EXPECT_EQ (image.getPixelFormat(), PixelFormat::RGB);
}

// ======================================================================
// RGBA PNG tests
// ======================================================================

TEST (ImageDataFilesTests, RgbAlpha8Png)
{
    const auto file = imagesDir().getChildFile ("rgb-alpha-8.png");
    ASSERT_TRUE (file.existsAsFile());

    auto image = readImageFromFile (file);
    ASSERT_TRUE (image.isValid());
    EXPECT_GT (image.getWidth(), 0);
    EXPECT_GT (image.getHeight(), 0);
    EXPECT_EQ (image.getPixelFormat(), PixelFormat::RGBA);
}

TEST (ImageDataFilesTests, RgbAlpha16Png)
{
    const auto file = imagesDir().getChildFile ("rgb-alpha-16.png");
    ASSERT_TRUE (file.existsAsFile());

    auto image = readImageFromFile (file);
    ASSERT_TRUE (image.isValid());
    EXPECT_GT (image.getWidth(), 0);
    EXPECT_GT (image.getHeight(), 0);
    EXPECT_EQ (image.getPixelFormat(), PixelFormat::RGBA);
}
#endif // YUP_IMAGE_FORMAT_PNG

// ======================================================================
// file_example.* — generic examples, just verify they load successfully
// ======================================================================

#if YUP_IMAGE_FORMAT_GIF
TEST (ImageDataFilesTests, FileExampleGif)
{
    const auto file = imagesDir().getChildFile ("file_example.gif");
    ASSERT_TRUE (file.existsAsFile());

    auto image = readImageFromFile (file);
    ASSERT_TRUE (image.isValid());
    EXPECT_GT (image.getWidth(), 0);
    EXPECT_GT (image.getHeight(), 0);
}
#endif

#if YUP_IMAGE_FORMAT_JPEG
TEST (ImageDataFilesTests, FileExampleJpg)
{
    const auto file = imagesDir().getChildFile ("file_example.jpg");
    ASSERT_TRUE (file.existsAsFile());

    auto image = readImageFromFile (file);
    ASSERT_TRUE (image.isValid());
    EXPECT_GT (image.getWidth(), 0);
    EXPECT_GT (image.getHeight(), 0);
}
#endif

#if YUP_IMAGE_FORMAT_PNG
TEST (ImageDataFilesTests, FileExamplePng)
{
    const auto file = imagesDir().getChildFile ("file_example.png");
    ASSERT_TRUE (file.existsAsFile());

    auto image = readImageFromFile (file);
    ASSERT_TRUE (image.isValid());
    EXPECT_GT (image.getWidth(), 0);
    EXPECT_GT (image.getHeight(), 0);
}
#endif

#if YUP_IMAGE_FORMAT_WEBP
TEST (ImageDataFilesTests, FileExampleWebP)
{
    const auto file = imagesDir().getChildFile ("file_example.webp");
    ASSERT_TRUE (file.existsAsFile());

    auto image = readImageFromFile (file);
    ASSERT_TRUE (image.isValid());
    EXPECT_GT (image.getWidth(), 0);
    EXPECT_GT (image.getHeight(), 0);
}
#endif

// ======================================================================
// Animated GIF test
// ======================================================================

#if YUP_IMAGE_FORMAT_GIF
TEST (ImageDataFilesTests, AnimationGifIsAnimated)
{
    const auto file = imagesDir().getChildFile ("animation.gif");
    ASSERT_TRUE (file.existsAsFile());

    auto* fis = file.createInputStream().release();
    ASSERT_NE (fis, nullptr);

    GifImageFormatReader reader (fis);
    EXPECT_TRUE (reader.isAnimated());
    EXPECT_GT (reader.getFrameCount(), 1);

    for (int i = 0; i < reader.getFrameCount(); ++i)
    {
        auto frame = reader.readFrame (i);
        EXPECT_TRUE (frame.isValid()) << "Frame " << i << " is invalid";
        EXPECT_GT (frame.getWidth(), 0);
        EXPECT_GT (frame.getHeight(), 0);
    }
}
#endif

// ======================================================================
// Non-existent file
// ======================================================================

TEST (ImageDataFilesTests, ReadNonExistentFileReturnsInvalid)
{
    const auto file = imagesDir().getChildFile ("does_not_exist.png");
    EXPECT_FALSE (file.existsAsFile());

    auto image = readImageFromFile (file);
    EXPECT_FALSE (image.isValid());
}

#endif // !YUP_WASM
