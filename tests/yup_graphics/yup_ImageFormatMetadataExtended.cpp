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

#include <memory>

#include "yup_ImageFormatTools.h"

using namespace yup;

namespace
{

// Helper: load image from memory block with options
static Image loadFromBlock (const MemoryBlock& block, const ImageFormat::Options& opts)
{
    auto data = block.asBytes();
    auto result = Image::loadFromData (data, opts);
    if (! result.wasOk())
        return {};
    return std::move (result).getValue();
}

} // namespace

//==============================================================================
// JPEG extended metadata tests
//==============================================================================

#if YUP_MODULE_AVAILABLE_libjpeg && YUP_IMAGE_FORMAT_JPEG

TEST (JpegMetadataExtendedTest, DpiInDotsPerCmSurvivesRoundTrip)
{
    // When DPI is stored in dots/cm in the JPEG, it must be converted to dots/inch.
    // Write a JPEG with DPI that triggers density_unit == 2 (dots/cm).
    Image img = generateSolidImage (16, 16, PixelFormat::RGB, 0xFF336699u);

    auto meta = ImageMetadata::create();
    // 150 DPI = ~59 dots/cm. When written and read back, the reader
    // converts from dots/cm to DPI by multiplying by 2.54.
    meta->dpiX = 150.0;
    meta->dpiY = 150.0;
    img.setMetadata (meta);

    auto block = writeImageToBlock<JpegImageFormatWriter> (img, 0);

    ImageFormat::Options opts = ImageFormat::Options().withMetadata (true);
    auto reloaded = loadFromBlock (block, opts);

    ASSERT_TRUE (reloaded.isValid());
    ASSERT_TRUE (reloaded.hasMetadata());
    // DPI should survive the round-trip (JPEG uses dots/inch internally)
    EXPECT_NEAR (150.0, reloaded.getMetadata()->dpiX, 1.0);
    EXPECT_NEAR (150.0, reloaded.getMetadata()->dpiY, 1.0);
}

TEST (JpegMetadataExtendedTest, XmpRawChunkSurvivesRoundTrip)
{
    Image img = generateSolidImage (16, 16, PixelFormat::RGB, 0xFF336699u);

    // Build a minimal XMP-like blob for the "jpeg/xmp" chunk.
    const char xmpData[] = "http://ns.adobe.com/xap/1.0/\x00<?xpacket begin...";

    auto meta = ImageMetadata::create();
    meta->setRawChunk ("jpeg/xmp", MemoryBlock (xmpData, sizeof (xmpData)));
    img.setMetadata (meta);

    auto block = writeImageToBlock<JpegImageFormatWriter> (img, 0);

    ImageFormat::Options opts = ImageFormat::Options().withRawChunks (true);
    auto reloaded = loadFromBlock (block, opts);

    ASSERT_TRUE (reloaded.isValid());
    ASSERT_TRUE (reloaded.hasMetadata());

    auto* xmpChunk = reloaded.getMetadata()->getRawChunk ("jpeg/xmp");
    ASSERT_NE (nullptr, xmpChunk);
    EXPECT_EQ (sizeof (xmpData), xmpChunk->getSize());
}

TEST (JpegMetadataExtendedTest, JfifRawChunkSurvivesRoundTrip)
{
    Image img = generateSolidImage (16, 16, PixelFormat::RGB, 0xFF336699u);

    // A minimal JFIF APP0 marker body (without the "JFIF\0" header).
    const uint8 jfifBody[] = { 0x01, 0x02, 0x01, 0x00, 0x60, 0x00, 0x60, 0x00, 0x00, 0x00 };

    auto meta = ImageMetadata::create();
    meta->setRawChunk ("jpeg/jfif", MemoryBlock (jfifBody, sizeof (jfifBody)));
    img.setMetadata (meta);

    auto block = writeImageToBlock<JpegImageFormatWriter> (img, 0);

    ImageFormat::Options opts = ImageFormat::Options().withRawChunks (true);
    auto reloaded = loadFromBlock (block, opts);

    ASSERT_TRUE (reloaded.isValid());
    ASSERT_TRUE (reloaded.hasMetadata());

    auto* jfifChunk = reloaded.getMetadata()->getRawChunk ("jpeg/jfif");
    ASSERT_NE (nullptr, jfifChunk);
    EXPECT_EQ (sizeof (jfifBody), jfifChunk->getSize());
}

TEST (JpegMetadataExtendedTest, CommentTextSurvivesRoundTrip)
{
    Image img = generateSolidImage (16, 16, PixelFormat::RGB, 0xFF336699u);

    auto meta = ImageMetadata::create();
    meta->textEntries.set ("Comment", "JPEG COM marker round-trip test.");
    img.setMetadata (meta);

    auto block = writeImageToBlock<JpegImageFormatWriter> (img, 0);

    ImageFormat::Options opts = ImageFormat::Options().withMetadata (true).withRawChunks (true);
    auto reloaded = loadFromBlock (block, opts);

    ASSERT_TRUE (reloaded.isValid());
    ASSERT_TRUE (reloaded.hasMetadata());
    EXPECT_EQ ("JPEG COM marker round-trip test.", reloaded.getMetadata()->textEntries.getValue ("Comment", {}));
}

TEST (JpegMetadataExtendedTest, IccProfileRawChunkSurvivesRoundTrip)
{
    Image img = generateSolidImage (16, 16, PixelFormat::RGB, 0xFF336699u);

    const uint8 iccData[] = { 'I', 'C', 'C', '_', 'P', 'R', 'O', 'F', 'I', 'L', 'E', 0x00, 0x01, 0x02, 0x03, 0x04 };

    auto meta = ImageMetadata::create();
    meta->setRawChunk ("jpeg/icc", MemoryBlock (iccData, sizeof (iccData)));
    img.setMetadata (meta);

    auto block = writeImageToBlock<JpegImageFormatWriter> (img, 0);

    ImageFormat::Options opts = ImageFormat::Options().withRawChunks (true);
    auto reloaded = loadFromBlock (block, opts);

    ASSERT_TRUE (reloaded.isValid());
    ASSERT_TRUE (reloaded.hasMetadata());

    auto* iccChunk = reloaded.getMetadata()->getRawChunk ("jpeg/icc");
    ASSERT_NE (nullptr, iccChunk);
    EXPECT_EQ (sizeof (iccData), iccChunk->getSize());
}

TEST (JpegMetadataExtendedTest, LoadInvalidDataReturnsEmptyImage)
{
    // Completely random bytes should fail to decode.
    const uint8 garbage[] = { 0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    auto result = Image::loadFromData (Span<const uint8> (garbage, sizeof (garbage)));

    // Should gracefully return a failure, not crash.
    EXPECT_FALSE (result.wasOk());
}

TEST (JpegMetadataExtendedTest, GrayScaleRoundTrip)
{
    Image img (8, 8, PixelFormat::Grayscale);
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x)
            img.setPixelColor (x, y, Color (0xFF, (uint8) (x * 32), (uint8) (x * 32), (uint8) (x * 32)));

    auto block = writeImageToBlock<JpegImageFormatWriter> (img, 0);

    auto data = block.asBytes();
    auto result = Image::loadFromData (data);
    ASSERT_TRUE (result.wasOk());

    auto reloaded = std::move (result).getValue();
    ASSERT_TRUE (reloaded.isValid());
    EXPECT_EQ (reloaded.getWidth(), 8);
    EXPECT_EQ (reloaded.getHeight(), 8);
}

#endif // YUP_IMAGE_FORMAT_JPEG

//==============================================================================
// PNG extended metadata tests
//==============================================================================

#if YUP_MODULE_AVAILABLE_libpng && YUP_IMAGE_FORMAT_PNG

TEST (PngMetadataExtendedTest, TimeChunkSurvivesRoundTrip)
{
    Image img = generateSolidImage (16, 16, PixelFormat::RGBA, 0x8044AA88u);

    auto meta = ImageMetadata::create();
    meta->textEntries.set ("png/time", "2024:06:15 14:30:00");
    img.setMetadata (meta);

    auto block = writeImageToBlock<PngImageFormatWriter> (img);

    ImageFormat::Options opts = ImageFormat::Options().withMetadata (true);
    auto reloaded = loadFromBlock (block, opts);

    ASSERT_TRUE (reloaded.isValid());
    ASSERT_TRUE (reloaded.hasMetadata());
    EXPECT_EQ ("2024:06:15 14:30:00", reloaded.getMetadata()->textEntries.getValue ("png/time", {}));
}

TEST (PngMetadataExtendedTest, SRgbChunkSurvivesRoundTrip)
{
    Image img = generateSolidImage (16, 16, PixelFormat::RGBA, 0x8044AA88u);

    auto meta = ImageMetadata::create();
    meta->textEntries.set ("png/sRGB", "0");
    img.setMetadata (meta);

    auto block = writeImageToBlock<PngImageFormatWriter> (img);

    ImageFormat::Options opts = ImageFormat::Options().withMetadata (true);
    auto reloaded = loadFromBlock (block, opts);

    ASSERT_TRUE (reloaded.isValid());
    ASSERT_TRUE (reloaded.hasMetadata());
    EXPECT_EQ ("0", reloaded.getMetadata()->textEntries.getValue ("png/sRGB", {}));
}

TEST (PngMetadataExtendedTest, GammaChunkSurvivesRoundTrip)
{
    Image img = generateSolidImage (16, 16, PixelFormat::RGBA, 0x8044AA88u);

    auto meta = ImageMetadata::create();
    meta->textEntries.set ("png/gamma", "2.2");
    img.setMetadata (meta);

    auto block = writeImageToBlock<PngImageFormatWriter> (img);

    ImageFormat::Options opts = ImageFormat::Options().withMetadata (true);
    auto reloaded = loadFromBlock (block, opts);

    ASSERT_TRUE (reloaded.isValid());
    ASSERT_TRUE (reloaded.hasMetadata());
    EXPECT_EQ ("2.2", reloaded.getMetadata()->textEntries.getValue ("png/gamma", {}));
}

TEST (PngMetadataExtendedTest, IccpRawChunkSurvivesRoundTrip)
{
    Image img = generateSolidImage (16, 16, PixelFormat::RGBA, 0x8044AA88u);

    const uint8 iccpData[] = { 0x49, 0x43, 0x43, 0x50, 0x00, 0x01, 0x02, 0x03 };

    auto meta = ImageMetadata::create();
    meta->setRawChunk ("png/iCCP", MemoryBlock (iccpData, sizeof (iccpData)));
    img.setMetadata (meta);

    auto block = writeImageToBlock<PngImageFormatWriter> (img);

    ImageFormat::Options opts = ImageFormat::Options().withRawChunks (true);
    auto reloaded = loadFromBlock (block, opts);

    ASSERT_TRUE (reloaded.isValid());
    ASSERT_TRUE (reloaded.hasMetadata());

    auto* iccpChunk = reloaded.getMetadata()->getRawChunk ("png/iCCP");
    ASSERT_NE (nullptr, iccpChunk);
    EXPECT_EQ (sizeof (iccpData), iccpChunk->getSize());
}

TEST (PngMetadataExtendedTest, ChrmRawChunkSurvivesRoundTrip)
{
    Image img = generateSolidImage (16, 16, PixelFormat::RGBA, 0x8044AA88u);

    const uint8 chrmData[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F };

    auto meta = ImageMetadata::create();
    meta->setRawChunk ("png/cHRM", MemoryBlock (chrmData, sizeof (chrmData)));
    img.setMetadata (meta);

    auto block = writeImageToBlock<PngImageFormatWriter> (img);

    ImageFormat::Options opts = ImageFormat::Options().withRawChunks (true);
    auto reloaded = loadFromBlock (block, opts);

    ASSERT_TRUE (reloaded.isValid());
    ASSERT_TRUE (reloaded.hasMetadata());

    auto* chrmChunk = reloaded.getMetadata()->getRawChunk ("png/cHRM");
    ASSERT_NE (nullptr, chrmChunk);
    EXPECT_EQ (sizeof (chrmData), chrmChunk->getSize());
}

TEST (PngMetadataExtendedTest, LoadInvalidDataReturnsEmptyImage)
{
    const uint8 garbage[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    auto result = Image::loadFromData (Span<const uint8> (garbage, sizeof (garbage)));
    EXPECT_FALSE (result.wasOk());
}

#endif // YUP_IMAGE_FORMAT_PNG
