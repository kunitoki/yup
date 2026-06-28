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

#include <yup_graphics/yup_graphics.h>

using namespace yup;

/** Generates a checkerboard test image.
    Even squares are red (ARGB: 0xFFFF0000), odd squares are blue (ARGB: 0xFF0000FF).
    Square size is 4 pixels. Works for any PixelFormat.
*/
inline Image generateTestImage (int w, int h, PixelFormat fmt)
{
    Image img (w, h, fmt);

    const bool useAlpha = (fmt == PixelFormat::RGBA);
    const uint32 even = useAlpha ? 0x80FF0000u : 0xFFFF0000u;
    const uint32 odd = useAlpha ? 0x800000FFu : 0xFF0000FFu;

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            bool isEven = ((x / 4) + (y / 4)) % 2 == 0;
            img.setPixel (x, y, isEven ? even : odd);
        }
    }

    return img;
}

/** Generates a solid-colour test image filled with the given ARGB value. */
inline Image generateSolidImage (int w, int h, PixelFormat fmt, uint32 argb = 0xFFFF0000u)
{
    Image img (w, h, fmt);
    img.fill (argb);
    return img;
}

/** Compares two images pixel-by-pixel (RGB channels only) within the given per-channel tolerance. */
inline bool imagesAreEqual (const Image& a, const Image& b, int tolerance = 0)
{
    if (a.getWidth() != b.getWidth() || a.getHeight() != b.getHeight())
        return false;

    if (a.getPixelFormat() != b.getPixelFormat())
        return false;

    for (int y = 0; y < a.getHeight(); ++y)
    {
        for (int x = 0; x < a.getWidth(); ++x)
        {
            uint32 pa = a.getPixel (x, y);
            uint32 pb = b.getPixel (x, y);

            if (std::abs (int ((pa >> 16) & 0xFF) - int ((pb >> 16) & 0xFF)) > tolerance)
                return false;
            if (std::abs (int ((pa >> 8) & 0xFF) - int ((pb >> 8) & 0xFF)) > tolerance)
                return false;
            if (std::abs (int ((pa >> 0) & 0xFF) - int ((pb >> 0) & 0xFF)) > tolerance)
                return false;
        }
    }

    return true;
}

/** Compares two RGBA images pixel-by-pixel (all 4 channels) within the given per-channel tolerance. */
inline bool imagesAreEqualRGBA (const Image& a, const Image& b, int tolerance = 0)
{
    if (a.getWidth() != b.getWidth() || a.getHeight() != b.getHeight())
        return false;

    if (a.getPixelFormat() != b.getPixelFormat())
        return false;

    for (int y = 0; y < a.getHeight(); ++y)
    {
        for (int x = 0; x < a.getWidth(); ++x)
        {
            uint32 pa = a.getPixel (x, y);
            uint32 pb = b.getPixel (x, y);

            if (std::abs (int ((pa >> 24) & 0xFF) - int ((pb >> 24) & 0xFF)) > tolerance)
                return false;
            if (std::abs (int ((pa >> 16) & 0xFF) - int ((pb >> 16) & 0xFF)) > tolerance)
                return false;
            if (std::abs (int ((pa >> 8) & 0xFF) - int ((pb >> 8) & 0xFF)) > tolerance)
                return false;
            if (std::abs (int ((pa >> 0) & 0xFF) - int ((pb >> 0) & 0xFF)) > tolerance)
                return false;
        }
    }

    return true;
}

/** Returns the tests/data/images/ directory. */
inline File getTestDataImagesDirectory()
{
    return File (__FILE__)
        .getParentDirectory()
        .getParentDirectory()
        .getChildFile ("data")
        .getChildFile ("images");
}

/** Returns the tests/ directory. */
inline File getTestDirectory()
{
    return File (__FILE__)
        .getParentDirectory()
        .getParentDirectory();
}

/** Ensures a test image exists on disk in tests/data/images/ by generating it if missing.
    Returns the File pointing to the image. */
inline File ensureTestImage (const String& filename, int w, int h, PixelFormat fmt, uint32 argb)
{
    auto dir = getTestDataImagesDirectory();
    dir.createDirectory();

    auto file = dir.getChildFile (filename);

    if (! file.existsAsFile())
    {
        Image img (w, h, fmt);
        img.fill (argb);

        auto* fos = file.createOutputStream().release();
        if (fos == nullptr)
            return file;

        auto ext = filename.fromLastOccurrenceOf (".", false, false).toLowerCase();

        if (ext == ".bmp")
            BmpImageFormatWriter (fos, fmt).writeImage (img);
        else if (ext == ".ppm" || ext == ".pgm")
            PpmImageFormatWriter (fos, fmt).writeImage (img);
#if YUP_IMAGE_FORMAT_PNG
        else if (ext == ".png")
            PngImageFormatWriter (fos, fmt).writeImage (img);
#endif
#if YUP_IMAGE_FORMAT_JPEG
        else if (ext == ".jpg" || ext == ".jpeg")
            JpegImageFormatWriter (fos, fmt, 0).writeImage (img);
#endif
#if YUP_IMAGE_FORMAT_WEBP
        else if (ext == ".webp")
            WebPImageFormatWriter (fos, fmt, 0).writeImage (img);
#endif
#if YUP_IMAGE_FORMAT_GIF
        else if (ext == ".gif")
            GifImageFormatWriter (fos, fmt).writeImage (img);
#endif
        else
            delete fos;
    }

    return file;
}

/** Writes an Image to a File using the format determined by the file extension. */
inline bool writeImageToFile (const Image& image, const File& file, int qualityIndex = 0)
{
    auto* fos = file.createOutputStream().release();
    if (fos == nullptr)
        return false;

    auto ext = file.getFileExtension().toLowerCase();

    if (ext == ".bmp")
        return BmpImageFormatWriter (fos, image.getPixelFormat()).writeImage (image);
    if (ext == ".ppm" || ext == ".pgm")
        return PpmImageFormatWriter (fos, image.getPixelFormat()).writeImage (image);
#if YUP_IMAGE_FORMAT_PNG
    if (ext == ".png")
        return PngImageFormatWriter (fos, image.getPixelFormat()).writeImage (image);
#endif
#if YUP_IMAGE_FORMAT_JPEG
    if (ext == ".jpg" || ext == ".jpeg")
        return JpegImageFormatWriter (fos, image.getPixelFormat(), qualityIndex).writeImage (image);
#endif
#if YUP_IMAGE_FORMAT_WEBP
    if (ext == ".webp")
        return WebPImageFormatWriter (fos, image.getPixelFormat(), qualityIndex).writeImage (image);
#endif
#if YUP_IMAGE_FORMAT_GIF
    if (ext == ".gif")
        return GifImageFormatWriter (fos, image.getPixelFormat()).writeImage (image);
#endif

    delete fos;
    return false;
}

/** Reads an Image from a File, auto-detecting the format from the extension. */
inline Image readImageFromFile (const File& file)
{
    auto* fis = file.createInputStream().release();
    if (fis == nullptr)
        return {};

    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto reader = manager.createReaderFor (fis);
    if (reader == nullptr)
        return {};

    return reader->readImage();
}
