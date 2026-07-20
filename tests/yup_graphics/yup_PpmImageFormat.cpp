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

// ======================================================================
// Reader header parsing tests
// ======================================================================

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

TEST (PpmImageFormatTests, ReaderParsesP1BitmapHeader)
{
    const char data[] = "P1\n2 3\n";
    auto* stream = new MemoryInputStream (data, sizeof (data) - 1, false);

    PpmImageFormatReader reader (stream);

    EXPECT_EQ (reader.width, 2);
    EXPECT_EQ (reader.height, 3);
    EXPECT_EQ (reader.pixelFormat, PixelFormat::Grayscale);
}

TEST (PpmImageFormatTests, ReaderParsesP2AsciiGrayscaleHeader)
{
    const char data[] = "P2\n4 2\n255\n";
    auto* stream = new MemoryInputStream (data, sizeof (data) - 1, false);

    PpmImageFormatReader reader (stream);

    EXPECT_EQ (reader.width, 4);
    EXPECT_EQ (reader.height, 2);
    EXPECT_EQ (reader.pixelFormat, PixelFormat::Grayscale);
}

TEST (PpmImageFormatTests, ReaderParsesP4BinaryBitmapHeader)
{
    const char data[] = "P4\n8 4\n";
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

TEST (PpmImageFormatTests, TruncatedHeaderReturnsZeroDimensions)
{
    const char data[] = "P6\n";
    auto* stream = new MemoryInputStream (data, sizeof (data) - 1, false);

    PpmImageFormatReader reader (stream);

    EXPECT_EQ (reader.width, 0);
    EXPECT_EQ (reader.height, 0);
}

// ======================================================================
// Roundtrip tests (memory-based)
// ======================================================================

TEST (PpmImageFormatTests, WriteAndReadBackRgbProducesPixelIdenticalImage)
{
    auto original = generateTestImage (16, 16, PixelFormat::RGB);

    auto* rawStream = new MemoryOutputStream();
    PpmImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
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

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    PpmImageFormatReader reader (inStream);
    auto result = reader.readImage();

    EXPECT_EQ (result.getWidth(), original.getWidth());
    EXPECT_EQ (result.getHeight(), original.getHeight());
    EXPECT_EQ (result.getPixelFormat(), original.getPixelFormat());
    EXPECT_TRUE (imagesAreEqual (original, result, 0));
}

TEST (PpmImageFormatTests, RgbaInputWrittenAsRgb)
{
    auto original = generateTestImage (16, 16, PixelFormat::RGBA);

    auto* rawStream = new MemoryOutputStream();
    PpmImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    PpmImageFormatReader reader (inStream);
    auto result = reader.readImage();

    EXPECT_EQ (result.getPixelFormat(), PixelFormat::RGB);
    EXPECT_EQ (result.getWidth(), original.getWidth());
    EXPECT_EQ (result.getHeight(), original.getHeight());

    for (int y = 0; y < original.getHeight(); ++y)
    {
        for (int x = 0; x < original.getWidth(); ++x)
        {
            const auto pa = original.getPixel (x, y);
            const auto pb = result.getPixel (x, y);

            EXPECT_EQ ((pa >> 16) & 0xFF, (pb >> 16) & 0xFF) << "R mismatch at (" << x << ", " << y << ")";
            EXPECT_EQ ((pa >> 8) & 0xFF, (pb >> 8) & 0xFF) << "G mismatch at (" << x << ", " << y << ")";
            EXPECT_EQ ((pa >> 0) & 0xFF, (pb >> 0) & 0xFF) << "B mismatch at (" << x << ", " << y << ")";
        }
    }
}

TEST (PpmImageFormatTests, SolidColorRgbRoundtripIsExact)
{
    auto original = generateSolidImage (20, 10, PixelFormat::RGB, 0xFF663399u);

    auto* rawStream = new MemoryOutputStream();
    PpmImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    PpmImageFormatReader reader (inStream);
    auto result = reader.readImage();

    EXPECT_TRUE (imagesAreEqual (original, result, 0));
}

TEST (PpmImageFormatTests, SolidColorGrayscaleRoundtripIsExact)
{
    Image original (6, 6, PixelFormat::Grayscale);
    original.fill (0xFF888888u);

    auto* rawStream = new MemoryOutputStream();
    PpmImageFormatWriter writer (rawStream, PixelFormat::Grayscale);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    PpmImageFormatReader reader (inStream);
    auto result = reader.readImage();

    EXPECT_TRUE (imagesAreEqual (original, result, 0));
}

TEST (PpmImageFormatTests, VariousSizesRoundtripCorrectly)
{
    const int sizes[][2] = { { 1, 1 }, { 3, 7 }, { 15, 1 }, { 1, 20 }, { 31, 17 } };

    for (auto [w, h] : sizes)
    {
        auto original = generateTestImage (w, h, PixelFormat::RGB);

        auto* rawStream = new MemoryOutputStream();
        PpmImageFormatWriter writer (rawStream, PixelFormat::RGB);
        ASSERT_TRUE (writer.writeImage (original));

        auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
        PpmImageFormatReader reader (inStream);
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

TEST (PpmImageFormatTests, FileRoundtripRgbViaTempFile)
{
    auto original = generateTestImage (12, 10, PixelFormat::RGB);
    auto tempFile = File::createTempFile (".ppm");

    {
        auto* fos = tempFile.createOutputStream().release();
        PpmImageFormatWriter writer (fos, PixelFormat::RGB);
        ASSERT_TRUE (writer.writeImage (original));
    }

    {
        auto* fis = tempFile.createInputStream().release();
        PpmImageFormatReader reader (fis);
        auto result = reader.readImage();

        ASSERT_TRUE (result.isValid());
        EXPECT_EQ (result.getWidth(), original.getWidth());
        EXPECT_EQ (result.getHeight(), original.getHeight());
        EXPECT_TRUE (imagesAreEqual (original, result, 0));
    }

    tempFile.deleteFile();
}

TEST (PpmImageFormatTests, FileRoundtripGrayscaleViaTempFile)
{
    auto original = generateTestImage (8, 8, PixelFormat::Grayscale);
    auto tempFile = File::createTempFile (".pgm");

    {
        auto* fos = tempFile.createOutputStream().release();
        PpmImageFormatWriter writer (fos, PixelFormat::Grayscale);
        ASSERT_TRUE (writer.writeImage (original));
    }

    {
        auto* fis = tempFile.createInputStream().release();
        PpmImageFormatReader reader (fis);
        auto result = reader.readImage();

        ASSERT_TRUE (result.isValid());
        EXPECT_EQ (result.getPixelFormat(), PixelFormat::Grayscale);
        EXPECT_TRUE (imagesAreEqual (original, result, 0));
    }

    tempFile.deleteFile();
}

TEST (PpmImageFormatTests, FileRoundtripSolidColorViaTempFile)
{
    auto original = generateSolidImage (16, 8, PixelFormat::RGB, 0xFFAA7733u);
    auto tempFile = File::createTempFile (".ppm");

    {
        auto* fos = tempFile.createOutputStream().release();
        PpmImageFormatWriter writer (fos, PixelFormat::RGB);
        ASSERT_TRUE (writer.writeImage (original));
    }

    {
        auto* fis = tempFile.createInputStream().release();
        PpmImageFormatReader reader (fis);
        auto result = reader.readImage();
        ASSERT_TRUE (imagesAreEqual (original, result, 0));
    }

    tempFile.deleteFile();
}

TEST (PpmImageFormatTests, SavedFileHasExpectedMinSize)
{
    auto original = generateSolidImage (4, 4, PixelFormat::RGB);
    auto tempFile = File::createTempFile (".ppm");

    {
        auto* fos = tempFile.createOutputStream().release();
        PpmImageFormatWriter writer (fos, PixelFormat::RGB);
        ASSERT_TRUE (writer.writeImage (original));
    }

    EXPECT_GT (tempFile.getSize(), 0);
    tempFile.deleteFile();
}

// ======================================================================
// Image::loadFromData tests
// ======================================================================

TEST (PpmImageFormatTests, LoadFromDataRoundtripRgb)
{
    auto original = generateTestImage (10, 10, PixelFormat::RGB);

    auto* rawStream = new MemoryOutputStream();
    PpmImageFormatWriter writer (rawStream, PixelFormat::RGB);
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

TEST (PpmImageFormatTests, LoadFromDataRoundtripGrayscale)
{
    Image original (6, 6, PixelFormat::Grayscale);
    original.fill (0xFF999999u);

    auto* rawStream = new MemoryOutputStream();
    PpmImageFormatWriter writer (rawStream, PixelFormat::Grayscale);
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

TEST (PpmImageFormatTests, FormatNameIsCorrect)
{
    PpmImageFormat fmt;
    EXPECT_EQ (fmt.getFormatName(), String ("PPM/PGM/PBM Image"));
}

TEST (PpmImageFormatTests, ExtensionsAreCorrect)
{
    PpmImageFormat fmt;

    auto readExts = fmt.getFileExtensions (ImageFormat::forReading);
    ASSERT_EQ (readExts.size(), 3);
    EXPECT_TRUE (readExts.contains (String (".ppm")));
    EXPECT_TRUE (readExts.contains (String (".pgm")));
    EXPECT_TRUE (readExts.contains (String (".pbm")));

    auto writeExts = fmt.getFileExtensions (ImageFormat::forWriting);
    ASSERT_EQ (writeExts.size(), 3);
    EXPECT_TRUE (writeExts.contains (String (".ppm")));
    EXPECT_TRUE (writeExts.contains (String (".pgm")));
    EXPECT_TRUE (writeExts.contains (String (".pbm")));
}

TEST (PpmImageFormatTests, PossiblePixelFormatsIncludeGrayscaleAndRgb)
{
    PpmImageFormat fmt;
    auto formats = fmt.getPossiblePixelFormats();

    EXPECT_EQ (formats.size(), 2);
    EXPECT_TRUE (formats.contains (PixelFormat::Grayscale));
    EXPECT_TRUE (formats.contains (PixelFormat::RGB));
}

TEST (PpmImageFormatTests, IsNotCompressed)
{
    PpmImageFormat fmt;
    EXPECT_FALSE (fmt.isCompressed());
}

TEST (PpmImageFormatTests, HasNoQualityOptions)
{
    PpmImageFormat fmt;
    EXPECT_EQ (fmt.getQualityOptions().size(), 0);
}

// ======================================================================
// canHandleFile tests
// ======================================================================

TEST (PpmImageFormatTests, CanHandleFileForAllExtensions)
{
    PpmImageFormat fmt;

    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/test.ppm"), ImageFormat::forReading));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/test.pgm"), ImageFormat::forReading));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/test.pbm"), ImageFormat::forReading));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/test.PPM"), ImageFormat::forReading));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/test.ppm"), ImageFormat::forWriting));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/test.pgm"), ImageFormat::forWriting));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/test.pbm"), ImageFormat::forWriting));
}

TEST (PpmImageFormatTests, CanHandleFileRejectsWrongExtensions)
{
    PpmImageFormat fmt;

    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/test.bmp"), ImageFormat::forReading));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/test.png"), ImageFormat::forReading));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/test.jpg"), ImageFormat::forReading));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/test"), ImageFormat::forReading));
}

// ======================================================================
// canHandleStream tests
// ======================================================================

TEST (PpmImageFormatTests, CanHandleStreamDetectsAllNetpbmMagics)
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

TEST (PpmImageFormatTests, CanHandleStreamRejectsNonPpmBytes)
{
    PpmImageFormat fmt;

    const uint8 data[] = { 0x42, 0x4D };
    MemoryInputStream stream (data, std::size (data), false);
    EXPECT_FALSE (fmt.canHandleStream (stream, ImageFormat::forReading));
    EXPECT_EQ (stream.getPosition(), 0);
}

TEST (PpmImageFormatTests, CanHandleStreamRejectsEmptyStream)
{
    PpmImageFormat fmt;

    MemoryInputStream stream (static_cast<const void*> (nullptr), 0, false);
    EXPECT_FALSE (fmt.canHandleStream (stream, ImageFormat::forReading));
}

// ======================================================================
// Manager integration tests
// ======================================================================

TEST (PpmImageFormatTests, ManagerCreatesReaderForStream)
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
    EXPECT_EQ (reader->width, 4);
    EXPECT_EQ (reader->height, 4);
}

TEST (PpmImageFormatTests, ManagerCreatesWriterForPpmExtension)
{
    auto tempFile = File::createTempFile (".ppm");

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto writer = manager.createWriterFor (tempFile, PixelFormat::RGB);
    ASSERT_NE (writer, nullptr);
    EXPECT_EQ (writer->getFormatName(), String ("PPM/PGM/PBM Image"));

    tempFile.deleteFile();
}

TEST (PpmImageFormatTests, ManagerCreatesWriterForPgmExtension)
{
    auto tempFile = File::createTempFile (".pgm");

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto writer = manager.createWriterFor (tempFile, PixelFormat::Grayscale);
    ASSERT_NE (writer, nullptr);
    EXPECT_EQ (writer->getFormatName(), String ("PPM/PGM/PBM Image"));

    tempFile.deleteFile();
}

TEST (PpmImageFormatTests, ManagerRoundtripViaFile)
{
    auto original = generateTestImage (8, 8, PixelFormat::RGB);
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

TEST (PpmImageFormatTests, ReaderDpiDefaultsToZero)
{
    auto* rawStream = new MemoryOutputStream();
    PpmImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (4, 4, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    PpmImageFormatReader reader (inStream);

    EXPECT_EQ (reader.metadata, nullptr); // PPM does not support metadata
}

TEST (PpmImageFormatTests, WriterFlushReturnsTrue)
{
    PpmImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB);
    EXPECT_TRUE (writer.flush());
}

TEST (PpmImageFormatTests, WriterReturnsCorrectPixelFormat)
{
    {
        PpmImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB);
        EXPECT_EQ (writer.getPixelFormat(), PixelFormat::RGB);
    }
    {
        PpmImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::Grayscale);
        EXPECT_EQ (writer.getPixelFormat(), PixelFormat::Grayscale);
    }
}

TEST (PpmImageFormatTests, WriterReturnsCorrectFormatName)
{
    PpmImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB);
    EXPECT_EQ (writer.getFormatName(), String ("PPM/PGM/PBM Image"));
}

// ======================================================================
// Invalid image writeImage
// ======================================================================

TEST (PpmImageFormatTests, WriteImageReturnsFalseForInvalidImage)
{
    auto* rawStream = new MemoryOutputStream();
    PpmImageFormatWriter writer (rawStream, PixelFormat::RGB);

    Image invalid;
    EXPECT_FALSE (writer.writeImage (invalid));
}

// ======================================================================
// Selective registration tests
// ======================================================================

TEST (PpmImageFormatTests, SelectiveRegistrationPpmOnly)
{
    ImageFormatManager manager;
    manager.registerDefaultFormats (ImageFormatType::ppm);

    auto ppmFile = File::createTempFile (".ppm");
    auto bmpFile = File::createTempFile (".bmp");

    EXPECT_NE (manager.createWriterFor (ppmFile, PixelFormat::RGB), nullptr);
    EXPECT_EQ (manager.createWriterFor (bmpFile, PixelFormat::RGB), nullptr);

    ppmFile.deleteFile();
    bmpFile.deleteFile();
}
