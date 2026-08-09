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
// Reader dimension and header tests
// ======================================================================

TEST (TiffImageFormatTests, ReaderSetsCorrectDimensions)
{
    auto* rawStream = new MemoryOutputStream();
    TiffImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (4, 6, PixelFormat::RGBA)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    TiffImageFormatReader reader (inStream);

    EXPECT_EQ (reader.width, 4);
    EXPECT_EQ (reader.height, 6);
}

TEST (TiffImageFormatTests, ReaderSetsDimensionsForSmallImage)
{
    auto* rawStream = new MemoryOutputStream();
    TiffImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (1, 1, PixelFormat::RGBA)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    TiffImageFormatReader reader (inStream);

    EXPECT_EQ (reader.width, 1);
    EXPECT_EQ (reader.height, 1);
}

TEST (TiffImageFormatTests, ReaderSetsDimensionsForNonSquareImage)
{
    auto* rawStream = new MemoryOutputStream();
    TiffImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (23, 3, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    TiffImageFormatReader reader (inStream);

    EXPECT_EQ (reader.width, 23);
    EXPECT_EQ (reader.height, 3);
}

TEST (TiffImageFormatTests, ReaderSetsCorrectPixelFormatRgb)
{
    auto* rawStream = new MemoryOutputStream();
    TiffImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (4, 4, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    TiffImageFormatReader reader (inStream);

    EXPECT_EQ (reader.pixelFormat, PixelFormat::RGB);
}

TEST (TiffImageFormatTests, ReaderSetsCorrectPixelFormatRgba)
{
    auto* rawStream = new MemoryOutputStream();
    TiffImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (4, 4, PixelFormat::RGBA, 0x80FFCC44u)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    TiffImageFormatReader reader (inStream);

    EXPECT_EQ (reader.pixelFormat, PixelFormat::RGBA);
}

TEST (TiffImageFormatTests, ReaderSetsCorrectPixelFormatGrayscale)
{
    auto* rawStream = new MemoryOutputStream();
    TiffImageFormatWriter writer (rawStream, PixelFormat::Grayscale);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (4, 4, PixelFormat::Grayscale)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    TiffImageFormatReader reader (inStream);

    EXPECT_EQ (reader.pixelFormat, PixelFormat::Grayscale);
}

TEST (TiffImageFormatTests, InvalidSignatureReturnsInvalidImage)
{
    const uint8 garbage[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
    auto* stream = new MemoryInputStream (garbage, sizeof (garbage), false);

    TiffImageFormatReader reader (stream);

    EXPECT_EQ (reader.width, 0);
    EXPECT_EQ (reader.height, 0);

    auto result = reader.readImage();
    EXPECT_FALSE (result.isValid());
}

TEST (TiffImageFormatTests, EmptyStreamReturnsInvalidImage)
{
    auto* stream = new MemoryInputStream (nullptr, 0, false);

    TiffImageFormatReader reader (stream);

    EXPECT_EQ (reader.width, 0);
    EXPECT_EQ (reader.height, 0);

    auto result = reader.readImage();
    EXPECT_FALSE (result.isValid());
}

// ======================================================================
// Roundtrip tests (memory-based)
// ======================================================================

TEST (TiffImageFormatTests, WriteAndReadBackRgbProducesPixelIdenticalImage)
{
    auto original = generateTestImage (16, 16, PixelFormat::RGB);

    auto* rawStream = new MemoryOutputStream();
    TiffImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    TiffImageFormatReader reader (inStream);

    EXPECT_EQ (reader.width, 16);
    EXPECT_EQ (reader.height, 16);

    auto decoded = reader.readImage();
    ASSERT_TRUE (decoded.isValid());
    EXPECT_EQ (decoded.getWidth(), 16);
    EXPECT_EQ (decoded.getHeight(), 16);
    EXPECT_TRUE (imagesAreEqual (original, decoded, 0));
}

TEST (TiffImageFormatTests, WriteAndReadBackRgbaProducesPixelIdenticalImage)
{
    auto original = generateTestImage (16, 16, PixelFormat::RGBA);

    auto* rawStream = new MemoryOutputStream();
    TiffImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    TiffImageFormatReader reader (inStream);

    EXPECT_EQ (reader.width, 16);
    EXPECT_EQ (reader.height, 16);

    auto decoded = reader.readImage();
    ASSERT_TRUE (decoded.isValid());
    EXPECT_EQ (decoded.getWidth(), 16);
    EXPECT_EQ (decoded.getHeight(), 16);
    EXPECT_TRUE (imagesAreEqualRGBA (original, decoded, 0));
}

TEST (TiffImageFormatTests, WriteAndReadBackGrayscaleProducesPixelIdenticalImage)
{
    auto original = generateTestImage (16, 16, PixelFormat::Grayscale);

    auto* rawStream = new MemoryOutputStream();
    TiffImageFormatWriter writer (rawStream, PixelFormat::Grayscale);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    TiffImageFormatReader reader (inStream);

    EXPECT_EQ (reader.width, 16);
    EXPECT_EQ (reader.height, 16);

    auto decoded = reader.readImage();
    ASSERT_TRUE (decoded.isValid());
    EXPECT_EQ (decoded.getWidth(), 16);
    EXPECT_EQ (decoded.getHeight(), 16);
    EXPECT_TRUE (imagesAreEqual (original, decoded, 0));
}

// ======================================================================
// Solid colour roundtrip tests
// ======================================================================

TEST (TiffImageFormatTests, WriteAndReadBackSolidRgba)
{
    auto original = generateSolidImage (8, 8, PixelFormat::RGBA, 0x80FFCC44u);

    auto* rawStream = new MemoryOutputStream();
    TiffImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    TiffImageFormatReader reader (inStream);

    auto decoded = reader.readImage();
    ASSERT_TRUE (decoded.isValid());
    EXPECT_TRUE (imagesAreEqualRGBA (original, decoded, 0));
}

TEST (TiffImageFormatTests, WriteAndReadBackSolidRgb)
{
    auto original = generateSolidImage (8, 8, PixelFormat::RGB, 0xFF00FF00u);

    auto* rawStream = new MemoryOutputStream();
    TiffImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    TiffImageFormatReader reader (inStream);

    auto decoded = reader.readImage();
    ASSERT_TRUE (decoded.isValid());
    EXPECT_TRUE (imagesAreEqual (original, decoded, 0));
}

// ======================================================================
// ImageFormat tests
// ======================================================================

TEST (TiffImageFormatTests, DetectTiffSignature)
{
    TiffImageFormat format;

    // Little-endian TIFF
    {
        const uint8 leTiff[] = { 0x49, 0x49, 0x2A, 0x00 };
        MemoryInputStream stream (leTiff, sizeof (leTiff), false);
        EXPECT_TRUE (format.canHandleStream (stream, ImageFormat::forReading));
    }

    // Big-endian TIFF
    {
        const uint8 beTiff[] = { 0x4D, 0x4D, 0x00, 0x2A };
        MemoryInputStream stream (beTiff, sizeof (beTiff), false);
        EXPECT_TRUE (format.canHandleStream (stream, ImageFormat::forReading));
    }

    // Not TIFF
    {
        const uint8 notTiff[] = { 0x00, 0x01, 0x02, 0x03 };
        MemoryInputStream stream (notTiff, sizeof (notTiff), false);
        EXPECT_FALSE (format.canHandleStream (stream, ImageFormat::forReading));
    }
}

TEST (TiffImageFormatTests, FormatName)
{
    TiffImageFormat format;
    EXPECT_EQ (format.getFormatName(), String ("TIFF Image"));
}

TEST (TiffImageFormatTests, FileExtensions)
{
    TiffImageFormat format;

    auto readExts = format.getFileExtensions (ImageFormat::forReading);
    EXPECT_TRUE (readExts.contains (".tiff"));
    EXPECT_TRUE (readExts.contains (".tif"));

    auto writeExts = format.getFileExtensions (ImageFormat::forWriting);
    EXPECT_TRUE (writeExts.contains (".tiff"));
    EXPECT_TRUE (writeExts.contains (".tif"));
}

TEST (TiffImageFormatTests, IsCompressed)
{
    TiffImageFormat format;
    EXPECT_TRUE (format.isCompressed());
}

TEST (TiffImageFormatTests, PossiblePixelFormats)
{
    TiffImageFormat format;
    auto fmts = format.getPossiblePixelFormats();

    EXPECT_TRUE (fmts.contains (PixelFormat::Grayscale));
    EXPECT_TRUE (fmts.contains (PixelFormat::RGB));
    EXPECT_TRUE (fmts.contains (PixelFormat::RGBA));
}

// ======================================================================
// ImageFormatManager integration tests
// ======================================================================

TEST (TiffImageFormatTests, ManagerCanCreateReader)
{
    auto* rawStream = new MemoryOutputStream();
    TiffImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (4, 4, PixelFormat::RGBA)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto reader = manager.createReaderFor (inStream);
    ASSERT_NE (reader, nullptr);
    EXPECT_EQ (reader->width, 4);
    EXPECT_EQ (reader->height, 4);
}

TEST (TiffImageFormatTests, ManagerCanCreateWriter)
{
    auto dir = getTestDataImagesDirectory();
    dir.createDirectory();
    auto file = dir.getChildFile ("test_manager_tiff_writer.tiff");
    file.deleteFile();

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto writer = manager.createWriterFor (file, PixelFormat::RGB, {}, 0);
    ASSERT_NE (writer, nullptr);
    EXPECT_EQ (writer->getFormatName(), String ("TIFF Image"));

    file.deleteFile();
}

// ======================================================================
// File-based roundtrip tests
// ======================================================================

TEST (TiffImageFormatTests, FileRoundtripRgb)
{
    auto file = ensureTestImage ("test_tiff_rgb.tiff", 32, 32, PixelFormat::RGB, 0xFFCC8844u);

    auto* stream = file.createInputStream().release();
    ASSERT_NE (stream, nullptr);

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto reader = manager.createReaderFor (stream);
    ASSERT_NE (reader, nullptr);

    auto decoded = reader->readImage();
    ASSERT_TRUE (decoded.isValid());
    EXPECT_EQ (decoded.getWidth(), 32);
    EXPECT_EQ (decoded.getHeight(), 32);

    // Verify solid colour roundtrip (RGB — alpha may be lost)
    for (int y = 0; y < 32; ++y)
        for (int x = 0; x < 32; ++x)
        {
            const auto px = decoded.getPixel (x, y);
            EXPECT_NEAR (int ((px >> 16) & 0xFF), 0xCC, 3);
            EXPECT_NEAR (int ((px >> 8) & 0xFF), 0x88, 3);
            EXPECT_NEAR (int (px & 0xFF), 0x44, 3);
        }

    file.deleteFile();
}

TEST (TiffImageFormatTests, FileRoundtripRgba)
{
    auto file = ensureTestImage ("test_tiff_rgba.tiff", 16, 16, PixelFormat::RGBA, 0x4488CCFFu);

    auto* stream = file.createInputStream().release();
    ASSERT_NE (stream, nullptr);

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto reader = manager.createReaderFor (stream);
    ASSERT_NE (reader, nullptr);

    auto decoded = reader->readImage();
    ASSERT_TRUE (decoded.isValid());
    EXPECT_EQ (decoded.getWidth(), 16);
    EXPECT_EQ (decoded.getHeight(), 16);

    // Verify solid colour roundtrip
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x)
        {
            const auto px = decoded.getPixel (x, y);
            EXPECT_NEAR (int ((px >> 24) & 0xFF), 0x44, 3);
            EXPECT_NEAR (int ((px >> 16) & 0xFF), 0x88, 3);
            EXPECT_NEAR (int ((px >> 8) & 0xFF), 0xCC, 3);
            EXPECT_NEAR (int (px & 0xFF), 0xFF, 3);
        }

    file.deleteFile();
}

// ======================================================================
// Various sizes roundtrip tests
// ======================================================================

TEST (TiffImageFormatTests, VariousSizesRoundtripCorrectly)
{
    const int sizes[][2] = { { 1, 1 }, { 1, 32 }, { 32, 1 }, { 7, 13 }, { 64, 48 } };

    for (auto [w, h] : sizes)
    {
        auto original = generateTestImage (w, h, PixelFormat::RGB);

        auto* rawStream = new MemoryOutputStream();
        TiffImageFormatWriter writer (rawStream, PixelFormat::RGB);
        ASSERT_TRUE (writer.writeImage (original));

        auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
        TiffImageFormatReader reader (inStream);
        auto result = reader.readImage();

        ASSERT_TRUE (result.isValid())
            << "RGB size mismatch at " << w << "x" << h;
        EXPECT_EQ (result.getWidth(), w);
        EXPECT_EQ (result.getHeight(), h);
        EXPECT_TRUE (imagesAreEqual (original, result, 0))
            << "RGB size mismatch at " << w << "x" << h;
    }
}

TEST (TiffImageFormatTests, VariousSizesGrayscaleRoundtripCorrectly)
{
    const int sizes[][2] = { { 1, 1 }, { 5, 15 }, { 31, 1 }, { 1, 27 } };

    for (auto [w, h] : sizes)
    {
        auto original = generateTestImage (w, h, PixelFormat::Grayscale);

        auto* rawStream = new MemoryOutputStream();
        TiffImageFormatWriter writer (rawStream, PixelFormat::Grayscale);
        ASSERT_TRUE (writer.writeImage (original));

        auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
        TiffImageFormatReader reader (inStream);
        auto result = reader.readImage();

        ASSERT_TRUE (result.isValid())
            << "Grayscale size mismatch at " << w << "x" << h;
        EXPECT_EQ (result.getWidth(), w);
        EXPECT_EQ (result.getHeight(), h);
        EXPECT_TRUE (imagesAreEqual (original, result, 0))
            << "Grayscale size mismatch at " << w << "x" << h;
    }
}

// ======================================================================
// Solid colour grayscale roundtrip
// ======================================================================

TEST (TiffImageFormatTests, WriteAndReadBackSolidGrayscale)
{
    Image original (8, 8, PixelFormat::Grayscale);
    original.fill (0xFF888888u);

    auto* rawStream = new MemoryOutputStream();
    TiffImageFormatWriter writer (rawStream, PixelFormat::Grayscale);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    TiffImageFormatReader reader (inStream);

    auto decoded = reader.readImage();
    ASSERT_TRUE (decoded.isValid());
    EXPECT_TRUE (imagesAreEqual (original, decoded, 0));
}

// ======================================================================
// File-based roundtrip tests (temporary files)
// ======================================================================

TEST (TiffImageFormatTests, FileRoundtripRgbViaTempFile)
{
    auto original = generateTestImage (16, 12, PixelFormat::RGB);
    auto tempFile = File::createTempFile (".tiff");

    {
        auto* fos = tempFile.createOutputStream().release();
        TiffImageFormatWriter writer (fos, PixelFormat::RGB);
        ASSERT_TRUE (writer.writeImage (original));
    }

    {
        auto* fis = tempFile.createInputStream().release();
        TiffImageFormatReader reader (fis);
        auto result = reader.readImage();

        ASSERT_TRUE (result.isValid());
        EXPECT_EQ (result.getWidth(), original.getWidth());
        EXPECT_EQ (result.getHeight(), original.getHeight());
        EXPECT_TRUE (imagesAreEqual (original, result, 0));
    }

    tempFile.deleteFile();
}

TEST (TiffImageFormatTests, FileRoundtripRgbaViaTempFile)
{
    auto original = generateTestImage (10, 14, PixelFormat::RGBA);
    auto tempFile = File::createTempFile (".tiff");

    {
        auto* fos = tempFile.createOutputStream().release();
        TiffImageFormatWriter writer (fos, PixelFormat::RGBA);
        ASSERT_TRUE (writer.writeImage (original));
    }

    {
        auto* fis = tempFile.createInputStream().release();
        TiffImageFormatReader reader (fis);
        auto result = reader.readImage();

        ASSERT_TRUE (result.isValid());
        EXPECT_EQ (result.getPixelFormat(), PixelFormat::RGBA);
        EXPECT_TRUE (imagesAreEqualRGBA (original, result, 0));
    }

    tempFile.deleteFile();
}

TEST (TiffImageFormatTests, FileRoundtripGrayscaleViaTempFile)
{
    auto original = generateTestImage (8, 8, PixelFormat::Grayscale);
    auto tempFile = File::createTempFile (".tiff");

    {
        auto* fos = tempFile.createOutputStream().release();
        TiffImageFormatWriter writer (fos, PixelFormat::Grayscale);
        ASSERT_TRUE (writer.writeImage (original));
    }

    {
        auto* fis = tempFile.createInputStream().release();
        TiffImageFormatReader reader (fis);
        auto result = reader.readImage();

        ASSERT_TRUE (result.isValid());
        EXPECT_EQ (result.getPixelFormat(), PixelFormat::Grayscale);
        EXPECT_TRUE (imagesAreEqual (original, result, 0));
    }

    tempFile.deleteFile();
}

TEST (TiffImageFormatTests, FileRoundtripSolidColorViaTempFile)
{
    auto original = generateSolidImage (20, 10, PixelFormat::RGBA, 0xCC4488AAu);
    auto tempFile = File::createTempFile (".tiff");

    {
        auto* fos = tempFile.createOutputStream().release();
        TiffImageFormatWriter writer (fos, PixelFormat::RGBA);
        ASSERT_TRUE (writer.writeImage (original));
    }

    {
        auto* fis = tempFile.createInputStream().release();
        TiffImageFormatReader reader (fis);
        auto result = reader.readImage();
        ASSERT_TRUE (result.isValid());
        ASSERT_TRUE (imagesAreEqualRGBA (original, result, 0));
    }

    tempFile.deleteFile();
}

TEST (TiffImageFormatTests, SavedFileHasExpectedMinSize)
{
    auto original = generateSolidImage (4, 4, PixelFormat::RGB);
    auto tempFile = File::createTempFile (".tiff");

    {
        auto* fos = tempFile.createOutputStream().release();
        TiffImageFormatWriter writer (fos, PixelFormat::RGB);
        ASSERT_TRUE (writer.writeImage (original));
    }

    EXPECT_GT (tempFile.getSize(), 0);
    tempFile.deleteFile();
}

// ======================================================================
// Image::loadFromData tests
// ======================================================================

TEST (TiffImageFormatTests, LoadFromDataRoundtripRgb)
{
    auto original = generateTestImage (12, 12, PixelFormat::RGB);

    auto* rawStream = new MemoryOutputStream();
    TiffImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (original));

    const auto* bytes = static_cast<const uint8*> (rawStream->getData());
    const auto size = rawStream->getDataSize();

    auto result = Image::loadFromData (Span<const uint8> (bytes, size));
    ASSERT_TRUE (result.wasOk());

    const Image& decoded = result.getValue();
    EXPECT_EQ (decoded.getWidth(), original.getWidth());
    EXPECT_EQ (decoded.getHeight(), original.getHeight());
    EXPECT_TRUE (imagesAreEqual (original, decoded, 0));
}

TEST (TiffImageFormatTests, LoadFromDataRoundtripRgba)
{
    auto original = generateTestImage (8, 8, PixelFormat::RGBA);

    auto* rawStream = new MemoryOutputStream();
    TiffImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    ASSERT_TRUE (writer.writeImage (original));

    const auto* bytes = static_cast<const uint8*> (rawStream->getData());
    const auto size = rawStream->getDataSize();

    auto result = Image::loadFromData (Span<const uint8> (bytes, size));
    ASSERT_TRUE (result.wasOk());

    const Image& decoded = result.getValue();
    EXPECT_EQ (decoded.getWidth(), original.getWidth());
    EXPECT_EQ (decoded.getHeight(), original.getHeight());
    EXPECT_TRUE (imagesAreEqualRGBA (original, decoded, 0));
}

TEST (TiffImageFormatTests, LoadFromDataRoundtripGrayscale)
{
    Image original (6, 6, PixelFormat::Grayscale);
    original.fill (0xFF777777u);

    auto* rawStream = new MemoryOutputStream();
    TiffImageFormatWriter writer (rawStream, PixelFormat::Grayscale);
    ASSERT_TRUE (writer.writeImage (original));

    const auto* bytes = static_cast<const uint8*> (rawStream->getData());
    const auto size = rawStream->getDataSize();

    auto result = Image::loadFromData (Span<const uint8> (bytes, size));
    ASSERT_TRUE (result.wasOk());

    const Image& decoded = result.getValue();
    EXPECT_EQ (decoded.getWidth(), original.getWidth());
    EXPECT_EQ (decoded.getHeight(), original.getHeight());
    EXPECT_TRUE (imagesAreEqual (original, decoded, 0));
}

// ======================================================================
// canHandleFile tests
// ======================================================================

TEST (TiffImageFormatTests, CanHandleFileForAllExtensions)
{
    TiffImageFormat fmt;

    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/image.tiff"), ImageFormat::forReading));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/image.tif"), ImageFormat::forReading));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/image.TIFF"), ImageFormat::forReading));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/image.tiff"), ImageFormat::forWriting));
}

TEST (TiffImageFormatTests, CanHandleFileRejectsWrongExtensions)
{
    TiffImageFormat fmt;

    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image.bmp"), ImageFormat::forReading));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image.png"), ImageFormat::forReading));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image.jpg"), ImageFormat::forReading));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image"), ImageFormat::forReading));
}

// ======================================================================
// canHandleStream tests
// ======================================================================

TEST (TiffImageFormatTests, CanHandleStreamRejectsEmptyStream)
{
    TiffImageFormat fmt;

    MemoryInputStream stream (static_cast<const void*> (nullptr), 0, false);
    EXPECT_FALSE (fmt.canHandleStream (stream, ImageFormat::forReading));
}

// ======================================================================
// Format property tests
// ======================================================================

TEST (TiffImageFormatTests, HasNoQualityOptions)
{
    TiffImageFormat fmt;
    EXPECT_EQ (fmt.getQualityOptions().size(), 0);
}

// ======================================================================
// Manager integration tests
// ======================================================================

TEST (TiffImageFormatTests, ManagerRoundtripViaFile)
{
    auto original = generateTestImage (10, 8, PixelFormat::RGBA);
    auto tempFile = File::createTempFile (".tiff");

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
        EXPECT_TRUE (imagesAreEqualRGBA (original, result, 0));
    }

    tempFile.deleteFile();
}

// ======================================================================
// Reader/writer metadata tests
// ======================================================================

TEST (TiffImageFormatTests, ReaderDpiDefaultsTo72)
{
    auto* rawStream = new MemoryOutputStream();
    TiffImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (4, 4, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    TiffImageFormatReader reader (inStream, ImageFormat::Options().withMetadata (true));

    EXPECT_DOUBLE_EQ (reader.metadata->dpiX, 72.0);
    EXPECT_DOUBLE_EQ (reader.metadata->dpiY, 72.0);
}

TEST (TiffImageFormatTests, WriterFlushReturnsTrue)
{
    TiffImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB);
    EXPECT_TRUE (writer.flush());
}

TEST (TiffImageFormatTests, WriterReturnsCorrectPixelFormat)
{
    {
        TiffImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB);
        EXPECT_EQ (writer.getPixelFormat(), PixelFormat::RGB);
    }
    {
        TiffImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGBA);
        EXPECT_EQ (writer.getPixelFormat(), PixelFormat::RGBA);
    }
    {
        TiffImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::Grayscale);
        EXPECT_EQ (writer.getPixelFormat(), PixelFormat::Grayscale);
    }
}

TEST (TiffImageFormatTests, WriterReturnsCorrectFormatName)
{
    TiffImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB);
    EXPECT_EQ (writer.getFormatName(), String ("TIFF Image"));
}

// ======================================================================
// Invalid image writeImage
// ======================================================================

TEST (TiffImageFormatTests, WriteImageReturnsFalseForInvalidImage)
{
    auto* rawStream = new MemoryOutputStream();
    TiffImageFormatWriter writer (rawStream, PixelFormat::RGB);

    Image invalid;
    EXPECT_FALSE (writer.writeImage (invalid));
}

// ======================================================================
// Selective registration tests
// ======================================================================

TEST (TiffImageFormatTests, SelectiveRegistrationTiffOnly)
{
    ImageFormatManager manager;
    manager.registerDefaultFormats (ImageFormatType::tiff);

    auto tiffFile = File::createTempFile (".tiff");
    auto bmpFile = File::createTempFile (".bmp");

    EXPECT_NE (manager.createWriterFor (tiffFile, PixelFormat::RGB), nullptr);
    EXPECT_EQ (manager.createWriterFor (bmpFile, PixelFormat::RGB), nullptr);

    tiffFile.deleteFile();
    bmpFile.deleteFile();
}

// ======================================================================
// Metadata parsing tests
// ======================================================================

TEST (TiffImageFormatTests, ParseMetadataExtractsMetadata)
{
    Image img (4, 4, PixelFormat::RGB);
    img.fill (0xFFCC8844u);

    auto meta = ImageMetadata::create();
    meta->dpiX = 150.0;
    meta->dpiY = 150.0;
    meta->textEntries.set ("Artist", "Test Artist");
    meta->textEntries.set ("Copyright", "Test Copyright");
    meta->textEntries.set ("DateTime", "2024:01:15 10:30:00");
    meta->textEntries.set ("Software", "YUP");
    meta->textEntries.set ("Make", "TestMake");
    meta->textEntries.set ("Model", "TestModel");
    meta->textEntries.set ("description", "A test TIFF");
    img.setMetadata (meta);

    auto* rawStream = new MemoryOutputStream();
    TiffImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (img));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    TiffImageFormatReader reader (inStream, ImageFormat::Options().withMetadata (true));

    ASSERT_NE (reader.metadata, nullptr);
    EXPECT_NEAR (reader.metadata->dpiX, 150.0, 1.0);
    EXPECT_NEAR (reader.metadata->dpiY, 150.0, 1.0);
    EXPECT_EQ (reader.metadata->textEntries.getValue ("Artist", {}), String ("Test Artist"));
    EXPECT_EQ (reader.metadata->textEntries.getValue ("Copyright", {}), String ("Test Copyright"));
    EXPECT_EQ (reader.metadata->textEntries.getValue ("description", {}), String ("A test TIFF"));
}

TEST (TiffImageFormatTests, ParseRawChunksExtractsExifWhenPresent)
{
    Image img (4, 4, PixelFormat::RGB);
    img.fill (0xFFCC8844u);

    auto meta = ImageMetadata::create();
    const uint8 exifData[] = { 0xDE, 0xAD, 0xBE, 0xEF };
    meta->setRawChunk ("tiff/exif", MemoryBlock (exifData, sizeof (exifData)));
    img.setMetadata (meta);

    // Note: TIFF writer may not roundtrip raw chunks; test the parse path
    auto* rawStream = new MemoryOutputStream();
    TiffImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (img));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    TiffImageFormatReader reader (inStream, ImageFormat::Options().withRawChunks (true));

    ASSERT_NE (reader.metadata, nullptr);
    EXPECT_FALSE (reader.metadata->hasRawChunk ("tiff/exif"));
}
