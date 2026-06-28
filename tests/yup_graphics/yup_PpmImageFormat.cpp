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

TEST (PpmImageFormatTests, ReaderParsesP6RgbHeader)
{
    const char data[] = "P6\n8 4\n255\n";
    auto* stream = new MemoryInputStream (data, sizeof (data) - 1, false);

    PpmImageFormatReader reader (stream);

    EXPECT_EQ (reader.width, 8);
    EXPECT_EQ (reader.height, 4);
    EXPECT_EQ (reader.pixelFormat, PixelFormat::RGB);
}

TEST (PpmImageFormatTests, ReaderParsesP5GrayscaleHeader)
{
    const char data[] = "P5\n8 4\n255\n";
    auto* stream = new MemoryInputStream (data, sizeof (data) - 1, false);

    PpmImageFormatReader reader (stream);

    EXPECT_EQ (reader.width, 8);
    EXPECT_EQ (reader.height, 4);
    EXPECT_EQ (reader.pixelFormat, PixelFormat::Grayscale);
}

TEST (PpmImageFormatTests, InvalidMagicReturnsInvalidImage)
{
    const char data[] = "XX\n2 2\n255\n";
    auto* stream = new MemoryInputStream (data, sizeof (data) - 1, false);

    PpmImageFormatReader reader (stream);

    EXPECT_EQ (reader.width, 0);
    EXPECT_EQ (reader.height, 0);

    auto result = reader.readImage();

    EXPECT_FALSE (result.isValid());
}

TEST (PpmImageFormatTests, WriteAndReadBackRgbProducesPixelIdenticalImage)
{
    auto original = generateTestImage (16, 16, PixelFormat::RGB);

    // Write to heap-allocated stream (writer takes ownership)
    auto* rawStream = new MemoryOutputStream();
    PpmImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (original));

    // Capture data before writer (and stream) are destroyed
    const void* data = rawStream->getData();
    size_t size = rawStream->getDataSize();

    auto* inStream = new MemoryInputStream (data, size, true); // keepCopy=true
    PpmImageFormatReader reader (inStream);
    auto result = reader.readImage();

    EXPECT_EQ (result.getWidth(), original.getWidth());
    EXPECT_EQ (result.getHeight(), original.getHeight());
    EXPECT_EQ (result.getPixelFormat(), original.getPixelFormat());
    EXPECT_TRUE (imagesAreEqual (original, result, 0));
}

TEST (PpmImageFormatTests, WriteAndReadBackGrayscaleProducesPixelIdenticalImage)
{
    auto original = generateTestImage (8, 8, PixelFormat::Grayscale);

    auto* rawStream = new MemoryOutputStream();
    PpmImageFormatWriter writer (rawStream, PixelFormat::Grayscale);
    ASSERT_TRUE (writer.writeImage (original));

    const void* data = rawStream->getData();
    size_t size = rawStream->getDataSize();

    auto* inStream = new MemoryInputStream (data, size, true);
    PpmImageFormatReader reader (inStream);
    auto result = reader.readImage();

    EXPECT_EQ (result.getWidth(), original.getWidth());
    EXPECT_EQ (result.getHeight(), original.getHeight());
    EXPECT_EQ (result.getPixelFormat(), original.getPixelFormat());
    EXPECT_TRUE (imagesAreEqual (original, result, 0));
}
