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

#if YUP_IMAGE_FORMAT_PNG

namespace yup
{

namespace
{

static void pngReadCallback (png_structp pngPtr, png_bytep data, png_size_t length)
{
    auto* stream = static_cast<InputStream*> (png_get_io_ptr (pngPtr));
    stream->read (data, static_cast<int> (length));
}

static void pngWriteCallback (png_structp pngPtr, png_bytep data, png_size_t length)
{
    auto* stream = static_cast<OutputStream*> (png_get_io_ptr (pngPtr));
    stream->write (data, static_cast<int> (length));
}

static void pngFlushCallback (png_structp pngPtr)
{
    // no-op for memory streams
    (void) pngPtr;
}

} // namespace

//==============================================================================
// PngImageFormatReader
//==============================================================================

PngImageFormatReader::PngImageFormatReader (InputStream* stream)
    : ImageFormatReader (stream, "PNG Image")
{
    auto* pngPtr = png_create_read_struct (PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (pngPtr == nullptr)
        return;

    auto* infoPtr = png_create_info_struct (pngPtr);
    if (infoPtr == nullptr)
    {
        png_destroy_read_struct (&pngPtr, nullptr, nullptr);
        return;
    }

    if (setjmp (png_jmpbuf (pngPtr)))
    {
        png_destroy_read_struct (&pngPtr, &infoPtr, nullptr);
        return;
    }

    png_set_read_fn (pngPtr, input.get(), pngReadCallback);
    png_read_info (pngPtr, infoPtr);

    width = static_cast<int> (png_get_image_width (pngPtr, infoPtr));
    height = static_cast<int> (png_get_image_height (pngPtr, infoPtr));

    auto colorType = png_get_color_type (pngPtr, infoPtr);
    auto bitDepth = png_get_bit_depth (pngPtr, infoPtr);

    // Normalize to 8-bit RGBA/RGB/Grayscale
    if (bitDepth == 16)
        png_set_strip_16 (pngPtr);
    if (colorType == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb (pngPtr);
    if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8)
        png_set_expand_gray_1_2_4_to_8 (pngPtr);
    if (png_get_valid (pngPtr, infoPtr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha (pngPtr);

    // Determine output format after transforms
    png_read_update_info (pngPtr, infoPtr);
    auto updatedColorType = png_get_color_type (pngPtr, infoPtr);

    if (updatedColorType == PNG_COLOR_TYPE_RGBA)
        pixelFormat = PixelFormat::RGBA;
    else if (updatedColorType == PNG_COLOR_TYPE_RGB)
        pixelFormat = PixelFormat::RGB;
    else if (updatedColorType == PNG_COLOR_TYPE_GRAY)
        pixelFormat = PixelFormat::Grayscale;
    else
        pixelFormat = PixelFormat::RGBA; // fallback

    // DPI from pHYs chunk
    png_uint_32 resX = 0, resY = 0;
    int unitType = 0;
    if (png_get_pHYs (pngPtr, infoPtr, &resX, &resY, &unitType) == PNG_INFO_pHYs)
    {
        if (unitType == PNG_RESOLUTION_METER)
        {
            dpiX = resX * 0.0254;
            dpiY = resY * 0.0254;
            metadataValues.set ("dpiX", String (dpiX));
            metadataValues.set ("dpiY", String (dpiY));
        }
    }

    // tEXt chunks
    png_textp textPtr = nullptr;
    int numText = 0;
    if (png_get_text (pngPtr, infoPtr, &textPtr, &numText) > 0)
    {
        for (int i = 0; i < numText; ++i)
            metadataValues.set (String (textPtr[i].key), String (textPtr[i].text));
    }

    png_destroy_read_struct (&pngPtr, &infoPtr, nullptr);
}

Image PngImageFormatReader::readImage()
{
    if (width <= 0 || height <= 0)
        return {};

    input->setPosition (0);

    auto* pngPtr = png_create_read_struct (PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (pngPtr == nullptr)
        return {};

    auto* infoPtr = png_create_info_struct (pngPtr);
    if (infoPtr == nullptr)
    {
        png_destroy_read_struct (&pngPtr, nullptr, nullptr);
        return {};
    }

    if (setjmp (png_jmpbuf (pngPtr)))
    {
        png_destroy_read_struct (&pngPtr, &infoPtr, nullptr);
        return {};
    }

    png_set_read_fn (pngPtr, input.get(), pngReadCallback);
    png_read_info (pngPtr, infoPtr);

    // Apply same transforms as constructor
    auto bitDepth = png_get_bit_depth (pngPtr, infoPtr);
    auto colorType = png_get_color_type (pngPtr, infoPtr);

    if (bitDepth == 16)
        png_set_strip_16 (pngPtr);
    if (colorType == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb (pngPtr);
    if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8)
        png_set_expand_gray_1_2_4_to_8 (pngPtr);
    if (png_get_valid (pngPtr, infoPtr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha (pngPtr);

    png_read_update_info (pngPtr, infoPtr);

    auto rowBytes = png_get_rowbytes (pngPtr, infoPtr);
    Image image (width, height, pixelFormat);

    std::vector<png_bytep> rowPointers (static_cast<size_t> (height));
    std::vector<std::vector<png_byte>> rowBuffers (static_cast<size_t> (height),
                                                   std::vector<png_byte> (rowBytes));
    for (int y = 0; y < height; ++y)
        rowPointers[static_cast<size_t> (y)] = rowBuffers[static_cast<size_t> (y)].data();

    png_read_image (pngPtr, rowPointers.data());

    // Convert rows to Image pixels
    for (int y = 0; y < height; ++y)
    {
        const png_byte* row = rowPointers[static_cast<size_t> (y)];

        if (pixelFormat == PixelFormat::RGBA)
        {
            for (int x = 0; x < width; ++x)
            {
                const png_byte* p = row + x * 4;
                image.setPixel (x, y, (uint32 (p[3]) << 24) | (uint32 (p[0]) << 16) | (uint32 (p[1]) << 8) | uint32 (p[2]));
            }
        }
        else if (pixelFormat == PixelFormat::RGB)
        {
            for (int x = 0; x < width; ++x)
            {
                const png_byte* p = row + x * 3;
                image.setPixel (x, y, 0xFF000000u | (uint32 (p[0]) << 16) | (uint32 (p[1]) << 8) | uint32 (p[2]));
            }
        }
        else // Grayscale
        {
            for (int x = 0; x < width; ++x)
            {
                uint32 g = row[static_cast<size_t> (x)];
                image.setPixel (x, y, 0xFF000000u | (g << 16) | (g << 8) | g);
            }
        }
    }

    png_destroy_read_struct (&pngPtr, &infoPtr, nullptr);
    return image;
}

//==============================================================================
// PngImageFormatWriter
//==============================================================================

PngImageFormatWriter::PngImageFormatWriter (OutputStream* stream, PixelFormat fmt)
    : ImageFormatWriter (stream, "PNG Image", fmt)
{
}

bool PngImageFormatWriter::writeImage (const Image& image)
{
    if (output == nullptr || ! image.isValid())
        return false;

    auto* pngPtr = png_create_write_struct (PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (pngPtr == nullptr)
        return false;

    auto* infoPtr = png_create_info_struct (pngPtr);
    if (infoPtr == nullptr)
    {
        png_destroy_write_struct (&pngPtr, nullptr);
        return false;
    }

    if (setjmp (png_jmpbuf (pngPtr)))
    {
        png_destroy_write_struct (&pngPtr, &infoPtr);
        return false;
    }

    png_set_write_fn (pngPtr, output.get(), pngWriteCallback, pngFlushCallback);

    int colorType;
    int channels;
    switch (getPixelFormat())
    {
        case PixelFormat::RGBA:
            colorType = PNG_COLOR_TYPE_RGBA;
            channels = 4;
            break;

        case PixelFormat::RGB:
            colorType = PNG_COLOR_TYPE_RGB;
            channels = 3;
            break;

        case PixelFormat::Grayscale:
            colorType = PNG_COLOR_TYPE_GRAY;
            channels = 1;
            break;

        default:
            colorType = PNG_COLOR_TYPE_RGBA;
            channels = 4;
            break;
    }

    png_set_IHDR (pngPtr, infoPtr, static_cast<png_uint_32> (image.getWidth()), static_cast<png_uint_32> (image.getHeight()), 8, colorType, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info (pngPtr, infoPtr);

    const int w = image.getWidth();
    const int h = image.getHeight();
    std::vector<png_byte> rowBuffer (static_cast<size_t> (w * channels));

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            auto argb = image.getPixel (x, y);
            auto* dst = rowBuffer.data() + x * channels;

            if (channels == 4)
            {
                dst[0] = static_cast<png_byte> ((argb >> 16) & 0xFF); // R
                dst[1] = static_cast<png_byte> ((argb >> 8) & 0xFF);  // G
                dst[2] = static_cast<png_byte> ((argb >> 0) & 0xFF);  // B
                dst[3] = static_cast<png_byte> ((argb >> 24) & 0xFF); // A
            }
            else if (channels == 3)
            {
                dst[0] = static_cast<png_byte> ((argb >> 16) & 0xFF);
                dst[1] = static_cast<png_byte> ((argb >> 8) & 0xFF);
                dst[2] = static_cast<png_byte> ((argb >> 0) & 0xFF);
            }
            else // grayscale
            {
                dst[0] = static_cast<png_byte> ((argb >> 16) & 0xFF); // use R channel
            }
        }

        png_write_row (pngPtr, rowBuffer.data());
    }

    png_write_end (pngPtr, infoPtr);
    png_destroy_write_struct (&pngPtr, &infoPtr);
    return true;
}

//==============================================================================
// PngImageFormat
//==============================================================================

PngImageFormat::PngImageFormat()
    : formatName ("PNG Image")
{
}

const String& PngImageFormat::getFormatName() const
{
    return formatName;
}

Array<String> PngImageFormat::getFileExtensions (Mode /*mode*/) const
{
    return { ".png" };
}

bool PngImageFormat::canHandleStream (InputStream& stream, Mode /*mode*/) const
{
    uint8 sig[8] = {};
    stream.read (sig, 8);
    stream.setPosition (0);
    return sig[0] == 0x89 && sig[1] == 'P' && sig[2] == 'N' && sig[3] == 'G'
        && sig[4] == '\r' && sig[5] == '\n' && sig[6] == 0x1a && sig[7] == '\n';
}

std::unique_ptr<ImageFormatReader> PngImageFormat::createReaderFor (InputStream* sourceStream)
{
    return std::make_unique<PngImageFormatReader> (sourceStream);
}

std::unique_ptr<ImageFormatWriter> PngImageFormat::createWriterFor (OutputStream* destStream,
                                                                    PixelFormat pixelFormat,
                                                                    const StringPairArray& /*metadataValues*/,
                                                                    int /*qualityOptionIndex*/)
{
    return std::make_unique<PngImageFormatWriter> (destStream, pixelFormat);
}

Array<PixelFormat> PngImageFormat::getPossiblePixelFormats() const
{
    return { PixelFormat::Grayscale, PixelFormat::RGB, PixelFormat::RGBA };
}

} // namespace yup

#endif // YUP_IMAGE_FORMAT_PNG
