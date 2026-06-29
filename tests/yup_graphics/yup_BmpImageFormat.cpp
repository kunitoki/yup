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

TEST (BmpImageFormatTests, ReaderSetsCorrectDimensionsFromHeader)
{
    auto* rawStream = new MemoryOutputStream();
    BmpImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (4, 8, PixelFormat::RGB)));

    const void* data = rawStream->getData();
    size_t size = rawStream->getDataSize();

    auto* inStream = new MemoryInputStream (data, size, true);
    BmpImageFormatReader reader (inStream);

    EXPECT_EQ (reader.width, 4);
    EXPECT_EQ (reader.height, 8);
    EXPECT_EQ (reader.pixelFormat, PixelFormat::RGB);
}

TEST (BmpImageFormatTests, InvalidSignatureReturnsInvalidImage)
{
    const uint8 garbage[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
    auto* stream = new MemoryInputStream (garbage, sizeof (garbage), false);

    BmpImageFormatReader reader (stream);

    EXPECT_EQ (reader.width, 0);
    EXPECT_EQ (reader.height, 0);

    auto result = reader.readImage();
    EXPECT_FALSE (result.isValid());
}

TEST (BmpImageFormatTests, ReaderSetsDimensionsForSmallImage)
{
    auto* rawStream = new MemoryOutputStream();
    BmpImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (1, 1, PixelFormat::RGBA)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    BmpImageFormatReader reader (inStream);

    EXPECT_EQ (reader.width, 1);
    EXPECT_EQ (reader.height, 1);
    EXPECT_EQ (reader.pixelFormat, PixelFormat::RGBA);
}

TEST (BmpImageFormatTests, ReaderSetsDimensionsForNonSquareImage)
{
    auto* rawStream = new MemoryOutputStream();
    BmpImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (13, 7, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    BmpImageFormatReader reader (inStream);

    EXPECT_EQ (reader.width, 13);
    EXPECT_EQ (reader.height, 7);
}

// ======================================================================
// Roundtrip tests (memory-based)
// ======================================================================

TEST (BmpImageFormatTests, WriteAndReadBackRgbProducesPixelIdenticalImage)
{
    auto original = generateTestImage (16, 16, PixelFormat::RGB);

    auto* rawStream = new MemoryOutputStream();
    BmpImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    BmpImageFormatReader reader (inStream);
    auto result = reader.readImage();

    EXPECT_EQ (result.getWidth(), original.getWidth());
    EXPECT_EQ (result.getHeight(), original.getHeight());
    EXPECT_EQ (result.getPixelFormat(), original.getPixelFormat());
    EXPECT_TRUE (imagesAreEqual (original, result, 0));
}

TEST (BmpImageFormatTests, WriteAndReadBackRgbaProducesPixelIdenticalImage)
{
    auto original = generateTestImage (16, 16, PixelFormat::RGBA);

    auto* rawStream = new MemoryOutputStream();
    BmpImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    BmpImageFormatReader reader (inStream);
    auto result = reader.readImage();

    ASSERT_EQ (result.getWidth(), original.getWidth());
    ASSERT_EQ (result.getHeight(), original.getHeight());
    EXPECT_EQ (result.getPixelFormat(), original.getPixelFormat());

    for (int y = 0; y < original.getHeight(); ++y)
    {
        for (int x = 0; x < original.getWidth(); ++x)
        {
            EXPECT_EQ (result.getPixel (x, y), original.getPixel (x, y))
                << "Pixel mismatch at (" << x << ", " << y << ")";
        }
    }
}

TEST (BmpImageFormatTests, SolidColorRgbRoundtripIsExact)
{
    auto original = generateSolidImage (12, 8, PixelFormat::RGB, 0xFF3355AAu);

    auto* rawStream = new MemoryOutputStream();
    BmpImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    BmpImageFormatReader reader (inStream);
    auto result = reader.readImage();

    EXPECT_TRUE (imagesAreEqual (original, result, 0));
}

TEST (BmpImageFormatTests, SolidColorRgbaRoundtripIsExact)
{
    auto original = generateSolidImage (10, 6, PixelFormat::RGBA, 0xAA224466u);

    auto* rawStream = new MemoryOutputStream();
    BmpImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    ASSERT_TRUE (writer.writeImage (original));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    BmpImageFormatReader reader (inStream);
    auto result = reader.readImage();

    for (int y = 0; y < original.getHeight(); ++y)
    {
        for (int x = 0; x < original.getWidth(); ++x)
            EXPECT_EQ (result.getPixel (x, y), original.getPixel (x, y));
    }
}

TEST (BmpImageFormatTests, VariousSizesRoundtripCorrectly)
{
    const int sizes[][2] = { { 1, 1 }, { 1, 32 }, { 32, 1 }, { 7, 13 }, { 64, 48 } };

    for (auto [w, h] : sizes)
    {
        auto original = generateTestImage (w, h, PixelFormat::RGB);

        auto* rawStream = new MemoryOutputStream();
        BmpImageFormatWriter writer (rawStream, PixelFormat::RGB);
        ASSERT_TRUE (writer.writeImage (original));

        auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
        BmpImageFormatReader reader (inStream);
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

TEST (BmpImageFormatTests, FileRoundtripRgbViaTempFile)
{
    auto original = generateTestImage (16, 16, PixelFormat::RGB);
    auto tempFile = File::createTempFile (".bmp");

    {
        auto* fos = tempFile.createOutputStream().release();
        BmpImageFormatWriter writer (fos, PixelFormat::RGB);
        ASSERT_TRUE (writer.writeImage (original));
    }

    {
        auto* fis = tempFile.createInputStream().release();
        BmpImageFormatReader reader (fis);
        auto result = reader.readImage();

        ASSERT_TRUE (result.isValid());
        EXPECT_EQ (result.getWidth(), original.getWidth());
        EXPECT_EQ (result.getHeight(), original.getHeight());
        EXPECT_TRUE (imagesAreEqual (original, result, 0));
    }

    tempFile.deleteFile();
}

TEST (BmpImageFormatTests, FileRoundtripRgbaViaTempFile)
{
    auto original = generateTestImage (8, 12, PixelFormat::RGBA);
    auto tempFile = File::createTempFile (".bmp");

    {
        auto* fos = tempFile.createOutputStream().release();
        BmpImageFormatWriter writer (fos, PixelFormat::RGBA);
        ASSERT_TRUE (writer.writeImage (original));
    }

    {
        auto* fis = tempFile.createInputStream().release();
        BmpImageFormatReader reader (fis);
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

TEST (BmpImageFormatTests, FileRoundtripSolidColorViaTempFile)
{
    auto original = generateSolidImage (20, 10, PixelFormat::RGB, 0xFF77AA33u);
    auto tempFile = File::createTempFile (".bmp");

    {
        auto* fos = tempFile.createOutputStream().release();
        BmpImageFormatWriter writer (fos, PixelFormat::RGB);
        ASSERT_TRUE (writer.writeImage (original));
    }

    {
        auto* fis = tempFile.createInputStream().release();
        BmpImageFormatReader reader (fis);
        auto result = reader.readImage();
        ASSERT_TRUE (imagesAreEqual (original, result, 0));
    }

    tempFile.deleteFile();
}

TEST (BmpImageFormatTests, SavedFileHasExpectedMinSize)
{
    auto original = generateSolidImage (4, 4, PixelFormat::RGB);
    auto tempFile = File::createTempFile (".bmp");

    {
        auto* fos = tempFile.createOutputStream().release();
        BmpImageFormatWriter writer (fos, PixelFormat::RGB);
        ASSERT_TRUE (writer.writeImage (original));
    }

    EXPECT_GT (tempFile.getSize(), 0);
    tempFile.deleteFile();
}

// ======================================================================
// Image::loadFromData tests
// ======================================================================

TEST (BmpImageFormatTests, LoadFromDataRoundtripRgb)
{
    auto original = generateTestImage (12, 12, PixelFormat::RGB);

    auto* rawStream = new MemoryOutputStream();
    BmpImageFormatWriter writer (rawStream, PixelFormat::RGB);
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

TEST (BmpImageFormatTests, LoadFromDataRoundtripRgba)
{
    auto original = generateTestImage (8, 8, PixelFormat::RGBA);

    auto* rawStream = new MemoryOutputStream();
    BmpImageFormatWriter writer (rawStream, PixelFormat::RGBA);
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

TEST (BmpImageFormatTests, FormatNameIsCorrect)
{
    BmpImageFormat fmt;
    EXPECT_EQ (fmt.getFormatName(), String ("BMP Image"));
}

TEST (BmpImageFormatTests, ExtensionsAreCorrect)
{
    BmpImageFormat fmt;

    auto readExts = fmt.getFileExtensions (ImageFormat::forReading);
    ASSERT_EQ (readExts.size(), 1);
    EXPECT_EQ (readExts[0], String (".bmp"));

    auto writeExts = fmt.getFileExtensions (ImageFormat::forWriting);
    ASSERT_EQ (writeExts.size(), 1);
    EXPECT_EQ (writeExts[0], String (".bmp"));
}

TEST (BmpImageFormatTests, PossiblePixelFormatsIncludeRgbAndRgba)
{
    BmpImageFormat fmt;
    auto formats = fmt.getPossiblePixelFormats();

    EXPECT_EQ (formats.size(), 2);
    EXPECT_TRUE (formats.contains (PixelFormat::RGB));
    EXPECT_TRUE (formats.contains (PixelFormat::RGBA));
}

TEST (BmpImageFormatTests, IsNotCompressed)
{
    BmpImageFormat fmt;
    EXPECT_FALSE (fmt.isCompressed());
}

TEST (BmpImageFormatTests, HasNoQualityOptions)
{
    BmpImageFormat fmt;
    EXPECT_EQ (fmt.getQualityOptions().size(), 0);
}

// ======================================================================
// canHandleFile tests
// ======================================================================

TEST (BmpImageFormatTests, CanHandleFileForBmpExtension)
{
    BmpImageFormat fmt;

    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/image.bmp"), ImageFormat::forReading));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/image.BMP"), ImageFormat::forReading));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/image.bmp"), ImageFormat::forWriting));
}

TEST (BmpImageFormatTests, CanHandleFileRejectsWrongExtensions)
{
    BmpImageFormat fmt;

    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image.png"), ImageFormat::forReading));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image.jpg"), ImageFormat::forReading));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image.ppm"), ImageFormat::forReading));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image.webp"), ImageFormat::forReading));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image"), ImageFormat::forReading));
}

// ======================================================================
// canHandleStream tests
// ======================================================================

TEST (BmpImageFormatTests, CanHandleStreamDetectsBmpMagic)
{
    BmpImageFormat fmt;

    const uint8 bmpHeader[] = { 0x42, 0x4D, 0x00, 0x00 };
    MemoryInputStream stream (bmpHeader, std::size (bmpHeader), false);

    EXPECT_TRUE (fmt.canHandleStream (stream, ImageFormat::forReading));
    EXPECT_EQ (stream.getPosition(), 0);
}

TEST (BmpImageFormatTests, CanHandleStreamRejectsPngMagic)
{
    BmpImageFormat fmt;

    const uint8 pngHeader[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
    MemoryInputStream stream (pngHeader, std::size (pngHeader), false);

    EXPECT_FALSE (fmt.canHandleStream (stream, ImageFormat::forReading));
    EXPECT_EQ (stream.getPosition(), 0);
}

TEST (BmpImageFormatTests, CanHandleStreamRejectsEmptyStream)
{
    BmpImageFormat fmt;

    MemoryInputStream stream (static_cast<const void*> (nullptr), 0, false);
    EXPECT_FALSE (fmt.canHandleStream (stream, ImageFormat::forReading));
}

// ======================================================================
// Manager integration tests
// ======================================================================

TEST (BmpImageFormatTests, ManagerCreatesReaderForStream)
{
    auto* rawStream = new MemoryOutputStream();
    BmpImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (generateTestImage (4, 4, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto reader = manager.createReaderFor (inStream);
    ASSERT_NE (reader, nullptr);
    EXPECT_EQ (reader->getFormatName(), String ("BMP Image"));
    EXPECT_EQ (reader->width, 4);
    EXPECT_EQ (reader->height, 4);
}

TEST (BmpImageFormatTests, ManagerCreatesWriterForBmpExtension)
{
    auto tempFile = File::createTempFile (".bmp");

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto writer = manager.createWriterFor (tempFile, PixelFormat::RGB);
    ASSERT_NE (writer, nullptr);
    EXPECT_EQ (writer->getFormatName(), String ("BMP Image"));
    EXPECT_EQ (writer->getPixelFormat(), PixelFormat::RGB);

    tempFile.deleteFile();
}

TEST (BmpImageFormatTests, ManagerRoundtripViaFile)
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

TEST (BmpImageFormatTests, ReaderDpiDefaultsToZero)
{
    auto* rawStream = new MemoryOutputStream();
    BmpImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (generateSolidImage (4, 4, PixelFormat::RGB)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);
    BmpImageFormatReader reader (inStream);

    EXPECT_NEAR (reader.dpiX, 72.009, 0.001);
    EXPECT_NEAR (reader.dpiY, 72.009, 0.001);
}

TEST (BmpImageFormatTests, WriterFlushReturnsTrue)
{
    auto* rawStream = new MemoryOutputStream();
    BmpImageFormatWriter writer (rawStream, PixelFormat::RGB);
    EXPECT_TRUE (writer.flush());
}

TEST (BmpImageFormatTests, WriterReturnsCorrectPixelFormat)
{
    {
        auto* rawStream = new MemoryOutputStream();
        BmpImageFormatWriter writer (rawStream, PixelFormat::RGB);
        EXPECT_EQ (writer.getPixelFormat(), PixelFormat::RGB);
    }
    {
        auto* rawStream = new MemoryOutputStream();
        BmpImageFormatWriter writer (rawStream, PixelFormat::RGBA);
        EXPECT_EQ (writer.getPixelFormat(), PixelFormat::RGBA);
    }
}

TEST (BmpImageFormatTests, WriterReturnsCorrectFormatName)
{
    auto* rawStream = new MemoryOutputStream();
    BmpImageFormatWriter writer (rawStream, PixelFormat::RGB);
    EXPECT_EQ (writer.getFormatName(), String ("BMP Image"));
}

// ======================================================================
// Helpers for crafting raw BMP byte sequences
// ======================================================================

namespace
{

static void writeLE16Raw (std::vector<uint8>& buf, uint16 v)
{
    buf.push_back (static_cast<uint8> (v));
    buf.push_back (static_cast<uint8> (v >> 8));
}

static void writeLE32Raw (std::vector<uint8>& buf, uint32 v)
{
    buf.push_back (static_cast<uint8> (v));
    buf.push_back (static_cast<uint8> (v >> 8));
    buf.push_back (static_cast<uint8> (v >> 16));
    buf.push_back (static_cast<uint8> (v >> 24));
}

static void writePaletteEntry (std::vector<uint8>& buf, uint8 r, uint8 g, uint8 b)
{
    buf.push_back (b);
    buf.push_back (g);
    buf.push_back (r);
    buf.push_back (0); // reserved
}

static std::vector<uint8> makeBmpHeader (int w, int h, uint16 bitCount, uint32 compression, uint32 paletteBytes, uint32 pixelDataSize)
{
    const uint32 pixelDataOffset = 14u + 40u + paletteBytes;
    const uint32 fileSize = pixelDataOffset + pixelDataSize;

    std::vector<uint8> buf;
    buf.reserve (pixelDataOffset);

    // BITMAPFILEHEADER (14 bytes)
    buf.push_back ('B');
    buf.push_back ('M');
    writeLE32Raw (buf, fileSize);
    writeLE32Raw (buf, 0); // reserved
    writeLE32Raw (buf, pixelDataOffset);

    // BITMAPINFOHEADER (40 bytes)
    writeLE32Raw (buf, 40); // header size
    writeLE32Raw (buf, static_cast<uint32> (w));
    writeLE32Raw (buf, static_cast<uint32> (h)); // positive = bottom-up
    writeLE16Raw (buf, 1);                       // planes
    writeLE16Raw (buf, bitCount);
    writeLE32Raw (buf, compression);
    writeLE32Raw (buf, pixelDataSize);    // imageSize
    writeLE32Raw (buf, 2835);             // xPelsPerMeter (~72 DPI)
    writeLE32Raw (buf, 2835);             // yPelsPerMeter
    writeLE32Raw (buf, paletteBytes / 4); // colors used
    writeLE32Raw (buf, 0);                // colors important

    return buf;
}

} // namespace

// ======================================================================
// 16-bit RGB555 reader tests
// ======================================================================

TEST (BmpImageFormatTests, Reader16BitRgb555DecodesRedAndBluePixels)
{
    // 2x1 16-bit RGB555 BMP:
    // pixel 0: pure red  → R=31, G=0, B=0 → 0x7C00
    // pixel 1: pure blue → R=0, G=0, B=31 → 0x001F
    // stride for 2 pixels * 2 bytes = 4 bytes (already aligned)

    constexpr int w = 2, h = 1;
    const uint32 pixelDataSize = 4; // 2 pixels * 2 bytes, no padding needed

    auto buf = makeBmpHeader (w, h, 16, 0, 0, pixelDataSize);

    // pixel 0: 0x7C00 → bytes 0x00, 0x7C
    buf.push_back (0x00);
    buf.push_back (0x7C);
    // pixel 1: 0x001F → bytes 0x1F, 0x00
    buf.push_back (0x1F);
    buf.push_back (0x00);

    auto* inStream = new MemoryInputStream (buf.data(), buf.size(), false);
    BmpImageFormatReader reader (inStream);

    ASSERT_EQ (reader.width, w);
    ASSERT_EQ (reader.height, h);

    const Image result = reader.readImage();
    ASSERT_TRUE (result.isValid());

    // Red pixel: R=255, G=0, B=0 → ARGB = 0xFF FF0000
    const uint32 px0 = result.getPixel (0, 0);
    EXPECT_EQ ((px0 >> 24) & 0xFF, 0xFFu); // alpha
    EXPECT_EQ ((px0 >> 16) & 0xFF, 0xFFu); // R=255
    EXPECT_EQ ((px0 >> 8) & 0xFF, 0x00u);  // G=0
    EXPECT_EQ (px0 & 0xFF, 0x00u);         // B=0

    // Blue pixel: R=0, G=0, B=255
    const uint32 px1 = result.getPixel (1, 0);
    EXPECT_EQ ((px1 >> 24) & 0xFF, 0xFFu);
    EXPECT_EQ ((px1 >> 16) & 0xFF, 0x00u);
    EXPECT_EQ ((px1 >> 8) & 0xFF, 0x00u);
    EXPECT_EQ (px1 & 0xFF, 0xFFu);
}

// ======================================================================
// 8-bit indexed (palette) reader tests
// ======================================================================

TEST (BmpImageFormatTests, Reader8BitIndexedPaletteProducesCorrectPixels)
{
    // 2x2 8-bit indexed BMP with 2-entry palette: [black, white]
    // colorUsed = 2  → palette = 2 * 4 = 8 bytes
    // stride for 2 pixels @ 1 byte each = 2, padded to 4 → 4 bytes per row
    // Pixel data (bottom-up): row0=[1,0,0,0], row1=[0,1,0,0]
    // After decode (bottom-up):
    //   image(0,0)=white, image(1,0)=black
    //   image(0,1)=black, image(1,1)=white

    constexpr int w = 2, h = 2;
    const uint32 paletteBytes = 2 * 4;
    const uint32 pixelDataSize = 4 * 2; // 4-byte stride * 2 rows

    auto buf = makeBmpHeader (w, h, 8, 0, paletteBytes, pixelDataSize);

    // Palette: index0=black, index1=white
    writePaletteEntry (buf, 0, 0, 0);
    writePaletteEntry (buf, 255, 255, 255);

    // Pixel data (bottom-up): file row 0 → image row 1
    buf.push_back (1);
    buf.push_back (0);
    buf.push_back (0);
    buf.push_back (0); // file row 0
    buf.push_back (0);
    buf.push_back (1);
    buf.push_back (0);
    buf.push_back (0); // file row 1

    auto* inStream = new MemoryInputStream (buf.data(), buf.size(), false);
    BmpImageFormatReader reader (inStream);

    ASSERT_EQ (reader.width, w);
    ASSERT_EQ (reader.height, h);

    const Image result = reader.readImage();
    ASSERT_TRUE (result.isValid());

    // file row 0 → image row 1: white, black
    EXPECT_EQ (result.getPixel (0, 1), 0xFFFFFFFFu); // white
    EXPECT_EQ (result.getPixel (1, 1), 0xFF000000u); // black

    // file row 1 → image row 0: black, white
    EXPECT_EQ (result.getPixel (0, 0), 0xFF000000u); // black
    EXPECT_EQ (result.getPixel (1, 0), 0xFFFFFFFFu); // white
}

// ======================================================================
// 4-bit indexed (palette) reader tests
// ======================================================================

TEST (BmpImageFormatTests, Reader4BitIndexedPaletteProducesCorrectPixels)
{
    // 4x1 4-bit indexed BMP with 2-entry palette: [black, white]
    // stride = ((4*4+31)/32)*4 = 4 bytes
    // Each row byte: high nibble = left pixel, low nibble = right pixel
    // Pixel layout bytes: byte0=0x01 (px0=0=black, px1=1=white),
    //                     byte1=0x10 (px2=1=white, px3=0=black)
    // + 2 bytes padding to reach 4-byte stride

    constexpr int w = 4, h = 1;
    const uint32 paletteBytes = 2 * 4;
    const uint32 pixelDataSize = 4; // 4-byte stride * 1 row

    auto buf = makeBmpHeader (w, h, 4, 0, paletteBytes, pixelDataSize);

    writePaletteEntry (buf, 0, 0, 0);       // index 0 = black
    writePaletteEntry (buf, 255, 255, 255); // index 1 = white

    buf.push_back (0x01); // px0=(0x01>>4)=0=black, px1=0x01&0xF=1=white
    buf.push_back (0x10); // px2=(0x10>>4)=1=white, px3=0x10&0xF=0=black
    buf.push_back (0);    // padding
    buf.push_back (0);    // padding

    auto* inStream = new MemoryInputStream (buf.data(), buf.size(), false);
    BmpImageFormatReader reader (inStream);

    ASSERT_EQ (reader.width, w);
    ASSERT_EQ (reader.height, h);

    const Image result = reader.readImage();
    ASSERT_TRUE (result.isValid());

    EXPECT_EQ (result.getPixel (0, 0), 0xFF000000u); // black
    EXPECT_EQ (result.getPixel (1, 0), 0xFFFFFFFFu); // white
    EXPECT_EQ (result.getPixel (2, 0), 0xFFFFFFFFu); // white
    EXPECT_EQ (result.getPixel (3, 0), 0xFF000000u); // black
}

// ======================================================================
// 1-bit monochrome reader tests
// ======================================================================

TEST (BmpImageFormatTests, Reader1BitMonochromeProducesCorrectPixels)
{
    // 8x1 1-bit BMP with 2-entry palette: [black, white]
    // stride = ((8*1+31)/32)*4 = 4 bytes
    // Row byte: 0xAA = 0b10101010 → px0=1=white, px1=0=black, ... alternating

    constexpr int w = 8, h = 1;
    const uint32 paletteBytes = 2 * 4;
    const uint32 pixelDataSize = 4; // 4-byte stride * 1 row

    auto buf = makeBmpHeader (w, h, 1, 0, paletteBytes, pixelDataSize);

    writePaletteEntry (buf, 0, 0, 0);       // index 0 = black
    writePaletteEntry (buf, 255, 255, 255); // index 1 = white

    buf.push_back (0xAA); // 10101010 → W,B,W,B,W,B,W,B
    buf.push_back (0);    // padding
    buf.push_back (0);    // padding
    buf.push_back (0);    // padding

    auto* inStream = new MemoryInputStream (buf.data(), buf.size(), false);
    BmpImageFormatReader reader (inStream);

    ASSERT_EQ (reader.width, w);
    ASSERT_EQ (reader.height, h);

    const Image result = reader.readImage();
    ASSERT_TRUE (result.isValid());

    for (int x = 0; x < 8; ++x)
    {
        const uint32 expected = (x % 2 == 0) ? 0xFFFFFFFFu : 0xFF000000u;
        EXPECT_EQ (result.getPixel (x, 0), expected) << "at x=" << x;
    }
}

// ======================================================================
// Top-down BMP (negative height) reader tests
// ======================================================================

TEST (BmpImageFormatTests, ReaderTopDownBmpDecodesRowsInCorrectOrder)
{
    // 2x2 24-bit top-down BMP (imageHeight stored as -2):
    // Row 0 in file → image row 0 (top)  → all red
    // Row 1 in file → image row 1 (bottom) → all blue

    constexpr int w = 2;
    const int32 hNeg = -2;          // negative = top-down
    const uint32 pixelDataSize = 8; // stride = ((2*24+31)/32)*4 = 8 bytes, 2 rows

    // Build header manually with negative height
    std::vector<uint8> buf;
    const uint32 pixelDataOffset = 14u + 40u;
    const uint32 fileSize = pixelDataOffset + pixelDataSize;

    buf.push_back ('B');
    buf.push_back ('M');
    writeLE32Raw (buf, fileSize);
    writeLE32Raw (buf, 0);
    writeLE32Raw (buf, pixelDataOffset);

    writeLE32Raw (buf, 40); // header size
    writeLE32Raw (buf, static_cast<uint32> (w));
    writeLE32Raw (buf, static_cast<uint32> (hNeg)); // negative → top-down
    writeLE16Raw (buf, 1);                          // planes
    writeLE16Raw (buf, 24);                         // bitCount
    writeLE32Raw (buf, 0);                          // compression=BI_RGB
    writeLE32Raw (buf, pixelDataSize);
    writeLE32Raw (buf, 2835);
    writeLE32Raw (buf, 2835);
    writeLE32Raw (buf, 0);
    writeLE32Raw (buf, 0);

    // Row 0 (top of image): 2 red pixels = B=0,G=0,R=255 each (BGR order)
    buf.push_back (0);
    buf.push_back (0);
    buf.push_back (255); // px0 red
    buf.push_back (0);
    buf.push_back (0);
    buf.push_back (255); // px1 red
    // padding to align stride to 4 bytes (2*3=6, pad 2)
    buf.push_back (0);
    buf.push_back (0);

    // Row 1 (bottom of image): 2 blue pixels = B=255,G=0,R=0
    buf.push_back (255);
    buf.push_back (0);
    buf.push_back (0); // px0 blue
    buf.push_back (255);
    buf.push_back (0);
    buf.push_back (0); // px1 blue
    buf.push_back (0);
    buf.push_back (0);

    auto* inStream = new MemoryInputStream (buf.data(), buf.size(), false);
    BmpImageFormatReader reader (inStream);

    ASSERT_EQ (reader.width, 2);
    ASSERT_EQ (reader.height, 2);

    const Image result = reader.readImage();
    ASSERT_TRUE (result.isValid());

    // Row 0 = red
    EXPECT_EQ (result.getPixel (0, 0), 0xFFFF0000u);
    EXPECT_EQ (result.getPixel (1, 0), 0xFFFF0000u);

    // Row 1 = blue
    EXPECT_EQ (result.getPixel (0, 1), 0xFF0000FFu);
    EXPECT_EQ (result.getPixel (1, 1), 0xFF0000FFu);
}

// ======================================================================
// RLE8-compressed BMP reader tests
// ======================================================================

TEST (BmpImageFormatTests, ReaderRle8DecodesRunLengthEncodedPixels)
{
    // 4x2 8-bit RLE8 BMP with 2-entry palette: [black, white]
    // Stream rows (y increments per EOL):
    //   stream row 0: run(4 × index 1 = white) + EOL = [0x04, 0x01, 0x00, 0x00]
    //   stream row 1: run(4 × index 0 = black) + EOB = [0x04, 0x00, 0x00, 0x01]
    // Bottom-up mapping: stream row 0 → image row 1, stream row 1 → image row 0

    constexpr int w = 4, h = 2;
    const uint32 paletteBytes = 2 * 4;
    const uint32 pixelDataSize = 8; // 4 bytes × 2 stream rows

    auto buf = makeBmpHeader (w, h, 8, 1 /*BI_RLE8*/, paletteBytes, pixelDataSize);

    writePaletteEntry (buf, 0, 0, 0);       // index 0 = black
    writePaletteEntry (buf, 255, 255, 255); // index 1 = white

    // Stream row 0: 4 white pixels + EOL
    buf.push_back (4);
    buf.push_back (1);
    buf.push_back (0);
    buf.push_back (0); // EOL

    // Stream row 1: 4 black pixels + EOB
    buf.push_back (4);
    buf.push_back (0);
    buf.push_back (0);
    buf.push_back (1); // EOB

    auto* inStream = new MemoryInputStream (buf.data(), buf.size(), false);
    BmpImageFormatReader reader (inStream);

    ASSERT_EQ (reader.width, w);
    ASSERT_EQ (reader.height, h);

    const Image result = reader.readImage();
    ASSERT_TRUE (result.isValid());

    // image row 0 = stream row 1 = black (bottom-up)
    for (int x = 0; x < 4; ++x)
        EXPECT_EQ (result.getPixel (x, 0), 0xFF000000u) << "at x=" << x;

    // image row 1 = stream row 0 = white
    for (int x = 0; x < 4; ++x)
        EXPECT_EQ (result.getPixel (x, 1), 0xFFFFFFFFu) << "at x=" << x;
}

// ======================================================================
// RLE4-compressed BMP reader tests
// ======================================================================

TEST (BmpImageFormatTests, ReaderRle4DecodesRunLengthEncodedPixels)
{
    // 4x2 4-bit RLE4 BMP with 2-entry palette: [black, white]
    // RLE4 run: count nibbles alternate from high/low nibbles of value byte
    //   count=4, value=0x11 → all white (nibble 1 repeated)
    //   count=4, value=0x00 → all black (nibble 0 repeated)
    // Stream rows: row0=all white+EOL, row1=all black+EOB
    // Bottom-up: row0→image row1, row1→image row0

    constexpr int w = 4, h = 2;
    const uint32 paletteBytes = 2 * 4;
    const uint32 pixelDataSize = 8;

    auto buf = makeBmpHeader (w, h, 4, 2 /*BI_RLE4*/, paletteBytes, pixelDataSize);

    writePaletteEntry (buf, 0, 0, 0);       // index 0 = black
    writePaletteEntry (buf, 255, 255, 255); // index 1 = white

    // Stream row 0: 4 white nibbles (value=0x11) + EOL
    buf.push_back (4);
    buf.push_back (0x11);
    buf.push_back (0);
    buf.push_back (0); // EOL

    // Stream row 1: 4 black nibbles (value=0x00) + EOB
    buf.push_back (4);
    buf.push_back (0x00);
    buf.push_back (0);
    buf.push_back (1); // EOB

    auto* inStream = new MemoryInputStream (buf.data(), buf.size(), false);
    BmpImageFormatReader reader (inStream);

    ASSERT_EQ (reader.width, w);
    ASSERT_EQ (reader.height, h);

    const Image result = reader.readImage();
    ASSERT_TRUE (result.isValid());

    // image row 0 = stream row 1 = black
    for (int x = 0; x < 4; ++x)
        EXPECT_EQ (result.getPixel (x, 0), 0xFF000000u) << "at x=" << x;

    // image row 1 = stream row 0 = white
    for (int x = 0; x < 4; ++x)
        EXPECT_EQ (result.getPixel (x, 1), 0xFFFFFFFFu) << "at x=" << x;
}

// ======================================================================
// Extended V4 header reader test
// ======================================================================

TEST (BmpImageFormatTests, ReaderV4HeaderSkipsExtraFieldsAndDecodesPixels)
{
    // 1x1 24-bit BMP using BITMAPV4HEADER (108 bytes), single green pixel.
    // Header size = 108; reader must skip the extra 68 bytes (108 - 40).

    constexpr int w = 1, h = 1;
    const uint32 v4HeaderSize = 108;
    const uint32 pixelDataOffset = 14u + v4HeaderSize;
    const uint32 pixelDataSize = 4; // stride for 1 pixel @ 24-bit = 4 bytes (padded)
    const uint32 fileSize = pixelDataOffset + pixelDataSize;

    std::vector<uint8> buf;

    // BITMAPFILEHEADER
    buf.push_back ('B');
    buf.push_back ('M');
    writeLE32Raw (buf, fileSize);
    writeLE32Raw (buf, 0);
    writeLE32Raw (buf, pixelDataOffset);

    // BITMAPV4HEADER (108 bytes)
    writeLE32Raw (buf, v4HeaderSize);
    writeLE32Raw (buf, static_cast<uint32> (w));
    writeLE32Raw (buf, static_cast<uint32> (h));
    writeLE16Raw (buf, 1);  // planes
    writeLE16Raw (buf, 24); // bitCount
    writeLE32Raw (buf, 0);  // compression
    writeLE32Raw (buf, pixelDataSize);
    writeLE32Raw (buf, 2835);
    writeLE32Raw (buf, 2835);
    writeLE32Raw (buf, 0);
    writeLE32Raw (buf, 0);

    // Extra V4 fields (68 bytes of zeros to pad to 108 total from header start)
    for (int i = 0; i < 68; ++i)
        buf.push_back (0);

    // Single green pixel: B=0, G=255, R=0 (BGR order)
    buf.push_back (0);   // B
    buf.push_back (255); // G
    buf.push_back (0);   // R
    buf.push_back (0);   // padding to 4-byte stride

    auto* inStream = new MemoryInputStream (buf.data(), buf.size(), false);
    BmpImageFormatReader reader (inStream);

    ASSERT_EQ (reader.width, w);
    ASSERT_EQ (reader.height, h);

    const Image result = reader.readImage();
    ASSERT_TRUE (result.isValid());

    EXPECT_EQ (result.getPixel (0, 0), 0xFF00FF00u); // green
}

// ======================================================================
// Invalid image writeImage
// ======================================================================

TEST (BmpImageFormatTests, WriteImageReturnsFalseForInvalidImage)
{
    auto* rawStream = new MemoryOutputStream();
    BmpImageFormatWriter writer (rawStream, PixelFormat::RGB);

    Image invalid;
    EXPECT_FALSE (writer.writeImage (invalid));
}
