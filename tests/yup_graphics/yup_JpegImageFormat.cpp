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

#if YUP_MODULE_AVAILABLE_libjpeg && YUP_IMAGE_FORMAT_JPEG

// ======================================================================
// Reader dimension and header tests
// ======================================================================

TEST (JpegImageFormatTests, ReaderSetsCorrectWidthAndHeight)
{
    auto* rawStream = new MemoryOutputStream();
    JpegImageFormatWriter writer (rawStream, PixelFormat::RGB, 0);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (5, 7, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    JpegImageFormatReader reader (inStream);

    EXPECT_EQ (reader.width, 5);
    EXPECT_EQ (reader.height, 7);
    EXPECT_EQ (reader.pixelFormat, PixelFormat::RGB);
}

TEST (JpegImageFormatTests, ReaderSetsDimensionsForSmallImage)
{
    auto* rawStream = new MemoryOutputStream();
    JpegImageFormatWriter writer (rawStream, PixelFormat::RGB, 0);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (1, 1, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    JpegImageFormatReader reader (inStream);

    EXPECT_EQ (reader.width, 1);
    EXPECT_EQ (reader.height, 1);
}

TEST (JpegImageFormatTests, ReaderSetsDimensionsForNonSquareImage)
{
    auto* rawStream = new MemoryOutputStream();
    JpegImageFormatWriter writer (rawStream, PixelFormat::Grayscale, 0);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (19, 5, PixelFormat::Grayscale)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    JpegImageFormatReader reader (inStream);

    EXPECT_EQ (reader.width, 19);
    EXPECT_EQ (reader.height, 5);
    EXPECT_EQ (reader.pixelFormat, PixelFormat::Grayscale);
}

TEST (JpegImageFormatTests, InvalidSignatureReturnsInvalidImage)
{
    const uint8 garbage[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
    auto* stream = new MemoryInputStream (garbage, sizeof (garbage), false);

    JpegImageFormatReader reader (stream);

    EXPECT_EQ (reader.width, 0);
    EXPECT_EQ (reader.height, 0);

    auto result = reader.readImage();
    EXPECT_FALSE (result.isValid());
}

// ======================================================================
// Roundtrip tests (memory-based)
// ======================================================================

TEST (JpegImageFormatTests, WriteAndReadBackRgbPreservesDimensionsAndApproximatePixels)
{
    Image original (16, 16, PixelFormat::RGB);
    original.fill (0xFF335577u);

    auto* rawStream = new MemoryOutputStream();
    JpegImageFormatWriter writer (rawStream, PixelFormat::RGB, 0);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    JpegImageFormatReader reader (inStream);
    auto result = reader.readImage();

    EXPECT_EQ (result.getWidth(), original.getWidth());
    EXPECT_EQ (result.getHeight(), original.getHeight());
    EXPECT_EQ (result.getPixelFormat(), PixelFormat::RGB);
    EXPECT_TRUE (imagesAreEqual (original, result, 3));
}

TEST (JpegImageFormatTests, WriteAndReadBackGrayscalePreservesPixelFormat)
{
    Image original (8, 8, PixelFormat::Grayscale);
    original.fill (0xFF777777u);

    auto* rawStream = new MemoryOutputStream();
    JpegImageFormatWriter writer (rawStream, PixelFormat::Grayscale, 0);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    JpegImageFormatReader reader (inStream);
    auto result = reader.readImage();

    EXPECT_EQ (result.getWidth(), original.getWidth());
    EXPECT_EQ (result.getHeight(), original.getHeight());
    EXPECT_EQ (result.getPixelFormat(), PixelFormat::Grayscale);
    EXPECT_TRUE (imagesAreEqual (original, result, 3));
}

TEST (JpegImageFormatTests, RgbaInputWrittenAsRgbLostAlpha)
{
    Image original (16, 16, PixelFormat::RGBA);
    original.fill (0x80FF0000u);

    auto* rawStream = new MemoryOutputStream();
    JpegImageFormatWriter writer (rawStream, PixelFormat::RGBA, 0);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    JpegImageFormatReader reader (inStream);
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

            EXPECT_NEAR (int ((pa >> 16) & 0xFF), int ((pb >> 16) & 0xFF), 3) << "R mismatch at (" << x << ", " << y << ")";
            EXPECT_NEAR (int ((pa >> 8) & 0xFF), int ((pb >> 8) & 0xFF), 3) << "G mismatch at (" << x << ", " << y << ")";
            EXPECT_NEAR (int ((pa >> 0) & 0xFF), int ((pb >> 0) & 0xFF), 3) << "B mismatch at (" << x << ", " << y << ")";
        }
    }
}

TEST (JpegImageFormatTests, VariousSizesRoundtripApproximately)
{
    const int sizes[][2] = { { 8, 8 }, { 32, 32 }, { 7, 13 }, { 16, 4 } };

    for (auto [w, h] : sizes)
    {
        Image original (w, h, PixelFormat::RGB);
        original.fill (0xFF664488u);

        auto* rawStream = new MemoryOutputStream();
        JpegImageFormatWriter writer (rawStream, PixelFormat::RGB, 0);
        ASSERT_TRUE (writer.writeImage (original));

        auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
        JpegImageFormatReader reader (inStream);
        auto result = reader.readImage();

        EXPECT_EQ (result.getWidth(), w);
        EXPECT_EQ (result.getHeight(), h);
        EXPECT_TRUE (imagesAreEqual (original, result, 3))
            << "Size mismatch at " << w << "x" << h;
    }
}

// ======================================================================
// File-based roundtrip tests (temporary files)
// ======================================================================

TEST (JpegImageFormatTests, FileRoundtripRgbViaTempFile)
{
    Image original (20, 12, PixelFormat::RGB);
    original.fill (0xFF335588u);
    auto tempFile = File::createTempFile (".jpg");

    {
        auto* fos = tempFile.createOutputStream().release();
        JpegImageFormatWriter writer (fos, PixelFormat::RGB, 0);
        ASSERT_TRUE (writer.writeImage (original));
    }

    {
        auto* fis = tempFile.createInputStream().release();
        JpegImageFormatReader reader (fis);
        auto result = reader.readImage();

        ASSERT_TRUE (result.isValid());
        EXPECT_EQ (result.getWidth(), original.getWidth());
        EXPECT_EQ (result.getHeight(), original.getHeight());
        EXPECT_TRUE (imagesAreEqual (original, result, 3));
    }

    tempFile.deleteFile();
}

TEST (JpegImageFormatTests, FileRoundtripGrayscaleViaTempFile)
{
    Image original (12, 12, PixelFormat::Grayscale);
    original.fill (0xFF888888u);
    auto tempFile = File::createTempFile (".jpg");

    {
        auto* fos = tempFile.createOutputStream().release();
        JpegImageFormatWriter writer (fos, PixelFormat::Grayscale, 0);
        ASSERT_TRUE (writer.writeImage (original));
    }

    {
        auto* fis = tempFile.createInputStream().release();
        JpegImageFormatReader reader (fis);
        auto result = reader.readImage();

        ASSERT_TRUE (result.isValid());
        EXPECT_EQ (result.getPixelFormat(), PixelFormat::Grayscale);
        EXPECT_TRUE (imagesAreEqual (original, result, 3));
    }

    tempFile.deleteFile();
}

TEST (JpegImageFormatTests, FileRoundtripSolidColorViaTempFile)
{
    Image original (16, 16, PixelFormat::RGB);
    original.fill (0xFFAA7733u);
    auto tempFile = File::createTempFile (".jpeg");

    {
        auto* fos = tempFile.createOutputStream().release();
        JpegImageFormatWriter writer (fos, PixelFormat::RGB, 0);
        ASSERT_TRUE (writer.writeImage (original));
    }

    {
        auto* fis = tempFile.createInputStream().release();
        JpegImageFormatReader reader (fis);
        auto result = reader.readImage();
        ASSERT_TRUE (imagesAreEqual (original, result, 3));
    }

    tempFile.deleteFile();
}

TEST (JpegImageFormatTests, SavedFileHasExpectedMinSize)
{
    Image original (4, 4, PixelFormat::RGB);
    original.fill (0xFFFF0000u);
    auto tempFile = File::createTempFile (".jpg");

    {
        auto* fos = tempFile.createOutputStream().release();
        JpegImageFormatWriter writer (fos, PixelFormat::RGB, 0);
        ASSERT_TRUE (writer.writeImage (original));
    }

    EXPECT_GT (tempFile.getSize(), 0);
    tempFile.deleteFile();
}

// ======================================================================
// Image::loadFromData tests
// ======================================================================

TEST (JpegImageFormatTests, LoadFromDataRoundtripRgb)
{
    Image original (10, 10, PixelFormat::RGB);
    original.fill (0xFFBB6644u);

    auto* rawStream = new MemoryOutputStream();
    JpegImageFormatWriter writer (rawStream, PixelFormat::RGB, 0);
    ASSERT_TRUE (writer.writeImage (original));

    const auto* bytes = static_cast<const uint8*> (rawStream->getData());
    const auto size = rawStream->getDataSize();

    auto result = Image::loadFromData (Span<const uint8> (bytes, size));
    ASSERT_TRUE (result.wasOk());

    const Image& decoded = result.getValue();
    EXPECT_EQ (decoded.getWidth(), original.getWidth());
    EXPECT_EQ (decoded.getHeight(), original.getHeight());
    EXPECT_TRUE (imagesAreEqual (original, decoded, 3));
}

TEST (JpegImageFormatTests, LoadFromDataRoundtripGrayscale)
{
    Image original (8, 8, PixelFormat::Grayscale);
    original.fill (0xFF999999u);

    auto* rawStream = new MemoryOutputStream();
    JpegImageFormatWriter writer (rawStream, PixelFormat::Grayscale, 0);
    ASSERT_TRUE (writer.writeImage (original));

    const auto* bytes = static_cast<const uint8*> (rawStream->getData());
    const auto size = rawStream->getDataSize();

    auto result = Image::loadFromData (Span<const uint8> (bytes, size));
    ASSERT_TRUE (result.wasOk());

    const Image& decoded = result.getValue();
    EXPECT_EQ (decoded.getWidth(), original.getWidth());
    EXPECT_EQ (decoded.getHeight(), original.getHeight());
    EXPECT_TRUE (imagesAreEqual (original, decoded, 3));
}

// ======================================================================
// Format property tests
// ======================================================================

TEST (JpegImageFormatTests, FormatNameIsCorrect)
{
    JpegImageFormat fmt;
    EXPECT_EQ (fmt.getFormatName(), String ("JPEG Image"));
}

TEST (JpegImageFormatTests, ExtensionsAreCorrect)
{
    JpegImageFormat fmt;

    auto readExts = fmt.getFileExtensions (ImageFormat::forReading);
    ASSERT_EQ (readExts.size(), 3);
    EXPECT_TRUE (readExts.contains (String (".jpg")));
    EXPECT_TRUE (readExts.contains (String (".jpeg")));
    EXPECT_TRUE (readExts.contains (String (".jpe")));

    auto writeExts = fmt.getFileExtensions (ImageFormat::forWriting);
    ASSERT_EQ (writeExts.size(), 3);
    EXPECT_TRUE (writeExts.contains (String (".jpg")));
}

TEST (JpegImageFormatTests, PossiblePixelFormatsIncludeAllThree)
{
    JpegImageFormat fmt;
    auto formats = fmt.getPossiblePixelFormats();

    EXPECT_EQ (formats.size(), 3);
    EXPECT_TRUE (formats.contains (PixelFormat::Grayscale));
    EXPECT_TRUE (formats.contains (PixelFormat::RGB));
    EXPECT_TRUE (formats.contains (PixelFormat::RGBA));
}

TEST (JpegImageFormatTests, IsCompressed)
{
    JpegImageFormat fmt;
    EXPECT_TRUE (fmt.isCompressed());
}

TEST (JpegImageFormatTests, HasQualityOptions)
{
    JpegImageFormat fmt;
    auto options = fmt.getQualityOptions();

    EXPECT_GT (options.size(), 0);
}

// ======================================================================
// canHandleFile tests
// ======================================================================

TEST (JpegImageFormatTests, CanHandleFileForAllExtensions)
{
    JpegImageFormat fmt;

    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/image.jpg"), ImageFormat::forReading));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/image.jpeg"), ImageFormat::forReading));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/image.jpe"), ImageFormat::forReading));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/image.JPG"), ImageFormat::forReading));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/image.jpg"), ImageFormat::forWriting));
}

TEST (JpegImageFormatTests, CanHandleFileRejectsWrongExtensions)
{
    JpegImageFormat fmt;

    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image.bmp"), ImageFormat::forReading));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image.png"), ImageFormat::forReading));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image.ppm"), ImageFormat::forReading));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image"), ImageFormat::forReading));
}

// ======================================================================
// canHandleStream tests
// ======================================================================

TEST (JpegImageFormatTests, CanHandleStreamDetectsJpegSignature)
{
    JpegImageFormat fmt;

    uint8 jpegSignature[] = { 0xFF, 0xD8, 0xFF, 0xE0 };
    MemoryInputStream stream (jpegSignature, sizeof (jpegSignature), false);

    EXPECT_TRUE (fmt.canHandleStream (stream, ImageFormat::forReading));
    EXPECT_EQ (stream.getPosition(), 0);
}

TEST (JpegImageFormatTests, CanHandleStreamDetectsJpegSignatureWithSosMarker)
{
    JpegImageFormat fmt;

    uint8 jpegSig[] = { 0xFF, 0xD8, 0xFF, 0xDA };
    MemoryInputStream stream (jpegSig, sizeof (jpegSig), false);

    EXPECT_TRUE (fmt.canHandleStream (stream, ImageFormat::forReading));
}

TEST (JpegImageFormatTests, CanHandleStreamRejectsBmpMagic)
{
    JpegImageFormat fmt;

    const uint8 bmpHeader[] = { 0x42, 0x4D };
    MemoryInputStream stream (bmpHeader, std::size (bmpHeader), false);

    EXPECT_FALSE (fmt.canHandleStream (stream, ImageFormat::forReading));
    EXPECT_EQ (stream.getPosition(), 0);
}

TEST (JpegImageFormatTests, CanHandleStreamRejectsEmptyStream)
{
    JpegImageFormat fmt;

    MemoryInputStream stream (static_cast<const void*> (nullptr), 0, false);
    EXPECT_FALSE (fmt.canHandleStream (stream, ImageFormat::forReading));
}

// ======================================================================
// Manager integration tests
// ======================================================================

TEST (JpegImageFormatTests, ManagerCreatesReaderForStream)
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

TEST (JpegImageFormatTests, ManagerCreatesWriterForJpgExtension)
{
    auto tempFile = File::createTempFile (".jpg");

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto writer = manager.createWriterFor (tempFile, PixelFormat::RGB);
    ASSERT_NE (writer, nullptr);
    EXPECT_EQ (writer->getFormatName(), String ("JPEG Image"));

    tempFile.deleteFile();
}

TEST (JpegImageFormatTests, ManagerCreatesWriterForJpegExtension)
{
    auto tempFile = File::createTempFile (".jpeg");

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto writer = manager.createWriterFor (tempFile, PixelFormat::RGB);
    ASSERT_NE (writer, nullptr);
    EXPECT_EQ (writer->getFormatName(), String ("JPEG Image"));

    tempFile.deleteFile();
}

TEST (JpegImageFormatTests, ManagerRoundtripViaFile)
{
    Image original (10, 8, PixelFormat::RGB);
    original.fill (0xFF4488AAu);
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
        auto result = reader->readImage();
        ASSERT_TRUE (result.isValid());
        EXPECT_EQ (result.getWidth(), original.getWidth());
        EXPECT_EQ (result.getHeight(), original.getHeight());
        EXPECT_TRUE (imagesAreEqual (original, result, 3));
    }

    tempFile.deleteFile();
}

// ======================================================================
// Reader/writer metadata tests
// ======================================================================

TEST (JpegImageFormatTests, ReaderDpiDefaultsToZero)
{
    auto* rawStream = new MemoryOutputStream();
    JpegImageFormatWriter writer (rawStream, PixelFormat::RGB, 0);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (8, 8, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    JpegImageFormatReader reader (inStream);

    EXPECT_EQ (reader.dpiX, 0.0);
    EXPECT_EQ (reader.dpiY, 0.0);
}

TEST (JpegImageFormatTests, WriterFlushReturnsTrue)
{
    JpegImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB, 0);
    EXPECT_TRUE (writer.flush());
}

TEST (JpegImageFormatTests, WriterReturnsCorrectPixelFormat)
{
    {
        JpegImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB, 0);
        EXPECT_EQ (writer.getPixelFormat(), PixelFormat::RGB);
    }
    {
        JpegImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::Grayscale, 0);
        EXPECT_EQ (writer.getPixelFormat(), PixelFormat::Grayscale);
    }
}

TEST (JpegImageFormatTests, WriterReturnsCorrectFormatName)
{
    JpegImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB, 0);
    EXPECT_EQ (writer.getFormatName(), String ("JPEG Image"));
}

#endif // YUP_MODULE_AVAILABLE_libjpeg && YUP_IMAGE_FORMAT_JPEG
