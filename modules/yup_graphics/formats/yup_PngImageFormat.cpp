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

#include <zlib.h>

namespace yup
{

namespace
{

//==============================================================================
// Stream-based callbacks for libpng I/O
//==============================================================================

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
    (void) pngPtr;
}

//==============================================================================
// Memory-based callbacks for libpng I/O
//==============================================================================

struct MemoryReadState
{
    const uint8_t* data = nullptr;
    size_t size = 0;
    size_t pos = 0;
};

static void memoryReadCallback (png_structp pngPtr, png_bytep data, png_size_t length)
{
    auto* state = static_cast<MemoryReadState*> (png_get_io_ptr (pngPtr));
    const size_t remaining = state->size - state->pos;
    const size_t toRead = std::min (static_cast<size_t> (length), remaining);
    std::memcpy (data, state->data + state->pos, toRead);
    state->pos += toRead;

    if (toRead < static_cast<size_t> (length))
        std::memset (data + toRead, 0, static_cast<size_t> (length) - toRead);
}

struct MemoryWriteState
{
    std::vector<uint8_t>* buffer = nullptr;
};

static void memoryWriteCallback (png_structp pngPtr, png_bytep data, png_size_t length)
{
    auto* state = static_cast<MemoryWriteState*> (png_get_io_ptr (pngPtr));
    state->buffer->insert (state->buffer->end(), data, data + length);
}

//==============================================================================
// APNG chunk helpers
//==============================================================================

static uint32_t readBE32 (const uint8_t* p)
{
    return (static_cast<uint32_t> (p[0]) << 24)
         | (static_cast<uint32_t> (p[1]) << 16)
         | (static_cast<uint32_t> (p[2]) << 8)
         | static_cast<uint32_t> (p[3]);
}

static uint16_t readBE16 (const uint8_t* p)
{
    return (static_cast<uint16_t> (p[0]) << 8)
         | static_cast<uint16_t> (p[1]);
}

static void writeBE32 (uint32_t v, uint8_t* p)
{
    p[0] = static_cast<uint8_t> ((v >> 24) & 0xFF);
    p[1] = static_cast<uint8_t> ((v >> 16) & 0xFF);
    p[2] = static_cast<uint8_t> ((v >> 8) & 0xFF);
    p[3] = static_cast<uint8_t> (v & 0xFF);
}

static uint32_t chunkCRC (const uint8_t* type, const uint8_t* data, size_t dataLen)
{
    auto crc = static_cast<uint32_t> (crc32 (0u, type, 4));
    if (dataLen > 0)
        crc = static_cast<uint32_t> (crc32 (crc, data, static_cast<uInt> (dataLen)));
    return crc;
}

// Build a chunk header + data + CRC into a buffer
static void writeChunk (std::vector<uint8_t>& buf, const char type[4], const uint8_t* data, size_t dataLen)
{
    uint8_t lenBE[4];
    writeBE32 (static_cast<uint32_t> (dataLen), lenBE);
    buf.insert (buf.end(), lenBE, lenBE + 4);
    buf.insert (buf.end(), reinterpret_cast<const uint8_t*> (type), reinterpret_cast<const uint8_t*> (type) + 4);

    if (dataLen > 0)
        buf.insert (buf.end(), data, data + dataLen);

    const auto crc = chunkCRC (reinterpret_cast<const uint8_t*> (type), data, dataLen);
    uint8_t crcBE[4];
    writeBE32 (crc, crcBE);
    buf.insert (buf.end(), crcBE, crcBE + 4);
}

// Build an IHDR chunk data payload (13 bytes)
static void buildIHDRData (uint32_t w, uint32_t h, uint8_t colorType, std::vector<uint8_t>& out)
{
    out.resize (13);
    writeBE32 (w, out.data());
    writeBE32 (h, out.data() + 4);
    out[8] = 8; // bit depth
    out[9] = colorType;
    out[10] = 0; // compression
    out[11] = 0; // filter
    out[12] = 0; // interlace
}

// APNG dispose operations (fcTL byte 24)
constexpr int kApngDisposeNone = 0;
constexpr int kApngDisposeBackground = 1;
constexpr int kApngDisposePrevious = 2;

// APNG blend operations (fcTL byte 25)
constexpr int kApngBlendSource = 0;
constexpr int kApngBlendOver = 1;
} // anonymous namespace

//==============================================================================
// PngImageFormatReader
//==============================================================================

PngImageFormatReader::PngImageFormatReader (InputStream* stream, const ImageFormat::Options& options)
    : ImageFormatReader (stream, "PNG Image", options)
{
    // Read entire stream into memory for chunk-level parsing
    uint8 chunk[4096];
    long bytesRead;

    while ((bytesRead = input->read (chunk, sizeof (chunk))) > 0)
        fileData.insert (fileData.end(), chunk, chunk + bytesRead);

    if (fileData.size() < 8)
        return;

    // First, parse using libpng to get the basic header info (width, height, format, DPI, metadata)
    parseHeader();

    if (width <= 0 || height <= 0)
        return;

    // Parse chunks for APNG frames and metadata
    parseChunks();

    if (isApng)
    {
        // For APNG, always use RGBA for compositing
        pixelFormat = PixelFormat::RGBA;

        if (width > 0 && height > 0)
        {
            canvas = Image (width, height, PixelFormat::RGBA);
            canvas.fill (0x00000000u);
        }
    }
}

void PngImageFormatReader::parseHeader()
{
    input->setPosition (0);

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

    // Capture original color info before transforms (needed for APNG frame decoding)
    apngOriginalColorType = colorType;
    apngOriginalBitDepth = bitDepth;

    // Capture PLTE and tRNS chunk data for APNG frame decoding
    if (colorType == PNG_COLOR_TYPE_PALETTE)
    {
        png_colorp palette = nullptr;
        int numPalette = 0;

        if (png_get_PLTE (pngPtr, infoPtr, &palette, &numPalette) == PNG_INFO_PLTE && numPalette > 0)
        {
            apngPLTEData.assign (reinterpret_cast<const uint8_t*> (palette),
                                 reinterpret_cast<const uint8_t*> (palette) + static_cast<size_t> (numPalette) * 3);
        }
    }

    // Capture tRNS for all color types (not just palette)
    {
        png_bytep transAlpha = nullptr;
        int numTrans = 0;
        png_color_16p transColor = nullptr;

        if (png_get_tRNS (pngPtr, infoPtr, &transAlpha, &numTrans, &transColor) == PNG_INFO_tRNS)
        {
            if (colorType == PNG_COLOR_TYPE_PALETTE && numTrans > 0)
            {
                // Palette tRNS: raw alpha bytes
                apngTRNSData.assign (transAlpha, transAlpha + numTrans);
            }
            else if (colorType == PNG_COLOR_TYPE_GRAY && transColor != nullptr)
            {
                // Grayscale tRNS: 2-byte big-endian gray value
                apngTRNSData.resize (2);
                apngTRNSData[0] = static_cast<uint8_t> ((transColor->gray >> 8) & 0xFF);
                apngTRNSData[1] = static_cast<uint8_t> (transColor->gray & 0xFF);
            }
            else if ((colorType == PNG_COLOR_TYPE_RGB || colorType == PNG_COLOR_TYPE_RGBA)
                     && transColor != nullptr)
            {
                // RGB tRNS: 2 bytes per channel (R, G, B), big-endian
                apngTRNSData.resize (6);
                apngTRNSData[0] = static_cast<uint8_t> ((transColor->red >> 8) & 0xFF);
                apngTRNSData[1] = static_cast<uint8_t> (transColor->red & 0xFF);
                apngTRNSData[2] = static_cast<uint8_t> ((transColor->green >> 8) & 0xFF);
                apngTRNSData[3] = static_cast<uint8_t> (transColor->green & 0xFF);
                apngTRNSData[4] = static_cast<uint8_t> ((transColor->blue >> 8) & 0xFF);
                apngTRNSData[5] = static_cast<uint8_t> (transColor->blue & 0xFF);
            }
        }
    }

    if (bitDepth == 16)
        png_set_strip_16 (pngPtr);
    if (colorType == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb (pngPtr);
    if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8)
        png_set_expand_gray_1_2_4_to_8 (pngPtr);
    if (png_get_valid (pngPtr, infoPtr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha (pngPtr);
    if (colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb (pngPtr);

    png_read_update_info (pngPtr, infoPtr);
    auto updatedColorType = png_get_color_type (pngPtr, infoPtr);

    if (updatedColorType == PNG_COLOR_TYPE_RGBA)
        pixelFormat = PixelFormat::RGBA;
    else if (updatedColorType == PNG_COLOR_TYPE_RGB)
        pixelFormat = PixelFormat::RGB;
    else if (updatedColorType == PNG_COLOR_TYPE_GRAY)
        pixelFormat = PixelFormat::Grayscale;
    else
        pixelFormat = PixelFormat::RGBA;

    if (getOptions().parseMetadata || getOptions().parseRawChunks)
        metadata = ImageMetadata::create();

    png_uint_32 resX = 0, resY = 0;
    int unitType = 0;
    if (getOptions().parseMetadata && png_get_pHYs (pngPtr, infoPtr, &resX, &resY, &unitType) == PNG_INFO_pHYs)
    {
        if (unitType == PNG_RESOLUTION_METER)
        {
            metadata->dpiX = resX * 0.0254;
            metadata->dpiY = resY * 0.0254;
            metadata->textEntries.set ("dpiX", String (metadata->dpiX));
            metadata->textEntries.set ("dpiY", String (metadata->dpiY));
        }
    }

    png_textp textPtr = nullptr;
    int numText = 0;
    if (getOptions().parseMetadata && png_get_text (pngPtr, infoPtr, &textPtr, &numText) > 0)
    {
        for (int i = 0; i < numText; ++i)
            metadata->textEntries.set (String::fromUTF8 (textPtr[i].key), String::fromUTF8 (textPtr[i].text));
    }

    png_destroy_read_struct (&pngPtr, &infoPtr, nullptr);
}

void PngImageFormatReader::parseChunks()
{
    // Start after the 8-byte PNG signature
    size_t pos = 8;
    uint32_t numFrames = 0;
    uint32_t numPlays = 0;
    bool foundAcTL = false;
    int frameIndex = -1;

    // First pass: find acTL and fcTL chunks, and extract metadata
    while (pos + 8 <= fileData.size())
    {
        const auto chunkLen = readBE32 (fileData.data() + pos);
        pos += 4;

        if (pos + 4 > fileData.size())
            break;

        const char* type = reinterpret_cast<const char*> (fileData.data() + pos);
        pos += 4;

        if (pos + chunkLen > fileData.size())
            break;

        const auto* chunkData = fileData.data() + pos;

        // Check for acTL (animation control)
        if (std::memcmp (type, "acTL", 4) == 0 && chunkLen >= 8)
        {
            numFrames = readBE32 (chunkData);
            numPlays = readBE32 (chunkData + 4);
            foundAcTL = true;
        }
        // Metadata chunk extraction
        else if (getOptions().parseRawChunks || getOptions().parseMetadata)
        {
            const bool ancillary = (type[0] & 0x20) != 0; // bit 5 set = ancillary

            if (getOptions().parseMetadata)
            {
                // tIME chunk
                if (std::memcmp (type, "tIME", 4) == 0 && chunkLen >= 7)
                {
                    char buf[32];
                    snprintf (buf, sizeof (buf), "%04d:%02d:%02d %02d:%02d:%02d", readBE16 (chunkData), chunkData[2], chunkData[3], chunkData[4], chunkData[5], chunkData[6]);
                    metadata->textEntries.set ("png/time", String (buf));
                }
                // sRGB chunk
                else if (std::memcmp (type, "sRGB", 4) == 0 && chunkLen >= 1)
                {
                    metadata->textEntries.set ("png/sRGB", String (static_cast<int> (chunkData[0])));
                }
                // gAMA chunk
                else if (std::memcmp (type, "gAMA", 4) == 0 && chunkLen >= 4)
                {
                    auto gamma = static_cast<double> (readBE32 (chunkData)) / 100000.0;
                    metadata->textEntries.set ("png/gamma", String (gamma));
                }
            }

            if (getOptions().parseRawChunks)
            {
                String chunkKey;

                // iCCP chunk
                if (std::memcmp (type, "iCCP", 4) == 0)
                    chunkKey = "png/iCCP";
                // cHRM chunk
                else if (std::memcmp (type, "cHRM", 4) == 0)
                    chunkKey = "png/cHRM";
                // eXIf chunk
                else if (std::memcmp (type, "eXIf", 4) == 0)
                    chunkKey = "png/eXIf";
                // Unknown ancillary chunks
                else if (ancillary && std::memcmp (type, "acTL", 4) != 0
                         && std::memcmp (type, "fcTL", 4) != 0
                         && std::memcmp (type, "fdAT", 4) != 0
                         && std::memcmp (type, "IDAT", 4) != 0
                         && std::memcmp (type, "IEND", 4) != 0)
                {
                    chunkKey = "png/chunk_" + String (type[0]) + type[1] + type[2] + type[3];
                }

                if (chunkKey.isNotEmpty())
                    metadata->rawChunks[std::move (chunkKey)] = MemoryBlock (chunkData, chunkLen);
            }
        }

        pos += chunkLen + 4; // skip data + CRC
    }

    if (! foundAcTL || numFrames < 1)
    {
        isApng = false;
        return;
    }

    isApng = true;
    loopCount = static_cast<int> (numPlays);
    frames.reserve (numFrames);

    // Second pass: collect frame data
    pos = 8;
    frameIndex = -1;

    while (pos + 8 <= fileData.size())
    {
        const auto chunkLen = readBE32 (fileData.data() + pos);
        pos += 4;

        if (pos + 4 > fileData.size())
            break;

        const char* type = reinterpret_cast<const char*> (fileData.data() + pos);
        pos += 4;

        if (pos + chunkLen > fileData.size())
            break;

        const auto* chunkData = fileData.data() + pos;

        if (std::memcmp (type, "fcTL", 4) == 0 && chunkLen >= 26)
        {
            // New frame starts
            ++frameIndex;

            FrameInfo info;
            // Skip sequence_number (4 bytes)
            info.frameWidth = static_cast<int> (readBE32 (chunkData + 4));
            info.frameHeight = static_cast<int> (readBE32 (chunkData + 8));
            info.xOffset = static_cast<int> (readBE32 (chunkData + 12));
            info.yOffset = static_cast<int> (readBE32 (chunkData + 16));

            const auto delayNum = readBE16 (chunkData + 20);
            const auto delayDen = readBE16 (chunkData + 22);
            if (delayDen == 0)
                info.delayMs = static_cast<int> (delayNum) * 10; // 100fps default
            else
                info.delayMs = static_cast<int> (delayNum) * 1000 / static_cast<int> (delayDen);

            info.disposeOp = static_cast<int> (chunkData[24]);
            info.blendOp = static_cast<int> (chunkData[25]);

            frames.push_back (std::move (info));
        }
        else if (std::memcmp (type, "IDAT", 4) == 0 || std::memcmp (type, "fdAT", 4) == 0)
        {
            // If no fcTL seen yet for frame 0, create a default frame
            if (frameIndex < 0)
            {
                frameIndex = 0;
                FrameInfo defaultFrame;
                defaultFrame.frameWidth = width;
                defaultFrame.frameHeight = height;
                defaultFrame.xOffset = 0;
                defaultFrame.yOffset = 0;
                defaultFrame.delayMs = 0;
                defaultFrame.disposeOp = kApngDisposeNone;
                defaultFrame.blendOp = kApngBlendSource;
                frames.push_back (std::move (defaultFrame));
            }

            if (static_cast<size_t> (frameIndex) < frames.size())
            {
                auto& frame = frames[static_cast<size_t> (frameIndex)];

                if (std::memcmp (type, "fdAT", 4) == 0 && chunkLen >= 4)
                {
                    // fdAT: skip 4-byte sequence number, rest is IDAT-equivalent data
                    frame.imageData.insert (frame.imageData.end(),
                                            chunkData + 4,
                                            chunkData + chunkLen);
                }
                else
                {
                    // IDAT (frame 0)
                    frame.imageData.insert (frame.imageData.end(),
                                            chunkData,
                                            chunkData + chunkLen);
                }
            }
        }

        pos += chunkLen + 4; // skip data + CRC
    }

    // If frame 0 has no fcTL before the first IDAT, insert a default fcTL
    if (! frames.empty()
        && frames[0].frameWidth == 0
        && frames[0].frameHeight == 0)
    {
        frames[0].frameWidth = width;
        frames[0].frameHeight = height;
        frames[0].xOffset = 0;
        frames[0].yOffset = 0;
        frames[0].delayMs = 0;
        frames[0].disposeOp = 0;
        frames[0].blendOp = 0;
    }
}

Image PngImageFormatReader::readImage()
{
    if (isApng)
    {
        Image dest;
        readFrame (0, dest);
        return dest;
    }

    // Static image path (unchanged)
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
    if (colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb (pngPtr);

    png_read_update_info (pngPtr, infoPtr);

    auto rowBytes = png_get_rowbytes (pngPtr, infoPtr);
    Image image (width, height, pixelFormat);

    std::vector<png_bytep> rowPointers (static_cast<size_t> (height));
    std::vector<std::vector<png_byte>> rowBuffers (static_cast<size_t> (height),
                                                   std::vector<png_byte> (rowBytes));
    for (int y = 0; y < height; ++y)
        rowPointers[static_cast<size_t> (y)] = rowBuffers[static_cast<size_t> (y)].data();

    png_read_image (pngPtr, rowPointers.data());

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

Image PngImageFormatReader::decodeFrameImage (int frameIndex)
{
    const auto& frame = frames[static_cast<size_t> (frameIndex)];

    if (frame.imageData.empty())
        return {};

    // Build a minimal PNG in memory: signature + IHDR + [PLTE] + [tRNS] + IDAT + IEND
    std::vector<uint8_t> minimalPng;
    minimalPng.reserve (8 + 25 + apngPLTEData.size() + 12 + apngTRNSData.size() + 12
                        + frame.imageData.size() + 12 + 12);

    // PNG signature
    const uint8_t pngSig[8] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n' };
    minimalPng.insert (minimalPng.end(), pngSig, pngSig + 8);

    // IHDR — use the original encoding color type and bit depth
    std::vector<uint8_t> ihdrData;
    buildIHDRData (static_cast<uint32_t> (frame.frameWidth),
                   static_cast<uint32_t> (frame.frameHeight),
                   static_cast<uint8_t> (apngOriginalColorType),
                   ihdrData);
    // Override bit depth in IHDR payload
    ihdrData[8] = static_cast<uint8_t> (apngOriginalBitDepth);
    writeChunk (minimalPng, "IHDR", ihdrData.data(), ihdrData.size());

    // PLTE (for palette-based images)
    if (! apngPLTEData.empty())
        writeChunk (minimalPng, "PLTE", apngPLTEData.data(), apngPLTEData.size());

    // tRNS (for palette transparency)
    if (! apngTRNSData.empty())
        writeChunk (minimalPng, "tRNS", apngTRNSData.data(), apngTRNSData.size());

    // IDAT
    writeChunk (minimalPng, "IDAT", frame.imageData.data(), frame.imageData.size());

    // IEND
    writeChunk (minimalPng, "IEND", nullptr, 0);

    // Decode with libpng
    MemoryReadState readState;
    readState.data = minimalPng.data();
    readState.size = minimalPng.size();
    readState.pos = 0;

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

    png_set_read_fn (pngPtr, &readState, memoryReadCallback);
    png_read_info (pngPtr, infoPtr);

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
    if (colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb (pngPtr);

    png_read_update_info (pngPtr, infoPtr);

    const auto rowBytes = png_get_rowbytes (pngPtr, infoPtr);
    Image image (frame.frameWidth, frame.frameHeight, PixelFormat::RGBA);

    // Single contiguous buffer with row-pointer indirection (avoids per-row allocations)
    std::vector<png_byte> rowBuffer (static_cast<size_t> (frame.frameHeight * rowBytes));
    std::vector<png_bytep> rowPointers (static_cast<size_t> (frame.frameHeight));

    for (int y = 0; y < frame.frameHeight; ++y)
        rowPointers[static_cast<size_t> (y)] = rowBuffer.data() + static_cast<size_t> (y * rowBytes);

    png_read_image (pngPtr, rowPointers.data());

    // Determine bytes per pixel after transforms
    const int bytesPerPixel = static_cast<int> (rowBytes) / frame.frameWidth;

    for (int y = 0; y < frame.frameHeight; ++y)
    {
        const png_byte* row = rowPointers[static_cast<size_t> (y)];

        for (int x = 0; x < frame.frameWidth; ++x)
        {
            const png_byte* p = row + x * bytesPerPixel;

            if (bytesPerPixel == 4)
            {
                image.setPixel (x, y, (uint32 (p[3]) << 24) | (uint32 (p[0]) << 16) | (uint32 (p[1]) << 8) | uint32 (p[2]));
            }
            else if (bytesPerPixel == 3)
            {
                image.setPixel (x, y, 0xFF000000u | (uint32 (p[0]) << 16) | (uint32 (p[1]) << 8) | uint32 (p[2]));
            }
            else if (bytesPerPixel == 2) // grayscale + alpha
            {
                uint32 g = p[0];
                image.setPixel (x, y, (uint32 (p[1]) << 24) | (g << 16) | (g << 8) | g);
            }
            else // grayscale (1 byte)
            {
                uint32 g = p[0];
                image.setPixel (x, y, 0xFF000000u | (g << 16) | (g << 8) | g);
            }
        }
    }

    png_destroy_read_struct (&pngPtr, &infoPtr, nullptr);
    return image;
}

Image PngImageFormatReader::readFrame (int frameIndex)
{
    Image dest;
    readFrame (frameIndex, dest);
    return dest;
}

bool PngImageFormatReader::readFrame (int frameIndex, Image& dest)
{
    if (! isApng || frameIndex < 0 || static_cast<size_t> (frameIndex) >= frames.size())
        return false;

    if (width <= 0 || height <= 0)
        return false;

    // Ensure canvas is valid and correct size
    if (! canvas.isValid() || canvas.getWidth() != width || canvas.getHeight() != height)
    {
        canvas = Image (width, height, PixelFormat::RGBA);
        canvas.fill (0x00000000u);
        lastRenderedFrame = -1;
    }

    // Seek strategy: backward or non-sequential → reset and composite from 0
    if (frameIndex < lastRenderedFrame || frameIndex > lastRenderedFrame + 1)
        resetCanvas();

    const int startFrame = lastRenderedFrame + 1;

    for (int fi = startFrame; fi <= frameIndex; ++fi)
        compositeFrame (fi);

    // Copy composited canvas to dest (reuse allocation when possible)
    const bool compatible = dest.isValid()
                         && dest.getWidth() == width
                         && dest.getHeight() == height
                         && dest.getPixelFormat() == PixelFormat::RGBA;

    if (! compatible)
        dest = Image (width, height, PixelFormat::RGBA);

    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            dest.setPixel (x, y, canvas.getPixel (x, y));

    return true;
}

void PngImageFormatReader::compositeFrame (int frameIndex)
{
    const auto& frame = frames[static_cast<size_t> (frameIndex)];

    // Apply previous frame's disposal method
    if (frameIndex > 0)
    {
        const auto& prevFrame = frames[static_cast<size_t> (frameIndex - 1)];

        if (prevFrame.disposeOp == kApngDisposeBackground)
        {
            // Clear previous frame's rectangle to transparent
            for (int y = prevFrame.yOffset; y < prevFrame.yOffset + prevFrame.frameHeight && y < height; ++y)
                for (int x = prevFrame.xOffset; x < prevFrame.xOffset + prevFrame.frameWidth && x < width; ++x)
                    canvas.setPixel (x, y, 0x00000000u);
        }
        else if (prevFrame.disposeOp == kApngDisposePrevious)
        {
            // Restore to previous canvas state
            if (previousCanvas.isValid())
                canvas = previousCanvas.duplicate();
        }
    }

    // Save current canvas before drawing if this frame uses dispose=previous
    if (frame.disposeOp == kApngDisposePrevious)
        previousCanvas = canvas.duplicate();

    // Decode the frame image
    auto frameImage = decodeFrameImage (frameIndex);
    if (! frameImage.isValid())
        return;

    // Apply to canvas based on blend method
    for (int row = 0; row < frame.frameHeight; ++row)
    {
        const int canvasY = frame.yOffset + row;
        if (canvasY < 0 || canvasY >= height)
            continue;

        for (int col = 0; col < frame.frameWidth; ++col)
        {
            const int canvasX = frame.xOffset + col;
            if (canvasX < 0 || canvasX >= width)
                continue;

            const uint32_t srcPixel = frameImage.getPixel (col, row);
            const uint32_t srcA = (srcPixel >> 24) & 0xFF;

            if (frame.blendOp == kApngBlendSource || srcA == 255)
            {
                canvas.setPixel (canvasX, canvasY, srcPixel);
            }
            else if (srcA == 0)
            {
                continue; // Fully transparent — leave canvas unchanged
            }
            else
            {
                // APNG_BLEND_OP_OVER (alpha blend)
                const uint32_t dstPixel = canvas.getPixel (canvasX, canvasY);
                const uint32_t dstA = (dstPixel >> 24) & 0xFF;
                const uint32_t dstR = (dstPixel >> 16) & 0xFF;
                const uint32_t dstG = (dstPixel >> 8) & 0xFF;
                const uint32_t dstB = dstPixel & 0xFF;

                const uint32_t srcR = (srcPixel >> 16) & 0xFF;
                const uint32_t srcG = (srcPixel >> 8) & 0xFF;
                const uint32_t srcB = srcPixel & 0xFF;

                const uint32_t outA = srcA + ((dstA * (255 - srcA)) / 255);
                const uint32_t outR = (srcR * srcA + dstR * dstA * (255 - srcA) / 255) / outA;
                const uint32_t outG = (srcG * srcA + dstG * dstA * (255 - srcA) / 255) / outA;
                const uint32_t outB = (srcB * srcA + dstB * dstA * (255 - srcA) / 255) / outA;

                canvas.setPixel (canvasX, canvasY, (outA << 24) | (outR << 16) | (outG << 8) | outB);
            }
        }
    }

    lastRenderedFrame = frameIndex;
}

void PngImageFormatReader::resetCanvas()
{
    canvas.fill (0x00000000u);
    previousCanvas = {};
    lastRenderedFrame = -1;
}

bool PngImageFormatReader::isAnimated() const
{
    return isApng && frames.size() > 1;
}

int PngImageFormatReader::getFrameCount() const
{
    return static_cast<int> (frames.size());
}

int PngImageFormatReader::getLoopCount() const
{
    return loopCount;
}

int PngImageFormatReader::getFrameDelayMs (int frameIndex) const
{
    if (frameIndex < 0 || static_cast<size_t> (frameIndex) >= frames.size())
        return 0;
    return frames[static_cast<size_t> (frameIndex)].delayMs;
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

    // Write metadata
    if (auto meta = image.getMetadata())
    {
        // DPI (pHYs)
        if (meta->dpiX > 0.0 || meta->dpiY > 0.0)
        {
            auto resX = static_cast<png_uint_32> (meta->dpiX / 0.0254 + 0.5);
            auto resY = static_cast<png_uint_32> (meta->dpiY / 0.0254 + 0.5);
            png_set_pHYs (pngPtr, infoPtr, resX, resY, PNG_RESOLUTION_METER);
        }

        // Text entries (tEXt chunks)
        if (! meta->textEntries.isEmpty())
        {
            std::vector<png_text> textChunks;
            for (const auto& key : meta->textEntries.getAllKeys())
            {
                auto value = meta->textEntries.getValue (key, {});
                if (value.isEmpty())
                    continue;

                // Skip internal DPI keys (already handled by pHYs)
                if (key == "dpiX" || key == "dpiY")
                    continue;

                png_text text;
                text.compression = PNG_TEXT_COMPRESSION_NONE;
                text.key = const_cast<char*> (key.toRawUTF8());
                text.text = const_cast<char*> (value.toRawUTF8());
                textChunks.push_back (text);
            }

            if (! textChunks.empty())
                png_set_text (pngPtr, infoPtr, textChunks.data(), static_cast<int> (textChunks.size()));
        }

        // iCCP chunk
        if (auto* icc = meta->getRawChunk ("png/iCCP"))
        {
            // iCCP data in rawChunks is the raw data; libpng expects name + compressed profile
            // For simplicity, write as unknown chunk
        }

        // Collect unknown/raw chunks to write
        std::vector<png_unknown_chunk> unknownChunks;
        for (const auto& [key, chunk] : meta->rawChunks)
        {
            if (! key.startsWith ("png/chunk_") && key != "png/eXIf" && key != "png/cHRM")
                continue;

            png_unknown_chunk unk;
            unk.data = const_cast<png_byte*> (static_cast<const png_byte*> (chunk.getData()));
            unk.size = chunk.getSize();

            // Parse chunk name from key
            if (key.startsWith ("png/chunk_") && key.length() >= 14)
                std::memcpy (unk.name, key.toRawUTF8() + 11, 4);
            else if (key == "png/eXIf")
                std::memcpy (unk.name, "eXIf", 4);
            else if (key == "png/cHRM")
                std::memcpy (unk.name, "cHRM", 4);
            else
                continue;

            unk.location = PNG_HAVE_IHDR;
            unknownChunks.push_back (unk);
        }

        if (! unknownChunks.empty())
            png_set_unknown_chunks (pngPtr, infoPtr, unknownChunks.data(), static_cast<int> (unknownChunks.size()));
    }

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
                dst[0] = static_cast<png_byte> ((argb >> 16) & 0xFF);
                dst[1] = static_cast<png_byte> ((argb >> 8) & 0xFF);
                dst[2] = static_cast<png_byte> ((argb >> 0) & 0xFF);
                dst[3] = static_cast<png_byte> ((argb >> 24) & 0xFF);
            }
            else if (channels == 3)
            {
                dst[0] = static_cast<png_byte> ((argb >> 16) & 0xFF);
                dst[1] = static_cast<png_byte> ((argb >> 8) & 0xFF);
                dst[2] = static_cast<png_byte> ((argb >> 0) & 0xFF);
            }
            else
            {
                dst[0] = static_cast<png_byte> ((argb >> 16) & 0xFF);
            }
        }

        png_write_row (pngPtr, rowBuffer.data());
    }

    png_write_end (pngPtr, infoPtr);
    png_destroy_write_struct (&pngPtr, &infoPtr);
    return true;
}

std::vector<uint8_t> PngImageFormatWriter::encodeFrameToPng (const Image& frame)
{
    std::vector<uint8_t> result;
    MemoryWriteState writeState;
    writeState.buffer = &result;

    auto* pngPtr = png_create_write_struct (PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (pngPtr == nullptr)
        return {};

    auto* infoPtr = png_create_info_struct (pngPtr);
    if (infoPtr == nullptr)
    {
        png_destroy_write_struct (&pngPtr, nullptr);
        return {};
    }

    if (setjmp (png_jmpbuf (pngPtr)))
    {
        png_destroy_write_struct (&pngPtr, &infoPtr);
        return {};
    }

    png_set_write_fn (pngPtr, &writeState, memoryWriteCallback, pngFlushCallback);

    // Always encode as RGBA for animation (outer IHDR is RGBA)
    png_set_IHDR (pngPtr, infoPtr, static_cast<png_uint_32> (frame.getWidth()), static_cast<png_uint_32> (frame.getHeight()), 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info (pngPtr, infoPtr);

    const int w = frame.getWidth();
    const int h = frame.getHeight();
    std::vector<png_byte> rowBuffer (static_cast<size_t> (w * 4));

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            auto argb = frame.getPixel (x, y);
            auto* dst = rowBuffer.data() + x * 4;
            dst[0] = static_cast<png_byte> ((argb >> 16) & 0xFF);
            dst[1] = static_cast<png_byte> ((argb >> 8) & 0xFF);
            dst[2] = static_cast<png_byte> ((argb >> 0) & 0xFF);
            dst[3] = static_cast<png_byte> ((argb >> 24) & 0xFF);
        }

        png_write_row (pngPtr, rowBuffer.data());
    }

    png_write_end (pngPtr, infoPtr);
    png_destroy_write_struct (&pngPtr, &infoPtr);
    return result;
}

bool PngImageFormatWriter::beginAnimation (int loopCount)
{
    if (output == nullptr)
        return false;

    bufferedFrames.clear();
    canvasWidth = 0;
    canvasHeight = 0;
    animLoopCount = loopCount;
    return true;
}

bool PngImageFormatWriter::writeFrame (const Image& frame, int delayMs)
{
    if (output == nullptr || ! frame.isValid())
        return false;

    const int w = frame.getWidth();
    const int h = frame.getHeight();

    if (w <= 0 || h <= 0)
        return false;

    // Preserve canvas dimensions from the first frame
    if (canvasWidth == 0)
    {
        canvasWidth = w;
        canvasHeight = h;
    }

    // Encode frame to a complete PNG, then extract the IDAT chunk data
    auto fullPng = encodeFrameToPng (frame);
    if (fullPng.size() < 8 + 25)
        return false;

    // Parse the PNG to extract IDAT data
    // Skip signature (8 bytes)
    size_t pos = 8;

    BufferedFrame buffered;
    buffered.width = w;
    buffered.height = h;
    buffered.delayMs = delayMs;
    buffered.xOffset = 0;
    buffered.yOffset = 0;

    while (pos + 8 <= fullPng.size())
    {
        const auto chunkLen = readBE32 (fullPng.data() + pos);
        pos += 4;

        if (pos + 4 > fullPng.size())
            break;

        const char* type = reinterpret_cast<const char*> (fullPng.data() + pos);
        pos += 4;

        if (pos + chunkLen > fullPng.size())
            break;

        if (std::memcmp (type, "IDAT", 4) == 0)
        {
            buffered.idatData.insert (buffered.idatData.end(),
                                      fullPng.data() + pos,
                                      fullPng.data() + pos + chunkLen);
        }

        pos += chunkLen + 4; // skip data + CRC
    }

    if (buffered.idatData.empty())
        return false;

    bufferedFrames.push_back (std::move (buffered));
    return true;
}

bool PngImageFormatWriter::endAnimation()
{
    if (bufferedFrames.empty())
        return false;

    // Write APNG:
    // 1. PNG signature
    // 2. IHDR (using canvas dimensions)
    // 3. acTL (frame count, loop count)
    // 4. For each frame: fcTL + IDAT (frame 0) or fdAT (frames 1+)
    // 5. IEND

    const uint8_t pngSig[8] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n' };
    output->write (pngSig, 8);

    // IHDR
    std::vector<uint8_t> ihdrData;
    buildIHDRData (static_cast<uint32_t> (canvasWidth),
                   static_cast<uint32_t> (canvasHeight),
                   PNG_COLOR_TYPE_RGBA,
                   ihdrData);
    std::vector<uint8_t> chunkBuf;
    writeChunk (chunkBuf, "IHDR", ihdrData.data(), ihdrData.size());
    output->write (chunkBuf.data(), chunkBuf.size());

    // acTL
    uint8_t actlData[8];
    writeBE32 (static_cast<uint32_t> (bufferedFrames.size()), actlData);
    writeBE32 (static_cast<uint32_t> (animLoopCount), actlData + 4);
    chunkBuf.clear();
    writeChunk (chunkBuf, "acTL", actlData, 8);
    output->write (chunkBuf.data(), chunkBuf.size());

    // Frames — sequence numbers must be globally monotonic and unique
    // Pattern: fcTL=N, fdAT=N+1 (matching reference APNG encoders)
    uint32_t seqNumber = 0;

    for (size_t fi = 0; fi < bufferedFrames.size(); ++fi)
    {
        const auto& bf = bufferedFrames[fi];

        // fcTL
        uint8_t fctlData[26] = {};
        writeBE32 (seqNumber++, fctlData); // sequence_number
        writeBE32 (static_cast<uint32_t> (bf.width), fctlData + 4);
        writeBE32 (static_cast<uint32_t> (bf.height), fctlData + 8);
        writeBE32 (static_cast<uint32_t> (bf.xOffset), fctlData + 12);
        writeBE32 (static_cast<uint32_t> (bf.yOffset), fctlData + 16);

        // delay_num / delay_den: convert ms to rational
        // Clamp delay to uint16_t max (65535 ms), with denominator 1000
        const auto delayMsClamped = std::min (bf.delayMs, 65535);
        const uint16_t delayNum = static_cast<uint16_t> (delayMsClamped);
        const uint16_t delayDen = 1000;
        fctlData[20] = static_cast<uint8_t> ((delayNum >> 8) & 0xFF);
        fctlData[21] = static_cast<uint8_t> (delayNum & 0xFF);
        fctlData[22] = static_cast<uint8_t> ((delayDen >> 8) & 0xFF);
        fctlData[23] = static_cast<uint8_t> (delayDen & 0xFF);
        fctlData[24] = 0; // dispose_op = none
        fctlData[25] = 0; // blend_op = source

        chunkBuf.clear();
        writeChunk (chunkBuf, "fcTL", fctlData, 26);
        output->write (chunkBuf.data(), chunkBuf.size());

        // Frame data: IDAT for frame 0, fdAT for subsequent frames
        if (fi == 0)
        {
            chunkBuf.clear();
            writeChunk (chunkBuf, "IDAT", bf.idatData.data(), bf.idatData.size());
            output->write (chunkBuf.data(), chunkBuf.size());
        }
        else
        {
            // fdAT: 4-byte sequence number (globally monotonic, no duplicate with fcTL)
            std::vector<uint8_t> fdatPayload;
            fdatPayload.reserve (4 + bf.idatData.size());
            uint8_t seqBE[4];
            writeBE32 (seqNumber++, seqBE);
            fdatPayload.insert (fdatPayload.end(), seqBE, seqBE + 4);
            fdatPayload.insert (fdatPayload.end(), bf.idatData.begin(), bf.idatData.end());

            chunkBuf.clear();
            writeChunk (chunkBuf, "fdAT", fdatPayload.data(), fdatPayload.size());
            output->write (chunkBuf.data(), chunkBuf.size());
        }
    }

    // IEND
    chunkBuf.clear();
    writeChunk (chunkBuf, "IEND", nullptr, 0);
    output->write (chunkBuf.data(), chunkBuf.size());

    bufferedFrames.clear();
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

StringArray PngImageFormat::getFileExtensions (Mode /*mode*/) const
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

std::unique_ptr<ImageFormatReader> PngImageFormat::createReaderFor (InputStream* sourceStream, const ImageFormat::Options& options)
{
    return std::make_unique<PngImageFormatReader> (sourceStream, options);
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
