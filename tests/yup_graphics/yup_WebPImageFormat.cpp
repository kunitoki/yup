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

#if YUP_MODULE_AVAILABLE_libwebp && YUP_IMAGE_FORMAT_WEBP

TEST (WebPImageFormatTests, WriteAndReadBackRgbaLosslessProducesPixelIdenticalImage)
{
    auto original = generateTestImage (16, 16, PixelFormat::RGBA);

    auto* rawStream = new MemoryOutputStream();
    WebPImageFormatWriter writer (rawStream, PixelFormat::RGBA, 0);
    ASSERT_TRUE (writer.writeImage (original));

    const void* data = rawStream->getData();
    size_t size = rawStream->getDataSize();

    auto* inStream = new MemoryInputStream (data, size, true);
    WebPImageFormatReader reader (inStream);
    auto result = reader.readImage();

    ASSERT_EQ (result.getWidth(), original.getWidth());
    ASSERT_EQ (result.getHeight(), original.getHeight());
    EXPECT_EQ (result.getPixelFormat(), original.getPixelFormat());
    EXPECT_TRUE (imagesAreEqual (original, result, 0));
}

TEST (WebPImageFormatTests, WriteAndReadBackRgbLosslessProducesPixelIdenticalImage)
{
    auto original = generateTestImage (16, 16, PixelFormat::RGB);

    auto* rawStream = new MemoryOutputStream();
    WebPImageFormatWriter writer (rawStream, PixelFormat::RGB, 0);
    ASSERT_TRUE (writer.writeImage (original));

    const void* data = rawStream->getData();
    size_t size = rawStream->getDataSize();

    auto* inStream = new MemoryInputStream (data, size, true);
    WebPImageFormatReader reader (inStream);
    auto result = reader.readImage();

    EXPECT_EQ (result.getWidth(), original.getWidth());
    EXPECT_EQ (result.getHeight(), original.getHeight());
    EXPECT_EQ (result.getPixelFormat(), original.getPixelFormat());
    EXPECT_TRUE (imagesAreEqual (original, result, 0));
}

TEST (WebPImageFormatTests, WriteAndReadBackRgbaLossyProducesNearEqualImage)
{
    auto original = generateTestImage (16, 16, PixelFormat::RGBA);

    auto* rawStream = new MemoryOutputStream();
    WebPImageFormatWriter writer (rawStream, PixelFormat::RGBA, 1); // qualityIndex 1 = Quality 90
    ASSERT_TRUE (writer.writeImage (original));

    const void* data = rawStream->getData();
    size_t size = rawStream->getDataSize();

    auto* inStream = new MemoryInputStream (data, size, true);
    WebPImageFormatReader reader (inStream);
    auto result = reader.readImage();

    ASSERT_EQ (result.getWidth(), original.getWidth());
    ASSERT_EQ (result.getHeight(), original.getHeight());
    EXPECT_TRUE (imagesAreEqual (original, result, 30));
}

TEST (WebPImageFormatTests, ReaderSetsCorrectDimensions)
{
    Image source (4, 6, PixelFormat::RGBA);
    source.fill (0xFF0000FFu);

    auto* rawStream = new MemoryOutputStream();
    WebPImageFormatWriter writer (rawStream, PixelFormat::RGBA, 0);
    ASSERT_TRUE (writer.writeImage (source));

    const void* data = rawStream->getData();
    size_t size = rawStream->getDataSize();

    auto* inStream = new MemoryInputStream (data, size, true);
    WebPImageFormatReader reader (inStream);

    EXPECT_EQ (reader.width, 4);
    EXPECT_EQ (reader.height, 6);
}

TEST (WebPImageFormatTests, WebPFormatHasCorrectQualityOptions)
{
    WebPImageFormat fmt;
    auto options = fmt.getQualityOptions();

    ASSERT_EQ (options.size(), 5);
    EXPECT_EQ (options[0], "Lossless");
}

#endif // YUP_MODULE_AVAILABLE_libwebp && YUP_IMAGE_FORMAT_WEBP
