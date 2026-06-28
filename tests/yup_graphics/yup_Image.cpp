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

#include <memory>
#include <stdexcept>
#include <utility>

using namespace yup;

TEST (ImageTests, RgbaBitmapStoresRGBABytesAndReturnsARGBColor)
{
    Image image (1, 1, PixelFormat::RGBA);

    image.setPixel (0, 0, 0x80123456);

    const auto raw = image.getRawData();

    ASSERT_EQ (raw.size(), 4u);
    EXPECT_EQ (raw[0], 0x12);
    EXPECT_EQ (raw[1], 0x34);
    EXPECT_EQ (raw[2], 0x56);
    EXPECT_EQ (raw[3], 0x80);
    EXPECT_EQ (image.getPixel (0, 0), 0x80123456u);
    EXPECT_EQ (image.getPixelColor (0, 0), Color (0x80123456));
}

TEST (ImageTests, RgbBitmapStoresRGBBytesAndReturnsOpaqueARGBColor)
{
    Image image (1, 1, PixelFormat::RGB);

    image.setPixelColor (0, 0, Color (0x80123456));

    const auto raw = image.getRawData();

    ASSERT_EQ (raw.size(), 3u);
    EXPECT_EQ (raw[0], 0x12);
    EXPECT_EQ (raw[1], 0x34);
    EXPECT_EQ (raw[2], 0x56);
    EXPECT_EQ (image.getPixel (0, 0), 0xff123456u);
}

TEST (ImageTests, GrayscaleBitmapStoresLuminanceAndReturnsOpaqueARGBColor)
{
    Image image (1, 1, PixelFormat::Grayscale);

    image.setPixelColor (0, 0, Color (0xffff0000));

    const auto raw = image.getRawData();

    ASSERT_EQ (raw.size(), 1u);
    EXPECT_EQ (raw[0], 54);
    EXPECT_EQ (image.getPixel (0, 0), 0xff363636u);
}

TEST (ImageTests, GrayscaleBitmapConvertsToOpaqueRGBATextureBytes)
{
    Image image (3, 1, PixelFormat::Grayscale);

    auto raw = image.getRawData();
    ASSERT_EQ (raw.size(), 3u);
    raw[0] = 0;
    raw[1] = 127;
    raw[2] = 255;

    uint8 textureBytes[12] = {};
    ColorVectorOperations::convertGrayscaleToRGBA (raw.data(), textureBytes, 3);

    const uint8 expected[] = { 0, 0, 0, 255, 127, 127, 127, 255, 255, 255, 255, 255 };

    for (size_t i = 0; i < std::size (expected); ++i)
        EXPECT_EQ (textureBytes[i], expected[i]);
}

TEST (ImageTests, RgbBitmapConvertsToOpaqueRGBATextureBytes)
{
    Image image (2, 1, PixelFormat::RGB);

    image.setPixel (0, 0, 0x80123456);
    image.setPixel (1, 0, 0xffabcdef);

    const auto raw = image.getRawData();
    uint8 textureBytes[8] = {};
    ColorVectorOperations::convertRGBToRGBA (raw.data(), textureBytes, 2);

    const uint8 expected[] = { 0x12, 0x34, 0x56, 0xff, 0xab, 0xcd, 0xef, 0xff };

    for (size_t i = 0; i < std::size (expected); ++i)
        EXPECT_EQ (textureBytes[i], expected[i]);
}

TEST (ImageTests, RgbaBitmapConvertsToPremultipliedRGBATextureBytes)
{
    Image image (2, 1, PixelFormat::RGBA);

    image.setPixel (0, 0, 0x80102040);
    image.setPixel (1, 0, 0xff010203);

    const auto raw = image.getRawData();
    uint8 textureBytes[8] = {};
    std::memcpy (textureBytes, raw.data(), raw.size());
    ColorVectorOperations::premultiplyRGBA (textureBytes, 2);

    const uint8 expected[] = { 8, 16, 32, 128, 1, 2, 3, 255 };

    for (size_t i = 0; i < std::size (expected); ++i)
        EXPECT_EQ (textureBytes[i], expected[i]);
}

TEST (ImageTests, ColorCanConvertToExplicitPackedByteOrders)
{
    const Color color (0x80123456);

    EXPECT_EQ (color.getARGB(), 0x80123456u);
    EXPECT_EQ (color.getRGBA(), 0x12345680u);
    EXPECT_EQ (color.getBGRA(), 0x56341280u);
    EXPECT_EQ (Color::fromRGBA (0x12345680), color);
    EXPECT_EQ (Color::fromBGRA (0x56341280), color);
}

TEST (BitmapDataTests, RgbaSetPixelWritesAtCorrectRowOffset)
{
    BitmapData bitmap (2, 2, PixelFormat::RGBA);
    bitmap.clear();

    bitmap.setPixel (1, 1, 0x80123456);

    const auto raw = bitmap.getRawData();

    ASSERT_EQ (raw.size(), 16u);
    EXPECT_EQ (raw[12], 0x12);
    EXPECT_EQ (raw[13], 0x34);
    EXPECT_EQ (raw[14], 0x56);
    EXPECT_EQ (raw[15], 0x80);
    EXPECT_EQ (bitmap.getPixel (1, 1), 0x80123456u);
}

TEST (BitmapDataTests, DefaultConstructorCreatesEmptyBitmap)
{
    BitmapData bitmap;

    EXPECT_EQ (bitmap.getWidth(), 0);
    EXPECT_EQ (bitmap.getHeight(), 0);
    EXPECT_EQ (bitmap.getPixelFormat(), PixelFormat::RGBA);
    EXPECT_EQ (bitmap.getPixelStride(), 4);
    EXPECT_TRUE (bitmap.getRawData().empty());

    EXPECT_NO_THROW (bitmap.clear());
    EXPECT_NO_THROW (bitmap.fill (0xffffffff));
    EXPECT_THROW (bitmap.getPixel (0, 0), std::out_of_range);
}

TEST (BitmapDataTests, ConstructorAdoptsProvidedPixelData)
{
    auto pixels = std::unique_ptr<uint8[]> (new uint8[4] { 0x12, 0x34, 0x56, 0x80 });

    BitmapData bitmap (1, 1, PixelFormat::RGBA, std::unique_ptr<const uint8[]> (pixels.release()));

    EXPECT_EQ (bitmap.getRawData().size(), 4u);
    EXPECT_EQ (bitmap.getPixel (0, 0), 0x80123456u);

    bitmap.setPixel (0, 0, 0xffabcdef);
    EXPECT_EQ (bitmap.getPixel (0, 0), 0xffabcdefu);
}

TEST (BitmapDataTests, FillWritesExpectedBytesForRGBA)
{
    BitmapData bitmap (2, 2, PixelFormat::RGBA);

    bitmap.fill (0x80123456);

    const auto raw = bitmap.getRawData();

    ASSERT_EQ (raw.size(), 16u);
    for (size_t i = 0; i < raw.size(); i += 4)
    {
        EXPECT_EQ (raw[i], 0x12);
        EXPECT_EQ (raw[i + 1], 0x34);
        EXPECT_EQ (raw[i + 2], 0x56);
        EXPECT_EQ (raw[i + 3], 0x80);
    }
}

TEST (BitmapDataTests, FillWritesExpectedBytesForRGB)
{
    BitmapData bitmap (2, 1, PixelFormat::RGB);

    bitmap.fill (0x80123456);

    const auto raw = bitmap.getRawData();

    ASSERT_EQ (raw.size(), 6u);
    EXPECT_EQ (raw[0], 0x12);
    EXPECT_EQ (raw[1], 0x34);
    EXPECT_EQ (raw[2], 0x56);
    EXPECT_EQ (raw[3], 0x12);
    EXPECT_EQ (raw[4], 0x34);
    EXPECT_EQ (raw[5], 0x56);
}

TEST (BitmapDataTests, FillWritesExpectedBytesForGrayscale)
{
    BitmapData bitmap (3, 1, PixelFormat::Grayscale);

    bitmap.fillColor (Color (0xff00ff00));

    const auto raw = bitmap.getRawData();

    ASSERT_EQ (raw.size(), 3u);
    EXPECT_EQ (raw[0], 182);
    EXPECT_EQ (raw[1], 182);
    EXPECT_EQ (raw[2], 182);
    EXPECT_EQ (bitmap.getPixel (2, 0), 0xffb6b6b6u);
}

TEST (BitmapDataTests, SetPixelColorAndGetPixelColorRoundTrip)
{
    BitmapData bitmap (1, 1, PixelFormat::RGBA);

    bitmap.setPixelColor (0, 0, Color (0x80123456));

    EXPECT_EQ (bitmap.getPixelColor (0, 0), Color (0x80123456));
}

TEST (BitmapDataTests, MutableRawDataUpdatesPixelValues)
{
    BitmapData bitmap (1, 1, PixelFormat::RGB);

    auto raw = bitmap.getRawData();
    ASSERT_EQ (raw.size(), 3u);

    raw[0] = 0x12;
    raw[1] = 0x34;
    raw[2] = 0x56;

    EXPECT_EQ (bitmap.getPixel (0, 0), 0xff123456u);
}

TEST (BitmapDataTests, ClearZerosRawData)
{
    BitmapData bitmap (2, 2, PixelFormat::RGBA);

    bitmap.fill (0xffffffff);
    bitmap.clear();

    const auto raw = bitmap.getRawData();

    ASSERT_EQ (raw.size(), 16u);
    for (const auto byte : raw)
        EXPECT_EQ (byte, 0);
}

TEST (BitmapDataTests, MoveConstructorPreservesPixelDataAndStrides)
{
    BitmapData original (2, 2, PixelFormat::RGBA);
    original.setPixel (1, 1, 0x80123456);

    BitmapData moved (std::move (original));

    EXPECT_EQ (moved.getWidth(), 2);
    EXPECT_EQ (moved.getHeight(), 2);
    EXPECT_EQ (moved.getPixelStride(), 4);
    EXPECT_EQ (moved.getRawData().size(), 16u);
    EXPECT_EQ (moved.getPixel (1, 1), 0x80123456u);

    EXPECT_EQ (original.getWidth(), 0);
    EXPECT_EQ (original.getHeight(), 0);
    EXPECT_EQ (original.getRawData().size(), 0u);
}

TEST (BitmapDataTests, MoveAssignmentPreservesPixelDataAndStrides)
{
    BitmapData source (2, 1, PixelFormat::RGB);
    source.setPixel (1, 0, 0x80123456);

    BitmapData target (1, 1, PixelFormat::Grayscale);
    target = std::move (source);

    EXPECT_EQ (target.getWidth(), 2);
    EXPECT_EQ (target.getHeight(), 1);
    EXPECT_EQ (target.getPixelFormat(), PixelFormat::RGB);
    EXPECT_EQ (target.getPixelStride(), 3);
    EXPECT_EQ (target.getRawData().size(), 6u);
    EXPECT_EQ (target.getPixel (1, 0), 0xff123456u);
}

TEST (BitmapDataTests, ConstructorRejectsInvalidDimensions)
{
    EXPECT_THROW (BitmapData (0, 1, PixelFormat::RGBA), std::invalid_argument);
    EXPECT_THROW (BitmapData (1, 0, PixelFormat::RGBA), std::invalid_argument);
    EXPECT_THROW (BitmapData (-1, 1, PixelFormat::RGBA), std::invalid_argument);
    EXPECT_THROW (BitmapData (1, -1, PixelFormat::RGBA), std::invalid_argument);
    EXPECT_THROW (BitmapData (1, 1, static_cast<PixelFormat> (255)), std::runtime_error);
}

TEST (BitmapDataTests, PixelAccessRejectsOutOfRangeCoordinates)
{
    BitmapData bitmap (2, 2, PixelFormat::RGBA);

    EXPECT_THROW (bitmap.setPixel (-1, 0, 0xffffffff), std::out_of_range);
    EXPECT_THROW (bitmap.setPixel (0, -1, 0xffffffff), std::out_of_range);
    EXPECT_THROW (bitmap.setPixel (2, 0, 0xffffffff), std::out_of_range);
    EXPECT_THROW (bitmap.setPixel (0, 2, 0xffffffff), std::out_of_range);
    EXPECT_THROW (bitmap.getPixel (2, 0), std::out_of_range);
    EXPECT_THROW (bitmap.getPixelColor (0, 2), std::out_of_range);
}

TEST (ImageTests, DefaultConstructorCreatesInvalidImage)
{
    const Image image;

    EXPECT_FALSE (image.isValid());
}

TEST (ImageTests, ConstructorExposesBitmapMetadata)
{
    Image image (3, 2, PixelFormat::RGB);

    EXPECT_TRUE (image.isValid());
    EXPECT_EQ (image.getWidth(), 3);
    EXPECT_EQ (image.getHeight(), 2);
    EXPECT_EQ (image.getPixelFormat(), PixelFormat::RGB);
    EXPECT_EQ (image.getPixelStride(), 3);
    EXPECT_EQ (image.getRawData().size(), 18u);

    const auto& constImage = image;
    EXPECT_EQ (constImage.getBitmapData().getWidth(), 3);
    EXPECT_EQ (image.getBitmapData().getHeight(), 2);
}

TEST (ImageTests, FillColorAndClearRoundTrip)
{
    Image image (2, 2, PixelFormat::RGBA);

    image.fillColor (Color (0x80123456));
    EXPECT_EQ (image.getPixelColor (0, 0), Color (0x80123456));
    EXPECT_EQ (image.getPixelColor (1, 1), Color (0x80123456));

    image.clear();
    EXPECT_EQ (image.getPixel (0, 0), 0u);
    EXPECT_EQ (image.getPixel (1, 1), 0u);
}

TEST (ImageTests, MutableRawDataUpdatesPixelValues)
{
    Image image (1, 1, PixelFormat::RGBA);

    auto raw = image.getRawData();
    ASSERT_EQ (raw.size(), 4u);

    raw[0] = 0x12;
    raw[1] = 0x34;
    raw[2] = 0x56;
    raw[3] = 0x80;

    EXPECT_EQ (image.getPixelColor (0, 0), Color (0x80123456));
}

TEST (ImageTests, CopyConstructorPreservesBitmapData)
{
    Image original (1, 1, PixelFormat::RGBA);
    original.setPixel (0, 0, 0x80123456);

    Image copy (original);

    EXPECT_TRUE (copy.isValid());
    EXPECT_EQ (copy.getWidth(), 1);
    EXPECT_EQ (copy.getHeight(), 1);
    EXPECT_EQ (copy.getPixelFormat(), PixelFormat::RGBA);
    EXPECT_EQ (copy.getPixel (0, 0), 0x80123456u);
}

TEST (ImageTests, CopyAssignmentPreservesBitmapData)
{
    Image original (1, 1, PixelFormat::RGB);
    original.setPixel (0, 0, 0x80123456);

    Image copy (2, 2, PixelFormat::RGBA);
    copy = original;

    EXPECT_EQ (copy.getWidth(), 1);
    EXPECT_EQ (copy.getHeight(), 1);
    EXPECT_EQ (copy.getPixelFormat(), PixelFormat::RGB);
    EXPECT_EQ (copy.getPixel (0, 0), 0xff123456u);
}

TEST (ImageTests, MoveConstructorTransfersBitmapData)
{
    Image source (1, 1, PixelFormat::RGBA);
    source.setPixel (0, 0, 0x80123456);

    Image moved (std::move (source));

    EXPECT_FALSE (source.isValid());
    EXPECT_TRUE (moved.isValid());
    EXPECT_EQ (moved.getWidth(), 1);
    EXPECT_EQ (moved.getHeight(), 1);
    EXPECT_EQ (moved.getPixel (0, 0), 0x80123456u);
}

TEST (ImageTests, MoveAssignmentTransfersBitmapData)
{
    Image source (1, 1, PixelFormat::RGBA);
    source.setPixel (0, 0, 0x80123456);

    Image moved (2, 2, PixelFormat::RGB);
    moved = std::move (source);

    EXPECT_FALSE (source.isValid());
    EXPECT_TRUE (moved.isValid());
    EXPECT_EQ (moved.getWidth(), 1);
    EXPECT_EQ (moved.getHeight(), 1);
    EXPECT_EQ (moved.getPixel (0, 0), 0x80123456u);
}

TEST (ImageTests, ConstructorRejectsInvalidDimensions)
{
    EXPECT_THROW (Image (0, 1, PixelFormat::RGBA), std::invalid_argument);
    EXPECT_THROW (Image (1, 0, PixelFormat::RGBA), std::invalid_argument);
}

TEST (ImageTests, PixelAccessRejectsOutOfRangeCoordinates)
{
    Image image (2, 2, PixelFormat::RGBA);

    EXPECT_THROW (image.setPixel (-1, 0, 0xffffffff), std::out_of_range);
    EXPECT_THROW (image.setPixelColor (0, -1, Color (0xffffffff)), std::out_of_range);
    EXPECT_THROW (image.getPixel (2, 0), std::out_of_range);
    EXPECT_THROW (image.getPixelColor (0, 2), std::out_of_range);
}

TEST (ImageTests, LoadFromDataFailsForEmptyInput)
{
    const auto result = Image::loadFromData ({});

    EXPECT_FALSE (result.wasOk());
}

TEST (ImageTests, LoadFromDataFailsForGarbageInput)
{
    const uint8 garbage[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 };
    const auto result = Image::loadFromData (Span<const uint8> (garbage, std::size (garbage)));

    EXPECT_FALSE (result.wasOk());
}

TEST (ImageTests, LoadFromDataRoundTripBmp)
{
    const Image source = generateTestImage (8, 6, PixelFormat::RGB);

    auto* rawStream = new MemoryOutputStream();
    BmpImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (source));

    const auto* bytes = static_cast<const uint8*> (rawStream->getData());
    const auto size = rawStream->getDataSize();

    const auto result = Image::loadFromData (Span<const uint8> (bytes, size));
    ASSERT_TRUE (result.wasOk());

    const Image& decoded = result.getValue();
    EXPECT_EQ (decoded.getWidth(), source.getWidth());
    EXPECT_EQ (decoded.getHeight(), source.getHeight());
    EXPECT_TRUE (imagesAreEqual (source, decoded));
}

TEST (ImageTests, LoadFromDataRoundTripPpm)
{
    const Image source = generateTestImage (8, 6, PixelFormat::RGB);

    auto* rawStream = new MemoryOutputStream();
    PpmImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (source));

    const auto* bytes = static_cast<const uint8*> (rawStream->getData());
    const auto size = rawStream->getDataSize();

    const auto result = Image::loadFromData (Span<const uint8> (bytes, size));
    ASSERT_TRUE (result.wasOk());

    const Image& decoded = result.getValue();
    EXPECT_EQ (decoded.getWidth(), source.getWidth());
    EXPECT_EQ (decoded.getHeight(), source.getHeight());
    EXPECT_TRUE (imagesAreEqual (source, decoded));
}

#if YUP_IMAGE_FORMAT_PNG
TEST (ImageTests, LoadFromDataRoundTripPng)
{
    const Image source = generateTestImage (8, 6, PixelFormat::RGB);

    auto* rawStream = new MemoryOutputStream();
    PngImageFormatWriter writer (rawStream, PixelFormat::RGB);
    ASSERT_TRUE (writer.writeImage (source));

    const auto* bytes = static_cast<const uint8*> (rawStream->getData());
    const auto size = rawStream->getDataSize();

    const auto result = Image::loadFromData (Span<const uint8> (bytes, size));
    ASSERT_TRUE (result.wasOk());

    const Image& decoded = result.getValue();
    EXPECT_EQ (decoded.getWidth(), source.getWidth());
    EXPECT_EQ (decoded.getHeight(), source.getHeight());
    EXPECT_TRUE (imagesAreEqual (source, decoded));
}
#endif

#if YUP_IMAGE_FORMAT_WEBP
TEST (ImageTests, LoadFromDataRoundTripWebP)
{
    const Image source = generateTestImage (8, 6, PixelFormat::RGB);

    auto* rawStream = new MemoryOutputStream();
    WebPImageFormatWriter writer (rawStream, PixelFormat::RGB, 0);
    ASSERT_TRUE (writer.writeImage (source));

    const auto* bytes = static_cast<const uint8*> (rawStream->getData());
    const auto size = rawStream->getDataSize();

    const auto result = Image::loadFromData (Span<const uint8> (bytes, size));
    ASSERT_TRUE (result.wasOk());

    const Image& decoded = result.getValue();
    EXPECT_EQ (decoded.getWidth(), source.getWidth());
    EXPECT_EQ (decoded.getHeight(), source.getHeight());
    EXPECT_TRUE (imagesAreEqual (source, decoded));
}
#endif
