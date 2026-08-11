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

#include <algorithm>
#include <csetjmp>
#include <cstdlib>

#if YUP_IMAGE_FORMAT_JPEG

namespace yup
{

namespace
{

struct JpegErrorManager
{
    jpeg_error_mgr publicFields;
    jmp_buf jumpBuffer;
};

static void jpegErrorExit (j_common_ptr info)
{
    auto* manager = reinterpret_cast<JpegErrorManager*> (info->err);
    longjmp (manager->jumpBuffer, 1);
}

static int getQualityValue (int qualityIndex)
{
    static constexpr int qualityValues[] = { 95, 85, 75, 60 };
    return qualityValues[std::clamp (qualityIndex, 0, 3)];
}

static uint8 getLuminanceFromARGB (uint32 argb)
{
    const auto red = (argb >> 16) & 0xFF;
    const auto green = (argb >> 8) & 0xFF;
    const auto blue = argb & 0xFF;

    return static_cast<uint8> ((red * 77 + green * 150 + blue * 29) >> 8);
}

} // namespace

//==============================================================================
// JpegImageFormatReader
//==============================================================================

JpegImageFormatReader::JpegImageFormatReader (InputStream* stream, const ImageFormat::Options& options)
    : ImageFormatReader (stream, "JPEG Image", options)
{
    uint8 chunk[4096];
    long bytesRead = 0;

    while ((bytesRead = input->read (chunk, sizeof (chunk))) > 0)
        fileData.insert (fileData.end(), chunk, chunk + bytesRead);

    if (fileData.empty())
        return;

    jpeg_decompress_struct info = {};
    JpegErrorManager errorManager = {};
    info.err = jpeg_std_error (&errorManager.publicFields);
    errorManager.publicFields.error_exit = jpegErrorExit;

    if (setjmp (errorManager.jumpBuffer))
    {
        jpeg_destroy_decompress (&info);
        return;
    }

    jpeg_create_decompress (&info);

    // Request marker saving for metadata extraction
    if (getOptions().parseRawChunks || getOptions().parseMetadata)
    {
        jpeg_save_markers (&info, JPEG_COM, 0xFFFF);
        for (int i = 0; i < 16; ++i)
            jpeg_save_markers (&info, JPEG_APP0 + i, 0xFFFF);
    }

    jpeg_mem_src (&info, fileData.data(), static_cast<unsigned long> (fileData.size()));
    jpeg_read_header (&info, TRUE);

    width = static_cast<int> (info.image_width);
    height = static_cast<int> (info.image_height);
    pixelFormat = (info.num_components == 1) ? PixelFormat::Grayscale : PixelFormat::RGB;

    if (getOptions().parseMetadata || getOptions().parseRawChunks)
        metadata = ImageMetadata::create();

    if (getOptions().parseMetadata)
    {
        if (info.density_unit == 1)
        {
            metadata->dpiX = info.X_density;
            metadata->dpiY = info.Y_density;
        }
        else if (info.density_unit == 2)
        {
            metadata->dpiX = info.X_density * 2.54;
            metadata->dpiY = info.Y_density * 2.54;
        }

        if (metadata->dpiX > 0.0)
            metadata->textEntries.set ("dpiX", String (metadata->dpiX));
        if (metadata->dpiY > 0.0)
            metadata->textEntries.set ("dpiY", String (metadata->dpiY));
    }

    // Extract markers for metadata
    if (getOptions().parseRawChunks || getOptions().parseMetadata)
    {
        for (auto* marker = info.marker_list; marker != nullptr; marker = marker->next)
        {
            const auto* data = marker->data;
            auto dataLength = marker->data_length;

            if (marker->marker == JPEG_COM)
            {
                if (getOptions().parseRawChunks)
                    metadata->rawChunks["jpeg/comment"] = MemoryBlock (data, dataLength);
                if (getOptions().parseMetadata)
                    metadata->textEntries.set ("Comment", String::createStringFromData (reinterpret_cast<const char*> (data), static_cast<int> (dataLength)));
            }
            else if (marker->marker == JPEG_APP0 + 1 && dataLength > 6
                     && std::memcmp (data, "Exif\0\0", 6) == 0)
            {
                if (getOptions().parseRawChunks)
                    metadata->rawChunks["jpeg/exif"] = MemoryBlock (data + 6, dataLength - 6);
            }
            else if (marker->marker == JPEG_APP0 + 1 && dataLength > 29
                     && std::memcmp (data, "http://ns.adobe.com/xap/1.0/", 29) == 0)
            {
                if (getOptions().parseRawChunks)
                    metadata->rawChunks["jpeg/xmp"] = MemoryBlock (data, dataLength);
            }
            else if (marker->marker == JPEG_APP0 + 2 && dataLength > 12
                     && std::memcmp (data, "ICC_PROFILE\0", 12) == 0)
            {
                if (getOptions().parseRawChunks)
                    metadata->rawChunks["jpeg/icc"] = MemoryBlock (data, dataLength);
            }
            else if (marker->marker == JPEG_APP0 && getOptions().parseRawChunks)
            {
                metadata->rawChunks["jpeg/jfif"] = MemoryBlock (data, dataLength);
            }
        }
    }

    jpeg_destroy_decompress (&info);
}

Image JpegImageFormatReader::readImage()
{
    if (width <= 0 || height <= 0 || fileData.empty())
        return {};

    jpeg_decompress_struct info = {};
    JpegErrorManager errorManager = {};
    info.err = jpeg_std_error (&errorManager.publicFields);
    errorManager.publicFields.error_exit = jpegErrorExit;

    if (setjmp (errorManager.jumpBuffer))
    {
        jpeg_destroy_decompress (&info);
        return {};
    }

    jpeg_create_decompress (&info);
    jpeg_mem_src (&info, fileData.data(), static_cast<unsigned long> (fileData.size()));
    jpeg_read_header (&info, TRUE);

    info.out_color_space = (pixelFormat == PixelFormat::Grayscale) ? JCS_GRAYSCALE : JCS_RGB;

    jpeg_start_decompress (&info);

    const int decodedWidth = static_cast<int> (info.output_width);
    const int decodedHeight = static_cast<int> (info.output_height);
    const int components = static_cast<int> (info.output_components);

    if (decodedWidth <= 0 || decodedHeight <= 0 || (components != 1 && components != 3))
    {
        jpeg_finish_decompress (&info);
        jpeg_destroy_decompress (&info);
        return {};
    }

    Image image (decodedWidth, decodedHeight, components == 1 ? PixelFormat::Grayscale : PixelFormat::RGB);
    std::vector<uint8> row (static_cast<size_t> (decodedWidth * components));

    while (info.output_scanline < info.output_height)
    {
        const int y = static_cast<int> (info.output_scanline);
        JSAMPROW rowPointer = row.data();
        jpeg_read_scanlines (&info, &rowPointer, 1);

        if (components == 1)
        {
            for (int x = 0; x < decodedWidth; ++x)
            {
                const auto g = uint32 (row[static_cast<size_t> (x)]);
                image.setPixel (x, y, 0xFF000000u | (g << 16) | (g << 8) | g);
            }
        }
        else
        {
            for (int x = 0; x < decodedWidth; ++x)
            {
                const auto* p = row.data() + (static_cast<size_t> (x) * 3);
                image.setPixel (x, y, 0xFF000000u | (uint32 (p[0]) << 16) | (uint32 (p[1]) << 8) | uint32 (p[2]));
            }
        }
    }

    jpeg_finish_decompress (&info);
    jpeg_destroy_decompress (&info);
    return image;
}

//==============================================================================
// JpegImageFormatWriter
//==============================================================================

JpegImageFormatWriter::JpegImageFormatWriter (OutputStream* stream, PixelFormat fmt, int qualityIndex_)
    : ImageFormatWriter (stream, "JPEG Image", fmt)
    , qualityIndex (qualityIndex_)
{
}

bool JpegImageFormatWriter::writeImage (const Image& image)
{
    if (output == nullptr || ! image.isValid())
        return false;

    const int width = image.getWidth();
    const int height = image.getHeight();

    if (width <= 0 || height <= 0)
        return false;

    const bool isGrayscale = getPixelFormat() == PixelFormat::Grayscale
                          || image.getPixelFormat() == PixelFormat::Grayscale;
    const int components = isGrayscale ? 1 : 3;
    const int stride = width * components;

    std::vector<uint8> rowBuffer (static_cast<size_t> (stride));

    jpeg_compress_struct info = {};
    JpegErrorManager errorManager = {};
    info.err = jpeg_std_error (&errorManager.publicFields);
    errorManager.publicFields.error_exit = jpegErrorExit;

    unsigned char* encoded = nullptr;
    unsigned long encodedSize = 0;

    if (setjmp (errorManager.jumpBuffer))
    {
        jpeg_destroy_compress (&info);
        std::free (encoded);
        return false;
    }

    jpeg_create_compress (&info);
    jpeg_mem_dest (&info, &encoded, &encodedSize);

    info.image_width = static_cast<JDIMENSION> (width);
    info.image_height = static_cast<JDIMENSION> (height);
    info.input_components = components;
    info.in_color_space = isGrayscale ? JCS_GRAYSCALE : JCS_RGB;

    jpeg_set_defaults (&info);
    jpeg_set_quality (&info, getQualityValue (qualityIndex), TRUE);

    // Write DPI from metadata
    if (auto meta = image.getMetadata())
    {
        if (meta->dpiX > 0.0)
        {
            info.density_unit = 1; // dots per inch
            info.X_density = static_cast<UINT16> (meta->dpiX + 0.5);
            info.Y_density = static_cast<UINT16> (meta->dpiY + 0.5);
        }
    }

    jpeg_start_compress (&info, TRUE);

    // Write metadata markers
    if (auto meta = image.getMetadata())
    {
        auto writeMarker = [&] (int marker, const void* data, unsigned int length)
        {
            jpeg_write_marker (&info, marker, static_cast<const JOCTET*> (data), length);
        };

        // EXIF (APP1)
        if (auto* exif = meta->getRawChunk ("jpeg/exif"))
        {
            // Prepend "Exif\0\0" header
            std::vector<uint8> exifData (6 + exif->getSize());
            std::memcpy (exifData.data(), "Exif\0\0", 6);
            std::memcpy (exifData.data() + 6, exif->getData(), exif->getSize());
            writeMarker (JPEG_APP0 + 1, exifData.data(), static_cast<unsigned int> (exifData.size()));
        }

        // XMP (APP1)
        if (auto* xmp = meta->getRawChunk ("jpeg/xmp"))
            writeMarker (JPEG_APP0 + 1, xmp->getData(), static_cast<unsigned int> (xmp->getSize()));

        // ICC (APP2)
        if (auto* icc = meta->getRawChunk ("jpeg/icc"))
            writeMarker (JPEG_APP0 + 2, icc->getData(), static_cast<unsigned int> (icc->getSize()));

        // Comment
        if (auto comment = meta->textEntries.getValue ("Comment", {}); comment.isNotEmpty())
            writeMarker (JPEG_COM, comment.toRawUTF8(), static_cast<unsigned int> (comment.getNumBytesAsUTF8()));
    }

    while (info.next_scanline < info.image_height)
    {
        const int y = static_cast<int> (info.next_scanline);

        if (isGrayscale)
        {
            for (int x = 0; x < width; ++x)
                rowBuffer[static_cast<size_t> (x)] = getLuminanceFromARGB (image.getPixel (x, y));
        }
        else
        {
            for (int x = 0; x < width; ++x)
            {
                const auto argb = image.getPixel (x, y);
                auto* dst = rowBuffer.data() + (static_cast<size_t> (x) * 3);
                dst[0] = static_cast<uint8> ((argb >> 16) & 0xFF);
                dst[1] = static_cast<uint8> ((argb >> 8) & 0xFF);
                dst[2] = static_cast<uint8> (argb & 0xFF);
            }
        }

        JSAMPROW rowPointer = rowBuffer.data();
        jpeg_write_scanlines (&info, &rowPointer, 1);
    }

    jpeg_finish_compress (&info);

    const bool ok = encoded != nullptr && encodedSize > 0 && output->write (encoded, encodedSize);

    jpeg_destroy_compress (&info);
    std::free (encoded);
    return ok;
}

//==============================================================================
// JpegImageFormat
//==============================================================================

JpegImageFormat::JpegImageFormat()
    : formatName ("JPEG Image")
{
}

const String& JpegImageFormat::getFormatName() const
{
    return formatName;
}

StringArray JpegImageFormat::getFileExtensions (Mode /*mode*/) const
{
    return { ".jpg", ".jpeg", ".jpe" };
}

bool JpegImageFormat::canHandleStream (InputStream& stream, Mode /*mode*/) const
{
    uint8 sig[3] = {};
    stream.read (sig, 3);
    stream.setPosition (0);

    return sig[0] == 0xFF && sig[1] == 0xD8 && sig[2] == 0xFF;
}

StringArray JpegImageFormat::getQualityOptions() const
{
    return { "Quality 95", "Quality 85", "Quality 75", "Quality 60" };
}

std::unique_ptr<ImageFormatReader> JpegImageFormat::createReaderFor (InputStream* sourceStream, const ImageFormat::Options& options)
{
    return std::make_unique<JpegImageFormatReader> (sourceStream, options);
}

std::unique_ptr<ImageFormatWriter> JpegImageFormat::createWriterFor (OutputStream* destStream,
                                                                     PixelFormat pixelFormat,
                                                                     const StringPairArray& /*metadataValues*/,
                                                                     int qualityOptionIndex)
{
    return std::make_unique<JpegImageFormatWriter> (destStream, pixelFormat, qualityOptionIndex);
}

Array<PixelFormat> JpegImageFormat::getPossiblePixelFormats() const
{
    return { PixelFormat::Grayscale, PixelFormat::RGB, PixelFormat::RGBA };
}

} // namespace yup

#endif // YUP_IMAGE_FORMAT_JPEG
