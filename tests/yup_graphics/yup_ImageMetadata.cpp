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

namespace
{

//==============================================================================
// Helper: build a valid TIFF/EXIF blob with multiple tags for testing the IFD walker.
// Big-endian EXIF with IFD0 containing Make, Model, DateTime, Orientation,
// ImageDescription, Copyright, Software, and a GPS IFD sub-block.
//==============================================================================
static MemoryBlock buildTestExifBlob()
{
    // TIFF header (8 bytes)
    const uint8 tiffHeader[] = {
        'M', 'M', // big-endian
        0x00,
        0x2A, // TIFF magic
        0x00,
        0x00,
        0x00,
        0x08 // offset to IFD0 = 8
    };

    // String data that will be pointed to by offsets
    // We need to know the offsets. Let's compute:
    // TIFF header: 8 bytes
    // IFD0: 2 (count) + N*12 (entries) + 4 (next IFD offset) = 2 + 9*12 + 4 = 114 bytes
    // IFD0 starts at offset 8, string data starts at offset 8 + 114 = 122

    // String data at offsets:
    // 122: "Canon\0" (6 bytes) → "Make"
    // 128: "EOS R5\0" (7 bytes) → "Model"
    // 135: "2024:06:15 14:30:00\0" (20 bytes) → "DateTime"
    // 155: "Test image\0" (11 bytes) → "ImageDescription"
    // 166: "YUP Test\0" (9 bytes) → "Software"
    // 175: "Copyright 2024\0" (16 bytes) → "Copyright"

    // GPS IFD starts after string data:
    // 191: GPS IFD (aligned)

    // Build the blob
    std::vector<uint8> blob;
    blob.insert (blob.end(), tiffHeader, tiffHeader + 8);

    // IFD0: 9 entries
    auto write16 = [&] (uint16 v)
    {
        blob.push_back (static_cast<uint8> (v >> 8));
        blob.push_back (static_cast<uint8> (v & 0xFF));
    };
    auto write32 = [&] (uint32 v)
    {
        blob.push_back (static_cast<uint8> (v >> 24));
        blob.push_back (static_cast<uint8> ((v >> 16) & 0xFF));
        blob.push_back (static_cast<uint8> ((v >> 8) & 0xFF));
        blob.push_back (static_cast<uint8> (v & 0xFF));
    };
    auto writeIFDEntry = [&] (uint16 tag, uint16 type, uint32 count, uint32 value)
    {
        write16 (tag);
        write16 (type);
        write32 (count);
        write32 (value);
    };

    write16 (9); // 9 entries

    // Entry 1: Make = "Canon" at offset 122
    writeIFDEntry (0x010F, 2, 6, 122);
    // Entry 2: Model = "EOS R5" at offset 128
    writeIFDEntry (0x0110, 2, 7, 128);
    // Entry 3: DateTime = "2024:06:15 14:30:00" at offset 135
    writeIFDEntry (0x0132, 2, 20, 135);
    // Entry 4: ImageDescription = "Test image" at offset 155
    writeIFDEntry (0x010E, 2, 11, 155);
    // Entry 5: Orientation = 6
    writeIFDEntry (0x0112, 3, 1, 6);
    // Entry 6: Software = "YUP Test" at offset 166
    writeIFDEntry (0x0131, 2, 9, 166);
    // Entry 7: Copyright = "Copyright 2024" at offset 175
    writeIFDEntry (0x8298, 2, 16, 175);
    // Entry 8: ExifIFDPointer = 0 (no sub-IFD)
    writeIFDEntry (0x8769, 4, 1, 0);
    // Entry 9: GPSIFDPointer = 191
    writeIFDEntry (0x8825, 4, 1, 191);

    // Next IFD offset = 0
    write32 (0);

    // String data
    auto writeString = [&] (const char* s, size_t len)
    {
        blob.insert (blob.end(), reinterpret_cast<const uint8*> (s), reinterpret_cast<const uint8*> (s) + len);
    };

    writeString ("Canon\0", 6);                // offset 122
    writeString ("EOS R5\0", 7);               // offset 128
    writeString ("2024:06:15 14:30:00\0", 20); // offset 135
    writeString ("Test image\0", 11);          // offset 155
    writeString ("YUP Test\0", 9);             // offset 166
    writeString ("Copyright 2024\0", 16);      // offset 175

    // Now at offset 191: GPS IFD
    // GPS entries: 4 entries
    // GPSLatitudeRef "N\0" (2 bytes) → fits in 4 bytes, stored inline: 'N', 0, 0, 0 → BE32 = 0x4E000000
    // GPSLatitude: 3 rationals = 24 bytes → offset to data after GPS IFD
    // GPSLongitudeRef "W\0" (2 bytes) → fits in 4 bytes, stored inline: 'W', 0, 0, 0 → BE32 = 0x57000000
    // GPSLongitude: 3 rationals = 24 bytes → offset to data after GPS IFD

    size_t gpsDataStart = blob.size() + 2 + 4 * 12 + 4; // after GPS IFD header

    write16 (4); // 4 GPS entries

    // GPSLatitudeRef: ASCII, count=2, inline value "N\0"
    writeIFDEntry (0x0001, 2, 2, 0x4E000000u);
    // GPSLatitude: RATIONAL, count=3, data at gpsDataStart
    writeIFDEntry (0x0002, 5, 3, static_cast<uint32> (gpsDataStart));
    // GPSLongitudeRef: ASCII, count=2, inline value "W\0"
    writeIFDEntry (0x0003, 2, 2, 0x57000000u);
    // GPSLongitude: RATIONAL, count=3, data at gpsDataStart + 24
    writeIFDEntry (0x0004, 5, 3, static_cast<uint32> (gpsDataStart + 24));

    // Next IFD offset = 0
    write32 (0);

    // GPS rational data: Lat (37/1, 15/1, 0/1), Lon (122/1, 0/1, 0/1)
    auto writeRational = [&] (uint32 num, uint32 den)
    {
        write32 (num);
        write32 (den);
    };
    writeRational (37, 1);
    writeRational (15, 1);
    writeRational (0, 1);
    writeRational (122, 1);
    writeRational (0, 1);
    writeRational (0, 1);

    return MemoryBlock (blob.data(), blob.size());
}

//==============================================================================
// Helper: build a minimal 1x1 JPEG with a custom EXIF blob appended as APP1
//==============================================================================
static std::vector<uint8> buildJpegWithExif (const MemoryBlock& exifBlob)
{
    // JPEG SOI
    std::vector<uint8> jpeg = { 0xFF, 0xD8 };

    // APP1 marker with EXIF
    auto exifSize = static_cast<uint16> (exifBlob.getSize() + 6 + 2); // 6=Exif\0\0 header, 2=length field
    jpeg.push_back (0xFF);
    jpeg.push_back (0xE1); // APP1
    jpeg.push_back (static_cast<uint8> (exifSize >> 8));
    jpeg.push_back (static_cast<uint8> (exifSize & 0xFF));
    jpeg.push_back ('E');
    jpeg.push_back ('x');
    jpeg.push_back ('i');
    jpeg.push_back ('f');
    jpeg.push_back (0x00);
    jpeg.push_back (0x00);
    const auto* exifData = static_cast<const uint8*> (exifBlob.getData());
    jpeg.insert (jpeg.end(), exifData, exifData + exifBlob.getSize());

    // Minimal JPEG data: DQT + SOF0 + DHT + SOS + scan data + EOI
    const uint8 jpegData[] = {
        0xFF, 0xDB, 0x00, 0x43, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x00, 0x01, 0x00, 0x01, 0x01, 0x01, 0x11, 0x00, 0xFF, 0xC4, 0x00, 0x1F, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01, 0x00, 0x00, 0x3F, 0x00, 0x7F, 0xA9, 0x87, 0xFF, 0xD9
    };
    jpeg.insert (jpeg.end(), jpegData, jpegData + sizeof (jpegData));
    return jpeg;
}

//==============================================================================
// Helper: load image from bytes with options
//==============================================================================
static Image loadWithOptions (const std::vector<uint8>& data, const ImageFormat::Options& opts)
{
    auto result = Image::loadFromData (Span<const uint8> (data.data(), data.size()), opts);
    EXPECT_TRUE (result.wasOk());
    if (! result.wasOk())
        return {};
    auto img = std::move (result).getValue();
    EXPECT_TRUE (img.isValid());
    return img;
}

//==============================================================================
// Helper: create metadata with common test values
//==============================================================================
static ImageMetadata::Ptr createTestMetadata()
{
    auto meta = ImageMetadata::create();
    meta->dpiX = 300.0;
    meta->dpiY = 300.0;
    meta->textEntries.set ("Title", "Test Image");
    meta->textEntries.set ("Author", "YUP Test Suite");
    meta->textEntries.set ("Comment", "This is a test comment for metadata round-trip verification.");
    return meta;
}

} // namespace

//==============================================================================
// ImageMetadata — Construction and defaults
//==============================================================================

TEST (ImageMetadataTest, CreateReturnsValidPointer)
{
    auto meta = ImageMetadata::create();
    ASSERT_NE (nullptr, meta);
}

TEST (ImageMetadataTest, CreateEmptyAlias)
{
    auto meta = ImageMetadata::createEmpty();
    ASSERT_NE (nullptr, meta);
    EXPECT_DOUBLE_EQ (0.0, meta->dpiX);
    EXPECT_DOUBLE_EQ (0.0, meta->dpiY);
}

TEST (ImageMetadataTest, DpiDefaultsToZero)
{
    auto meta = ImageMetadata::create();
    EXPECT_DOUBLE_EQ (0.0, meta->dpiX);
    EXPECT_DOUBLE_EQ (0.0, meta->dpiY);
}

TEST (ImageMetadataTest, TextEntriesStartEmpty)
{
    auto meta = ImageMetadata::create();
    EXPECT_TRUE (meta->textEntries.isEmpty());
    EXPECT_EQ (0, meta->textEntries.getAllKeys().size());
}

TEST (ImageMetadataTest, TextEntriesSetAndGet)
{
    auto meta = ImageMetadata::create();
    meta->textEntries.set ("Title", "My Image");
    meta->textEntries.set ("Author", "John Doe");

    EXPECT_EQ ("My Image", meta->textEntries.getValue ("Title", {}));
    EXPECT_EQ ("John Doe", meta->textEntries.getValue ("Author", {}));
    EXPECT_EQ (String(), meta->textEntries.getValue ("Missing", {}));
}

TEST (ImageMetadataTest, TextEntriesOverwrite)
{
    auto meta = ImageMetadata::create();
    meta->textEntries.set ("Key", "Value1");
    meta->textEntries.set ("Key", "Value2");
    EXPECT_EQ ("Value2", meta->textEntries.getValue ("Key", {}));
}

TEST (ImageMetadataTest, RawChunksStartEmpty)
{
    auto meta = ImageMetadata::create();
    EXPECT_EQ (nullptr, meta->getRawChunk ("anything"));
    EXPECT_FALSE (meta->hasRawChunk ("anything"));
}

TEST (ImageMetadataTest, RawChunksSetAndGet)
{
    auto meta = ImageMetadata::create();
    const uint8 data[] = { 0xDE, 0xAD, 0xBE, 0xEF };
    meta->setRawChunk ("jpeg/exif", MemoryBlock (data, sizeof (data)));

    EXPECT_TRUE (meta->hasRawChunk ("jpeg/exif"));
    auto* chunk = meta->getRawChunk ("jpeg/exif");
    ASSERT_NE (nullptr, chunk);
    EXPECT_EQ (4u, chunk->getSize());
    EXPECT_TRUE (chunk->matches (data, sizeof (data)));
}

TEST (ImageMetadataTest, SetRawChunkReplaces)
{
    auto meta = ImageMetadata::create();
    meta->setRawChunk ("key", MemoryBlock ("ab", 2));
    meta->setRawChunk ("key", MemoryBlock ("cde", 3));
    auto* chunk = meta->getRawChunk ("key");
    ASSERT_NE (nullptr, chunk);
    EXPECT_EQ (3u, chunk->getSize());
}

TEST (ImageMetadataTest, WellKnownChunkKeys)
{
    EXPECT_EQ (String ("exif"), ImageMetadata::kExifChunk);
    EXPECT_EQ (String ("icc"), ImageMetadata::kIccChunk);
    EXPECT_EQ (String ("xmp"), ImageMetadata::kXmpChunk);
}

//==============================================================================
// ImageMetadata — EXIF IFD walker (lazy parsing)
//==============================================================================

TEST (ImageMetadataExifTest, ReturnsDefaultsWhenNoExif)
{
    auto meta = ImageMetadata::create();
    EXPECT_EQ (0, meta->getOrientation());
    EXPECT_TRUE (meta->getCreationDate().isEmpty());
    EXPECT_TRUE (meta->getCameraMake().isEmpty());
    EXPECT_TRUE (meta->getCameraModel().isEmpty());
    EXPECT_TRUE (meta->getImageDescription().isEmpty());
    EXPECT_TRUE (meta->getCopyright().isEmpty());
    EXPECT_TRUE (meta->getSoftware().isEmpty());

    auto [lat, lon] = meta->getGpsCoordinates();
    EXPECT_DOUBLE_EQ (0.0, lat);
    EXPECT_DOUBLE_EQ (0.0, lon);
}

TEST (ImageMetadataExifTest, ParsesOrientation)
{
    // Minimal valid EXIF blob: big-endian, 1 entry (orientation = 6)
    const uint8 rawExif[] = {
        'M', 'M', 0x00, 0x2A, 0x00, 0x00, 0x00, 0x08, // TIFF header, IFD0 at offset 8
        0x00,
        0x01, // 1 entry
        0x01,
        0x12,
        0x00,
        0x03,
        0x00,
        0x00,
        0x00,
        0x01,
        0x00,
        0x06,
        0x00,
        0x00, // Orientation=6
        0x00,
        0x00,
        0x00,
        0x00 // next IFD = 0
    };

    auto meta = ImageMetadata::create();
    meta->setRawChunk ("jpeg/exif", MemoryBlock (rawExif, sizeof (rawExif)));
    EXPECT_EQ (6, meta->getOrientation());
}

TEST (ImageMetadataExifTest, ParsesCameraMake)
{
    auto exifBlob = buildTestExifBlob();

    auto meta = ImageMetadata::create();
    meta->setRawChunk ("jpeg/exif", std::move (exifBlob));
    EXPECT_EQ (String ("Canon"), meta->getCameraMake());
}

TEST (ImageMetadataExifTest, ParsesCameraModel)
{
    auto exifBlob = buildTestExifBlob();

    auto meta = ImageMetadata::create();
    meta->setRawChunk ("jpeg/exif", std::move (exifBlob));
    EXPECT_EQ (String ("EOS R5"), meta->getCameraModel());
}

TEST (ImageMetadataExifTest, ParsesCreationDate)
{
    auto exifBlob = buildTestExifBlob();

    auto meta = ImageMetadata::create();
    meta->setRawChunk ("jpeg/exif", std::move (exifBlob));
    EXPECT_EQ (String ("2024:06:15 14:30:00"), meta->getCreationDate());
}

TEST (ImageMetadataExifTest, ParsesImageDescription)
{
    auto exifBlob = buildTestExifBlob();

    auto meta = ImageMetadata::create();
    meta->setRawChunk ("jpeg/exif", std::move (exifBlob));
    EXPECT_EQ (String ("Test image"), meta->getImageDescription());
}

TEST (ImageMetadataExifTest, ParsesCopyright)
{
    auto exifBlob = buildTestExifBlob();

    auto meta = ImageMetadata::create();
    meta->setRawChunk ("jpeg/exif", std::move (exifBlob));
    EXPECT_EQ (String ("Copyright 2024"), meta->getCopyright());
}

TEST (ImageMetadataExifTest, ParsesSoftware)
{
    auto exifBlob = buildTestExifBlob();

    auto meta = ImageMetadata::create();
    meta->setRawChunk ("jpeg/exif", std::move (exifBlob));
    EXPECT_EQ (String ("YUP Test"), meta->getSoftware());
}

TEST (ImageMetadataExifTest, ParsesGpsCoordinates)
{
    auto exifBlob = buildTestExifBlob();

    auto meta = ImageMetadata::create();
    meta->setRawChunk ("jpeg/exif", std::move (exifBlob));

    auto [lat, lon] = meta->getGpsCoordinates();
    EXPECT_NEAR (37.25, lat, 0.001);
    EXPECT_NEAR (-122.0, lon, 0.001);
}

TEST (ImageMetadataExifTest, FindsExifFromTiffKey)
{
    const uint8 rawExif[] = {
        'M', 'M', 0x00, 0x2A, 0x00, 0x00, 0x00, 0x08, 0x00, 0x01, 0x01, 0x12, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    auto meta = ImageMetadata::create();
    meta->setRawChunk ("tiff/exif", MemoryBlock (rawExif, sizeof (rawExif)));
    EXPECT_EQ (6, meta->getOrientation());
}

//==============================================================================
// Image + ImageMetadata integration
//==============================================================================

TEST (ImageMetadataIntegrationTest, DefaultImageHasNoMetadata)
{
    Image img;
    EXPECT_FALSE (img.hasMetadata());
    EXPECT_EQ (nullptr, img.getMetadata());
}

TEST (ImageMetadataIntegrationTest, ConstructedImageHasNoMetadata)
{
    Image img (16, 16, PixelFormat::RGBA);
    EXPECT_TRUE (img.isValid());
    EXPECT_FALSE (img.hasMetadata());
}

TEST (ImageMetadataIntegrationTest, SetAndGetMetadata)
{
    Image img (8, 8, PixelFormat::RGBA);
    auto meta = ImageMetadata::create();
    meta->dpiX = 300.0;
    meta->textEntries.set ("Key", "Value");
    img.setMetadata (meta);

    ASSERT_TRUE (img.hasMetadata());
    EXPECT_DOUBLE_EQ (300.0, img.getMetadata()->dpiX);
    EXPECT_EQ ("Value", img.getMetadata()->textEntries.getValue ("Key", {}));
}

TEST (ImageMetadataIntegrationTest, CopySharesMetadata)
{
    Image img (8, 8, PixelFormat::RGBA);
    auto meta = ImageMetadata::create();
    meta->dpiX = 150.0;
    img.setMetadata (meta);

    Image copy = img;
    ASSERT_TRUE (copy.hasMetadata());
    EXPECT_EQ (img.getMetadata().get(), copy.getMetadata().get());
    EXPECT_DOUBLE_EQ (150.0, copy.getMetadata()->dpiX);
}

TEST (ImageMetadataIntegrationTest, MoveTransfersMetadata)
{
    Image img (8, 8, PixelFormat::RGBA);
    auto meta = ImageMetadata::create();
    meta->dpiX = 150.0;
    img.setMetadata (meta);

    Image moved = std::move (img);
    EXPECT_TRUE (moved.hasMetadata());
    EXPECT_FALSE (img.hasMetadata());
    EXPECT_DOUBLE_EQ (150.0, moved.getMetadata()->dpiX);
}

TEST (ImageMetadataIntegrationTest, DuplicateSharesMetadata)
{
    Image img (8, 8, PixelFormat::RGBA);
    auto meta = ImageMetadata::create();
    meta->dpiX = 72.0;
    meta->textEntries.set ("Title", "Original");
    img.setMetadata (meta);

    Image dup = img.duplicate();
    ASSERT_TRUE (dup.hasMetadata());
    EXPECT_DOUBLE_EQ (72.0, dup.getMetadata()->dpiX);
    EXPECT_EQ ("Original", dup.getMetadata()->textEntries.getValue ("Title", {}));
    // Pixel data should be different buffers
    EXPECT_NE (img.getPixelData().getRawData().data(), dup.getPixelData().getRawData().data());
}

TEST (ImageMetadataIntegrationTest, SetMetadataToNullptrRemovesMetadata)
{
    Image img (8, 8, PixelFormat::RGBA);
    img.setMetadata (ImageMetadata::create());
    EXPECT_TRUE (img.hasMetadata());

    img.setMetadata (nullptr);
    EXPECT_FALSE (img.hasMetadata());
}
