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

using namespace yup;

TEST (ImageFormatManagerTests, BmpFormatHandlesBmpExtension)
{
    BmpImageFormat fmt;

    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/image.bmp"), ImageFormat::forReading));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/image.bmp"), ImageFormat::forWriting));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image.png"), ImageFormat::forReading));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image.ppm"), ImageFormat::forWriting));
}

TEST (ImageFormatManagerTests, PpmFormatHandlesPpmPgmPbmExtensions)
{
    PpmImageFormat fmt;

    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/test.ppm"), ImageFormat::forReading));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/test.pgm"), ImageFormat::forReading));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/test.pbm"), ImageFormat::forReading));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/test.ppm"), ImageFormat::forWriting));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/test.bmp"), ImageFormat::forReading));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/test.png"), ImageFormat::forWriting));
}

TEST (ImageFormatManagerTests, BmpFormatNameIsCorrect)
{
    BmpImageFormat fmt;

    EXPECT_EQ (fmt.getFormatName(), String ("BMP Image"));
}

TEST (ImageFormatManagerTests, PpmFormatNameIsCorrect)
{
    PpmImageFormat fmt;

    EXPECT_EQ (fmt.getFormatName(), String ("PPM/PGM/PBM Image"));
}

TEST (ImageFormatManagerTests, RegisteredManagerReturnsNullWriterForUnknownExtension)
{
    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto writer = manager.createWriterFor (File ("/nonexistent/image.xyz"));

    EXPECT_EQ (writer, nullptr);
}

TEST (ImageFormatManagerTests, RegisteredManagerReturnsNullReaderForNonExistentFile)
{
    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto reader = manager.createReaderFor (File ("/nonexistent/image.bmp"));

    EXPECT_EQ (reader, nullptr);
}

TEST (ImageFormatManagerTests, BmpCanHandleStreamWithBmpMagicBytes)
{
    BmpImageFormat fmt;

    const uint8 bmpHeader[] = { 0x42, 0x4D, 0x00, 0x00 };
    MemoryInputStream stream (bmpHeader, std::size (bmpHeader), false);

    EXPECT_TRUE (fmt.canHandleStream (stream, ImageFormat::forReading));
    EXPECT_EQ (stream.getPosition(), 0);
}

TEST (ImageFormatManagerTests, BmpCanHandleStreamReturnsFalseForNonBmpBytes)
{
    BmpImageFormat fmt;

    const uint8 data[] = { 0x89, 0x50, 0x4E, 0x47 };
    MemoryInputStream stream (data, std::size (data), false);

    EXPECT_FALSE (fmt.canHandleStream (stream, ImageFormat::forReading));
    EXPECT_EQ (stream.getPosition(), 0);
}

TEST (ImageFormatManagerTests, PpmCanHandleStreamWithAllNetpbmMagics)
{
    PpmImageFormat fmt;

    for (char c = '1'; c <= '6'; ++c)
    {
        const uint8 data[] = { static_cast<uint8> ('P'), static_cast<uint8> (c) };
        MemoryInputStream stream (data, std::size (data), false);
        EXPECT_TRUE (fmt.canHandleStream (stream, ImageFormat::forReading));
        EXPECT_EQ (stream.getPosition(), 0);
    }
}

TEST (ImageFormatManagerTests, PpmCanHandleStreamReturnsFalseForNonPpmBytes)
{
    PpmImageFormat fmt;

    const uint8 data[] = { 0x42, 0x4D };
    MemoryInputStream stream (data, std::size (data), false);

    EXPECT_FALSE (fmt.canHandleStream (stream, ImageFormat::forReading));
    EXPECT_EQ (stream.getPosition(), 0);
}

TEST (ImageFormatManagerTests, CreateReaderForStreamReturnsBmpReaderForBmpData)
{
    const Image source = generateTestImage (4, 4, PixelFormat::RGB);

    auto* rawStream = new MemoryOutputStream();
    BmpImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (source));

    const auto* bytes = static_cast<const uint8*> (rawStream->getData());
    const auto size = rawStream->getDataSize();

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto* inStream = new MemoryInputStream (bytes, size, true);
    auto reader = manager.createReaderFor (inStream);

    ASSERT_NE (reader, nullptr);
    EXPECT_EQ (reader->width, 4);
    EXPECT_EQ (reader->height, 4);
}

TEST (ImageFormatManagerTests, CreateReaderForStreamReturnsNullForGarbageData)
{
    const uint8 garbage[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01, 0x02, 0x03 };

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto reader = manager.createReaderFor (new MemoryInputStream (garbage, std::size (garbage), false));

    EXPECT_EQ (reader, nullptr);
}

TEST (ImageFormatManagerTests, CreateReaderForStreamReturnsNullForNullStream)
{
    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto reader = manager.createReaderFor (static_cast<InputStream*> (nullptr));

    EXPECT_EQ (reader, nullptr);
}
