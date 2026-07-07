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

/** Reads a 16-bit unsigned integer in little-endian byte order from the stream. */
static uint16 readLE16 (InputStream& s)
{
    uint8 b[2] = {};
    s.read (b, 2);
    return static_cast<uint16> (b[0] | (static_cast<uint16> (b[1]) << 8));
}

/** Reads a 32-bit unsigned integer in little-endian byte order from the stream. */
static uint32 readLE32 (InputStream& s)
{
    uint8 b[4] = {};
    s.read (b, 4);
    return static_cast<uint32> (b[0])
         | (static_cast<uint32> (b[1]) << 8)
         | (static_cast<uint32> (b[2]) << 16)
         | (static_cast<uint32> (b[3]) << 24);
}

/** Writes a 16-bit unsigned integer in little-endian byte order to the stream. */
static void writeLE16 (OutputStream& s, uint16 v)
{
    uint8 b[2] = { static_cast<uint8> (v), static_cast<uint8> (v >> 8) };
    s.write (b, 2);
}

/** Writes a 32-bit unsigned integer in little-endian byte order to the stream. */
static void writeLE32 (OutputStream& s, uint32 v)
{
    uint8 b[4] = {
        static_cast<uint8> (v),
        static_cast<uint8> (v >> 8),
        static_cast<uint8> (v >> 16),
        static_cast<uint8> (v >> 24)
    };
    s.write (b, 4);
}

} // namespace

//==============================================================================
// BmpImageFormatReader
//==============================================================================

BmpImageFormatReader::BmpImageFormatReader (InputStream* stream)
    : ImageFormatReader (stream, "BMP Image")
{
    if (input == nullptr)
        return;

    // --- BITMAPFILEHEADER (14 bytes) ---

    // Signature: must be "BM"
    const uint8 sig0 = static_cast<uint8> (input->readByte());
    const uint8 sig1 = static_cast<uint8> (input->readByte());

    if (sig0 != 'B' || sig1 != 'M')
        return;

    readLE32 (*input); // fileSize (unused)
    readLE32 (*input); // reserved (skip)

    pixelDataOffset = readLE32 (*input);

    // --- BITMAPINFOHEADER (40 bytes minimum) ---

    const uint32 headerSize = readLE32 (*input);

    if (headerSize < 40)
        return;

    const int32 imageWidth = static_cast<int32> (readLE32 (*input));
    const int32 imageHeight = static_cast<int32> (readLE32 (*input));

    readLE16 (*input); // planes (always 1, ignored)

    bitCount = readLE16 (*input);
    compression = readLE32 (*input);

    readLE32 (*input); // imageSize (may be 0 for BI_RGB)

    const int32 xPelsPerMeter = static_cast<int32> (readLE32 (*input));
    const int32 yPelsPerMeter = static_cast<int32> (readLE32 (*input));

    uint32 colorUsed = readLE32 (*input);
    readLE32 (*input); // colorImportant (unused)

    // For V4/V5 headers, skip extra bytes to reach pixel data offset marker.
    if (headerSize > 40)
        input->setPosition (14 + static_cast<int64> (headerSize));

    // --- Color palette (for indexed images) ---

    if (bitCount <= 8)
    {
        const uint32 numColors = (colorUsed > 0) ? colorUsed : (1u << bitCount);
        palette.resize (numColors);

        for (uint32 i = 0; i < numColors; ++i)
        {
            // Each entry is 4 bytes: B, G, R, reserved
            const uint8 b = static_cast<uint8> (input->readByte());
            const uint8 g = static_cast<uint8> (input->readByte());
            const uint8 r = static_cast<uint8> (input->readByte());
            input->readByte(); // reserved

            palette[i] = 0xff000000u
                       | (static_cast<uint32> (r) << 16)
                       | (static_cast<uint32> (g) << 8)
                       | static_cast<uint32> (b);
        }
    }

    // --- Populate public fields ---

    width = std::abs (imageWidth);
    height = std::abs (imageHeight);
    topDown = (imageHeight < 0);

    pixelFormat = (bitCount == 32) ? PixelFormat::RGBA : PixelFormat::RGB;

    // Convert pixels/meter to DPI (1 inch = 0.0254 metres).
    if (xPelsPerMeter > 0)
        dpiX = static_cast<double> (xPelsPerMeter) * 0.0254;

    if (yPelsPerMeter > 0)
        dpiY = static_cast<double> (yPelsPerMeter) * 0.0254;
}

Image BmpImageFormatReader::readImage()
{
    if (input == nullptr || width <= 0 || height <= 0)
        return {};

    input->setPosition (static_cast<int64> (pixelDataOffset));

    Image image (width, height, pixelFormat);

    // Row stride is padded to a multiple of 4 bytes.
    const int stride = ((width * bitCount + 31) / 32) * 4;

    // BI_RGB (0) and BI_BITFIELDS (3): uncompressed pixel data.
    if (compression == 0 || compression == 3)
    {
        std::vector<uint8> rowBuffer (static_cast<size_t> (stride));

        for (int row = 0; row < height; ++row)
        {
            const int yDest = topDown ? row : (height - 1 - row);

            input->read (rowBuffer.data(), stride);

            switch (bitCount)
            {
                case 24:
                {
                    for (int x = 0; x < width; ++x)
                    {
                        const uint8 b = rowBuffer[static_cast<size_t> (x) * 3 + 0];
                        const uint8 g = rowBuffer[static_cast<size_t> (x) * 3 + 1];
                        const uint8 r = rowBuffer[static_cast<size_t> (x) * 3 + 2];
                        image.setPixel (x, yDest, 0xff000000u | (static_cast<uint32> (r) << 16) | (static_cast<uint32> (g) << 8) | static_cast<uint32> (b));
                    }

                    break;
                }

                case 32:
                {
                    for (int x = 0; x < width; ++x)
                    {
                        const uint8 b = rowBuffer[static_cast<size_t> (x) * 4 + 0];
                        const uint8 g = rowBuffer[static_cast<size_t> (x) * 4 + 1];
                        const uint8 r = rowBuffer[static_cast<size_t> (x) * 4 + 2];
                        const uint8 a = rowBuffer[static_cast<size_t> (x) * 4 + 3];
                        image.setPixel (x, yDest, (static_cast<uint32> (a) << 24) | (static_cast<uint32> (r) << 16) | (static_cast<uint32> (g) << 8) | static_cast<uint32> (b));
                    }

                    break;
                }

                case 16:
                {
                    for (int x = 0; x < width; ++x)
                    {
                        const uint8 lo = rowBuffer[static_cast<size_t> (x) * 2 + 0];
                        const uint8 hi = rowBuffer[static_cast<size_t> (x) * 2 + 1];
                        const uint16 v = static_cast<uint16> (lo | (static_cast<uint16> (hi) << 8));

                        // RGB555: bits [14:10]=R, [9:5]=G, [4:0]=B - scale 5-bit to 8-bit.
                        const uint8 r = static_cast<uint8> (((v >> 10) & 0x1fu) * 255u / 31u);
                        const uint8 g = static_cast<uint8> (((v >> 5) & 0x1fu) * 255u / 31u);
                        const uint8 b = static_cast<uint8> ((v & 0x1fu) * 255u / 31u);

                        image.setPixel (x, yDest, 0xff000000u | (static_cast<uint32> (r) << 16) | (static_cast<uint32> (g) << 8) | static_cast<uint32> (b));
                    }

                    break;
                }

                case 8:
                {
                    for (int x = 0; x < width; ++x)
                    {
                        const uint8 idx = rowBuffer[static_cast<size_t> (x)];

                        if (idx < static_cast<uint8> (palette.size()))
                            image.setPixel (x, yDest, palette[idx]);
                    }

                    break;
                }

                case 4:
                {
                    for (int x = 0; x < width; ++x)
                    {
                        const uint8 byte = rowBuffer[static_cast<size_t> (x) / 2];
                        const uint8 idx = (x % 2 == 0) ? ((byte >> 4) & 0x0fu) : (byte & 0x0fu);

                        if (idx < static_cast<uint8> (palette.size()))
                            image.setPixel (x, yDest, palette[idx]);
                    }

                    break;
                }

                case 1:
                {
                    for (int x = 0; x < width; ++x)
                    {
                        const uint8 byte = rowBuffer[static_cast<size_t> (x) / 8];
                        const uint8 bit = 7u - static_cast<uint8> (x % 8);
                        const uint8 idx = (byte >> bit) & 0x01u;

                        if (idx < static_cast<uint8> (palette.size()))
                            image.setPixel (x, yDest, palette[idx]);
                    }

                    break;
                }

                default:
                    return {};
            }
        }

        return image;
    }

    // BI_RLE8 (1): run-length encoded 8-bit indices.
    if (compression == 1)
    {
        // Decode into a flat index buffer (row-major, top-down).
        std::vector<uint8> indexBuffer (static_cast<size_t> (width) * static_cast<size_t> (height), 0);

        int x = 0;
        int y = 0; // logical row from top

        const auto clampedSet = [&] (int px, int py, uint8 idx)
        {
            if (px < width && py < height)
                indexBuffer[static_cast<size_t> (py) * static_cast<size_t> (width)
                            + static_cast<size_t> (px)] = idx;
        };

        for (;;)
        {
            if (input->isExhausted())
                break;

            const uint8 count = static_cast<uint8> (input->readByte());
            const uint8 value = static_cast<uint8> (input->readByte());

            if (count > 0)
            {
                // Encoded run: repeat value count times.
                for (uint8 i = 0; i < count; ++i)
                    clampedSet (x++, y, value);
            }
            else
            {
                // Escape sequence.
                if (value == 0)
                {
                    // End of line.
                    x = 0;
                    ++y;
                }
                else if (value == 1)
                {
                    // End of bitmap.
                    break;
                }
                else if (value == 2)
                {
                    // Delta: move cursor by (dx, dy).
                    const uint8 dx = static_cast<uint8> (input->readByte());
                    const uint8 dy = static_cast<uint8> (input->readByte());
                    x += dx;
                    y += dy;
                }
                else
                {
                    // Absolute mode: read value bytes literally.
                    for (uint8 i = 0; i < value; ++i)
                        clampedSet (x++, y, static_cast<uint8> (input->readByte()));

                    // Absolute mode is padded to a word boundary.
                    if (value % 2 != 0)
                        input->readByte();
                }
            }
        }

        // Apply palette and set pixels.
        for (int row = 0; row < height; ++row)
        {
            const int yDest = topDown ? row : (height - 1 - row);

            for (int col = 0; col < width; ++col)
            {
                const uint8 idx = indexBuffer[static_cast<size_t> (row) * static_cast<size_t> (width)
                                              + static_cast<size_t> (col)];

                if (idx < static_cast<uint8> (palette.size()))
                    image.setPixel (col, yDest, palette[idx]);
            }
        }

        return image;
    }

    // BI_RLE4 (2): run-length encoded 4-bit indices.
    if (compression == 2)
    {
        // Decode into a flat index buffer (row-major, top-down).
        std::vector<uint8> indexBuffer (static_cast<size_t> (width) * static_cast<size_t> (height), 0);

        int x = 0;
        int y = 0;

        const auto clampedSet = [&] (int px, int py, uint8 idx)
        {
            if (px < width && py < height)
                indexBuffer[static_cast<size_t> (py) * static_cast<size_t> (width)
                            + static_cast<size_t> (px)] = idx;
        };

        for (;;)
        {
            if (input->isExhausted())
                break;

            const uint8 count = static_cast<uint8> (input->readByte());
            const uint8 value = static_cast<uint8> (input->readByte());

            if (count > 0)
            {
                // Encoded run: alternating nibbles from value byte.
                for (uint8 i = 0; i < count; ++i)
                {
                    const uint8 idx = (i % 2 == 0) ? ((value >> 4) & 0x0fu) : (value & 0x0fu);
                    clampedSet (x++, y, idx);
                }
            }
            else
            {
                if (value == 0)
                {
                    // End of line.
                    x = 0;
                    ++y;
                }
                else if (value == 1)
                {
                    // End of bitmap.
                    break;
                }
                else if (value == 2)
                {
                    // Delta.
                    const uint8 dx = static_cast<uint8> (input->readByte());
                    const uint8 dy = static_cast<uint8> (input->readByte());
                    x += dx;
                    y += dy;
                }
                else
                {
                    // Absolute mode: value nibbles from ceil(value/2) bytes.
                    const uint8 byteCount = (value + 1) / 2;

                    for (uint8 i = 0; i < byteCount; ++i)
                    {
                        const uint8 byte = static_cast<uint8> (input->readByte());
                        const uint8 hi = (byte >> 4) & 0x0fu;
                        const uint8 lo = byte & 0x0fu;

                        clampedSet (x++, y, hi);

                        if (x < width && (i * 2 + 1) < value)
                            clampedSet (x++, y, lo);
                    }

                    // Absolute mode is padded to a word boundary (in bytes).
                    if (byteCount % 2 != 0)
                        input->readByte();
                }
            }
        }

        // Apply palette and set pixels.
        for (int row = 0; row < height; ++row)
        {
            const int yDest = topDown ? row : (height - 1 - row);

            for (int col = 0; col < width; ++col)
            {
                const uint8 idx = indexBuffer[static_cast<size_t> (row) * static_cast<size_t> (width)
                                              + static_cast<size_t> (col)];

                if (idx < static_cast<uint8> (palette.size()))
                    image.setPixel (col, yDest, palette[idx]);
            }
        }

        return image;
    }

    return {};
}

//==============================================================================
// BmpImageFormatWriter
//==============================================================================

BmpImageFormatWriter::BmpImageFormatWriter (OutputStream* stream, PixelFormat fmt)
    : ImageFormatWriter (stream, "BMP Image", fmt)
{
}

bool BmpImageFormatWriter::writeImage (const Image& image)
{
    if (output == nullptr || ! image.isValid())
        return false;

    const int w = image.getWidth();
    const int h = image.getHeight();
    const bool isRGBA = (image.getPixelFormat() == PixelFormat::RGBA);
    const uint16 bitCount = isRGBA ? 32u : 24u;

    // Row stride padded to 4-byte boundary.
    const int stride = ((w * bitCount + 31) / 32) * 4;
    const uint32 pixelDataSize = static_cast<uint32> (stride) * static_cast<uint32> (h);
    const uint32 pixelDataOffset = 14u + 40u;
    const uint32 fileSize = pixelDataOffset + pixelDataSize;

    // --- BITMAPFILEHEADER (14 bytes) ---

    output->write ("BM", 2);
    writeLE32 (*output, fileSize);
    writeLE32 (*output, 0u); // reserved
    writeLE32 (*output, pixelDataOffset);

    // --- BITMAPINFOHEADER (40 bytes) ---

    writeLE32 (*output, 40u); // header size
    writeLE32 (*output, static_cast<uint32> (w));
    // Negative height: rows stored top-to-bottom.
    writeLE32 (*output, static_cast<uint32> (-h));
    writeLE16 (*output, 1u); // planes
    writeLE16 (*output, bitCount);
    writeLE32 (*output, 0u); // BI_RGB, no compression
    writeLE32 (*output, pixelDataSize);
    writeLE32 (*output, 2835u); // ~72 DPI in pixels/metre
    writeLE32 (*output, 2835u);
    writeLE32 (*output, 0u); // colorUsed
    writeLE32 (*output, 0u); // colorImportant

    // --- Pixel data (top-to-bottom, left-to-right) ---

    const int paddingBytes = stride - w * (bitCount / 8);

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            const uint32 argb = image.getPixel (x, y);
            const uint8 r = static_cast<uint8> ((argb >> 16) & 0xffu);
            const uint8 g = static_cast<uint8> ((argb >> 8) & 0xffu);
            const uint8 b = static_cast<uint8> (argb & 0xffu);

            if (isRGBA)
            {
                const uint8 a = static_cast<uint8> ((argb >> 24) & 0xffu);
                const uint8 bgra[4] = { b, g, r, a };
                output->write (bgra, 4);
            }
            else
            {
                const uint8 bgr[3] = { b, g, r };
                output->write (bgr, 3);
            }
        }

        // Pad row to 4-byte boundary.
        for (int p = 0; p < paddingBytes; ++p)
        {
            const uint8 zero = 0;
            output->write (&zero, 1);
        }
    }

    return true;
}

//==============================================================================
// BmpImageFormat
//==============================================================================

BmpImageFormat::BmpImageFormat()
    : formatName ("BMP Image")
{
}

const String& BmpImageFormat::getFormatName() const
{
    return formatName;
}

Array<String> BmpImageFormat::getFileExtensions (Mode /*mode*/) const
{
    return { ".bmp" };
}

bool BmpImageFormat::canHandleStream (InputStream& stream, Mode /*mode*/) const
{
    uint8 sig[2] = {};
    stream.read (sig, 2);
    stream.setPosition (0);
    return sig[0] == 0x42 && sig[1] == 0x4D;
}

std::unique_ptr<ImageFormatReader> BmpImageFormat::createReaderFor (InputStream* sourceStream)
{
    return std::make_unique<BmpImageFormatReader> (sourceStream);
}

std::unique_ptr<ImageFormatWriter> BmpImageFormat::createWriterFor (OutputStream* destStream,
                                                                    PixelFormat pixelFormat,
                                                                    const StringPairArray& /*metadataValues*/,
                                                                    int /*qualityOptionIndex*/)
{
    return std::make_unique<BmpImageFormatWriter> (destStream, pixelFormat);
}

Array<PixelFormat> BmpImageFormat::getPossiblePixelFormats() const
{
    return { PixelFormat::RGB, PixelFormat::RGBA };
}

} // namespace yup
