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

TEST (BmpImageFormatTests, ReaderSetsCorrectDimensionsFromHeader)
{
    // Write a small BMP to memory to get a valid header for the reader
    Image source (4, 8, PixelFormat::RGB);
    source.fill (0xFFFF0000u);

    auto* rawStream = new MemoryOutputStream();
    BmpImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (source));

    const void* data = rawStream->getData();
    size_t size = rawStream->getDataSize();

    auto* inStream = new MemoryInputStream (data, size, true);
    BmpImageFormatReader reader (inStream);

    EXPECT_EQ (reader.width, 4);
    EXPECT_EQ (reader.height, 8);
    EXPECT_EQ (reader.pixelFormat, PixelFormat::RGB);
}

TEST (BmpImageFormatTests, InvalidSignatureReturnsInvalidImage)
{
    // BMP data must start with "BM" — feed garbage bytes instead
    const uint8 garbage[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
    auto* stream = new MemoryInputStream (garbage, sizeof (garbage), false);

    BmpImageFormatReader reader (stream);

    EXPECT_EQ (reader.width, 0);
    EXPECT_EQ (reader.height, 0);

    auto result = reader.readImage();

    EXPECT_FALSE (result.isValid());
}

TEST (BmpImageFormatTests, WriteAndReadBackRgbProducesPixelIdenticalImage)
{
    auto original = generateTestImage (16, 16, PixelFormat::RGB);

    auto* rawStream = new MemoryOutputStream();
    BmpImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (original));

    const void* data = rawStream->getData();
    size_t size = rawStream->getDataSize();

    auto* inStream = new MemoryInputStream (data, size, true);
    BmpImageFormatReader reader (inStream);
    auto result = reader.readImage();

    EXPECT_EQ (result.getWidth(), original.getWidth());
    EXPECT_EQ (result.getHeight(), original.getHeight());
    EXPECT_EQ (result.getPixelFormat(), original.getPixelFormat());
    EXPECT_TRUE (imagesAreEqual (original, result, 0));
}

TEST (BmpImageFormatTests, WriteAndReadBackRgbaProducesPixelIdenticalImage)
{
    auto original = generateTestImage (16, 16, PixelFormat::RGBA);

    auto* rawStream = new MemoryOutputStream();
    BmpImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    ASSERT_TRUE (writer.writeImage (original));

    const void* data = rawStream->getData();
    size_t size = rawStream->getDataSize();

    auto* inStream = new MemoryInputStream (data, size, true);
    BmpImageFormatReader reader (inStream);
    auto result = reader.readImage();

    ASSERT_EQ (result.getWidth(), original.getWidth());
    ASSERT_EQ (result.getHeight(), original.getHeight());
    EXPECT_EQ (result.getPixelFormat(), original.getPixelFormat());

    // Compare all four channels including alpha
    for (int y = 0; y < original.getHeight(); ++y)
    {
        for (int x = 0; x < original.getWidth(); ++x)
        {
            EXPECT_EQ (result.getPixel (x, y), original.getPixel (x, y))
                << "Pixel mismatch at (" << x << ", " << y << ")";
        }
    }
}
