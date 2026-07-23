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
// Helper: encode image to memory and decode back via TGA reader/writer
// ======================================================================

namespace
{

struct TgaRoundtripResult
{
    Image original;
    Image decoded;
};

TgaRoundtripResult tgaRoundtrip (const Image& original, bool useRLE = false)
{
    auto* rawStream = new MemoryOutputStream();
    TgaImageFormatWriter writer (rawStream, original.getPixelFormat(), useRLE);
    writer.writeImage (original);

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    TgaImageFormatReader reader (inStream);

    return { original, reader.readImage() };
}

/** Creates a valid TGA header in memory with the given parameters and returns the raw bytes. */
std::vector<uint8> makeTgaHeader (uint8 imageType, uint16 w, uint16 h, uint8 pixelDepth, uint8 descriptor = 0x20, uint8 colorMapType = 0, uint16 colorMapLength = 0, uint8 colorMapEntrySize = 0)
{
    std::vector<uint8> header (18, 0);
    header[0] = 0;            // ID length
    header[1] = colorMapType; // Color map type
    header[2] = imageType;    // Image type
    // firstEntryIndex (bytes 3-4) = 0
    header[5] = static_cast<uint8> (colorMapLength & 0xFF);
    header[6] = static_cast<uint8> ((colorMapLength >> 8) & 0xFF);
    header[7] = colorMapEntrySize;
    // xOrigin, yOrigin (bytes 8-11) = 0
    header[12] = static_cast<uint8> (w & 0xFF);
    header[13] = static_cast<uint8> ((w >> 8) & 0xFF);
    header[14] = static_cast<uint8> (h & 0xFF);
    header[15] = static_cast<uint8> ((h >> 8) & 0xFF);
    header[16] = pixelDepth;
    header[17] = descriptor;
    return header;
}

} // namespace

// ======================================================================
// Reader dimension and header tests
// ======================================================================

TEST (TgaImageFormatTests, ReaderSetsCorrectDimensionsFromHeader)
{
    auto* rawStream = new MemoryOutputStream();
    TgaImageFormatWriter writer (rawStream, PixelFormat::RGB, false);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (4, 8, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    TgaImageFormatReader reader (inStream);

    EXPECT_EQ (reader.width, 4);
    EXPECT_EQ (reader.height, 8);
    EXPECT_EQ (reader.pixelFormat, PixelFormat::RGB);
}

TEST (TgaImageFormatTests, ReaderSetsDimensionsForSmallImage)
{
    auto* rawStream = new MemoryOutputStream();
    TgaImageFormatWriter writer (rawStream, PixelFormat::RGBA, false);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (1, 1, PixelFormat::RGBA)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    TgaImageFormatReader reader (inStream);

    EXPECT_EQ (reader.width, 1);
    EXPECT_EQ (reader.height, 1);
    EXPECT_EQ (reader.pixelFormat, PixelFormat::RGBA);
}

TEST (TgaImageFormatTests, ReaderSetsDimensionsForNonSquareImage)
{
    auto* rawStream = new MemoryOutputStream();
    TgaImageFormatWriter writer (rawStream, PixelFormat::RGB, false);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (13, 7, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    TgaImageFormatReader reader (inStream);

    EXPECT_EQ (reader.width, 13);
    EXPECT_EQ (reader.height, 7);
}

TEST (TgaImageFormatTests, InvalidImageTypeReturnsZeroDimensions)
{
    auto header = makeTgaHeader (99, 10, 10, 24); // type 99 is invalid
    auto* stream = new MemoryInputStream (header.data(), header.size(), true);

    TgaImageFormatReader reader (stream);

    EXPECT_EQ (reader.width, 0);
    EXPECT_EQ (reader.height, 0);

    auto result = reader.readImage();
    EXPECT_FALSE (result.isValid());
}

TEST (TgaImageFormatTests, InvalidPixelDepthReturnsZeroDimensions)
{
    auto header = makeTgaHeader (2, 10, 10, 7); // pixel depth 7 is invalid
    auto* stream = new MemoryInputStream (header.data(), header.size(), true);

    TgaImageFormatReader reader (stream);

    EXPECT_EQ (reader.width, 0);
    EXPECT_EQ (reader.height, 0);
}

TEST (TgaImageFormatTests, GarbageDataReturnsInvalidImage)
{
    const uint8 garbage[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
    auto* stream = new MemoryInputStream (garbage, sizeof (garbage), false);

    TgaImageFormatReader reader (stream);

    EXPECT_EQ (reader.width, 0);
    EXPECT_EQ (reader.height, 0);

    auto result = reader.readImage();
    EXPECT_FALSE (result.isValid());
}

TEST (TgaImageFormatTests, NullStreamReturnsInvalidImage)
{
    TgaImageFormatReader reader (nullptr);

    EXPECT_EQ (reader.width, 0);
    EXPECT_EQ (reader.height, 0);

    auto result = reader.readImage();
    EXPECT_FALSE (result.isValid());
}

// ======================================================================
// Reader pixel format detection
// ======================================================================

TEST (TgaImageFormatTests, ReaderDetectsRgbPixelFormat)
{
    auto [original, decoded] = tgaRoundtrip (generateSolidImage (8, 8, PixelFormat::RGB));
    EXPECT_EQ (decoded.getPixelFormat(), PixelFormat::RGB);
}

TEST (TgaImageFormatTests, ReaderDetectsRgbaPixelFormat)
{
    auto [original, decoded] = tgaRoundtrip (generateSolidImage (8, 8, PixelFormat::RGBA));
    EXPECT_EQ (decoded.getPixelFormat(), PixelFormat::RGBA);
}

// ======================================================================
// Roundtrip tests: uncompressed (type 2)
// ======================================================================

TEST (TgaImageFormatTests, WriteAndReadBackRgbProducesPixelIdenticalImage)
{
    auto original = generateTestImage (16, 16, PixelFormat::RGB);
    auto [_, decoded] = tgaRoundtrip (original, false);

    EXPECT_EQ (decoded.getWidth(), original.getWidth());
    EXPECT_EQ (decoded.getHeight(), original.getHeight());
    EXPECT_EQ (decoded.getPixelFormat(), original.getPixelFormat());
    EXPECT_TRUE (imagesAreEqual (original, decoded, 0));
}

TEST (TgaImageFormatTests, WriteAndReadBackRgbaProducesPixelIdenticalImage)
{
    auto original = generateTestImage (16, 16, PixelFormat::RGBA);
    auto [_, decoded] = tgaRoundtrip (original, false);

    ASSERT_EQ (decoded.getWidth(), original.getWidth());
    ASSERT_EQ (decoded.getHeight(), original.getHeight());
    EXPECT_EQ (decoded.getPixelFormat(), original.getPixelFormat());

    for (int y = 0; y < original.getHeight(); ++y)
    {
        for (int x = 0; x < original.getWidth(); ++x)
        {
            EXPECT_EQ (decoded.getPixel (x, y), original.getPixel (x, y))
                << "Pixel mismatch at (" << x << ", " << y << ")";
        }
    }
}

TEST (TgaImageFormatTests, SolidColorRgbRoundtripIsExact)
{
    auto original = generateSolidImage (12, 8, PixelFormat::RGB, 0xFF3355AAu);
    auto [_, decoded] = tgaRoundtrip (original, false);
    EXPECT_TRUE (imagesAreEqual (original, decoded, 0));
}

TEST (TgaImageFormatTests, SolidColorRgbaRoundtripIsExact)
{
    auto original = generateSolidImage (10, 6, PixelFormat::RGBA, 0xAA224466u);
    auto [_, decoded] = tgaRoundtrip (original, false);

    for (int y = 0; y < original.getHeight(); ++y)
    {
        for (int x = 0; x < original.getWidth(); ++x)
            EXPECT_EQ (decoded.getPixel (x, y), original.getPixel (x, y));
    }
}

TEST (TgaImageFormatTests, SolidColorFullyOpaqueRgbaRoundtrip)
{
    auto original = generateSolidImage (8, 8, PixelFormat::RGBA, 0xFF00FF00u); // opaque green
    auto [_, decoded] = tgaRoundtrip (original, false);

    for (int y = 0; y < original.getHeight(); ++y)
    {
        for (int x = 0; x < original.getWidth(); ++x)
            EXPECT_EQ (decoded.getPixel (x, y), 0xFF00FF00u);
    }
}

TEST (TgaImageFormatTests, SolidColorFullyTransparentRgbaRoundtrip)
{
    auto original = generateSolidImage (8, 8, PixelFormat::RGBA, 0x00000000u);
    auto [_, decoded] = tgaRoundtrip (original, false);

    for (int y = 0; y < original.getHeight(); ++y)
    {
        for (int x = 0; x < original.getWidth(); ++x)
            EXPECT_EQ (decoded.getPixel (x, y), 0x00000000u);
    }
}

TEST (TgaImageFormatTests, SolidColorSemiTransparentRgbaRoundtrip)
{
    auto original = generateSolidImage (6, 6, PixelFormat::RGBA, 0x7FFF0000u); // 50% alpha red
    auto [_, decoded] = tgaRoundtrip (original, false);

    for (int y = 0; y < original.getHeight(); ++y)
    {
        for (int x = 0; x < original.getWidth(); ++x)
            EXPECT_EQ (decoded.getPixel (x, y), 0x7FFF0000u);
    }
}

TEST (TgaImageFormatTests, VariousSizesRoundtripCorrectly)
{
    const int sizes[][2] = {
        { 1, 1 }, { 1, 32 }, { 32, 1 }, { 7, 13 }, { 64, 48 }, { 128, 3 }, { 3, 128 }
    };

    for (auto [w, h] : sizes)
    {
        auto original = generateTestImage (w, h, PixelFormat::RGB);
        auto [_, decoded] = tgaRoundtrip (original, false);

        EXPECT_EQ (decoded.getWidth(), w);
        EXPECT_EQ (decoded.getHeight(), h);
        EXPECT_TRUE (imagesAreEqual (original, decoded, 0))
            << "Size mismatch at " << w << "x" << h;
    }
}

TEST (TgaImageFormatTests, VariousSizesRgbaRoundtripCorrectly)
{
    const int sizes[][2] = { { 1, 1 }, { 5, 17 }, { 32, 1 }, { 13, 11 } };

    for (auto [w, h] : sizes)
    {
        auto original = generateTestImage (w, h, PixelFormat::RGBA);
        auto [_, decoded] = tgaRoundtrip (original, false);

        EXPECT_EQ (decoded.getWidth(), w);
        EXPECT_EQ (decoded.getHeight(), h);
        EXPECT_TRUE (imagesAreEqualRGBA (original, decoded, 0))
            << "RGBA size mismatch at " << w << "x" << h;
    }
}

// ======================================================================
// Roundtrip tests: RLE compressed (type 10)
// ======================================================================

TEST (TgaImageFormatTests, RleRgbRoundtripIsPixelIdentical)
{
    auto original = generateTestImage (16, 16, PixelFormat::RGB);
    auto [_, decoded] = tgaRoundtrip (original, true);

    EXPECT_EQ (decoded.getWidth(), original.getWidth());
    EXPECT_EQ (decoded.getHeight(), original.getHeight());
    EXPECT_TRUE (imagesAreEqual (original, decoded, 0));
}

TEST (TgaImageFormatTests, RleRgbaRoundtripIsPixelIdentical)
{
    auto original = generateTestImage (16, 16, PixelFormat::RGBA);
    auto [_, decoded] = tgaRoundtrip (original, true);

    ASSERT_EQ (decoded.getWidth(), original.getWidth());
    ASSERT_EQ (decoded.getHeight(), original.getHeight());

    for (int y = 0; y < original.getHeight(); ++y)
    {
        for (int x = 0; x < original.getWidth(); ++x)
            EXPECT_EQ (decoded.getPixel (x, y), original.getPixel (x, y));
    }
}

TEST (TgaImageFormatTests, RleSolidColorRgbRoundtrip)
{
    auto original = generateSolidImage (32, 32, PixelFormat::RGB, 0xFF4477AAu);
    auto [_, decoded] = tgaRoundtrip (original, true);
    EXPECT_TRUE (imagesAreEqual (original, decoded, 0));
}

TEST (TgaImageFormatTests, RleSolidColorRgbaRoundtrip)
{
    auto original = generateSolidImage (32, 32, PixelFormat::RGBA, 0xCC224488u);
    auto [_, decoded] = tgaRoundtrip (original, true);

    for (int y = 0; y < original.getHeight(); ++y)
    {
        for (int x = 0; x < original.getWidth(); ++x)
            EXPECT_EQ (decoded.getPixel (x, y), original.getPixel (x, y));
    }
}

TEST (TgaImageFormatTests, RleVariousSizesRoundtrip)
{
    const int sizes[][2] = { { 1, 1 }, { 1, 50 }, { 50, 1 }, { 15, 9 }, { 64, 32 } };

    for (auto [w, h] : sizes)
    {
        auto original = generateTestImage (w, h, PixelFormat::RGB);
        auto [_, decoded] = tgaRoundtrip (original, true);

        EXPECT_TRUE (imagesAreEqual (original, decoded, 0))
            << "RLE size mismatch at " << w << "x" << h;
    }
}

TEST (TgaImageFormatTests, RleProducesSmallerOutputForSolidImage)
{
    auto original = generateSolidImage (64, 64, PixelFormat::RGB);

    auto* uncompressedStream = new MemoryOutputStream();
    TgaImageFormatWriter uncompressedWriter (uncompressedStream, PixelFormat::RGB, false);
    ASSERT_TRUE (uncompressedWriter.writeImage (original));

    auto* compressedStream = new MemoryOutputStream();
    TgaImageFormatWriter compressedWriter (compressedStream, PixelFormat::RGB, true);
    ASSERT_TRUE (compressedWriter.writeImage (original));

    // RLE should produce significantly smaller output for solid images
    auto uncompressedSize = uncompressedStream->getDataSize();
    auto compressedSize = compressedStream->getDataSize();

    EXPECT_LT (compressedSize, uncompressedSize)
        << "Uncompressed: " << uncompressedSize << " bytes, RLE: " << compressedSize << " bytes";
}

// ======================================================================
// File-based roundtrip tests
// ======================================================================

TEST (TgaImageFormatTests, FileRoundtripRgbViaTempFile)
{
    auto original = generateTestImage (16, 16, PixelFormat::RGB);
    auto tempFile = File::createTempFile (".tga");

    {
        auto* fos = tempFile.createOutputStream().release();
        TgaImageFormatWriter writer (fos, PixelFormat::RGB, false);
        ASSERT_TRUE (writer.writeImage (original));
    }

    {
        auto* fis = tempFile.createInputStream().release();
        TgaImageFormatReader reader (fis);
        auto result = reader.readImage();

        ASSERT_TRUE (result.isValid());
        EXPECT_EQ (result.getWidth(), original.getWidth());
        EXPECT_EQ (result.getHeight(), original.getHeight());
        EXPECT_TRUE (imagesAreEqual (original, result, 0));
    }

    tempFile.deleteFile();
}

TEST (TgaImageFormatTests, FileRoundtripRgbaViaTempFile)
{
    auto original = generateTestImage (8, 12, PixelFormat::RGBA);
    auto tempFile = File::createTempFile (".tga");

    {
        auto* fos = tempFile.createOutputStream().release();
        TgaImageFormatWriter writer (fos, PixelFormat::RGBA, false);
        ASSERT_TRUE (writer.writeImage (original));
    }

    {
        auto* fis = tempFile.createInputStream().release();
        TgaImageFormatReader reader (fis);
        auto result = reader.readImage();

        ASSERT_TRUE (result.isValid());
        for (int y = 0; y < original.getHeight(); ++y)
        {
            for (int x = 0; x < original.getWidth(); ++x)
                EXPECT_EQ (result.getPixel (x, y), original.getPixel (x, y));
        }
    }

    tempFile.deleteFile();
}

TEST (TgaImageFormatTests, FileRoundtripRleViaTempFile)
{
    auto original = generateTestImage (12, 12, PixelFormat::RGB);
    auto tempFile = File::createTempFile (".tga");

    {
        auto* fos = tempFile.createOutputStream().release();
        TgaImageFormatWriter writer (fos, PixelFormat::RGB, true);
        ASSERT_TRUE (writer.writeImage (original));
    }

    {
        auto* fis = tempFile.createInputStream().release();
        TgaImageFormatReader reader (fis);
        auto result = reader.readImage();

        ASSERT_TRUE (result.isValid());
        EXPECT_TRUE (imagesAreEqual (original, result, 0));
    }

    tempFile.deleteFile();
}

TEST (TgaImageFormatTests, SavedFileHasExpectedMinSize)
{
    auto original = generateSolidImage (4, 4, PixelFormat::RGB);
    auto tempFile = File::createTempFile (".tga");

    {
        auto* fos = tempFile.createOutputStream().release();
        TgaImageFormatWriter writer (fos, PixelFormat::RGB, false);
        ASSERT_TRUE (writer.writeImage (original));
    }

    // 18-byte header + 4*4*3 pixel data + 26-byte footer = 92 bytes minimum
    EXPECT_GT (tempFile.getSize(), 18 + 4 * 4 * 3);
    tempFile.deleteFile();
}

TEST (TgaImageFormatTests, SavedFileContainsFooter)
{
    auto original = generateSolidImage (4, 4, PixelFormat::RGB);
    auto tempFile = File::createTempFile (".tga");

    {
        auto* fos = tempFile.createOutputStream().release();
        TgaImageFormatWriter writer (fos, PixelFormat::RGB, false);
        ASSERT_TRUE (writer.writeImage (original));
    }

    auto* fis = tempFile.createInputStream().release();
    fis->setPosition (fis->getTotalLength() - 26);

    // Footers offset bytes + signature
    EXPECT_EQ (fis->readInt(), 0); // extension area offset
    EXPECT_EQ (fis->readInt(), 0); // developer directory offset

    char sig[17] = {};
    fis->read (sig, 16);
    EXPECT_STREQ (sig, "TRUEVISION-XFILE");

    delete fis;
    tempFile.deleteFile();
}

// ======================================================================
// Image::loadFromData tests
// ======================================================================

TEST (TgaImageFormatTests, LoadFromDataRoundtripRgb)
{
    auto original = generateTestImage (12, 12, PixelFormat::RGB);

    auto* rawStream = new MemoryOutputStream();
    TgaImageFormatWriter writer (rawStream, PixelFormat::RGB, false);
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

TEST (TgaImageFormatTests, LoadFromDataRoundtripRgba)
{
    auto original = generateTestImage (8, 8, PixelFormat::RGBA);

    auto* rawStream = new MemoryOutputStream();
    TgaImageFormatWriter writer (rawStream, PixelFormat::RGBA, false);
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

TEST (TgaImageFormatTests, LoadFromDataRoundtripRle)
{
    auto original = generateTestImage (12, 12, PixelFormat::RGB);

    auto* rawStream = new MemoryOutputStream();
    TgaImageFormatWriter writer (rawStream, PixelFormat::RGB, true);
    ASSERT_TRUE (writer.writeImage (original));

    const auto* bytes = static_cast<const uint8*> (rawStream->getData());
    const auto size = rawStream->getDataSize();

    auto result = Image::loadFromData (Span<const uint8> (bytes, size));
    ASSERT_TRUE (result.wasOk());

    const Image& decoded = result.getValue();
    EXPECT_TRUE (imagesAreEqual (original, decoded));
}

// ======================================================================
// Format property tests
// ======================================================================

TEST (TgaImageFormatTests, FormatNameIsCorrect)
{
    TgaImageFormat fmt;
    EXPECT_EQ (fmt.getFormatName(), String ("TGA Image"));
}

TEST (TgaImageFormatTests, ExtensionsAreCorrect)
{
    TgaImageFormat fmt;

    auto readExts = fmt.getFileExtensions (ImageFormat::forReading);
    ASSERT_EQ (readExts.size(), 4);
    EXPECT_TRUE (readExts.contains (String (".tga")));
    EXPECT_TRUE (readExts.contains (String (".icb")));
    EXPECT_TRUE (readExts.contains (String (".vda")));
    EXPECT_TRUE (readExts.contains (String (".vst")));

    auto writeExts = fmt.getFileExtensions (ImageFormat::forWriting);
    ASSERT_EQ (writeExts.size(), 4);
    EXPECT_TRUE (writeExts.contains (String (".tga")));
}

TEST (TgaImageFormatTests, PossiblePixelFormatsIncludeRgbAndRgba)
{
    TgaImageFormat fmt;
    auto formats = fmt.getPossiblePixelFormats();

    EXPECT_EQ (formats.size(), 2);
    EXPECT_TRUE (formats.contains (PixelFormat::RGB));
    EXPECT_TRUE (formats.contains (PixelFormat::RGBA));
}

TEST (TgaImageFormatTests, IsNotCompressedByDefault)
{
    TgaImageFormat fmt;
    EXPECT_FALSE (fmt.isCompressed());
}

TEST (TgaImageFormatTests, HasQualityOptions)
{
    TgaImageFormat fmt;
    auto options = fmt.getQualityOptions();

    ASSERT_EQ (options.size(), 2);
    EXPECT_EQ (options[0], String ("Uncompressed"));
    EXPECT_EQ (options[1], String ("RLE Compressed"));
}

// ======================================================================
// canHandleFile tests
// ======================================================================

TEST (TgaImageFormatTests, CanHandleFileForTgaExtensions)
{
    TgaImageFormat fmt;

    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/image.tga"), ImageFormat::forReading));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/image.TGA"), ImageFormat::forReading));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/image.tga"), ImageFormat::forWriting));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/icon.icb"), ImageFormat::forReading));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/icon.vda"), ImageFormat::forReading));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/icon.vst"), ImageFormat::forReading));
}

TEST (TgaImageFormatTests, CanHandleFileRejectsWrongExtensions)
{
    TgaImageFormat fmt;

    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image.png"), ImageFormat::forReading));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image.jpg"), ImageFormat::forReading));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image.bmp"), ImageFormat::forReading));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image.ppm"), ImageFormat::forReading));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image"), ImageFormat::forReading));
}

// ======================================================================
// canHandleStream tests
// ======================================================================

TEST (TgaImageFormatTests, CanHandleStreamDetectsValidImageType2)
{
    TgaImageFormat fmt;

    auto header = makeTgaHeader (2, 10, 10, 24); // type 2 = uncompressed true-color
    MemoryInputStream stream (header.data(), header.size(), false);

    EXPECT_TRUE (fmt.canHandleStream (stream, ImageFormat::forReading));
    EXPECT_EQ (stream.getPosition(), 0);
}

TEST (TgaImageFormatTests, CanHandleStreamDetectsValidImageType10)
{
    TgaImageFormat fmt;

    auto header = makeTgaHeader (10, 10, 10, 24); // type 10 = RLE true-color
    MemoryInputStream stream (header.data(), header.size(), false);

    EXPECT_TRUE (fmt.canHandleStream (stream, ImageFormat::forReading));
    EXPECT_EQ (stream.getPosition(), 0);
}

TEST (TgaImageFormatTests, CanHandleStreamDetectsValidImageType3)
{
    TgaImageFormat fmt;

    auto header = makeTgaHeader (3, 10, 10, 8); // type 3 = uncompressed grayscale
    MemoryInputStream stream (header.data(), header.size(), false);

    EXPECT_TRUE (fmt.canHandleStream (stream, ImageFormat::forReading));
}

TEST (TgaImageFormatTests, CanHandleStreamDetectsValidImageType11)
{
    TgaImageFormat fmt;

    auto header = makeTgaHeader (11, 10, 10, 8); // type 11 = RLE grayscale
    MemoryInputStream stream (header.data(), header.size(), false);

    EXPECT_TRUE (fmt.canHandleStream (stream, ImageFormat::forReading));
}

TEST (TgaImageFormatTests, CanHandleStreamDetectsValidImageType1)
{
    TgaImageFormat fmt;

    auto header = makeTgaHeader (1, 10, 10, 8); // type 1 = uncompressed color-mapped
    MemoryInputStream stream (header.data(), header.size(), false);

    EXPECT_TRUE (fmt.canHandleStream (stream, ImageFormat::forReading));
}

TEST (TgaImageFormatTests, CanHandleStreamDetectsValidImageType9)
{
    TgaImageFormat fmt;

    auto header = makeTgaHeader (9, 10, 10, 8); // type 9 = RLE color-mapped
    MemoryInputStream stream (header.data(), header.size(), false);

    EXPECT_TRUE (fmt.canHandleStream (stream, ImageFormat::forReading));
}

TEST (TgaImageFormatTests, CanHandleStreamRejectsInvalidImageType)
{
    TgaImageFormat fmt;

    auto header = makeTgaHeader (0, 10, 10, 24); // type 0 = no image data
    MemoryInputStream stream (header.data(), header.size(), false);

    EXPECT_FALSE (fmt.canHandleStream (stream, ImageFormat::forReading));
    EXPECT_EQ (stream.getPosition(), 0);
}

TEST (TgaImageFormatTests, CanHandleStreamRejectsTooHighImageType)
{
    TgaImageFormat fmt;

    auto header = makeTgaHeader (32, 10, 10, 24); // type 32 = vendor-specific, not in 1-3,9-11
    MemoryInputStream stream (header.data(), header.size(), false);

    EXPECT_FALSE (fmt.canHandleStream (stream, ImageFormat::forReading));
}

TEST (TgaImageFormatTests, CanHandleStreamRejectsEmptyStream)
{
    TgaImageFormat fmt;

    MemoryInputStream stream (static_cast<const void*> (nullptr), 0, false);
    EXPECT_FALSE (fmt.canHandleStream (stream, ImageFormat::forReading));
}

// ======================================================================
// Manager integration tests
// ======================================================================

TEST (TgaImageFormatTests, ManagerCreatesReaderForStream)
{
    auto* rawStream = new MemoryOutputStream();
    TgaImageFormatWriter writer (rawStream, PixelFormat::RGB, false);
    ASSERT_TRUE (writer.writeImage (generateTestImage (4, 4, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto reader = manager.createReaderFor (inStream);
    ASSERT_NE (reader, nullptr);
    EXPECT_EQ (reader->getFormatName(), String ("TGA Image"));
    EXPECT_EQ (reader->width, 4);
    EXPECT_EQ (reader->height, 4);

    auto image = reader->readImage();
    EXPECT_TRUE (image.isValid());
}

TEST (TgaImageFormatTests, ManagerCreatesWriterForFile)
{
    auto tempFile = File::createTempFile (".tga");

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto writer = manager.createWriterFor (tempFile, PixelFormat::RGB);
    ASSERT_NE (writer, nullptr);
    EXPECT_EQ (writer->getFormatName(), String ("TGA Image"));

    tempFile.deleteFile();
}

TEST (TgaImageFormatTests, ManagerRoundtripViaTempFile)
{
    auto original = generateTestImage (12, 10, PixelFormat::RGBA);
    auto tempFile = File::createTempFile (".tga");

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    // Write
    {
        auto writer = manager.createWriterFor (tempFile, PixelFormat::RGBA);
        ASSERT_NE (writer, nullptr);
        ASSERT_TRUE (writer->writeImage (original));
    }

    // Read
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

TEST (TgaImageFormatTests, ManagerRegisterTgaType)
{
    ImageFormatManager manager;
    manager.registerDefaultFormats (ImageFormatType::tga);

    // Verify TGA is the only registered format
    auto* rawStream = new MemoryOutputStream();
    TgaImageFormatWriter writer (rawStream, PixelFormat::RGB, false);
    ASSERT_TRUE (writer.writeImage (generateTestImage (8, 8, PixelFormat::RGB)));

    auto* tgaStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    auto reader = manager.createReaderFor (tgaStream);
    ASSERT_NE (reader, nullptr);
    EXPECT_EQ (reader->getFormatName(), String ("TGA Image"));
}

// ======================================================================
// Writer: invalid input tests
// ======================================================================

TEST (TgaImageFormatTests, WriteInvalidImageReturnsFalse)
{
    Image invalidImage;

    auto* rawStream = new MemoryOutputStream();
    TgaImageFormatWriter writer (rawStream, PixelFormat::RGB, false);
    EXPECT_FALSE (writer.writeImage (invalidImage));
}

TEST (TgaImageFormatTests, WriteWithNullStreamReturnsFalse)
{
    auto image = generateSolidImage (4, 4, PixelFormat::RGB);
    TgaImageFormatWriter writer (nullptr, PixelFormat::RGB, false);
    EXPECT_FALSE (writer.writeImage (image));
}

// ======================================================================
// TGA-specific: type 2 vs type 10 header byte
// ======================================================================

TEST (TgaImageFormatTests, UncompressedWriterProducesType2Header)
{
    auto image = generateSolidImage (4, 4, PixelFormat::RGB);
    auto* rawStream = new MemoryOutputStream();
    TgaImageFormatWriter writer (rawStream, PixelFormat::RGB, false);
    ASSERT_TRUE (writer.writeImage (image));

    const auto* bytes = static_cast<const uint8*> (rawStream->getData());
    EXPECT_EQ (bytes[2], 2); // image type byte = 2 (uncompressed true-color)
}

TEST (TgaImageFormatTests, RleWriterProducesType10Header)
{
    auto image = generateSolidImage (4, 4, PixelFormat::RGB);
    auto* rawStream = new MemoryOutputStream();
    TgaImageFormatWriter writer (rawStream, PixelFormat::RGB, true);
    ASSERT_TRUE (writer.writeImage (image));

    const auto* bytes = static_cast<const uint8*> (rawStream->getData());
    EXPECT_EQ (bytes[2], 10); // image type byte = 10 (RLE true-color)
}

// ======================================================================
// Writer: descriptor byte tests
// ======================================================================

TEST (TgaImageFormatTests, WriterSetsTopDownBit)
{
    auto image = generateSolidImage (4, 4, PixelFormat::RGB);
    auto* rawStream = new MemoryOutputStream();
    TgaImageFormatWriter writer (rawStream, PixelFormat::RGB, false);
    ASSERT_TRUE (writer.writeImage (image));

    const auto* bytes = static_cast<const uint8*> (rawStream->getData());
    EXPECT_NE (bytes[17] & 0x20, 0); // bit 5 (top-down) should be set
}

TEST (TgaImageFormatTests, WriterSetsAttributeBitsForRgba)
{
    auto image = generateSolidImage (4, 4, PixelFormat::RGBA);
    auto* rawStream = new MemoryOutputStream();
    TgaImageFormatWriter writer (rawStream, PixelFormat::RGBA, false);
    ASSERT_TRUE (writer.writeImage (image));

    const auto* bytes = static_cast<const uint8*> (rawStream->getData());
    EXPECT_EQ (bytes[17] & 0x0F, 8); // 8 attribute bits (alpha)
}

TEST (TgaImageFormatTests, WriterSetsZeroAttributeBitsForRgb)
{
    auto image = generateSolidImage (4, 4, PixelFormat::RGB);
    auto* rawStream = new MemoryOutputStream();
    TgaImageFormatWriter writer (rawStream, PixelFormat::RGB, false);
    ASSERT_TRUE (writer.writeImage (image));

    const auto* bytes = static_cast<const uint8*> (rawStream->getData());
    EXPECT_EQ (bytes[17] & 0x0F, 0); // 0 attribute bits (no alpha)
}

// ======================================================================
// TGA-specific: mutual RLE decode of uncompressed and vice versa
// ======================================================================

TEST (TgaImageFormatTests, RleReaderCanDecodeUncompressedFile)
{
    // RLE reader naturally handles uncompressed data because each RLE raw
    // packet is just literal pixels — and uncompressed is all-raw packets.
    auto original = generateTestImage (8, 8, PixelFormat::RGB);

    auto* rawStream = new MemoryOutputStream();
    TgaImageFormatWriter writer (rawStream, PixelFormat::RGB, true);
    ASSERT_TRUE (writer.writeImage (original));

    // Decode via reader constructor which auto-detects image type
    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    TgaImageFormatReader reader (inStream);
    auto decoded = reader.readImage();

    EXPECT_TRUE (imagesAreEqual (original, decoded, 0));
}

// ======================================================================
// Const-correctness: ImageFormat factory
// ======================================================================

TEST (TgaImageFormatTests, FactoryCreatesReader)
{
    TgaImageFormat fmt;

    auto image = generateSolidImage (4, 4, PixelFormat::RGB);
    auto* rawStream = new MemoryOutputStream();
    TgaImageFormatWriter writer (rawStream, PixelFormat::RGB, false);
    ASSERT_TRUE (writer.writeImage (image));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    auto reader = fmt.createReaderFor (inStream);

    ASSERT_NE (reader, nullptr);
    auto decoded = reader->readImage();
    EXPECT_TRUE (decoded.isValid());
    EXPECT_EQ (decoded.getWidth(), 4);
    EXPECT_EQ (decoded.getHeight(), 4);
}

TEST (TgaImageFormatTests, FactoryCreatesWriterForRgb)
{
    TgaImageFormat fmt;

    auto* outStream = new MemoryOutputStream();
    auto writer = fmt.createWriterFor (outStream, PixelFormat::RGB, {}, 0);

    ASSERT_NE (writer, nullptr);
    EXPECT_TRUE (writer->writeImage (generateSolidImage (2, 2, PixelFormat::RGB)));
}

TEST (TgaImageFormatTests, FactoryCreatesWriterForRgba)
{
    TgaImageFormat fmt;

    auto* outStream = new MemoryOutputStream();
    auto writer = fmt.createWriterFor (outStream, PixelFormat::RGBA, {}, 0);

    ASSERT_NE (writer, nullptr);
    EXPECT_TRUE (writer->writeImage (generateSolidImage (2, 2, PixelFormat::RGBA)));
}

TEST (TgaImageFormatTests, FactoryCreatesWriterWithRleQualityOption)
{
    TgaImageFormat fmt;

    auto* outStream = new MemoryOutputStream();
    auto writer = fmt.createWriterFor (outStream, PixelFormat::RGB, {}, 1);

    ASSERT_NE (writer, nullptr);
    EXPECT_TRUE (writer->writeImage (generateSolidImage (4, 4, PixelFormat::RGB)));

    // Verify it wrote type 10 (RLE)
    const auto* bytes = static_cast<const uint8*> (outStream->getData());
    EXPECT_EQ (bytes[2], 10);
}

// ======================================================================
// Fixture file tests: load real TGA images from tests/data/images/
// ======================================================================

namespace
{

/** Loads an image directly via TgaImageFormatReader from a file in tests/data/images/. */
Image loadTgaFromFixture (const String& filename)
{
    auto file = getTestDataImagesDirectory().getChildFile (filename);
    auto* fis = file.createInputStream().release();
    if (fis == nullptr)
        return {};

    TgaImageFormatReader reader (fis);
    return reader.readImage();
}

} // namespace

// ----------------------------------------------------------------------
// football_seal.tga — type 1, color-mapped, 350×350, 8-bit
// ----------------------------------------------------------------------

TEST (TgaImageFormatTests, LoadFootballSealHasCorrectDimensions)
{
    auto image = loadTgaFromFixture ("football_seal.tga");
    ASSERT_TRUE (image.isValid());

    EXPECT_EQ (image.getWidth(), 350);
    EXPECT_EQ (image.getHeight(), 350);
    EXPECT_EQ (image.getPixelFormat(), PixelFormat::RGB);
}

TEST (TgaImageFormatTests, FootballSealRoundtripUncompressed)
{
    auto original = loadTgaFromFixture ("football_seal.tga");
    ASSERT_TRUE (original.isValid());

    auto* rawStream = new MemoryOutputStream();
    TgaImageFormatWriter writer (rawStream, PixelFormat::RGB, false);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    TgaImageFormatReader reader (inStream);
    auto decoded = reader.readImage();

    ASSERT_TRUE (decoded.isValid());
    EXPECT_EQ (decoded.getWidth(), original.getWidth());
    EXPECT_EQ (decoded.getHeight(), original.getHeight());
    EXPECT_EQ (decoded.getPixelFormat(), original.getPixelFormat());
    EXPECT_TRUE (imagesAreEqual (original, decoded, 0));
}

TEST (TgaImageFormatTests, FootballSealRoundtripRle)
{
    auto original = loadTgaFromFixture ("football_seal.tga");
    ASSERT_TRUE (original.isValid());

    auto* rawStream = new MemoryOutputStream();
    TgaImageFormatWriter writer (rawStream, PixelFormat::RGB, true);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    TgaImageFormatReader reader (inStream);
    auto decoded = reader.readImage();

    ASSERT_TRUE (decoded.isValid());
    EXPECT_TRUE (imagesAreEqual (original, decoded, 0));
}

TEST (TgaImageFormatTests, FootballSealRoundtripViaTempFile)
{
    auto original = loadTgaFromFixture ("football_seal.tga");
    ASSERT_TRUE (original.isValid());

    auto tempFile = File::createTempFile (".tga");

    {
        auto* fos = tempFile.createOutputStream().release();
        TgaImageFormatWriter writer (fos, PixelFormat::RGB, false);
        ASSERT_TRUE (writer.writeImage (original));
    }

    {
        auto* fis = tempFile.createInputStream().release();
        TgaImageFormatReader reader (fis);
        auto decoded = reader.readImage();

        ASSERT_TRUE (decoded.isValid());
        EXPECT_EQ (decoded.getWidth(), original.getWidth());
        EXPECT_EQ (decoded.getHeight(), original.getHeight());
        EXPECT_TRUE (imagesAreEqual (original, decoded, 0));
    }

    tempFile.deleteFile();
}

TEST (TgaImageFormatTests, FootballSealLoadFromData)
{
    auto original = loadTgaFromFixture ("football_seal.tga");
    ASSERT_TRUE (original.isValid());

    // Re-encode to get clean bytes
    auto* rawStream = new MemoryOutputStream();
    TgaImageFormatWriter writer (rawStream, PixelFormat::RGB, false);
    ASSERT_TRUE (writer.writeImage (original));

    const auto* bytes = static_cast<const uint8*> (rawStream->getData());
    const auto size = rawStream->getDataSize();

    auto result = Image::loadFromData (Span<const uint8> (bytes, size));
    ASSERT_TRUE (result.wasOk());

    const Image& decoded = result.getValue();
    EXPECT_EQ (decoded.getWidth(), 350);
    EXPECT_EQ (decoded.getHeight(), 350);
    EXPECT_TRUE (imagesAreEqual (original, decoded, 0));
}

// ----------------------------------------------------------------------
// shuttle.tga — type 10, RLE true-color, 640×480, 24-bit
// ----------------------------------------------------------------------

TEST (TgaImageFormatTests, LoadShuttleHasCorrectDimensions)
{
    auto image = loadTgaFromFixture ("shuttle.tga");
    ASSERT_TRUE (image.isValid());

    EXPECT_EQ (image.getWidth(), 640);
    EXPECT_EQ (image.getHeight(), 480);
    EXPECT_EQ (image.getPixelFormat(), PixelFormat::RGB);
}

TEST (TgaImageFormatTests, ShuttleRoundtripUncompressed)
{
    auto original = loadTgaFromFixture ("shuttle.tga");
    ASSERT_TRUE (original.isValid());

    auto* rawStream = new MemoryOutputStream();
    TgaImageFormatWriter writer (rawStream, PixelFormat::RGB, false);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    TgaImageFormatReader reader (inStream);
    auto decoded = reader.readImage();

    ASSERT_TRUE (decoded.isValid());
    EXPECT_EQ (decoded.getWidth(), original.getWidth());
    EXPECT_EQ (decoded.getHeight(), original.getHeight());
    EXPECT_EQ (decoded.getPixelFormat(), original.getPixelFormat());
    EXPECT_TRUE (imagesAreEqual (original, decoded, 0));
}

TEST (TgaImageFormatTests, ShuttleRoundtripRle)
{
    auto original = loadTgaFromFixture ("shuttle.tga");
    ASSERT_TRUE (original.isValid());

    auto* rawStream = new MemoryOutputStream();
    TgaImageFormatWriter writer (rawStream, PixelFormat::RGB, true);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    TgaImageFormatReader reader (inStream);
    auto decoded = reader.readImage();

    ASSERT_TRUE (decoded.isValid());
    EXPECT_TRUE (imagesAreEqual (original, decoded, 0));
}

TEST (TgaImageFormatTests, ShuttleRoundtripViaTempFile)
{
    auto original = loadTgaFromFixture ("shuttle.tga");
    ASSERT_TRUE (original.isValid());

    auto tempFile = File::createTempFile (".tga");

    {
        auto* fos = tempFile.createOutputStream().release();
        TgaImageFormatWriter writer (fos, PixelFormat::RGB, false);
        ASSERT_TRUE (writer.writeImage (original));
    }

    {
        auto* fis = tempFile.createInputStream().release();
        TgaImageFormatReader reader (fis);
        auto decoded = reader.readImage();

        ASSERT_TRUE (decoded.isValid());
        EXPECT_EQ (decoded.getWidth(), original.getWidth());
        EXPECT_EQ (decoded.getHeight(), original.getHeight());
        EXPECT_TRUE (imagesAreEqual (original, decoded, 0));
    }

    tempFile.deleteFile();
}

TEST (TgaImageFormatTests, ShuttleLoadFromData)
{
    auto original = loadTgaFromFixture ("shuttle.tga");
    ASSERT_TRUE (original.isValid());

    auto* rawStream = new MemoryOutputStream();
    TgaImageFormatWriter writer (rawStream, PixelFormat::RGB, false);
    ASSERT_TRUE (writer.writeImage (original));

    const auto* bytes = static_cast<const uint8*> (rawStream->getData());
    const auto size = rawStream->getDataSize();

    auto result = Image::loadFromData (Span<const uint8> (bytes, size));
    ASSERT_TRUE (result.wasOk());

    const Image& decoded = result.getValue();
    EXPECT_EQ (decoded.getWidth(), 640);
    EXPECT_EQ (decoded.getHeight(), 480);
    EXPECT_TRUE (imagesAreEqual (original, decoded, 0));
}

// ----------------------------------------------------------------------
// Manager integration: load fixture files via ImageFormatManager
// ----------------------------------------------------------------------

TEST (TgaImageFormatTests, ManagerLoadsFootballSeal)
{
    auto file = getTestDataImagesDirectory().getChildFile ("football_seal.tga");

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto reader = manager.createReaderFor (file);
    ASSERT_NE (reader, nullptr);
    EXPECT_EQ (reader->getFormatName(), String ("TGA Image"));
    EXPECT_EQ (reader->width, 350);
    EXPECT_EQ (reader->height, 350);

    auto image = reader->readImage();
    ASSERT_TRUE (image.isValid());
    EXPECT_EQ (image.getWidth(), 350);
    EXPECT_EQ (image.getHeight(), 350);
}

TEST (TgaImageFormatTests, ManagerLoadsShuttle)
{
    auto file = getTestDataImagesDirectory().getChildFile ("shuttle.tga");

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto reader = manager.createReaderFor (file);
    ASSERT_NE (reader, nullptr);
    EXPECT_EQ (reader->getFormatName(), String ("TGA Image"));
    EXPECT_EQ (reader->width, 640);
    EXPECT_EQ (reader->height, 480);

    auto image = reader->readImage();
    ASSERT_TRUE (image.isValid());
    EXPECT_EQ (image.getWidth(), 640);
    EXPECT_EQ (image.getHeight(), 480);
}

// ======================================================================
// Synthetic format coverage tests
// ======================================================================

namespace
{

/** Assembles a TGA file from header + optional ID + pixel data into a byte vector. */
std::vector<uint8> makeTgaData (const std::vector<uint8>& header,
                                const std::vector<uint8>& imageId,
                                const std::vector<uint8>& pixelData)
{
    std::vector<uint8> all;
    all.insert (all.end(), header.begin(), header.end());
    all.insert (all.end(), imageId.begin(), imageId.end());
    all.insert (all.end(), pixelData.begin(), pixelData.end());
    return all;
}

/** Convenience: no image ID. */
std::vector<uint8> makeTgaData (const std::vector<uint8>& header,
                                const std::vector<uint8>& pixelData)
{
    return makeTgaData (header, {}, pixelData);
}

/** Decodes an image from raw TGA bytes. Allocates the stream on the heap so the reader can own it. */
Image decodeRawTga (const std::vector<uint8>& data)
{
    auto* stream = new MemoryInputStream (data.data(), data.size(), true);
    TgaImageFormatReader reader (stream);
    return reader.readImage();
}

} // namespace

// ----------------------------------------------------------------------
// Grayscale decode (type 3, uncompressed)
// ----------------------------------------------------------------------

TEST (TgaImageFormatTests, DecodeGrayscaleType3)
{
    // 4×4 grayscale image with known gradient values
    auto header = makeTgaHeader (3, 4, 4, 8);
    std::vector<uint8> pixels;
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            pixels.push_back (static_cast<uint8> ((x + y * 4) * 16)); // 0, 16, 32, ...

    MemoryInputStream stream (pixels.data(), pixels.size(), false);
    stream.setPosition (0);

    // Prepend header manually by reconstructing
    std::vector<uint8> fullStream (header);
    fullStream.insert (fullStream.end(), pixels.begin(), pixels.end());

    auto dataCopy = fullStream; // keep alive
    auto image = decodeRawTga (dataCopy);

    ASSERT_TRUE (image.isValid());
    EXPECT_EQ (image.getWidth(), 4);
    EXPECT_EQ (image.getHeight(), 4);
    EXPECT_EQ (image.getPixelFormat(), PixelFormat::RGB);

    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            const uint8 gray = static_cast<uint8> ((x + y * 4) * 16);
            const uint32 expected = 0xFF000000u | (static_cast<uint32> (gray) << 16) | (static_cast<uint32> (gray) << 8) | gray;
            EXPECT_EQ (image.getPixel (x, y), expected)
                << "Grayscale mismatch at (" << x << ", " << y << ")";
        }
    }
}

// ----------------------------------------------------------------------
// RLE grayscale decode (type 11)
// ----------------------------------------------------------------------

TEST (TgaImageFormatTests, DecodeRleGrayscaleType11)
{
    // 4×4 RLE grayscale: each row is a single run of 4 identical pixels
    auto header = makeTgaHeader (11, 4, 4, 8);
    std::vector<uint8> rleData;

    // Row 0: run of 4 pixels = value 0x10
    rleData.push_back (0x80 | 3); // run packet, count=4
    rleData.push_back (0x10);
    // Row 1: run of 4 pixels = value 0x40
    rleData.push_back (0x80 | 3);
    rleData.push_back (0x40);
    // Row 2: run of 4 pixels = value 0x80
    rleData.push_back (0x80 | 3);
    rleData.push_back (0x80);
    // Row 3: run of 4 pixels = value 0xC0
    rleData.push_back (0x80 | 3);
    rleData.push_back (0xC0);

    auto stream = makeTgaData (header, rleData);
    auto image = decodeRawTga (stream);

    ASSERT_TRUE (image.isValid());
    EXPECT_EQ (image.getWidth(), 4);
    EXPECT_EQ (image.getHeight(), 4);

    const uint8 grayValues[4] = { 0x10, 0x40, 0x80, 0xC0 };
    for (int y = 0; y < 4; ++y)
    {
        const uint8 gray = grayValues[y];
        const uint32 expected = 0xFF000000u | (static_cast<uint32> (gray) << 16) | (static_cast<uint32> (gray) << 8) | gray;
        for (int x = 0; x < 4; ++x)
        {
            EXPECT_EQ (image.getPixel (x, y), expected)
                << "RLE grayscale mismatch at (" << x << ", " << y << ")";
        }
    }
}

// ----------------------------------------------------------------------
// 16-bit RGB555 true-color decode (type 2)
// ----------------------------------------------------------------------

TEST (TgaImageFormatTests, Decode16BitRgb555Type2)
{
    // 2×2 image with known 16-bit RGB555 values
    auto header = makeTgaHeader (2, 2, 2, 16);
    std::vector<uint8> pixels;

    // R=31 G=0 B=0 → full red: 0x7C00
    const uint16 fullRed = 0x7C00;
    pixels.push_back (static_cast<uint8> (fullRed & 0xFF));
    pixels.push_back (static_cast<uint8> (fullRed >> 8));

    // R=0 G=31 B=0 → full green: 0x03E0
    const uint16 fullGreen = 0x03E0;
    pixels.push_back (static_cast<uint8> (fullGreen & 0xFF));
    pixels.push_back (static_cast<uint8> (fullGreen >> 8));

    // R=0 G=0 B=31 → full blue: 0x001F
    const uint16 fullBlue = 0x001F;
    pixels.push_back (static_cast<uint8> (fullBlue & 0xFF));
    pixels.push_back (static_cast<uint8> (fullBlue >> 8));

    // R=16 G=16 B=16 → mid gray: 0x4210
    const uint16 midGray = 0x4210;
    pixels.push_back (static_cast<uint8> (midGray & 0xFF));
    pixels.push_back (static_cast<uint8> (midGray >> 8));

    auto stream = makeTgaData (header, pixels);
    auto image = decodeRawTga (stream);

    ASSERT_TRUE (image.isValid());
    EXPECT_EQ (image.getWidth(), 2);
    EXPECT_EQ (image.getHeight(), 2);
    EXPECT_EQ (image.getPixelFormat(), PixelFormat::RGB);

    // Full red pixel at (0,0): R=255, G=0, B=0
    const uint32 redPixel = image.getPixel (0, 0);
    EXPECT_GE ((redPixel >> 16) & 0xFF, 245u); // R ≈ 255
    EXPECT_LE ((redPixel >> 8) & 0xFF, 10u);   // G ≈ 0
    EXPECT_LE (redPixel & 0xFF, 10u);          // B ≈ 0

    // Full green pixel at (1,0)
    const uint32 greenPixel = image.getPixel (1, 0);
    EXPECT_LE ((greenPixel >> 16) & 0xFF, 10u);
    EXPECT_GE ((greenPixel >> 8) & 0xFF, 245u);
    EXPECT_LE (greenPixel & 0xFF, 10u);

    // Full blue pixel at (0,1)
    const uint32 bluePixel = image.getPixel (0, 1);
    EXPECT_LE ((bluePixel >> 16) & 0xFF, 10u);
    EXPECT_LE ((bluePixel >> 8) & 0xFF, 10u);
    EXPECT_GE (bluePixel & 0xFF, 245u);

    // Mid gray at (1,1)
    const uint32 grayPixel = image.getPixel (1, 1);
    const uint8 grayCh = static_cast<uint8> ((grayPixel >> 16) & 0xFF);
    EXPECT_GE (grayCh, 125u);
    EXPECT_LE (grayCh, 140u);
    EXPECT_EQ ((grayPixel >> 16) & 0xFF, (grayPixel >> 8) & 0xFF);
    EXPECT_EQ ((grayPixel >> 8) & 0xFF, grayPixel & 0xFF);
}

// ----------------------------------------------------------------------
// Image ID field skip (idLength > 0)
// ----------------------------------------------------------------------

TEST (TgaImageFormatTests, SkipsImageIdField)
{
    // Write a valid 4×4 RGB TGA, prepend idLength + ID bytes
    auto image = generateSolidImage (4, 4, PixelFormat::RGB, 0xFF123456u);

    auto* rawStream = new MemoryOutputStream();
    TgaImageFormatWriter writer (rawStream, PixelFormat::RGB, false);
    ASSERT_TRUE (writer.writeImage (image));

    const auto* origBytes = static_cast<const uint8*> (rawStream->getData());
    const auto origSize = rawStream->getDataSize();

    // Reconstruct with idLength=8 and "TESTDATA" as ID
    std::vector<uint8> modified (18 + 8 + (origSize - 18), 0);
    modified[0] = 8;                                                   // idLength = 8
    std::memcpy (modified.data() + 1, origBytes + 1, 17);              // rest of header
    std::memcpy (modified.data() + 18, "TESTDATA", 8);                 // ID bytes
    std::memcpy (modified.data() + 26, origBytes + 18, origSize - 18); // pixel data

    auto decoded = decodeRawTga (modified);

    ASSERT_TRUE (decoded.isValid());
    EXPECT_EQ (decoded.getWidth(), 4);
    EXPECT_EQ (decoded.getHeight(), 4);
    EXPECT_TRUE (imagesAreEqual (image, decoded, 0));
}

// ----------------------------------------------------------------------
// Left-right origin (descriptor bit 0x10)
// ----------------------------------------------------------------------

TEST (TgaImageFormatTests, DecodeLeftRightOrigin)
{
    // 4×2 image with a simple horizontal gradient — write as top-down+left-right
    // Then verify pixels are reversed left-to-right
    auto header = makeTgaHeader (2, 4, 2, 24, 0x30); // top-down (0x20) + left-right (0x10)

    // Row 0 pixels (left-to-right in file, but should appear right-to-left): R,G,B
    // Pixel 0: R=0x11 G=0x22 B=0x33   → should end up at x=3
    // Pixel 1: R=0x44 G=0x55 B=0x66   → should end up at x=2
    // Pixel 2: R=0x77 G=0x88 B=0x99   → should end up at x=1
    // Pixel 3: R=0xAA G=0xBB B=0xCC   → should end up at x=0
    // Row 1 (same pattern with 0xDD, 0xEE prefixes)
    std::vector<uint8> pixels;
    for (int row = 0; row < 2; ++row)
    {
        const uint8 base = static_cast<uint8> (row * 0x40);
        for (int x = 0; x < 4; ++x)
        {
            const uint8 r = static_cast<uint8> (base + static_cast<uint8> (x) * 0x10 + 1);
            const uint8 g = static_cast<uint8> (base + static_cast<uint8> (x) * 0x10 + 2);
            const uint8 b = static_cast<uint8> (base + static_cast<uint8> (x) * 0x10 + 3);
            pixels.push_back (b); // BGR order in TGA
            pixels.push_back (g);
            pixels.push_back (r);
        }
    }

    auto stream = makeTgaData (header, pixels);
    auto image = decodeRawTga (stream);

    ASSERT_TRUE (image.isValid());
    EXPECT_EQ (image.getWidth(), 4);
    EXPECT_EQ (image.getHeight(), 2);

    // With left-right origin, file pixel 0 appears at x=3, pixel 3 at x=0
    for (int row = 0; row < 2; ++row)
    {
        const uint8 base = static_cast<uint8> (row * 0x40);
        for (int fileX = 0; fileX < 4; ++fileX)
        {
            const int displayX = 3 - fileX; // left-right flip
            const uint32 pixel = image.getPixel (displayX, row);

            const uint8 expectedR = static_cast<uint8> (base + static_cast<uint8> (fileX) * 0x10 + 1);
            const uint8 expectedG = static_cast<uint8> (base + static_cast<uint8> (fileX) * 0x10 + 2);
            const uint8 expectedB = static_cast<uint8> (base + static_cast<uint8> (fileX) * 0x10 + 3);

            EXPECT_EQ ((pixel >> 16) & 0xFF, expectedR) << "R at display (" << displayX << "," << row << ")";
            EXPECT_EQ ((pixel >> 8) & 0xFF, expectedG) << "G at display (" << displayX << "," << row << ")";
            EXPECT_EQ (pixel & 0xFF, expectedB) << "B at display (" << displayX << "," << row << ")";
        }
    }
}

// ----------------------------------------------------------------------
// Bottom-up origin (descriptor bit 0x20 == 0)
// ----------------------------------------------------------------------

TEST (TgaImageFormatTests, DecodeBottomUpOrigin)
{
    // 2×3 image with unique pixel per row, bottom-up
    auto header = makeTgaHeader (2, 2, 3, 24, 0x00); // bottom-up, no left-right

    // Row 0 (bottom row in display): R=0xAA
    // Row 1: R=0xBB
    // Row 2 (top row in display): R=0xCC
    std::vector<uint8> pixels;
    for (int fileRow = 0; fileRow < 3; ++fileRow)
    {
        const uint8 r = static_cast<uint8> (0xAA + static_cast<uint8> (fileRow) * 0x11);
        for (int x = 0; x < 2; ++x)
        {
            pixels.push_back (0x00); // B
            pixels.push_back (0x00); // G
            pixels.push_back (r);    // R
        }
    }

    auto stream = makeTgaData (header, pixels);
    auto image = decodeRawTga (stream);

    ASSERT_TRUE (image.isValid());
    EXPECT_EQ (image.getWidth(), 2);
    EXPECT_EQ (image.getHeight(), 3);

    // fileRow 0 → display row 2 (bottom)
    EXPECT_EQ ((image.getPixel (0, 2) >> 16) & 0xFF, 0xAAu);
    // fileRow 1 → display row 1 (middle)
    EXPECT_EQ ((image.getPixel (0, 1) >> 16) & 0xFF, 0xBBu);
    // fileRow 2 → display row 0 (top)
    EXPECT_EQ ((image.getPixel (0, 0) >> 16) & 0xFF, 0xCCu);
}

// ----------------------------------------------------------------------
// RLE max run length (128 pixels)
// ----------------------------------------------------------------------

TEST (TgaImageFormatTests, RleHandlesMaxRunLength128)
{
    // 200×1 solid image → should produce a run of 128 followed by run of 72
    auto original = generateSolidImage (200, 1, PixelFormat::RGB, 0xFF77AA33u);

    auto* rawStream = new MemoryOutputStream();
    TgaImageFormatWriter writer (rawStream, PixelFormat::RGB, true);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    TgaImageFormatReader reader (inStream);
    auto decoded = reader.readImage();

    ASSERT_TRUE (decoded.isValid());
    EXPECT_EQ (decoded.getWidth(), 200);
    EXPECT_EQ (decoded.getHeight(), 1);
    EXPECT_TRUE (imagesAreEqual (original, decoded, 0));
}

// ----------------------------------------------------------------------
// RLE max raw packet (128 different pixels)
// ----------------------------------------------------------------------

TEST (TgaImageFormatTests, RleHandlesMaxRawPacket128)
{
    // 128×1 image where each pixel is different → should produce a raw packet of 128
    Image original (128, 1, PixelFormat::RGB);
    for (int x = 0; x < 128; ++x)
    {
        const uint8 r = static_cast<uint8> (x * 2);
        const uint8 g = static_cast<uint8> (255 - x * 2);
        const uint8 b = static_cast<uint8> ((x * 7) % 256);
        original.setPixel (x, 0, 0xFF000000u | (static_cast<uint32> (r) << 16) | (static_cast<uint32> (g) << 8) | b);
    }

    auto* rawStream = new MemoryOutputStream();
    TgaImageFormatWriter writer (rawStream, PixelFormat::RGB, true);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    TgaImageFormatReader reader (inStream);
    auto decoded = reader.readImage();

    ASSERT_TRUE (decoded.isValid());
    EXPECT_EQ (decoded.getWidth(), 128);
    EXPECT_EQ (decoded.getHeight(), 1);
    EXPECT_TRUE (imagesAreEqual (original, decoded, 0));
}

// ----------------------------------------------------------------------
// Empty image (0×0)
// ----------------------------------------------------------------------

TEST (TgaImageFormatTests, EmptyImageZeroByZeroReturnsInvalid)
{
    auto header = makeTgaHeader (2, 0, 0, 24);
    auto* stream = new MemoryInputStream (header.data(), header.size(), true);

    TgaImageFormatReader reader (stream);

    EXPECT_EQ (reader.width, 0);
    EXPECT_EQ (reader.height, 0);

    auto result = reader.readImage();
    EXPECT_FALSE (result.isValid());
}

// ----------------------------------------------------------------------
// Truncated stream
// ----------------------------------------------------------------------

TEST (TgaImageFormatTests, TruncatedStreamDoesNotCrash)
{
    // Write a valid 16×16 image, then truncate to half
    auto image = generateTestImage (16, 16, PixelFormat::RGB);

    auto* rawStream = new MemoryOutputStream();
    TgaImageFormatWriter writer (rawStream, PixelFormat::RGB, false);
    ASSERT_TRUE (writer.writeImage (image));

    const size_t fullSize = rawStream->getDataSize();
    const size_t halfSize = fullSize / 2;

    // Read with truncated data — should not crash and may return partial/invalid image
    auto* inStream = new MemoryInputStream (rawStream->getData(), halfSize, true);
    TgaImageFormatReader reader (inStream);

    ASSERT_EQ (reader.width, 16);
    ASSERT_EQ (reader.height, 16);

    // readImage should handle the truncated stream gracefully
    auto result = reader.readImage();
    // We don't assert on validity — whatever happens, it must not crash
    ignoreUnused (result);
}

// ----------------------------------------------------------------------
// Out-of-range quality option index
// ----------------------------------------------------------------------

TEST (TgaImageFormatTests, QualityOptionIndexOutOfRangeDefaultsToUncompressed)
{
    TgaImageFormat fmt;

    // Index 2 → should default to uncompressed (useRLE = false)
    auto* outStream1 = new MemoryOutputStream();
    auto writer1 = fmt.createWriterFor (outStream1, PixelFormat::RGB, {}, 2);
    ASSERT_NE (writer1, nullptr);
    ASSERT_TRUE (writer1->writeImage (generateSolidImage (4, 4, PixelFormat::RGB)));

    const auto* bytes1 = static_cast<const uint8*> (outStream1->getData());
    EXPECT_EQ (bytes1[2], 2); // type 2 (uncompressed), not type 10

    // Index -1 → should also default to uncompressed
    auto* outStream2 = new MemoryOutputStream();
    auto writer2 = fmt.createWriterFor (outStream2, PixelFormat::RGB, {}, -1);
    ASSERT_NE (writer2, nullptr);
    ASSERT_TRUE (writer2->writeImage (generateSolidImage (4, 4, PixelFormat::RGB)));

    const auto* bytes2 = static_cast<const uint8*> (outStream2->getData());
    EXPECT_EQ (bytes2[2], 2); // type 2 (uncompressed)
}

// ----------------------------------------------------------------------
// 16-bit palette (colorMapEntrySize == 16)
// ----------------------------------------------------------------------

TEST (TgaImageFormatTests, DecodeColorMappedWith16BitPalette)
{
    // Type 1, 2×2, 8-bit depth, 4-color 16-bit 555 palette
    auto header = makeTgaHeader (1, 2, 2, 8, 0x20, 1, 4, 16);

    // Palette: 4 entries of 16-bit RGB555 (LE)
    // Entry 0: full red   R=31 G=0  B=0  → 0x7C00
    // Entry 1: full green R=0  G=31 B=0  → 0x03E0
    // Entry 2: full blue  R=0  G=0  B=31 → 0x001F
    // Entry 3: white      R=31 G=31 B=31 → 0x7FFF
    const uint16 palette555[4] = { 0x7C00, 0x03E0, 0x001F, 0x7FFF };
    std::vector<uint8> paletteBytes;
    for (auto v : palette555)
    {
        paletteBytes.push_back (static_cast<uint8> (v & 0xFF));
        paletteBytes.push_back (static_cast<uint8> (v >> 8));
    }

    // Pixel indices: row0 = [0,1], row1 = [2,3]
    std::vector<uint8> pixels = { 0, 1, 2, 3 };

    // Assemble: header + palette + pixels
    std::vector<uint8> fullData (header);
    fullData.insert (fullData.end(), paletteBytes.begin(), paletteBytes.end());
    fullData.insert (fullData.end(), pixels.begin(), pixels.end());

    MemoryInputStream stream (fullData.data(), fullData.size(), false);
    auto image = decodeRawTga (fullData);

    ASSERT_TRUE (image.isValid());
    EXPECT_EQ (image.getWidth(), 2);
    EXPECT_EQ (image.getHeight(), 2);

    // Index 0 → red at (0,0)
    const uint32 p0 = image.getPixel (0, 0);
    EXPECT_GE ((p0 >> 16) & 0xFF, 245u);
    EXPECT_LE ((p0 >> 8) & 0xFF, 10u);
    EXPECT_LE (p0 & 0xFF, 10u);

    // Index 1 → green at (1,0)
    const uint32 p1 = image.getPixel (1, 0);
    EXPECT_LE ((p1 >> 16) & 0xFF, 10u);
    EXPECT_GE ((p1 >> 8) & 0xFF, 245u);
    EXPECT_LE (p1 & 0xFF, 10u);

    // Index 2 → blue at (0,1)
    const uint32 p2 = image.getPixel (0, 1);
    EXPECT_LE ((p2 >> 16) & 0xFF, 10u);
    EXPECT_LE ((p2 >> 8) & 0xFF, 10u);
    EXPECT_GE (p2 & 0xFF, 245u);

    // Index 3 → white at (1,1)
    const uint32 p3 = image.getPixel (1, 1);
    EXPECT_GE ((p3 >> 16) & 0xFF, 245u);
    EXPECT_GE ((p3 >> 8) & 0xFF, 245u);
    EXPECT_GE (p3 & 0xFF, 245u);
}

// ----------------------------------------------------------------------
// Palette index out-of-bounds fallback
// ----------------------------------------------------------------------

TEST (TgaImageFormatTests, PaletteIndexOutOfBoundsReturnsOpaqueBlack)
{
    // Type 1, 2×1, 8-bit depth, 2-color palette, but pixel index is 5 (OOB)
    auto header = makeTgaHeader (1, 2, 1, 8, 0x20, 1, 2, 24);

    // 24-bit palette: entry 0 = red, entry 1 = blue
    std::vector<uint8> paletteBytes = {
        0x00, 0x00, 0xFF, // B=0, G=0, R=255 → red
        0xFF,
        0x00,
        0x00 // B=255, G=0, R=0 → blue
    };

    // Pixel 0 = index 5 (OOB), pixel 1 = index 0 (valid)
    std::vector<uint8> pixels = { 5, 0 };

    std::vector<uint8> fullData (header);
    fullData.insert (fullData.end(), paletteBytes.begin(), paletteBytes.end());
    fullData.insert (fullData.end(), pixels.begin(), pixels.end());

    MemoryInputStream stream (fullData.data(), fullData.size(), false);
    auto image = decodeRawTga (fullData);

    ASSERT_TRUE (image.isValid());

    // Index 5 should fall back to 0xFF000000 (opaque black)
    EXPECT_EQ (image.getPixel (0, 0), 0xFF000000u);

    // Index 0 should be red
    EXPECT_EQ (image.getPixel (1, 0) & 0x00FFFFFFu, 0x00FF0000u);
}

// ----------------------------------------------------------------------
// RLE color-mapped decode (type 9)
// ----------------------------------------------------------------------

TEST (TgaImageFormatTests, DecodeRleColorMappedType9)
{
    // Type 9, 4×2, 8-bit depth, 3-color 24-bit palette, RLE encoded
    auto header = makeTgaHeader (9, 4, 2, 8, 0x20, 1, 3, 24);

    // 24-bit palette: 0=red, 1=green, 2=blue
    std::vector<uint8> paletteBytes = {
        0x00, 0x00, 0xFF, // red
        0x00,
        0xFF,
        0x00, // green
        0xFF,
        0x00,
        0x00 // blue
    };

    // RLE data: Row 0 = [run 4 of index 0], Row 1 = [run 2 of index 1, run 2 of index 2]
    std::vector<uint8> rleData = {
        0x80 | 3, 0x00, // run of 4 × palette[0] = red
        0x80 | 1,
        0x01, // run of 2 × palette[1] = green
        0x80 | 1,
        0x02 // run of 2 × palette[2] = blue
    };

    std::vector<uint8> fullData (header);
    fullData.insert (fullData.end(), paletteBytes.begin(), paletteBytes.end());
    fullData.insert (fullData.end(), rleData.begin(), rleData.end());

    MemoryInputStream stream (fullData.data(), fullData.size(), false);
    auto image = decodeRawTga (fullData);

    ASSERT_TRUE (image.isValid());
    EXPECT_EQ (image.getWidth(), 4);
    EXPECT_EQ (image.getHeight(), 2);

    // Row 0: all red
    for (int x = 0; x < 4; ++x)
        EXPECT_EQ (image.getPixel (x, 0) & 0x00FFFFFFu, 0x00FF0000u);

    // Row 1: first 2 green, last 2 blue
    EXPECT_EQ (image.getPixel (0, 1) & 0x00FFFFFFu, 0x0000FF00u);
    EXPECT_EQ (image.getPixel (1, 1) & 0x00FFFFFFu, 0x0000FF00u);
    EXPECT_EQ (image.getPixel (2, 1) & 0x00FFFFFFu, 0x000000FFu);
    EXPECT_EQ (image.getPixel (3, 1) & 0x00FFFFFFu, 0x000000FFu);
}

// ----------------------------------------------------------------------
// Grayscale TGA (type 3 and type 11) read tests
// ----------------------------------------------------------------------

TEST (TgaImageFormatTests, DecodeType3UncompressedGrayscale)
{
    // Type 3, 2×2, 8-bit grayscale
    auto header = makeTgaHeader (3, 2, 2, 8, 0x20);

    // Pixels: row0 = [10, 200], row1 = [50, 100]
    std::vector<uint8> pixels = { 10, 200, 50, 100 };

    auto stream = makeTgaData (header, pixels);
    auto image = decodeRawTga (stream);

    ASSERT_TRUE (image.isValid());
    EXPECT_EQ (image.getWidth(), 2);
    EXPECT_EQ (image.getHeight(), 2);

    EXPECT_EQ (image.getPixel (0, 0), 0xFF0A0A0Au);
    EXPECT_EQ (image.getPixel (1, 0), 0xFFC8C8C8u);
    EXPECT_EQ (image.getPixel (0, 1), 0xFF323232u);
    EXPECT_EQ (image.getPixel (1, 1), 0xFF646464u);
}

TEST (TgaImageFormatTests, DecodeType11RleGrayscale)
{
    // Type 11, 4×2, 8-bit RLE grayscale
    auto header = makeTgaHeader (11, 4, 2, 8, 0x20);

    // RLE: Row 0 = run(4×128), Row 1 = run(2×64) + run(2×192)
    std::vector<uint8> rleData = {
        0x80 | 3, 128, // run of 4 × gray 128
        0x80 | 1,
        64, // run of 2 × gray 64
        0x80 | 1,
        192 // run of 2 × gray 192
    };

    std::vector<uint8> fullData (header);
    fullData.insert (fullData.end(), rleData.begin(), rleData.end());

    MemoryInputStream stream (fullData.data(), fullData.size(), false);
    auto image = decodeRawTga (fullData);

    ASSERT_TRUE (image.isValid());
    EXPECT_EQ (image.getWidth(), 4);
    EXPECT_EQ (image.getHeight(), 2);

    // Row 0: all gray 128
    for (int x = 0; x < 4; ++x)
        EXPECT_EQ (image.getPixel (x, 0), 0xFF808080u);

    // Row 1: first 2 gray 64, last 2 gray 192
    EXPECT_EQ (image.getPixel (0, 1), 0xFF404040u);
    EXPECT_EQ (image.getPixel (1, 1), 0xFF404040u);
    EXPECT_EQ (image.getPixel (2, 1), 0xFFC0C0C0u);
    EXPECT_EQ (image.getPixel (3, 1), 0xFFC0C0C0u);
}

// ----------------------------------------------------------------------
// 16-bit true-color TGA read test
// ----------------------------------------------------------------------

TEST (TgaImageFormatTests, DecodeType2Uncompressed16BitRgb555)
{
    // Type 2, 2×1, 16-bit RGB555
    auto header = makeTgaHeader (2, 2, 1, 16, 0x20);

    // Pixel 0: R=31, G=31, B=31 (white) → LE: 0xFF, 0x7F
    // Pixel 1: R=31, G=0, B=0 (red) → LE: 0x00, 0x7C
    std::vector<uint8> pixels = { 0xFF, 0x7F, 0x00, 0x7C };

    auto stream = makeTgaData (header, pixels);
    auto image = decodeRawTga (stream);

    ASSERT_TRUE (image.isValid());
    EXPECT_EQ (image.getWidth(), 2);
    EXPECT_EQ (image.getHeight(), 1);

    // White (R=31→248, G=31→248, B=31→248)
    const uint32 p0 = image.getPixel (0, 0);
    EXPECT_GE ((p0 >> 16) & 0xFF, 245u);
    EXPECT_GE ((p0 >> 8) & 0xFF, 245u);
    EXPECT_GE (p0 & 0xFF, 245u);

    // Red (R=31→248, G=0→0, B=0→0)
    const uint32 p1 = image.getPixel (1, 0);
    EXPECT_GE ((p1 >> 16) & 0xFF, 245u);
    EXPECT_LE ((p1 >> 8) & 0xFF, 10u);
    EXPECT_LE (p1 & 0xFF, 10u);
}
