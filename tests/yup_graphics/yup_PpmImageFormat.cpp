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
// ASCII format readImage tests (P1, P2, P3)
// ======================================================================

TEST (PpmImageFormatTests, ReadP1AsciiBitmapProducesCorrectPixels)
{
    // P1 ASCII bitmap: '1' = black, '0' = white
    // 2x2 image: top row white,white; bottom row black,white
    const char data[] = "P1\n2 2\n1 0 0 1\n";
    auto* stream = new MemoryInputStream (data, sizeof (data) - 1, false);

    PpmImageFormatReader reader (stream);
    ASSERT_EQ (reader.width, 2);
    ASSERT_EQ (reader.height, 2);
    ASSERT_EQ (reader.pixelFormat, PixelFormat::Grayscale);

    const Image result = reader.readImage();
    ASSERT_TRUE (result.isValid());

    // '1' → black (0), '0' → white (255)
    EXPECT_EQ (result.getPixel (0, 0), 0xFF000000u); // black
    EXPECT_EQ (result.getPixel (1, 0), 0xFFFFFFFFu); // white
    EXPECT_EQ (result.getPixel (0, 1), 0xFFFFFFFFu); // white
    EXPECT_EQ (result.getPixel (1, 1), 0xFF000000u); // black
}

TEST (PpmImageFormatTests, ReadP2AsciiGrayscaleProducesCorrectPixels)
{
    // P2 ASCII grayscale: values from 0 to maxval
    const char data[] = "P2\n2 2\n255\n0 128 255 64\n";
    auto* stream = new MemoryInputStream (data, sizeof (data) - 1, false);

    PpmImageFormatReader reader (stream);
    ASSERT_EQ (reader.width, 2);
    ASSERT_EQ (reader.height, 2);

    const Image result = reader.readImage();
    ASSERT_TRUE (result.isValid());

    EXPECT_EQ (result.getPixel (0, 0), 0xFF000000u);
    EXPECT_EQ (result.getPixel (1, 0), 0xFF808080u);
    EXPECT_EQ (result.getPixel (0, 1), 0xFFFFFFFFu);
    EXPECT_EQ (result.getPixel (1, 1), 0xFF404040u);
}

TEST (PpmImageFormatTests, ReadP3AsciiRgbProducesCorrectPixels)
{
    // P3 ASCII RGB: R G B triplets
    const char data[] = "P3\n2 2\n255\n255 0 0 0 255 0 0 0 255 128 128 128\n";
    auto* stream = new MemoryInputStream (data, sizeof (data) - 1, false);

    PpmImageFormatReader reader (stream);
    ASSERT_EQ (reader.width, 2);
    ASSERT_EQ (reader.height, 2);
    ASSERT_EQ (reader.pixelFormat, PixelFormat::RGB);

    const Image result = reader.readImage();
    ASSERT_TRUE (result.isValid());

    EXPECT_EQ (result.getPixel (0, 0), 0xFFFF0000u); // red
    EXPECT_EQ (result.getPixel (1, 0), 0xFF00FF00u); // green
    EXPECT_EQ (result.getPixel (0, 1), 0xFF0000FFu); // blue
    EXPECT_EQ (result.getPixel (1, 1), 0xFF808080u); // gray
}

// ======================================================================
// Binary bitmap (P4) readImage test
// ======================================================================

TEST (PpmImageFormatTests, ReadP4BinaryBitmapProducesCorrectPixels)
{
    // P4 binary bitmap: each row packed into ceil(width/8) bytes, MSB first
    // 8x2 image: row0 = alternating B/W, row1 = all black
    // bit 1 = black, bit 0 = white
    // 0b10101010 = 0xAA → W,B,W,B,W,B,W,B
    // 0b11111111 = 0xFF → B,B,B,B,B,B,B,B
    const uint8 rawData[] = {
        'P', '4', '\n', '8', ' ', '2', '\n',
        0xAA, // row 0: alternating
        0xFF  // row 1: all black
    };
    auto* stream = new MemoryInputStream (rawData, sizeof (rawData), false);

    PpmImageFormatReader reader (stream);
    ASSERT_EQ (reader.width, 8);
    ASSERT_EQ (reader.height, 2);

    const Image result = reader.readImage();
    ASSERT_TRUE (result.isValid());

    // Row 0: alternating white/black (bit 1=black, bit 0=white)
    for (int x = 0; x < 8; ++x)
    {
        // 0xAA = 10101010 → MSB first → x=0 sees bit7=1=black, x=1 sees bit6=0=white...
        const uint32 expected = (x % 2 == 0) ? 0xFF000000u : 0xFFFFFFFFu;
        EXPECT_EQ (result.getPixel (x, 0), expected) << "at x=" << x;
    }
    // Row 1: all black
    for (int x = 0; x < 8; ++x)
        EXPECT_EQ (result.getPixel (x, 1), 0xFF000000u);
}

// ======================================================================
// 16-bit readImage tests (P5, P6 with maxval > 255)
// ======================================================================

TEST (PpmImageFormatTests, ReadP5With16BitMaxvalProducesCorrectPixels)
{
    // P5 binary grayscale, 2x1, maxval=65535, two pixels: lo=0xFFFF, hi=0x7FFF
    const uint8 rawData[] = {
        'P', '5', '\n', '2', ' ', '1', '\n', '6', '5', '5', '3', '5', '\n', 0xFF, 0xFF, // pixel 0: 65535 → 255
        0x7F,
        0xFF // pixel 1: 32767 → ~127
    };
    auto* stream = new MemoryInputStream (rawData, sizeof (rawData), false);

    PpmImageFormatReader reader (stream);
    ASSERT_EQ (reader.width, 2);
    ASSERT_EQ (reader.height, 1);

    const Image result = reader.readImage();
    ASSERT_TRUE (result.isValid());

    EXPECT_EQ (result.getPixel (0, 0), 0xFFFFFFFFu); // 65535 normalized → 255
    const uint32 p1 = result.getPixel (1, 0);
    const uint8 p1Gray = static_cast<uint8> (p1 & 0xFF);
    EXPECT_NEAR (p1Gray, 127, 1); // 32767/65535*255 ≈ 127.5
}

TEST (PpmImageFormatTests, ReadP6With16BitMaxvalProducesCorrectPixels)
{
    // P6 binary RGB, 1x1, maxval=65535, one pixel: 65535, 0, 0 (red)
    const uint8 rawData[] = {
        'P', '6', '\n', '1', ' ', '1', '\n', '6', '5', '5', '3', '5', '\n', 0xFF, 0xFF, // R: 65535 → 255
        0x00,
        0x00, // G: 0 → 0
        0x00,
        0x00 // B: 0 → 0
    };
    auto* stream = new MemoryInputStream (rawData, sizeof (rawData), false);

    PpmImageFormatReader reader (stream);
    ASSERT_EQ (reader.width, 1);
    ASSERT_EQ (reader.height, 1);

    const Image result = reader.readImage();
    ASSERT_TRUE (result.isValid());

    EXPECT_EQ (result.getPixel (0, 0), 0xFFFF0000u); // red
}

// ======================================================================
// Invalid maxval / header tests
// ======================================================================

TEST (PpmImageFormatTests, InvalidMaxvalReturnsInvalidImage)
{
    const char data[] = "P5\n2 2\n99999\n";
    auto* stream = new MemoryInputStream (data, sizeof (data) - 1, false);

    PpmImageFormatReader reader (stream);
    EXPECT_EQ (reader.width, 0);
    EXPECT_EQ (reader.height, 0);

    auto result = reader.readImage();
    EXPECT_FALSE (result.isValid());
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
