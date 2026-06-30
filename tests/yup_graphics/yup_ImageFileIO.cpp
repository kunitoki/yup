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

#include "yup_ImageFormatTools.h"

// ======================================================================
// Tests that write test images to tests/data/images/ and load them back
// ======================================================================

TEST (ImageFileIOTests, GenerateAndLoadBmpFromTestDataDir)
{
    const auto imgFile = ensureTestImage ("test_rgb.bmp", 16, 16, PixelFormat::RGB, 0xFF336699u);

    ASSERT_TRUE (imgFile.existsAsFile());

    auto loaded = readImageFromFile (imgFile);
    ASSERT_TRUE (loaded.isValid());
    EXPECT_EQ (loaded.getWidth(), 16);
    EXPECT_EQ (loaded.getHeight(), 16);
    EXPECT_EQ (loaded.getPixel (0, 0), 0xFF336699u);
    EXPECT_EQ (loaded.getPixel (15, 15), 0xFF336699u);
}

TEST (ImageFileIOTests, GenerateAndLoadPpmFromTestDataDir)
{
    const auto imgFile = ensureTestImage ("test_rgb.ppm", 12, 8, PixelFormat::RGB, 0xFFAA7733u);

    ASSERT_TRUE (imgFile.existsAsFile());

    auto loaded = readImageFromFile (imgFile);
    ASSERT_TRUE (loaded.isValid());
    EXPECT_EQ (loaded.getWidth(), 12);
    EXPECT_EQ (loaded.getHeight(), 8);
    EXPECT_EQ (loaded.getPixel (0, 0), 0xFFAA7733u);
}

TEST (ImageFileIOTests, GenerateAndLoadGrayscalePgmFromTestDataDir)
{
    const auto imgFile = ensureTestImage ("test_gray.pgm", 8, 8, PixelFormat::Grayscale, 0xFF888888u);

    ASSERT_TRUE (imgFile.existsAsFile());

    auto loaded = readImageFromFile (imgFile);
    ASSERT_TRUE (loaded.isValid());
    EXPECT_EQ (loaded.getWidth(), 8);
    EXPECT_EQ (loaded.getHeight(), 8);
}

#if YUP_IMAGE_FORMAT_PNG
TEST (ImageFileIOTests, GenerateAndLoadPngFromTestDataDir)
{
    const auto imgFile = ensureTestImage ("test_rgba.png", 16, 16, PixelFormat::RGBA, 0xCC4488AAu);

    ASSERT_TRUE (imgFile.existsAsFile());

    auto loaded = readImageFromFile (imgFile);
    ASSERT_TRUE (loaded.isValid());
    EXPECT_EQ (loaded.getWidth(), 16);
    EXPECT_EQ (loaded.getHeight(), 16);
    EXPECT_EQ (loaded.getPixelFormat(), PixelFormat::RGBA);
    EXPECT_EQ (loaded.getPixel (0, 0), 0xCC4488AAu);
}
#endif

#if YUP_IMAGE_FORMAT_JPEG
TEST (ImageFileIOTests, GenerateAndLoadJpegFromTestDataDir)
{
    const auto imgFile = ensureTestImage ("test_rgb.jpg", 16, 16, PixelFormat::RGB, 0xFF226688u);

    ASSERT_TRUE (imgFile.existsAsFile());

    auto loaded = readImageFromFile (imgFile);
    ASSERT_TRUE (loaded.isValid());
    EXPECT_EQ (loaded.getWidth(), 16);
    EXPECT_EQ (loaded.getHeight(), 16);
    EXPECT_TRUE (imagesAreEqual (generateSolidImage (16, 16, PixelFormat::RGB, 0xFF226688u), loaded, 3));
}
#endif

#if YUP_IMAGE_FORMAT_WEBP
TEST (ImageFileIOTests, GenerateAndLoadWebPFromTestDataDir)
{
    const auto imgFile = ensureTestImage ("test_rgba.webp", 12, 12, PixelFormat::RGBA, 0x88AABBCCu);

    ASSERT_TRUE (imgFile.existsAsFile());

    auto loaded = readImageFromFile (imgFile);
    ASSERT_TRUE (loaded.isValid());
    EXPECT_EQ (loaded.getWidth(), 12);
    EXPECT_EQ (loaded.getHeight(), 12);
    EXPECT_EQ (loaded.getPixelFormat(), PixelFormat::RGBA);
    EXPECT_EQ (loaded.getPixel (0, 0), 0x88AABBCCu);
}
#endif

#if YUP_IMAGE_FORMAT_GIF
TEST (ImageFileIOTests, GenerateAndLoadGifFromTestDataDir)
{
    const auto imgFile = ensureTestImage ("test_rgba.gif", 12, 12, PixelFormat::RGBA, 0xCC66AA22u);

    ASSERT_TRUE (imgFile.existsAsFile());

    auto loaded = readImageFromFile (imgFile);
    ASSERT_TRUE (loaded.isValid());
    EXPECT_EQ (loaded.getWidth(), 12);
    EXPECT_EQ (loaded.getHeight(), 12);
    EXPECT_EQ (loaded.getPixelFormat(), PixelFormat::RGBA);
    EXPECT_TRUE (imagesAreEqual (generateSolidImage (12, 12, PixelFormat::RGBA, 0xCC66AA22u), loaded, 8));
}
#endif

// ======================================================================
// Roundtrip using writeImageToFile / readImageFromFile helpers
// ======================================================================

TEST (ImageFileIOTests, WriteAndReadBmpViaHelper)
{
    auto original = generateTestImage (14, 10, PixelFormat::RGB);
    auto tempFile = File::createTempFile (".bmp");

    ASSERT_TRUE (writeImageToFile (original, tempFile));

    auto loaded = readImageFromFile (tempFile);
    ASSERT_TRUE (loaded.isValid());
    EXPECT_TRUE (imagesAreEqual (original, loaded, 0));

    tempFile.deleteFile();
}

TEST (ImageFileIOTests, WriteAndReadPpmViaHelper)
{
    auto original = generateTestImage (10, 6, PixelFormat::RGB);
    auto tempFile = File::createTempFile (".ppm");

    ASSERT_TRUE (writeImageToFile (original, tempFile));

    auto loaded = readImageFromFile (tempFile);
    ASSERT_TRUE (loaded.isValid());
    EXPECT_TRUE (imagesAreEqual (original, loaded, 0));

    tempFile.deleteFile();
}

#if YUP_IMAGE_FORMAT_PNG
TEST (ImageFileIOTests, WriteAndReadPngViaHelper)
{
    auto original = generateTestImage (16, 12, PixelFormat::RGBA);
    auto tempFile = File::createTempFile (".png");

    ASSERT_TRUE (writeImageToFile (original, tempFile));

    auto loaded = readImageFromFile (tempFile);
    ASSERT_TRUE (loaded.isValid());
    EXPECT_TRUE (imagesAreEqual (original, loaded, 0));

    tempFile.deleteFile();
}
#endif

#if YUP_IMAGE_FORMAT_JPEG
TEST (ImageFileIOTests, WriteAndReadJpegViaHelper)
{
    Image original (12, 8, PixelFormat::RGB);
    original.fill (0xFF556677u);
    auto tempFile = File::createTempFile (".jpg");

    ASSERT_TRUE (writeImageToFile (original, tempFile));

    auto loaded = readImageFromFile (tempFile);
    ASSERT_TRUE (loaded.isValid());
    EXPECT_TRUE (imagesAreEqual (original, loaded, 3));

    tempFile.deleteFile();
}
#endif

#if YUP_IMAGE_FORMAT_WEBP
TEST (ImageFileIOTests, WriteAndReadWebPViaHelper)
{
    auto original = generateTestImage (12, 12, PixelFormat::RGBA);
    auto tempFile = File::createTempFile (".webp");

    ASSERT_TRUE (writeImageToFile (original, tempFile));

    auto loaded = readImageFromFile (tempFile);
    ASSERT_TRUE (loaded.isValid());
    EXPECT_TRUE (imagesAreEqual (original, loaded, 0));

    tempFile.deleteFile();
}
#endif

#if YUP_IMAGE_FORMAT_GIF
TEST (ImageFileIOTests, WriteAndReadGifViaHelper)
{
    Image original (10, 10, PixelFormat::RGBA);
    original.fill (0xFFDDAA33u);
    auto tempFile = File::createTempFile (".gif");

    ASSERT_TRUE (writeImageToFile (original, tempFile));

    auto loaded = readImageFromFile (tempFile);
    ASSERT_TRUE (loaded.isValid());
    EXPECT_TRUE (imagesAreEqual (original, loaded, 8));

    tempFile.deleteFile();
}
#endif

// ======================================================================
// Edge case - non-existent file
// ======================================================================

TEST (ImageFileIOTests, ReadImageFromNonExistentFileReturnsInvalid)
{
    auto result = readImageFromFile (File ("/nonexistent/path/image.bmp"));
    EXPECT_FALSE (result.isValid());
}

TEST (ImageFileIOTests, WriteImageToInvalidPathReturnsFalse)
{
    auto img = generateSolidImage (4, 4, PixelFormat::RGB);
    EXPECT_FALSE (writeImageToFile (img, File ("/nonexistent/path/image.bmp")));
}
