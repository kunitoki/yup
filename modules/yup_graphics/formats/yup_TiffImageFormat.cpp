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

#if YUP_IMAGE_FORMAT_TIFF

namespace yup
{

namespace
{

//==============================================================================
// I/O state for both reading and writing.
// The reader operates on a pre-filled buffer (the entire file in memory).
// The writer buffers everything in memory, then flushes to the output stream
// on close. This is required because libtiff needs random read access during
// writing (e.g. for Deflate compression to update directory offsets), and
// OutputStream::setPosition fails when seeking beyond the stream end.
//==============================================================================

struct TiffMemoryIO
{
    std::vector<uint8_t> buffer;
    size_t pos = 0;
    OutputStream* outputStream = nullptr; // non-null for write path
};

//==============================================================================
// Unified callbacks — read/write/seek operate on buffer; close flushes for write
//==============================================================================

static tmsize_t tiffReadProc (thandle_t handle, void* buf, tmsize_t size)
{
    auto* state = static_cast<TiffMemoryIO*> (handle);

    if (state == nullptr || buf == nullptr || size <= 0)
        return 0;

    if (state->pos >= state->buffer.size())
        return 0;

    const auto remaining = static_cast<tmsize_t> (state->buffer.size() - state->pos);
    const auto toRead = std::min (size, remaining);

    if (toRead > 0)
    {
        std::memcpy (buf, state->buffer.data() + state->pos, static_cast<size_t> (toRead));
        state->pos += static_cast<size_t> (toRead);
    }

    return toRead;
}

static tmsize_t tiffWriteProc (thandle_t handle, void* buf, tmsize_t size)
{
    auto* state = static_cast<TiffMemoryIO*> (handle);

    if (state == nullptr || buf == nullptr || size <= 0)
        return 0;

    const auto writePos = state->pos;
    const auto needed = writePos + static_cast<size_t> (size);

    if (needed > state->buffer.size())
        state->buffer.resize (needed);

    std::memcpy (state->buffer.data() + writePos, buf, static_cast<size_t> (size));
    state->pos = needed;

    return size;
}

static toff_t tiffSeekProc (thandle_t handle, toff_t off, int whence)
{
    auto* state = static_cast<TiffMemoryIO*> (handle);

    if (state == nullptr)
        return static_cast<toff_t> (-1);

    int64_t newPos = 0;

    switch (whence)
    {
        case SEEK_SET:
            newPos = static_cast<int64_t> (off);
            break;
        case SEEK_CUR:
            newPos = static_cast<int64_t> (state->pos) + static_cast<int64_t> (off);
            break;
        case SEEK_END:
            newPos = static_cast<int64_t> (state->buffer.size()) + static_cast<int64_t> (off);
            break;
        default:
            return static_cast<toff_t> (-1);
    }

    if (newPos < 0)
        return static_cast<toff_t> (-1);

    state->pos = static_cast<size_t> (newPos);

    // CRITICAL: must return the exact requested position for SeekOK to pass.
    // The buffer grows on-demand via tiffWriteProc when writing at newPos.
    return static_cast<toff_t> (newPos);
}

static int tiffCloseProc (thandle_t handle)
{
    auto* state = static_cast<TiffMemoryIO*> (handle);

    if (state != nullptr && state->outputStream != nullptr && ! state->buffer.empty())
    {
        state->outputStream->write (state->buffer.data(), static_cast<int> (state->buffer.size()));
        state->outputStream->flush();
    }

    return 0;
}

static toff_t tiffSizeProc (thandle_t handle)
{
    auto* state = static_cast<TiffMemoryIO*> (handle);
    return (state != nullptr) ? static_cast<toff_t> (state->buffer.size()) : 0;
}

static int tiffMapProc (thandle_t /*handle*/, void** /*base*/, toff_t* /*size*/)
{
    return 0;
}

static void tiffUnmapProc (thandle_t /*handle*/, void* /*base*/, toff_t /*size*/)
{
}

//==============================================================================
// Pixel format helpers
//==============================================================================

static int getSamplesPerPixel (PixelFormat fmt)
{
    switch (fmt)
    {
        case PixelFormat::Grayscale:
            return 1;
        case PixelFormat::RGB:
            return 3;
        case PixelFormat::RGBA:
            return 4;
    }

    return 0;
}

} // anonymous namespace

//==============================================================================
// TiffImageFormatReader
//==============================================================================

TiffImageFormatReader::TiffImageFormatReader (InputStream* stream, const ImageFormat::Options& options)
    : ImageFormatReader (stream, "TIFF Image", options)
{
    // Read entire stream into memory (libtiff needs random access)
    uint8 chunk[4096];
    long bytesRead;

    while ((bytesRead = input->read (chunk, sizeof (chunk))) > 0)
        fileData.insert (fileData.end(), chunk, chunk + bytesRead);

    if (fileData.size() < 4)
        return;

    parseHeader();
}

void TiffImageFormatReader::parseHeader()
{
    TiffMemoryIO state;
    state.buffer = fileData;
    state.pos = 0;

    auto* tif = TIFFClientOpen ("yup_tiff_read", "rm", &state, tiffReadProc, tiffWriteProc, tiffSeekProc, tiffCloseProc, tiffSizeProc, tiffMapProc, tiffUnmapProc);

    if (tif == nullptr)
        return;

    uint32_t w = 0, h = 0;
    uint16_t bitsPerSample = 0, samplesPerPixel = 0;
    uint16_t photoMetric = 0;
    uint16_t extraSampleCount = 0;
    uint16_t* extraSampleTypes = nullptr;

    TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &w);
    TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &h);
    TIFFGetField (tif, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
    TIFFGetField (tif, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);

    width = static_cast<int> (w);
    height = static_cast<int> (h);

    if (width <= 0 || height <= 0 || bitsPerSample != 8)
    {
        TIFFClose (tif);
        return;
    }

    // Determine pixel format
    if (samplesPerPixel == 1)
    {
        pixelFormat = PixelFormat::Grayscale;
    }
    else if (samplesPerPixel == 3)
    {
        pixelFormat = PixelFormat::RGB;
    }
    else if (samplesPerPixel >= 4)
    {
        // Check if this is RGBA with associated/unassociated alpha
        if (TIFFGetField (tif, TIFFTAG_EXTRASAMPLES, &extraSampleCount, &extraSampleTypes)
            && extraSampleCount > 0)
        {
            pixelFormat = PixelFormat::RGBA;
        }
        else
        {
            // Assume RGBA if 4 samples
            pixelFormat = PixelFormat::RGBA;
        }
    }

    if (getOptions().parseMetadata || getOptions().parseRawChunks)
        metadata = ImageMetadata::create();

    // Read DPI
    if (getOptions().parseMetadata)
    {
        float xRes = 0.0f, yRes = 0.0f;
        uint16_t resUnit = RESUNIT_INCH;

        if (TIFFGetField (tif, TIFFTAG_XRESOLUTION, &xRes))
        {
            TIFFGetField (tif, TIFFTAG_RESOLUTIONUNIT, &resUnit);

            if (resUnit == RESUNIT_INCH)
                metadata->dpiX = xRes;
            else if (resUnit == RESUNIT_CENTIMETER)
                metadata->dpiX = xRes * 2.54f;
        }

        if (TIFFGetField (tif, TIFFTAG_YRESOLUTION, &yRes))
        {
            if (resUnit == RESUNIT_INCH)
                metadata->dpiY = yRes;
            else if (resUnit == RESUNIT_CENTIMETER)
                metadata->dpiY = yRes * 2.54f;
        }

        if (metadata->dpiX > 0.0)
            metadata->textEntries.set ("dpiX", String (metadata->dpiX));
        if (metadata->dpiY > 0.0)
            metadata->textEntries.set ("dpiY", String (metadata->dpiY));

        // Read description
        char* description = nullptr;
        if (TIFFGetField (tif, TIFFTAG_IMAGEDESCRIPTION, &description) && description != nullptr)
            metadata->textEntries.set ("description", String (description));
    }

    // Additional text metadata
    if (getOptions().parseMetadata)
    {
        auto setField = [&] (uint32 tag, const String& key)
        {
            char* value = nullptr;
            if (TIFFGetField (tif, tag, &value) && value != nullptr)
                metadata->textEntries.set (key, String (value));
        };

        setField (TIFFTAG_ARTIST, "Artist");
        setField (TIFFTAG_COPYRIGHT, "Copyright");
        setField (TIFFTAG_DATETIME, "DateTime");
        setField (TIFFTAG_SOFTWARE, "Software");
        setField (TIFFTAG_MAKE, "Make");
        setField (TIFFTAG_MODEL, "Model");
    }

    // Raw binary chunks
    if (getOptions().parseRawChunks)
    {
        auto extractRawTag = [&] (uint32 tag, const String& key)
        {
            uint32 count = 0;
            void* rawData = nullptr;
            if (TIFFGetField (tif, tag, &count, &rawData) && rawData != nullptr && count > 0)
                metadata->rawChunks[key] = MemoryBlock (rawData, count);
        };

        extractRawTag (TIFFTAG_EXIFIFD, "tiff/exif");
        extractRawTag (TIFFTAG_GPSIFD, "tiff/gps");
        extractRawTag (TIFFTAG_ICCPROFILE, "tiff/icc");
        extractRawTag (TIFFTAG_XMLPACKET, "tiff/xmp");
        extractRawTag (TIFFTAG_RICHTIFFIPTC, "tiff/iptc");
    }

    TIFFClose (tif);
}

Image TiffImageFormatReader::readImage()
{
    if (width <= 0 || height <= 0 || fileData.empty())
        return {};

    TiffMemoryIO state;
    state.buffer = fileData;
    state.pos = 0;

    auto* tif = TIFFClientOpen ("yup_tiff_read", "rm", &state, tiffReadProc, tiffWriteProc, tiffSeekProc, tiffCloseProc, tiffSizeProc, tiffMapProc, tiffUnmapProc);

    if (tif == nullptr)
        return {};

    // Use TIFFReadRGBAImage for simple RGBA decoding (handles all sample formats)
    const auto w = static_cast<uint32_t> (width);
    const auto h = static_cast<uint32_t> (height);
    const auto rasterSize = static_cast<size_t> (w) * static_cast<size_t> (h);

    auto raster = std::make_unique<uint32_t[]> (rasterSize);

    if (! TIFFReadRGBAImageOriented (tif, w, h, raster.get(), ORIENTATION_TOPLEFT, 0))
    {
        TIFFClose (tif);
        return {};
    }

    TIFFClose (tif);

    Image image (width, height, pixelFormat);

    if (pixelFormat == PixelFormat::RGBA)
    {
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                const uint32_t rgba = raster[y * w + x];
                const uint8_t a = static_cast<uint8_t> (TIFFGetA (rgba));
                const uint8_t r = static_cast<uint8_t> (TIFFGetR (rgba));
                const uint8_t g = static_cast<uint8_t> (TIFFGetG (rgba));
                const uint8_t b = static_cast<uint8_t> (TIFFGetB (rgba));

                image.setPixel (x, y, (uint32 (a) << 24) | (uint32 (r) << 16) | (uint32 (g) << 8) | uint32 (b));
            }
        }
    }
    else if (pixelFormat == PixelFormat::RGB)
    {
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                const uint32_t rgba = raster[y * w + x];
                const uint8_t r = static_cast<uint8_t> (TIFFGetR (rgba));
                const uint8_t g = static_cast<uint8_t> (TIFFGetG (rgba));
                const uint8_t b = static_cast<uint8_t> (TIFFGetB (rgba));

                image.setPixel (x, y, (0xFFu << 24) | (uint32 (r) << 16) | (uint32 (g) << 8) | uint32 (b));
            }
        }
    }
    else // Grayscale
    {
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                const uint32_t rgba = raster[y * w + x];
                const uint8_t r = static_cast<uint8_t> (TIFFGetR (rgba));
                const uint8_t g = static_cast<uint8_t> (TIFFGetG (rgba));
                const uint8_t b = static_cast<uint8_t> (TIFFGetB (rgba));
                // Luminance from RGB
                const uint8_t gray = static_cast<uint8_t> ((r * 77 + g * 150 + b * 29) >> 8);

                image.setPixel (x, y, (0xFFu << 24) | (uint32 (gray) << 16) | (uint32 (gray) << 8) | uint32 (gray));
            }
        }
    }

    return image;
}

//==============================================================================
// TiffImageFormatWriter
//==============================================================================

TiffImageFormatWriter::TiffImageFormatWriter (OutputStream* stream, PixelFormat fmt)
    : ImageFormatWriter (stream, "TIFF Image", fmt)
{
}

bool TiffImageFormatWriter::writeImage (const Image& image)
{
    if (output == nullptr || ! image.isValid())
        return false;

    const int w = image.getWidth();
    const int h = image.getHeight();

    if (w <= 0 || h <= 0)
        return false;

    const auto fmt = getPixelFormat();
    const int samplesPerPixel = getSamplesPerPixel (fmt);

    TiffMemoryIO state;
    state.outputStream = output.get();

    auto* tif = TIFFClientOpen ("yup_tiff_write", "wm", &state, tiffReadProc, tiffWriteProc, tiffSeekProc, tiffCloseProc, tiffSizeProc, tiffMapProc, tiffUnmapProc);

    if (tif == nullptr)
        return false;

    // Configure TIFF fields
    TIFFSetField (tif, TIFFTAG_IMAGEWIDTH, static_cast<uint32_t> (w));
    TIFFSetField (tif, TIFFTAG_IMAGELENGTH, static_cast<uint32_t> (h));
    TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, static_cast<uint16_t> (samplesPerPixel));
    TIFFSetField (tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

    // Compression: Deflate
    TIFFSetField (tif, TIFFTAG_COMPRESSION, COMPRESSION_ADOBE_DEFLATE);

    // Photometric interpretation
    if (fmt == PixelFormat::Grayscale)
        TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    else if (fmt == PixelFormat::RGB)
        TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    else // RGBA
    {
        TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

        // Mark the extra sample as associated alpha
        const uint16_t extraSampleType = EXTRASAMPLE_ASSOCALPHA;
        TIFFSetField (tif, TIFFTAG_EXTRASAMPLES, 1, &extraSampleType);
    }

    // DPI
    TIFFSetField (tif, TIFFTAG_XRESOLUTION, 72.0f);
    TIFFSetField (tif, TIFFTAG_YRESOLUTION, 72.0f);
    TIFFSetField (tif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);

    // Write metadata from Image
    if (auto meta = image.getMetadata())
    {
        // DPI override
        if (meta->dpiX > 0.0)
        {
            TIFFSetField (tif, TIFFTAG_XRESOLUTION, static_cast<float> (meta->dpiX));
            TIFFSetField (tif, TIFFTAG_YRESOLUTION, static_cast<float> (meta->dpiY));
        }

        // Text metadata
        auto setText = [&] (uint32 tag, const String& key)
        {
            auto value = meta->textEntries.getValue (key, {});
            if (value.isNotEmpty())
                TIFFSetField (tif, tag, value.toRawUTF8());
        };

        setText (TIFFTAG_ARTIST, "Artist");
        setText (TIFFTAG_COPYRIGHT, "Copyright");
        setText (TIFFTAG_DATETIME, "DateTime");
        setText (TIFFTAG_SOFTWARE, "Software");
        setText (TIFFTAG_MAKE, "Make");
        setText (TIFFTAG_MODEL, "Model");
        setText (TIFFTAG_IMAGEDESCRIPTION, "description");
    }

    // Write scanlines
    const auto rowSize = static_cast<tmsize_t> (w) * samplesPerPixel;
    auto rowBuffer = std::make_unique<uint8_t[]> (static_cast<size_t> (rowSize));

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            const uint32_t argb = image.getPixel (x, y);
            auto* dst = rowBuffer.get() + x * samplesPerPixel;

            switch (fmt)
            {
                case PixelFormat::Grayscale:
                    // Luminance from RGB
                    dst[0] = static_cast<uint8_t> ((((argb >> 16) & 0xFF) * 77 + ((argb >> 8) & 0xFF) * 150 + (argb & 0xFF) * 29) >> 8);
                    break;

                case PixelFormat::RGB:
                    dst[0] = static_cast<uint8_t> ((argb >> 16) & 0xFF); // R
                    dst[1] = static_cast<uint8_t> ((argb >> 8) & 0xFF);  // G
                    dst[2] = static_cast<uint8_t> (argb & 0xFF);         // B
                    break;

                case PixelFormat::RGBA:
                    dst[0] = static_cast<uint8_t> ((argb >> 16) & 0xFF); // R
                    dst[1] = static_cast<uint8_t> ((argb >> 8) & 0xFF);  // G
                    dst[2] = static_cast<uint8_t> (argb & 0xFF);         // B
                    dst[3] = static_cast<uint8_t> ((argb >> 24) & 0xFF); // A
                    break;
            }
        }

        if (TIFFWriteScanline (tif, rowBuffer.get(), y, 0) < 0)
        {
            TIFFClose (tif);
            return false;
        }
    }

    TIFFClose (tif);
    return true;
}

//==============================================================================
// TiffImageFormat
//==============================================================================

TiffImageFormat::TiffImageFormat()
    : formatName ("TIFF Image")
{
}

const String& TiffImageFormat::getFormatName() const
{
    return formatName;
}

StringArray TiffImageFormat::getFileExtensions (Mode /*mode*/) const
{
    return { ".tiff", ".tif" };
}

bool TiffImageFormat::canHandleStream (InputStream& stream, Mode /*mode*/) const
{
    uint8 sig[4] = {};
    stream.read (sig, 4);
    stream.setPosition (0);

    // TIFF byte-order signatures:
    //   II: 0x49 0x49 0x2A 0x00 (little-endian)
    //   MM: 0x4D 0x4D 0x00 0x2A (big-endian)
    return (sig[0] == 0x49 && sig[1] == 0x49 && sig[2] == 0x2A && sig[3] == 0x00)
        || (sig[0] == 0x4D && sig[1] == 0x4D && sig[2] == 0x00 && sig[3] == 0x2A);
}

std::unique_ptr<ImageFormatReader> TiffImageFormat::createReaderFor (InputStream* sourceStream, const ImageFormat::Options& options)
{
    return std::make_unique<TiffImageFormatReader> (sourceStream, options);
}

std::unique_ptr<ImageFormatWriter> TiffImageFormat::createWriterFor (OutputStream* destStream,
                                                                     PixelFormat pixelFormat,
                                                                     const StringPairArray& /*metadataValues*/,
                                                                     int /*qualityOptionIndex*/)
{
    return std::make_unique<TiffImageFormatWriter> (destStream, pixelFormat);
}

Array<PixelFormat> TiffImageFormat::getPossiblePixelFormats() const
{
    return { PixelFormat::Grayscale, PixelFormat::RGB, PixelFormat::RGBA };
}

} // namespace yup

#endif // YUP_IMAGE_FORMAT_TIFF
