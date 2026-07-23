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

namespace yup
{

//==============================================================================
// Internal helpers (file-scope)
//==============================================================================

namespace
{

/** Expands a 5-bit color channel value to 8 bits. */
static uint8 expand5To8 (uint8 v)
{
    return static_cast<uint8> ((v * 255u + 15u) / 31u);
}

} // namespace

//==============================================================================
// TgaImageFormatReader
//==============================================================================

TgaImageFormatReader::TgaImageFormatReader (InputStream* stream, const ImageFormat::Options& options)
    : ImageFormatReader (stream, "TGA Image", options)
{
    if (input == nullptr)
        return;

    // --- TGA Header (18 bytes) ---

    idLength = static_cast<uint8> (input->readByte());
    colorMapType = static_cast<uint8> (input->readByte());
    imageType = static_cast<uint8> (input->readByte());

    firstEntryIndex = static_cast<uint16> (input->readShort());
    colorMapLength = static_cast<uint16> (input->readShort());
    colorMapEntrySize = static_cast<uint8> (input->readByte());

    xOrigin = static_cast<uint16> (input->readShort());
    yOrigin = static_cast<uint16> (input->readShort());
    imageWidth = static_cast<uint16> (input->readShort());
    imageHeight = static_cast<uint16> (input->readShort());
    pixelDepth = static_cast<uint8> (input->readByte());
    descriptor = static_cast<uint8> (input->readByte());

    // Validate image type
    if (imageType != 1 && imageType != 2 && imageType != 3
        && imageType != 9 && imageType != 10 && imageType != 11)
    {
        return;
    }

    // Validate pixel depth
    if (pixelDepth != 8 && pixelDepth != 16 && pixelDepth != 24 && pixelDepth != 32)
        return;

    // Skip Image ID (if any)
    if (idLength > 0)
        input->skipNextBytes (idLength);

    // Read colour map if present
    if (colorMapType == 1)
    {
        const int bytesPerEntry = (colorMapEntrySize + 7) / 8;
        const int totalBytes = static_cast<int> (colorMapLength) * bytesPerEntry;

        if (totalBytes > 0)
        {
            std::vector<uint8> mapData (static_cast<size_t> (totalBytes));
            input->read (mapData.data(), totalBytes);
            palette.resize (colorMapLength);

            for (uint16 i = 0; i < colorMapLength; ++i)
            {
                const uint8* entry = mapData.data() + static_cast<size_t> (i) * static_cast<size_t> (bytesPerEntry);
                uint32 r = 0, g = 0, b = 0, a = 255;

                if (colorMapEntrySize == 15 || colorMapEntrySize == 16)
                {
                    const uint16 val = ByteOrder::littleEndianShort (entry);
                    r = expand5To8 (static_cast<uint8> ((val >> 10) & 0x1Fu));
                    g = expand5To8 (static_cast<uint8> ((val >> 5) & 0x1Fu));
                    b = expand5To8 (static_cast<uint8> (val & 0x1Fu));
                }
                else if (colorMapEntrySize == 24)
                {
                    // TGA stores palette in BGR order, littleEndian24Bit gives 0x00RRGGBB
                    const uint32 rgb = static_cast<uint32> (ByteOrder::littleEndian24Bit (entry));
                    r = static_cast<uint8> ((rgb >> 16) & 0xFF);
                    g = static_cast<uint8> ((rgb >> 8) & 0xFF);
                    b = static_cast<uint8> (rgb & 0xFF);
                }
                else if (colorMapEntrySize == 32)
                {
                    // BGRA → ARGB via ByteOrder
                    const uint32 argb = ByteOrder::littleEndianInt (entry);
                    palette[i] = argb;
                    continue;
                }

                palette[i] = (static_cast<uint32> (a) << 24)
                           | (static_cast<uint32> (r) << 16)
                           | (static_cast<uint32> (g) << 8)
                           | static_cast<uint32> (b);
            }
        }
    }

    leftRight = (descriptor & 0x10) != 0;
    topDown = (descriptor & 0x20) != 0;

    // Populate public fields
    width = static_cast<int> (imageWidth);
    height = static_cast<int> (imageHeight);

    if (imageType == 1 || imageType == 9)
        pixelFormat = PixelFormat::RGB;
    else if (imageType == 2 || imageType == 10)
        pixelFormat = (pixelDepth == 32) ? PixelFormat::RGBA : PixelFormat::RGB;
    else // grayscale
        pixelFormat = PixelFormat::RGB;
}

Image TgaImageFormatReader::readImage()
{
    if (input == nullptr || width <= 0 || height <= 0)
        return {};

    Image image (width, height, pixelFormat);
    const int bytesPerPixel = (pixelDepth + 7) / 8;
    std::vector<uint8> pixelBuf (static_cast<size_t> (bytesPerPixel));

    const auto convertPixel = [this, bytesPerPixel] (const uint8* data) -> uint32
    {
        if (imageType == 1 || imageType == 9) // color-mapped
        {
            const uint16 idx = (pixelDepth == 16)
                                 ? ByteOrder::littleEndianShort (data)
                                 : data[0];

            if (idx < static_cast<uint16> (palette.size()))
                return palette[idx];

            return 0xFF000000u;
        }

        if (imageType == 2 || imageType == 10) // true-color
        {
            if (pixelDepth == 24)
            {
                // TGA 24-bit BGR → ARGB via ByteOrder
                return 0xFF000000u | static_cast<uint32> (ByteOrder::littleEndian24Bit (data));
            }

            if (pixelDepth == 32)
            {
                // TGA 32-bit BGRA → ARGB: littleEndianInt on BGRA bytes gives ARGB directly
                return ByteOrder::littleEndianInt (data);
            }

            if (pixelDepth == 16)
            {
                const uint16 v = ByteOrder::littleEndianShort (data);
                const uint8 r = expand5To8 (static_cast<uint8> ((v >> 10) & 0x1Fu));
                const uint8 g = expand5To8 (static_cast<uint8> ((v >> 5) & 0x1Fu));
                const uint8 b = expand5To8 (static_cast<uint8> (v & 0x1Fu));

                return 0xFF000000u
                     | (static_cast<uint32> (r) << 16)
                     | (static_cast<uint32> (g) << 8)
                     | static_cast<uint32> (b);
            }
        }
        else // grayscale
        {
            const uint8 gray = data[0];
            return 0xFF000000u
                 | (static_cast<uint32> (gray) << 16)
                 | (static_cast<uint32> (gray) << 8)
                 | static_cast<uint32> (gray);
        }

        return 0xFF000000u;
    };

    if (imageType == 1 || imageType == 2 || imageType == 3)
    {
        // Uncompressed
        for (int y = 0; y < height; ++y)
        {
            const int destY = topDown ? y : (height - 1 - y);

            for (int x = 0; x < width; ++x)
            {
                input->read (pixelBuf.data(), static_cast<size_t> (bytesPerPixel));
                const int destX = leftRight ? (width - 1 - x) : x;
                image.setPixel (destX, destY, convertPixel (pixelBuf.data()));
            }
        }
    }
    else // RLE (9, 10, 11)
    {
        for (int y = 0; y < height; ++y)
        {
            int x = 0;
            const int destY = topDown ? y : (height - 1 - y);

            while (x < width)
            {
                const uint8 header = static_cast<uint8> (input->readByte());
                const uint8 count = (header & 0x7F) + 1;
                const bool isRun = (header & 0x80) != 0;

                if (isRun)
                {
                    input->read (pixelBuf.data(), static_cast<size_t> (bytesPerPixel));
                    const uint32 argb = convertPixel (pixelBuf.data());

                    for (uint8 i = 0; i < count && x < width; ++i)
                    {
                        const int destX = leftRight ? (width - 1 - x) : x;
                        image.setPixel (destX, destY, argb);
                        ++x;
                    }
                }
                else
                {
                    for (uint8 i = 0; i < count && x < width; ++i)
                    {
                        input->read (pixelBuf.data(), static_cast<size_t> (bytesPerPixel));
                        const int destX = leftRight ? (width - 1 - x) : x;
                        image.setPixel (destX, destY, convertPixel (pixelBuf.data()));
                        ++x;
                    }
                }
            }
        }
    }

    return image;
}

//==============================================================================
// TgaImageFormatWriter
//==============================================================================

TgaImageFormatWriter::TgaImageFormatWriter (OutputStream* stream, PixelFormat fmt, bool useRLE)
    : ImageFormatWriter (stream, "TGA Image", fmt)
    , useRLE (useRLE)
{
}

bool TgaImageFormatWriter::writeImage (const Image& image)
{
    if (output == nullptr || ! image.isValid())
        return false;

    const int w = image.getWidth();
    const int h = image.getHeight();
    const bool isRGBA = (image.getPixelFormat() == PixelFormat::RGBA);
    const int bytesPerPixel = isRGBA ? 4 : 3;
    const uint8 pixelDepth = isRGBA ? 32 : 24;
    const uint8 imgType = useRLE ? 10 : 2;
    const uint8 attributeBits = isRGBA ? 8 : 0;
    const uint8 descriptor = 0x20 | (attributeBits & 0x0F); // top-down

    // --- Header (18 bytes) ---

    output->writeByte (0); // ID length
    output->writeByte (0); // Color map type (none)
    output->writeByte (imgType);
    output->writeShort (0); // first entry index
    output->writeShort (0); // color map length
    output->writeByte (0);  // color map entry size
    output->writeShort (0); // X origin
    output->writeShort (0); // Y origin
    output->writeShort (static_cast<short> (w));
    output->writeShort (static_cast<short> (h));
    output->writeByte (pixelDepth);
    output->writeByte (descriptor);

    // Helper to write a single pixel in TGA byte order.
    // On little-endian, ARGB in memory is B,G,R,A — exactly TGA 32-bit BGRA order.
    const auto writePackedPixel = [bytesPerPixel] (OutputStream& s, uint32 argb)
    {
        if (bytesPerPixel == 4)
        {
            s.write (&argb, 4);
        }
        else
        {
            // 24-bit: write the low 3 bytes (B, G, R in LE memory order)
            s.write (&argb, 3);
        }
    };

    // --- Image Data ---

    if (! useRLE)
    {
        std::vector<uint8> rowBuf (static_cast<size_t> (w) * static_cast<size_t> (bytesPerPixel));

        for (int y = 0; y < h; ++y)
        {
            size_t off = 0;

            for (int x = 0; x < w; ++x)
            {
                const uint32 argb = image.getPixel (x, y);
                const uint8* pixel = reinterpret_cast<const uint8*> (&argb);

                if (isRGBA)
                {
                    rowBuf[off++] = pixel[0]; // B (ARGB LE byte 0)
                    rowBuf[off++] = pixel[1]; // G (ARGB LE byte 1)
                    rowBuf[off++] = pixel[2]; // R (ARGB LE byte 2)
                    rowBuf[off++] = pixel[3]; // A (ARGB LE byte 3)
                }
                else
                {
                    rowBuf[off++] = pixel[0]; // B
                    rowBuf[off++] = pixel[1]; // G
                    rowBuf[off++] = pixel[2]; // R
                }
            }

            output->write (rowBuf.data(), rowBuf.size());
        }
    }
    else
    {
        for (int y = 0; y < h; ++y)
        {
            int x = 0;

            while (x < w)
            {
                // Find run length
                int runLen = 1;
                const uint32 first = image.getPixel (x, y);

                while (x + runLen < w && runLen < 128
                       && image.getPixel (x + runLen, y) == first)
                {
                    ++runLen;
                }

                if (runLen > 1)
                {
                    // Run-length packet
                    output->writeByte (static_cast<uint8> (0x80 | (runLen - 1)));
                    writePackedPixel (*output, first);
                    x += runLen;
                }
                else
                {
                    // Raw packet: accumulate different pixels, stop before a run
                    std::vector<uint8> rawData;
                    rawData.reserve (128u * static_cast<size_t> (bytesPerPixel));
                    int count = 0;

                    while (x < w && count < 128)
                    {
                        // Check if the next pixel starts a run (of length >= 2)
                        int nextRun = 1;

                        if (x + 1 < w)
                        {
                            while (x + nextRun < w && nextRun < 128
                                   && image.getPixel (x + nextRun, y) == image.getPixel (x, y))
                            {
                                ++nextRun;
                            }
                        }

                        // If next pixel is a run and we already have raw pixels, stop the raw packet
                        if (nextRun > 1 && count > 0)
                            break;

                        // If we are at the start of a run, stop (will be handled by run branch)
                        if (nextRun > 1 && count == 0)
                            break;

                        // Append pixel to raw packet
                        const uint32 p = image.getPixel (x, y);
                        const uint8* pixel = reinterpret_cast<const uint8*> (&p);

                        if (bytesPerPixel == 4)
                        {
                            rawData.insert (rawData.end(), pixel, pixel + 4);
                        }
                        else
                        {
                            rawData.insert (rawData.end(), pixel, pixel + 3);
                        }

                        ++x;
                        ++count;
                    }

                    output->writeByte (static_cast<uint8> (count - 1));
                    output->write (rawData.data(), rawData.size());
                }
            }
        }
    }

    // --- Footer (26 bytes) ---

    output->writeInt (0);                   // Extension Area Offset
    output->writeInt (0);                   // Developer Directory Offset
    output->write ("TRUEVISION-XFILE", 16); // Signature
    output->writeByte ('.');                // Reserved
    output->writeByte (0);                  // Null terminator

    return true;
}

//==============================================================================
// TgaImageFormat
//==============================================================================

TgaImageFormat::TgaImageFormat()
    : formatName ("TGA Image")
{
}

const String& TgaImageFormat::getFormatName() const
{
    return formatName;
}

StringArray TgaImageFormat::getFileExtensions (Mode /*mode*/) const
{
    return { ".tga", ".icb", ".vda", ".vst" };
}

bool TgaImageFormat::canHandleStream (InputStream& stream, Mode /*mode*/) const
{
    // Read the first 3 bytes: ID length, color map type, image type.
    // The image type must be a valid TGA value (1, 2, 3, 9, 10, 11).
    const uint8 idLen = static_cast<uint8> (stream.readByte());
    const uint8 cmType = static_cast<uint8> (stream.readByte());
    const uint8 imgType = static_cast<uint8> (stream.readByte());
    stream.setPosition (0);

    ignoreUnused (idLen, cmType);

    return (imgType == 1 || imgType == 2 || imgType == 3
            || imgType == 9 || imgType == 10 || imgType == 11);
}

std::unique_ptr<ImageFormatReader> TgaImageFormat::createReaderFor (InputStream* sourceStream, const ImageFormat::Options& options)
{
    return std::make_unique<TgaImageFormatReader> (sourceStream, options);
}

std::unique_ptr<ImageFormatWriter> TgaImageFormat::createWriterFor (OutputStream* destStream,
                                                                    PixelFormat pixelFormat,
                                                                    const StringPairArray& /*metadataValues*/,
                                                                    int qualityOptionIndex)
{
    const bool useRLE = (qualityOptionIndex == 1);
    return std::make_unique<TgaImageFormatWriter> (destStream, pixelFormat, useRLE);
}

Array<PixelFormat> TgaImageFormat::getPossiblePixelFormats() const
{
    return { PixelFormat::RGB, PixelFormat::RGBA };
}

StringArray TgaImageFormat::getQualityOptions() const
{
    return { "Uncompressed", "RLE Compressed" };
}

} // namespace yup
