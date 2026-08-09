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
    JpegImageFormatReader reader (inStream, ImageFormat::Options().withMetadata (true));

    EXPECT_DOUBLE_EQ (reader.metadata->dpiX, 0.0);
    EXPECT_DOUBLE_EQ (reader.metadata->dpiY, 0.0);
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

// ======================================================================
// Quality level sweep tests
// ======================================================================

TEST (JpegImageFormatTests, VariousQualityLevelsProduceValidImages)
{
    Image original (16, 16, PixelFormat::RGB);
    original.fill (0xFF4488CCu);

    for (int qi = 0; qi <= 3; ++qi)
    {
        auto* rawStream = new MemoryOutputStream();
        JpegImageFormatWriter writer (rawStream, PixelFormat::RGB, qi);
        ASSERT_TRUE (writer.writeImage (original));

        auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
        JpegImageFormatReader reader (inStream);
        auto result = reader.readImage();

        ASSERT_TRUE (result.isValid())
            << "Failed at quality index " << qi;
        EXPECT_EQ (result.getWidth(), original.getWidth());
        EXPECT_EQ (result.getHeight(), original.getHeight());
        EXPECT_TRUE (imagesAreEqual (original, result, 4))
            << "Quality index " << qi << " deviates too much";
    }
}

// ======================================================================
// Invalid image writeImage
// ======================================================================

TEST (JpegImageFormatTests, WriteImageReturnsFalseForInvalidImage)
{
    auto* rawStream = new MemoryOutputStream();
    JpegImageFormatWriter writer (rawStream, PixelFormat::RGB, 0);

    Image invalid;
    EXPECT_FALSE (writer.writeImage (invalid));
}

// ======================================================================
// Selective registration tests
// ======================================================================

TEST (JpegImageFormatTests, SelectiveRegistrationJpegOnly)
{
    ImageFormatManager manager;
    manager.registerDefaultFormats (ImageFormatType::jpeg);

    auto jpgFile = File::createTempFile (".jpg");
    auto bmpFile = File::createTempFile (".bmp");

    EXPECT_NE (manager.createWriterFor (jpgFile, PixelFormat::RGB), nullptr);
    EXPECT_EQ (manager.createWriterFor (bmpFile, PixelFormat::RGB), nullptr);

    jpgFile.deleteFile();
    bmpFile.deleteFile();
}

// ======================================================================
// Metadata parsing tests
// ======================================================================

TEST (JpegImageFormatTests, ParseMetadataReadsDpi)
{
    Image img (4, 4, PixelFormat::RGB);
    img.fill (0xFFCC8844u);

    auto meta = ImageMetadata::create();
    meta->dpiX = 300.0;
    meta->dpiY = 300.0;
    img.setMetadata (meta);

    auto* rawStream = new MemoryOutputStream();
    JpegImageFormatWriter writer (rawStream, PixelFormat::RGB, 0);
    ASSERT_TRUE (writer.writeImage (img));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    JpegImageFormatReader reader (inStream, ImageFormat::Options().withMetadata (true));

    ASSERT_NE (reader.metadata, nullptr);
    EXPECT_NEAR (reader.metadata->dpiX, 300.0, 5.0);
    EXPECT_NEAR (reader.metadata->dpiY, 300.0, 5.0);
}

TEST (JpegImageFormatTests, ParseMetadataReadsComment)
{
    Image img (4, 4, PixelFormat::RGB);
    img.fill (0xFFCC8844u);

    auto meta = ImageMetadata::create();
    meta->textEntries.set ("Comment", "JPEG test comment");
    img.setMetadata (meta);

    auto* rawStream = new MemoryOutputStream();
    JpegImageFormatWriter writer (rawStream, PixelFormat::RGB, 0);
    ASSERT_TRUE (writer.writeImage (img));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    JpegImageFormatReader reader (inStream, ImageFormat::Options().withMetadata (true));

    ASSERT_NE (reader.metadata, nullptr);
    EXPECT_EQ (reader.metadata->textEntries.getValue ("Comment", {}), String ("JPEG test comment"));
}

TEST (JpegImageFormatTests, ParseRawChunksExtractsExifWhenPresent)
{
    Image img (4, 4, PixelFormat::RGB);
    img.fill (0xFFCC8844u);

    auto meta = ImageMetadata::create();
    const uint8 rawExif[] = {
        'M', 'M', 0x00, 0x2A, 0x00, 0x00, 0x00, 0x08, 0x00, 0x01, 0x01, 0x12, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    meta->setRawChunk ("jpeg/exif", MemoryBlock (rawExif, sizeof (rawExif)));
    img.setMetadata (meta);

    auto* rawStream = new MemoryOutputStream();
    JpegImageFormatWriter writer (rawStream, PixelFormat::RGB, 0);
    ASSERT_TRUE (writer.writeImage (img));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    JpegImageFormatReader reader (inStream, ImageFormat::Options().withRawChunks (true));

    ASSERT_NE (reader.metadata, nullptr);
    EXPECT_TRUE (reader.metadata->hasRawChunk ("jpeg/exif"));
}

TEST (JpegImageFormatTests, ParseRawChunksExtractsCommentWhenPresent)
{
    Image img (4, 4, PixelFormat::RGB);
    img.fill (0xFFCC8844u);

    auto meta = ImageMetadata::create();
    meta->textEntries.set ("Comment", "Raw chunk comment");
    img.setMetadata (meta);

    auto* rawStream = new MemoryOutputStream();
    JpegImageFormatWriter writer (rawStream, PixelFormat::RGB, 0);
    ASSERT_TRUE (writer.writeImage (img));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    JpegImageFormatReader reader (inStream, ImageFormat::Options().withRawChunks (true));

    ASSERT_NE (reader.metadata, nullptr);
    EXPECT_TRUE (reader.metadata->hasRawChunk ("jpeg/comment"));
}

TEST (JpegImageFormatTests, ParseRawChunksExtractsIccWhenPresent)
{
    Image img (4, 4, PixelFormat::RGB);
    img.fill (0xFFCC8844u);

    auto meta = ImageMetadata::create();
    // The reader stores the full APP2 marker data including "ICC_PROFILE\0" prefix,
    // so the raw chunk must include it for roundtrip.
    const uint8 iccData[] = { 'I', 'C', 'C', '_', 'P', 'R', 'O', 'F', 'I', 'L', 'E', '\0', 0xDE, 0xAD, 0xBE, 0xEF };
    meta->setRawChunk ("jpeg/icc", MemoryBlock (iccData, sizeof (iccData)));
    img.setMetadata (meta);

    auto* rawStream = new MemoryOutputStream();
    JpegImageFormatWriter writer (rawStream, PixelFormat::RGB, 0);
    ASSERT_TRUE (writer.writeImage (img));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    JpegImageFormatReader reader (inStream, ImageFormat::Options().withRawChunks (true));

    ASSERT_NE (reader.metadata, nullptr);
    EXPECT_TRUE (reader.metadata->hasRawChunk ("jpeg/icc"));
}

TEST (JpegImageFormatTests, MetadataExtractsExifOrientation)
{
    Image img (4, 4, PixelFormat::RGB);
    img.fill (0xFFCC8844u);

    // Build a JPEG with EXIF containing orientation=6
    const uint8 rawExif[] = {
        'M', 'M', 0x00, 0x2A, 0x00, 0x00, 0x00, 0x08, 0x00, 0x01, 0x01, 0x12, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    auto meta = ImageMetadata::create();
    meta->setRawChunk ("jpeg/exif", MemoryBlock (rawExif, sizeof (rawExif)));
    img.setMetadata (meta);

    auto* rawStream = new MemoryOutputStream();
    JpegImageFormatWriter writer (rawStream, PixelFormat::RGB, 0);
    ASSERT_TRUE (writer.writeImage (img));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    // Need parseRawChunks to store EXIF data; getOrientation reads from raw chunks
    JpegImageFormatReader reader (inStream, ImageFormat::Options().withRawChunks (true));

    ASSERT_NE (reader.metadata, nullptr);
    EXPECT_EQ (reader.metadata->getOrientation(), 6);
}
