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

#if YUP_MODULE_AVAILABLE_libgif && YUP_IMAGE_FORMAT_GIF

//==============================================================================
// Helpers
//==============================================================================

namespace
{

std::vector<uint8_t> encodeToGif (const Image& image)
{
    auto* rawStream = new MemoryOutputStream();
    GifImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    if (! writer.writeImage (image))
        return {};
    const auto* data = static_cast<const uint8_t*> (rawStream->getData());
    return std::vector<uint8_t> (data, data + rawStream->getDataSize());
}

std::vector<uint8_t> encodeAnimationToGif (const std::vector<Image>& frames,
                                           const std::vector<int>& delaysMs,
                                           int loopCount = 0)
{
    auto* rawStream = new MemoryOutputStream();
    GifImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    if (! writer.beginAnimation (loopCount))
        return {};
    for (size_t i = 0; i < frames.size(); ++i)
        if (! writer.writeFrame (frames[i], delaysMs[i]))
            return {};
    if (! writer.endAnimation())
        return {};
    const auto* data = static_cast<const uint8_t*> (rawStream->getData());
    return std::vector<uint8_t> (data, data + rawStream->getDataSize());
}

Image makeSolidRgba (int w, int h, uint32 argb)
{
    Image img (w, h, PixelFormat::RGBA);
    img.fill (argb);
    return img;
}

} // namespace

// ======================================================================
// Format property tests
// ======================================================================

TEST (GifImageFormatTests, FormatNameIsCorrect)
{
    GifImageFormat fmt;
    EXPECT_EQ (fmt.getFormatName(), "GIF Image");
}

TEST (GifImageFormatTests, ExtensionsAreCorrect)
{
    GifImageFormat fmt;

    auto readExts = fmt.getFileExtensions (ImageFormat::forReading);
    ASSERT_EQ (readExts.size(), 1);
    EXPECT_EQ (readExts[0], String (".gif"));

    auto writeExts = fmt.getFileExtensions (ImageFormat::forWriting);
    ASSERT_EQ (writeExts.size(), 1);
    EXPECT_EQ (writeExts[0], String (".gif"));
}

TEST (GifImageFormatTests, PossiblePixelFormatsIsRgbaOnly)
{
    GifImageFormat fmt;
    auto fmts = fmt.getPossiblePixelFormats();

    ASSERT_EQ (fmts.size(), 1);
    EXPECT_EQ (fmts[0], PixelFormat::RGBA);
}

TEST (GifImageFormatTests, IsCompressed)
{
    GifImageFormat fmt;
    EXPECT_TRUE (fmt.isCompressed());
}

TEST (GifImageFormatTests, HasNoQualityOptions)
{
    GifImageFormat fmt;
    EXPECT_EQ (fmt.getQualityOptions().size(), 0);
}

// ======================================================================
// canHandleFile tests
// ======================================================================

TEST (GifImageFormatTests, CanHandleFileForGifExtension)
{
    GifImageFormat fmt;

    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/image.gif"), ImageFormat::forReading));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/image.GIF"), ImageFormat::forReading));
    EXPECT_TRUE (fmt.canHandleFile (File ("/any/path/image.gif"), ImageFormat::forWriting));
}

TEST (GifImageFormatTests, CanHandleFileRejectsWrongExtensions)
{
    GifImageFormat fmt;

    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image.bmp"), ImageFormat::forReading));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image.png"), ImageFormat::forReading));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image.jpg"), ImageFormat::forReading));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image.webp"), ImageFormat::forReading));
    EXPECT_FALSE (fmt.canHandleFile (File ("/any/path/image"), ImageFormat::forReading));
}

// ======================================================================
// canHandleStream tests
// ======================================================================

TEST (GifImageFormatTests, CanHandleStreamDetectsGifMagic)
{
    auto bytes = encodeToGif (makeSolidRgba (4, 4, 0xFFCC4400u));
    ASSERT_FALSE (bytes.empty());

    auto* inStream = new MemoryInputStream (bytes.data(), bytes.size(), true);
    GifImageFormat fmt;
    EXPECT_TRUE (fmt.canHandleStream (*inStream, ImageFormat::forReading));
    delete inStream;
}

TEST (GifImageFormatTests, CanHandleStreamRejectsPngSignature)
{
    const uint8_t pngSig[] = { 0x89, 'P', 'N', 'G', '\r', '\n' };
    MemoryInputStream stream (pngSig, sizeof (pngSig), false);
    GifImageFormat fmt;
    EXPECT_FALSE (fmt.canHandleStream (stream, ImageFormat::forReading));
    EXPECT_EQ (stream.getPosition(), 0);
}

TEST (GifImageFormatTests, CanHandleStreamRejectsBmpMagic)
{
    const uint8_t bmpSig[] = { 0x42, 0x4D };
    MemoryInputStream stream (bmpSig, sizeof (bmpSig), false);
    GifImageFormat fmt;
    EXPECT_FALSE (fmt.canHandleStream (stream, ImageFormat::forReading));
    EXPECT_EQ (stream.getPosition(), 0);
}

TEST (GifImageFormatTests, CanHandleStreamRejectsEmptyStream)
{
    GifImageFormat fmt;
    MemoryInputStream stream (static_cast<const void*> (nullptr), 0, false);
    EXPECT_FALSE (fmt.canHandleStream (stream, ImageFormat::forReading));
}

// ======================================================================
// Reader dimension and header tests
// ======================================================================

TEST (GifImageFormatTests, ReaderSetsCorrectDimensions)
{
    auto bytes = encodeToGif (makeSolidRgba (7, 5, 0xFF0000FFu));
    ASSERT_FALSE (bytes.empty());

    auto* inStream = new MemoryInputStream (bytes.data(), bytes.size(), true);
    GifImageFormatReader reader (inStream);
    EXPECT_EQ (reader.width, 7);
    EXPECT_EQ (reader.height, 5);
    EXPECT_EQ (reader.pixelFormat, PixelFormat::RGBA);
}

TEST (GifImageFormatTests, ReaderSetsDimensionsForSmallImage)
{
    auto bytes = encodeToGif (makeSolidRgba (1, 1, 0xFFFF0000u));
    ASSERT_FALSE (bytes.empty());

    auto* inStream = new MemoryInputStream (bytes.data(), bytes.size(), true);
    GifImageFormatReader reader (inStream);
    EXPECT_EQ (reader.width, 1);
    EXPECT_EQ (reader.height, 1);
}

TEST (GifImageFormatTests, ReaderSetsDimensionsForNonSquareImage)
{
    auto bytes = encodeToGif (makeSolidRgba (17, 3, 0xFF00FF00u));
    ASSERT_FALSE (bytes.empty());

    auto* inStream = new MemoryInputStream (bytes.data(), bytes.size(), true);
    GifImageFormatReader reader (inStream);
    EXPECT_EQ (reader.width, 17);
    EXPECT_EQ (reader.height, 3);
}

TEST (GifImageFormatTests, InvalidSignatureReturnsInvalidImage)
{
    const uint8 garbage[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
    auto* stream = new MemoryInputStream (garbage, sizeof (garbage), false);

    GifImageFormatReader reader (stream);
    EXPECT_EQ (reader.width, 0);
    EXPECT_EQ (reader.height, 0);

    auto result = reader.readImage();
    EXPECT_FALSE (result.isValid());
}

TEST (GifImageFormatTests, InvalidSignatureSetsFrameCountToZero)
{
    const uint8 garbage[] = { 0xDE, 0xAD, 0xBE, 0xEF };
    auto* stream = new MemoryInputStream (garbage, sizeof (garbage), false);

    GifImageFormatReader reader (stream);
    EXPECT_FALSE (reader.isAnimated());
    EXPECT_EQ (reader.getFrameCount(), 0);
}

// ======================================================================
// Single-frame roundtrip tests (memory-based)
// ======================================================================

TEST (GifImageFormatTests, WriteAndReadBackSolidRedWithinPaletteTolerance)
{
    Image original (16, 16, PixelFormat::RGBA);
    original.fill (0xFFFF0000u);

    auto bytes = encodeToGif (original);
    ASSERT_FALSE (bytes.empty());

    auto* inStream = new MemoryInputStream (bytes.data(), bytes.size(), true);
    GifImageFormatReader reader (inStream);
    auto result = reader.readImage();

    ASSERT_EQ (result.getWidth(), original.getWidth());
    ASSERT_EQ (result.getHeight(), original.getHeight());
    EXPECT_TRUE (imagesAreEqual (original, result, 8));
}

TEST (GifImageFormatTests, WriteAndReadBackSolidGreenWithinPaletteTolerance)
{
    Image original (12, 12, PixelFormat::RGBA);
    original.fill (0xFF00FF00u);

    auto bytes = encodeToGif (original);
    ASSERT_FALSE (bytes.empty());

    auto* inStream = new MemoryInputStream (bytes.data(), bytes.size(), true);
    GifImageFormatReader reader (inStream);
    auto result = reader.readImage();

    ASSERT_TRUE (result.isValid());
    EXPECT_TRUE (imagesAreEqual (original, result, 8));
}

TEST (GifImageFormatTests, WriteAndReadBackSolidBlueWithinPaletteTolerance)
{
    Image original (10, 10, PixelFormat::RGBA);
    original.fill (0xFF0000FFu);

    auto bytes = encodeToGif (original);
    ASSERT_FALSE (bytes.empty());

    auto* inStream = new MemoryInputStream (bytes.data(), bytes.size(), true);
    GifImageFormatReader reader (inStream);
    auto result = reader.readImage();

    ASSERT_TRUE (result.isValid());
    EXPECT_TRUE (imagesAreEqual (original, result, 8));
}

TEST (GifImageFormatTests, WriteAndReadBackCheckerboardWithinPaletteTolerance)
{
    auto original = generateTestImage (16, 16, PixelFormat::RGBA);

    auto bytes = encodeToGif (original);
    ASSERT_FALSE (bytes.empty());

    auto* inStream = new MemoryInputStream (bytes.data(), bytes.size(), true);
    GifImageFormatReader reader (inStream);
    auto result = reader.readImage();

    ASSERT_EQ (result.getWidth(), original.getWidth());
    ASSERT_EQ (result.getHeight(), original.getHeight());
    EXPECT_TRUE (imagesAreEqual (original, result, 20));
}

TEST (GifImageFormatTests, VariousSizesRoundtrip)
{
    const int sizes[][2] = { { 1, 1 }, { 4, 4 }, { 7, 13 }, { 32, 1 }, { 1, 32 } };

    for (auto [w, h] : sizes)
    {
        Image original (w, h, PixelFormat::RGBA);
        original.fill (0xFFCC8844u);

        auto bytes = encodeToGif (original);
        ASSERT_FALSE (bytes.empty())
            << "Failed to encode " << w << "x" << h;

        auto* inStream = new MemoryInputStream (bytes.data(), bytes.size(), true);
        GifImageFormatReader reader (inStream);
        auto result = reader.readImage();

        ASSERT_TRUE (result.isValid())
            << "Invalid result for " << w << "x" << h;
        EXPECT_EQ (result.getWidth(), w);
        EXPECT_EQ (result.getHeight(), h);
    }
}

TEST (GifImageFormatTests, SingleFrameGifIsNotAnimated)
{
    auto bytes = encodeToGif (makeSolidRgba (4, 4, 0xFFAAAAAAu));
    ASSERT_FALSE (bytes.empty());

    auto* inStream = new MemoryInputStream (bytes.data(), bytes.size(), true);
    GifImageFormatReader reader (inStream);

    EXPECT_FALSE (reader.isAnimated());
    EXPECT_EQ (reader.getFrameCount(), 1);
    EXPECT_EQ (reader.getLoopCount(), 1);
}

// ======================================================================
// File-based roundtrip tests (temporary files)
// ======================================================================

TEST (GifImageFormatTests, FileRoundtripSingleFrameViaTempFile)
{
    Image original (16, 12, PixelFormat::RGBA);
    original.fill (0xFFAA7733u);
    auto tempFile = File::createTempFile (".gif");

    {
        auto* fos = tempFile.createOutputStream().release();
        GifImageFormatWriter writer (fos, PixelFormat::RGBA);
        ASSERT_TRUE (writer.writeImage (original));
    }

    {
        auto* fis = tempFile.createInputStream().release();
        GifImageFormatReader reader (fis);
        auto result = reader.readImage();

        ASSERT_TRUE (result.isValid());
        EXPECT_EQ (result.getWidth(), original.getWidth());
        EXPECT_EQ (result.getHeight(), original.getHeight());
        EXPECT_TRUE (imagesAreEqual (original, result, 8));
    }

    tempFile.deleteFile();
}

TEST (GifImageFormatTests, FileRoundtripAnimationViaTempFile)
{
    Image red (8, 8, PixelFormat::RGBA);
    red.fill (0xFFFF0000u);
    Image green (8, 8, PixelFormat::RGBA);
    green.fill (0xFF00FF00u);
    auto tempFile = File::createTempFile (".gif");

    {
        auto* fos = tempFile.createOutputStream().release();
        GifImageFormatWriter writer (fos, PixelFormat::RGBA);
        ASSERT_TRUE (writer.beginAnimation (0));
        ASSERT_TRUE (writer.writeFrame (red, 100));
        ASSERT_TRUE (writer.writeFrame (green, 200));
        ASSERT_TRUE (writer.endAnimation());
    }

    {
        auto* fis = tempFile.createInputStream().release();
        GifImageFormatReader reader (fis);

        ASSERT_TRUE (reader.isAnimated());
        EXPECT_EQ (reader.getFrameCount(), 2);
        EXPECT_EQ (reader.getLoopCount(), 0);
        EXPECT_EQ (reader.getFrameDelayMs (0), 100);
        EXPECT_EQ (reader.getFrameDelayMs (1), 200);

        auto frame0 = reader.readFrame (0);
        auto frame1 = reader.readFrame (1);
        ASSERT_TRUE (frame0.isValid());
        ASSERT_TRUE (frame1.isValid());
        EXPECT_TRUE (imagesAreEqual (red, frame0, 8));
        EXPECT_TRUE (imagesAreEqual (green, frame1, 8));
    }

    tempFile.deleteFile();
}

TEST (GifImageFormatTests, FileRoundtripSolidColorViaTempFile)
{
    auto original = makeSolidRgba (24, 10, 0xFF336699u);
    auto tempFile = File::createTempFile (".gif");

    {
        auto* fos = tempFile.createOutputStream().release();
        GifImageFormatWriter writer (fos, PixelFormat::RGBA);
        ASSERT_TRUE (writer.writeImage (original));
    }

    {
        auto* fis = tempFile.createInputStream().release();
        GifImageFormatReader reader (fis);
        auto result = reader.readImage();
        ASSERT_TRUE (imagesAreEqual (original, result, 8));
    }

    tempFile.deleteFile();
}

TEST (GifImageFormatTests, SavedFileHasExpectedMinSize)
{
    auto original = makeSolidRgba (4, 4, 0xFFFF0000u);
    auto tempFile = File::createTempFile (".gif");

    {
        auto* fos = tempFile.createOutputStream().release();
        GifImageFormatWriter writer (fos, PixelFormat::RGBA);
        ASSERT_TRUE (writer.writeImage (original));
    }

    EXPECT_GT (tempFile.getSize(), 0);
    tempFile.deleteFile();
}

// ======================================================================
// Image::loadFromData tests
// ======================================================================

TEST (GifImageFormatTests, LoadFromDataRoundtripSingleFrame)
{
    Image original (12, 12, PixelFormat::RGBA);
    original.fill (0xFFBB6644u);

    auto bytes = encodeToGif (original);
    ASSERT_FALSE (bytes.empty());

    auto result = Image::loadFromData (Span<const uint8> (bytes));
    ASSERT_TRUE (result.wasOk());

    const Image& decoded = result.getValue();
    EXPECT_EQ (decoded.getWidth(), original.getWidth());
    EXPECT_EQ (decoded.getHeight(), original.getHeight());
    EXPECT_TRUE (imagesAreEqual (original, decoded, 8));
}

TEST (GifImageFormatTests, LoadFromDataFailsForInvalidGif)
{
    const uint8 bad[] = { 0x47, 0x49, 0x46, 0x38 }; // GIF8 truncated
    auto result = Image::loadFromData (Span<const uint8> (bad, sizeof (bad)));
    EXPECT_FALSE (result.wasOk());
}

// ======================================================================
// Animation tests
// ======================================================================

TEST (GifImageFormatTests, ThreeFrameAnimationRoundTripMetadata)
{
    Image red (8, 8, PixelFormat::RGBA);
    red.fill (0xFFFF0000u);
    Image green (8, 8, PixelFormat::RGBA);
    green.fill (0xFF00FF00u);
    Image blue (8, 8, PixelFormat::RGBA);
    blue.fill (0xFF0000FFu);

    const std::vector<Image> frames = { red, green, blue };
    const std::vector<int> delays = { 100, 200, 300 };

    auto bytes = encodeAnimationToGif (frames, delays, 0);
    ASSERT_FALSE (bytes.empty());

    auto* inStream = new MemoryInputStream (bytes.data(), bytes.size(), true);
    GifImageFormatReader reader (inStream);

    EXPECT_TRUE (reader.isAnimated());
    EXPECT_EQ (reader.getFrameCount(), 3);
    EXPECT_EQ (reader.getLoopCount(), 0);
    EXPECT_EQ (reader.getFrameDelayMs (0), 100);
    EXPECT_EQ (reader.getFrameDelayMs (1), 200);
    EXPECT_EQ (reader.getFrameDelayMs (2), 300);
}

TEST (GifImageFormatTests, FrameDominantColorMatchesSourceWithinTolerance)
{
    Image red (8, 8, PixelFormat::RGBA);
    red.fill (0xFFFF0000u);
    Image green (8, 8, PixelFormat::RGBA);
    green.fill (0xFF00FF00u);
    Image blue (8, 8, PixelFormat::RGBA);
    blue.fill (0xFF0000FFu);

    const std::vector<Image> frames = { red, green, blue };
    const std::vector<int> delays = { 100, 200, 300 };

    auto bytes = encodeAnimationToGif (frames, delays);
    ASSERT_FALSE (bytes.empty());

    auto* inStream = new MemoryInputStream (bytes.data(), bytes.size(), true);
    GifImageFormatReader reader (inStream);

    ASSERT_EQ (reader.getFrameCount(), 3);

    const std::vector<Image> sources = { red, green, blue };
    for (int fi = 0; fi < 3; ++fi)
    {
        auto decoded = reader.readFrame (fi);
        ASSERT_TRUE (decoded.isValid()) << "Frame " << fi << " is invalid";
        EXPECT_TRUE (imagesAreEqual (sources[static_cast<size_t> (fi)], decoded, 64))
            << "Frame " << fi << " dominant color mismatch";
    }
}

TEST (GifImageFormatTests, SequentialAndRandomAccessProduceSamePixels)
{
    Image red (8, 8, PixelFormat::RGBA);
    red.fill (0xFFFF0000u);
    Image green (8, 8, PixelFormat::RGBA);
    green.fill (0xFF00FF00u);
    Image blue (8, 8, PixelFormat::RGBA);
    blue.fill (0xFF0000FFu);

    const std::vector<Image> frames = { red, green, blue };
    const std::vector<int> delays = { 100, 200, 300 };
    auto bytes = encodeAnimationToGif (frames, delays);
    ASSERT_FALSE (bytes.empty());

    auto* streamA = new MemoryInputStream (bytes.data(), bytes.size(), true);
    GifImageFormatReader seqReader (streamA);
    Image seq0 = seqReader.readFrame (0);
    Image seq1 = seqReader.readFrame (1);
    Image seq2 = seqReader.readFrame (2);

    auto* streamB = new MemoryInputStream (bytes.data(), bytes.size(), true);
    GifImageFormatReader randReader (streamB);
    Image rand2 = randReader.readFrame (2);
    Image rand0 = randReader.readFrame (0);
    Image rand1 = randReader.readFrame (1);

    ASSERT_TRUE (seq0.isValid());
    ASSERT_TRUE (rand0.isValid());
    ASSERT_TRUE (seq1.isValid());
    ASSERT_TRUE (rand1.isValid());
    ASSERT_TRUE (seq2.isValid());
    ASSERT_TRUE (rand2.isValid());

    EXPECT_TRUE (imagesAreEqual (seq0, rand0, 0));
    EXPECT_TRUE (imagesAreEqual (seq1, rand1, 0));
    EXPECT_TRUE (imagesAreEqual (seq2, rand2, 0));
}

TEST (GifImageFormatTests, RandomAccessBackwardRecompositesCorrectly)
{
    Image red (8, 8, PixelFormat::RGBA);
    red.fill (0xFFFF0000u);
    Image green (8, 8, PixelFormat::RGBA);
    green.fill (0xFF00FF00u);
    Image blue (8, 8, PixelFormat::RGBA);
    blue.fill (0xFF0000FFu);
    Image white (8, 8, PixelFormat::RGBA);
    white.fill (0xFFFFFFFFu);

    const std::vector<Image> frames = { red, green, blue, white };
    const std::vector<int> delays = { 100, 100, 100, 100 };
    auto bytes = encodeAnimationToGif (frames, delays);
    ASSERT_FALSE (bytes.empty());

    auto* stream = new MemoryInputStream (bytes.data(), bytes.size(), true);
    GifImageFormatReader reader (stream);

    auto f3 = reader.readFrame (3);
    auto f0 = reader.readFrame (0); // backward seek triggers re-composite
    auto f2 = reader.readFrame (2); // forward again

    ASSERT_TRUE (f0.isValid());
    ASSERT_TRUE (f2.isValid());
    ASSERT_TRUE (f3.isValid());

    EXPECT_TRUE (imagesAreEqual (red, f0, 8));
    EXPECT_TRUE (imagesAreEqual (blue, f2, 8));
    EXPECT_TRUE (imagesAreEqual (white, f3, 8));
}

TEST (GifImageFormatTests, ReadFrameOutOfRangeReturnsInvalid)
{
    Image red (8, 8, PixelFormat::RGBA);
    red.fill (0xFFFF0000u);

    auto bytes = encodeToGif (red);
    ASSERT_FALSE (bytes.empty());

    auto* inStream = new MemoryInputStream (bytes.data(), bytes.size(), true);
    GifImageFormatReader reader (inStream);

    EXPECT_FALSE (reader.readFrame (-1).isValid());
    EXPECT_FALSE (reader.readFrame (1).isValid());
    EXPECT_FALSE (reader.readFrame (100).isValid());
}

// ======================================================================
// Buffer reuse tests (readFrame with dest)
// ======================================================================

TEST (GifImageFormatTests, ReadFrameReusesBufferWhenDimensionsMatch)
{
    Image red (8, 8, PixelFormat::RGBA);
    red.fill (0xFFFF0000u);
    Image green (8, 8, PixelFormat::RGBA);
    green.fill (0xFF00FF00u);

    const std::vector<Image> frames = { red, green };
    const std::vector<int> delays = { 100, 200 };
    auto bytes = encodeAnimationToGif (frames, delays);
    ASSERT_FALSE (bytes.empty());

    auto* inStream = new MemoryInputStream (bytes.data(), bytes.size(), true);
    GifImageFormatReader reader (inStream);

    Image dest (8, 8, PixelFormat::RGBA);
    const auto* ptrBefore = dest.getRawData().data();

    ASSERT_TRUE (reader.readFrame (0, dest));
    const auto* ptrAfter = dest.getRawData().data();

    EXPECT_EQ (ptrBefore, ptrAfter);
}

TEST (GifImageFormatTests, ReadFrameReallocatesWhenDimensionsDiffer)
{
    Image large (16, 16, PixelFormat::RGBA);
    large.fill (0xFFFF0000u);
    Image small (4, 4, PixelFormat::RGBA);
    small.fill (0xFF00FF00u);

    const std::vector<Image> frames = { large, small };
    const std::vector<int> delays = { 100, 200 };
    auto bytes = encodeAnimationToGif (frames, delays);
    ASSERT_FALSE (bytes.empty());

    auto* inStream = new MemoryInputStream (bytes.data(), bytes.size(), true);
    GifImageFormatReader reader (inStream);

    Image dest (4, 4, PixelFormat::RGBA); // smaller than frame 0
    ASSERT_TRUE (reader.readFrame (0, dest));

    EXPECT_EQ (dest.getWidth(), 16);
    EXPECT_EQ (dest.getHeight(), 16);
}

// ======================================================================
// Loop count tests
// ======================================================================

TEST (GifImageFormatTests, InfiniteLoopCountRoundTrips)
{
    Image src (4, 4, PixelFormat::RGBA);
    src.fill (0xFF123456u);
    auto bytes = encodeAnimationToGif ({ src }, { 100 }, 0);
    ASSERT_FALSE (bytes.empty());

    auto* inStream = new MemoryInputStream (bytes.data(), bytes.size(), true);
    GifImageFormatReader reader (inStream);
    EXPECT_EQ (reader.getLoopCount(), 0);
}

TEST (GifImageFormatTests, CustomLoopCountRoundTrips)
{
    const int loops[] = { 1, 3, 5, 10 };

    for (int lc : loops)
    {
        Image src (4, 4, PixelFormat::RGBA);
        src.fill (0xFF654321u);
        auto bytes = encodeAnimationToGif ({ src }, { 100 }, lc);
        ASSERT_FALSE (bytes.empty())
            << "Failed for loopCount " << lc;

        auto* inStream = new MemoryInputStream (bytes.data(), bytes.size(), true);
        GifImageFormatReader reader (inStream);
        EXPECT_EQ (reader.getLoopCount(), lc)
            << "Loop count mismatch for " << lc;
    }
}

TEST (GifImageFormatTests, SingleFrameLoopCountDefaultsToOne)
{
    auto bytes = encodeToGif (makeSolidRgba (4, 4, 0xFF998877u));
    ASSERT_FALSE (bytes.empty());

    auto* inStream = new MemoryInputStream (bytes.data(), bytes.size(), true);
    GifImageFormatReader reader (inStream);
    EXPECT_FALSE (reader.isAnimated());
    EXPECT_EQ (reader.getLoopCount(), 1);
}

// ======================================================================
// Frame delay tests
// ======================================================================

TEST (GifImageFormatTests, VariousFrameDelaysRoundtrip)
{
    Image src (8, 8, PixelFormat::RGBA);
    src.fill (0xFFFF0000u);

    const std::vector<int> delays = { 0, 50, 100, 250, 1000, 5000 };

    for (auto delay : delays)
    {
        auto bytes = encodeAnimationToGif ({ src }, { delay }, 0);
        ASSERT_FALSE (bytes.empty())
            << "Failed for delay " << delay;

        auto* inStream = new MemoryInputStream (bytes.data(), bytes.size(), true);
        GifImageFormatReader reader (inStream);

        int actualDelay = reader.getFrameDelayMs (0);
        int error = std::abs (actualDelay - delay);

        EXPECT_LE (error, 10)
            << "Delay " << delay << "ms: got " << actualDelay << "ms (error " << error << "ms)";
    }
}

// ======================================================================
// Multi-frame varying dimension tests
// ======================================================================

TEST (GifImageFormatTests, FramesWithDifferentSizesEncodeAndDecode)
{
    Image large (16, 16, PixelFormat::RGBA);
    large.fill (0xFFFF0000u);
    Image medium (8, 8, PixelFormat::RGBA);
    medium.fill (0xFF00FF00u);
    Image small (4, 4, PixelFormat::RGBA);
    small.fill (0xFF0000FFu);

    const std::vector<Image> frames = { large, medium, small };
    const std::vector<int> delays = { 100, 200, 300 };
    auto bytes = encodeAnimationToGif (frames, delays);
    ASSERT_FALSE (bytes.empty());

    auto* inStream = new MemoryInputStream (bytes.data(), bytes.size(), true);
    GifImageFormatReader reader (inStream);

    ASSERT_EQ (reader.getFrameCount(), 3);

    auto f0 = reader.readFrame (0);
    auto f1 = reader.readFrame (1);
    auto f2 = reader.readFrame (2);

    ASSERT_TRUE (f0.isValid());
    ASSERT_TRUE (f1.isValid());
    ASSERT_TRUE (f2.isValid());

    EXPECT_EQ (f0.getWidth(), 16);
    EXPECT_EQ (f0.getHeight(), 16);
    EXPECT_EQ (f1.getWidth(), 16);
    EXPECT_EQ (f1.getHeight(), 16);
    EXPECT_EQ (f2.getWidth(), 16);
    EXPECT_EQ (f2.getHeight(), 16);

    const auto expectPixelNear = [] (uint32 actual, uint32 expected, int tolerance)
    {
        EXPECT_EQ ((actual >> 24) & 0xFF, (expected >> 24) & 0xFF);
        EXPECT_LE (std::abs (int ((actual >> 16) & 0xFF) - int ((expected >> 16) & 0xFF)), tolerance);
        EXPECT_LE (std::abs (int ((actual >> 8) & 0xFF) - int ((expected >> 8) & 0xFF)), tolerance);
        EXPECT_LE (std::abs (int ((actual >> 0) & 0xFF) - int ((expected >> 0) & 0xFF)), tolerance);
    };

    expectPixelNear (f1.getPixel (0, 0), 0xFF00FF00u, 8);
    EXPECT_EQ (f1.getPixel (8, 8), 0x00000000u);
    expectPixelNear (f2.getPixel (0, 0), 0xFF0000FFu, 8);
    EXPECT_EQ (f2.getPixel (4, 4), 0x00000000u);
}

// ======================================================================
// Edge case tests
// ======================================================================

TEST (GifImageFormatTests, VerySmallImageRoundtrip)
{
    Image original (1, 1, PixelFormat::RGBA);
    original.fill (0xFF112233u);

    auto bytes = encodeToGif (original);
    ASSERT_FALSE (bytes.empty());

    auto* inStream = new MemoryInputStream (bytes.data(), bytes.size(), true);
    GifImageFormatReader reader (inStream);
    auto result = reader.readImage();

    ASSERT_TRUE (result.isValid());
    EXPECT_EQ (result.getWidth(), 1);
    EXPECT_EQ (result.getHeight(), 1);
    EXPECT_TRUE (imagesAreEqual (original, result, 8));
}

TEST (GifImageFormatTests, FullyTransparentFrameRoundtrip)
{
    Image original (8, 8, PixelFormat::RGBA);
    original.fill (0x00000000u);

    auto bytes = encodeToGif (original);
    ASSERT_FALSE (bytes.empty());

    auto* inStream = new MemoryInputStream (bytes.data(), bytes.size(), true);
    GifImageFormatReader reader (inStream);
    auto result = reader.readImage();

    ASSERT_TRUE (result.isValid());
    EXPECT_EQ (result.getWidth(), original.getWidth());
    EXPECT_EQ (result.getHeight(), original.getHeight());
    EXPECT_TRUE (imagesAreEqualRGBA (original, result, 8));
}

TEST (GifImageFormatTests, SemiTransparentFrameBecomesOpaque)
{
    Image original (8, 8, PixelFormat::RGBA);
    original.fill (0x80FF8844u);

    auto bytes = encodeToGif (original);
    ASSERT_FALSE (bytes.empty());

    auto* inStream = new MemoryInputStream (bytes.data(), bytes.size(), true);
    GifImageFormatReader reader (inStream);
    auto result = reader.readImage();

    ASSERT_TRUE (result.isValid());
    EXPECT_TRUE (imagesAreEqual (original, result, 30));
    EXPECT_EQ ((result.getPixel (0, 0) >> 24) & 0xFF, 0xFFu);
}

TEST (GifImageFormatTests, LargeFrameCountAnimationRoundtrip)
{
    const int frameCount = 20;
    std::vector<Image> frames;
    std::vector<int> delays;

    for (int i = 0; i < frameCount; ++i)
    {
        Image frame (4, 4, PixelFormat::RGBA);
        uint32 color = 0xFF000000u | (static_cast<uint32> (i * 12) << 16) | (static_cast<uint32> ((frameCount - i) * 12) << 8);
        frame.fill (color);
        frames.push_back (frame);
        delays.push_back (i * 10 + 10);
    }

    auto bytes = encodeAnimationToGif (frames, delays, 2);
    ASSERT_FALSE (bytes.empty());

    auto* inStream = new MemoryInputStream (bytes.data(), bytes.size(), true);
    GifImageFormatReader reader (inStream);

    EXPECT_TRUE (reader.isAnimated());
    EXPECT_EQ (reader.getFrameCount(), frameCount);
    EXPECT_EQ (reader.getLoopCount(), 2);

    for (int i = 0; i < frameCount; ++i)
    {
        auto decoded = reader.readFrame (i);
        ASSERT_TRUE (decoded.isValid()) << "Frame " << i << " is invalid";
        EXPECT_EQ (reader.getFrameDelayMs (i), delays[static_cast<size_t> (i)]);
    }
}

TEST (GifImageFormatTests, ZeroDelayFrameRoundtrip)
{
    Image src (4, 4, PixelFormat::RGBA);
    src.fill (0xFFCC8844u);

    auto bytes = encodeAnimationToGif ({ src }, { 0 }, 0);
    ASSERT_FALSE (bytes.empty());

    auto* inStream = new MemoryInputStream (bytes.data(), bytes.size(), true);
    GifImageFormatReader reader (inStream);

    EXPECT_FALSE (reader.isAnimated());
    EXPECT_EQ (reader.getFrameCount(), 1);
    EXPECT_EQ (reader.getFrameDelayMs (0), 0);
}

// ======================================================================
// Manager integration tests
// ======================================================================

TEST (GifImageFormatTests, ManagerCreatesReaderForStream)
{
    auto* rawStream = new MemoryOutputStream();
    GifImageFormatWriter writer (rawStream, PixelFormat::RGBA);
    ASSERT_TRUE (writer.writeImage (generateTestImage (4, 4, PixelFormat::RGBA)));

    auto* inStream = new MemoryInputStream (rawStream->getData(), rawStream->getDataSize(), true);

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto reader = manager.createReaderFor (inStream);
    ASSERT_NE (reader, nullptr);
    EXPECT_EQ (reader->getFormatName(), String ("GIF Image"));
    EXPECT_EQ (reader->width, 4);
    EXPECT_EQ (reader->height, 4);
}

TEST (GifImageFormatTests, ManagerCreatesWriterForGifExtension)
{
    auto tempFile = File::createTempFile (".gif");

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto writer = manager.createWriterFor (tempFile, PixelFormat::RGBA);
    ASSERT_NE (writer, nullptr);
    EXPECT_EQ (writer->getFormatName(), String ("GIF Image"));
    EXPECT_EQ (writer->getPixelFormat(), PixelFormat::RGBA);

    tempFile.deleteFile();
}

TEST (GifImageFormatTests, ManagerRoundtripSingleFrameViaFile)
{
    Image original (10, 8, PixelFormat::RGBA);
    original.fill (0xFF4488AAu);
    auto tempFile = File::createTempFile (".gif");

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
        EXPECT_TRUE (imagesAreEqual (original, result, 8));
    }

    tempFile.deleteFile();
}

TEST (GifImageFormatTests, ManagerRoundtripAnimationViaFile)
{
    Image red (8, 8, PixelFormat::RGBA);
    red.fill (0xFFFF0000u);
    Image blue (8, 8, PixelFormat::RGBA);
    blue.fill (0xFF0000FFu);
    auto tempFile = File::createTempFile (".gif");

    {
        auto* fos = tempFile.createOutputStream().release();
        GifImageFormatWriter writer (fos, PixelFormat::RGBA);
        ASSERT_TRUE (writer.beginAnimation (3));
        ASSERT_TRUE (writer.writeFrame (red, 150));
        ASSERT_TRUE (writer.writeFrame (blue, 250));
        ASSERT_TRUE (writer.endAnimation());
    }

    {
        ImageFormatManager manager;
        manager.registerDefaultFormats();
        auto reader = manager.createReaderFor (tempFile);

        ASSERT_NE (reader, nullptr);
        EXPECT_TRUE (reader->isAnimated());
        EXPECT_EQ (reader->getFrameCount(), 2);
        EXPECT_EQ (reader->getLoopCount(), 3);
    }

    tempFile.deleteFile();
}

// ======================================================================
// Writer metadata tests
// ======================================================================

TEST (GifImageFormatTests, WriterSupportsAnimation)
{
    GifImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGBA);
    EXPECT_TRUE (writer.supportsAnimation());
}

TEST (GifImageFormatTests, WriterBeginEndAnimationWithoutFrames)
{
    auto* rawStream = new MemoryOutputStream();
    GifImageFormatWriter writer (rawStream, PixelFormat::RGBA);

    EXPECT_TRUE (writer.beginAnimation (0));
    EXPECT_TRUE (writer.endAnimation());
}

TEST (GifImageFormatTests, WriterFlushReturnsTrue)
{
    GifImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGBA);
    EXPECT_TRUE (writer.flush());
}

TEST (GifImageFormatTests, WriterReturnsCorrectPixelFormat)
{
    GifImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGBA);
    EXPECT_EQ (writer.getPixelFormat(), PixelFormat::RGBA);
}

TEST (GifImageFormatTests, WriterReturnsCorrectFormatName)
{
    GifImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGBA);
    EXPECT_EQ (writer.getFormatName(), String ("GIF Image"));
}

// ======================================================================
// Animation writer error handling
// ======================================================================

TEST (GifImageFormatTests, WriteFrameWithoutBeginAnimationReturnsFalse)
{
    GifImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGBA);
    Image frame (4, 4, PixelFormat::RGBA);
    frame.fill (0xFFFF0000u);
    EXPECT_FALSE (writer.writeFrame (frame, 100));
}

TEST (GifImageFormatTests, EndAnimationWithoutBeginReturnsFalse)
{
    GifImageFormatWriter writer (new MemoryOutputStream(), PixelFormat::RGBA);
    EXPECT_FALSE (writer.endAnimation());
}

// ======================================================================
// Selective registration tests
// ======================================================================

TEST (GifImageFormatTests, SelectiveRegistrationGifOnly)
{
    ImageFormatManager manager;
    manager.registerDefaultFormats (ImageFormatType::gif);

    auto gifFile = File::createTempFile (".gif");
    auto bmpFile = File::createTempFile (".bmp");

    EXPECT_NE (manager.createWriterFor (gifFile, PixelFormat::RGBA), nullptr);
    EXPECT_EQ (manager.createWriterFor (bmpFile, PixelFormat::RGB), nullptr);

    gifFile.deleteFile();
    bmpFile.deleteFile();
}

// ======================================================================
// Reader metadata tests
// ======================================================================

TEST (GifImageFormatTests, ReaderGetFormatNameIsCorrect)
{
    auto bytes = encodeToGif (makeSolidRgba (4, 4, 0xFF000000u));
    ASSERT_FALSE (bytes.empty());

    auto* inStream = new MemoryInputStream (bytes.data(), bytes.size(), true);
    GifImageFormatReader reader (inStream);
    EXPECT_EQ (reader.getFormatName(), String ("GIF Image"));
}

TEST (GifImageFormatTests, ReaderDpiDefaultsToZero)
{
    auto bytes = encodeToGif (makeSolidRgba (4, 4, 0xFF000000u));
    ASSERT_FALSE (bytes.empty());

    auto* inStream = new MemoryInputStream (bytes.data(), bytes.size(), true);
    GifImageFormatReader reader (inStream);

    EXPECT_EQ (reader.dpiX, 0.0);
    EXPECT_EQ (reader.dpiY, 0.0);
}

#endif // YUP_MODULE_AVAILABLE_libgif && YUP_IMAGE_FORMAT_GIF
