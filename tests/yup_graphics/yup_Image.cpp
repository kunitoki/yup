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

#include <yup_graphics/yup_graphics.h>

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
}

TEST (BitmapDataTests, PixelAccessRejectsOutOfRangeCoordinates)
{
    BitmapData bitmap (2, 2, PixelFormat::RGBA);

    EXPECT_THROW (bitmap.setPixel (-1, 0, 0xffffffff), std::out_of_range);
    EXPECT_THROW (bitmap.setPixel (0, -1, 0xffffffff), std::out_of_range);
    EXPECT_THROW (bitmap.setPixel (2, 0, 0xffffffff), std::out_of_range);
    EXPECT_THROW (bitmap.setPixel (0, 2, 0xffffffff), std::out_of_range);
    EXPECT_THROW (bitmap.getPixel (2, 0), std::out_of_range);
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
