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

#if YUP_MODULE_AVAILABLE_libpng && YUP_IMAGE_FORMAT_PNG

TEST (PngImageFormatTests, WriteAndReadBackRgbaProducesPixelIdenticalImage)
{
    auto original = generateTestImage (16, 16, PixelFormat::RGBA);

    auto* rawStream = new MemoryOutputStream();
    PngImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    ASSERT_TRUE (writer.writeImage (original));

    const void* data = rawStream->getData();
    size_t size = rawStream->getDataSize();

    auto* inStream = new MemoryInputStream (data, size, true);
    PngImageFormatReader reader (inStream);
    auto result = reader.readImage();

    ASSERT_EQ (result.getWidth(), original.getWidth());
    ASSERT_EQ (result.getHeight(), original.getHeight());
    EXPECT_EQ (result.getPixelFormat(), original.getPixelFormat());
    EXPECT_TRUE (imagesAreEqual (original, result, 0));
}

TEST (PngImageFormatTests, WriteAndReadBackRgbProducesPixelIdenticalImage)
{
    auto original = generateTestImage (16, 16, PixelFormat::RGB);

    auto* rawStream = new MemoryOutputStream();
    PngImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (original));

    const void* data = rawStream->getData();
    size_t size = rawStream->getDataSize();

    auto* inStream = new MemoryInputStream (data, size, true);
    PngImageFormatReader reader (inStream);
    auto result = reader.readImage();

    EXPECT_EQ (result.getWidth(), original.getWidth());
    EXPECT_EQ (result.getHeight(), original.getHeight());
    EXPECT_EQ (result.getPixelFormat(), original.getPixelFormat());
    EXPECT_TRUE (imagesAreEqual (original, result, 0));
}

TEST (PngImageFormatTests, WriteAndReadBackGrayscaleProducesPixelIdenticalImage)
{
    auto original = generateTestImage (8, 8, PixelFormat::Grayscale);

    auto* rawStream = new MemoryOutputStream();
    PngImageFormatWriter writer (rawStream, PixelFormat::Grayscale);
    ASSERT_TRUE (writer.writeImage (original));

    const void* data = rawStream->getData();
    size_t size = rawStream->getDataSize();

    auto* inStream = new MemoryInputStream (data, size, true);
    PngImageFormatReader reader (inStream);
    auto result = reader.readImage();

    EXPECT_EQ (result.getWidth(), original.getWidth());
    EXPECT_EQ (result.getHeight(), original.getHeight());
    EXPECT_EQ (result.getPixelFormat(), original.getPixelFormat());
    EXPECT_TRUE (imagesAreEqual (original, result, 0));
}

TEST (PngImageFormatTests, ReaderSetsCorrectWidthAndHeight)
{
    Image source (5, 7, PixelFormat::RGBA);
    source.fill (0xFF00FF00u);

    auto* rawStream = new MemoryOutputStream();
    PngImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    ASSERT_TRUE (writer.writeImage (source));

    const void* data = rawStream->getData();
    size_t size = rawStream->getDataSize();

    auto* inStream = new MemoryInputStream (data, size, true);
    PngImageFormatReader reader (inStream);

    EXPECT_EQ (reader.width, 5);
    EXPECT_EQ (reader.height, 7);
}

TEST (PngImageFormatTests, ReaderHasAccessibleMetadataValues)
{
    auto original = generateTestImage (8, 8, PixelFormat::RGBA);

    auto* rawStream = new MemoryOutputStream();
    PngImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    ASSERT_TRUE (writer.writeImage (original));

    const void* data = rawStream->getData();
    size_t size = rawStream->getDataSize();

    auto* inStream = new MemoryInputStream (data, size, true);
    PngImageFormatReader reader (inStream);

    // metadataValues may be empty for simple images — verify the field exists and is accessible
    EXPECT_EQ (reader.dpiX, 0.0);
    EXPECT_EQ (reader.dpiY, 0.0);
}

#endif // YUP_MODULE_AVAILABLE_libpng && YUP_IMAGE_FORMAT_PNG
