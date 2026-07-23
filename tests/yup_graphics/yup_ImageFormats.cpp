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

namespace
{

//==============================================================================
// Helper: load a file from disk
//==============================================================================
static std::unique_ptr<InputStream> loadTestFile (const char* relativePath)
{
    // Try source-relative path first (works on desktop builds)
    auto file = File (__FILE__)
                    .getParentDirectory()
                    .getParentDirectory()
                    .getChildFile ("data")
                    .getChildFile (relativePath);

    if (file.existsAsFile())
        return file.createInputStream();

    file = File::getCurrentWorkingDirectory()
               .getParentDirectory()
               .getParentDirectory()
               .getParentDirectory()
               .getChildFile ("tests")
               .getChildFile ("data")
               .getChildFile (relativePath);

    if (file.existsAsFile())
        return file.createInputStream();

    return File ("/data")
        .getChildFile (relativePath)
        .createInputStream();
}

//==============================================================================
// Helper: create a simple animated Image sequence
//==============================================================================
static std::vector<Image> createTestFrames (int canvasW, int canvasH, int frameCount)
{
    std::vector<Image> frames;
    frames.reserve (static_cast<size_t> (frameCount));

    for (int fi = 0; fi < frameCount; ++fi)
    {
        Image frame (canvasW, canvasH, PixelFormat::RGBA);
        frame.fill (0x00000000u);

        // Draw a colored square that moves across frames
        const int x = fi * (canvasW - 20) / std::max (frameCount - 1, 1);
        const int y = canvasH / 2 - 10;
        const uint32_t color = 0xFF0000FFu | (static_cast<uint32_t> (fi * 50) << 8);

        for (int row = y; row < y + 20 && row < canvasH; ++row)
            for (int col = x; col < x + 20 && col < canvasW; ++col)
                frame.setPixel (col, row, color);

        frames.push_back (std::move (frame));
    }

    return frames;
}

} // namespace

//==============================================================================
// Static WebP read tests
//==============================================================================

TEST (WebPImageFormatTests, StaticImage_ReadsCorrectDimensions)
{
    auto stream = loadTestFile ("images/file_example.webp");
    ASSERT_NE (stream, nullptr);

    WebPImageFormat format;
    auto reader = format.createReaderFor (stream.release());
    ASSERT_NE (reader, nullptr);

    EXPECT_GT (reader->width, 0);
    EXPECT_GT (reader->height, 0);
    EXPECT_FALSE (reader->isAnimated());
    EXPECT_EQ (reader->getFrameCount(), 0); // 0 for non-animated (frames vector empty)
}

TEST (WebPImageFormatTests, StaticImage_ReadImageSucceeds)
{
    auto stream = loadTestFile ("images/file_example.webp");
    ASSERT_NE (stream, nullptr);

    WebPImageFormat format;
    auto reader = format.createReaderFor (stream.release());
    ASSERT_NE (reader, nullptr);

    auto image = reader->readImage();
    EXPECT_TRUE (image.isValid());
    EXPECT_EQ (image.getWidth(), reader->width);
    EXPECT_EQ (image.getHeight(), reader->height);
}

//==============================================================================
// Animated WebP read tests
//==============================================================================

TEST (WebPImageFormatTests, AnimatedImage_DetectsAnimation)
{
    auto stream = loadTestFile ("images/animation.webp");
    ASSERT_NE (stream, nullptr);

    WebPImageFormat format;
    auto reader = format.createReaderFor (stream.release());
    ASSERT_NE (reader, nullptr);

    EXPECT_TRUE (reader->isAnimated());
    EXPECT_GT (reader->getFrameCount(), 1);
}

TEST (WebPImageFormatTests, AnimatedImage_HasReasonableFrameDelays)
{
    auto stream = loadTestFile ("images/animation.webp");
    ASSERT_NE (stream, nullptr);

    WebPImageFormat format;
    auto reader = format.createReaderFor (stream.release());
    ASSERT_NE (reader, nullptr);
    ASSERT_TRUE (reader->isAnimated());

    const int frameCount = reader->getFrameCount();

    for (int fi = 0; fi < frameCount; ++fi)
    {
        const int delay = reader->getFrameDelayMs (fi);
        EXPECT_GE (delay, 0) << "Frame " << fi << " has negative delay";
        EXPECT_LT (delay, 60000) << "Frame " << fi << " has unreasonably large delay";
    }
}

TEST (WebPImageFormatTests, AnimatedImage_ReadsAllFrames)
{
    auto stream = loadTestFile ("images/animation.webp");
    ASSERT_NE (stream, nullptr);

    WebPImageFormat format;
    auto reader = format.createReaderFor (stream.release());
    ASSERT_NE (reader, nullptr);
    ASSERT_TRUE (reader->isAnimated());

    const int frameCount = reader->getFrameCount();
    const int w = reader->width;
    const int h = reader->height;

    for (int fi = 0; fi < frameCount; ++fi)
    {
        auto frame = reader->readFrame (fi);
        EXPECT_TRUE (frame.isValid()) << "Frame " << fi << " failed to decode";
        EXPECT_EQ (frame.getWidth(), w) << "Frame " << fi << " has wrong width";
        EXPECT_EQ (frame.getHeight(), h) << "Frame " << fi << " has wrong height";
    }
}

TEST (WebPImageFormatTests, AnimatedImage_ReadImageReturnsFrame0)
{
    auto stream = loadTestFile ("images/animation.webp");
    ASSERT_NE (stream, nullptr);

    WebPImageFormat format;
    auto reader = format.createReaderFor (stream.release());
    ASSERT_NE (reader, nullptr);
    ASSERT_TRUE (reader->isAnimated());

    auto image = reader->readImage();
    auto frame0 = reader->readFrame (0);

    EXPECT_TRUE (image.isValid());
    EXPECT_TRUE (frame0.isValid());
    EXPECT_EQ (image.getWidth(), frame0.getWidth());
    EXPECT_EQ (image.getHeight(), frame0.getHeight());
}

TEST (WebPImageFormatTests, AnimatedImage_SequentialAccessOptimized)
{
    auto stream = loadTestFile ("images/animation.webp");
    ASSERT_NE (stream, nullptr);

    WebPImageFormat format;
    auto reader = format.createReaderFor (stream.release());
    ASSERT_NE (reader, nullptr);
    ASSERT_TRUE (reader->isAnimated());

    const int frameCount = reader->getFrameCount();

    // Sequential access should work
    for (int fi = 0; fi < frameCount; ++fi)
    {
        auto frame = reader->readFrame (fi);
        EXPECT_TRUE (frame.isValid()) << "Sequential frame " << fi;
    }
}

TEST (WebPImageFormatTests, AnimatedImage_RandomAccessWorks)
{
    auto stream = loadTestFile ("images/animation.webp");
    ASSERT_NE (stream, nullptr);

    WebPImageFormat format;
    auto reader = format.createReaderFor (stream.release());
    ASSERT_NE (reader, nullptr);
    ASSERT_TRUE (reader->isAnimated());

    const int frameCount = reader->getFrameCount();

    // Spot-check reverse order (forces reset + recomposite) on last 3 frames
    for (int fi = frameCount - 1; fi >= std::max (0, frameCount - 3); --fi)
    {
        auto frame = reader->readFrame (fi);
        EXPECT_TRUE (frame.isValid()) << "Reverse frame " << fi;
    }
}

TEST (WebPImageFormatTests, AnimatedImage_ZeroAllocationPathWorks)
{
    auto stream = loadTestFile ("images/animation.webp");
    ASSERT_NE (stream, nullptr);

    WebPImageFormat format;
    auto reader = format.createReaderFor (stream.release());
    ASSERT_NE (reader, nullptr);
    ASSERT_TRUE (reader->isAnimated());

    const int frameCount = reader->getFrameCount();

    // Pre-allocate a destination image
    Image dest (reader->width, reader->height, PixelFormat::RGBA);

    for (int fi = 0; fi < frameCount; ++fi)
    {
        EXPECT_TRUE (reader->readFrame (fi, dest));
        EXPECT_TRUE (dest.isValid());
    }
}

TEST (WebPImageFormatTests, AnimatedImage_InvalidFrameIndexReturnsFalse)
{
    auto stream = loadTestFile ("images/animation.webp");
    ASSERT_NE (stream, nullptr);

    WebPImageFormat format;
    auto reader = format.createReaderFor (stream.release());
    ASSERT_NE (reader, nullptr);

    Image dest;
    EXPECT_FALSE (reader->readFrame (-1, dest));
    EXPECT_FALSE (reader->readFrame (999999, dest));
}

TEST (WebPImageFormatTests, AnimatedImage_LoopCount)
{
    auto stream = loadTestFile ("images/animation.webp");
    ASSERT_NE (stream, nullptr);

    WebPImageFormat format;
    auto reader = format.createReaderFor (stream.release());
    ASSERT_NE (reader, nullptr);
    ASSERT_TRUE (reader->isAnimated());

    // Loop count should be >= 0 (0 = infinite)
    EXPECT_GE (reader->getLoopCount(), 0);
}

//==============================================================================
// Animated WebP round-trip tests
//==============================================================================

TEST (WebPImageFormatTests, RoundTrip_WriteThenReadPreservesFrameCount)
{
    const int canvasW = 64;
    const int canvasH = 64;
    const int frameCount = 5;
    const int frameDelayMs = 100;

    auto frames = createTestFrames (canvasW, canvasH, frameCount);

    // Write animated WebP
    MemoryBlock block;
    auto* memOut = new MemoryOutputStream();
    {
        WebPImageFormat format;
        auto writer = format.createWriterFor (memOut, PixelFormat::RGBA, {}, 0);

        ASSERT_TRUE (writer->supportsAnimation());
        ASSERT_TRUE (writer->beginAnimation (0));

        for (const auto& frame : frames)
            ASSERT_TRUE (writer->writeFrame (frame, frameDelayMs));

        ASSERT_TRUE (writer->endAnimation());
        block = memOut->getMemoryBlock();
    }

    // Read back
    auto memIn = std::make_unique<MemoryInputStream> (block.getData(), block.getSize(), false);
    WebPImageFormat format;
    auto reader = format.createReaderFor (memIn.release());

    ASSERT_NE (reader, nullptr);
    EXPECT_TRUE (reader->isAnimated());
    EXPECT_EQ (reader->getFrameCount(), frameCount);
    EXPECT_EQ (reader->width, canvasW);
    EXPECT_EQ (reader->height, canvasH);

    for (int fi = 0; fi < frameCount; ++fi)
    {
        EXPECT_EQ (reader->getFrameDelayMs (fi), frameDelayMs);
        auto frame = reader->readFrame (fi);
        EXPECT_TRUE (frame.isValid());
        EXPECT_EQ (frame.getWidth(), canvasW);
        EXPECT_EQ (frame.getHeight(), canvasH);

        // Verify lossless roundtrip: every pixel must be identical
        const auto& original = frames[static_cast<size_t> (fi)];
        for (int y = 0; y < canvasH; ++y)
            for (int x = 0; x < canvasW; ++x)
                EXPECT_EQ (frame.getPixel (x, y), original.getPixel (x, y))
                    << "Pixel mismatch at frame " << fi << " (" << x << "," << y << ")";
    }
}

//==============================================================================
// APNG round-trip tests
//==============================================================================

TEST (PngImageFormatTests, RoundTrip_WriteThenReadPreservesFrameCount)
{
    const int canvasW = 64;
    const int canvasH = 64;
    const int frameCount = 5;
    const int frameDelayMs = 100;

    auto frames = createTestFrames (canvasW, canvasH, frameCount);

    // Write APNG
    MemoryBlock block;
    auto* memOut = new MemoryOutputStream();
    {
        PngImageFormat format;
        auto writer = format.createWriterFor (memOut, PixelFormat::RGBA, {}, 0);

        ASSERT_TRUE (writer->supportsAnimation());
        ASSERT_TRUE (writer->beginAnimation (0));

        for (const auto& frame : frames)
            ASSERT_TRUE (writer->writeFrame (frame, frameDelayMs));

        ASSERT_TRUE (writer->endAnimation());
        block = memOut->getMemoryBlock();
    }

    // Read back
    auto memIn = std::make_unique<MemoryInputStream> (block.getData(), block.getSize(), false);
    PngImageFormat format;
    auto reader = format.createReaderFor (memIn.release());

    ASSERT_NE (reader, nullptr);
    EXPECT_TRUE (reader->isAnimated());
    EXPECT_EQ (reader->getFrameCount(), frameCount);
    EXPECT_EQ (reader->width, canvasW);
    EXPECT_EQ (reader->height, canvasH);

    for (int fi = 0; fi < frameCount; ++fi)
    {
        EXPECT_EQ (reader->getFrameDelayMs (fi), frameDelayMs);
        auto frame = reader->readFrame (fi);
        EXPECT_TRUE (frame.isValid());
        EXPECT_EQ (frame.getWidth(), canvasW);
        EXPECT_EQ (frame.getHeight(), canvasH);

        // Verify lossless roundtrip: every pixel must be identical
        const auto& original = frames[static_cast<size_t> (fi)];
        for (int y = 0; y < canvasH; ++y)
            for (int x = 0; x < canvasW; ++x)
                EXPECT_EQ (frame.getPixel (x, y), original.getPixel (x, y))
                    << "Pixel mismatch at frame " << fi << " (" << x << "," << y << ")";
    }
}

TEST (PngImageFormatTests, RoundTrip_LoopCountPreserved)
{
    const int canvasW = 32;
    const int canvasH = 32;
    const int loopCount = 3;

    Image frame (canvasW, canvasH, PixelFormat::RGBA);
    frame.fill (0xFFFF0000u);

    MemoryBlock block;
    auto* memOut = new MemoryOutputStream();
    {
        PngImageFormat format;
        auto writer = format.createWriterFor (memOut, PixelFormat::RGBA, {}, 0);

        ASSERT_TRUE (writer->beginAnimation (loopCount));
        ASSERT_TRUE (writer->writeFrame (frame, 50));
        ASSERT_TRUE (writer->writeFrame (frame, 50));
        ASSERT_TRUE (writer->endAnimation());
        block = memOut->getMemoryBlock();
    }

    auto memIn = std::make_unique<MemoryInputStream> (block.getData(), block.getSize(), false);
    PngImageFormat format;
    auto reader = format.createReaderFor (memIn.release());

    ASSERT_NE (reader, nullptr);
    EXPECT_TRUE (reader->isAnimated());
    EXPECT_EQ (reader->getLoopCount(), loopCount);

    // Verify lossless roundtrip
    for (int fi = 0; fi < 2; ++fi)
    {
        auto result = reader->readFrame (fi);
        for (int y = 0; y < canvasH; ++y)
            for (int x = 0; x < canvasW; ++x)
                EXPECT_EQ (result.getPixel (x, y), frame.getPixel (x, y))
                    << "Pixel mismatch at frame " << fi << " (" << x << "," << y << ")";
    }
}

TEST (PngImageFormatTests, RoundTrip_SequentialAccess)
{
    const int frameCount = 10;
    const int canvasW = 48;
    const int canvasH = 48;

    auto frames = createTestFrames (canvasW, canvasH, frameCount);

    MemoryBlock block;
    auto* memOut = new MemoryOutputStream();
    {
        PngImageFormat format;
        auto writer = format.createWriterFor (memOut, PixelFormat::RGBA, {}, 0);
        ASSERT_TRUE (writer->beginAnimation (0));

        for (const auto& frame : frames)
            ASSERT_TRUE (writer->writeFrame (frame, 42));

        ASSERT_TRUE (writer->endAnimation());
        block = memOut->getMemoryBlock();
    }

    auto memIn = std::make_unique<MemoryInputStream> (block.getData(), block.getSize(), false);
    PngImageFormat format;
    auto reader = format.createReaderFor (memIn.release());
    ASSERT_NE (reader, nullptr);
    ASSERT_EQ (reader->getFrameCount(), frameCount);

    // Sequential forward
    for (int fi = 0; fi < frameCount; ++fi)
    {
        auto result = reader->readFrame (fi);
        EXPECT_TRUE (result.isValid());
        for (int y = 0; y < canvasH; ++y)
            for (int x = 0; x < canvasW; ++x)
                EXPECT_EQ (result.getPixel (x, y), frames[static_cast<size_t> (fi)].getPixel (x, y))
                    << "Pixel mismatch at frame " << fi << " (" << x << "," << y << ")";
    }
}

TEST (PngImageFormatTests, RoundTrip_RandomAccess)
{
    const int frameCount = 8;
    const int canvasW = 48;
    const int canvasH = 48;

    auto frames = createTestFrames (canvasW, canvasH, frameCount);

    MemoryBlock block;
    auto* memOut = new MemoryOutputStream();
    {
        PngImageFormat format;
        auto writer = format.createWriterFor (memOut, PixelFormat::RGBA, {}, 0);
        ASSERT_TRUE (writer->beginAnimation (0));

        for (const auto& frame : frames)
            ASSERT_TRUE (writer->writeFrame (frame, 42));

        ASSERT_TRUE (writer->endAnimation());
        block = memOut->getMemoryBlock();
    }

    auto memIn = std::make_unique<MemoryInputStream> (block.getData(), block.getSize(), false);
    PngImageFormat format;
    auto reader = format.createReaderFor (memIn.release());
    ASSERT_NE (reader, nullptr);

    // Reverse order (forces reset)
    for (int fi = frameCount - 1; fi >= 0; --fi)
    {
        auto result = reader->readFrame (fi);
        EXPECT_TRUE (result.isValid());
        for (int y = 0; y < canvasH; ++y)
            for (int x = 0; x < canvasW; ++x)
                EXPECT_EQ (result.getPixel (x, y), frames[static_cast<size_t> (fi)].getPixel (x, y))
                    << "Pixel mismatch at frame " << fi << " (" << x << "," << y << ")";
    }

    // Random access — spot-check key frames
    for (int fi : { 2, 5, 1, 7 })
    {
        auto result = reader->readFrame (fi);
        EXPECT_TRUE (result.isValid());
        EXPECT_EQ (result.getPixel (canvasW / 2, canvasH / 2),
                   frames[static_cast<size_t> (fi)].getPixel (canvasW / 2, canvasH / 2));
    }
}

TEST (PngImageFormatTests, RoundTrip_SingleFrameNotAnimated)
{
    Image frame (32, 32, PixelFormat::RGBA);
    frame.fill (0xFF00FF00u);

    MemoryBlock block;
    auto* memOut = new MemoryOutputStream();
    {
        PngImageFormat format;
        auto writer = format.createWriterFor (memOut, PixelFormat::RGBA, {}, 0);
        ASSERT_TRUE (writer->beginAnimation (0));
        ASSERT_TRUE (writer->writeFrame (frame, 100));
        ASSERT_TRUE (writer->endAnimation());
        block = memOut->getMemoryBlock();
    }

    auto memIn = std::make_unique<MemoryInputStream> (block.getData(), block.getSize(), false);
    PngImageFormat format;
    auto reader = format.createReaderFor (memIn.release());

    ASSERT_NE (reader, nullptr);
    // Single frame — isAnimated should be false
    EXPECT_FALSE (reader->isAnimated());

    // Verify lossless roundtrip
    auto result = reader->readImage();
    for (int y = 0; y < 32; ++y)
        for (int x = 0; x < 32; ++x)
            EXPECT_EQ (result.getPixel (x, y), frame.getPixel (x, y))
                << "Pixel mismatch at (" << x << "," << y << ")";
}

TEST (PngImageFormatTests, RoundTrip_ZeroAllocationPath)
{
    const int frameCount = 4;
    const int canvasW = 32;
    const int canvasH = 32;

    auto frames = createTestFrames (canvasW, canvasH, frameCount);

    MemoryBlock block;
    auto* memOut = new MemoryOutputStream();
    {
        PngImageFormat format;
        auto writer = format.createWriterFor (memOut, PixelFormat::RGBA, {}, 0);
        ASSERT_TRUE (writer->beginAnimation (0));

        for (const auto& frame : frames)
            ASSERT_TRUE (writer->writeFrame (frame, 42));

        ASSERT_TRUE (writer->endAnimation());
        block = memOut->getMemoryBlock();
    }

    auto memIn = std::make_unique<MemoryInputStream> (block.getData(), block.getSize(), false);
    PngImageFormat format;
    auto reader = format.createReaderFor (memIn.release());
    ASSERT_NE (reader, nullptr);
    ASSERT_EQ (reader->getFrameCount(), frameCount);

    Image dest (canvasW, canvasH, PixelFormat::RGBA);

    for (int fi = 0; fi < frameCount; ++fi)
    {
        EXPECT_TRUE (reader->readFrame (fi, dest));
        for (int y = 0; y < canvasH; ++y)
            for (int x = 0; x < canvasW; ++x)
                EXPECT_EQ (dest.getPixel (x, y), frames[static_cast<size_t> (fi)].getPixel (x, y))
                    << "Pixel mismatch at frame " << fi << " (" << x << "," << y << ")";
    }
}

TEST (PngImageFormatTests, StaticImage_NotAnimated)
{
    auto stream = loadTestFile ("images/file_example.png");
    ASSERT_NE (stream, nullptr);

    PngImageFormat format;
    auto reader = format.createReaderFor (stream.release());
    ASSERT_NE (reader, nullptr);

    EXPECT_FALSE (reader->isAnimated());
    EXPECT_GT (reader->width, 0);
    EXPECT_GT (reader->height, 0);

    auto image = reader->readImage();
    EXPECT_TRUE (image.isValid());
}

//==============================================================================
// APNG read tests (real animation.png)
//==============================================================================

TEST (PngImageFormatTests, AnimatedImage_DetectsAnimation)
{
    auto stream = loadTestFile ("images/animation.png");
    ASSERT_NE (stream, nullptr);

    PngImageFormat format;
    auto reader = format.createReaderFor (stream.release());
    ASSERT_NE (reader, nullptr);

    EXPECT_TRUE (reader->isAnimated());
    EXPECT_GT (reader->getFrameCount(), 1);
}

TEST (PngImageFormatTests, AnimatedImage_HasReasonableFrameDelays)
{
    auto stream = loadTestFile ("images/animation.png");
    ASSERT_NE (stream, nullptr);

    PngImageFormat format;
    auto reader = format.createReaderFor (stream.release());
    ASSERT_NE (reader, nullptr);
    ASSERT_TRUE (reader->isAnimated());

    const int frameCount = reader->getFrameCount();

    for (int fi = 0; fi < frameCount; ++fi)
    {
        const int delay = reader->getFrameDelayMs (fi);
        EXPECT_GE (delay, 0) << "Frame " << fi << " has negative delay";
        EXPECT_LT (delay, 60000) << "Frame " << fi << " has unreasonably large delay";
    }
}

TEST (PngImageFormatTests, AnimatedImage_ReadsAllFrames)
{
    auto stream = loadTestFile ("images/animation.png");
    ASSERT_NE (stream, nullptr);

    PngImageFormat format;
    auto reader = format.createReaderFor (stream.release());
    ASSERT_NE (reader, nullptr);
    ASSERT_TRUE (reader->isAnimated());

    const int frameCount = reader->getFrameCount();
    const int w = reader->width;
    const int h = reader->height;

    for (int fi = 0; fi < frameCount; ++fi)
    {
        auto frame = reader->readFrame (fi);
        EXPECT_TRUE (frame.isValid()) << "Frame " << fi << " failed to decode";
        EXPECT_EQ (frame.getWidth(), w) << "Frame " << fi << " has wrong width";
        EXPECT_EQ (frame.getHeight(), h) << "Frame " << fi << " has wrong height";
    }
}

TEST (PngImageFormatTests, AnimatedImage_ReadImageReturnsFrame0)
{
    auto stream = loadTestFile ("images/animation.png");
    ASSERT_NE (stream, nullptr);

    PngImageFormat format;
    auto reader = format.createReaderFor (stream.release());
    ASSERT_NE (reader, nullptr);
    ASSERT_TRUE (reader->isAnimated());

    auto image = reader->readImage();
    auto frame0 = reader->readFrame (0);

    EXPECT_TRUE (image.isValid());
    EXPECT_TRUE (frame0.isValid());
    EXPECT_EQ (image.getWidth(), frame0.getWidth());
    EXPECT_EQ (image.getHeight(), frame0.getHeight());
}

TEST (PngImageFormatTests, AnimatedImage_SequentialAccess)
{
    auto stream = loadTestFile ("images/animation.png");
    ASSERT_NE (stream, nullptr);

    PngImageFormat format;
    auto reader = format.createReaderFor (stream.release());
    ASSERT_NE (reader, nullptr);
    ASSERT_TRUE (reader->isAnimated());

    const int frameCount = reader->getFrameCount();

    for (int fi = 0; fi < frameCount; ++fi)
    {
        auto frame = reader->readFrame (fi);
        EXPECT_TRUE (frame.isValid()) << "Sequential frame " << fi;
    }
}

TEST (PngImageFormatTests, AnimatedImage_RandomAccess)
{
    auto stream = loadTestFile ("images/animation.png");
    ASSERT_NE (stream, nullptr);

    PngImageFormat format;
    auto reader = format.createReaderFor (stream.release());
    ASSERT_NE (reader, nullptr);
    ASSERT_TRUE (reader->isAnimated());

    const int frameCount = reader->getFrameCount();

    // Spot-check reverse order (forces reset + recomposite) on last 3 frames
    for (int fi = frameCount - 1; fi >= std::max (0, frameCount - 3); --fi)
    {
        auto frame = reader->readFrame (fi);
        EXPECT_TRUE (frame.isValid()) << "Reverse frame " << fi;
    }
}

TEST (PngImageFormatTests, AnimatedImage_ZeroAllocationPath)
{
    auto stream = loadTestFile ("images/animation.png");
    ASSERT_NE (stream, nullptr);

    PngImageFormat format;
    auto reader = format.createReaderFor (stream.release());
    ASSERT_NE (reader, nullptr);
    ASSERT_TRUE (reader->isAnimated());

    const int frameCount = reader->getFrameCount();

    Image dest (reader->width, reader->height, PixelFormat::RGBA);

    for (int fi = 0; fi < frameCount; ++fi)
        EXPECT_TRUE (reader->readFrame (fi, dest));
}

TEST (PngImageFormatTests, AnimatedImage_InvalidFrameIndexReturnsFalse)
{
    auto stream = loadTestFile ("images/animation.png");
    ASSERT_NE (stream, nullptr);

    PngImageFormat format;
    auto reader = format.createReaderFor (stream.release());
    ASSERT_NE (reader, nullptr);

    Image dest;
    EXPECT_FALSE (reader->readFrame (-1, dest));
    EXPECT_FALSE (reader->readFrame (999999, dest));
}

TEST (PngImageFormatTests, AnimatedImage_LoopCount)
{
    auto stream = loadTestFile ("images/animation.png");
    ASSERT_NE (stream, nullptr);

    PngImageFormat format;
    auto reader = format.createReaderFor (stream.release());
    ASSERT_NE (reader, nullptr);
    ASSERT_TRUE (reader->isAnimated());

    EXPECT_GE (reader->getLoopCount(), 0);
}

TEST (PngImageFormatTests, AnimatedImage_FramesHaveOpaquePixels)
{
    auto stream = loadTestFile ("images/animation.png");
    ASSERT_NE (stream, nullptr);

    PngImageFormat format;
    auto reader = format.createReaderFor (stream.release());
    ASSERT_NE (reader, nullptr);
    ASSERT_TRUE (reader->isAnimated());

    const int frameCount = reader->getFrameCount();
    ASSERT_GT (frameCount, 1);

    // Read all frames sequentially and verify each has at least some opaque content
    for (int fi = 0; fi < frameCount; ++fi)
    {
        Image dest;
        ASSERT_TRUE (reader->readFrame (fi, dest));

        int opaqueCount = 0;
        const int w = dest.getWidth();
        const int h = dest.getHeight();

        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                if ((dest.getPixel (x, y) >> 24) != 0)
                    ++opaqueCount;

        const int totalPixels = w * h;
        const float opaqueRatio = static_cast<float> (opaqueCount) / static_cast<float> (totalPixels);

        // Each frame should have at least 10% opaque pixels
        EXPECT_GT (opaqueRatio, 0.1f)
            << "Frame " << fi << " has only " << (opaqueRatio * 100.0f)
            << "% opaque pixels (expected > 10%)";
    }
}

//==============================================================================
// Static format detection tests
//==============================================================================

TEST (ImageFormatManagerTests, DetectsWebP)
{
    auto stream = loadTestFile ("images/file_example.webp");
    ASSERT_NE (stream, nullptr);

    ImageFormatManager manager;
    manager.registerDefaultFormats();
    auto reader = manager.createReaderFor (stream.release());
    EXPECT_NE (reader, nullptr);
}

TEST (ImageFormatManagerTests, DetectsPNG)
{
    auto stream = loadTestFile ("images/file_example.png");
    ASSERT_NE (stream, nullptr);

    ImageFormatManager manager;
    manager.registerDefaultFormats();
    auto reader = manager.createReaderFor (stream.release());
    EXPECT_NE (reader, nullptr);
}

//==============================================================================
// APNG writer structural verification tests
//==============================================================================

// Verify that saved APNG has no duplicate sequence numbers
TEST (PngImageFormatTests, Writer_ProducesUniqueSequenceNumbers)
{
    Image frame (32, 32, PixelFormat::RGBA);
    frame.fill (0xFFFF0000u);

    MemoryBlock block;
    auto* memOut = new MemoryOutputStream();
    {
        PngImageFormat format;
        auto writer = format.createWriterFor (memOut, PixelFormat::RGBA, {}, 0);
        ASSERT_TRUE (writer->beginAnimation (0));

        for (int i = 0; i < 10; ++i)
            ASSERT_TRUE (writer->writeFrame (frame, 50));

        ASSERT_TRUE (writer->endAnimation());
        block = memOut->getMemoryBlock();
    }

    // Parse the output and verify no duplicate sequence numbers
    const auto* data = static_cast<const uint8_t*> (block.getData());
    const auto size = block.getSize();

    std::set<uint32_t> sequences;
    size_t pos = 8; // skip PNG signature

    while (pos + 8 <= size)
    {
        const auto chunkLen = (static_cast<uint32_t> (data[pos]) << 24)
                            | (static_cast<uint32_t> (data[pos + 1]) << 16)
                            | (static_cast<uint32_t> (data[pos + 2]) << 8)
                            | static_cast<uint32_t> (data[pos + 3]);
        pos += 4;
        const char* type = reinterpret_cast<const char*> (data + pos);
        pos += 4;

        if (pos + chunkLen > size)
            break;

        if ((std::memcmp (type, "fcTL", 4) == 0 && chunkLen >= 4)
            || (std::memcmp (type, "fdAT", 4) == 0 && chunkLen >= 4))
        {
            const uint32_t seq = (static_cast<uint32_t> (data[pos]) << 24)
                               | (static_cast<uint32_t> (data[pos + 1]) << 16)
                               | (static_cast<uint32_t> (data[pos + 2]) << 8)
                               | static_cast<uint32_t> (data[pos + 3]);

            EXPECT_FALSE (sequences.count (seq))
                << "Duplicate sequence number " << seq << " in " << type << " chunk";
            sequences.insert (seq);
        }

        pos += chunkLen + 4;
    }

    // Should have 20 sequence-bearing chunks (10 fcTL + 10 fdAT, minus 1 IDAT for frame 0)
    EXPECT_EQ (sequences.size(), 19u) << "Expected 19 unique sequence numbers (10 fcTL + 9 fdAT)";
}

// Verify the output has correct APNG chunk structure
TEST (PngImageFormatTests, Writer_ProducesCorrectChunkOrder)
{
    Image frame (16, 16, PixelFormat::RGBA);
    frame.fill (0xFF00FF00u);

    MemoryBlock block;
    auto* memOut = new MemoryOutputStream();
    {
        PngImageFormat format;
        auto writer = format.createWriterFor (memOut, PixelFormat::RGBA, {}, 0);
        ASSERT_TRUE (writer->beginAnimation (0));
        ASSERT_TRUE (writer->writeFrame (frame, 42));
        ASSERT_TRUE (writer->writeFrame (frame, 100));
        ASSERT_TRUE (writer->writeFrame (frame, 200));
        ASSERT_TRUE (writer->endAnimation());
        block = memOut->getMemoryBlock();
    }

    // Verify chunk order: sig → IHDR → acTL → fcTL → IDAT → fcTL → fdAT → fcTL → fdAT → IEND
    const auto* data = static_cast<const uint8_t*> (block.getData());
    const auto size = block.getSize();

    std::vector<std::string> chunkTypes;
    size_t pos = 8;

    while (pos + 8 <= size)
    {
        const auto chunkLen = (static_cast<uint32_t> (data[pos]) << 24)
                            | (static_cast<uint32_t> (data[pos + 1]) << 16)
                            | (static_cast<uint32_t> (data[pos + 2]) << 8)
                            | static_cast<uint32_t> (data[pos + 3]);
        pos += 4;
        const char* type = reinterpret_cast<const char*> (data + pos);
        pos += 4;

        if (pos + chunkLen > size)
            break;

        chunkTypes.push_back (std::string (type, 4));
        pos += chunkLen + 4;
    }

    const std::vector<std::string> expected = {
        "IHDR", "acTL", "fcTL", "IDAT", "fcTL", "fdAT", "fcTL", "fdAT", "IEND"
    };

    EXPECT_EQ (chunkTypes, expected);
}

//==============================================================================
// APNG round-trip pixel identity tests
//==============================================================================

TEST (PngImageFormatTests, RoundTrip_PixelValuesAreIdentical)
{
    const int canvasW = 32;
    const int canvasH = 32;

    // Create frames with known pixel values at specific positions
    Image frame0 (canvasW, canvasH, PixelFormat::RGBA);
    frame0.fill (0x00000000u);
    frame0.setPixel (10, 10, 0xFFFF0000u); // opaque red
    frame0.setPixel (20, 20, 0x8000FF00u); // semi-transparent green

    Image frame1 (canvasW, canvasH, PixelFormat::RGBA);
    frame1.fill (0x00000000u);
    frame1.setPixel (5, 5, 0xFF0000FFu);   // opaque blue
    frame1.setPixel (15, 15, 0x40FF0000u); // semi-transparent red

    MemoryBlock block;
    auto* memOut = new MemoryOutputStream();
    {
        PngImageFormat format;
        auto writer = format.createWriterFor (memOut, PixelFormat::RGBA, {}, 0);
        ASSERT_TRUE (writer->beginAnimation (0));
        ASSERT_TRUE (writer->writeFrame (frame0, 50));
        ASSERT_TRUE (writer->writeFrame (frame1, 50));
        ASSERT_TRUE (writer->endAnimation());
        block = memOut->getMemoryBlock();
    }

    auto memIn = std::make_unique<MemoryInputStream> (block.getData(), block.getSize(), false);
    PngImageFormat format;
    auto reader = format.createReaderFor (memIn.release());
    ASSERT_NE (reader, nullptr);
    ASSERT_EQ (reader->getFrameCount(), 2);

    // Read frame 0 and verify pixel values
    Image result0;
    ASSERT_TRUE (reader->readFrame (0, result0));
    EXPECT_EQ (result0.getPixel (10, 10), 0xFFFF0000u); // opaque red
    EXPECT_EQ (result0.getPixel (20, 20), 0x8000FF00u); // semi-transparent green
    EXPECT_EQ (result0.getPixel (5, 5), 0x00000000u);   // transparent (not set)

    // Read frame 1 (composites with blend=SOURCE via writer default)
    Image result1;
    ASSERT_TRUE (reader->readFrame (1, result1));
    EXPECT_EQ (result1.getPixel (5, 5), 0xFF0000FFu);   // opaque blue
    EXPECT_EQ (result1.getPixel (15, 15), 0x40FF0000u); // semi-transparent red
}

//==============================================================================
// APNG round-trip edge case tests
//==============================================================================

TEST (PngImageFormatTests, RoundTrip_ZeroDelay)
{
    Image frame (16, 16, PixelFormat::RGBA);
    frame.fill (0xFF0000FFu);

    MemoryBlock block;
    auto* memOut = new MemoryOutputStream();
    {
        PngImageFormat format;
        auto writer = format.createWriterFor (memOut, PixelFormat::RGBA, {}, 0);
        ASSERT_TRUE (writer->beginAnimation (0));
        ASSERT_TRUE (writer->writeFrame (frame, 0));
        ASSERT_TRUE (writer->writeFrame (frame, 0));
        ASSERT_TRUE (writer->endAnimation());
        block = memOut->getMemoryBlock();
    }

    auto memIn = std::make_unique<MemoryInputStream> (block.getData(), block.getSize(), false);
    PngImageFormat format;
    auto reader = format.createReaderFor (memIn.release());
    ASSERT_NE (reader, nullptr);
    ASSERT_EQ (reader->getFrameCount(), 2);
    EXPECT_EQ (reader->getFrameDelayMs (0), 0);
    EXPECT_EQ (reader->getFrameDelayMs (1), 0);

    // Verify lossless roundtrip
    for (int fi = 0; fi < 2; ++fi)
    {
        auto result = reader->readFrame (fi);
        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < 16; ++x)
                EXPECT_EQ (result.getPixel (x, y), frame.getPixel (x, y))
                    << "Pixel mismatch at frame " << fi << " (" << x << "," << y << ")";
    }
}

TEST (PngImageFormatTests, RoundTrip_MaxDelay)
{
    Image frame (16, 16, PixelFormat::RGBA);
    frame.fill (0xFF0000FFu);

    MemoryBlock block;
    auto* memOut = new MemoryOutputStream();
    {
        PngImageFormat format;
        auto writer = format.createWriterFor (memOut, PixelFormat::RGBA, {}, 0);
        ASSERT_TRUE (writer->beginAnimation (0));
        ASSERT_TRUE (writer->writeFrame (frame, 65535)); // uint16 max
        ASSERT_TRUE (writer->endAnimation());
        block = memOut->getMemoryBlock();
    }

    auto memIn = std::make_unique<MemoryInputStream> (block.getData(), block.getSize(), false);
    PngImageFormat format;
    auto reader = format.createReaderFor (memIn.release());
    ASSERT_NE (reader, nullptr);
    EXPECT_EQ (reader->getFrameDelayMs (0), 65535);

    // Verify lossless roundtrip
    auto result = reader->readFrame (0);
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x)
            EXPECT_EQ (result.getPixel (x, y), frame.getPixel (x, y))
                << "Pixel mismatch at (" << x << "," << y << ")";
}

TEST (PngImageFormatTests, RoundTrip_LargeDelayClamped)
{
    Image frame (16, 16, PixelFormat::RGBA);
    frame.fill (0xFF0000FFu);

    MemoryBlock block;
    auto* memOut = new MemoryOutputStream();
    {
        PngImageFormat format;
        auto writer = format.createWriterFor (memOut, PixelFormat::RGBA, {}, 0);
        ASSERT_TRUE (writer->beginAnimation (0));
        ASSERT_TRUE (writer->writeFrame (frame, 100000)); // > 65535
        ASSERT_TRUE (writer->endAnimation());
        block = memOut->getMemoryBlock();
    }

    auto memIn = std::make_unique<MemoryInputStream> (block.getData(), block.getSize(), false);
    PngImageFormat format;
    auto reader = format.createReaderFor (memIn.release());
    ASSERT_NE (reader, nullptr);
    EXPECT_EQ (reader->getFrameDelayMs (0), 65535); // clamped

    // Verify lossless roundtrip
    auto result = reader->readFrame (0);
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x)
            EXPECT_EQ (result.getPixel (x, y), frame.getPixel (x, y))
                << "Pixel mismatch at (" << x << "," << y << ")";
}

TEST (PngImageFormatTests, RoundTrip_TinyCanvas)
{
    Image frame (1, 1, PixelFormat::RGBA);
    frame.setPixel (0, 0, 0xFFFF00FFu);

    MemoryBlock block;
    auto* memOut = new MemoryOutputStream();
    {
        PngImageFormat format;
        auto writer = format.createWriterFor (memOut, PixelFormat::RGBA, {}, 0);
        ASSERT_TRUE (writer->beginAnimation (0));
        ASSERT_TRUE (writer->writeFrame (frame, 100));
        ASSERT_TRUE (writer->endAnimation());
        block = memOut->getMemoryBlock();
    }

    auto memIn = std::make_unique<MemoryInputStream> (block.getData(), block.getSize(), false);
    PngImageFormat format;
    auto reader = format.createReaderFor (memIn.release());
    ASSERT_NE (reader, nullptr);
    ASSERT_EQ (reader->getFrameCount(), 1);
    EXPECT_FALSE (reader->isAnimated());

    Image result;
    ASSERT_TRUE (reader->readFrame (0, result));
    EXPECT_EQ (result.getPixel (0, 0), 0xFFFF00FFu);
}

TEST (PngImageFormatTests, RoundTrip_ManyFrames)
{
    const int frameCount = 50;
    Image frame (8, 8, PixelFormat::RGBA);
    frame.fill (0xFF000000u);
    frame.setPixel (frameCount % 8, frameCount / 8, 0xFFFFFFFFu);

    MemoryBlock block;
    auto* memOut = new MemoryOutputStream();
    {
        PngImageFormat format;
        auto writer = format.createWriterFor (memOut, PixelFormat::RGBA, {}, 0);
        ASSERT_TRUE (writer->beginAnimation (0));

        for (int i = 0; i < frameCount; ++i)
        {
            Image f (8, 8, PixelFormat::RGBA);
            f.fill (0xFF000000u);
            f.setPixel (i % 8, i / 8, 0xFFFFFFFFu);
            ASSERT_TRUE (writer->writeFrame (f, 10));
        }

        ASSERT_TRUE (writer->endAnimation());
        block = memOut->getMemoryBlock();
    }

    auto memIn = std::make_unique<MemoryInputStream> (block.getData(), block.getSize(), false);
    PngImageFormat format;
    auto reader = format.createReaderFor (memIn.release());
    ASSERT_NE (reader, nullptr);
    EXPECT_TRUE (reader->isAnimated());
    EXPECT_EQ (reader->getFrameCount(), frameCount);

    for (int i = 0; i < frameCount; ++i)
        EXPECT_EQ (reader->getFrameDelayMs (i), 10);

    // Spot-check first and last frame pixels
    for (int fi : { 0, frameCount - 1 })
    {
        auto result = reader->readFrame (fi);
        EXPECT_EQ (result.getPixel (fi % 8, fi / 8), 0xFFFFFFFFu);
        EXPECT_EQ (result.getPixel ((fi + 1) % 8, 0), 0xFF000000u);
    }
}

TEST (PngImageFormatTests, RoundTrip_LoopCountZero)
{
    Image frame (16, 16, PixelFormat::RGBA);
    frame.fill (0xFFFF0000u);

    MemoryBlock block;
    auto* memOut = new MemoryOutputStream();
    {
        PngImageFormat format;
        auto writer = format.createWriterFor (memOut, PixelFormat::RGBA, {}, 0);
        ASSERT_TRUE (writer->beginAnimation (0)); // 0 = infinite
        ASSERT_TRUE (writer->writeFrame (frame, 50));
        ASSERT_TRUE (writer->writeFrame (frame, 50));
        ASSERT_TRUE (writer->endAnimation());
        block = memOut->getMemoryBlock();
    }

    auto memIn = std::make_unique<MemoryInputStream> (block.getData(), block.getSize(), false);
    PngImageFormat format;
    auto reader = format.createReaderFor (memIn.release());
    ASSERT_NE (reader, nullptr);
    EXPECT_EQ (reader->getLoopCount(), 0);

    // Verify lossless roundtrip
    for (int fi = 0; fi < 2; ++fi)
    {
        auto result = reader->readFrame (fi);
        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < 16; ++x)
                EXPECT_EQ (result.getPixel (x, y), frame.getPixel (x, y))
                    << "Pixel mismatch at frame " << fi << " (" << x << "," << y << ")";
    }
}

TEST (PngImageFormatTests, RoundTrip_LoopCountOne)
{
    Image frame (16, 16, PixelFormat::RGBA);
    frame.fill (0xFFFF0000u);

    MemoryBlock block;
    auto* memOut = new MemoryOutputStream();
    {
        PngImageFormat format;
        auto writer = format.createWriterFor (memOut, PixelFormat::RGBA, {}, 0);
        ASSERT_TRUE (writer->beginAnimation (1)); // play once
        ASSERT_TRUE (writer->writeFrame (frame, 50));
        ASSERT_TRUE (writer->writeFrame (frame, 50));
        ASSERT_TRUE (writer->endAnimation());
        block = memOut->getMemoryBlock();
    }

    auto memIn = std::make_unique<MemoryInputStream> (block.getData(), block.getSize(), false);
    PngImageFormat format;
    auto reader = format.createReaderFor (memIn.release());
    ASSERT_NE (reader, nullptr);
    EXPECT_EQ (reader->getLoopCount(), 1);

    // Verify lossless roundtrip
    for (int fi = 0; fi < 2; ++fi)
    {
        auto result = reader->readFrame (fi);
        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < 16; ++x)
                EXPECT_EQ (result.getPixel (x, y), frame.getPixel (x, y))
                    << "Pixel mismatch at frame " << fi << " (" << x << "," << y << ")";
    }
}

TEST (PngImageFormatTests, RoundTrip_DifferentFrameDelays)
{
    Image frame (8, 8, PixelFormat::RGBA);
    frame.fill (0xFF000000u);

    const std::vector<int> delays = { 10, 50, 100, 500, 0 };

    MemoryBlock block;
    auto* memOut = new MemoryOutputStream();
    {
        PngImageFormat format;
        auto writer = format.createWriterFor (memOut, PixelFormat::RGBA, {}, 0);
        ASSERT_TRUE (writer->beginAnimation (0));

        for (auto d : delays)
            ASSERT_TRUE (writer->writeFrame (frame, d));

        ASSERT_TRUE (writer->endAnimation());
        block = memOut->getMemoryBlock();
    }

    auto memIn = std::make_unique<MemoryInputStream> (block.getData(), block.getSize(), false);
    PngImageFormat format;
    auto reader = format.createReaderFor (memIn.release());
    ASSERT_NE (reader, nullptr);
    ASSERT_EQ (reader->getFrameCount(), static_cast<int> (delays.size()));

    for (size_t i = 0; i < delays.size(); ++i)
        EXPECT_EQ (reader->getFrameDelayMs (static_cast<int> (i)), delays[i]);

    // Verify lossless roundtrip
    for (size_t i = 0; i < delays.size(); ++i)
    {
        auto result = reader->readFrame (static_cast<int> (i));
        for (int y = 0; y < 8; ++y)
            for (int x = 0; x < 8; ++x)
                EXPECT_EQ (result.getPixel (x, y), frame.getPixel (x, y))
                    << "Pixel mismatch at frame " << i << " (" << x << "," << y << ")";
    }
}

TEST (PngImageFormatTests, RoundTrip_ReadThenWritePreservesFrameCount)
{
    // Load real APNG, save it, read back and verify frame count
    auto stream = loadTestFile ("images/animation.png");
    ASSERT_NE (stream, nullptr);

    PngImageFormat format;
    auto reader = format.createReaderFor (stream.release());
    ASSERT_NE (reader, nullptr);
    ASSERT_TRUE (reader->isAnimated());

    const int originalFrameCount = reader->getFrameCount();
    ASSERT_GT (originalFrameCount, 1);

    // Read all frames
    std::vector<Image> frames;
    std::vector<int> delays;

    for (int i = 0; i < originalFrameCount; ++i)
    {
        frames.push_back (reader->readFrame (i));
        delays.push_back (reader->getFrameDelayMs (i));
    }

    // Write them back
    MemoryBlock block;
    auto* memOut = new MemoryOutputStream();
    {
        PngImageFormat format2;
        auto writer = format2.createWriterFor (memOut, PixelFormat::RGBA, {}, 0);
        ASSERT_TRUE (writer->beginAnimation (reader->getLoopCount()));

        for (int i = 0; i < originalFrameCount; ++i)
            ASSERT_TRUE (writer->writeFrame (frames[static_cast<size_t> (i)], delays[static_cast<size_t> (i)]));

        ASSERT_TRUE (writer->endAnimation());
        block = memOut->getMemoryBlock();
    }

    // Read back and verify
    auto memIn = std::make_unique<MemoryInputStream> (block.getData(), block.getSize(), false);
    PngImageFormat format3;
    auto reader2 = format3.createReaderFor (memIn.release());
    ASSERT_NE (reader2, nullptr);
    EXPECT_TRUE (reader2->isAnimated());
    EXPECT_EQ (reader2->getFrameCount(), originalFrameCount);

    // Verify all frames are valid, have the same dimensions, and spot-check pixels
    for (int i = 0; i < originalFrameCount; ++i)
    {
        auto frame = reader2->readFrame (i);
        EXPECT_TRUE (frame.isValid());
        EXPECT_EQ (frame.getWidth(), reader2->width);
        EXPECT_EQ (frame.getHeight(), reader2->height);

        // Spot-check center pixel against originally loaded frame
        const auto& orig = frames[static_cast<size_t> (i)];
        EXPECT_EQ (frame.getPixel (reader2->width / 2, reader2->height / 2),
                   orig.getPixel (reader2->width / 2, reader2->height / 2))
            << "Center pixel mismatch at frame " << i;
    }
}

//==============================================================================
// Animated WebP round-trip edge case tests
//==============================================================================

TEST (WebPImageFormatTests, RoundTrip_PixelValuesAreIdentical)
{
    const int canvasW = 32;
    const int canvasH = 32;

    Image frame0 (canvasW, canvasH, PixelFormat::RGBA);
    frame0.fill (0x00000000u);
    frame0.setPixel (10, 10, 0xFFFF0000u);
    frame0.setPixel (20, 20, 0x8000FF00u);

    Image frame1 (canvasW, canvasH, PixelFormat::RGBA);
    frame1.fill (0x00000000u);
    frame1.setPixel (5, 5, 0xFF0000FFu);
    frame1.setPixel (15, 15, 0x40FF0000u);

    MemoryBlock block;
    auto* memOut = new MemoryOutputStream();
    {
        WebPImageFormat format;
        auto writer = format.createWriterFor (memOut, PixelFormat::RGBA, {}, 0);
        ASSERT_TRUE (writer->beginAnimation (0));
        ASSERT_TRUE (writer->writeFrame (frame0, 50));
        ASSERT_TRUE (writer->writeFrame (frame1, 50));
        ASSERT_TRUE (writer->endAnimation());
        block = memOut->getMemoryBlock();
    }

    auto memIn = std::make_unique<MemoryInputStream> (block.getData(), block.getSize(), false);
    WebPImageFormat format;
    auto reader = format.createReaderFor (memIn.release());
    ASSERT_NE (reader, nullptr);
    ASSERT_EQ (reader->getFrameCount(), 2);

    Image result0;
    ASSERT_TRUE (reader->readFrame (0, result0));
    EXPECT_EQ (result0.getPixel (10, 10), 0xFFFF0000u);
    EXPECT_EQ (result0.getPixel (20, 20), 0x8000FF00u);

    Image result1;
    ASSERT_TRUE (reader->readFrame (1, result1));
    EXPECT_EQ (result1.getPixel (5, 5), 0xFF0000FFu);
    EXPECT_EQ (result1.getPixel (15, 15), 0x40FF0000u);
}

TEST (WebPImageFormatTests, RoundTrip_ManyFrames)
{
    const int frameCount = 30;
    Image frame (8, 8, PixelFormat::RGBA);

    MemoryBlock block;
    auto* memOut = new MemoryOutputStream();
    {
        WebPImageFormat format;
        auto writer = format.createWriterFor (memOut, PixelFormat::RGBA, {}, 0);
        ASSERT_TRUE (writer->beginAnimation (0));

        for (int i = 0; i < frameCount; ++i)
        {
            Image f (8, 8, PixelFormat::RGBA);
            f.fill (0xFF000000u);
            f.setPixel (i % 8, i / 8, 0xFFFFFFFFu);
            ASSERT_TRUE (writer->writeFrame (f, 10));
        }

        ASSERT_TRUE (writer->endAnimation());
        block = memOut->getMemoryBlock();
    }

    auto memIn = std::make_unique<MemoryInputStream> (block.getData(), block.getSize(), false);
    WebPImageFormat format;
    auto reader = format.createReaderFor (memIn.release());
    ASSERT_NE (reader, nullptr);
    EXPECT_TRUE (reader->isAnimated());
    EXPECT_EQ (reader->getFrameCount(), frameCount);

    for (int i = 0; i < frameCount; ++i)
        EXPECT_EQ (reader->getFrameDelayMs (i), 10);

    // Read all frames to verify they're valid
    for (int i = 0; i < frameCount; ++i)
        EXPECT_TRUE (reader->readFrame (i).isValid());

    // Spot-check first and last frame pixels
    for (int fi : { 0, frameCount - 1 })
    {
        auto result = reader->readFrame (fi);
        EXPECT_EQ (result.getPixel (fi % 8, fi / 8), 0xFFFFFFFFu);
        EXPECT_EQ (result.getPixel ((fi + 1) % 8, 0), 0xFF000000u);
    }
}

TEST (WebPImageFormatTests, RoundTrip_LoopCounts)
{
    Image frame0 (16, 16, PixelFormat::RGBA);
    frame0.fill (0xFFFF0000u);
    Image frame1 (16, 16, PixelFormat::RGBA);
    frame1.fill (0xFF0000FFu); // different from frame0 to avoid encoder dedup

    for (int loopCount : { 0, 1, 5 })
    {
        MemoryBlock block;
        auto* memOut = new MemoryOutputStream();
        {
            WebPImageFormat format;
            auto writer = format.createWriterFor (memOut, PixelFormat::RGBA, {}, 0);
            ASSERT_TRUE (writer->beginAnimation (loopCount));
            ASSERT_TRUE (writer->writeFrame (frame0, 50));
            ASSERT_TRUE (writer->writeFrame (frame1, 50));
            ASSERT_TRUE (writer->endAnimation());
            block = memOut->getMemoryBlock();
        }

        auto memIn = std::make_unique<MemoryInputStream> (block.getData(), block.getSize(), false);
        WebPImageFormat format;
        auto reader = format.createReaderFor (memIn.release());
        ASSERT_NE (reader, nullptr);
        EXPECT_EQ (reader->getLoopCount(), loopCount) << "loopCount=" << loopCount;

        // Verify lossless roundtrip
        for (int fi = 0; fi < 2; ++fi)
        {
            auto result = reader->readFrame (fi);
            const auto& expected = (fi == 0) ? frame0 : frame1;
            for (int y = 0; y < 16; ++y)
                for (int x = 0; x < 16; ++x)
                    EXPECT_EQ (result.getPixel (x, y), expected.getPixel (x, y))
                        << "Pixel mismatch at frame " << fi << " (" << x << "," << y << ")";
        }
    }
}

TEST (WebPImageFormatTests, RoundTrip_ReadThenWritePreservesFrameCount)
{
    auto stream = loadTestFile ("images/animation.webp");
    ASSERT_NE (stream, nullptr);

    WebPImageFormat format;
    auto reader = format.createReaderFor (stream.release());
    ASSERT_NE (reader, nullptr);
    ASSERT_TRUE (reader->isAnimated());

    const int originalFrameCount = reader->getFrameCount();
    ASSERT_GT (originalFrameCount, 1);

    std::vector<Image> frames;
    std::vector<int> delays;

    for (int i = 0; i < originalFrameCount; ++i)
    {
        frames.push_back (reader->readFrame (i));
        delays.push_back (reader->getFrameDelayMs (i));
    }

    MemoryBlock block;
    auto* memOut = new MemoryOutputStream();
    {
        WebPImageFormat format2;
        auto writer = format2.createWriterFor (memOut, PixelFormat::RGBA, {}, 0);
        ASSERT_TRUE (writer->beginAnimation (reader->getLoopCount()));

        for (int i = 0; i < originalFrameCount; ++i)
            ASSERT_TRUE (writer->writeFrame (frames[static_cast<size_t> (i)], delays[static_cast<size_t> (i)]));

        ASSERT_TRUE (writer->endAnimation());
        block = memOut->getMemoryBlock();
    }

    auto memIn = std::make_unique<MemoryInputStream> (block.getData(), block.getSize(), false);
    WebPImageFormat format3;
    auto reader2 = format3.createReaderFor (memIn.release());
    ASSERT_NE (reader2, nullptr);
    EXPECT_TRUE (reader2->isAnimated());
    EXPECT_EQ (reader2->getFrameCount(), originalFrameCount);

    for (int i = 0; i < originalFrameCount; ++i)
    {
        auto frame = reader2->readFrame (i);
        EXPECT_TRUE (frame.isValid());
        EXPECT_EQ (frame.getWidth(), reader2->width);
        EXPECT_EQ (frame.getHeight(), reader2->height);

        // Spot-check center pixel against originally loaded frame
        const auto& orig = frames[static_cast<size_t> (i)];
        EXPECT_EQ (frame.getPixel (reader2->width / 2, reader2->height / 2),
                   orig.getPixel (reader2->width / 2, reader2->height / 2))
            << "Center pixel mismatch at frame " << i;
    }
}
