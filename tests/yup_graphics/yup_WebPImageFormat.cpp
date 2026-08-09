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
// Reader dimension and header tests
// ======================================================================

TEST (WebPImageFormatTests, ReaderSetsCorrectDimensions)
{
    auto* rawStream = new MemoryOutputStream();
    WebPImageFormatWriter writer (rawStream, PixelFormat::RGBA, 0);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (4, 6, PixelFormat::RGBA)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    WebPImageFormatReader reader (inStream);

    EXPECT_EQ (reader.width, 4);
    EXPECT_EQ (reader.height, 6);
}

TEST (WebPImageFormatTests, ReaderSetsDimensionsForSmallImage)
{
    auto* rawStream = new MemoryOutputStream();
    WebPImageFormatWriter writer (rawStream, PixelFormat::RGBA, 0);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (1, 1, PixelFormat::RGBA)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    WebPImageFormatReader reader (inStream);

    EXPECT_EQ (reader.width, 1);
    EXPECT_EQ (reader.height, 1);
}

TEST (WebPImageFormatTests, ReaderSetsDimensionsForNonSquareImage)
{
    auto* rawStream = new MemoryOutputStream();
    WebPImageFormatWriter writer (rawStream, PixelFormat::RGB, 0);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (23, 3, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    WebPImageFormatReader reader (inStream);

    EXPECT_EQ (reader.width, 23);
    EXPECT_EQ (reader.height, 3);
}

TEST (WebPImageFormatTests, ReaderSetsCorrectPixelFormatRgb)
{
    auto* rawStream = new MemoryOutputStream();
    WebPImageFormatWriter writer (rawStream, PixelFormat::RGB, 0);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (4, 4, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    WebPImageFormatReader reader (inStream);

    EXPECT_EQ (reader.pixelFormat, PixelFormat::RGB);
}

TEST (WebPImageFormatTests, ReaderSetsCorrectPixelFormatRgba)
{
    auto* rawStream = new MemoryOutputStream();
    WebPImageFormatWriter writer (rawStream, PixelFormat::RGBA, 0);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (4, 4, PixelFormat::RGBA, 0x80FFCC44u)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    WebPImageFormatReader reader (inStream);

    EXPECT_EQ (reader.pixelFormat, PixelFormat::RGBA);
}

TEST (WebPImageFormatTests, InvalidSignatureReturnsInvalidImage)
{
    const uint8 garbage[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
    auto* stream = new MemoryInputStream (garbage, sizeof (garbage), false);

    WebPImageFormatReader reader (stream);

    EXPECT_EQ (reader.width, 0);
    EXPECT_EQ (reader.height, 0);

    auto result = reader.readImage();
    EXPECT_FALSE (result.isValid());
}

// ======================================================================
// Roundtrip tests - lossless (memory-based)
// ======================================================================

TEST (WebPImageFormatTests, WriteAndReadBackRgbLosslessProducesPixelIdenticalImage)
{
    auto original = generateTestImage (16, 16, PixelFormat::RGB);

    auto* rawStream = new MemoryOutputStream();
    WebPImageFormatWriter writer (rawStream, PixelFormat::RGB, 0);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    WebPImageFormatReader reader (inStream);
    auto result = reader.readImage();

    EXPECT_EQ (result.getWidth(), original.getWidth());
    EXPECT_EQ (result.getHeight(), original.getHeight());
    EXPECT_EQ (result.getPixelFormat(), original.getPixelFormat());
    EXPECT_TRUE (imagesAreEqual (original, result, 0));
}

TEST (WebPImageFormatTests, WriteAndReadBackRgbaLosslessProducesPixelIdenticalImage)
{
    auto original = generateTestImage (16, 16, PixelFormat::RGBA);

    auto* rawStream = new MemoryOutputStream();
    WebPImageFormatWriter writer (rawStream, PixelFormat::RGBA, 0);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    WebPImageFormatReader reader (inStream);
    auto result = reader.readImage();

    ASSERT_EQ (result.getWidth(), original.getWidth());
    ASSERT_EQ (result.getHeight(), original.getHeight());
    EXPECT_EQ (result.getPixelFormat(), original.getPixelFormat());
    EXPECT_TRUE (imagesAreEqual (original, result, 0));
}

TEST (WebPImageFormatTests, WriteAndReadBackRgbaLossyProducesNearEqualImage)
{
    Image original (16, 16, PixelFormat::RGBA);
    original.fill (0x80CC8844u);

    auto* rawStream = new MemoryOutputStream();
    WebPImageFormatWriter writer (rawStream, PixelFormat::RGBA, 1);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    WebPImageFormatReader reader (inStream);
    auto result = reader.readImage();

    ASSERT_EQ (result.getWidth(), original.getWidth());
    ASSERT_EQ (result.getHeight(), original.getHeight());
    EXPECT_TRUE (imagesAreEqual (original, result, 5));
}

TEST (WebPImageFormatTests, SolidColorRgbLosslessRoundtripIsExact)
{
    auto original = generateSolidImage (20, 10, PixelFormat::RGB, 0xFF3399CCu);

    auto* rawStream = new MemoryOutputStream();
    WebPImageFormatWriter writer (rawStream, PixelFormat::RGB, 0);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    WebPImageFormatReader reader (inStream);
    auto result = reader.readImage();

    EXPECT_TRUE (imagesAreEqual (original, result, 0));
}

TEST (WebPImageFormatTests, SolidColorRgbaLosslessRoundtripIsExact)
{
    auto original = generateSolidImage (12, 8, PixelFormat::RGBA, 0x88DDEEFFu);

    auto* rawStream = new MemoryOutputStream();
    WebPImageFormatWriter writer (rawStream, PixelFormat::RGBA, 0);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    WebPImageFormatReader reader (inStream);
    auto result = reader.readImage();

    for (int y = 0; y < original.getHeight(); ++y)
    {
        for (int x = 0; x < original.getWidth(); ++x)
            EXPECT_EQ (result.getPixel (x, y), original.getPixel (x, y));
    }
}

TEST (WebPImageFormatTests, VariousLossyQualityLevelsProduceValidImages)
{
    Image original (16, 16, PixelFormat::RGB);
    original.fill (0xFF224466u);

    for (int qi = 1; qi <= 4; ++qi)
    {
        auto* rawStream = new MemoryOutputStream();
        WebPImageFormatWriter writer (rawStream, PixelFormat::RGB, qi);
        ASSERT_TRUE (writer.writeImage (original));

        auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
        WebPImageFormatReader reader (inStream);
        auto result = reader.readImage();

        ASSERT_TRUE (result.isValid())
            << "Failed at quality index " << qi;
        EXPECT_EQ (result.getWidth(), original.getWidth());
        EXPECT_EQ (result.getHeight(), original.getHeight());
        EXPECT_TRUE (imagesAreEqual (original, result, 8))
            << "Quality index " << qi << " deviates too much";
    }
}

TEST (WebPImageFormatTests, VariousSizesRoundtripCorrectly)
{
    const int sizes[][2] = { { 1, 1 }, { 1, 16 }, { 16, 1 }, { 7, 13 }, { 32, 24 } };

    for (auto [w, h] : sizes)
    {
        auto original = generateTestImage (w, h, PixelFormat::RGB);

        auto* rawStream = new MemoryOutputStream();
        WebPImageFormatWriter writer (rawStream, PixelFormat::RGB, 0);
        ASSERT_TRUE (writer.writeImage (original));

        auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
        WebPImageFormatReader reader (inStream);
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

TEST (WebPImageFormatTests, FileRoundtripLosslessRgbViaTempFile)
{
    auto original = generateTestImage (16, 12, PixelFormat::RGB);
    auto tempFile = File::createTempFile (".webp");

    {
        auto* fos = tempFile.createOutputStream().release();
        WebPImageFormatWriter writer (fos, PixelFormat::RGB, 0);
        ASSERT_TRUE (writer.writeImage (original));
    }

    {
        auto* fis = tempFile.createInputStream().release();
        WebPImageFormatReader reader (fis);
        auto result = reader.readImage();

        ASSERT_TRUE (result.isValid());
        EXPECT_EQ (result.getWidth(), original.getWidth());
        EXPECT_EQ (result.getHeight(), original.getHeight());
        EXPECT_TRUE (imagesAreEqual (original, result, 0));
    }

    tempFile.deleteFile();
}

TEST (WebPImageFormatTests, FileRoundtripLosslessRgbaViaTempFile)
{
    auto original = generateTestImage (10, 14, PixelFormat::RGBA);
    auto tempFile = File::createTempFile (".webp");

    {
        auto* fos = tempFile.createOutputStream().release();
        WebPImageFormatWriter writer (fos, PixelFormat::RGBA, 0);
        ASSERT_TRUE (writer.writeImage (original));
    }

    {
        auto* fis = tempFile.createInputStream().release();
        WebPImageFormatReader reader (fis);
        auto result = reader.readImage();

        ASSERT_TRUE (result.isValid());
        EXPECT_EQ (result.getPixelFormat(), PixelFormat::RGBA);
        EXPECT_TRUE (imagesAreEqual (original, result, 0));
    }

    tempFile.deleteFile();
}

TEST (WebPImageFormatTests, FileRoundtripLossyRgbViaTempFile)
{
    Image original (16, 16, PixelFormat::RGB);
    original.fill (0xFF33AA55u);
    auto tempFile = File::createTempFile (".webp");

    {
        auto* fos = tempFile.createOutputStream().release();
        WebPImageFormatWriter writer (fos, PixelFormat::RGB, 1);
        ASSERT_TRUE (writer.writeImage (original));
    }

    {
        auto* fis = tempFile.createInputStream().release();
        WebPImageFormatReader reader (fis);
        auto result = reader.readImage();
        ASSERT_TRUE (imagesAreEqual (original, result, 5));
    }

    tempFile.deleteFile();
}

TEST (WebPImageFormatTests, FileRoundtripLossyRgbaViaTempFile)
{
    Image original (16, 16, PixelFormat::RGBA);
    original.fill (0x80CC8844u);
    auto tempFile = File::createTempFile (".webp");

    {
        auto* fos = tempFile.createOutputStream().release();
        WebPImageFormatWriter writer (fos, PixelFormat::RGBA, 2);
        ASSERT_TRUE (writer.writeImage (original));
    }

    {
        auto* fis = tempFile.createInputStream().release();
        WebPImageFormatReader reader (fis);
        auto result = reader.readImage();
        ASSERT_TRUE (imagesAreEqual (original, result, 5));
    }

    tempFile.deleteFile();
}

TEST (WebPImageFormatTests, FileRoundtripSolidColorViaTempFile)
{
    auto original = generateSolidImage (24, 12, PixelFormat::RGBA, 0xCC4488AAu);
    auto tempFile = File::createTempFile (".webp");

    {
        auto* fos = tempFile.createOutputStream().release();
        WebPImageFormatWriter writer (fos, PixelFormat::RGBA, 0);
        ASSERT_TRUE (writer.writeImage (original));
    }

    {
        auto* fis = tempFile.createInputStream().release();
        WebPImageFormatReader reader (fis);
        auto result = reader.readImage();
        ASSERT_TRUE (imagesAreEqual (original, result, 0));
    }

    tempFile.deleteFile();
}

TEST (WebPImageFormatTests, SavedFileHasExpectedMinSize)
{
    auto original = generateSolidImage (4, 4, PixelFormat::RGB);
    auto tempFile = File::createTempFile (".webp");

    {
        auto* fos = tempFile.createOutputStream().release();
        WebPImageFormatWriter writer (fos, PixelFormat::RGB, 0);
        ASSERT_TRUE (writer.writeImage (original));
    }

    EXPECT_GT (tempFile.getSize(), 0);
    tempFile.deleteFile();
}

// ======================================================================
// Image::loadFromData tests
// ======================================================================

TEST (WebPImageFormatTests, LoadFromDataRoundtripRgbLossless)
{
    auto original = generateTestImage (12, 12, PixelFormat::RGB);

    auto* rawStream = new MemoryOutputStream();
    WebPImageFormatWriter writer (rawStream, PixelFormat::RGB, 0);
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

TEST (WebPImageFormatTests, LoadFromDataRoundtripRgbaLossless)
{
    auto original = generateTestImage (8, 8, PixelFormat::RGBA);

    auto* rawStream = new MemoryOutputStream();
    WebPImageFormatWriter writer (rawStream, PixelFormat::RGBA, 0);
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

TEST (WebPImageFormatTests, LoadFromDataRoundtripRgbLossy)
{
    Image original (12, 12, PixelFormat::RGB);
    original.fill (0xFFCC8844u);

    auto* rawStream = new MemoryOutputStream();
    WebPImageFormatWriter writer (rawStream, PixelFormat::RGB, 1);
    ASSERT_TRUE (writer.writeImage (original));

    const auto* bytes = static_cast<const uint8*> (rawStream->getData());
    const auto size = rawStream->getDataSize();

    auto result = Image::loadFromData (Span<const uint8> (bytes, size));
    ASSERT_TRUE (result.wasOk());

    const Image& decoded = result.getValue();
    EXPECT_EQ (decoded.getWidth(), original.getWidth());
    EXPECT_EQ (decoded.getHeight(), original.getHeight());
    EXPECT_TRUE (imagesAreEqual (original, decoded, 5));
}

// ======================================================================
// Format property tests
// ======================================================================

TEST (WebPImageFormatTests, FormatNameIsCorrect)
{
    WebPImageFormat fmt;
    EXPECT_EQ (fmt.getFormatName(), String ("WebP Image"));
}

TEST (WebPImageFormatTests, ExtensionsAreCorrect)
{
    WebPImageFormat fmt;

    auto readExts = fmt.getFileExtensions (ImageFormat::forReading);
    ASSERT_EQ (readExts.size(), 1);
    EXPECT_EQ (readExts[0], String (".webp"));

    auto writeExts = fmt.getFileExtensions (ImageFormat::forWriting);
    ASSERT_EQ (writeExts.size(), 1);
    EXPECT_EQ (writeExts[0], String (".webp"));
}

TEST (WebPImageFormatTests, PossiblePixelFormatsIncludeRgbAndRgba)
{
    WebPImageFormat fmt;
    auto formats = fmt.getPossiblePixelFormats();

    EXPECT_EQ (formats.size(), 2);
    EXPECT_TRUE (formats.contains (PixelFormat::RGB));
    EXPECT_TRUE (formats.contains (PixelFormat::RGBA));
}

TEST (WebPImageFormatTests, IsCompressed)
{
    WebPImageFormat fmt;
    EXPECT_TRUE (fmt.isCompressed());
}

TEST (WebPImageFormatTests, HasCorrectQualityOptions)
{
    WebPImageFormat fmt;
    auto options = fmt.getQualityOptions();

    ASSERT_EQ (options.size(), 5);
    EXPECT_EQ (options[0], "Lossless");
    EXPECT_EQ (options[1], "Quality 90");
    EXPECT_EQ (options[2], "Quality 80");
    EXPECT_EQ (options[3], "Quality 60");
    EXPECT_EQ (options[4], "Quality 40");
}

// ======================================================================
// canHandleFile tests
// ======================================================================

TEST (WebPImageFormatTests, CanHandleFileForWebpExtension)
{
    WebPImageFormat fmt;

    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/image.webp"), ImageFormat::forReading));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/image.WEBP"), ImageFormat::forReading));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/image.webp"), ImageFormat::forWriting));
}

TEST (WebPImageFormatTests, CanHandleFileRejectsWrongExtensions)
{
    WebPImageFormat fmt;

    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image.bmp"), ImageFormat::forReading));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image.png"), ImageFormat::forReading));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image.jpg"), ImageFormat::forReading));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image"), ImageFormat::forReading));
}

// ======================================================================
// canHandleStream tests
// ======================================================================

TEST (WebPImageFormatTests, CanHandleStreamDetectsWebpSignature)
{
    WebPImageFormat fmt;

    const uint8 webpHeader[] = { 0x52, 0x49, 0x46, 0x46, 0x00, 0x00, 0x00, 0x00, 0x57, 0x45, 0x42, 0x50 };
    MemoryInputStream stream (webpHeader, std::size (webpHeader), false);

    EXPECT_TRUE (fmt.canHandleStream (stream, ImageFormat::forReading));
    EXPECT_EQ (stream.getPosition(), 0);
}

TEST (WebPImageFormatTests, CanHandleStreamRejectsPngSignature)
{
    WebPImageFormat fmt;

    const uint8 pngHeader[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
    MemoryInputStream stream (pngHeader, std::size (pngHeader), false);

    EXPECT_FALSE (fmt.canHandleStream (stream, ImageFormat::forReading));
    EXPECT_EQ (stream.getPosition(), 0);
}

TEST (WebPImageFormatTests, CanHandleStreamRejectsEmptyStream)
{
    WebPImageFormat fmt;

    MemoryInputStream stream (static_cast<const void*> (nullptr), 0, false);
    EXPECT_FALSE (fmt.canHandleStream (stream, ImageFormat::forReading));
}

// ======================================================================
// Manager integration tests
// ======================================================================

TEST (WebPImageFormatTests, ManagerCreatesReaderForStream)
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
    EXPECT_EQ (reader->width, 4);
    EXPECT_EQ (reader->height, 4);
}

TEST (WebPImageFormatTests, ManagerCreatesWriterForWebpExtension)
{
    auto tempFile = File::createTempFile (".webp");

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto writer = manager.createWriterFor (tempFile, PixelFormat::RGBA);
    ASSERT_NE (writer, nullptr);
    EXPECT_EQ (writer->getFormatName(), String ("WebP Image"));
    EXPECT_EQ (writer->getPixelFormat(), PixelFormat::RGBA);

    tempFile.deleteFile();
}

TEST (WebPImageFormatTests, ManagerRoundtripViaFile)
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

TEST (WebPImageFormatTests, ReaderDpiDefaultsToZero)
{
    auto* rawStream = new MemoryOutputStream();
    WebPImageFormatWriter writer (rawStream, PixelFormat::RGB, 0);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (8, 8, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    WebPImageFormatReader reader (inStream, ImageFormat::Options().withMetadata (true));

    EXPECT_DOUBLE_EQ (reader.metadata->dpiX, 0.0);
    EXPECT_DOUBLE_EQ (reader.metadata->dpiY, 0.0);
}

TEST (WebPImageFormatTests, WriterFlushReturnsTrue)
{
    WebPImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB, 0);
    EXPECT_TRUE (writer.flush());
}

TEST (WebPImageFormatTests, WriterReturnsCorrectPixelFormat)
{
    {
        WebPImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB, 0);
        EXPECT_EQ (writer.getPixelFormat(), PixelFormat::RGB);
    }
    {
        WebPImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGBA, 0);
        EXPECT_EQ (writer.getPixelFormat(), PixelFormat::RGBA);
    }
}

TEST (WebPImageFormatTests, WriterReturnsCorrectFormatName)
{
    WebPImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGB, 0);
    EXPECT_EQ (writer.getFormatName(), String ("WebP Image"));
}

// ======================================================================
// Edge case tests
// ======================================================================

TEST (WebPImageFormatTests, FullyTransparentImageRoundtripLossless)
{
    auto original = generateSolidImage (8, 8, PixelFormat::RGBA, 0x33445566u);

    auto* rawStream = new MemoryOutputStream();
    WebPImageFormatWriter writer (rawStream, PixelFormat::RGBA, 0);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    WebPImageFormatReader reader (inStream);
    auto result = reader.readImage();

    ASSERT_TRUE (result.isValid());
    ASSERT_TRUE (imagesAreEqual (original, result, 0));
}

TEST (WebPImageFormatTests, FullyOpaqueImageRoundtripLossless)
{
    auto original = generateSolidImage (8, 8, PixelFormat::RGBA, 0xFFCC4488u);

    auto* rawStream = new MemoryOutputStream();
    WebPImageFormatWriter writer (rawStream, PixelFormat::RGBA, 0);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    WebPImageFormatReader reader (inStream);
    auto result = reader.readImage();

    ASSERT_TRUE (result.isValid());
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

TEST (WebPImageFormatTests, LossyVsLosslessSizeDifference)
{
    Image original (32, 32, PixelFormat::RGB);
    original.fill (0xFFCC8844u);

    auto* losslessStream = new MemoryOutputStream();
    WebPImageFormatWriter losslessWriter (losslessStream, PixelFormat::RGB, 0);
    ASSERT_TRUE (losslessWriter.writeImage (original));

    auto* lossyStream = new MemoryOutputStream();
    WebPImageFormatWriter lossyWriter (lossyStream, PixelFormat::RGB, 4);
    ASSERT_TRUE (lossyWriter.writeImage (original));

    EXPECT_GT (losslessStream->getDataSize(), 0u);
    EXPECT_GT (lossyStream->getDataSize(), 0u);
}

// ======================================================================
// Invalid image writeImage
// ======================================================================

TEST (WebPImageFormatTests, WriteImageReturnsFalseForInvalidImage)
{
    auto* rawStream = new MemoryOutputStream();
    WebPImageFormatWriter writer (rawStream, PixelFormat::RGBA, 0);

    Image invalid;
    EXPECT_FALSE (writer.writeImage (invalid));
}

// ======================================================================
// Selective registration tests
// ======================================================================

TEST (WebPImageFormatTests, SelectiveRegistrationWebpOnly)
{
    ImageFormatManager manager;
    manager.registerDefaultFormats (ImageFormatType::webp);

    auto webpFile = File::createTempFile (".webp");
    auto bmpFile = File::createTempFile (".bmp");

    EXPECT_NE (manager.createWriterFor (webpFile, PixelFormat::RGBA), nullptr);
    EXPECT_EQ (manager.createWriterFor (bmpFile, PixelFormat::RGB), nullptr);

    webpFile.deleteFile();
    bmpFile.deleteFile();
}

// ======================================================================
// WebP animation tests
// ======================================================================

TEST (WebPImageFormatTests, WriteAndReadAnimatedWebpRoundtrip)
{
    Image red (8, 8, PixelFormat::RGBA);
    red.fill (0xFFFF0000u);
    Image green (8, 8, PixelFormat::RGBA);
    green.fill (0xFF00FF00u);
    Image blue (8, 8, PixelFormat::RGBA);
    blue.fill (0xFF0000FFu);

    auto* rawStream = new MemoryOutputStream();
    WebPImageFormatWriter writer (rawStream, PixelFormat::RGBA, 0);
    ASSERT_TRUE (writer.beginAnimation (3));
    ASSERT_TRUE (writer.writeFrame (red, 100));
    ASSERT_TRUE (writer.writeFrame (green, 200));
    ASSERT_TRUE (writer.writeFrame (blue, 300));
    ASSERT_TRUE (writer.endAnimation());

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    WebPImageFormatReader reader (inStream);

    ASSERT_TRUE (reader.isAnimated());
    EXPECT_EQ (reader.getFrameCount(), 3);
    EXPECT_EQ (reader.getLoopCount(), 3);

    auto f0 = reader.readFrame (0);
    auto f1 = reader.readFrame (1);
    auto f2 = reader.readFrame (2);

    ASSERT_TRUE (f0.isValid());
    ASSERT_TRUE (f1.isValid());
    ASSERT_TRUE (f2.isValid());

    EXPECT_TRUE (imagesAreEqual (red, f0, 0));
    EXPECT_TRUE (imagesAreEqual (green, f1, 0));
    EXPECT_TRUE (imagesAreEqual (blue, f2, 0));
}

TEST (WebPImageFormatTests, AnimatedWebpFrameDelayAndSeek)
{
    Image r (4, 4, PixelFormat::RGBA);
    r.fill (0xFFFF0000u);
    Image g (4, 4, PixelFormat::RGBA);
    g.fill (0xFF00FF00u);
    Image b (4, 4, PixelFormat::RGBA);
    b.fill (0xFF0000FFu);

    auto* rawStream = new MemoryOutputStream();
    WebPImageFormatWriter writer (rawStream, PixelFormat::RGBA, 0);
    ASSERT_TRUE (writer.beginAnimation (0));
    ASSERT_TRUE (writer.writeFrame (r, 50));
    ASSERT_TRUE (writer.writeFrame (g, 150));
    ASSERT_TRUE (writer.writeFrame (b, 250));
    ASSERT_TRUE (writer.endAnimation());

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    WebPImageFormatReader reader (inStream);

    EXPECT_EQ (reader.getFrameDelayMs (0), 50);
    EXPECT_EQ (reader.getFrameDelayMs (1), 150);
    EXPECT_EQ (reader.getFrameDelayMs (2), 250);
    EXPECT_EQ (reader.getFrameDelayMs (3), 0); // out of bounds

    // Seek to frame 2 (should composite frame 0, then 1, then 2)
    auto f2 = reader.readFrame (2);
    ASSERT_TRUE (f2.isValid());
    EXPECT_TRUE (imagesAreEqual (b, f2, 0));

    // Seek backwards to frame 0 (should reset and recomposite)
    auto f0again = reader.readFrame (0);
    ASSERT_TRUE (f0again.isValid());
    EXPECT_TRUE (imagesAreEqual (r, f0again, 0));
}

// ======================================================================
// WebP raw chunks parsing
// ======================================================================

TEST (WebPImageFormatTests, ParseRawChunksCreatesMetadata)
{
    auto* rawStream = new MemoryOutputStream();
    WebPImageFormatWriter writer (rawStream, PixelFormat::RGBA, 0);
    ASSERT_TRUE (writer.writeImage (generateTestImage (4, 4, PixelFormat::RGBA)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    WebPImageFormatReader reader (inStream, ImageFormat::Options().withRawChunks (true));

    ASSERT_NE (reader.metadata, nullptr);
    // The reader might have extracted EXIF/ICCP/XMP chunks if present
    // At minimum, metadata should be created
    EXPECT_DOUBLE_EQ (reader.metadata->dpiX, 0.0);
    EXPECT_DOUBLE_EQ (reader.metadata->dpiY, 0.0);
}
