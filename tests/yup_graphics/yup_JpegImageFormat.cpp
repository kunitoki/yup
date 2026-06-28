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

#if YUP_MODULE_AVAILABLE_libjpeg_turbo && YUP_IMAGE_FORMAT_JPEG

TEST (JpegImageFormatTests, WriteAndReadBackRgbPreservesDimensionsAndApproximatePixels)
{
    Image original (16, 16, PixelFormat::RGB);
    original.fill (0xFF335577u);

    auto* rawStream = new MemoryOutputStream();
    JpegImageFormatWriter writer (rawStream, PixelFormat::RGB, 0);
    ASSERT_TRUE (writer.writeImage (original));

    const void* data = rawStream->getData();
    size_t size = rawStream->getDataSize();

    auto* inStream = new MemoryInputStream (data, size, true);
    JpegImageFormatReader reader (inStream);
    auto result = reader.readImage();

    EXPECT_EQ (result.getWidth(), original.getWidth());
    EXPECT_EQ (result.getHeight(), original.getHeight());
    EXPECT_EQ (result.getPixelFormat(), PixelFormat::RGB);
    EXPECT_TRUE (imagesAreEqual (original, result, 3));
}

TEST (JpegImageFormatTests, WriteAndReadBackGrayscalePreservesPixelFormat)
{
    Image original (8, 8, PixelFormat::Grayscale);
    original.fill (0xFF777777u);

    auto* rawStream = new MemoryOutputStream();
    JpegImageFormatWriter writer (rawStream, PixelFormat::Grayscale, 0);
    ASSERT_TRUE (writer.writeImage (original));

    const void* data = rawStream->getData();
    size_t size = rawStream->getDataSize();

    auto* inStream = new MemoryInputStream (data, size, true);
    JpegImageFormatReader reader (inStream);
    auto result = reader.readImage();

    EXPECT_EQ (result.getWidth(), original.getWidth());
    EXPECT_EQ (result.getHeight(), original.getHeight());
    EXPECT_EQ (result.getPixelFormat(), PixelFormat::Grayscale);
    EXPECT_TRUE (imagesAreEqual (original, result, 3));
}

TEST (JpegImageFormatTests, ReaderSetsCorrectWidthAndHeight)
{
    Image source (5, 7, PixelFormat::RGB);
    source.fill (0xFF00FF00u);

    auto* rawStream = new MemoryOutputStream();
    JpegImageFormatWriter writer (rawStream, PixelFormat::RGB, 0);
    ASSERT_TRUE (writer.writeImage (source));

    const void* data = rawStream->getData();
    size_t size = rawStream->getDataSize();

    auto* inStream = new MemoryInputStream (data, size, true);
    JpegImageFormatReader reader (inStream);

    EXPECT_EQ (reader.width, 5);
    EXPECT_EQ (reader.height, 7);
    EXPECT_EQ (reader.pixelFormat, PixelFormat::RGB);
}

TEST (JpegImageFormatTests, CanHandleStreamDetectsJpegSignature)
{
    uint8 jpegSignature[] = { 0xFF, 0xD8, 0xFF, 0xE0 };
    MemoryInputStream stream (jpegSignature, sizeof (jpegSignature), false);

    JpegImageFormat format;
    EXPECT_TRUE (format.canHandleStream (stream, ImageFormat::forReading));
    EXPECT_EQ (stream.getPosition(), 0);
}

TEST (JpegImageFormatTests, DefaultManagerCreatesReaderForJpegStream)
{
    Image source (4, 4, PixelFormat::RGB);
    source.fill (0xFFFF0000u);

    auto* rawStream = new MemoryOutputStream();
    JpegImageFormatWriter writer (rawStream, PixelFormat::RGB, 0);
    ASSERT_TRUE (writer.writeImage (source));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto reader = manager.createReaderFor (inStream);
    ASSERT_NE (reader, nullptr);
    EXPECT_EQ (reader->getFormatName(), String ("JPEG Image"));
}

#endif // YUP_MODULE_AVAILABLE_libjpeg_turbo && YUP_IMAGE_FORMAT_JPEG
