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
// Reader format name tests
// ======================================================================

TEST (ImageFormatReaderTests, PpmReaderHasCorrectFormatName)
{
    const char data[] = "P6\n1 1\n255\n\xFF\x00\x00";
    auto* stream = new MemoryInputStream (data, sizeof (data) - 1, false);

    PpmImageFormatReader reader (stream);

    EXPECT_EQ (reader.getFormatName(), String ("PPM/PGM/PBM Image"));
}

TEST (ImageFormatReaderTests, BmpReaderHasCorrectFormatName)
{
    Image source (1, 1, PixelFormat::RGB);
    source.fill (0xFFFF0000u);

    auto* rawStream = new MemoryOutputStream();
    BmpImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (source));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    BmpImageFormatReader reader (inStream);

    EXPECT_EQ (reader.getFormatName(), String ("BMP Image"));
}

#if YUP_IMAGE_FORMAT_PNG
TEST (ImageFormatReaderTests, PngReaderHasCorrectFormatName)
{
    auto* rawStream = new MemoryOutputStream();
    PngImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (1, 1, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    PngImageFormatReader reader (inStream);

    EXPECT_EQ (reader.getFormatName(), String ("PNG Image"));
}
#endif

#if YUP_IMAGE_FORMAT_JPEG
TEST (ImageFormatReaderTests, JpegReaderHasCorrectFormatName)
{
    auto* rawStream = new MemoryOutputStream();
    JpegImageFormatWriter writer (rawStream, PixelFormat::RGB, 0);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (1, 1, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    JpegImageFormatReader reader (inStream);

    EXPECT_EQ (reader.getFormatName(), String ("JPEG Image"));
}
#endif

#if YUP_IMAGE_FORMAT_WEBP
TEST (ImageFormatReaderTests, WebPReaderHasCorrectFormatName)
{
    auto* rawStream = new MemoryOutputStream();
    WebPImageFormatWriter writer (rawStream, PixelFormat::RGB, 0);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (1, 1, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    WebPImageFormatReader reader (inStream);

    EXPECT_EQ (reader.getFormatName(), String ("WebP Image"));
}
#endif

#if YUP_IMAGE_FORMAT_GIF
TEST (ImageFormatReaderTests, GifReaderHasCorrectFormatName)
{
    auto* rawStream = new MemoryOutputStream();
    GifImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (1, 1, PixelFormat::RGBA)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    GifImageFormatReader reader (inStream);

    EXPECT_EQ (reader.getFormatName(), String ("GIF Image"));
}
#endif

// ======================================================================
// Reader dimension and header parsing tests
// ======================================================================

TEST (ImageFormatReaderTests, PpmReaderSetsWidthAndHeightFromHeader)
{
    const char data[] = "P6\n4 8\n255\n";
    auto* stream = new MemoryInputStream (data, sizeof (data) - 1, false);

    PpmImageFormatReader reader (stream);

    EXPECT_EQ (reader.width, 4);
    EXPECT_EQ (reader.height, 8);
    EXPECT_EQ (reader.pixelFormat, PixelFormat::RGB);
}

TEST (ImageFormatReaderTests, ReaderHasZeroWidthHeightForInvalidStream)
{
    const char data[] = "XX\n4 8\n255\n";
    auto* stream = new MemoryInputStream (data, sizeof (data) - 1, false);

    PpmImageFormatReader reader (stream);

    EXPECT_EQ (reader.width, 0);
    EXPECT_EQ (reader.height, 0);
}

TEST (ImageFormatReaderTests, PpmReaderParsesP5GrayscaleHeader)
{
    const char data[] = "P5\n8 4\n255\n";
    auto* stream = new MemoryInputStream (data, sizeof (data) - 1, false);

    PpmImageFormatReader reader (stream);

    EXPECT_EQ (reader.width, 8);
    EXPECT_EQ (reader.height, 4);
    EXPECT_EQ (reader.pixelFormat, PixelFormat::Grayscale);
}

TEST (ImageFormatReaderTests, PpmReaderParsesP3AsciiRgbHeader)
{
    const char data[] = "P3\n3 2\n255\n";
    auto* stream = new MemoryInputStream (data, sizeof (data) - 1, false);

    PpmImageFormatReader reader (stream);

    EXPECT_EQ (reader.width, 3);
    EXPECT_EQ (reader.height, 2);
    EXPECT_EQ (reader.pixelFormat, PixelFormat::RGB);
}

TEST (ImageFormatReaderTests, BmpReaderSetsCorrectPixelFormatRgb)
{
    auto* rawStream = new MemoryOutputStream();
    BmpImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (4, 4, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    BmpImageFormatReader reader (inStream);

    EXPECT_EQ (reader.pixelFormat, PixelFormat::RGB);
}

TEST (ImageFormatReaderTests, BmpReaderSetsCorrectPixelFormatRgba)
{
    auto* rawStream = new MemoryOutputStream();
    BmpImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (4, 4, PixelFormat::RGBA)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    BmpImageFormatReader reader (inStream);

    EXPECT_EQ (reader.pixelFormat, PixelFormat::RGBA);
}

// ======================================================================
// readImage round-trip tests
// ======================================================================

TEST (ImageFormatReaderTests, PpmReaderReadImageReturnsValidImage)
{
    Image source (4, 4, PixelFormat::RGB);
    source.fill (0xFFFF0000u);

    auto* rawStream = new MemoryOutputStream();
    PpmImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (source));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    PpmImageFormatReader reader (inStream);

    const Image result = reader.readImage();
    EXPECT_TRUE (result.isValid());
    EXPECT_EQ (result.getWidth(), 4);
    EXPECT_EQ (result.getHeight(), 4);
}

TEST (ImageFormatReaderTests, BmpReaderReadImageReturnsCorrectSize)
{
    auto* rawStream = new MemoryOutputStream();
    BmpImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (8, 6, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    BmpImageFormatReader reader (inStream);

    const Image result = reader.readImage();
    ASSERT_TRUE (result.isValid());
    EXPECT_EQ (result.getWidth(), 8);
    EXPECT_EQ (result.getHeight(), 6);
}

// ======================================================================
// isAnimated / frame count / loop count / frame delay
// ======================================================================

TEST (ImageFormatReaderTests, BmpReaderIsNotAnimated)
{
    auto* rawStream = new MemoryOutputStream();
    BmpImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (1, 1, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    BmpImageFormatReader reader (inStream);

    EXPECT_FALSE (reader.isAnimated());
    EXPECT_EQ (reader.getFrameCount(), 1);
    EXPECT_EQ (reader.getLoopCount(), 1);
    EXPECT_EQ (reader.getFrameDelayMs (0), 0);
}

TEST (ImageFormatReaderTests, PpmReaderIsNotAnimated)
{
    const char data[] = "P6\n2 2\n255\n\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00";
    auto* stream = new MemoryInputStream (data, sizeof (data) - 1, false);
    PpmImageFormatReader reader (stream);

    EXPECT_FALSE (reader.isAnimated());
    EXPECT_EQ (reader.getFrameCount(), 1);
    EXPECT_EQ (reader.getLoopCount(), 1);
    EXPECT_EQ (reader.getFrameDelayMs (0), 0);
}

// ======================================================================
// readFrame
// ======================================================================

TEST (ImageFormatReaderTests, BmpReaderReadFrameZeroMatchesReadImage)
{
    auto* rawStream = new MemoryOutputStream();
    BmpImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (4, 4, PixelFormat::RGB)));

    const auto encoded = rawStream->getMemoryBlock();

    auto* inStream1 = new MemoryInputStream (encoded.getData(), encoded.getSize(), false);
    BmpImageFormatReader reader1 (inStream1);
    const Image fromReadImage = reader1.readImage();

    auto* inStream2 = new MemoryInputStream (encoded.getData(), encoded.getSize(), false);
    BmpImageFormatReader reader2 (inStream2);
    const Image fromReadFrame = reader2.readFrame (0);

    ASSERT_TRUE (fromReadImage.isValid());
    ASSERT_TRUE (fromReadFrame.isValid());
    EXPECT_EQ (fromReadImage.getWidth(), fromReadFrame.getWidth());
    EXPECT_EQ (fromReadImage.getHeight(), fromReadFrame.getHeight());
}

// ======================================================================
// readFrame with non-zero index on non-animated format
// ======================================================================

TEST (ImageFormatReaderTests, ReadFrameNonZeroOnStaticImageReturnsEmpty)
{
    auto* rawStream = new MemoryOutputStream();
    BmpImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (4, 4, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    BmpImageFormatReader reader (inStream);

    auto frame = reader.readFrame (5);
    EXPECT_FALSE (frame.isValid());
}

TEST (ImageFormatReaderTests, ReadFrameNonZeroWithDestReturnsFalse)
{
    auto* rawStream = new MemoryOutputStream();
    BmpImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (4, 4, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    BmpImageFormatReader reader (inStream);

    Image dest;
    EXPECT_FALSE (reader.readFrame (5, dest));
}
