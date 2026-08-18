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

#pragma once

#include <gtest/gtest.h>

#include <yup_core/yup_core.h>
#include <yup_rhi/yup_rhi.h>
#include <yup_graphics/yup_graphics.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <vector>

namespace yup::test
{

//==============================================================================
/** A CPU-side RGBA8 image in top-to-bottom row order.

    Both GpuDevice backends already normalise their readback to this layout:
    the OpenGL device flips the rows that glReadPixels returns bottom-up, and
    the Metal device blits through an RGBA8Unorm staging texture. Tests can
    therefore compare bytes directly across platforms, and any disagreement is
    a real difference rather than a layout artifact.
*/
struct Bitmap
{
    int width = 0;
    int height = 0;
    std::vector<uint8_t> pixels; ///< width * height * 4 bytes, RGBA, top-down.

    /** Returns true when the bitmap holds the pixels its size implies. */
    bool isValid() const noexcept
    {
        return width > 0
            && height > 0
            && pixels.size() == static_cast<size_t> (width) * static_cast<size_t> (height) * 4u;
    }

    /** Returns one pixel packed as 0xAABBGGRR, matching the byte order in memory. */
    uint32_t getPixel (int x, int y) const noexcept
    {
        jassert (x >= 0 && x < width && y >= 0 && y < height);

        const size_t index = (static_cast<size_t> (y) * static_cast<size_t> (width) + static_cast<size_t> (x)) * 4u;

        return static_cast<uint32_t> (pixels[index])
             | (static_cast<uint32_t> (pixels[index + 1]) << 8)
             | (static_cast<uint32_t> (pixels[index + 2]) << 16)
             | (static_cast<uint32_t> (pixels[index + 3]) << 24);
    }

    /** Returns one pixel packed as 0xAARRGGBB, which is what Image and Color use. */
    uint32_t getPixelARGB (int x, int y) const noexcept
    {
        jassert (x >= 0 && x < width && y >= 0 && y < height);

        const size_t index = (static_cast<size_t> (y) * static_cast<size_t> (width) + static_cast<size_t> (x)) * 4u;

        return (static_cast<uint32_t> (pixels[index + 3]) << 24)
             | (static_cast<uint32_t> (pixels[index]) << 16)
             | (static_cast<uint32_t> (pixels[index + 1]) << 8)
             | static_cast<uint32_t> (pixels[index + 2]);
    }
};

//==============================================================================
/** Packs an RGBA quadruple the same way Bitmap::getPixel returns it. */
constexpr uint32_t packRGBA (uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept
{
    return static_cast<uint32_t> (r)
         | (static_cast<uint32_t> (g) << 8)
         | (static_cast<uint32_t> (b) << 16)
         | (static_cast<uint32_t> (a) << 24);
}

//==============================================================================
/** Reads an offscreen target back into a Bitmap.

    Returns an invalid Bitmap when the backend cannot read the target.

    @param device  The device that owns the target.
    @param target  The target to read.
    @param width   The target's width in pixels.
    @param height  The target's height in pixels.
*/
inline Bitmap readTarget (GpuDevice& device, OffscreenTarget& target, int width, int height)
{
    Bitmap result;

    if (width <= 0 || height <= 0)
        return result;

    std::vector<uint8_t> buffer (static_cast<size_t> (width) * static_cast<size_t> (height) * 4u);

    if (! device.readOffscreenPixels (target, buffer.data(), buffer.size()))
        return result;

    result.width = width;
    result.height = height;
    result.pixels = std::move (buffer);

    return result;
}

//==============================================================================
/** The outcome of comparing two bitmaps. */
struct BitmapDiff
{
    bool sameSize = false;         ///< Whether both bitmaps share dimensions.
    int64 differingPixels = 0;     ///< Pixels whose channels are not all within tolerance.
    int maxChannelDelta = 0;       ///< The largest absolute channel difference found.
    int firstDifferenceX = -1;     ///< X of the first differing pixel in scan order, or -1.
    int firstDifferenceY = -1;     ///< Y of the first differing pixel in scan order, or -1.

    /** Returns the fraction of differing pixels, from 0 to 1. */
    double getDifferingFraction (int width, int height) const noexcept
    {
        const auto total = static_cast<double> (width) * static_cast<double> (height);

        return total > 0.0 ? static_cast<double> (differingPixels) / total : 0.0;
    }
};

//==============================================================================
/** Compares two bitmaps channel by channel.

    A pixel counts as differing when any of its four channels is further than
    @a channelTolerance from its counterpart. Use a tolerance of zero for flat
    fills and clears, and a small tolerance for content with antialiased edges,
    where two rasterisers legitimately disagree on coverage.

    @param a                 The first bitmap.
    @param b                 The second bitmap.
    @param channelTolerance  The per-channel difference that still counts as equal.
*/
inline BitmapDiff compareBitmaps (const Bitmap& a, const Bitmap& b, int channelTolerance = 0)
{
    BitmapDiff diff;

    if (! a.isValid() || ! b.isValid() || a.width != b.width || a.height != b.height)
        return diff;

    diff.sameSize = true;

    for (int y = 0; y < a.height; ++y)
    {
        for (int x = 0; x < a.width; ++x)
        {
            const size_t index = (static_cast<size_t> (y) * static_cast<size_t> (a.width) + static_cast<size_t> (x)) * 4u;

            bool pixelDiffers = false;

            for (int channel = 0; channel < 4; ++channel)
            {
                const int delta = std::abs (static_cast<int> (a.pixels[index + static_cast<size_t> (channel)])
                                            - static_cast<int> (b.pixels[index + static_cast<size_t> (channel)]));

                diff.maxChannelDelta = std::max (diff.maxChannelDelta, delta);

                if (delta > channelTolerance)
                    pixelDiffers = true;
            }

            if (pixelDiffers)
            {
                if (diff.firstDifferenceX < 0)
                {
                    diff.firstDifferenceX = x;
                    diff.firstDifferenceY = y;
                }

                ++diff.differingPixels;
            }
        }
    }

    return diff;
}

//==============================================================================
/** Measures how much of a bitmap is locally flat.

    Returns the fraction of pixels identical to all four of their neighbours.
    Real rendered content is mostly flat fills and scores high, while
    uninitialised GPU memory has no two adjacent pixels alike and scores near
    zero. This is the same measure tools/check_screenshot.py applies to CI
    captures, so a test can reject a frame that was never actually presented
    without needing a reference image.
*/
inline double measureFlatFraction (const Bitmap& bitmap)
{
    if (! bitmap.isValid() || bitmap.width < 3 || bitmap.height < 3)
        return 0.0;

    const auto stride = static_cast<size_t> (bitmap.width) * 4u;
    const auto* bytes = bitmap.pixels.data();

    const auto samePixel = [bytes] (size_t lhs, size_t rhs)
    {
        return bytes[lhs] == bytes[rhs]
            && bytes[lhs + 1] == bytes[rhs + 1]
            && bytes[lhs + 2] == bytes[rhs + 2]
            && bytes[lhs + 3] == bytes[rhs + 3];
    };

    int64 flat = 0;

    for (int y = 1; y < bitmap.height - 1; ++y)
    {
        for (int x = 1; x < bitmap.width - 1; ++x)
        {
            const size_t index = static_cast<size_t> (y) * stride + static_cast<size_t> (x) * 4u;

            if (samePixel (index, index - 4)
                && samePixel (index, index + 4)
                && samePixel (index, index - stride)
                && samePixel (index, index + stride))
            {
                ++flat;
            }
        }
    }

    const auto interior = static_cast<double> (bitmap.width - 2) * static_cast<double> (bitmap.height - 2);

    return interior > 0.0 ? static_cast<double> (flat) / interior : 0.0;
}

//==============================================================================
/** Returns the directory failing tests write their artifacts into.

    Set YUP_TEST_ARTIFACT_DIR to collect them somewhere durable, which is what
    a CI job wants so the images survive the runner. Defaults to the system
    temporary directory.
*/
inline File getArtifactDirectory()
{
    if (auto fromEnvironment = SystemStats::getEnvironmentVariable ("YUP_TEST_ARTIFACT_DIR", {}); fromEnvironment.isNotEmpty())
        return File (fromEnvironment);

    return File::getSpecialLocation (File::tempDirectory);
}

/** Writes a bitmap to a PNG inside the artifact directory.

    @param bitmap  The bitmap to write.
    @param name    A file name stem, without an extension.

    @returns The file written, or an invalid File on failure.
*/
inline File writeBitmapArtifact (const Bitmap& bitmap, StringRef name)
{
#if YUP_IMAGE_FORMAT_PNG
    if (! bitmap.isValid())
        return {};

    auto directory = getArtifactDirectory();
    if (! directory.createDirectory())
        return {};

    auto file = directory.getChildFile (String (name) + ".png");
    file.deleteFile();

    Image image (bitmap.width, bitmap.height, PixelFormat::RGBA);

    // Image::setPixel takes 0xAARRGGBB, while Bitmap holds bytes in RGBA order.
    // Writing the packed bitmap value straight through swaps red and blue, which
    // makes the dumped artifact disagree with the failure message.
    for (int y = 0; y < bitmap.height; ++y)
        for (int x = 0; x < bitmap.width; ++x)
            image.setPixel (x, y, bitmap.getPixelARGB (x, y));

    auto stream = file.createOutputStream();
    if (stream == nullptr)
        return {};

    if (! PngImageFormatWriter (stream.release(), PixelFormat::RGBA).writeImage (image))
        return {};

    return file;
#else
    ignoreUnused (bitmap, name);
    return {};
#endif
}

//==============================================================================
/** Asserts that two bitmaps match, dumping both to PNG when they do not.

    @param actual            The bitmap under test.
    @param expected          The reference bitmap.
    @param name              A stem for the artifacts written on failure.
    @param channelTolerance  The per-channel difference that still counts as equal.
    @param maxDifferingFraction  The fraction of pixels allowed to differ.

    @returns An assertion result carrying the measured difference.
*/
inline ::testing::AssertionResult bitmapsMatch (const Bitmap& actual,
                                                const Bitmap& expected,
                                                StringRef name,
                                                int channelTolerance = 0,
                                                double maxDifferingFraction = 0.0)
{
    if (! actual.isValid())
        return ::testing::AssertionFailure() << "actual bitmap is invalid or was never read back";

    if (! expected.isValid())
        return ::testing::AssertionFailure() << "expected bitmap is invalid";

    const auto diff = compareBitmaps (actual, expected, channelTolerance);

    if (! diff.sameSize)
    {
        return ::testing::AssertionFailure()
            << "size mismatch: actual " << actual.width << "x" << actual.height
            << ", expected " << expected.width << "x" << expected.height;
    }

    const auto fraction = diff.getDifferingFraction (actual.width, actual.height);

    if (fraction <= maxDifferingFraction)
        return ::testing::AssertionSuccess();

    const auto actualFile = writeBitmapArtifact (actual, String (name) + "_actual");
    const auto expectedFile = writeBitmapArtifact (expected, String (name) + "_expected");

    auto failure = ::testing::AssertionFailure()
                 << diff.differingPixels << " of " << (actual.width * actual.height)
                 << " pixels differ (" << (fraction * 100.0) << "%, allowed "
                 << (maxDifferingFraction * 100.0) << "%)"
                 << ", largest channel delta " << diff.maxChannelDelta
                 << ", first at (" << diff.firstDifferenceX << ", " << diff.firstDifferenceY << ")";

    if (actualFile.exists())
        failure << "\n  actual:   " << actualFile.getFullPathName();

    if (expectedFile.exists())
        failure << "\n  expected: " << expectedFile.getFullPathName();

    return failure;
}

//==============================================================================
/** Asserts that every pixel of a bitmap holds the given RGBA value. */
inline ::testing::AssertionResult bitmapIsSolid (const Bitmap& bitmap, uint32_t expectedRGBA, StringRef name)
{
    if (! bitmap.isValid())
        return ::testing::AssertionFailure() << "bitmap is invalid or was never read back";

    for (int y = 0; y < bitmap.height; ++y)
    {
        for (int x = 0; x < bitmap.width; ++x)
        {
            const auto found = bitmap.getPixel (x, y);

            if (found == expectedRGBA)
                continue;

            auto file = writeBitmapArtifact (bitmap, String (name) + "_actual");

            auto failure = ::testing::AssertionFailure()
                         << "pixel (" << x << ", " << y << ") is 0x" << String::toHexString (static_cast<int> (found))
                         << ", expected 0x" << String::toHexString (static_cast<int> (expectedRGBA))
                         << " (both packed as 0xAABBGGRR)";

            if (file.exists())
                failure << "\n  actual: " << file.getFullPathName();

            return failure;
        }
    }

    return ::testing::AssertionSuccess();
}

} // namespace yup::test
