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

#include <yup_graphics/yup_graphics.h>

using namespace yup;

// ======================================================================
// Writer format name tests
// ======================================================================

TEST (ImageFormatWriterTests, PpmWriterHasCorrectFormatName)
{
    PpmImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB);
    EXPECT_EQ (writer.getFormatName(), String ("PPM/PGM/PBM Image"));
}

TEST (ImageFormatWriterTests, BmpWriterHasCorrectFormatName)
{
    BmpImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB);
    EXPECT_EQ (writer.getFormatName(), String ("BMP Image"));
}

#if YUP_IMAGE_FORMAT_PNG
TEST (ImageFormatWriterTests, PngWriterHasCorrectFormatName)
{
    PngImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB);
    EXPECT_EQ (writer.getFormatName(), String ("PNG Image"));
}
#endif

#if YUP_IMAGE_FORMAT_JPEG
TEST (ImageFormatWriterTests, JpegWriterHasCorrectFormatName)
{
    JpegImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB, 0);
    EXPECT_EQ (writer.getFormatName(), String ("JPEG Image"));
}
#endif

#if YUP_IMAGE_FORMAT_WEBP
TEST (ImageFormatWriterTests, WebPWriterHasCorrectFormatName)
{
    WebPImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB, 0);
    EXPECT_EQ (writer.getFormatName(), String ("WebP Image"));
}
#endif

#if YUP_IMAGE_FORMAT_GIF
TEST (ImageFormatWriterTests, GifWriterHasCorrectFormatName)
{
    GifImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGBA);
    EXPECT_EQ (writer.getFormatName(), String ("GIF Image"));
}
#endif

// ======================================================================
// Writer flush tests
// ======================================================================

TEST (ImageFormatWriterTests, PpmWriterFlushReturnsTrue)
{
    PpmImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB);
    EXPECT_TRUE (writer.flush());
}

TEST (ImageFormatWriterTests, BmpWriterFlushReturnsTrue)
{
    BmpImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB);
    EXPECT_TRUE (writer.flush());
}

#if YUP_IMAGE_FORMAT_PNG
TEST (ImageFormatWriterTests, PngWriterFlushReturnsTrue)
{
    PngImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB);
    EXPECT_TRUE (writer.flush());
}
#endif

#if YUP_IMAGE_FORMAT_JPEG
TEST (ImageFormatWriterTests, JpegWriterFlushReturnsTrue)
{
    JpegImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB, 0);
    EXPECT_TRUE (writer.flush());
}
#endif

#if YUP_IMAGE_FORMAT_WEBP
TEST (ImageFormatWriterTests, WebPWriterFlushReturnsTrue)
{
    WebPImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB, 0);
    EXPECT_TRUE (writer.flush());
}
#endif

#if YUP_IMAGE_FORMAT_GIF
TEST (ImageFormatWriterTests, GifWriterFlushReturnsTrue)
{
    GifImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGBA);
    EXPECT_TRUE (writer.flush());
}

TEST (ImageFormatWriterTests, GifWriterSupportsAnimation)
{
    GifImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGBA);
    EXPECT_TRUE (writer.supportsAnimation());
}
#endif

// ======================================================================
// Writer pixel format tests
// ======================================================================

TEST (ImageFormatWriterTests, PpmWriterReturnsCorrectPixelFormat)
{
    {
        PpmImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB);
        EXPECT_EQ (writer.getPixelFormat(), PixelFormat::RGB);
    }
    {
        PpmImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::Grayscale);
        EXPECT_EQ (writer.getPixelFormat(), PixelFormat::Grayscale);
    }
}

TEST (ImageFormatWriterTests, BmpWriterReturnsCorrectPixelFormat)
{
    {
        BmpImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB);
        EXPECT_EQ (writer.getPixelFormat(), PixelFormat::RGB);
    }
    {
        BmpImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGBA);
        EXPECT_EQ (writer.getPixelFormat(), PixelFormat::RGBA);
    }
}

// ======================================================================
// Writer writeImage tests
// ======================================================================

TEST (ImageFormatWriterTests, BmpWriterWriteImageSucceedsForValidRgbImage)
{
    auto* rawStream = new MemoryOutputStream();
    BmpImageFormatWriter writer (rawStream, PixelFormat::RGB);

    Image source (4, 4, PixelFormat::RGB);
    source.fill (0xFFFF0000u);

    EXPECT_TRUE (writer.writeImage (source));
    EXPECT_GT (rawStream->getDataSize(), 0u);
}

TEST (ImageFormatWriterTests, PpmWriterWriteImageSucceedsForValidRgbImage)
{
    auto* rawStream = new MemoryOutputStream();
    PpmImageFormatWriter writer (rawStream, PixelFormat::RGB);

    Image source (4, 4, PixelFormat::RGB);
    source.fill (0xFFFF0000u);

    EXPECT_TRUE (writer.writeImage (source));
    EXPECT_GT (rawStream->getDataSize(), 0u);
}
