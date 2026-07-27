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

// ======================================================================
// BMP format registration and file handling
// ======================================================================

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

// ======================================================================
// Manager file-based reader/writer creation
// ======================================================================

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

TEST (ImageFormatManagerTests, RegisteredManagerReturnsNullWriterForEmptyExtension)
{
    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto writer = manager.createWriterFor (File ("/nonexistent/image"));
    EXPECT_EQ (writer, nullptr);
}

// ======================================================================
// BMP stream detection
// ======================================================================

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

// ======================================================================
// PPM stream detection
// ======================================================================

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

// ======================================================================
// Manager stream-based reader creation
// ======================================================================

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

TEST (ImageFormatManagerTests, CreateReaderForStreamReturnsPpmReaderForPpmData)
{
    auto* rawStream = new MemoryOutputStream();
    PpmImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (generateTestImage (4, 4, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto reader = manager.createReaderFor (inStream);
    ASSERT_NE (reader, nullptr);
    EXPECT_EQ (reader->getFormatName(), String ("PPM/PGM/PBM Image"));
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

// ======================================================================
// Manager file-based roundtrip tests
// ======================================================================

TEST (ImageFormatManagerTests, ManagerBmpFileRoundtrip)
{
    auto original = generateTestImage (10, 8, PixelFormat::RGB);
    auto tempFile = File::createTempFile (".bmp");

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    {
        auto writer = manager.createWriterFor (tempFile, PixelFormat::RGB);
        ASSERT_NE (writer, nullptr);
        ASSERT_TRUE (writer->writeImage (original));
    }

    {
        auto reader = manager.createReaderFor (tempFile);
        ASSERT_NE (reader, nullptr);
        EXPECT_EQ (reader->getFormatName(), String ("BMP Image"));

        auto result = reader->readImage();
        ASSERT_TRUE (result.isValid());
        EXPECT_TRUE (imagesAreEqual (original, result, 0));
    }

    tempFile.deleteFile();
}

TEST (ImageFormatManagerTests, ManagerPpmFileRoundtrip)
{
    auto original = generateTestImage (10, 8, PixelFormat::RGB);
    auto tempFile = File::createTempFile (".ppm");

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    {
        auto writer = manager.createWriterFor (tempFile, PixelFormat::RGB);
        ASSERT_NE (writer, nullptr);
        ASSERT_TRUE (writer->writeImage (original));
    }

    {
        auto reader = manager.createReaderFor (tempFile);
        ASSERT_NE (reader, nullptr);
        EXPECT_EQ (reader->getFormatName(), String ("PPM/PGM/PBM Image"));

        auto result = reader->readImage();
        ASSERT_TRUE (result.isValid());
        EXPECT_TRUE (imagesAreEqual (original, result, 0));
    }

    tempFile.deleteFile();
}

#if YUP_IMAGE_FORMAT_PNG
TEST (ImageFormatManagerTests, ManagerPngFileRoundtrip)
{
    auto original = generateTestImage (10, 8, PixelFormat::RGBA);
    auto tempFile = File::createTempFile (".png");

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    {
        auto writer = manager.createWriterFor (tempFile, PixelFormat::RGBA);
        ASSERT_NE (writer, nullptr);
        ASSERT_TRUE (writer->writeImage (original));
    }

    {
        auto reader = manager.createReaderFor (tempFile);
        ASSERT_NE (reader, nullptr);
        EXPECT_EQ (reader->getFormatName(), String ("PNG Image"));

        auto result = reader->readImage();
        ASSERT_TRUE (result.isValid());
        EXPECT_TRUE (imagesAreEqual (original, result, 0));
    }

    tempFile.deleteFile();
}

TEST (ImageFormatManagerTests, CreateReaderForStreamReturnsPngReaderForPngData)
{
    auto* rawStream = new MemoryOutputStream();
    PngImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (generateTestImage (4, 4, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto reader = manager.createReaderFor (inStream);
    ASSERT_NE (reader, nullptr);
    EXPECT_EQ (reader->getFormatName(), String ("PNG Image"));
}
#endif

#if YUP_IMAGE_FORMAT_JPEG
TEST (ImageFormatManagerTests, ManagerJpegFileRoundtrip)
{
    Image original (10, 8, PixelFormat::RGB);
    original.fill (0xFF445566u);
    auto tempFile = File::createTempFile (".jpg");

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    {
        auto writer = manager.createWriterFor (tempFile, PixelFormat::RGB);
        ASSERT_NE (writer, nullptr);
        ASSERT_TRUE (writer->writeImage (original));
    }

    {
        auto reader = manager.createReaderFor (tempFile);
        ASSERT_NE (reader, nullptr);
        EXPECT_EQ (reader->getFormatName(), String ("JPEG Image"));

        auto result = reader->readImage();
        ASSERT_TRUE (result.isValid());
        EXPECT_TRUE (imagesAreEqual (original, result, 3));
    }

    tempFile.deleteFile();
}

TEST (ImageFormatManagerTests, ManagerJpegFileRoundtripViaJpegExtension)
{
    Image original (8, 8, PixelFormat::RGB);
    original.fill (0xFF778899u);
    auto tempFile = File::createTempFile (".jpeg");

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    {
        auto writer = manager.createWriterFor (tempFile, PixelFormat::RGB);
        ASSERT_NE (writer, nullptr);
        ASSERT_TRUE (writer->writeImage (original));
    }

    {
        auto reader = manager.createReaderFor (tempFile);
        ASSERT_NE (reader, nullptr);
        EXPECT_EQ (reader->getFormatName(), String ("JPEG Image"));
        auto result = reader->readImage();
        ASSERT_TRUE (result.isValid());
    }

    tempFile.deleteFile();
}

TEST (ImageFormatManagerTests, CreateReaderForStreamReturnsJpegReaderForJpegData)
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
#endif

#if YUP_IMAGE_FORMAT_WEBP
TEST (ImageFormatManagerTests, ManagerWebPFileRoundtrip)
{
    auto original = generateTestImage (10, 8, PixelFormat::RGBA);
    auto tempFile = File::createTempFile (".webp");

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    {
        auto writer = manager.createWriterFor (tempFile, PixelFormat::RGBA);
        ASSERT_NE (writer, nullptr);
        ASSERT_TRUE (writer->writeImage (original));
    }

    {
        auto reader = manager.createReaderFor (tempFile);
        ASSERT_NE (reader, nullptr);
        EXPECT_EQ (reader->getFormatName(), String ("WebP Image"));

        auto result = reader->readImage();
        ASSERT_TRUE (result.isValid());
        EXPECT_TRUE (imagesAreEqual (original, result, 0));
    }

    tempFile.deleteFile();
}

TEST (ImageFormatManagerTests, CreateReaderForStreamReturnsWebPReaderForWebPData)
{
    auto* rawStream = new MemoryOutputStream();
    WebPImageFormatWriter writer (rawStream, PixelFormat::RGB, 0);
    ASSERT_TRUE (writer.writeImage (generateTestImage (4, 4, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto reader = manager.createReaderFor (inStream);
    ASSERT_NE (reader, nullptr);
    EXPECT_EQ (reader->getFormatName(), String ("WebP Image"));
}
#endif

#if YUP_IMAGE_FORMAT_GIF
TEST (ImageFormatManagerTests, ManagerGifFileRoundtrip)
{
    Image original (10, 8, PixelFormat::RGBA);
    original.fill (0xFFAA7744u);
    auto tempFile = File::createTempFile (".gif");

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    {
        auto writer = manager.createWriterFor (tempFile, PixelFormat::RGBA);
        ASSERT_NE (writer, nullptr);
        ASSERT_TRUE (writer->writeImage (original));
    }

    {
        auto reader = manager.createReaderFor (tempFile);
        ASSERT_NE (reader, nullptr);
        EXPECT_EQ (reader->getFormatName(), String ("GIF Image"));

        auto result = reader->readImage();
        ASSERT_TRUE (result.isValid());
        EXPECT_TRUE (imagesAreEqual (original, result, 8));
    }

    tempFile.deleteFile();
}

TEST (ImageFormatManagerTests, CreateReaderForStreamReturnsGifReaderForGifData)
{
    auto* rawStream = new MemoryOutputStream();
    GifImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (4, 4, PixelFormat::RGBA, 0xFFCC4400u)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto reader = manager.createReaderFor (inStream);
    ASSERT_NE (reader, nullptr);
    EXPECT_EQ (reader->getFormatName(), String ("GIF Image"));
}
#endif

// ======================================================================
// Manager registration tests
// ======================================================================

TEST (ImageFormatManagerTests, SelectiveFormatRegistration)
{
    {
        ImageFormatManager manager;
        manager.registerDefaultFormats (ImageFormatType::bmp);

        auto tempFile = File::createTempFile (".bmp");
        EXPECT_NE (manager.createWriterFor (tempFile, PixelFormat::RGB), nullptr);

        auto ppmTempFile = File::createTempFile (".ppm");
        EXPECT_EQ (manager.createWriterFor (ppmTempFile, PixelFormat::RGB), nullptr);

        tempFile.deleteFile();
        ppmTempFile.deleteFile();
    }
    {
        ImageFormatManager manager;
        manager.registerDefaultFormats (ImageFormatType::ppm);

        auto tempFile = File::createTempFile (".ppm");
        EXPECT_NE (manager.createWriterFor (tempFile, PixelFormat::RGB), nullptr);

        auto bmpTempFile = File::createTempFile (".bmp");
        EXPECT_EQ (manager.createWriterFor (bmpTempFile, PixelFormat::RGB), nullptr);

        tempFile.deleteFile();
        bmpTempFile.deleteFile();
    }
}

TEST (ImageFormatManagerTests, RegisterAllFormatsByDefault)
{
    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto bmpFile = File::createTempFile (".bmp");
    auto ppmFile = File::createTempFile (".ppm");

    EXPECT_NE (manager.createWriterFor (bmpFile, PixelFormat::RGB), nullptr);
    EXPECT_NE (manager.createWriterFor (ppmFile, PixelFormat::RGB), nullptr);

    bmpFile.deleteFile();
    ppmFile.deleteFile();
}

// ======================================================================
// Reader/writer format name tests via manager
// ======================================================================

TEST (ImageFormatManagerTests, BmpReaderHasCorrectFormatName)
{
    auto* rawStream = new MemoryOutputStream();
    BmpImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (4, 4, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    BmpImageFormatReader reader (inStream);

    EXPECT_EQ (reader.getFormatName(), String ("BMP Image"));
}

TEST (ImageFormatManagerTests, PpmReaderHasCorrectFormatName)
{
    auto* rawStream = new MemoryOutputStream();
    PpmImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (4, 4, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    PpmImageFormatReader reader (inStream);

    EXPECT_EQ (reader.getFormatName(), String ("PPM/PGM/PBM Image"));
}

// ======================================================================
// getFormatFileExtensions tests
// ======================================================================

TEST (ImageFormatManagerTests, GetFormatFileExtensionsReturnsAllExtensions)
{
    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto exts = manager.getFormatFileExtensions();
    EXPECT_GT (exts.size(), 0);
    EXPECT_TRUE (exts.contains (".bmp"));
    EXPECT_TRUE (exts.contains (".ppm"));
}

// ======================================================================
// createWriterFor file: writer fallback when canHandleFile fails
// ======================================================================

TEST (ImageFormatManagerTests, CreateWriterForFileForNonexistentDirReturnsNull)
{
    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto writer = manager.createWriterFor (File ("/nonexistent/path/image.bmp"));
    EXPECT_EQ (writer, nullptr);
}
