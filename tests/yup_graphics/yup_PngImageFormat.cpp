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

// ======================================================================
// Reader dimension and header tests
// ======================================================================

TEST (PngImageFormatTests, ReaderSetsCorrectWidthAndHeight)
{
    auto* rawStream = new MemoryOutputStream();
    PngImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (5, 7, PixelFormat::RGBA)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    PngImageFormatReader reader (inStream);

    EXPECT_EQ (reader.width, 5);
    EXPECT_EQ (reader.height, 7);
}

TEST (PngImageFormatTests, ReaderSetsDimensionsForSmallImage)
{
    auto* rawStream = new MemoryOutputStream();
    PngImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (1, 1, PixelFormat::RGBA)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    PngImageFormatReader reader (inStream);

    EXPECT_EQ (reader.width, 1);
    EXPECT_EQ (reader.height, 1);
}

TEST (PngImageFormatTests, ReaderSetsDimensionsForNonSquareImage)
{
    auto* rawStream = new MemoryOutputStream();
    PngImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (17, 3, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    PngImageFormatReader reader (inStream);

    EXPECT_EQ (reader.width, 17);
    EXPECT_EQ (reader.height, 3);
}

TEST (PngImageFormatTests, ReaderSetsCorrectPixelFormatRgb)
{
    auto* rawStream = new MemoryOutputStream();
    PngImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (4, 4, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    PngImageFormatReader reader (inStream);

    EXPECT_EQ (reader.pixelFormat, PixelFormat::RGB);
}

TEST (PngImageFormatTests, ReaderSetsCorrectPixelFormatRgba)
{
    auto* rawStream = new MemoryOutputStream();
    PngImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (4, 4, PixelFormat::RGBA)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    PngImageFormatReader reader (inStream);

    EXPECT_EQ (reader.pixelFormat, PixelFormat::RGBA);
}

TEST (PngImageFormatTests, ReaderSetsCorrectPixelFormatGrayscale)
{
    auto* rawStream = new MemoryOutputStream();
    PngImageFormatWriter writer (rawStream, PixelFormat::Grayscale);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (4, 4, PixelFormat::Grayscale)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    PngImageFormatReader reader (inStream);

    EXPECT_EQ (reader.pixelFormat, PixelFormat::Grayscale);
}

TEST (PngImageFormatTests, InvalidSignatureReturnsInvalidImage)
{
    const uint8 garbage[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
    auto* stream = new MemoryInputStream (garbage, sizeof (garbage), false);

    PngImageFormatReader reader (stream);

    EXPECT_EQ (reader.width, 0);
    EXPECT_EQ (reader.height, 0);

    auto result = reader.readImage();
    EXPECT_FALSE (result.isValid());
}

TEST (PngImageFormatTests, ReaderHasAccessibleMetadataValues)
{
    auto* rawStream = new MemoryOutputStream();
    PngImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    ASSERT_TRUE (writer.writeImage (generateTestImage (8, 8, PixelFormat::RGBA)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    PngImageFormatReader reader (inStream);

    EXPECT_EQ (reader.dpiX, 0.0);
    EXPECT_EQ (reader.dpiY, 0.0);
}

// ======================================================================
// Roundtrip tests (memory-based)
// ======================================================================

TEST (PngImageFormatTests, WriteAndReadBackRgbProducesPixelIdenticalImage)
{
    auto original = generateTestImage (16, 16, PixelFormat::RGB);

    auto* rawStream = new MemoryOutputStream();
    PngImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    PngImageFormatReader reader (inStream);
    auto result = reader.readImage();

    EXPECT_EQ (result.getWidth(), original.getWidth());
    EXPECT_EQ (result.getHeight(), original.getHeight());
    EXPECT_EQ (result.getPixelFormat(), original.getPixelFormat());
    EXPECT_TRUE (imagesAreEqual (original, result, 0));
}

TEST (PngImageFormatTests, WriteAndReadBackRgbaProducesPixelIdenticalImage)
{
    auto original = generateTestImage (16, 16, PixelFormat::RGBA);

    auto* rawStream = new MemoryOutputStream();
    PngImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    PngImageFormatReader reader (inStream);
    auto result = reader.readImage();

    ASSERT_EQ (result.getWidth(), original.getWidth());
    ASSERT_EQ (result.getHeight(), original.getHeight());
    EXPECT_EQ (result.getPixelFormat(), original.getPixelFormat());
    EXPECT_TRUE (imagesAreEqual (original, result, 0));
}

TEST (PngImageFormatTests, WriteAndReadBackGrayscaleProducesPixelIdenticalImage)
{
    auto original = generateTestImage (8, 8, PixelFormat::Grayscale);

    auto* rawStream = new MemoryOutputStream();
    PngImageFormatWriter writer (rawStream, PixelFormat::Grayscale);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    PngImageFormatReader reader (inStream);
    auto result = reader.readImage();

    EXPECT_EQ (result.getWidth(), original.getWidth());
    EXPECT_EQ (result.getHeight(), original.getHeight());
    EXPECT_EQ (result.getPixelFormat(), original.getPixelFormat());
    EXPECT_TRUE (imagesAreEqual (original, result, 0));
}

TEST (PngImageFormatTests, SolidColorRgbRoundtripIsExact)
{
    auto original = generateSolidImage (20, 10, PixelFormat::RGB, 0xFF4488CCu);

    auto* rawStream = new MemoryOutputStream();
    PngImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    PngImageFormatReader reader (inStream);
    auto result = reader.readImage();

    EXPECT_TRUE (imagesAreEqual (original, result, 0));
}

TEST (PngImageFormatTests, SolidColorRgbaRoundtripIsExact)
{
    auto original = generateSolidImage (12, 8, PixelFormat::RGBA, 0x88AABBCCu);

    auto* rawStream = new MemoryOutputStream();
    PngImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    PngImageFormatReader reader (inStream);
    auto result = reader.readImage();

    for (int y = 0; y < original.getHeight(); ++y)
    {
        for (int x = 0; x < original.getWidth(); ++x)
            EXPECT_EQ (result.getPixel (x, y), original.getPixel (x, y));
    }
}

TEST (PngImageFormatTests, VerySmallImageRoundtrip)
{
    auto original = generateSolidImage (1, 1, PixelFormat::RGBA, 0xFF112233u);

    auto* rawStream = new MemoryOutputStream();
    PngImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    PngImageFormatReader reader (inStream);
    auto result = reader.readImage();

    EXPECT_EQ (result.getPixel (0, 0), 0xFF112233u);
}

TEST (PngImageFormatTests, VariousSizesRoundtripCorrectly)
{
    const int sizes[][2] = { { 1, 1 }, { 1, 32 }, { 32, 1 }, { 7, 13 }, { 64, 48 } };

    for (auto [w, h] : sizes)
    {
        auto original = generateTestImage (w, h, PixelFormat::RGB);

        auto* rawStream = new MemoryOutputStream();
        PngImageFormatWriter writer (rawStream, PixelFormat::RGB);
        ASSERT_TRUE (writer.writeImage (original));

        auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
        PngImageFormatReader reader (inStream);
        auto result = reader.readImage();

        EXPECT_EQ (result.getWidth(), w);
        EXPECT_EQ (result.getHeight(), h);
        EXPECT_TRUE (imagesAreEqual (original, result, 0))
            << "Size mismatch at " << w << "x" << h;
    }
}

// ======================================================================
// File-based roundtrip tests (temporary files)
// ======================================================================

TEST (PngImageFormatTests, FileRoundtripRgbViaTempFile)
{
    auto original = generateTestImage (16, 12, PixelFormat::RGB);
    auto tempFile = File::createTempFile (".png");

    {
        auto* fos = tempFile.createOutputStream().release();
        PngImageFormatWriter writer (fos, PixelFormat::RGB);
        ASSERT_TRUE (writer.writeImage (original));
    }

    {
        auto* fis = tempFile.createInputStream().release();
        PngImageFormatReader reader (fis);
        auto result = reader.readImage();

        ASSERT_TRUE (result.isValid());
        EXPECT_EQ (result.getWidth(), original.getWidth());
        EXPECT_EQ (result.getHeight(), original.getHeight());
        EXPECT_TRUE (imagesAreEqual (original, result, 0));
    }

    tempFile.deleteFile();
}

TEST (PngImageFormatTests, FileRoundtripRgbaViaTempFile)
{
    auto original = generateTestImage (10, 14, PixelFormat::RGBA);
    auto tempFile = File::createTempFile (".png");

    {
        auto* fos = tempFile.createOutputStream().release();
        PngImageFormatWriter writer (fos, PixelFormat::RGBA);
        ASSERT_TRUE (writer.writeImage (original));
    }

    {
        auto* fis = tempFile.createInputStream().release();
        PngImageFormatReader reader (fis);
        auto result = reader.readImage();

        ASSERT_TRUE (result.isValid());
        EXPECT_EQ (result.getPixelFormat(), PixelFormat::RGBA);
        EXPECT_TRUE (imagesAreEqual (original, result, 0));
    }

    tempFile.deleteFile();
}

TEST (PngImageFormatTests, FileRoundtripGrayscaleViaTempFile)
{
    auto original = generateTestImage (8, 8, PixelFormat::Grayscale);
    auto tempFile = File::createTempFile (".png");

    {
        auto* fos = tempFile.createOutputStream().release();
        PngImageFormatWriter writer (fos, PixelFormat::Grayscale);
        ASSERT_TRUE (writer.writeImage (original));
    }

    {
        auto* fis = tempFile.createInputStream().release();
        PngImageFormatReader reader (fis);
        auto result = reader.readImage();

        ASSERT_TRUE (result.isValid());
        EXPECT_EQ (result.getPixelFormat(), PixelFormat::Grayscale);
        EXPECT_TRUE (imagesAreEqual (original, result, 0));
    }

    tempFile.deleteFile();
}

TEST (PngImageFormatTests, FileRoundtripSolidColorViaTempFile)
{
    auto original = generateSolidImage (24, 12, PixelFormat::RGBA, 0xCC4488AAu);
    auto tempFile = File::createTempFile (".png");

    {
        auto* fos = tempFile.createOutputStream().release();
        PngImageFormatWriter writer (fos, PixelFormat::RGBA);
        ASSERT_TRUE (writer.writeImage (original));
    }

    {
        auto* fis = tempFile.createInputStream().release();
        PngImageFormatReader reader (fis);
        auto result = reader.readImage();
        ASSERT_TRUE (imagesAreEqual (original, result, 0));
    }

    tempFile.deleteFile();
}

TEST (PngImageFormatTests, SavedFileHasExpectedMinSize)
{
    auto original = generateSolidImage (4, 4, PixelFormat::RGB);
    auto tempFile = File::createTempFile (".png");

    {
        auto* fos = tempFile.createOutputStream().release();
        PngImageFormatWriter writer (fos, PixelFormat::RGB);
        ASSERT_TRUE (writer.writeImage (original));
    }

    EXPECT_GT (tempFile.getSize(), 0);
    tempFile.deleteFile();
}

// ======================================================================
// Image::loadFromData tests
// ======================================================================

TEST (PngImageFormatTests, LoadFromDataRoundtripRgb)
{
    auto original = generateTestImage (12, 12, PixelFormat::RGB);

    auto* rawStream = new MemoryOutputStream();
    PngImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (original));

    const auto* bytes = static_cast<const uint8*> (rawStream->getData());
    const auto size = rawStream->getDataSize();

    auto result = Image::loadFromData (Span<const uint8> (bytes, size));
    ASSERT_TRUE (result.wasOk());

    const Image& decoded = result.getValue();
    EXPECT_EQ (decoded.getWidth(), original.getWidth());
    EXPECT_EQ (decoded.getHeight(), original.getHeight());
    EXPECT_TRUE (imagesAreEqual (original, decoded));
}

TEST (PngImageFormatTests, LoadFromDataRoundtripRgba)
{
    auto original = generateTestImage (8, 8, PixelFormat::RGBA);

    auto* rawStream = new MemoryOutputStream();
    PngImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    ASSERT_TRUE (writer.writeImage (original));

    const auto* bytes = static_cast<const uint8*> (rawStream->getData());
    const auto size = rawStream->getDataSize();

    auto result = Image::loadFromData (Span<const uint8> (bytes, size));
    ASSERT_TRUE (result.wasOk());

    const Image& decoded = result.getValue();
    EXPECT_EQ (decoded.getWidth(), original.getWidth());
    EXPECT_EQ (decoded.getHeight(), original.getHeight());
    EXPECT_TRUE (imagesAreEqual (original, decoded));
}

TEST (PngImageFormatTests, LoadFromDataRoundtripGrayscale)
{
    Image original (6, 6, PixelFormat::Grayscale);
    original.fill (0xFF777777u);

    auto* rawStream = new MemoryOutputStream();
    PngImageFormatWriter writer (rawStream, PixelFormat::Grayscale);
    ASSERT_TRUE (writer.writeImage (original));

    const auto* bytes = static_cast<const uint8*> (rawStream->getData());
    const auto size = rawStream->getDataSize();

    auto result = Image::loadFromData (Span<const uint8> (bytes, size));
    ASSERT_TRUE (result.wasOk());

    const Image& decoded = result.getValue();
    EXPECT_EQ (decoded.getWidth(), original.getWidth());
    EXPECT_EQ (decoded.getHeight(), original.getHeight());
    EXPECT_TRUE (imagesAreEqual (original, decoded));
}

// ======================================================================
// Format property tests
// ======================================================================

TEST (PngImageFormatTests, FormatNameIsCorrect)
{
    PngImageFormat fmt;
    EXPECT_EQ (fmt.getFormatName(), String ("PNG Image"));
}

TEST (PngImageFormatTests, ExtensionsAreCorrect)
{
    PngImageFormat fmt;

    auto readExts = fmt.getFileExtensions (ImageFormat::forReading);
    ASSERT_EQ (readExts.size(), 1);
    EXPECT_EQ (readExts[0], String (".png"));

    auto writeExts = fmt.getFileExtensions (ImageFormat::forWriting);
    ASSERT_EQ (writeExts.size(), 1);
    EXPECT_EQ (writeExts[0], String (".png"));
}

TEST (PngImageFormatTests, PossiblePixelFormatsIncludeAllThree)
{
    PngImageFormat fmt;
    auto formats = fmt.getPossiblePixelFormats();

    EXPECT_EQ (formats.size(), 3);
    EXPECT_TRUE (formats.contains (PixelFormat::Grayscale));
    EXPECT_TRUE (formats.contains (PixelFormat::RGB));
    EXPECT_TRUE (formats.contains (PixelFormat::RGBA));
}

TEST (PngImageFormatTests, IsCompressed)
{
    PngImageFormat fmt;
    EXPECT_TRUE (fmt.isCompressed());
}

TEST (PngImageFormatTests, HasNoQualityOptions)
{
    PngImageFormat fmt;
    EXPECT_EQ (fmt.getQualityOptions().size(), 0);
}

// ======================================================================
// canHandleFile tests
// ======================================================================

TEST (PngImageFormatTests, CanHandleFileForPngExtension)
{
    PngImageFormat fmt;

    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/image.png"), ImageFormat::forReading));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/image.PNG"), ImageFormat::forReading));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/image.png"), ImageFormat::forWriting));
}

TEST (PngImageFormatTests, CanHandleFileRejectsWrongExtensions)
{
    PngImageFormat fmt;

    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image.bmp"), ImageFormat::forReading));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image.jpg"), ImageFormat::forReading));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image.ppm"), ImageFormat::forReading));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image"), ImageFormat::forReading));
}

// ======================================================================
// canHandleStream tests
// ======================================================================

TEST (PngImageFormatTests, CanHandleStreamDetectsPngSignature)
{
    PngImageFormat fmt;

    const uint8 pngHeader[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
    MemoryInputStream stream (pngHeader, std::size (pngHeader), false);

    EXPECT_TRUE (fmt.canHandleStream (stream, ImageFormat::forReading));
    EXPECT_EQ (stream.getPosition(), 0);
}

TEST (PngImageFormatTests, CanHandleStreamRejectsBmpMagic)
{
    PngImageFormat fmt;

    const uint8 bmpHeader[] = { 0x42, 0x4D, 0x00, 0x00 };
    MemoryInputStream stream (bmpHeader, std::size (bmpHeader), false);

    EXPECT_FALSE (fmt.canHandleStream (stream, ImageFormat::forReading));
    EXPECT_EQ (stream.getPosition(), 0);
}

TEST (PngImageFormatTests, CanHandleStreamRejectsNearPngSignature)
{
    PngImageFormat fmt;

    const uint8 data[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0B };
    MemoryInputStream stream (data, std::size (data), false);

    EXPECT_FALSE (fmt.canHandleStream (stream, ImageFormat::forReading));
}

// ======================================================================
// Manager integration tests
// ======================================================================

TEST (PngImageFormatTests, ManagerCreatesReaderForStream)
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
    EXPECT_EQ (reader->width, 4);
    EXPECT_EQ (reader->height, 4);
}

TEST (PngImageFormatTests, ManagerCreatesWriterForPngExtension)
{
    auto tempFile = File::createTempFile (".png");

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto writer = manager.createWriterFor (tempFile, PixelFormat::RGBA);
    ASSERT_NE (writer, nullptr);
    EXPECT_EQ (writer->getFormatName(), String ("PNG Image"));
    EXPECT_EQ (writer->getPixelFormat(), PixelFormat::RGBA);

    tempFile.deleteFile();
}

TEST (PngImageFormatTests, ManagerRoundtripViaFile)
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
        auto result = reader->readImage();
        ASSERT_TRUE (result.isValid());
        EXPECT_EQ (result.getWidth(), original.getWidth());
        EXPECT_EQ (result.getHeight(), original.getHeight());
        EXPECT_TRUE (imagesAreEqual (original, result, 0));
    }

    tempFile.deleteFile();
}

// ======================================================================
// Reader/writer metadata tests
// ======================================================================

TEST (PngImageFormatTests, WriterFlushReturnsTrue)
{
    PngImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB);
    EXPECT_TRUE (writer.flush());
}

TEST (PngImageFormatTests, WriterReturnsCorrectPixelFormat)
{
    {
        PngImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB);
        EXPECT_EQ (writer.getPixelFormat(), PixelFormat::RGB);
    }
    {
        PngImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGBA);
        EXPECT_EQ (writer.getPixelFormat(), PixelFormat::RGBA);
    }
    {
        PngImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::Grayscale);
        EXPECT_EQ (writer.getPixelFormat(), PixelFormat::Grayscale);
    }
}

TEST (PngImageFormatTests, WriterReturnsCorrectFormatName)
{
    PngImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB);
    EXPECT_EQ (writer.getFormatName(), String ("PNG Image"));
}

// ======================================================================
// Edge case and stress tests
// ======================================================================

TEST (PngImageFormatTests, FullyTransparentImageRoundtrip)
{
    auto original = generateSolidImage (8, 8, PixelFormat::RGBA, 0x00000000u);

    auto* rawStream = new MemoryOutputStream();
    PngImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    PngImageFormatReader reader (inStream);
    auto result = reader.readImage();

    ASSERT_TRUE (imagesAreEqual (original, result, 0));
}

TEST (PngImageFormatTests, FullyOpaqueImageRoundtrip)
{
    auto original = generateSolidImage (8, 8, PixelFormat::RGBA, 0xFFFFFFFFu);

    auto* rawStream = new MemoryOutputStream();
    PngImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    PngImageFormatReader reader (inStream);
    auto result = reader.readImage();

    ASSERT_TRUE (imagesAreEqual (original, result, 0));
}

TEST (PngImageFormatTests, GrayscaleBlackAndWhiteRoundtrip)
{
    Image original (4, 4, PixelFormat::Grayscale);
    original.fill (0xFF000000u);
    original.setPixel (2, 2, 0xFFFFFFFFu);

    auto* rawStream = new MemoryOutputStream();
    PngImageFormatWriter writer (rawStream, PixelFormat::Grayscale);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    PngImageFormatReader reader (inStream);
    auto result = reader.readImage();

    EXPECT_TRUE (imagesAreEqual (original, result, 0));
}

#endif // YUP_MODULE_AVAILABLE_libpng && YUP_IMAGE_FORMAT_PNG
